/*
 * Copyright (c) 2020 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldEnter.h"
#include "../ClientContext.h"
#include "../ClientHandler.h"
#include "../ClientConnection.h"
#include "../RealmQueue.h"
#include <commands/Utility.h>
#include <logger/CommandSink.h>
#include <protocol/Deserialise.h>
#include <protocol/Packets.h>
#include <chrono>
#include <format>
#include <span>
#include <ctime>

/*
 * This entire file is just temporary test code 
 */

namespace ember::realm::world_enter {

std::uint32_t get_time() {
	// bbrrrr, it'll do for now
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm_now;

#if _MSC_VER && !__INTEL_COMPILER
	localtime_s(&tm_now, &now_time_t);
#else
	localtime_r(&now_time_t, &tm_now);
#endif

	int year = (tm_now.tm_year + 1900 - 2000) << 24;
	int month = tm_now.tm_mon << 20;  
	int day = (tm_now.tm_mday - 1) << 14;
	int dow = tm_now.tm_wday << 11;
	int hour = tm_now.tm_hour << 6;

	return tm_now.tm_min + hour + dow + day + month + year;
}

void initiate_player_login(ClientContext& ctx, const PlayerLogin& event) {
    auto& state_ctx = std::get<Context>(ctx.state_ctx);
    state_ctx.character_id = event.character_id_;

	protocol::smsg_trigger_cinematic cinematic;
	cinematic->sequence_id = protocol::CinematicSequence::human;
	ctx.send(cinematic);

	protocol::smsg_login_verify_world verify_world;
	verify_world->map_id = 0;
	verify_world->position.x = -6240.32f;
	verify_world->position.y = 331.033f;
	verify_world->position.z = 382.758;
	verify_world->position.o = 0.f;
	ctx.send(verify_world);

	protocol::smsg_tutorial_flags tutorial_flags;
	ctx.send(tutorial_flags);

	protocol::smsg_update_object update_object;
	ctx.send(update_object);

	protocol::smsg_login_settimespeed time_speed;
	time_speed->speed = 0.01666667f;
	time_speed->time = get_time();
	ctx.send(time_speed);

	protocol::smsg_account_data_times adt;
	ctx.send(adt);

	/*protocol::smsg_weather weather;
	weather->change = weather->INSTANT;
	weather->grade = 1.f;
	weather->type = weather->RAIN;
	weather->sound_id = 8535;
	ctx.send(weather);*/

	protocol::smsg_messagechat motd;
	motd->language = 0;
	motd->type = protocol::server::SYSTEM;
	motd->message = "Welcome to a hacked together Ember test.";
	motd->player_guid = 0;
	motd->player_tag = protocol::server::TAG_NONE;
	ctx.send(motd);
}

void enter(ClientContext& ctx) {
	ctx.state_ctx = Context{};
}

void handle_name_query(ClientContext& ctx) {
	message_view<protocol::cmsg_name_query> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_name_query_response response{};
	response->name = "Chaosvex";
	response->guid = packet->guid;
	ctx.send(response);
}

void handle_active_mover(ClientContext& ctx) {
	message_view<protocol::cmsg_set_active_mover> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}
}

void handle_query_time(ClientContext& ctx) {
	protocol::cmsg_query_time packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_query_time_response response;
	response->time = get_time();
	ctx.send(response);
}

void handle_request_raid_info(ClientContext& ctx) {
	protocol::cmsg_request_raid_info packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_raid_instance_info response;
	ctx.send(response);
}

void handle_item_query(ClientContext& ctx) {
	message_view<protocol::cmsg_item_query_single> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_item_query_single_response response;
	response->item = packet->item;
	ctx.send(response);
}

void handle_mail_query(ClientContext& ctx) {
	protocol::msg_query_next_mail_time_c packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::msg_query_next_mail_time_s response;
	response->next_time = -1.f;
	ctx.send(response);
}

void handle_gmticket_getticket(ClientContext& ctx) {
	protocol::msg_query_next_mail_time_c packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_gmticket_getticket response;
	response->status = 0;
	ctx.send(response);
}

void handle_battlefield_status(ClientContext& ctx) {
	protocol::cmsg_battlefield_status packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_battlefield_status response;
	response->map = 0;
	response->position = 0;
	ctx.send(response);
}

void handle_meetingstone_info(ClientContext& ctx) {
	message_view<protocol::cmsg_meetingstone_info> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_meetingstone_setqueue response;
	response->area = 0;
	response->status = 5;
	ctx.send(response);
}

std::uint64_t packed_guid = 0;

void handle_move_time_skipped(ClientContext& ctx) {
	message_view<protocol::cmsg_move_time_skipped> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	// squirrel this away :)
	packed_guid = packet->guid;

	protocol::move_time_skipped_s response;
	response->guid = packet->guid;
	response->lag = packet->lag;
	ctx.send(response);
}

