/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/metrics/Monitor.h>
#include <shared/metrics/Metrics.h>
#include <memory>
#include <sstream>

namespace ember {

using namespace std::chrono_literals;
namespace bai = boost::asio::ip;

Monitor::Monitor(boost::asio::io_service& service, const std::string& interface,
                 std::uint16_t port, Metrics& metrics, std::chrono::seconds frequency)
                 : timer_(service), strand_(service), metrics_(metrics), TIMER_FREQUENCY(frequency),
                   socket_(service, bai::udp::endpoint(bai::address::from_string(interface), port)),
                   signals_(service, SIGINT, SIGTERM) {
	signals_.async_wait(std::bind(&Monitor::shutdown, this));
	set_timer();
	receive();
}

void Monitor::shutdown() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
	timer_.cancel(ec);;
}

void Monitor::receive() {
	socket_.async_receive_from(boost::asio::buffer(buffer_), endpoint_, 
		strand_.wrap([this](const boost::system::error_code& ec, std::size_t) {
			if(!ec || ec == boost::asio::error::message_size
				|| ec.value() == 0xEA) { // todo, ASIO bug workaround, patch at: https://github.com/boostorg/asio/pull/27
				send_health_status();
				receive();
			}
	}));
}

void Monitor::send_health_status() {
	auto message = std::make_shared<std::string>(generate_message());

	socket_.async_send_to(boost::asio::buffer(*message), endpoint_,
		strand_.wrap([message](const boost::system::error_code&, std::size_t) { }));
}

void Monitor::add_source(Source source, Severity severity, LogCallback log_callback) {
	source.triggered = false;
	std::lock_guard<std::mutex> guard(source_lock_);
	sources_.emplace_back(source, severity, log_callback, 0s);
}

void Monitor::timer_tick(const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	std::lock_guard<std::mutex> guard(source_lock_);

	for(auto& source : sources_) {
		execute_source(std::get<0>(source), std::get<1>(source),
		               std::get<2>(source), std::get<3>(source));
	}

	set_timer();
}

void Monitor::execute_source(Source& source, Severity severity, LogCallback& log,
                             std::chrono::seconds& last_tick) {
	last_tick += TIMER_FREQUENCY;

	if(last_tick < source.frequency) {
		return; // too soon!
	} else {
		last_tick = 0s;
	}

	auto value = source.callback();
	bool trigger = source.comparator(value, source.threshold);

	if(trigger && !source.triggered) { // Uh, it's probably not a problem, probably, but I'm showing a small discrepancy in...
		source.triggered = true;
		log(source, severity, value);
		++counters_[severity];
	} else if(!trigger && source.triggered) { // Well, no, it's well within acceptable bounds again.
		source.triggered = false;
		log(source, Severity::INFO, value);
		--counters_[severity];
	}

	metrics_.gauge(source.key.c_str(), value);
}

std::string Monitor::generate_message() {
	std::lock_guard<std::mutex> guard(source_lock_);
	std::stringstream message;

	message << "Status: ";

	if(counters_[Severity::FATAL] || counters_[Severity::ERROR]) {
		message << "ERROR; ";
	} else if(counters_[Severity::WARN]) {
		message << "WARNING; ";
	} else {
		message << "OK; ";
	}

	for(auto& source : sources_) {
		if(std::get<0>(source).triggered) {
			message << std::get<0>(source).message << ";";
		}
	}

	return message.str();
}

void Monitor::set_timer() {
	timer_.expires_from_now(TIMER_FREQUENCY);
	timer_.async_wait(std::bind(&Monitor::timer_tick, this, std::placeholders::_1));
}

} // ember