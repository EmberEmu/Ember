#pragma once

#include <spark/Spark.h>
#include <logger/Logging.h>
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

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
		LOG_INFO(logger_) << "Received a message!" << LOG_ASYNC;
	}

	void handle_link_event(const spark::Link& link, spark::LinkState event) {
		if(event == spark::LinkState::LINK_UP) {
			LOG_INFO(logger_) << "Link to client up!" << LOG_ASYNC;
		} else {
			LOG_INFO(logger_) << "Link to client down!" << LOG_ASYNC;
		}

		auto time = sc::duration_cast<sc::milliseconds>(sc::steady_clock::now().time_since_epoch()).count();
		auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
		auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Service_Core, 0,
			messaging::Data::Data_Ping, messaging::CreatePing(*fbb, time).Union());
		fbb->Finish(msg);
		service_.send(link, fbb);
	}
};

}