/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/detail/CommandHistory.h>
#include <utility>

namespace ember::log::detail {

CommandHistory::CommandHistory(std::size_t max_entries)
	: max_entries_(max_entries)
	, index_(0) {}

std::string_view CommandHistory::scroll(ScrollDirection direction) {
	if(history_.empty()) {
		return {};
	}

	if(direction == ScrollDirection::up) {
		if(index_) {
			--index_;
		}
	} else {
		if(index_ >= history_.size()) {
			return {};
		}

		++index_;
	}

	if(index_ != history_.size()) {
		return history_[index_];
	}

	return {};
}

void CommandHistory::insert(std::string command) {
	// don't insert duplicates of the previous command
	if(!history_.empty() && history_.back() == command) {
		index_ = size();
		return;
	}

	history_.emplace_back(std::move(command));

	if(history_.size() > max_entries_) {
		history_.pop_front();
	}

	index_ = size();
}

bool CommandHistory::empty() const {
	return history_.empty();
}

std::string_view CommandHistory::current() const {
	if(index_ >= history_.size()) {
		return {};
	}

	return history_[index_];
}

std::size_t CommandHistory::size() const {
	return history_.size();
}

constexpr std::size_t CommandHistory::capacity() const {
	return max_entries_;
}

} // detail, log, ember