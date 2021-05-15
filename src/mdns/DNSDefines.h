/*
 * Copyright (c) 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/endian.hpp>
#include <string_view>
#include <variant>
#include <cstdint>

namespace ember::dns {

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
constexpr auto RCODE_OFFSET  = 12;

constexpr auto QR_MASK     = 0x01 << QR_OFFSET;
constexpr auto OPCODE_MASK = 0x0F << OPCODE_OFFSET;
constexpr auto AA_MASK     = 0x01 << AA_OFFSET;
constexpr auto TC_MASK     = 0x01 << TC_OFFSET;
constexpr auto RD_MASK     = 0x01 << RD_OFFSET;
constexpr auto RA_MASK     = 0x01 << RA_OFFSET;
constexpr auto Z_MASK      = 0x07 << Z_OFFSET;
constexpr auto RCODE_MASK  = 0x0F << RCODE_OFFSET;

enum class QR {
    QUERY, REPLY
};

enum class Opcode {
    QUERY, IQUERY, STATUS
};

enum class ResCode {
    NOERROR, FORMERR, SERVFAIL, NXDOMAIN
};

enum class RRType : std::uint16_t {
    A = 1, AAAA = 28, AFSDB = 18, APL = 42, CAA = 257, CDNSKEY = 60, CDS = 59,
    CERT = 37, CNAME = 5, CSYNC = 62, DHCID = 49, DLV = 32769, DNAME = 39,
    DNSKEY = 48, DS = 43, HINFO = 13, HIP = 55, IPSECKEY = 45,
    KEY = 25, KX = 36, LOC = 29, MX = 15, NAPTR = 35, NS = 2, NSEC = 47,
    NSEC3 = 50, NSEC3PARAM = 51, OPENPGPKEY = 61, PTR = 12, RRSIG = 46,
    RP = 17, SIG = 24, SMIMEA = 53, SOA = 6, SRV = 33, SSHFP = 44, TA = 32768,
    TKEY = 249, TLSA = 52, TSIG = 250, TXT = 16, URI = 256, ZONEMD = 63
};

enum class ClassCode : std::uint16_t {
    IN = 1, CH = 3, HS = 4
};

struct Ptr {

};

struct Txt { 
    std::string_view key;
    std::string_view value;
};

struct Srv {
    std::string_view service;
    std::string_view protocol;
    std::string_view name;
    std::uint32_t ttl;
    ClassCode ccode;
    std::uint16_t priority;
    std::uint16_t weight;
    std::uint16_t port;
    std::string_view host;
};

struct Flags {
    std::uint16_t qr     : 1;
    std::uint16_t opcode : 4;
    std::uint16_t aa     : 1;
    std::uint16_t tc     : 1;
    std::uint16_t rd     : 1;
    std::uint16_t ra     : 1;
    std::uint16_t z      : 3;
    std::uint16_t rcode  : 4;
};

struct Header {
    big_uint16_t id;
    big_uint16_t flags;
    big_uint16_t questions;
    big_uint16_t answers;
    big_uint16_t authority_rrs;
    big_uint16_t additional_rrs;
};

static_assert(sizeof(Header) == DNS_HDR_SIZE, "Bad header size");

struct Answer {
    std::string_view name;
    RRType type;
    ClassCode ccode;
    std::uint32_t ttl;
    std::uint16_t rdlen;
    RecordData record;
};

struct Question {
    std::string_view name;
    RRType type;
    ClassCode cc;
};

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

} // dns, ember