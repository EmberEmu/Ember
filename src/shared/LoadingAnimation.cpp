/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoadingAnimation.h"
#include <chrono>
#include <iostream>

namespace ember {

LoadingAnimation::LoadingAnimation(int fps, char fill, Animation animation) {
	if(fps <= 0 || fps > 120) {
		fps = 30;
	}

	sleep_time_ = std::chrono::milliseconds(1000 / fps);
	fill_ = fill;
	type_ = animation;
}

LoadingAnimation::~LoadingAnimation() {
	if(thread_ != nullptr && thread_->joinable()) {
		stop();
	}
}

void LoadingAnimation::tick() {
	std::lock_guard<std::mutex> guard(lock_);
	int free_space = (COLUMN_WIDTH_ - text_.length()) - (PADDING_ * 2);
	int bar_width = progress_ * 2 > free_space ? free_space : progress_ * 2;
	int alignment = free_space - bar_width;
	std::string align(alignment / 2, ' ');
	std::string bar(bar_width / 2, fill_);
	std::string pad(PADDING_, ' ');
	std::string final(align + bar + pad + text_ + pad + bar + align);
	std::cout << final << "\r" << std::flush;

	if(type_ == Animation::PULSE) {
		reverse_ = (progress_ % 30 == 0) ? !reverse_ : reverse_;
		progress_ = reverse_? progress_ - 1 : progress_ + 1;
	} else {
		++progress_;
		progress_ %= free_space / 2;
	}
}

void LoadingAnimation::start(std::string text) {
	if(thread_) {
		throw std::runtime_error("Called start twice before stop");
	}

	text_ = std::move(text);
	thread_ = std::make_unique<std::thread>(std::bind(&LoadingAnimation::loop, this));
}

void LoadingAnimation::stop() {
	exit_ = true;
	thread_->join();
}

void LoadingAnimation::message(std::string text) {
	std::lock_guard<std::mutex> guard(lock_);
	text_ = std::move(text);
}

void LoadingAnimation::loop() {
	while(!exit) {
		tick();
		std::this_thread::sleep_for(sleep_time_);
	}

	progress_ = 0;
	tick();
	std::cout << "\n";
}

} //ember