void handle_move_fall_land(ClientContext& ctx) {
	protocol::move_fall_land_c packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::move_fall_land_s response;
	response->guid = packed_guid;
	response->info = packet->info;
	ctx.send(response);
}

void handle_move_set_facing(ClientContext& ctx) {
	protocol::move_set_facing_c packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::msg_move_set_facing_s response;
	response->guid = packed_guid;
	response->info = packet->info;
	ctx.send(response);
}

void handle_zone_update(ClientContext& ctx) {
	message_view<protocol::cmsg_zone_update> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}
}

void handle_update_account_data(ClientContext& ctx) {
	message_view<protocol::cmsg_update_account_data> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}
}

void handle_join_channel(ClientContext& ctx) {
	message_view<protocol::cmsg_join_channel> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_channel_notify response;
	response->type = response->YOU_JOINED_NOTICE;
	response->name = packet->name;
	ctx.send(response);

	LOG_DEBUG(ctx.logger, "{}", response->name);
}

void handle_tutorial_flag(ClientContext& ctx) {
	protocol::cmsg_tutorial_flag packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}
}

// again, this entire file is just for testing other dev functionality
#ifdef _WIN32
std::shared_ptr<log::CommandSink> cmdsink;

void patch_console_test(ClientContext& ctx) {
	const auto sinks = ctx.logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		return;
	}

	cmdsink = dynamic_pointer_cast<log::CommandSink>(sinks.front());
}
#endif

// also temporary, if it's not clear - this needs to be properly hooked
// up to the actual command system at some point in the future, but not right now
bool handle_command(ClientContext& ctx, std::string_view message) try {
	message.remove_prefix(1); // remove the leading dot
	auto tokens = commands::parse_input(message);
	std::span token_span(tokens);

	if(tokens.empty()) {
		return false;
	}

#ifdef _WIN32
	if(token_span.front() == "console_stop") {
		cmdsink.reset();
		return true;
	}

	if(token_span.front() == "console") {
		if(!cmdsink) {
			patch_console_test(ctx);
		}

		if(!cmdsink) {
			return false;
		}

		// trim message
		if(!message.starts_with("console ")) {
			return false;
		}

		message = message.substr(8, -1);
		cmdsink->invoke(message);
		return true;
	}
#endif

	if(token_span.front() == "log") {
		token_span = token_span.subspan(1, -1);
		
		if(token_span.empty()) {
			return false;
		}

		if(token_span.front() == "stop") {
			ctx.handler().log_redirect_stop();
			return true;
		}

		const auto severity = log::severity_string(token_span.front());

		token_span = token_span.subspan(1, -1);

		if(token_span.empty()) {
			return false;
		}

		auto type = LogRedirect::Type::message;

		if(token_span.front() == "console") {
			type = LogRedirect::Type::console;
		} else if(token_span.front() == "chat") {
			type = LogRedirect::Type::message;
		} else {
			return false;
		}

		if(!ctx.packet_logging()) {
			ctx.handler().log_redirect(type, severity);
		} else {
			protocol::smsg_notification resp;
			resp->console = "Disable packet logging for yourself first - "
			                "this would wipe out time, forwards and backwards";
			ctx.send(resp);
			return false;
		}

		return true;
	}

	return false;
} catch(std::exception& e) {
	return false;
}

void handle_messagechat(ClientContext& ctx) {
	message_view<protocol::cmsg_messagechat> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	LOG_DEBUG(ctx.logger, "{}: {}", packet->destination, packet->message);

	if(packet->message.starts_with(".")) {
		if(!handle_command(ctx, packet->message)) {
			protocol::smsg_notification resp;
			resp->console = "Could not execute command";
			ctx.send(resp);
		}

		return;
	}

	protocol::smsg_messagechat response;
	response->type = (decltype(response->type))packet->type; // kek
	response->message = packet->message;
	response->language = 0; // universal, we don't have any learned skills in this test
	response->player_guid = packed_guid;
	response->player_tag = protocol::server::TAG_GM;
	
	if(packet->type == protocol::client::CHANNEL) {
		response->channel_name = packet->destination;
		response->player_rank = 14;
	} else {
		response->player_guid = packed_guid;
		response->chat_name_attr = packed_guid;
		response->speech_bubble_attr = packed_guid;
	}

	ctx.send(response);
}

void handle_logout_request(ClientContext& ctx) {
	protocol::smsg_character_login_failed response;
	response->reason = protocol::Result::char_login_no_world;
	ctx.send(response);
	ctx.state_update(ClientState::cs_character_list);
}

