/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/pcp/Daemon.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <algorithm>
#include <random>

namespace ember::ports {

Daemon::Daemon(Client& client, boost::asio::io_context& ctx, EventHandler&& handler)
	: client_(client),
	  strand_(boost::asio::make_strand(ctx)),
	  timer_(strand_),
	  daemon_callback_(handler),
	  consecutive_failures_(0) {
	client_.announce_handler(boost::asio::bind_executor(strand_, [&](std::uint32_t epoch) {
		check_epoch(epoch);
	}));
	
	start_renew_timer();
}

void Daemon::start_renew_timer(const std::chrono::seconds time) {
	state_ = State::timer_wait;

	timer_.expires_after(time);
	timer_.async_wait([&](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		const auto time = std::chrono::steady_clock::now();

		for(const auto& mapping : mappings_) {
			const auto time_remaining = mapping.expiry - time;

			if(time_remaining < 0s) {
				invoke_daemon_callback(Event::mapping_expired, mapping.request);
			}

			if(time_remaining < renew_when_below) {
				queue_.emplace_back(mapping);
			}
		}

		process_queue();
	});
}

/*
 * This must only be *initiated* from the timer, otherwise there's a risk that
 * the NAT-PMP/PCP client is issued with multiple mapping requests, even if
 * everything is done through a strand (concurrent, not parallel, in the
 * Rob Pike school of thought)
 * 
 * Calling it from within the renew handler is also fine, because it only continues
 * once the previously issued request has been finished
 *
 *         ←←←←←←←←←←←←←←←
 *         ↓             ↑
 *         ↓          (empty)
 *         ↓             ↑
 * timer elapsed → process queue → (not empty) → renew_mapping
 *                       ↑                             ↓
 *                       ↑                    completion handler
 *                       ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
 */
void Daemon::process_queue() {
	state_ = State::queue;
	timer_.cancel();

	if(queue_.empty()) {
		begin_timer();
		return;
	}

	auto mapping = std::move(queue_.front());
	queue_.pop_front();
	renew_mapping(mapping);
	mapping.handler = nullptr;
}

void Daemon::begin_timer() {
	std::chrono::seconds nearest_expiry = timer_interval + renew_leeway;

	// adjust the timer based on mappings' expiry times
	for(const auto& mapping : mappings_) {
		const auto time_remaining = std::chrono::duration_cast<std::chrono::seconds>(
			mapping.expiry - std::chrono::steady_clock::now()
		);

		if(time_remaining < nearest_expiry) {
			nearest_expiry = time_remaining;
		}
	}

	if(nearest_expiry > renew_leeway) {
		nearest_expiry -= renew_leeway;
	} else {
		nearest_expiry = 0s;
	}

	// Fire after the shortest remaining time, if sooner than we'd otherwise fire
	start_renew_timer(nearest_expiry);
}

void Daemon::renew_mapping(const Mapping& mapping) {
	client_.add_mapping(mapping.request, mapping.strict,				
	                    boost::asio::bind_executor(strand_, [&, mapping](const Result& result) {
		last_received_time_ = std::chrono::steady_clock::now();

		// we don't auto-delete the mapping if it fails because testing
		// showed that it's possible to have transient errors,
		// so we'll keep the entry and hope we have better luck next time
		if(result) {
			if(result->lifetime) {
				if(mapping.handler) {
					invoke_daemon_callback(Event::added_mapping, *result);
				} else {
					invoke_daemon_callback(Event::renewed_mapping, *result);
				}
			} else {
				invoke_daemon_callback(Event::deleted_mapping, *result);
			}

			update_mapping(result, mapping.request);
			check_epoch(result->epoch);
		} else {
			if(mapping.handler) {
				invoke_daemon_callback(Event::failed_to_renew, mapping.request);
			} else {
				invoke_daemon_callback(Event::failed_mapping, mapping.request);
			}
		}

		if(mapping.handler) {
			mapping.handler(result);
		}

		process_queue();
	}));
}

void Daemon::update_mapping(const Result& result, const MapRequest& request) {
	for(auto& mapping : mappings_) {
		if(mapping.request.internal_port == result->internal_port) {
			const auto time = std::chrono::steady_clock::now();
			mapping.expiry = time + std::chrono::seconds(result->lifetime);
		}
	}
}

void Daemon::renew_mappings() {
	for(const auto& mapping : mappings_) {
		queue_.emplace_back(mapping);
	}
}

void Daemon::check_epoch(std::uint32_t epoch) {
	// if we haven't received anything yet, we can't compare previous epoch
	if(!epoch_acquired_) {
		gateway_epoch_ = epoch;
		epoch_acquired_ = true;
		return;
	}

	// if epoch is less than recorded time, gateway has (likely) dropped mappings
	if(epoch < gateway_epoch_) {
		gateway_epoch_ = epoch;

		if(consecutive_failures_ <= max_consecutive_failures) {
			renew_mappings();
		}

		return;
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::steady_clock::now() - last_received_time_
	);
	
	// RFC6886 - 3.6.  Seconds Since Start of Epoch
	const auto expected_epoch = std::chrono::seconds(gateway_epoch_) + (elapsed * 0.875);
	gateway_epoch_ = epoch;

	if(std::chrono::seconds(epoch) < (expected_epoch - 2s)) {
		++consecutive_failures_;

		if(consecutive_failures_ <= max_consecutive_failures) {
			renew_mappings();
		}
	} else {
		consecutive_failures_ = 0;
	}
}

void Daemon::add_mapping(MapRequest request, bool strict, RequestHandler&& callback) {
	// If there's no ID set, we'll set our own and make sure we keep hold of it
	// for refreshes (spec requires it to match OG request to refresh)
	const auto it = std::ranges::find_if(request.nonce,
		[](const std::uint8_t val) {
			return val != 0;
		});

	if(it == request.nonce.end()) {
		std::random_device engine;
		std::ranges::generate(request.nonce, std::ref(engine));
	}

	RequestHandler wrapped = [&, strict, request, callback](const Result& result) {
		if(result) {
			const auto expiry = std::chrono::steady_clock::now()
				+ std::chrono::seconds(result->lifetime);

			Mapping mapping {
				.request = request,
				.expiry = expiry,
				.strict = strict,
			};

			mapping.request.external_ip = result.value().external_ip;
			mappings_.emplace_back(std::move(mapping));
			check_epoch(result->epoch);
		}

		callback(result);
	};

	Mapping mapping {
		.request = request,
		.handler = std::move(wrapped)
	};

	boost::asio::post(strand_, [&, mapping]() {
		queue_.emplace_front(mapping);

		if(state_ == State::timer_wait) {
			start_renew_timer(0s);
		}
	});
}

void Daemon::delete_mapping(std::uint16_t internal_port, Protocol protocol, RequestHandler&& handler) {
	RequestHandler wrapped = [&, handler = std::move(handler)](const Result& result) {
		if(result) {
			boost::asio::dispatch(strand_, [&, r = result] {
				erase_mapping(r);
			});
		}

		handler(result);
	};

	const MapRequest request {
		.protocol = protocol,
		.internal_port = internal_port,
	};

	Mapping mapping {
		.request = request,
		.handler = std::move(wrapped)
	};

	boost::asio::post(strand_, [&, mapping = std::move(mapping)]() mutable {
		queue_.emplace_front(std::move(mapping));

		if(state_ == State::timer_wait) {
			start_renew_timer(0s);
		}
	});
}

/*
 * Not using a map here because there's no ideal key that
 * wouldn't make the API more awkward to use. 'internal_port'
 * would be a good candidate but it fails if a router allows
 * multiple ext. port -> same int. port mappings (test HW didn't)
 * - don't want a map of vectors
 * 
 * Other option might be the PCP ID but we also support NAT-PMP (no IDs)
 * and a delete request does not need to be 1-to-1 with an add request,
 * so we can't ask the user to carry an ID around
 * 
 * Iteration speed for refreshes is more important here, so this'll do
 */
void Daemon::erase_mapping(const Result& result) {
	std::erase_if(mappings_, [&](const auto& ele) {
		return ele.request.internal_port == result->internal_port;
	});
}

void Daemon::invoke_daemon_callback(Event event, const MapRequest& request) {
	if(daemon_callback_) {
		daemon_callback_(event, request);
	}
}

} // ports, ember
