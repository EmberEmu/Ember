/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <conpool/PoolImpl.h>
#include <memory>
#include <utility>

namespace ember::connection_pool {

template<typename Driver>
struct PoolInterface {
	using ConType = typename Driver::ConnectionType;

	virtual ~PoolInterface () = default;

	virtual void logging_callback(LoggingCallback&& callback) = 0;
	virtual void close() = 0;
	virtual std::optional<Connection<ConType>> try_acquire() = 0;
	virtual Connection<ConType> acquire() = 0;
	virtual Connection<ConType> try_acquire_for(std::chrono::milliseconds duration) = 0;
	virtual void return_connection(Connection<ConType>& connection) = 0;
	virtual std::size_t size() const = 0;
	virtual bool dirty() const = 0;
	virtual bool checked_out() const = 0;
	virtual Driver* get_driver() = 0;
	virtual const Driver* get_driver() const = 0;
};

template<typename Driver, typename ReusePolicy, typename GrowthPolicy, unsigned int size_hint = 32>
class PoolProxy final : public PoolInterface<Driver> {
	PoolImpl<Driver, ReusePolicy, GrowthPolicy, size_hint> impl;

public:
	using ConType = typename Driver::ConnectionType;

	PoolProxy(Driver&& driver, auto&&... args)
		: impl(std::forward<Driver>(driver), std::forward<decltype(args)>(args)...) {}

	void logging_callback(LoggingCallback&& callback) override {
		impl.logging_callback(std::move(callback));
	}

	void close() override {
		impl.close();
	}

	std::optional<Connection<ConType>> try_acquire() override {
		return impl.try_acquire();
	}

	Connection<ConType> acquire() override {
		return impl.acquire();
	}

	Connection<ConType> try_acquire_for(std::chrono::milliseconds duration) override {
		return impl.try_acquire_for(duration);
	}

	void return_connection(Connection<ConType>& connection) override {
		return impl.return_connection(connection);
	}

	std::size_t size() const override {
		return impl.size();
	}

	bool dirty() const override {
		return impl.dirty();
	}

	bool checked_out() const override {
		return impl.checked_out();
	}

	Driver* get_driver() override {
		return impl.get_driver();
	}

	const Driver* get_driver() const override {
		return impl.get_driver();
	}
};

template<typename Driver>
struct Pool {
	std::unique_ptr<PoolInterface<Driver>> base;

public:
	Pool() = default;

	Pool(std::unique_ptr<PoolInterface<Driver>> base)
		: base(std::move(base)) {}

	auto operator->() {
		return base.get();
	}

	const auto operator->() const {
		return base.get();
	}
};

template<typename Driver, typename ReusePolicy, typename GrowthPolicy, unsigned int size_hint = 32>
static inline auto create(Driver driver, auto&& ...args) {
	return Pool<Driver> {
		std::make_unique<PoolProxy<Driver, ReusePolicy, GrowthPolicy, size_hint>>(
			std::forward<Driver>(driver), std::forward<decltype(args)>(args)...
		)
	};
}

} // connection_pool, ember