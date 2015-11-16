/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/MessageHandler.h>
#include <flatbuffers/flatbuffers.h>
#include <spark/temp/MessageRoot_generated.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace ember { namespace spark {

MessageHandler::MessageHandler(log::Logger* logger, log::Filter filter)
                               : logger_(logger), filter_(filter) { }

bool MessageHandler::handle_message(const std::vector<std::uint8_t>& net_buffer) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	flatbuffers::Verifier verifier(net_buffer.data(), net_buffer.size());

	if(!messaging::VerifyMessageRootBuffer(verifier)) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Message failed validation, dropping peer" << LOG_ASYNC;
		return false;
	}
	
	auto message = messaging::GetMessageRoot(net_buffer.data());

	if(message->data_type() == messaging::Data_Banner) {
		const messaging::Banner* banner = (const messaging::Banner*)message->data();
		boost::uuids::uuid remote_id;
		std::copy(banner->server_uuid()->begin(), banner->server_uuid()->end(), remote_id.data);
	
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Remote UUID: " << boost::uuids::to_string(remote_id) << LOG_ASYNC;
	}

	return true;
}

}} // spark, ember