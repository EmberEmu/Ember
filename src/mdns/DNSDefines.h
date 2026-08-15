/*
 * Copyright (c) 2021 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <array>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cstdint>

namespace ember::dns {

/*
 * Controls the maximum allowable datagram size
 * 
 * This does not take the MTU into consideration,
 * so fragmentation may occur before hitting these
 * limits.
 */
constexpr auto UDP_HDR_SIZE  = 8u;
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

constexpr auto UNICAST_RESP_OFFSET = 0x0F;
constexpr auto UNICAST_RESP_MASK = 0x01 << UNICAST_RESP_OFFSET;

enum class QR {
    query,
	reply
};

enum class ResCode {
    rnoerror,
	formerr,
	servfail,
	nxdomain
};

smart_enum_class(RecordType, std::uint16_t,
	a          = 1,
	aaaa       = 28,
	afsdb      = 18,
	apl        = 42,
	caa        = 257,
	cdnskey    = 60,
	cds        = 59,
	cert       = 47,
	cname      = 5,
	dhcid      = 49,
	dlv        = 32769,
	dname      = 39,
	dnskey     = 48,
	ds         = 43,
	hip        = 55,
	ipseckey   = 45,
	key        = 25,
	kx         = 36,
	loc        = 29,
	mx         = 15,
	naptr      = 35,
	ns         = 2,
	nsec       = 47,
	nsec3      = 50,
	nsec3param = 51,
	openpgpkey = 61,
	ptr        = 12,
	rrsig      = 46,
	rp         = 17,
	sig        = 24,
	soa        = 6,
	srv        = 33,
	sshfp      = 44,
	ta         = 32769,
	tkey       = 249,
	tlsa       = 52,
	tsig       = 250,
	txt        = 16,
	uri        = 256,

	// misc
	all        = 255,
	axfr       = 252,
	ixfr       = 251,
	opt        = 41,

	// obsolete
	md         = 3,
	mf         = 4,
	maila      = 254,
	mb         = 7,
	mg         = 8,
	mr         = 9,
	minfo      = 14,
	mailb      = 253,
	wks        = 11,
	nb         = 32,
	// nbstat     = 33,
	null_      = 10,
	a6         = 38,
	nxt        = 30,
	key_       = 25,
	sig_       = 24,
	hinfo      = 13,
	rp_        = 17,
	x25        = 19,
	isdn       = 20,
	rt         = 21,
	nsap       = 22,
	nsap_ptr   = 23,
	px         = 26,
	eid        = 31,
	nimloc     = 32,
	atma       = 34,
	apl_       = 42,
	sink       = 40,
	gpos       = 27,
	uinfo      = 100,
	uid        = 101,
	gid        = 102,
	unspec     = 103,
	spf        = 99
)

smart_enum_class(Class, std::uint16_t,
	class_in = 1, // Internet
	class_cs = 2, // CSNET, obsolete
	class_ch = 3, // Chaos
	class_hs = 4, // Hesiod
	class_any = 255
)

smart_enum_class(Opcode, std::uint16_t,
	standard_query,
	iquery,
	status
)

smart_enum_class(ReplyCode, std::uint16_t,
	reply_no_error,
	format_error,
	server_failure,
	name_error,
	not_implemented,
	refused
)

/* 
 * This whole thing is really 16 bits on the wire but
 * keeping it as int32 removes some obnoxious casting
*/
struct Flags {
	std::int32_t qr; // response
	Opcode opcode;   // opcode
	std::int32_t aa; // authoritative
	std::int32_t tc; // truncated
	std::int32_t rd; // recursion_desired
	std::int32_t ra; // recursion_available
	std::int32_t z;  // reserved
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

struct QMeta {
	bool accepts_unicast_response;
	std::vector<std::string_view> labels;
};

struct Question {
    std::string name;
	RecordType type;
	Class cc;
	QMeta meta;
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

struct Record_PTR {
	std::string ptrdname;
};

struct Record_CNAME {
	std::string cname;
};

struct Record_HINFO {
	std::string cpu;
	std::string os;
};

struct Record_TXT {
	std::vector<std::string> txt;
};

struct Record_MX {
	std::uint16_t preference;
	std::string exchange;
};

struct Record_URI {
	std::uint16_t priority;
	std::uint16_t weight;
	std::string target;
};

struct Record_SRV {
	std::uint16_t priority;
	std::uint16_t weight;
	std::uint16_t port;
	std::string target;
};

struct Record_SOA {
	std::string mname;
	std::string rname;
	std::uint32_t serial;
	std::uint32_t refresh;
	std::uint32_t retry;
	std::uint32_t expire;
	std::uint32_t minimum;
};

struct Record_NSEC {
	std::string next_domain;
	std::vector<RecordType> bitmap;
};

struct Record_OPT {
	struct Option {
		std::uint16_t option_code;
		std::vector<std::uint8_t> option_data;
	};

	std::vector<Option> options;
};

using RecordData = std::variant<
	Record_A,
	Record_AAAA,
	Record_Authority,
	Record_PTR,
	Record_TXT,
	Record_MX,
	Record_SOA,
	Record_URI,
	Record_SRV,
	Record_CNAME,
	Record_HINFO,
	Record_NSEC,
	Record_OPT
>;

struct ResourceRecord {
	std::string name;
	RecordType type;
	Class resource_class;
	std::uint32_t ttl;
	std::uint16_t rdata_len;
	RecordData rdata;
};

struct Answer {
	std::string name;
	RecordType type;
	Class ccode;
	std::uint32_t ttl;
	std::uint16_t rdlen;
	RecordData rdata;
};

struct Query {
	Header header;
	std::vector<Question> questions;
	std::vector<ResourceRecord> answers;
	std::vector<ResourceRecord> authorities;
	std::vector<ResourceRecord> additional;
};

} // dns, ember