/*
 * Copyright (c) 2020 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldEnter.h"
#include "ClientContext.h"
#include "../ClientConnection.h"
#include "../Events.h"
#include <protocol/Packets.h>

#include <chrono>
#include <ctime>

namespace ember::gateway::world_enter {

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

void initiate_player_login(ClientContext& ctx, const PlayerLogin* event) {
    auto& state_ctx = std::get<Context>(ctx.state_ctx);
    state_ctx.character_id = event->character_id_;

	protocol::smsg_login_verify_world verify_world;
	verify_world->map_id = 0;
	verify_world->position.x = -6240.32f;
	verify_world->position.y = 331.033f;
	verify_world->position.z = 382.758;
	verify_world->position.o = 0.f;
	ctx.connection.send(verify_world);

	protocol::smsg_tutorial_flags tutorial_flags;
	ctx.connection.send(tutorial_flags);

	protocol::smsg_update_object update_object;
	ctx.connection.send(update_object);

	protocol::smsg_login_settimespeed time_speed;
	time_speed->speed = 0.f;
	time_speed->time = get_time();
	ctx.connection.send(time_speed);

	protocol::smsg_account_data_times adt;
	ctx.connection.send(adt);

	protocol::smsg_weather weather;
	weather->change = weather->INSTANT;
	weather->grade = 1.f;
	weather->type = weather->RAIN;
	weather->sound_id = 8535;
	ctx.connection.send(weather);

	protocol::smsg_trigger_cinematic cinematic;
	cinematic->id = 81;
	ctx.connection.send(cinematic);

	protocol::smsg_messagechat motd;
	motd->language = 0;
	motd->type = protocol::server::SYSTEM;
	motd->message = "Welcome to a hacked together Ember test. Does this spell the end of the 'world wen' memes?";
	motd->player_guid = 0;
	motd->player_tag = protocol::server::TAG_NONE;
	ctx.connection.send(motd);
}

void enter(ClientContext& ctx) {
    ctx.state_ctx = Context{};
}

void handle_name_query(ClientContext& ctx) {
	protocol::cmsg_name_query packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	protocol::smsg_name_query_response response{};
	response->name = "Chaosvex";
	response->guid = packet->guid;
	ctx.connection.send(response);
}

void handle_active_mover(ClientContext& ctx) {
	protocol::cmsg_set_active_mover packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	/*protocol::smsg_spline_move_set_walk_mode response;
	response->guid = packet->guid;*/
	//ctx.connection.send(response);
}

void handle_query_time(ClientContext& ctx) {
	protocol::cmsg_set_active_mover packet;

	//if(!ctx.handler.deserialise(packet, *ctx.stream)) {
	//	return;
	//}

	protocol::smsg_query_time_response response;
	response->time = get_time();
	ctx.connection.send(response);
}

void handle_request_raid_info(ClientContext& ctx) {
	protocol::cmsg_request_raid_info packet;

	//if(!ctx.handler.deserialise(packet, *ctx.stream)) {
	//	return;
	//}

	protocol::smsg_raid_instance_info response;
	ctx.connection.send(response);
}

void handle_item_query(ClientContext& ctx) {
	protocol::cmsg_item_query_single packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	protocol::smsg_item_query_single_response response;
	response->item = packet->item;
	ctx.connection.send(response);
}

void handle_mail_query(ClientContext& ctx) {
	protocol::msg_query_next_mail_time_c packet;

	/*if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}*/

	protocol::msg_query_next_mail_time_s response;
	response->next_time = -1.f;
	ctx.connection.send(response);
}

void handle_gmticket_getticket(ClientContext& ctx) {
	protocol::msg_query_next_mail_time_c packet;

	/*if(!ctx.handler.deserialise(packet, *ctx.stream)) {
	return;
	}*/

	protocol::smsg_gmticket_getticket response;
	response->status = 0;
	ctx.connection.send(response);
}

void handle_battlefield_status(ClientContext& ctx) {
	protocol::cmsg_battlefield_status packet;

	/*if(!ctx.handler.deserialise(packet, *ctx.stream)) {
	return;
	}*/

	protocol::smsg_battlefield_status response;
	response->map = 0;
	response->position = 0;
	ctx.connection.send(response);
}

void handle_meetingstone_info(ClientContext& ctx) {
	protocol::cmsg_meetingstone_info packet;

	/*if(!ctx.handler.deserialise(packet, *ctx.stream)) {
	return;
	}*/

	protocol::smsg_meetingstone_setqueue response;
	response->area = 0;
	response->status = 5;
	ctx.connection.send(response);
}

std::uint64_t packed_guid = 0;

