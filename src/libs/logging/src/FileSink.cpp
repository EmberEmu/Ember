/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/FileSink.h>
#include <logger/Exception.h>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

#pragma warning(push)
#pragma warning(disable: 4996)

namespace ember::log {

namespace fs = boost::filesystem;

FileSink::FileSink(Severity severity, Filter filter, std::string file_name, Mode mode)
                   : Sink(severity, filter), file_name_format_(std::move(file_name)) {
	format_file_name();

	if(mode == Mode::APPEND) {
		boost::system::error_code ec;
		std::uintmax_t size = fs::file_size(fs::path(file_name_), ec);

		// file_size returns -1 if the file doesn't exist
		if(size == static_cast<std::uintmax_t>(-1)) {
			size = 0;
		} else if(ec) {
			throw exception("Unable to determine initial log file size");
		}

		current_size_ = size;
	}

	open(mode);
	set_initial_rotation();
}

FileSink::~FileSink() {
	// logger is being closed, not much we can do about this
	if(file_->close() != 0) {
		std::fprintf(stderr, "Log file did not close cleanly - buffered messages may have been lost");
	}
}

bool FileSink::file_exists(const std::string& name) try {
	return fs::exists(fs::path(name));
} catch(boost::filesystem::filesystem_error& e) {
	throw exception(e.what());
}

void FileSink::set_initial_rotation() {
	auto max = std::numeric_limits<decltype(rotations_)>::max();

	while(file_exists(file_name_ + std::to_string(rotations_)) && rotations_ < max) {
		++rotations_;
	}

	if(rotations_ == max) {
		throw exception("Unable to set initial log rotation count. How did this happen?");
	}
}

void FileSink::time_format(const std::string& format) {
	time_format_ = format;
}

void FileSink::format_file_name() {
	std::tm time = detail::current_time();
	file_name_ = std::move(detail::put_time(time, file_name_format_));
}

void FileSink::open(Mode mode) {
	if(mode == Mode::APPEND && !rotations_) {
		file_ = std::make_unique<File>(file_name_, "ab");
	} else {
		file_ = std::make_unique<File>(file_name_, "wb");
	}

	if(!file_->handle()) {
		throw exception("Logger could not open " + file_name_);
	}
}

void FileSink::size_limit(std::uintmax_t megabytes) {
	max_size_ = megabytes * 1024 * 1024;
}

void FileSink::rotate() {
	if(file_->close() != 0) {
		throw exception("Unable to close log file during rotation - buffered messages may have been lost");
	}

	std::string rotated_name = file_name_ + std::to_string(rotations_);

	if(std::rename(file_name_.c_str(), rotated_name.c_str()) != 0) {
		throw exception("Unable to rotate log file");
	}

	++rotations_;
	current_size_ = 0;
	
	format_file_name();
	open();
}

std::string FileSink::generate_record_detail(Severity severity, const std::tm& curr_time) {
	std::string prepend;

	if(log_date_) {
		prepend = std::move(detail::put_time(curr_time, time_format_));
	}

	if(log_severity_) {
		std::string sev = detail::severity_string(severity);

		if(!log_date_) {
			prepend = std::move(sev);
		} else {
			prepend.append(sev);
		}
	}

	return prepend;
}

void FileSink::rotate_check(std::size_t buffer_size, const std::tm& curr_time) {
	if((max_size_ && current_size_ + buffer_size > max_size_)
	    || (midnight_rotate_ && last_mday_ != curr_time.tm_mday)) {
		rotate();
		last_mday_ = curr_time.tm_mday;
	}
}

void FileSink::batch_write(const std::vector<std::pair<RecordDetail, std::vector<char>>>& records) {
	std::tm curr_time = detail::current_time();
	std::size_t size = 0;
	Severity severity = this->severity();
	Filter filter = this->filter();
	bool matches = false;

	for(auto& r : records) {
		if(severity <= r.first.severity && !(filter & r.first.type)) {
			size += r.second.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	std::vector<char> buffer;
	buffer.reserve(size + (20 * records.size()));

	for(auto& r : records) {
		if(severity <= r.first.severity && !(filter & r.first.type)) {
			std::string prepend = std::move(generate_record_detail(r.first.severity, curr_time));
			std::copy(prepend.begin(), prepend.end(), std::back_inserter(buffer));
			std::copy(r.second.begin(), r.second.end(), std::back_inserter(buffer));
		}
	}

	std::size_t buffer_size = buffer.size();
	rotate_check(buffer_size, curr_time);

	if(!std::fwrite(buffer.data(), buffer_size, 1, *file_)) {
		throw exception("Unable to write log record batch to file");
	}

	current_size_ += buffer_size;
}


void FileSink::write(Severity severity, Filter type, const std::vector<char>& record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	std::tm curr_time = detail::current_time();
	std::string prepend = std::move(generate_record_detail(severity, curr_time));
	std::size_t prep_size = prepend.size();
	std::size_t rec_size = record.size();

	rotate_check(prep_size + rec_size, curr_time);

	std::size_t count = std::fwrite(prepend.c_str(), prep_size, 1, *file_);
	count += std::fwrite(record.data(), rec_size, 1, *file_);

	if(count != 2) {
		throw exception("Unable to write log record to file");
	}

	current_size_ += (prep_size + rec_size);

	if(flush) {
		if(std::fflush(*file_) != 0) {
			throw exception("Unable to flush log record to file");
		}
	}
}

} //log, ember

#pragma warning(pop)