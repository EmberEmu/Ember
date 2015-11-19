#pragma once

#include <spark/Spark.h>
#include <logger/Logging.h>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <functional>

namespace ember {

namespace sc = std::chrono;

class TestService {
	spark::Service& service_;
	log::Logger* logger_;

public:
	TestService(spark::Service& service, log::Logger* logger) : service_(service), logger_(logger) { 
		service.handlers()->register_handler(
			std::bind(&TestService::handle_message, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&TestService::handle_link_event, this, std::placeholders::_1, std::placeholders::_2),
			messaging::Service_Keystone, spark::HandlerMap::Mode::SERVER
		);
	}

	void test_tracking(const spark::Link& link) {
		LOG_INFO(logger_) << "Sent tracked" << LOG_ASYNC;
		boost::uuids::uuid id = boost::uuids::random_generator()();
		auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
		auto uuid = fbb->CreateVector(id.begin(), id.size());
		auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Service_Keystone, uuid, 0,
												messaging::Data::Data_Ping, messaging::CreatePing(*fbb, 5).Union());
		fbb->Finish(msg);

		service_.send_tracked(link, id, fbb,
			std::bind(&TestService::tracking_cb, this, link, std::placeholders::_1));
		
	}

	void tracking_cb(const spark::Link& link, boost::optional<const messaging::MessageRoot*> msg) {
		if(msg) {
			LOG_INFO(logger_) << "Got tracked reply" << LOG_ASYNC;
		} else {
			LOG_INFO(logger_) << "Timed out" << LOG_ASYNC;
		}
	}

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
		LOG_INFO(logger_) << "Received a message!" << LOG_ASYNC;
	}

	void handle_link_event(const spark::Link& link, spark::LinkState event) {
		if(event == spark::LinkState::LINK_UP) {
			test_tracking(link);
			LOG_INFO(logger_) << "Link to client up!" << LOG_ASYNC;
		} else {
			LOG_INFO(logger_) << "Link to client down!" << LOG_ASYNC;
		}
	}
};

}