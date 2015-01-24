/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <logger/Logger.h>
#include <logger/LoggerImpl.h>

namespace ember { namespace log {

Logger::Logger() : pimpl_(std::make_unique<impl>()) {}
Logger::~Logger() = default;

void Logger::finalise() {
	pimpl_->finalise();
}

void Logger::finalise_sync() {
	pimpl_->finalise_sync();
}

void Logger::add_sink(std::unique_ptr<Sink> sink) {
	pimpl_->add_sink(std::move(sink));
}

SEVERITY Logger::severity() {
	return pimpl_->severity();
}

void Logger::thread_exit() {
	pimpl_->thread_exit();
}

Logger& Logger::operator <<(Logger& (*m)(Logger&)) {
	return (*m)(*this);
}

Logger& Logger::operator <<(SEVERITY severity) {
	*pimpl_ << severity;
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

Logger& Logger::operator <<(unsigned int data) {
	*pimpl_ << data;
	return *this;
}

Logger& Logger::operator <<(const std::string& data) {
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

}} //log, ember