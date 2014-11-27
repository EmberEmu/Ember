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

	LoadingAnimation(int fps, char fill, Animation animation);
	~LoadingAnimation();

	void start(std::string text = "");
	void stop();
	void message(std::string text);

private:
	Animation type_;
	const char COLUMN_WIDTH_ = 79;
	const char PADDING_ = 1;
	int progress_ = 1;
	bool exit_ = false;
	bool reverse_ = false;
	char fill_;
	std::chrono::milliseconds sleep_time_;

	std::string text_;
	std::mutex lock_;
	std::unique_ptr<std::thread> thread_;

	void loop();
	void tick();
};

}