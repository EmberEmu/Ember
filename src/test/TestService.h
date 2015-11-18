#pragma once

#include <spark/Spark.h>
#include <logger/Logging.h>
#include <functional>

namespace ember {

class TestService {
	spark::Service& service_;
	log::Logger* logger_;

public:
	TestService(spark::Service& service, log::Logger* logger) : service_(service), logger_(logger) { 
		service.handlers()->register_handler(
			std::bind(&TestService::handle_message, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&TestService::handle_link_event, this, std::placeholders::_1, std::placeholders::_2),
			messaging::Service_Keystone, spark::HandlerMap::Mode::CLIENT
		);
	}

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) {
		LOG_INFO(logger_) << "Received a message!" << LOG_ASYNC;
	}

	void handle_link_event(const spark::Link& link, spark::LinkState event) {
		if(event == spark::LinkState::LINK_UP) {
			LOG_INFO(logger_) << "Link to server up!" << LOG_ASYNC;
		} else {
			LOG_INFO(logger_) << "Link to server down!" << LOG_ASYNC;
		}
	}
};

}