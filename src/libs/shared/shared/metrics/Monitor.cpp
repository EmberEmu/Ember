/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/metrics/Monitor.h>
#include <boost/asio/strand.hpp>
#include <memory>
#include <sstream>

namespace ember {

using namespace std::chrono_literals;
namespace bai = boost::asio::ip;

Monitor::Monitor(boost::asio::io_context& service,
                 std::string_view interface,
                 std::uint16_t port,
                 std::chrono::seconds frequency)
	: timer_(service)
	, timer_frequency(frequency)
	, socket_(boost::asio::make_strand(service), bai::udp::endpoint(bai::make_address(interface), port)) {
	set_timer();
	receive();
}

void Monitor::shutdown() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
	socket_.close(ec);

	try {
		timer_.cancel();
	} catch(const boost::system::system_error&) {
		// swallow exception, Asio removed the error code overload
	}
}

void Monitor::receive() {
	socket_.async_wait(boost::asio::ip::udp::socket::wait_read,
		[&](const boost::system::error_code& ec) {
			if(ec) {
				return;
			}

			// ignore all of the data
			std::vector<char> buffer(socket_.available());
			socket_.receive_from(boost::asio::buffer(buffer), endpoint_);

			if(!ec || ec == boost::asio::error::message_size) {
				send_health_status();
				receive();
			}
		}
	);
}

void Monitor::send_health_status() {
	auto message = std::make_shared<std::string>(generate_message());

	socket_.async_send_to(boost::asio::buffer(*message), endpoint_, 
		[message](const boost::system::error_code&, std::size_t) { });
}

void Monitor::add_source(Source source, Severity severity, LogCallback log_callback) {
	source.triggered = false;
	std::lock_guard guard(source_lock_);
	sources_.emplace_back(source, severity, log_callback, 0s);
}

void Monitor::timer_tick(const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	std::lock_guard guard(source_lock_);

	for(auto& source : sources_) {
		execute_source(std::get<0>(source), std::get<1>(source),
		               std::get<2>(source), std::get<3>(source));
	}

	set_timer();
}

void Monitor::execute_source(Source& source, Severity severity, const LogCallback& log,
                             std::chrono::seconds& last_tick) {
	last_tick += timer_frequency;

	if(last_tick < source.frequency) {
		return; // too soon!
	} else {
		last_tick = 0s;
	}

	auto value = source.callback();
	bool trigger = source.comparator(value, source.threshold);

	// Uh, it's probably not a problem, probably, but I'm showing a small discrepancy in...
	if(trigger && !source.triggered) {
		source.triggered = true;
		log(source, severity, value);
		++counters_[severity];
	} else if(!trigger && source.triggered) { // Well, no, it's well within acceptable bounds again.
		source.triggered = false;
		log(source, Severity::info, value);
		--counters_[severity];
	}
}

bool Monitor::has_severity(const Severity sev) const {
	auto it = counters_.find(sev);
	return it != counters_.end() && it->second;
}

std::string Monitor::generate_message() const {
	std::lock_guard guard(source_lock_);
	std::stringstream message;

	message << "Status: ";

	if(has_severity(Severity::fatal) || has_severity(Severity::error)) {
		message << "ERROR; ";
	} else if(has_severity(Severity::warn)) {
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
	timer_.expires_after(timer_frequency);
	timer_.async_wait([&](auto ec) {
		timer_tick(ec);
	});
}

} // ember