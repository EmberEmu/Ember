/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/ServiceDiscovery.h>
#include <spark/temp/Multicast_generated.h>
#include <boost/lexical_cast.hpp>

namespace bai = boost::asio::ip;
namespace mcast = ember::messaging::multicast;

namespace ember { namespace spark {

ServiceDiscovery::ServiceDiscovery(std::string desired_hostname, boost::asio::io_service& service,
                                   bai::address interface, std::uint16_t port,           // Spark TCP details
                                   bai::address mcast_address, std::uint16_t mcast_port, // Spark UDP multicast details
                                   log::Logger* logger, log::Filter filter)
                                   : hostname_(std::move(desired_hostname)), socket_(service),
                                     logger_(logger), filter_(filter), service_(service) {
	boost::asio::ip::udp::endpoint listen_endpoint(interface, mcast_port);

	socket_.open(listen_endpoint.protocol());
	socket_.set_option(bai::udp::socket::reuse_address(true));
	socket_.bind(listen_endpoint);

	socket_.set_option(bai::multicast::join_group(mcast_address));
	receive();
}

void ServiceDiscovery::receive() {
	socket_.async_receive_from(boost::asio::buffer(buffer_.data(), buffer_.size()), remote_ep_,
		std::bind(&ServiceDiscovery::handle_receive, this,
		          std::placeholders::_1, std::placeholders::_2)
	);
}

void ServiceDiscovery::handle_receive(const boost::system::error_code& ec, std::size_t size) {
	if(ec && ec == boost::asio::error::operation_aborted) {
		return;
	}

	if(!ec) {
		handle_packet(size);
	}

	receive();
}

void ServiceDiscovery::handle_packet(std::size_t size) {
	flatbuffers::Verifier verifier(buffer_.data(), size);

	if(!mcast::VerifyMessageRootBuffer(verifier)) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Multicast message failed validation" << LOG_ASYNC;
		return;
	}

	auto message = mcast::GetMessageRoot(buffer_.data());

	switch(message->data_type()) {
		case mcast::Data::Data_Locate:
			handle_locate(static_cast<const mcast::Locate*>(message->data()));
			break;
		case mcast::Data_LocateAnswer:
			handle_locate_answer(static_cast<const mcast::LocateAnswer*>(message->data()));
			break;
		default:
			LOG_WARN_FILTER(logger_, filter_)
				<< "[spark] Received an unknown multicast packet type from "
				<< boost::lexical_cast<std::string>(remote_ep_.address()) << LOG_ASYNC;
	}
}

void ServiceDiscovery::send(std::shared_ptr<flatbuffers::FlatBufferBuilder> fbb) {
	socket_.async_send_to(boost::asio::buffer(fbb->GetBufferPointer(), fbb->GetSize()), endpoint_,
		[this, fbb](const boost::system::error_code& ec, std::size_t /*size*/) {
			if(ec) {
				LOG_WARN_FILTER(logger_, filter_)
					<< "[spark] Error on sending service discovery packet: "
					<< ec.message() << LOG_ASYNC;
			}
		}
	);
}

void ServiceDiscovery::locate_service(messaging::Service service, LocateCallback cb) {

}

void ServiceDiscovery::register_service(messaging::Service service) {
	std::lock_guard<std::mutex> guard(lock_);

	auto it = std::find(services_.begin(), services_.end(), service);

	if(it != services_.end()) {
		throw std::runtime_error("Attempted to register a duplicate service");
	}

	services_.emplace_back(service);

	auto timer = std::make_shared<Timer>(service_);
	unannounced_timer_set(timer, service, 0);
}

void ServiceDiscovery::unannounced_timer_set(std::shared_ptr<Timer> timer, messaging::Service service,
                                             std::uint8_t ticks) {
	std::chrono::milliseconds jitter(1); // todo
	timer->expires_from_now(ANNOUNCE_REPEAT_DELAY + jitter);
	timer->async_wait(std::bind(&ServiceDiscovery::unsolicited_announce, this,
								std::placeholders::_1, timer, service, ticks));
}

void ServiceDiscovery::unsolicited_announce(const boost::system::error_code& ec, std::shared_ptr<Timer> timer,
                                            messaging::Service service, std::uint8_t ticks) {
	if(ec || ticks >= 2) {
		return;
	}

	send_announce(service);
	unannounced_timer_set(timer, service, ++ticks);
}

void ServiceDiscovery::send_announce(messaging::Service service) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto msg = mcast::CreateMessageRoot(*fbb, mcast::Data::Data_Locate,
		mcast::CreateLocate(*fbb, service).Union());
	fbb->Finish(msg);
	send(fbb);
}

void ServiceDiscovery::handle_locate(const mcast::Locate* message) {
	auto it = std::find(services_.begin(), services_.end(), message->type());

	if(it == services_.end()) {
		return; // we have no services of interest to the querier
	}

	// check to see whether querier is already aware of us
	auto kh = message->known_hosts();

	if(kh) {
		auto kh_it = std::find_if(kh->begin(), kh->end(), [&](const auto& host) {
			return host->c_str() == this->hostname_; // this-> is a GCC bug workaround
		});

		if(kh_it == kh->end()) {
			return; // the querier already knows about us, don't bother broadcasting
		}
	}

	send_announce(message->type());
}

void ServiceDiscovery::handle_locate_answer(const mcast::LocateAnswer* message) {

}

}} // multicast, spark, ember