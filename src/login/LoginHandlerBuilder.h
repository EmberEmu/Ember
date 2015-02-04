/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "LoginHandler.h"
#include <shared/database/daos/UserDAO.h>
#include <shared/threading/ThreadPool.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <memory>

namespace ember {

class LoginHandlerBuilder {
	ASIOAllocator& allocator_;
	log::Logger* logger_;
	const std::vector<GameVersion>& versions_;
	dal::UserDAO& user_dao_;
	ThreadPool& tpool_;

	Authenticator create_authenticator() {
		return Authenticator(user_dao_, versions_);
	}

public:
	LoginHandlerBuilder(ASIOAllocator& allocator, log::Logger* logger,
	                    const std::vector<GameVersion>& versions,
                        dal::UserDAO& user_dao, ThreadPool& pool)
	                    : allocator_(allocator), logger_(logger), versions_(versions),
                          user_dao_(user_dao), tpool_(pool) { }

	std::shared_ptr<LoginHandler> create(boost::asio::ip::tcp::socket& socket) {
		return std::allocate_shared<LoginHandler>(boost::fast_pool_allocator<LoginHandler>(),
		                                          std::move(socket), allocator_,
		                                          Authenticator(user_dao_, versions_), tpool_, logger_);
	}
};

} //ember