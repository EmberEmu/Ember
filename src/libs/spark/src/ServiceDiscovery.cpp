/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/ServiceDiscovery.h>

namespace bai = boost::asio::ip;
using namespace std::chrono_literals;

namespace ember { namespace spark {

ServiceDiscovery::ServiceDiscovery(std::string desired_hostname, boost::asio::io_service& service,
                                   bai::address interface, bai::address mcast_address, std::uint16_t port,
                                   log::Logger* logger, log::Filter filter)
                                   : hostname_(std::move(desired_hostname)), socket_(service),
                                     logger_(logger), filter_(filter), service_(service) {
	boost::asio::ip::udp::endpoint listen_endpoint(interface, port);

	socket_.open(listen_endpoint.protocol());
	socket_.set_option(bai::udp::socket::reuse_address(true));
	socket_.bind(listen_endpoint);

	socket_.set_option(bai::multicast::join_group(mcast_address));
	receive();
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

void ServiceDiscovery::receive() {
	socket_.async_receive_from(buffer_, endpoint_,
		std::bind(&ServiceDiscovery::handle_receive, this, std::placeholders::_1, std::placeholders::_2)
	);
}

void ServiceDiscovery::handle_packet(std::size_t size) {
	flatbuffers::Verifier verifier(buffer_.data(), size);

	if(!multicast::VerifyMessageRootBuffer(verifier)) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Multicast message failed validation" << LOG_ASYNC;
		return;
	}

	auto message = multicast::GetMessageRoot(buffer_.data());
}

void ServiceDiscovery::send(std::shared_ptr<flatbuffers::FlatBufferBuilder> fbb) {
	socket_.async_send_to(boost::asio::buffer(fbb->GetBufferPointer(), fbb->GetSize()), endpoint_,
		[this, fbb](boost::system::error_code& ec) {
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

void ServiceDiscovery::resolve_host(const std::string& host, ResolveCallback cb) {

}

void ServiceDiscovery::register_service(messaging::Service service) {
	std::lock_guard<std::mutex> guard(lock_);

	auto it = std::find(services_.begin(), services_.end(), service);

	if(it != services_.end()) {
		throw std::runtime_error("Attempted to register a duplicate service");
	}

	services_.emplace_back(service);

	auto timer = std::make_shared<Timer>(service_);
	unannounced_timer_tick(timer, service, 0);
}

void ServiceDiscovery::unannounced_timer_tick(std::shared_ptr<Timer> timer, messaging::Service service,
                                              std::uint8_t ticks) {
	std::chrono::milliseconds jitter(1); // todo
	timer->expires_from_now(1s + jitter);
	timer->async_wait(std::bind(&ServiceDiscovery::unsolicited_announce, this,
								std::placeholders::_1, service, timer, ticks));
}

std::string ServiceDiscovery::hostname() const {
	std::lock_guard<std::mutex> guard(lock_);
	return hostname_;
}

void ServiceDiscovery::unsolicited_announce(const boost::system::error_code& ec, std::shared_ptr<Timer> timer,
                                            messaging::Service service, std::uint8_t ticks) {
	if(ec || ticks >= 2) {
		return;
	}

	announce(service);
	unannounced_timer_tick(timer, service, ticks);
}

void ServiceDiscovery::announce(messaging::Service service) {

}

}} // multicast, spark, ember