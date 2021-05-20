/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <boost/endian.hpp>
#include <array>
#include <string_view>
#include <variant>
#include <cstdint>

namespace ember::dns {

/*
 * Controls the maximum allowable datagram size
 * 
 * This does not take the MTU into consideration,
 * so fragmentation may occur before hitting these
 * limits.
 */
constexpr auto UDP_HDR_SIZE = 8u;
constexpr auto IPV4_HDR_SIZE = 20u;
constexpr auto IPV6_HDR_SIZE = 40u;

/* 
 * rfc6762 s17
 * Even when fragmentation is used, a Multicast DNS packet, including IP
 * and UDP headers, MUST NOT exceed 9000 bytes.
 */
constexpr auto MAX_DGRAM_LEN = 9000;
constexpr auto MAX_DGRAM_PAYLOAD_IPV4 = MAX_DGRAM_LEN - (UDP_HDR_SIZE + IPV4_HDR_SIZE);
constexpr auto MAX_DGRAM_PAYLOAD_IPV6 = MAX_DGRAM_LEN - (UDP_HDR_SIZE + IPV6_HDR_SIZE);


struct Srv;
struct Txt;
struct Ptr;

using RecordData = std::variant<Ptr, Srv, Txt>;
using big_uint16_t = boost::endian::big_uint16_at;

constexpr auto DNS_HDR_SIZE = 12;

constexpr auto QR_OFFSET     = 0;
constexpr auto OPCODE_OFFSET = 1;
constexpr auto AA_OFFSET     = 5;
constexpr auto TC_OFFSET     = 6;
constexpr auto RD_OFFSET     = 7;
constexpr auto RA_OFFSET     = 8;
constexpr auto Z_OFFSET      = 9;
constexpr auto AD_OFFSET     = 10;
constexpr auto CD_OFFSET     = 11;
constexpr auto RCODE_OFFSET  = 12;

constexpr auto QR_MASK     = 0x01 << QR_OFFSET;
constexpr auto OPCODE_MASK = 0x0F << OPCODE_OFFSET;
constexpr auto AA_MASK     = 0x01 << AA_OFFSET;
constexpr auto TC_MASK     = 0x01 << TC_OFFSET;
constexpr auto RD_MASK     = 0x01 << RD_OFFSET;
constexpr auto RA_MASK     = 0x01 << RA_OFFSET;
constexpr auto Z_MASK      = 0x01 << Z_OFFSET;
constexpr auto AD_MASK     = 0x01 << AD_OFFSET;
constexpr auto CD_MASK     = 0x01 << CD_OFFSET;
constexpr auto RCODE_MASK  = 0x0F << RCODE_OFFSET;

constexpr auto NOTATION_OFFSET = 0x06;
constexpr auto NOTATION_STR = 0x00;
constexpr auto NOTATION_PTR = 0x03;

enum class QR {
    QUERY, REPLY
};

enum class ResCode {
    NOERROR, FORMERR, SERVFAIL, NXDOMAIN
};

smart_enum_class(RecordType, std::uint16_t,
	A          = 1,
	AAAA       = 28,
	AFSDB      = 18,
	APL        = 42,
	CAA        = 257,
	CDNSKEY    = 60,
	CDS        = 59,
	CERT       = 47,
	CNAME      = 5,
	DHCID      = 49,
	DLV        = 32769,
	DNAME      = 39,
	DNSKEY     = 48,
	DS         = 43,
	HIP        = 55,
	IPSECKEY   = 45,
	KEY        = 25,
	KX         = 36,
	LOC        = 29,
	MX         = 15,
	NAPTR      = 35,
	NS         = 2,
	NSEC       = 47,
	NSEC3      = 50,
	NSEC3PARAM = 51,
	OPENPGPKEY = 61,
	PTR        = 12,
	RRSIG      = 46,
	RP         = 17,
	SIG        = 24,
	SOA        = 6,
	SRV        = 33,
	SSHFP      = 44,
	TA         = 32769,
	TKEY       = 249,
	TLSA       = 52,
	TSIG       = 250,
	TXT        = 16,
	URI        = 256,

	// misc
	ALL        = 255,
	AXFR       = 252,
	IXFR       = 251,
	OPT        = 41,

	// obsolete
	MD         = 3,
	MF         = 4,
	MAILA      = 254,
	MB         = 7,
	MG         = 8,
	MR         = 9,
	MINFO      = 14,
	MAILB      = 253,
	WKS        = 11,
	NB         = 32,
	NBSTAT     = 33,
	NULL_      = 10,
	A6         = 38,
	NXT        = 30,
	KEY_       = 25,
	SIG_       = 24,
	HINFO      = 13,
	RP_        = 17,
	X25        = 19,
	ISDN       = 20,
	RT         = 21,
	NSAP       = 22,
	NSAP_PTR   = 23,
	PX         = 26,
	EID        = 31,
	NIMLOC     = 32,
	ATMA       = 34,
	APL_       = 42,
	SINK       = 40,
	GPOS       = 27,
	UINFO      = 100,
	UID        = 101,
	GID        = 102,
	UNSPEC     = 103,
	SPF        = 99
)

smart_enum_class(Class, std::uint16_t,
	CLASS_IN = 1, // Internet
	CLASS_CS = 2, // CSNET, obsolete
	CLASS_CH = 3, // Chaos
	CLASS_HS = 4, // Hesiod
)

struct Ptr {

};

struct Txt { 
    std::string key;
    std::string value;
};

struct Srv {
    std::string service;
    std::string protocol;
    std::string name;
    std::uint32_t ttl;
	Class ccode;
    std::uint16_t priority;
    std::uint16_t weight;
    std::uint16_t port;
    std::string host;
};

smart_enum_class(Opcode, std::uint16_t,
	STANDARD_QUERY, IQUERY, STATUS
)

smart_enum_class(ReplyCode, std::uint16_t,
	REPLY_NO_ERROR, FORMAT_ERROR,
	SERVER_FAILURE, NAME_ERROR,
	NOT_IMPLEMENTED, REFUSED
)

/* 
 * This whole thing is really 16 bits on the wire but
 * keeping it as int32 removes some obnoxious casting
*/
struct Flags {
	std::int32_t qr; // response
	Opcode opcode; // opcode
	std::int32_t aa; // authoritative
	std::int32_t tc; // truncated
	std::int32_t rd; // recursion_desired
	std::int32_t ra; // recursion_available
	std::int32_t z; // reserved
	std::int32_t ad; // answer_authenticated
	std::int32_t cd; // non_auth_unacceptable
	ReplyCode rcode; // reply_code
};

struct Header {
    std::uint16_t id;
	Flags flags;
	std::uint16_t questions;
	std::uint16_t answers;
	std::uint16_t authority_rrs;
	std::uint16_t additional_rrs;
};

struct Answer {
    std::string name;
	RecordType type;
	Class ccode;
    std::uint32_t ttl;
    std::uint16_t rdlen;
    RecordData record;
};

struct Question {
    std::string name;
	RecordType type;
	Class cc;
};

struct RecordEntry {
	std::string name;
	//boost::asio::ip::address answer; todo
	RecordType type;
	std::uint32_t ttl;
};

struct Record_Authority {
	std::string master_name;
	std::string responsible_name;
	std::uint32_t serial;
	std::uint32_t refresh_interval;
	std::uint32_t retry_interval;
	std::uint32_t expire_interval;
	std::uint32_t negative_caching_ttl;
};

struct Record_A {
	std::uint32_t ip;
};

struct Record_AAAA {
	std::array<unsigned char, 16> ip;
};

struct ResourceRecord {
	std::string name;
	RecordType type;
	Class resource_class;
	std::uint32_t ttl;
	std::uint16_t rdata_len;
	std::variant<
		Record_A, Record_AAAA,
		Record_Authority
	> rdata;
};

struct Query {
	Header header;
	std::vector<Question> questions;
	std::vector<ResourceRecord> answers;
	std::vector<ResourceRecord> authorities;
	std::vector<ResourceRecord> additional;
};

} // dns, ember