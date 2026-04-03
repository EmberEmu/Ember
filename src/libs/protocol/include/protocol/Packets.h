/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packet.h"
#include "Opcodes.h"
#include <protocol/server/AuthChallenge.h>
#include <protocol/server/AuthResponse.h>
#include <protocol/server/Pong.h>
#include <protocol/server/CharacterEnum.h>
#include <protocol/server/CharacterCreate.h>
#include <protocol/server/CharacterDelete.h>
#include <protocol/server/CharacterRename.h>
#include <protocol/server/CharacterLoginFailed.h>
#include <protocol/server/LogoutComplete.h>
#include <protocol/server/AddonInfo.h>
#include <protocol/server/LoginVerifyWorld.h>
#include <protocol/server/TutorialFlags.h>
#include <protocol/server/UpdateObject.h>
#include <protocol/server/NameQueryResponse.h>
#include <protocol/server/SplineMoveSetWalkMode.h>
#include <protocol/server/QueryTimeResponse.h>
#include <protocol/server/SetLoginTimeSpeed.h>
#include <protocol/server/RaidInstanceInfo.h>
#include <protocol/server/ItemQuerySingleResponse.h>
#include <protocol/server/QueryNextMailTime.h>
#include <protocol/server/GMTicketGetTicket.h>
#include <protocol/server/BattlefieldStatus.h>
#include <protocol/server/MeetingStoneSetQueue.h>
#include <protocol/server/Motd.h>
#include <protocol/server/MoveTimeSkipped.h>
#include <protocol/server/MoveFallLand.h>
#include <protocol/server/ChannelNotify.h>
#include <protocol/server/Weather.h>
#include <protocol/server/AccountDataTimes.h>
#include <protocol/server/GenericMove.h>
#include <protocol/server/MessageChat.h>
#include <protocol/server/TriggerCinematic.h>
#include <protocol/server/StandStateUpdate.h>
#include <protocol/server/Notification.h>
#include <protocol/server/MoveSetRawPositionAck.h>
#include <protocol/server/NewWorld.h>
#include <protocol/client/AuthSession.h>
#include <protocol/client/Ping.h>
#include <protocol/client/CharacterCreate.h>
#include <protocol/client/CharacterDelete.h>
#include <protocol/client/CharacterEnum.h>
#include <protocol/client/CharacterRename.h>
#include <protocol/client/PlayerLogin.h>
#include <protocol/client/NameQuery.h>
#include <protocol/client/SetActiveMover.h>
#include <protocol/client/QueryTime.h>
#include <protocol/client/RequestRaidInfo.h>
#include <protocol/client/ItemQuerySingle.h>
#include <protocol/client/QueryNextMailTime.h>
#include <protocol/client/GMTicketGetTicket.h>
#include <protocol/client/BattlefieldStatus.h>
#include <protocol/client/MeetingStoneInfo.h>
#include <protocol/client/MoveTimeSkipped.h>
#include <protocol/client/MoveFallLand.h>
#include <protocol/client/ZoneUpdate.h>
#include <protocol/client/UpdateAccountData.h>
#include <protocol/client/JoinChannel.h>
#include <protocol/client/GenericMove.h>
#include <protocol/client/TutorialFlag.h>
#include <protocol/client/MessageChat.h>
#include <protocol/client/StandStateChange.h>
#include <protocol/client/WorldTeleport.h>
#include <protocol/client/MoveSetRawPosition.h>

