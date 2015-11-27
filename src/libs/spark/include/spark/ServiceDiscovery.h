/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/ServiceListener.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <spark/temp/Multicast_generated.h>
#include <logger/Logging.h>
#include <flatbuffers/flatbuffers.h>
#include <boost/asio.hpp>
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember { namespace spark {

class ServiceDiscovery {
	static const std::size_t BUFFER_SIZE = 1024;

	std::string address_;
	std::uint16_t port_;
	boost::asio::io_service& service_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint endpoint_, remote_ep_;
	std::array<std::uint8_t, BUFFER_SIZE> buffer_;
	std::vector<messaging::Service> services_;
	std::unordered_map<messaging::Service, std::vector<const ServiceListener*>> listeners_;
	mutable std::mutex lock_;

	log::Logger* logger_;
	log::Filter filter_;

	void remove_listener(const ServiceListener* listener);

	// incoming packet handlers
	void receive();
	void handle_packet(std::size_t size);
	void handle_locate(const messaging::multicast::Locate* message);
	void handle_locate_answer(const messaging::multicast::LocateAnswer* message);

	// packet senders
	void send(std::shared_ptr<flatbuffers::FlatBufferBuilder> fbb);
	void send_announce(messaging::Service service);

	void locate_service(messaging::Service);
	void handle_receive(const boost::system::error_code& ec, std::size_t size);

public:
	ServiceDiscovery(boost::asio::io_service& service,
	                 std::string address, std::uint16_t port, 
					 const std::string& mcast_iface, const std::string& mcast_group,
	                 std::uint16_t mcast_port, log::Logger* logger, log::Filter filter);

	void register_service(messaging::Service service);
	void remove_service(messaging::Service service);
	std::unique_ptr<ServiceListener> listener(messaging::Service service, LocateCallback cb);

	friend class ServiceListener;
};


}} // multicast, spark, ember