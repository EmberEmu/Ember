/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <memory>

namespace ember {

class LoadingAnimation {
public:
	enum class Animation { PULSE, LOOP };
private:
	Animation type;
	const char COLUMN_WIDTH = 79;
	const char PADDING = 1;
	int progress = 1;
	bool exit = false;
	bool reverse = false;
	char fill;
	std::chrono::milliseconds sleep_time;

	std::string text;
	std::mutex lock;
	std::unique_ptr<std::thread> thread;

	void loop();
	void tick();

public:
	LoadingAnimation(int fps, char fill, Animation animation);
	~LoadingAnimation();
	void start(std::string text = "");
	void stop();
	void message(std::string text);
};

}