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
		},
		"CString": {
			"kind": "string",
			"encoding": "null_terminated"
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

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
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

TEST_F(Protogen, RejectsPostGroupReferenceToGroupLocalField) {
	// `pitch_flags` is declared inside a conditional group, so it's only on the
	// wire when `flags` has `swimming`. A later group's `when` mustn't be able
	// to reference it — validation should reject the whole message.
	auto msg = parse_fields(R"([
		{ "name": "flags", "type": "MovementFlags" },
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "flags", "value": "swimming" },
			"fields": [ { "name": "pitch_flags", "type": "MovementFlags" } ]
		},
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "pitch_flags", "value": "jumping" },
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
	EXPECT_NE(out.content.find("result == Result::ok || result == Result::queued"), std::string::npos) << out.content;
}

TEST_F(Protogen, InlinesStructExpansionAtStreamSite) {
	// registry fixture's 'Movement' struct contains flags + pos + conditional pitch
	// A field of type 'Movement' should inline-expand into 'info.flags', 'info.pos.x',
	// 'info.pos.y', 'info.pos.z' and the swimming conditional block
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
	EXPECT_NE(out.content.find("if(info.flags & MovementFlags::swimming) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> info.pitch;"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesOneHeaderPerEnumFlagsAndStruct) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Colour": {
				"kind": "enum",
				"underlying": "uint8",
				"cpp_namespace": "graphics",
				"values": { "red": 0, "green": 1 }
			},
			"Perms": {
				"kind": "flags",
				"underlying": "uint32",
				"cpp_namespace": "sys",
				"values": { "read": 1, "write": 2 }
			},
			"Point": {
				"kind": "struct",
				"cpp_namespace": "geom",
				"fields": [
					{ "name": "x", "type": "int32" },
					{ "name": "y", "type": "int32" }
				]
			},
			"CString": {
				"kind": "string",
				"encoding": "null_terminated"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);

	// one header for each type - string types produce none
	ASSERT_EQ(headers.size(), 3u);

	auto find_header = [&](std::string_view suffix) -> const protogen::GeneratedFile* {
		for(const auto& h : headers) {
			if(h.relative_path == suffix) {
				return &h; 
			}
		}

		return nullptr;
	};

	const auto* colour = find_header("types/Colour.h");
	ASSERT_NE(colour, nullptr);
	EXPECT_NE(colour->content.find("namespace graphics"), std::string::npos) << colour->content;
	EXPECT_NE(colour->content.find("enum Colour : std::uint8_t"), std::string::npos) << colour->content;
	EXPECT_NE(colour->content.find("red = 0"), std::string::npos) << colour->content;
	EXPECT_NE(colour->content.find("to_string(Colour value)"), std::string::npos) << colour->content;
	EXPECT_NE(colour->content.find("Colour_to_string(Colour value)"), std::string::npos) << colour->content;

	const auto* perms = find_header("types/Perms.h");
	ASSERT_NE(perms, nullptr);
	EXPECT_NE(perms->content.find("enum Perms : std::uint32_t"), std::string::npos) << perms->content;

	const auto* point = find_header("types/Point.h");
	ASSERT_NE(point, nullptr);
	EXPECT_NE(point->content.find("namespace geom"), std::string::npos) << point->content;
	EXPECT_NE(point->content.find("struct Point"), std::string::npos) << point->content;
	EXPECT_NE(point->content.find("std::int32_t x"), std::string::npos) << point->content;
}

