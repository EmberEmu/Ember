/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Service.h>
#include <spark/MessageHandler.h>
#include <spark/NetworkSession.h>
#include <spark/Listener.h>
#include <boost/uuid/uuid_generators.hpp>
#include <functional>

namespace ember { namespace spark {

namespace bai = boost::asio::ip;

Service::Service(std::string description, boost::asio::io_service& service, const std::string& interface,
                 std::uint16_t port, log::Logger* logger, log::Filter filter)
                 : service_(service), logger_(logger), filter_(filter), signals_(service, SIGINT, SIGTERM),
                   listener_(service, interface, port, sessions_, handlers_, link_, logger, filter),
                   core_handler_(logger, filter), socket_(service),
	               link_ { boost::uuids::random_generator()(), std::move(description) },
                   handlers_(std::bind(&Service::default_handler, this,
                             std::placeholders::_1, std::placeholders::_2)) {
	signals_.async_wait(std::bind(&Service::shutdown, this));

	handlers_.register_handler(
		std::bind(&CoreHandler::handle_message, &core_handler_, std::placeholders::_1, std::placeholders::_2),
		std::bind(&CoreHandler::handle_event, &core_handler_, std::placeholders::_1, std::placeholders::_2),
		messaging::Service::Service_Core
	);
}

void Service::shutdown() {
	LOG_DEBUG_FILTER(logger_, filter_) << "[spark] Service shutting down..." << LOG_ASYNC;
	listener_.shutdown();
	sessions_.stop_all();
}

void Service::start_session(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	MessageHandler m_handler(handlers_, link_, true, logger_, filter_);
	auto session = std::make_shared<NetworkSession>(sessions_, std::move(socket),
                                                    m_handler, logger_, filter_);
	sessions_.start(session);
}

void Service::connect(const std::string& host, std::uint16_t port) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	bai::tcp::resolver resolver(service_);
	auto endpoint_it = resolver.resolve({ host, std::to_string(port) });

	boost::asio::async_connect(socket_, endpoint_it,
		[this, host, port](boost::system::error_code ec, bai::tcp::resolver::iterator it) {
			if(!ec) {
				start_session(std::move(socket_)); 
			}

			LOG_DEBUG_FILTER(logger_, filter_)
				<< "[spark] " << (ec? "Unable to establish" : "Established")
				<< " connection to " << host << ":" << port << LOG_ASYNC;
		}
	);
}

void Service::default_link_state_handler(const Link& link, LinkState state) {
	LOG_DEBUG_FILTER(logger_, filter_) << "[spark] Unhandled link event! " << LOG_ASYNC;
}

void Service::default_handler(const Link& link, const messaging::MessageRoot* message) {
	LOG_DEBUG_FILTER(logger_, filter_) << "[spark] Unhandled message of type "
		<< messaging::EnumNameData(message->data_type()) << LOG_ASYNC;
}

void Service::send(const Link& link, messaging::MessageRoot* message) {

}

void Service::send_tracked(const Link& link, messaging::MessageRoot* message, MsgHandler callback) {

}

void Service::broadcast(messaging::Service service, messaging::MessageRoot* message) {

}

HandlerMap* Service::handlers() {
	return &handlers_;
}

}} // spark, ember