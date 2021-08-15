/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Logger.h>
#include <logger/LoggerImpl.h>

namespace ember::log {

Logger::Logger() : pimpl_(std::make_unique<impl>()) {}
Logger::~Logger() = default;

std::vector<char>* Logger::get_buffer() {
	return pimpl_->get_buffer();
}

void Logger::finalise() {
	pimpl_->finalise();
}

void Logger::finalise_sync() {
	pimpl_->finalise_sync();
}

void Logger::add_sink(std::unique_ptr<Sink> sink) {
	pimpl_->add_sink(std::move(sink));
}

Severity Logger::severity() {
	return pimpl_->severity();
}

Filter Logger::filter() {
	return pimpl_->filter();
}

Logger& Logger::operator <<(Logger& (*m)(Logger&)) {
	return (*m)(*this);
}

Logger& Logger::operator <<(Severity severity) {
	*pimpl_ << severity;
	return *this;
}

Logger& Logger::operator <<(Filter type) {
	*pimpl_ << type;
	return *this;
}

Logger& Logger::operator <<(float data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(double data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(bool data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(int data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(long data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(long long data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(unsigned long data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(unsigned long long data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(unsigned int data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(const std::string& data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(const std::string_view data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(const char* data) {
	*pimpl_ << data;
	return *this;
}

Logger& flush(Logger& out) {
	out.finalise();
	return out;
}

Logger& flush_sync(Logger& out) {
	out.finalise_sync();
	return out;
}

} //log, ember