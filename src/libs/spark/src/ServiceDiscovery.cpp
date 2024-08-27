/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define FLATBUFFERS_TRACK_VERIFIER_BUFFER_SIZE
#include "Core_generated.h"
#include "Multicast_generated.h"
#include <spark/ServiceDiscovery.h>
#include <spark/ServiceListener.h>
#include <logger/Logging.h>
#include <shared/FilterTypes.h>

namespace bai = boost::asio::ip;
namespace mcast = ember::messaging::multicast;

namespace ember::spark::inline v1 {

ServiceDiscovery::ServiceDiscovery(boost::asio::io_context& service, std::string address,
                                   std::uint16_t port, const std::string& mcast_iface,
                                   const std::string& mcast_group, std::uint16_t mcast_port,
                                   log::Logger* logger)
                                   : address_(std::move(address)), port_(port),
                                     service_(service), socket_(service),
                                     endpoint_(bai::address::from_string(mcast_group), mcast_port),
                                     logger_(logger){
	// boost::asio::ip::udp::endpoint listen_endpoint(bai::address::from_string(mcast_iface), mcast_port);

	// socket_.open(listen_endpoint.protocol());
	// socket_.set_option(bai::udp::socket::reuse_address(true));
	// socket_.bind(listen_endpoint);
	
	// socket_.set_option(bai::multicast::join_group(bai::address::from_string(mcast_group)));

	// receive();
}

void ServiceDiscovery::shutdown() {
	LOG_DEBUG_FILTER(logger_, LF_SPARK) << "[spark] Discovery service shutting down..." << LOG_ASYNC;
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
	socket_.close(ec);
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

	auto root = flatbuffers::GetSizePrefixedRoot<messaging::core::Header>(buffer_.data());
	
	if(!root->Verify(verifier)) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Multicast message header failed validation" << LOG_ASYNC;
		return;
	}

	auto body = buffer_.data() + verifier.GetComputedSize();
	
	switch(static_cast<mcast::Opcode>(root->opcode())) {
		case mcast::Opcode::CMSG_LOCATE:
			handle_locate(flatbuffers::GetRoot<mcast::Locate>(body));
			break;
		case mcast::Opcode::SMSG_LOCATE:
			handle_locate_answer(flatbuffers::GetRoot<mcast::LocateResponse>(body));
			break;
		default:
			LOG_WARN_FILTER(logger_, LF_SPARK)
				<< "[spark] Received an unknown multicast packet type from "
				<< remote_ep_.address().to_string() << LOG_ASYNC;
	}
}

void ServiceDiscovery::send(const std::shared_ptr<flatbuffers::FlatBufferBuilder>& msg,
							mcast::Opcode opcode) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto header = messaging::core::CreateHeader(*fbb, static_cast<std::uint16_t>(opcode),
	                                            messaging::Service::CORE_DISCOVERY);
	fbb->FinishSizePrefixed(header);

	std::vector<boost::asio::const_buffer> buffers {
		{ fbb->GetBufferPointer(), fbb->GetSize() },
		{ msg->GetBufferPointer(), msg->GetSize() }
	};

	socket_.async_send_to(buffers, endpoint_,
		[this, msg, fbb](const boost::system::error_code& ec, std::size_t size) {
			if(ec) {
				LOG_ERROR_FILTER(logger_, LF_SPARK)
					<< "[spark] Error on sending service discovery packet: "
					<< ec.message() << ", size " << size << LOG_ASYNC;
			}
		}
	);
}

void ServiceDiscovery::remove_listener(const ServiceListener* listener) {
	std::lock_guard<std::mutex> guard(lock_);
	auto& vec = listeners_[listener->service()];
	vec.erase(std::remove(vec.begin(), vec.end(), listener), vec.end());
}

std::unique_ptr<ServiceListener> ServiceDiscovery::listener(messaging::Service service,
                                                            LocateCallback cb) {
	auto listener = std::make_unique<ServiceListener>(this, service, cb);
	std::lock_guard<std::mutex> guard(lock_);
	listeners_[service].emplace_back(listener.get());
	return listener;
}

void ServiceDiscovery::locate_service(messaging::Service service) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto msg = mcast::CreateLocate(*fbb, service);
	fbb->FinishSizePrefixed(msg);
	send(fbb, mcast::Opcode::CMSG_LOCATE);
}

void ServiceDiscovery::send_announce(messaging::Service service) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto ip = fbb->CreateString(address_);
	auto msg = 	mcast::CreateLocateResponse(*fbb, ip, port_, service);
	fbb->FinishSizePrefixed(msg);
	send(fbb, mcast::Opcode::SMSG_LOCATE);
}

void ServiceDiscovery::handle_locate(const mcast::Locate* message) {
	// check to see if we a matching service registered
	std::lock_guard<std::mutex> guard(lock_);
	
	auto it = std::find(services_.begin(), services_.end(), message->service());

	if(it == services_.end()) {
		return; // no matching services
	}

	send_announce(message->service());
}

void ServiceDiscovery::handle_locate_answer(const mcast::LocateResponse* message) {
	if(message->type() != messaging::Service::CORE_DISCOVERY
	   || !message->ip() || !message->port()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Received incompatible locate answer " << LOG_ASYNC;
		return;
	}

	std::lock_guard<std::mutex> guard(lock_);
	auto& listeners = listeners_[message->type()];

	for(auto& listener : listeners) {
		listener->cb_(message);
	}
}

void ServiceDiscovery::register_service(messaging::Service service) {
	std::lock_guard<std::mutex> guard(lock_);
	services_.emplace_back(service);
	send_announce(service);
}

void ServiceDiscovery::remove_service(messaging::Service service) {
	std::lock_guard<std::mutex> guard(lock_);
	services_.erase(std::remove(services_.begin(), services_.end(), service), services_.end());
}

} // spark, ember