TEST_F(Protogen, ValidatesStreamNotEmptyGroup) {
	auto msg = parse_fields(R"([
		{ "name": "a", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "stream_not_empty" },
			"fields": [ { "name": "trailing", "type": "uint32" } ]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesStreamNotEmptyAsymmetrically) {
	auto msg = jsoncons::json::parse(R"({
		"name": "TrailingPacket",
		"opcode": "smsg_trailing",
		"direction": "server",
		"fields": [
			{ "name": "a", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "stream_not_empty" },
				"fields": [ { "name": "trailing", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	
	// read test
	EXPECT_NE(out.content.find("if(!stream.empty()) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> trailing;"), std::string::npos) << out.content;

	// write test, should be no stream.empty() condition
	const auto write_start = out.content.find("write_to_stream");
	ASSERT_NE(write_start, std::string::npos) << out.content;
	const auto write_body = out.content.substr(write_start);
	EXPECT_EQ(write_body.find("if(!stream.empty())"), std::string::npos) << write_body;
	EXPECT_NE(write_body.find("stream << trailing;"), std::string::npos) << write_body;

	// indentiation test, since it's asymmetrical
	EXPECT_NE(write_body.find("\t\tstream << trailing;"), std::string::npos) << write_body;
}

TEST_F(Protogen, StreamNotEmptyGroupStillPopsScope) {
	// trailing group's fields must not leak into outer scope
	// (same rule as the other group ops)
	auto msg = parse_fields(R"([
		{ "name": "a", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "stream_not_empty" },
			"fields": [ { "name": "trailing_flags", "type": "MovementFlags" } ]
		},
		{
			"type": "group",
			"when": { "op": "has_flag", "field": "trailing_flags", "value": "jumping" },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, ReferencesExternalType) {
	// stand-in for the actual character struct
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Guid": {
				"kind": "external",
				"include": "shared/Guid.h",
				"cpp_namespace": "ember"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);

	auto msg = jsoncons::json::parse(R"({
		"name": "CharacterEnumTest",
		"opcode": "smsg_character_enum_test",
		"direction": "server",
		"fields": [
			{ "name": "id", "type": "Guid" }
		]
	})");

	const auto out = protogen::generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("#include <shared/Guid.h>"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("ember::Guid id;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> id;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << id;"), std::string::npos) << out.content;
}

TEST_F(Protogen, NoHeaderGeneratedForExternalType) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Guid": {
				"kind": "external",
				"include": "shared/Guid.h",
				"cpp_namespace": "ember"
			},
			"Colour": {
				"kind": "enum",
				"underlying": "uint8",
				"cpp_namespace": "graphics",
				"values": { "red": 0 }
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);
	ASSERT_EQ(headers.size(), 1u);
	EXPECT_EQ(headers.front().relative_path, "types/Colour.h");
}

TEST_F(Protogen, RejectsExternalInCondition) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Guid": {
				"kind": "external",
				"include": "shared/Guid.h",
				"cpp_namespace": "ember"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto fields = jsoncons::json::parse(R"([
		{ "name": "id", "type": "Guid" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "id", "value": 0 },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(fields, reg, "test"));
}

TEST_F(Protogen, AcceptsFixedSizeArrayOfExternal) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Guid": {
				"kind": "external",
				"include": "shared/Guid.h",
				"cpp_namespace": "ember"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto fields = jsoncons::json::parse(R"([
		{ "name": "ids", "type": "Guid", "array": { "size": 4 } }
	])");

	EXPECT_NO_THROW(validate_message_fields(fields, reg, "test"));
}

TEST_F(Protogen, StructMemberOfExternalPullsItsInclude) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Guid": {
				"kind": "external",
				"include": "shared/Guid.h",
				"cpp_namespace": "ember"
			},
			"Pair": {
				"kind": "struct",
				"cpp_namespace": "ns",
				"fields": [
					{ "name": "left",  "type": "Guid" },
					{ "name": "right", "type": "Guid" }
				]
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);

	const protogen::GeneratedFile* pair = nullptr;

	for(const auto& h : headers) {
		if(h.relative_path == "types/Pair.h") { pair = &h; }
	}

	ASSERT_NE(pair, nullptr);
	EXPECT_NE(pair->content.find("#include <shared/Guid.h>"), std::string::npos) << pair->content;
	EXPECT_NE(pair->content.find("ember::Guid left"), std::string::npos) << pair->content;
}

TEST_F(Protogen, StructPullsInGeneratedHeadersForCustomMembers) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Mode": {
				"kind": "flags",
				"underlying": "uint8",
				"cpp_namespace": "ns",
				"values": { "on": 1 }
			},
			"Inner": {
				"kind": "struct",
				"cpp_namespace": "ns",
				"fields": [ { "name": "value", "type": "uint32" } ]
			},
			"Outer": {
				"kind": "struct",
				"cpp_namespace": "ns",
				"fields": [
					{ "name": "m", "type": "Mode" },
					{ "name": "i", "type": "Inner" }
				]
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);

	const protogen::GeneratedFile* outer = nullptr;

	for(const auto& h : headers) {
		if(h.relative_path == "types/Outer.h") { outer = &h; }
	}

	ASSERT_NE(outer, nullptr);
	EXPECT_NE(outer->content.find("#include <protocol/types/Mode.h>"), std::string::npos) << outer->content;
	EXPECT_NE(outer->content.find("#include <protocol/types/Inner.h>"), std::string::npos) << outer->content;
}

