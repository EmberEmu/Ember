/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <protogen/Validator.h>
#include <protogen/Generator.h>
#include <jsoncons/json.hpp>
#include <gtest/gtest.h>
#include <array>
#include <filesystem>
#include <string>

using namespace ember;

static const std::filesystem::path templates_dir { "test_data/templates/" };

constexpr auto types_json = R"({
	"types": {
		"Result": {
			"kind": "enum",
			"underlying": "uint8",
			"values": { "ok": 0, "queued": 1, "failed": 2 }
		},
		"MovementFlags": {
			"kind": "flags",
			"underlying": "uint32",
			"values": { "swimming": 1, "jumping": 2, "flying": 4 }
		},
		"Vector3": {
			"kind": "struct",
			"fields": [
				{ "name": "x", "type": "float" },
				{ "name": "y", "type": "float" },
				{ "name": "z", "type": "float" }
			]
		},
		"Movement": {
			"kind": "struct",
			"fields": [
				{ "name": "flags", "type": "MovementFlags" },
				{ "name": "pos",   "type": "Vector3"       },
				{
					"type": "group",
					"when": { "op": "has_flag", "field": "flags", "value": "swimming" },
					"fields": [ { "name": "pitch", "type": "float" } ]
				}
			]
		}
	}
})";

jsoncons::json parse_fields(std::string_view body) {
	const auto doc = std::string(R"({"fields":)") + std::string(body) + "}";
	return jsoncons::json::parse(doc);
}

class Protogen : public ::testing::Test {
public:
	void SetUp() override {
		auto types_doc = jsoncons::json::parse(types_json);
		reg = protogen::build_registry(types_doc);
	}

	protogen::TypeRegistry reg;
};

TEST_F(Protogen, RegistryInternalsValidate) {
	ASSERT_NO_THROW(validate_registry_internals(reg));
}

