#include "LoadingAnimation.h"
#include <chrono>
#include <iostream>

namespace ember {

LoadingAnimation::LoadingAnimation(int fps, char fill, Animation animation) {
	if(fps <= 0 || fps > 120) {
		fps = 30;
	}

	this->sleep_time = std::chrono::milliseconds(1000 / fps);
	this->fill = fill;
	this->type = animation;
}

LoadingAnimation::~LoadingAnimation() {
	if(thread != nullptr && thread->joinable()) {
		stop();
	}
}

void LoadingAnimation::tick() {
	std::lock_guard<std::mutex> guard(lock);
	int free_space = (COLUMN_WIDTH - text.length()) - (PADDING * 2);
	int bar_width = progress * 2 > free_space ? free_space : progress * 2;
	int alignment = free_space - bar_width;
	std::string align(alignment / 2, ' ');
	std::string bar(bar_width / 2, fill);
	std::string pad(PADDING, ' ');
	std::string final(align + bar + pad + text + pad + bar + align);
	std::cout << final << "\r" << std::flush;

	if(type == Animation::PULSE) {
		reverse = (progress % 30 == 0) ? !reverse : reverse;
		progress = reverse? progress - 1 : progress + 1;
	} else {
		++progress;
		progress %= free_space / 2;
	}
}

void LoadingAnimation::start(std::string text) {
	if(thread) {
		throw std::runtime_error(__FILE__ + std::string(": Called start twice before stop"));
	}

	this->text = std::move(text);
	thread = std::make_unique<std::thread>(std::bind(&LoadingAnimation::loop, this));
}

void LoadingAnimation::stop() {
	exit = true;
	thread->join();
}

void LoadingAnimation::message(std::string text) {
	std::lock_guard<std::mutex> guard(lock);
	this->text = std::move(text);
}

void LoadingAnimation::loop() {
	while(!exit) {
		tick();
		std::this_thread::sleep_for(sleep_time);
	}

	progress = 0;
	tick();
	std::cout << "\n";
}

} //ember