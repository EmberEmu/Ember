/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/ConsoleSink.h>
#include <logger/Exception.h>
#include <logger/Utility.h>
#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <cstdio>
#include <cstring>

namespace ember::log {

ConsoleSink::ConsoleSink(ConsoleSinkOptions options)
	: Sink(options.name, std::string(type()), options.severity, options.filter)
	, opts_(std::move(options)) {}

void ConsoleSink::batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) {
	if(!opts_.colours) [[unlikely]] {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record
		for(const auto& [detail, prefix, data] : records) {
			write(detail.severity, detail.type, prefix, data, false);
		}
	}
}

void ConsoleSink::do_batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) {
	std::size_t size = 0;
	Severity sink_sev = this->severity();
	Filter sink_filter = this->filter();
	bool matches = false;

	for(auto&& [detail, prefix, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter &detail.type)) {
			size += data.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(opts_.timestamp) {
		ts_size = generate_timestamp(ts_buffer);
	}

	const std::span<char> timestamp(ts_buffer.data(), ts_size);
	out_buf_.reserve(size + (10 * records.size()));

	for(auto&& [detail, prefix, data] : records) {
		if(sink_sev > detail.severity || (sink_filter & detail.type)) {
			continue;
		}

		std::string_view severity = severity_string(detail.severity);

		// todo, size_t literal on bump
		const auto padding = !prefix.empty()?
			std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
				prefix_pad_.size() - prefix.size() : 0u) : 0u;

		const auto cur_sz = out_buf_.size();

		const auto req_buf_size
			= cur_sz
			+ timestamp.size()
			+ prefix.size()
			+ padding
			+ severity.size()
			+ data.size();

		out_buf_.resize(req_buf_size, boost::container::default_init);
		auto write_ptr = out_buf_.data() + cur_sz;
		std::size_t offset = 0;
		
		std::memcpy(write_ptr + offset, timestamp.data(), timestamp.size());
		offset += timestamp.size();

		std::memcpy(write_ptr + offset, prefix.data(), prefix.size());
		offset += prefix.size();

		std::memcpy(write_ptr + offset, prefix_pad_.data(), padding);
		offset += padding;

		std::memcpy(write_ptr + offset, severity.data(), severity.size());
		offset += severity.size();

		std::memcpy(write_ptr + offset, data.data(), data.size());
	}

	std::fwrite(out_buf_.data(), out_buf_.size(), 1, stdout);

	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

void ConsoleSink::write(Severity severity, Filter type, std::string_view prefix, std::string_view record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(opts_.timestamp) {
		ts_size = generate_timestamp(ts_buffer);
	}

	const std::span<char> timestamp(ts_buffer.data(), ts_size);

	// todo, size_t literal on bump
	const auto padding = !prefix.empty()?
		std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
			prefix_pad_.size() - prefix.size() : 0u) : 0u;

	std::string_view sevsv = severity_string(severity);

	const std::size_t req_buf_size
		= timestamp.size()
		+ prefix.size()
		+ padding
		+ record.size()
		+ sevsv.size();

	out_buf_.resize(req_buf_size, boost::container::default_init);

	std::size_t offset = 0;

	const auto append = [&](auto&& fragment) {
		std::copy(fragment.begin(), fragment.end(), out_buf_.begin() + offset);
		offset += fragment.size();
	};

	append(timestamp);
	append(prefix);
	append(std::string_view(prefix_pad_.data(), padding));
	append(sevsv);
	append(record);

	if(opts_.colours) [[likely]] {
		std::lock_guard guard(colour_lock);
		ConsoleColour concol(detail::severity_colour(severity));
		std::fwrite(out_buf_.data(), out_buf_.size(), 1, stdout);
	} else {
		std::fwrite(out_buf_.data(), out_buf_.size(), 1, stdout);
	}

	if(flush) {
		if(std::fflush(stdout) != 0) {
			throw exception("Unable to flush log record to console");
		}
	}

	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

std::size_t ConsoleSink::generate_timestamp(std::span<char> buffer) {
	const auto curr_time = detail::current_time();
	const auto size = detail::put_time(curr_time, opts_.timestamp_fmt, buffer);

	if(!size) [[unlikely]] {
		throw std::runtime_error("timestamp formatting failed");
	}

		if(size >= buffer.size()) [[unlikely]] {
			throw std::runtime_error("timestamp buffer out of space");
		}

	buffer[size] = ' '; // add a trailing space
	return size + 1;    // we don't need the null terminator
}

} // log, ember