void handle_standstate_change(ClientContext& ctx) {
	message_view<protocol::cmsg_standstatechange> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::smsg_standstate_update response;
	response->state = packet->state;
	ctx.send(response);
}

void handle_world_teleport(ClientContext& ctx) {
	message_view<protocol::cmsg_world_teleport> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	LOG_DEBUG(ctx.logger, "Worldport, map: {}, x: {}, y: {}, z: {}, o: {}",
	          packet->map, packet->x, packet->y, packet->z, packet->o);

	protocol::smsg_new_world response;
	response->map_id = packet->map;
	response->position.x = packet->x;
	response->position.y = packet->y;
	response->position.z = packet->z;
	response->position.o = packet->o;
	ctx.send(response);

	protocol::smsg_update_object update_object;
	ctx.send(update_object);

}

void handle_move_set_raw_position(ClientContext& ctx) {
	message_view<protocol::cmsg_move_set_raw_position> packet;

	if(auto result = protocol::deserialise(packet, ctx.stream()); !result) {
		return ctx.stream_err(result);
	}

	protocol::msg_move_set_raw_position_ack response;
	ctx.send(response);
}

// everything in this file is for testing only
void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	using enum protocol::ClientOpcode;

	switch(opcode) {
		case cmsg_name_query:
			handle_name_query(ctx);
			break;
		case cmsg_set_active_mover:
			handle_active_mover(ctx);
			break;
		case cmsg_query_time:
			handle_query_time(ctx);
			break;
		case cmsg_request_raid_info:
			handle_request_raid_info(ctx);
			break;
		case cmsg_item_query_single:
			handle_item_query(ctx);
			break;
		case msg_query_next_mail_time:
			handle_mail_query(ctx);
			break;
		case cmsg_gmticket_getticket:
			handle_gmticket_getticket(ctx);
			break;
		case cmsg_battlefield_status:
			handle_battlefield_status(ctx);
			break;
		case cmsg_meetingstone_info:
			handle_meetingstone_info(ctx);
			break;
		case cmsg_move_time_skipped:
			handle_move_time_skipped(ctx);
			break;
		case msg_move_fall_land:
			handle_move_fall_land(ctx);
			break;
		case cmsg_zoneupdate:
			handle_zone_update(ctx);
			break;
		case cmsg_update_account_data:
			handle_update_account_data(ctx);
			break;
		case cmsg_join_channel:
			handle_join_channel(ctx);
			break;
		case msg_move_set_facing:
			handle_move_set_facing(ctx);
			break;
		case cmsg_tutorial_flag:
			handle_tutorial_flag(ctx);
			break;
		case cmsg_messagechat:
			handle_messagechat(ctx);
			break;
		case cmsg_logout_request:
			handle_logout_request(ctx);
			break;
		case cmsg_standstatechange:
			handle_standstate_change(ctx);
			break;
		case cmsg_world_teleport:
			handle_world_teleport(ctx);
			break;
		case cmsg_move_set_raw_position:
			handle_move_set_raw_position(ctx);
			break;
		default:
			break;
	}
}

void system_notification(ClientContext& ctx, std::string_view message) {
	protocol::smsg_notification packet;
	packet->console = message;
	ctx.send(packet);
}

void log_msg(ClientContext& ctx, const LogRedirect& event) {
	if(event.type == LogRedirect::Type::console) {
		system_notification(ctx, event.message);
		return;
	}

	protocol::smsg_messagechat msg;
	msg->language = 0;
	msg->type = protocol::server::SYSTEM;
	msg->message = event.message;
	msg->player_guid = 0;
	msg->player_tag = protocol::server::TAG_NONE;
	ctx.send(msg);
}

void system_msg(ClientContext& ctx, const SystemMessage& event) {
	if(event.type == SystemMessage::Type::console) {
		system_notification(ctx, event.message);
		return;
	}

	protocol::smsg_messagechat msg;
	msg->language = 0;

	if(event.type == SystemMessage::Type::whisper) {
		msg->type = protocol::server::MONSTER_WHISPER;
		msg->monster_name = "System";
		msg->message = event.message;
	} else {
		msg->type = protocol::server::SYSTEM;
		msg->message = std::format("[SERVER] {}", event.message);
	}

	msg->player_guid = 0;
	msg->player_tag = protocol::server::TAG_NONE;
	ctx.send(msg);
}

void handle_event(ClientContext& ctx, const Event& event) {
	using enum EventType;

    switch(event.type) {
        case player_login:
            initiate_player_login(ctx, event.as<PlayerLogin>());
            break;
		case system_message:
			system_msg(ctx, event.as<SystemMessage>());
			break;
		case log_redirect:
			log_msg(ctx, event.as<LogRedirect>());
			break;
        default:
            break;
    }
}

void exit(ClientContext& ctx) {
	// nothing to do
}

} // world_enter, realm, ember