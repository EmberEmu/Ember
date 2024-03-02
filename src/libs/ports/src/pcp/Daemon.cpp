/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Daemon.h>
#include <algorithm>
#include <random>

namespace ember::ports {

Daemon::Daemon(Client& client, boost::asio::io_context& ctx)
	: client_(client), timer_(ctx), strand_(ctx) {
	daemon_epoch_ = std::chrono::steady_clock::now();

	client_.announce_handler(strand_.wrap([&](std::uint32_t epoch) {
		check_epoch(epoch);
	}));
	
	start_renew_timer();
}

void Daemon::start_renew_timer(const std::chrono::seconds time) {
	state_ = State::TIMER_WAIT;

	timer_.expires_from_now(time);
	timer_.async_wait(strand_.wrap([&](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		const auto time = std::chrono::steady_clock::now();

		for(const auto& mapping : mappings_) {
			const auto time_remaining = mapping.expiry - time;

			if(time_remaining < 0s) {
				handler_(Event::MAPPING_EXPIRED, mapping.request);
			}

			if(time_remaining < RENEW_WHEN_BELOW) {
				queue_.emplace_back(mapping);
			}
		}

		process_queue();
	}));
}

/*
 * This must only be *initiated* from the timer, otherwise there's a risk that
 * the NAT-PMP/PCP client is issued with multiple mapping requests, even if
 * everything is done through a strand (concurrent, not parallel, in the
 * Rob Pike school of thought)
 * 
 * Calling it from within the renew handler is also fine, because it only continues
 * once the previously issued request has been finished

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
	state_ = State::QUEUE;
	timer_.cancel();

	if(queue_.empty()) {
		start_renew_timer();
		return;
	}

	auto& mapping = queue_.front();
	queue_.pop_front();
	renew_mapping(mapping);
	mapping.handler = nullptr;
}

void Daemon::renew_mapping(const Mapping& mapping) {
	client_.add_mapping(mapping.request, mapping.strict,
		strand_.wrap([&](const Result& result) mutable {
		// we don't auto-delete the mapping if it fails because testing
		// showed that it's possible to have transient errors,
		// so we'll keep the entry and hope we have better luck next time
		if(result) {
			handler_(Event::RENEWED_MAPPING, mapping.request);
			update_mapping(result);
			check_epoch(result->epoch);
		} else {
			handler_(Event::FAILED_TO_RENEW, mapping.request);
		}

		if(mapping.handler) {
			mapping.handler(mapping.request);
		}

		process_queue();
	}));
}

void Daemon::update_mapping(const Result& result) {
	for(auto& mapping : mappings_) {
		if(mapping.request.internal_port == result->internal_port) {
			mapping.request.external_port = result->external_port;
			mapping.request.external_ip = result->external_ip;
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
		renew_mappings();
		return;
	}

	// does anybody actually like std::chrono?
	const auto daemon_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::steady_clock::now() - daemon_epoch_
	);
	
	// RFC 6886 - 3.6.  Seconds Since Start of Epoch
	const std::chrono::seconds gs {gateway_epoch_};
	const auto expected_epoch = gs + (daemon_elapsed * 0.875);
	const std::chrono::seconds es {epoch};
	gateway_epoch_ = epoch;

	if(es < (expected_epoch - 2s)) {
		renew_mappings();
	}
}

void Daemon::add_mapping(MapRequest request, bool strict, RequestHandler&& handler) {
	// If there's no ID set, we'll set our own and make sure we keep hold of it
	// for refreshes (spec requires it to match OG request to refresh)
	const auto it = std::find_if(request.nonce.begin(), request.nonce.end(),
		[](const std::uint8_t val) {
			return val != 0;
		});

	if(it == request.nonce.end()) {
;		std::random_device engine;
		std::generate(request.nonce.begin(), request.nonce.end(), std::ref(engine));
	}

	RequestHandler wrapped =
		[&, strict, request, handler = std::move(handler)](const Result& result) {
		if(result) {
			const auto expiry = std::chrono::steady_clock::now()
				+ std::chrono::seconds(result->lifetime);

			Mapping mapping {
				.request = {
					.external_port = result->external_port,
					.external_ip = result->external_ip,
				},
				.expiry = expiry,
				.strict = strict,
			};

			handler_(Event::ADDED_MAPPING, mapping.request);
			mappings_.emplace_back(std::move(mapping));
			check_epoch(result->epoch);
		}

		handler(result);
	};

	Mapping mapping{
		.request = request,
		.handler = std::move(wrapped)
	};

	strand_.post([&, mapping = std::move(mapping)]() mutable {
		queue_.emplace_front(std::move(mapping));

		if(state_ == State::TIMER_WAIT) {
			start_renew_timer(0s);
		}
	});
}

void Daemon::delete_mapping(std::uint16_t internal_port, Protocol protocol,
							RequestHandler&& handler) {
	RequestHandler wrapped = 
		[&, handler = std::move(handler)](const Result& result) {
		if(result) {
			strand_.dispatch([&, r = result] {
				erase_mapping(r);
			});
		}

		handler(result);
	};

	const MapRequest request {
		.protocol = protocol,
		.internal_port = internal_port,
	};

	Mapping mapping{
		.request = request,
		.handler = std::move(wrapped)
	};

	strand_.post([&, mapping = std::move(mapping), request]() mutable {
		queue_.emplace_front(std::move(mapping));

		if(state_ == State::TIMER_WAIT) {
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
	for(auto it = mappings_.begin(); it != mappings_.end();) {
		if(it->request.internal_port == result->internal_port) {
			it = mappings_.erase(it);
		} else {
			++it;
		}
	}
}

void Daemon::event_handler(EventHandler&& handler) {
	if(!handler) {
		throw std::invalid_argument("Handler cannot be null");
	}

	handler_ = std::move(handler);
}

} // ports, ember
