/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/temp/ServiceTypes_generated.h>
#include <logger/Logging.h>
#include <flatbuffers/flatbuffers.h>
#include <boost/asio.hpp>
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace ember { namespace spark {

struct Endpoint;

typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> Timer;
typedef std::function<void(const Endpoint*)> ResolveCallback;
typedef std::function<void(std::string)> LocateCallback;

class ServiceDiscovery {
	static const std::size_t BUFFER_SIZE = 1024;
	//static const std::chrono::milliseconds UNSOLICITED_ANNOUNCE_DELAY { 1ms };

	struct Endpoint {
		boost::asio::deadline_timer ttl_timer;
		std::string address;
		std::uint16_t port;
	};

	std::string hostname_;
	boost::asio::io_service& service_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint endpoint_, remote_ep_;
	std::array<std::uint8_t, BUFFER_SIZE> buffer_;
	std::unordered_map<std::string, std::unique_ptr<Endpoint>> cache_;
	std::vector<messaging::Service> services_;
	mutable std::mutex lock_;
	log::Logger* logger_;
	log::Filter filter_;

	void receive();
	void handle_packet(std::size_t size);
	void send(std::shared_ptr<flatbuffers::FlatBufferBuilder> fbb);
	void handle_receive(const boost::system::error_code& ec, std::size_t size);
	void announce(messaging::Service service);
	void unannounced_timer_tick(std::shared_ptr<Timer> timer, messaging::Service service, std::uint8_t ticks);
	void unsolicited_announce(const boost::system::error_code& ec, std::shared_ptr<Timer> timer,
	                          messaging::Service service, std::uint8_t count);

public:
	ServiceDiscovery(std::string desired_hostname, boost::asio::io_service& service,
	                 boost::asio::ip::address interface, boost::asio::ip::address mcast_group,
	                 std::uint16_t port, log::Logger* logger, log::Filter filter);

	void locate_service(messaging::Service service, LocateCallback cb);
	void resolve_host(const std::string& host, ResolveCallback cb);
	void register_service(messaging::Service);
	std::string hostname() const;
};


}} // multicast, spark, ember