void handle_move_time_skipped(ClientContext& ctx) {
	protocol::cmsg_move_time_skipped packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	// squirrel this away :)
	packed_guid = packet->guid;

	protocol::move_time_skipped_s response;
	response->guid = packet->guid;
	response->lag = packet->lag;
	ctx.connection.send(response);
}

void handle_move_fall_land(ClientContext& ctx) {
	protocol::move_fall_land_c packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	protocol::move_fall_land_s response;
	response->guid = packed_guid;
	response->info = packet->info;
	ctx.connection.send(response);
}

void handle_move_set_facing(ClientContext& ctx) {
	protocol::move_set_facing_c packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	protocol::msg_move_set_facing_s response;
	response->guid = packed_guid;
	response->info = packet->info;
	ctx.connection.send(response);
}

void handle_zone_update(ClientContext& ctx) {
	protocol::cmsg_zone_update packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}
}

void handle_update_account_data(ClientContext& ctx) {
	protocol::cmsg_update_account_data packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	/*protocol::smsg_account_data_times adt;
	ctx.connection.send(adt);*/
}

void handle_join_channel(ClientContext& ctx) {
	protocol::cmsg_join_channel packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	protocol::smsg_channel_notify response;
	response->type = response->YOU_JOINED_NOTICE;
	response->name = packet->name;
	ctx.connection.send(response);

	LOG_DEBUG_ASYNC(ctx.logger, "{}", response->name);

	protocol::smsg_channel_notify response2;
	response2->type = response->LEFT_NOTICE;
	response2->name = "LocalDefense - Elwynn Forest";
	ctx.connection.send(response2);
}

void handle_tutorial_flag(ClientContext& ctx) {
	protocol::cmsg_tutorial_flag packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}
}

void handle_messagechat(ClientContext& ctx) {
	protocol::cmsg_messagechat packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	LOG_DEBUG_ASYNC(ctx.logger, "{}", packet->message);

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

	ctx.connection.send(response);
}

void handle_logout_request(ClientContext& ctx) {
	ctx.handler.skip(*ctx.stream);

	protocol::smsg_character_login_failed response;
	response->reason = 0x3e;
	ctx.connection.send(response);
	ctx.handler.state_update(ClientState::cs_character_list);
}

// everything in this file is for testing only
void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	switch(opcode) {
		case protocol::ClientOpcode::cmsg_name_query:
			handle_name_query(ctx);
			break;
		case protocol::ClientOpcode::cmsg_set_active_mover:
			handle_active_mover(ctx);
			break;
		case protocol::ClientOpcode::cmsg_query_time:
			handle_query_time(ctx);
			break;
		case protocol::ClientOpcode::cmsg_request_raid_info:
			handle_request_raid_info(ctx);
			break;
		case protocol::ClientOpcode::cmsg_item_query_single:
			handle_item_query(ctx);
			break;
		case protocol::ClientOpcode::msg_query_next_mail_time:
			handle_mail_query(ctx);
			break;
		case protocol::ClientOpcode::cmsg_gmticket_getticket:
			handle_gmticket_getticket(ctx);
			break;
		case protocol::ClientOpcode::cmsg_battlefield_status:
			handle_battlefield_status(ctx);
			break;
		case protocol::ClientOpcode::cmsg_meetingstone_info:
			handle_meetingstone_info(ctx);
			break;
		case protocol::ClientOpcode::cmsg_move_time_skipped:
			handle_move_time_skipped(ctx);
			break;
		case protocol::ClientOpcode::msg_move_fall_land:
			handle_move_fall_land(ctx);
			break;
		case protocol::ClientOpcode::cmsg_zoneupdate:
			handle_zone_update(ctx);
			break;
		case protocol::ClientOpcode::cmsg_update_account_data:
			handle_update_account_data(ctx);
			break;
		case protocol::ClientOpcode::cmsg_join_channel:
			handle_join_channel(ctx);
			break;
		case protocol::ClientOpcode::msg_move_set_facing:
			handle_move_set_facing(ctx);
			break;
		case protocol::ClientOpcode::cmsg_tutorial_flag:
			handle_tutorial_flag(ctx);
			break;
		case protocol::ClientOpcode::cmsg_messagechat:
			handle_messagechat(ctx);
			break;
		case protocol::ClientOpcode::cmsg_logout_request:
			handle_logout_request(ctx);
			break;
		default:
			ctx.handler.skip(*ctx.stream);
	}
}

void handle_event(ClientContext& ctx, const Event* event) {
    switch(event->type) {
        case EventType::player_login:
            initiate_player_login(ctx, static_cast<const PlayerLogin*>(event));
            break;
        default:
            break;
    }
}

void exit(ClientContext& ctx) {

}

} // world_enter, gateway, ember