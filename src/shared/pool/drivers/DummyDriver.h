/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

class DummyConnection;

class DummyDriver {
	
public:
	DummyDriver();
	DummyConnection open() const;
	DummyConnection clean(DummyConnection conn) const;
	void close(DummyConnection conn) const;
	void clear_state(DummyConnection conn) const;
	DummyConnection keep_alive(DummyConnection conn) const;
	void thread_enter() const;
	void thread_exit() const;
};

} //ember