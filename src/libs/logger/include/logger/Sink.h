/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Severity.h>
#include <string_view>
#include <utility>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

namespace ember::log {

class Sink {
	Severity severity_;
	Filter filter_;
	std::string name_;
	std::string type_;

protected:
	Sink(std::string name, std::string type, Severity severity, Filter filter)
		: severity_(severity)
		, filter_(filter)
		, name_(std::move(name))
		, type_(std::move(type)) {
		if(name_.empty()) {
			throw std::runtime_error("sink name cannot be empty");
		}
	}

public:
	Severity severity() const { return severity_; }
	Filter filter() const { return filter_; }
	std::string_view type() const { return type_; }
	std::string_view name() const { return name_; }
	void severity(Severity severity) { severity_ = severity; }
	void filter(const Filter& filter) { filter_ = filter; }
	virtual bool unique() { return false;  }
	virtual void write(Severity severity, Filter type, std::string_view prefix, std::string_view record, bool flush) = 0;
	virtual void batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) = 0;
	virtual ~Sink() = default;
};

} // log, ember