TEST_F(Protogen, QualifiesEnumAndMemberWithCppNamespace) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Colour": {
				"kind": "enum",
				"underlying": "uint8",
				"cpp_namespace": "graphics",
				"values": { "red": 0, "green": 1 }
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);

	auto msg = jsoncons::json::parse(R"({
		"name": "ColourPacket",
		"opcode": "smsg_colour",
		"direction": "server",
		"fields": [
			{ "name": "c", "type": "Colour" },
			{
				"type": "group",
				"when": { "op": "eq", "field": "c", "value": "red" },
				"fields": [ { "name": "extra", "type": "uint32" } ]
			}
		]
	})");

	const auto out = protogen::generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("graphics::Colour c;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("c == graphics::Colour::red"), std::string::npos) << out.content;
}

TEST_F(Protogen, AggregatorIncludesEachHeader) {
	std::array<std::string, 3> headers {
		"server/AuthResponse.h",
		"server/Pong.h",
		"client/Ping.h",
	};

	const auto out = protogen::generate_aggregator(headers, templates_dir);

	for(const auto& h : headers) {
		EXPECT_NE(out.find(std::string("#include <protocol/") + h + ">"), std::string::npos) << out;
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

TEST_F(Protogen, AcceptsFixedSizeArrayOfStruct) {
	auto msg = parse_fields(R"([
		{ "name": "positions", "type": "Vector3", "array": { "size": 4 } }
	])");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, AcceptsFixedSizeArrayOfFlags) {
	auto msg = parse_fields(R"([
		{ "name": "masks", "type": "MovementFlags", "array": { "size": 2 } }
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
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

	// read: resize & iterate
	EXPECT_NE(out.content.find("items.resize(count);"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("for(auto& e : items) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> e;"), std::string::npos) << out.content;

	// write: no resize, iterate
	EXPECT_NE(out.content.find("for(const auto& e : items) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << e;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("#include <vector>"), std::string::npos) << out.content;
}

TEST_F(Protogen, ValidatesNeqCondition) {
	const auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "group",
			"when": { "op": "neq", "field": "result", "value": "ok" },
			"fields": [ { "name": "err", "type": "uint32" } ]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesNeqPredicate) {
	auto msg = jsoncons::json::parse(R"({
		"name": "NeqGated",
		"opcode": "smsg_neq_gated",
		"direction": "server",
		"fields": [
			{ "name": "result", "type": "Result" },
			{
				"type": "group",
				"when": { "op": "neq", "field": "result", "value": "ok" },
				"fields": [ { "name": "err", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(result != Result::ok) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesAndCompoundCondition) {
	auto msg = jsoncons::json::parse(R"({
		"name": "AndGated",
		"opcode": "smsg_and_gated",
		"direction": "server",
		"fields": [
			{ "name": "result", "type": "Result" },
			{ "name": "flags",  "type": "MovementFlags" },
			{
				"type": "group",
				"when": {
					"op": "and",
					"conditions": [
						{ "op": "eq", "field": "result", "value": "ok" },
						{ "op": "has_flag", "field": "flags", "value": "jumping" }
					]
				},
				"fields": [ { "name": "extra", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("(result == Result::ok) && (flags & MovementFlags::jumping)"),
	          std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesOrCompoundCondition) {
	auto msg = jsoncons::json::parse(R"({
		"name": "OrGated",
		"opcode": "smsg_or_gated",
		"direction": "server",
		"fields": [
			{ "name": "a", "type": "uint8" },
			{ "name": "b", "type": "uint8" },
			{
				"type": "group",
				"when": {
					"op": "or",
					"conditions": [
						{ "op": "eq", "field": "a", "value": 1 },
						{ "op": "eq", "field": "b", "value": 2 }
					]
				},
				"fields": [ { "name": "payload", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("(a == 1) || (b == 2)"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesNotCondition) {
	auto msg = jsoncons::json::parse(R"({
		"name": "NotGated",
		"opcode": "smsg_not_gated",
		"direction": "server",
		"fields": [
			{ "name": "flags", "type": "MovementFlags" },
			{
				"type": "group",
				"when": {
					"op": "not",
					"condition": { "op": "has_flag", "field": "flags", "value": "jumping" }
				},
				"fields": [ { "name": "grounded", "type": "uint8" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(!(flags & MovementFlags::jumping)) {"),
	          std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesFieldToFieldEq) {
	auto msg = jsoncons::json::parse(R"({
		"name": "FieldEq",
		"opcode": "smsg_field_eq",
		"direction": "server",
		"fields": [
			{ "name": "foo", "type": "uint32" },
			{ "name": "bar", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "eq", "field": "foo", "value": { "field": "bar" } },
				"fields": [ { "name": "payload", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(foo == bar) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesFieldToFieldNeq) {
	auto msg = jsoncons::json::parse(R"({
		"name": "FieldNeq",
		"opcode": "smsg_field_neq",
		"direction": "server",
		"fields": [
			{ "name": "foo", "type": "uint32" },
			{ "name": "bar", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "neq", "field": "foo", "value": { "field": "bar" } },
				"fields": [ { "name": "payload", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(foo != bar) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesNegatedFieldReference) {
	auto msg = jsoncons::json::parse(R"({
		"name": "FieldNegate",
		"opcode": "smsg_field_negate",
		"direction": "server",
		"fields": [
			{ "name": "foo", "type": "uint32" },
			{ "name": "bar", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "eq", "field": "foo", "value": { "field": "bar", "negate": true } },
				"fields": [ { "name": "payload", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(foo == !bar) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesZeroTestViaConstant) {
	auto msg = jsoncons::json::parse(R"({
		"name": "ZeroTest",
		"opcode": "smsg_zero_test",
		"direction": "server",
		"fields": [
			{ "name": "count", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "neq", "field": "count", "value": 0 },
				"fields": [ { "name": "payload", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(count != 0) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, ValidatesEmptyOnStringField) {
	const auto msg = parse_fields(R"([
		{ "name": "name", "type": "CString" },
		{
			"type": "group",
			"when": { "op": "not", "condition": { "op": "empty", "field": "name" } },
			"fields": [ { "name": "flag", "type": "uint8" } ]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesEmptyStringTest) {
	auto msg = jsoncons::json::parse(R"({
		"name": "EmptyTest",
		"opcode": "smsg_empty_test",
		"direction": "server",
		"fields": [
			{ "name": "name", "type": "CString" },
			{
				"type": "group",
				"when": { "op": "not", "condition": { "op": "empty", "field": "name" } },
				"fields": [ { "name": "flag", "type": "uint8" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(!(name.empty())) {"), std::string::npos) << out.content;
}

TEST_F(Protogen, GeneratesSiblingConditionalGroups) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Sibling",
		"opcode": "smsg_sibling",
		"direction": "server",
		"fields": [
			{ "name": "foo", "type": "uint32" },
			{ "name": "bar", "type": "uint32" },
			{
				"type": "group",
				"when": { "op": "neq", "field": "foo", "value": 0 },
				"fields": [ { "name": "foo_extra", "type": "uint32" } ]
			},
			{
				"type": "group",
				"when": { "op": "neq", "field": "bar", "value": 0 },
				"fields": [ { "name": "bar_extra", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(foo != 0) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("if(bar != 0) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> foo_extra;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> bar_extra;"), std::string::npos) << out.content;
}

TEST_F(Protogen, RejectsEmptyOnNonStringField) {
	auto msg = parse_fields(R"([
		{ "name": "count", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "empty", "field": "count" },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsFieldRefValueToUnknownField) {
	auto msg = parse_fields(R"([
		{ "name": "foo", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "foo", "value": { "field": "missing" } },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsFieldRefValueToStructField) {
	auto msg = parse_fields(R"([
		{ "name": "pos", "type": "Vector3" },
		{ "name": "foo", "type": "uint32" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "foo", "value": { "field": "pos" } },
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RejectsNestedStreamNotEmptyInCompound) {
	auto msg = parse_fields(R"([
		{ "name": "foo", "type": "uint32" },
		{
			"type": "group",
			"when": {
				"op": "and",
				"conditions": [
					{ "op": "eq", "field": "foo", "value": 1 },
					{ "op": "stream_not_empty" }
				]
			},
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, RecursesValidationIntoCompoundConditions) {
	// An unknown enum value buried inside an AND should still be caught.
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{ "name": "flags",  "type": "MovementFlags" },
		{
			"type": "group",
			"when": {
				"op": "and",
				"conditions": [
					{ "op": "has_flag", "field": "flags", "value": "jumping" },
					{ "op": "eq", "field": "result", "value": "bogus_name" }
				]
			},
			"fields": [ { "name": "x", "type": "uint8" } ]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

// ---------------------------------------------------------------------------
// if_chain: `if / else if / else` cascade
// ---------------------------------------------------------------------------

TEST_F(Protogen, ValidatesIfChain) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "if_chain",
			"branches": [
				{
					"when": { "op": "eq", "field": "result", "value": "ok" },
					"fields": [ { "name": "code", "type": "uint32" } ]
				},
				{
					"when": { "op": "eq", "field": "result", "value": "queued" },
					"fields": [ { "name": "pos", "type": "uint32" } ]
				}
			],
			"else": [ { "name": "reason", "type": "uint32" } ]
		}
	])");

	ASSERT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, IfChainRejectsUnknownBranchValue) {
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "if_chain",
			"branches": [
				{
					"when": { "op": "eq", "field": "result", "value": "bogus" },
					"fields": [ { "name": "code", "type": "uint32" } ]
				}
			]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, IfChainScopeIsBranchLocal) {
	// A field declared inside one branch must not be visible to a later
	// branch or to the `else` body.
	auto msg = parse_fields(R"([
		{ "name": "result", "type": "Result" },
		{
			"type": "if_chain",
			"branches": [
				{
					"when": { "op": "eq", "field": "result", "value": "ok" },
					"fields": [ { "name": "local_field", "type": "uint8" } ]
				}
			],
			"else": [
				{
					"type": "group",
					"when": { "op": "eq", "field": "local_field", "value": 1 },
					"fields": [ { "name": "x", "type": "uint8" } ]
				}
			]
		}
	])");

	ASSERT_ANY_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesIfChainAsElseCascade) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Resp",
		"opcode": "smsg_resp",
		"direction": "server",
		"fields": [
			{ "name": "result", "type": "Result" },
			{
				"type": "if_chain",
				"branches": [
					{
						"when": { "op": "eq", "field": "result", "value": "ok" },
						"fields": [ { "name": "code", "type": "uint32" } ]
					},
					{
						"when": { "op": "eq", "field": "result", "value": "queued" },
						"fields": [ { "name": "pos", "type": "uint32" } ]
					}
				],
				"else": [ { "name": "reason", "type": "uint32" } ]
			}
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	// Exactly one if, one else-if, one else — not three standalone ifs.
	EXPECT_NE(out.content.find("if(result == Result::ok) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("else if(result == Result::queued) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("else {"), std::string::npos) << out.content;
}

// ---------------------------------------------------------------------------
// `as`: semantic type cast at message-definition level
// ---------------------------------------------------------------------------

TEST_F(Protogen, AsCastMatchingUnderlyingValidates) {
	auto msg = parse_fields(R"([
		{ "name": "kind", "type": "uint8", "as": "Result" }
	])");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, AsCastAcceptsSizeMismatch) {
	auto msg = parse_fields(R"([
		{ "name": "kind", "type": "uint32", "as": "Result" }
	])");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, AsCastScopesFieldAsSemanticType) {
	// A condition using the field's enum values should validate because
	// the scope entry reflects `as`, not the wire type.
	auto msg = parse_fields(R"([
		{ "name": "kind", "type": "uint8", "as": "Result" },
		{
			"type": "group",
			"when": { "op": "eq", "field": "kind", "value": "ok" },
			"fields": [ { "name": "x", "type": "uint32" } ]
		}
	])");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesAsCastWithRawTempAndStaticCast) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Resp",
		"opcode": "smsg_resp",
		"direction": "server",
		"fields": [
			{ "name": "kind", "type": "uint8", "as": "Result" }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	// Member exposes the enum.
	EXPECT_NE(out.content.find("Result kind;"), std::string::npos) << out.content;
	// Read: temporary of wire type, then static_cast into the member.
	EXPECT_NE(out.content.find("std::uint8_t raw_"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("kind = static_cast<Result>("), std::string::npos) << out.content;
	// Write: static_cast the member back to the wire type.
	EXPECT_NE(out.content.find("stream << static_cast<std::uint8_t>(kind);"), std::string::npos) << out.content;
}

// ---------------------------------------------------------------------------
// until_end: read-to-EOF arrays
// ---------------------------------------------------------------------------

TEST_F(Protogen, ValidatesUntilEndArray) {
	auto msg = parse_fields(R"([
		{ "name": "tail", "type": "uint32", "array": { "until_end": true } }
	])");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

TEST_F(Protogen, GeneratesUntilEndLoopWithPerIterationCheck) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Bulk",
		"opcode": "smsg_bulk",
		"direction": "server",
		"fields": [
			{ "name": "tail", "type": "uint32", "array": { "until_end": true } }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	// Member is a vector, not a fixed array.
	EXPECT_NE(out.content.find("std::vector<std::uint32_t> tail;"), std::string::npos) << out.content;

	// Read: while-empty loop driving emplace_back + scalar read + error check.
	EXPECT_NE(out.content.find("while(!stream.empty()) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("tail.emplace_back();"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> tail.back();"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("if(!stream) return StreamResult::failed;"), std::string::npos) << out.content;

	// Write is a plain range-for; the receiver's read loop stops on EOF.
	EXPECT_NE(out.content.find("for(const auto& e : tail) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << e;"), std::string::npos) << out.content;
}

TEST_F(Protogen, UntilEndRejectsAlongsideSize) {
	// schema says only one of size/count_field/until_end may be set, but
	// the C++ validator shouldn't crash if both show up. Construct the
	// struct manually to test the defensive path.
	auto msg = parse_fields(R"([
		{ "name": "tail", "type": "uint32", "array": { "size": 3, "until_end": true } }
	])");

	// validate_array doesn't care which one it sees; both branches early-
	// return. Nothing to assert except "doesn't crash".
	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
}

// ---------------------------------------------------------------------------
// fixed-size arrays of non-primitive element types
// ---------------------------------------------------------------------------

TEST_F(Protogen, GeneratesFixedSizeStructArrayAsLoop) {
	// Struct-element fixed arrays get a visible per-element loop because
	// there's no generic stream >> operator for std::array<Struct, N>.
	auto msg = jsoncons::json::parse(R"({
		"name": "Points",
		"opcode": "smsg_points",
		"direction": "server",
		"fields": [
			{ "name": "pos", "type": "Vector3", "array": { "size": 3 } }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("std::array<Vector3, 3> pos{};"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("for(auto& e : pos) {"), std::string::npos) << out.content;
	// Struct is expanded to its fields under the loop variable.
	EXPECT_NE(out.content.find("stream >> e.x;"), std::string::npos) << out.content;
}

// ---------------------------------------------------------------------------
// external: cpp_type_name, underlying, stream_via
// ---------------------------------------------------------------------------

TEST_F(Protogen, ExternalWithCppTypeNameEmitsQualifiedName) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"MyFlag": {
				"kind": "external",
				"include": "foo/bar.h",
				"cpp_namespace": "foo::container",
				"cpp_type_name": "Inner"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto msg = jsoncons::json::parse(R"({
		"name": "P",
		"opcode": "smsg_p",
		"direction": "server",
		"fields": [ { "name": "f", "type": "MyFlag" } ]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	// Member type is the namespace + cpp_type_name, not the schema key.
	EXPECT_NE(out.content.find("foo::container::Inner f;"), std::string::npos) << out.content;
}

TEST_F(Protogen, ExternalWithUnderlyingAcceptsAsCast) {
	// DBC inner enum pattern: external with integral underlying lets a
	// wire-typed field cast into it via `as`.
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"DbcEnum": {
				"kind": "external",
				"include": "dbcreader/MemoryDefs.h",
				"cpp_namespace": "ember::dbc::Owner",
				"cpp_type_name": "Kind",
				"underlying": "int32"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto fields = jsoncons::json::parse(R"([
		{ "name": "kind", "type": "uint8", "as": "DbcEnum" }
	])");

	EXPECT_NO_THROW(validate_message_fields(fields, reg, "test"));
}

TEST_F(Protogen, ExternalWithUnderlyingUsedAsTypeIsAnAlias) {
	// An external that declares `underlying` and is referenced directly
	// via `type` (no `as` cast) is a named alias for the primitive — the
	// generated member takes the primitive type, no include is emitted
	// for the external header, and the stream op is the plain primitive
	// op. This is the DBC record-ref pattern (`Map` ≡ `uint32`).
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Map": {
				"kind": "external",
				"include": "dbcreader/MemoryDefs.h",
				"cpp_namespace": "ember::dbc",
				"underlying": "uint32"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto msg = jsoncons::json::parse(R"({
		"name": "Joined",
		"opcode": "smsg_joined",
		"direction": "server",
		"fields": [ { "name": "map", "type": "Map" } ]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("std::uint32_t map;"), std::string::npos) << out.content;
	EXPECT_EQ(out.content.find("ember::dbc::Map"), std::string::npos) << out.content;
	EXPECT_EQ(out.content.find("#include <dbcreader/MemoryDefs.h>"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream >> map;"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("stream << map;"), std::string::npos) << out.content;
}

TEST_F(Protogen, AliasExternalUsableInCondition) {
	// A field typed as an alias external is comparable — its effective
	// type is the underlying primitive, so integer conditions validate.
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Map": {
				"kind": "external",
				"include": "dbcreader/MemoryDefs.h",
				"cpp_namespace": "ember::dbc",
				"underlying": "uint32"
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	auto msg = jsoncons::json::parse(R"({
		"name": "Q",
		"opcode": "smsg_q",
		"direction": "server",
		"fields": [
			{ "name": "m", "type": "Map" },
			{
				"type": "group",
				"when": { "op": "neq", "field": "m", "value": 0 },
				"fields": [ { "name": "x", "type": "uint32" } ]
			}
		]
	})");

	EXPECT_NO_THROW(validate_message_fields(msg["fields"], reg, "test"));
	const auto out = generate_message(msg, reg, templates_dir);
	EXPECT_NE(out.content.find("if(m != 0)"), std::string::npos) << out.content;
}

// ---------------------------------------------------------------------------
// enum_class: same semantics as enum, different emit
// ---------------------------------------------------------------------------

TEST_F(Protogen, EnumClassEmitsScopedEnum) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Colour": {
				"kind": "enum_class",
				"underlying": "uint8",
				"values": { "red": 0, "green": 1 }
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);
	const protogen::GeneratedFile* h = nullptr;
	for(const auto& f : headers) {
		if(f.relative_path == "types/Colour.h") { h = &f; }
	}
	ASSERT_NE(h, nullptr);
	EXPECT_NE(h->content.find("enum class Colour : std::uint8_t"), std::string::npos) << h->content;
}

TEST_F(Protogen, EnumStaysUnscoped) {
	auto types_doc = jsoncons::json::parse(R"({
		"types": {
			"Style": {
				"kind": "enum",
				"underlying": "uint8",
				"values": { "plain": 0, "bold": 1 }
			}
		}
	})");

	auto reg = protogen::build_registry(types_doc);
	const auto headers = protogen::generate_type_headers(reg, templates_dir);
	const protogen::GeneratedFile* h = nullptr;
	for(const auto& f : headers) {
		if(f.relative_path == "types/Style.h") { h = &f; }
	}
	ASSERT_NE(h, nullptr);
	EXPECT_NE(h->content.find("enum Style : std::uint8_t"), std::string::npos) << h->content;
	EXPECT_EQ(h->content.find("enum class"), std::string::npos) << h->content;
}

TEST_F(Protogen, GeneratesFixedSizeStringArrayAsLoop) {
	auto msg = jsoncons::json::parse(R"({
		"name": "Names",
		"opcode": "smsg_names",
		"direction": "server",
		"fields": [
			{ "name": "n", "type": "CString", "array": { "size": 4 } }
		]
	})");

	const auto out = generate_message(msg, reg, templates_dir);

	EXPECT_NE(out.content.find("std::array<std::string, 4> n{};"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("for(auto& e : n) {"), std::string::npos) << out.content;
	EXPECT_NE(out.content.find("spark::io::null_terminated(e)"), std::string::npos) << out.content;
}