namespace ember::protocol {

using smsg_addon_info                 = ServerPacket<ServerOpcode::smsg_addon_info,                 server::AddonInfo>;
using smsg_auth_challenge             = ServerPacket<ServerOpcode::smsg_auth_challenge,             server::AuthChallenge>;
using smsg_auth_response              = ServerPacket<ServerOpcode::smsg_auth_response,              server::AuthResponse>;
using smsg_pong                       = ServerPacket<ServerOpcode::smsg_pong,                       server::Pong>;
using smsg_char_enum                  = ServerPacket<ServerOpcode::smsg_char_enum,                  server::CharacterEnum>;
using smsg_char_create                = ServerPacket<ServerOpcode::smsg_char_create,                server::CharacterCreate>;
using smsg_char_delete                = ServerPacket<ServerOpcode::smsg_char_delete,                server::CharacterDelete>;
using smsg_char_rename                = ServerPacket<ServerOpcode::smsg_char_rename,                server::CharacterRename>;
using smsg_character_login_failed     = ServerPacket<ServerOpcode::smsg_character_login_failed,     server::CharacterLoginFailed>;
using smsg_logout_complete            = ServerPacket<ServerOpcode::smsg_logout_complete,            server::LogoutComplete>;
using smsg_login_verify_world         = ServerPacket<ServerOpcode::smsg_login_verify_world,         server::LoginVerifyWorld>;
using smsg_tutorial_flags             = ServerPacket<ServerOpcode::smsg_tutorial_flags,             server::TutorialFlags>;
using smsg_update_object              = ServerPacket<ServerOpcode::smsg_update_object,              server::UpdateObject>;
using smsg_name_query_response        = ServerPacket<ServerOpcode::smsg_name_query_response,        server::NameQueryResponse>;
using smsg_spline_move_set_walk_mode  = ServerPacket<ServerOpcode::smsg_spline_move_set_walk_mode,  server::SplineMoveSetWalkMode>;
using smsg_query_time_response        = ServerPacket<ServerOpcode::smsg_query_time_response,        server::QueryTimeResponse>;
using smsg_login_settimespeed         = ServerPacket<ServerOpcode::smsg_login_settimespeed,         server::SetLoginTimeSpeed>;
using smsg_raid_instance_info         = ServerPacket<ServerOpcode::smsg_raid_instance_info,         server::RaidInstanceInfo>;
using smsg_item_query_single_response = ServerPacket<ServerOpcode::smsg_item_query_single_response, server::ItemQuerySingleResponse>;
using msg_query_next_mail_time_s      = ServerPacket<ServerOpcode::msg_query_next_mail_time,        server::QueryNextMailTime>;
using smsg_gmticket_getticket         = ServerPacket<ServerOpcode::smsg_gmticket_getticket,         server::GMTicketGetTicket>;
using smsg_battlefield_status         = ServerPacket<ServerOpcode::smsg_battlefield_status,         server::BattlefieldStatus>;
using smsg_meetingstone_setqueue      = ServerPacket<ServerOpcode::smsg_meetingstone_setqueue,      server::MeetingStoneSetQueue>;
using smsg_motd                       = ServerPacket<ServerOpcode::smsg_motd,                       server::Motd>;
using move_time_skipped_s             = ServerPacket<ServerOpcode::msg_move_time_skipped,           server::MoveTimeSkipped>;
using move_fall_land_s                = ServerPacket<ServerOpcode::msg_move_fall_land,              server::MoveFallLand>;
using smsg_weather                    = ServerPacket<ServerOpcode::smsg_weather,                    server::Weather>;
using smsg_account_data_times         = ServerPacket<ServerOpcode::smsg_account_data_times,         server::AccountDataTimes>;
using smsg_channel_notify             = ServerPacket<ServerOpcode::smsg_channel_notify,             server::ChannelNotify>;
using msg_move_set_facing_s           = ServerPacket<ServerOpcode::msg_move_set_facing,             server::GenericMove>;
using smsg_messagechat                = ServerPacket<ServerOpcode::smsg_messagechat,                server::MessageChat>;
using smsg_trigger_cinematic          = ServerPacket<ServerOpcode::smsg_trigger_cinematic,          server::TriggerCinematic>;
using smsg_standstate_update          = ServerPacket<ServerOpcode::smsg_standstate_update,          server::StandStateUpdate>;
using smsg_notification               = ServerPacket<ServerOpcode::smsg_notification,               server::Notification>;
using msg_move_set_raw_position_ack   = ServerPacket<ServerOpcode::msg_move_set_raw_position_ack,   server::MoveSetRawPositionAck>;
using smsg_new_world                  = ServerPacket<ServerOpcode::smsg_new_world,                  server::NewWorld>;

using cmsg_auth_session           = ClientPacket<ClientOpcode::cmsg_auth_session,          client::AuthSession>;
using cmsg_ping                   = ClientPacket<ClientOpcode::cmsg_ping,                  client::Ping>;
using cmsg_char_create            = ClientPacket<ClientOpcode::cmsg_char_create,           client::CharacterCreate>;
using cmsg_char_delete            = ClientPacket<ClientOpcode::cmsg_char_delete,           client::CharacterDelete>;
using cmsg_char_enum              = ClientPacket<ClientOpcode::cmsg_char_enum,             client::CharacterEnum>;
using cmsg_char_rename            = ClientPacket<ClientOpcode::cmsg_char_rename,           client::CharacterRename>;
using cmsg_player_login           = ClientPacket<ClientOpcode::cmsg_player_login,          client::PlayerLogin>;
using cmsg_name_query             = ClientPacket<ClientOpcode::cmsg_name_query,            client::NameQuery>;
using cmsg_set_active_mover       = ClientPacket<ClientOpcode::cmsg_set_active_mover,      client::SetActiveMover>;
using cmsg_query_time             = ClientPacket<ClientOpcode::cmsg_query_time,            client::QueryTime>;
using cmsg_request_raid_info      = ClientPacket<ClientOpcode::cmsg_request_raid_info,     client::RequestRaidInfo>;
using cmsg_item_query_single      = ClientPacket<ClientOpcode::cmsg_item_query_single,     client::ItemQuerySingle>;
using msg_query_next_mail_time_c  = ClientPacket<ClientOpcode::msg_query_next_mail_time,   client::QueryNextMailTime>;
using cmsg_gmticket_getticket     = ClientPacket<ClientOpcode::cmsg_gmticket_getticket,    client::GMTicketGetTicket>;
using cmsg_battlefield_status     = ClientPacket<ClientOpcode::cmsg_battlefield_status,    client::BattlefieldStatus>;
using cmsg_meetingstone_info      = ClientPacket<ClientOpcode::cmsg_meetingstone_info,     client::MeetingStoneInfo>;
using cmsg_move_time_skipped      = ClientPacket<ClientOpcode::cmsg_move_time_skipped,     client::MoveTimeSkipped>;
using move_fall_land_c            = ClientPacket<ClientOpcode::msg_move_fall_land,         client::MoveFallLand>;
using cmsg_zone_update            = ClientPacket<ClientOpcode::cmsg_zoneupdate,            client::ZoneUpdate>;
using cmsg_update_account_data    = ClientPacket<ClientOpcode::cmsg_update_account_data,   client::UpdateAccountData>;
using cmsg_join_channel           = ClientPacket<ClientOpcode::cmsg_join_channel,          client::JoinChannel>;
using move_set_facing_c           = ClientPacket<ClientOpcode::msg_move_set_facing,        client::GenericMove>;
using cmsg_tutorial_flag          = ClientPacket<ClientOpcode::cmsg_tutorial_flag,         client::TutorialFlag>;
using cmsg_messagechat            = ClientPacket<ClientOpcode::cmsg_messagechat,           client::MessageChat>;
using cmsg_standstatechange       = ClientPacket<ClientOpcode::cmsg_standstatechange,      client::StandStateChange>;
using cmsg_move_set_raw_position  = ClientPacket<ClientOpcode::cmsg_move_set_raw_position, client::MoveSetRawPosition>;
using cmsg_world_teleport         = ClientPacket<ClientOpcode::cmsg_world_teleport,        client::WorldTeleport>;

} // protocol, ember