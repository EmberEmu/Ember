/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdint>

namespace ember { namespace opcodes {

#pragma pack(push, 1)

enum class CLIENT : std::uint8_t {
	CMSG_LOGIN_CHALLENGE,
	CMSG_LOGIN_PROOF,
	CMSG_RECONNECT_CHALLENGE,
	CMSG_RECONNECT_PROOF,
	CMSG_REQUEST_REALM__LIST = 0x10,
    SMSG_TRANSFER_INITIATE   = 0x30,
    SMSG_TRANSFER_DATA
};

enum class Result : std::uint8_t {
    WOW_SUCCESS                     = 0x00,
    WOW_FAIL_UNKNOWN0               = 0x01,                 ///< ? Unable to connect
    WOW_FAIL_UNKNOWN1               = 0x02,                 ///< ? Unable to connect
    WOW_FAIL_BANNED                 = 0x03,                 ///< This <game> account has been closed and is no longer available for use. Please go to <site>/banned.html for further information.
    WOW_FAIL_UNKNOWN_ACCOUNT        = 0x04,                 ///< The information you have entered is not valid. Please check the spelling of the account name and password. If you need help in retrieving a lost or stolen password, see <site> for more information
    WOW_FAIL_INCORRECT_PASSWORD     = 0x05,                 ///< The information you have entered is not valid. Please check the spelling of the account name and password. If you need help in retrieving a lost or stolen password, see <site> for more information
    // client reject next login attempts after this error, so in code used WOW_FAIL_UNKNOWN_ACCOUNT for both cases
    WOW_FAIL_ALREADY_ONLINE         = 0x06,                 ///< This account is already logged into <game>. Please check the spelling and try again.
    WOW_FAIL_NO_TIME                = 0x07,                 ///< You have used up your prepaid time for this account. Please purchase more to continue playing
    WOW_FAIL_DB_BUSY                = 0x08,                 ///< Could not log in to <game> at this time. Please try again later.
    WOW_FAIL_VERSION_INVALID        = 0x09,                 ///< Unable to validate game version. This may be caused by file corruption or interference of another program. Please visit <site> for more information and possible solutions to this issue.
    WOW_FAIL_VERSION_UPDATE         = 0x0A,                 ///< Downloading
    WOW_FAIL_INVALID_SERVER         = 0x0B,                 ///< Unable to connect
    WOW_FAIL_SUSPENDED              = 0x0C,                 ///< This <game> account has been temporarily suspended. Please go to <site>/banned.html for further information
    WOW_FAIL_FAIL_NOACCESS          = 0x0D,                 ///< Unable to connect
    WOW_SUCCESS_SURVEY              = 0x0E,                 ///< Connected.
    WOW_FAIL_PARENTCONTROL          = 0x0F,                 ///< Access to this account has been blocked by parental controls. Your settings may be changed in your account preferences at <site>
    WOW_FAIL_LOCKED_ENFORCED        = 0x10,                 ///< You have applied a lock to your account. You can change your locked status by calling your account lock phone number.
    WOW_FAIL_TRIAL_ENDED            = 0x11,                 ///< Your trial subscription has expired. Please visit <site> to upgrade your account.
    WOW_FAIL_USE_BATTLENET          = 0x12,                 ///< WOW_FAIL_OTHER This account is now attached to a Battle.net account. Please login with your Battle.net account email address and password.
};

struct ClientLoginProof {
	CLIENT opcode;
	std::uint8_t A[32];
	std::uint8_t M1[20];
	std::uint8_t crc_hash[20];
	std::uint8_t key_count;
	std::uint8_t unknown;
};

struct ClientLoginChallenge {
	struct Header {
		CLIENT opcode;
		Result error;
		std::uint16_t size;
	} header;
	std::uint8_t game[4];
	std::uint8_t major;
	std::uint8_t minor;
	std::uint8_t patch;
	std::uint16_t build;
	std::uint8_t platform[4];
	std::uint8_t os[4];
	std::uint8_t country[4];
	std::uint32_t timezone_bias;
	std::uint32_t ip;
	std::uint8_t username_len;
	char username[1];
};

struct ServerLoginChallenge {
	CLIENT opcode;
	Result error;
	std::uint8_t unk2;
	std::uint8_t B[32];
	std::uint8_t g_len;
	std::uint8_t g;
	std::uint8_t N_len;
	std::uint8_t N[32];
	std::uint8_t s[32];
	std::uint8_t unk3[16];
	std::uint8_t unk4;
};

#pragma pack(pop)

}} //opcodes, ember