TEST_F(Protogen, PrimitivesOnly) {
	const auto msg = parse_fields(R"([
		{ "name": "a", "type": "uint8"  },
		{ "name": "b", "type": "uint32" },
		{ "name": "c", "type": "double" }
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, CustomTypeReference) {
	const auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result"   },
		{ "name": "flags",  "type": "MovementFlags" },
		{ "name": "pos",    "type": "Vector3"  }
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GroupWithEqCondition) {
	const auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "result", "value": "ok" },
			"fields": [ { "name": "payload", "type": "uint32" } ]
		}
	])");
	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GroupWithInCondition) {
	const auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "in", "field": "result", "value": ["ok", "queued"] },
			"fields": [ { "name": "payload", "type": "uint32" } ]
		}
	])");
	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GroupWithHasFlagCondition) {
	const auto msg = parse_fields(R"([
		{ "name": "flags", "type": "MovementFlags" },
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "flags", "value": "jumping" },
			"fields": [ { "name": "z_speed", "type": "float" } ]
		}
	])");
	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, EqConditionOnPrimitiveWithIntegerValue) {
	const auto msg = parse_fields(R"([
		{ "name": "version", "type": "uint8" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "version", "value": 3 },
			"fields": [ { "name": "payload", "type": "uint32" } ]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, NestedGroups) {
	const auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{ "name": "flags",  "type": "MovementFlags" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "result", "value": "ok" },
			"fields": [
				{
					"type": "group",
					"when": { "op": "has_flag", "field": "flags", "value": "flying" },
					"fields": [ { "name": "altitude", "type": "float" } ]
				}
			]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, StructWithInternalConditionInRegistry) {
	// 'Movement' in the fixture carries its own has_flag group referencing flags
	// validate_registry_internals is what sanity-checks that internal condition
	ASSERT_NO_THROW(validate_registry_internals(reg));
}

TEST_F(Protogen, RejectsUnknownType) {
	auto msg = parse_fields(R"([
		{ "name": "x", "type": "Nonexistent" }
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsConditionOnUnknownField) {
	auto msg = parse_fields(R"([
		{ "name": "a", "type": "uint8" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "does_not_exist", "value": 1 },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"););
}

TEST_F(Protogen, RejectsHasFlagOnEnum) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "result", "value": "ok" },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsHasFlagOnPrimitive) {
	auto msg = parse_fields(R"([
		{ "name": "raw_flags", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "raw_flags", "value": 1 },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsUnknownEnumeratorInEq) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "result", "value": "bogus_name" },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsUnknownEnumeratorInInList) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "in", "field": "result", "value": ["ok", "not_real"] },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsConditionOnStructField) {
	auto msg = parse_fields(R"([
		{ "name": "pos", "type": "Vector3" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "pos", "value": 0 },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsNamedValueAgainstPrimitive) {
	auto msg = parse_fields(R"([
		{ "name": "a", "type": "uint8" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "a", "value": "some_name" },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesStructAndUsingAlias) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Ping",
		"opcode": "cmsg_ping",
		"direction": "client",
		"fields": [
			{ "name": "ping",   "type": "uint32" },
			{ "name": "latency","type": "uint32" }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_EQ(out.relative_path, "client/Ping.h");

	EXPECT_NE(out.content.find("struct Ping final"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("std::uint32_t ping;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> ping;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << latency;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find(
		"using cmsg_ping = ClientPacket<ClientOpcode::cmsg_ping, client::Ping>;"
	), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesConditionalGroupAsIfBlock) {
	auto msg = jsoncons::json::parse(R"({
		"name": "AuthResult",
		"opcode": "smsg_auth_result",
		"direction": "server",
		"fields": [
			{ "name": "result", "type": "Result" },
			{
				"type": "group",
				"when": { "op": "eq", "field": "result", "value": "ok" },
				"fields": [ { "name": "extra", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_EQ(out.relative_path, "server/AuthResult.h");

	EXPECT_NE(out.content.find("if(result == Result::ok) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> extra;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << extra;"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesHasFlagPredicate) {
	auto msg = jsoncons::json::parse(R"({
		"name": "FlagGated",
		"opcode": "smsg_flag_gated",
		"direction": "server",
		"fields": [
			{ "name": "flags", "type": "MovementFlags" },
			{
				"type": "group",
				"when": { "op": "has_flag", "field": "flags", "value": "jumping" },
				"fields": [ { "name": "z_speed", "type": "float" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(flags & MovementFlags::jumping) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesInPredicateAsOrChain) {
	auto msg = jsoncons::json::parse(R"({
		"name": "InGated",
		"opcode": "smsg_in_gated",
		"direction": "server",
		"fields": [
			{ "name": "result", "type": "Result" },
			{
				"type": "group",
				"when": { "op": "in", "field": "result", "value": ["ok", "queued"] },
				"fields": [ { "name": "extra", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("result == Result::ok || result == Result::queued"),
	          std::string::npos) << out.content;
}

TEST_F(Protogen, InlinesStructExpansionAtStreamSite) {
	// registry fixture's 'Movement' struct contains flags + pos + conditional pitch
	// A field of type Movement should inline-expand into `info.flags`, `info.pos.x`,
	// `info.pos.y`, `info.pos.z` and the swimming conditional block
	auto msg = jsoncons::json::parse(R"({
		"name": "MovementPacket",
		"opcode": "smsg_movement_packet",
		"direction": "server",
		"fields": [
			{ "name": "info", "type": "Movement" }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("Movement info;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> info.flags;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> info.pos.x;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("if(info.flags & MovementFlags::swimming) {"),
	          std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> info.pitch;"), std::string::npos) << out.content;
}

TEST_F(Protogen, AggregatorIncludesEachHeader) {
	std::array<std::string, 3> headers {
		"server/AuthResponse.h",
		"server/Pong.h",
		"client/Ping.h",
	};

	const auto out = protogen::generate_aggregator(headers, templates_dir);

	for(const auto& h : headers) {
		EXPECT_NE(out.find(std::string("#include <generated/") + h + ">"), std::string::npos) << out;
	}
}

TEST_F(Protogen, ValidatesFixedSizeArray) {
	auto msg = parse_fields(R"([
		{ "name": "data", "type": "uint32", "array": { "size": 32 } }
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, ValidatesDynamicArrayWithCountField) {
	auto msg = parse_fields(R"([
		{ "name": "count", "type": "uint8" },
		{ "name": "items", "type": "uint32", "array": { "count_field": "count" } }
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsArrayWithUnknownCountField) {
	auto msg = parse_fields(R"([
		{ "name": "items", "type": "uint32", "array": { "count_field": "missing" } }
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsArrayWithNonIntegerCountField) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{ "name": "items",  "type": "uint32", "array": { "count_field": "result" } }
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesFixedSizeArray) {
	auto msg = jsoncons::json::parse(R"({
		"name": "AccountData",
		"opcode": "smsg_account_data",
		"direction": "server",
		"fields": [
			{ "name": "data", "type": "uint32", "array": { "size": 32 } }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("std::array<std::uint32_t, 32> data{};"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> data;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << data;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("#include <array>"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesDynamicVectorWithCountLoop) {
	auto msg = jsoncons::json::parse(R"({
		"name": "ItemList",
		"opcode": "smsg_item_list",
		"direction": "server",
		"fields": [
			{ "name": "count", "type": "uint8" },
			{ "name": "items", "type": "uint32", "array": { "count_field": "count" } }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("std::vector<std::uint32_t> items;"), std::string::npos) << out.content;

	// Read: resize + loop
	EXPECT_NE(out.content.find("items.resize(count);"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("for(auto& e : items) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> e;"), std::string::npos) << out.content;

	// Write: no resize
	EXPECT_NE(out.content.find("for(const auto& e : items) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << e;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("#include <vector>"), std::string::npos) << out.content;
}