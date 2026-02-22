/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/commands/Registry.h>
#include <shared/commands/Utility.h>
#include <gtest/gtest.h>
#include <array>
#include <ranges>

using namespace ember;

class Registry : public ::testing::Test {
public:
	virtual void SetUp() {
		auto cmd = registry.insert("root")
			->description("root command")
			->argument("arg1", commands::ArgumentType::at_string)
			->argument("arg2", commands::ArgumentType::at_string)
			->optional_argument("arg3", commands::ArgumentType::at_uint32)
			->handler([&](auto /*args*/) {
				success = true;
			});

		cmd->insert("root_nested_1");
		cmd->insert("root_nested_2")
			->insert("root_nested_2_1");

		registry.insert("root_1"); // error: unavailable
		registry.insert("root_2")->insert("root_2_1"); // error: subcommands only
	}

	virtual void TearDown() {}

	commands::Registry registry;
	bool success = false;
};

using Commands = Registry;

TEST_F(Registry, ParseInput) {
	const auto input = "Lorem ipsum dolor sit amet";
	const auto tokens = commands::Registry::parse_input(input);

	const std::array<std::string_view, 5> expected {
		"Lorem", "ipsum", "dolor", "sit", "amet"
	};

	ASSERT_EQ(tokens.size(), expected.size());

	for(auto [t, e] : std::views::zip(tokens, expected)) {
		ASSERT_EQ(t, e);
	}
}

TEST_F(Registry, ParseQuotedInput) {
	const auto input = R"(Lorem "ipsum dolor" sit amet)";
	const auto tokens = commands::Registry::parse_input(input);

	const std::array<std::string_view, 4> expected {
		"Lorem", "ipsum dolor", "sit", "amet"
	};

	ASSERT_EQ(tokens.size(), expected.size());

	for(auto [t, e] : std::views::zip(tokens, expected)) {
		ASSERT_EQ(t, e);
	}
}

TEST_F(Registry, ParseEscapeQuotedInput) {
	const auto input = R"("Hello, "\"world!"\")";
	const auto tokens = commands::Registry::parse_input(input);

	const std::array<std::string_view, 1> expected {
		R"(Hello, "world!")"
	};

	ASSERT_EQ(tokens.size(), expected.size());

	for(auto [t, e] : std::views::zip(tokens, expected)) {
		ASSERT_EQ(t, e);
	}
}

TEST_F(Registry, ParseBadEscapedInput) {
	const auto input = R"("Hello, \world")";
	ASSERT_THROW(commands::Registry::parse_input(input), commands::parse_error);
}

TEST_F(Registry, AddCommand) {
	registry.insert("test")
		->description("test_description");

	const auto input = commands::Registry::parse_input("test");
	ASSERT_NE(registry.search(input).command, nullptr);
}

TEST_F(Registry, EraseCommand) {
	ASSERT_TRUE(registry.erase("root"));
	ASSERT_EQ(registry.search("root").command, nullptr);
}

TEST_F(Registry, EraseNotFoundCommand) {
	ASSERT_FALSE(registry.erase("fake_command"));
}

TEST_F(Registry, GetRoot) {
	ASSERT_TRUE(registry.root());
}

TEST_F(Registry, SearchSubcommands) {
	const auto result = registry.search("root root_nested_1 root_nested_2");
	ASSERT_TRUE(result.command);
	ASSERT_EQ(result.depth, 2);
}

TEST_F(Registry, Autocomplete) {
	const auto result = registry.autocomplete("r");
	ASSERT_EQ(result.substring, "root ");
	ASSERT_EQ(result.records.size(), 3);
	ASSERT_EQ(result.records.front().name, "root");
	ASSERT_EQ(result.records.front().desc, "root command");
}
	
TEST_F(Registry, AutocompleteSubcommands) {
	const auto result = registry.autocomplete("root ");
	ASSERT_EQ(result.substring, "root root_nested_");
	ASSERT_EQ(result.records.size(), 2);
	ASSERT_EQ(result.records.front().name, "root_nested_1");
	ASSERT_EQ(result.records.back().name, "root_nested_2");
}

TEST_F(Registry, AutocompleteNestedSubcommand) {
	const auto result = registry.autocomplete("root root_nested_2 ");
	ASSERT_EQ(result.substring, "root root_nested_2 root_nested_2_1");
	ASSERT_EQ(result.records.size(), 1);
	ASSERT_EQ(result.records.front().name, "root_nested_2_1");
}

TEST_F(Registry, SearchNoMatch) {
	const auto result = registry.autocomplete("foo");
	ASSERT_TRUE(result.substring.empty());
	ASSERT_TRUE(result.records.empty());
}

TEST_F(Registry, PathFragment) {
	const auto tokens = registry.parse_input("root root_nested_2 root_nested_2_1");
	ASSERT_EQ(commands::path_fragment(tokens, 0), "");
	ASSERT_EQ(commands::path_fragment(tokens, 1), "root");
	ASSERT_EQ(commands::path_fragment(tokens, 2), "root root_nested_2");
	ASSERT_EQ(commands::path_fragment(tokens, 3), "root root_nested_2 root_nested_2_1");
	ASSERT_EQ(commands::path_fragment(tokens, 4), "");
}

TEST_F(Commands, InsertCommand) {
	auto cmd = registry.search("root").command;
	cmd->insert("subcommand")->description("It's a subcommand");
	auto subcommands = cmd->commands();
	ASSERT_EQ(subcommands.size(), 3);
	ASSERT_EQ(subcommands.contains("subcommand"), 1);
	ASSERT_EQ(subcommands["subcommand"]->description(), "It's a subcommand");
}

TEST_F(Commands, EraseCommand) {
	auto cmd = registry.search("root").command;
	cmd->erase("root_nested_1");
	ASSERT_FALSE(cmd->commands().contains("root_nested_1"));
}

TEST_F(Commands, ClearCommands) {
	auto cmd = registry.search("root").command;
	cmd->clear_commands();
	ASSERT_TRUE(cmd->commands().empty());
}

TEST_F(Commands, UpdateDescription) {
	auto cmd = registry.search("root").command;
	ASSERT_EQ(cmd->description(), "root command");
	cmd->description("New description!");
	ASSERT_EQ(cmd->description(), "New description!");
}

TEST_F(Commands, ArgumentCount) {
	auto cmd = registry.search("root").command;
	ASSERT_EQ(cmd->argument_count(), 3);
}

TEST_F(Commands, AddArgument) {
	auto cmd = registry.search("root").command;
	cmd->optional_argument("arg4", commands::ArgumentType::at_char);
	ASSERT_EQ(cmd->argument_count(), 4);
}

TEST_F(Commands, AddArgumentOutOfOrder) {
	auto cmd = registry.search("root").command;
	ASSERT_THROW(cmd->argument("arg4", commands::ArgumentType::at_char), std::invalid_argument);
}

TEST_F(Commands, EraseInsertArgument) {
	auto cmd = registry.search("root").command;
	ASSERT_TRUE(cmd->erase_argument("arg3"));
	cmd->argument("arg3", commands::ArgumentType::at_char);
	ASSERT_EQ(cmd->argument_count(), 3);
}

TEST_F(Commands, EraseArgument) {
	auto cmd = registry.search("root").command;
	ASSERT_TRUE(cmd->erase_argument("arg1"));
}

TEST_F(Commands, EraseNotFoundArgument) {
	auto cmd = registry.search("root").command;
	ASSERT_FALSE(cmd->erase_argument("fake_arg1"));
}

TEST_F(Commands, ClearArguments) {
	auto cmd = registry.search("root").command;
	cmd->clear_arguments();
	ASSERT_TRUE(cmd->arguments().empty());
}

TEST_F(Commands, ExecuteNoHandler) {
	auto cmd = registry.search("root_1").command;
	ASSERT_EQ(cmd->execute(), commands::Result::unavailable);
}

TEST_F(Commands, Execute_InvalidTypes) {
	auto cmd = registry.search("root").command;

	std::array<commands::ArgumentValue, 3> args {
		"Are you sure that's the point, Doctor?",
		"Of course. What else could it be?",
		"That you should never tell the same lie twice."
	};

	ASSERT_EQ(cmd->execute(args), commands::Result::invalid_types);
}

TEST_F(Commands, Execute_SubcommandsOnly) {
	auto cmd = registry.search("root_2").command;
	ASSERT_EQ(cmd->execute(), commands::Result::subcommands);
}

TEST_F(Commands, Execute_MissingArgsEmpty) {
	auto cmd = registry.search("root").command;
	ASSERT_EQ(cmd->execute(), commands::Result::missing_args);
}

TEST_F(Commands, Execute_TooManyArgs) {
	auto cmd = registry.search("root").command;

	std::array<commands::ArgumentValue, 4> args {
		"The", "quick", "brown", "fox"
	};

	ASSERT_EQ(cmd->execute(args), commands::Result::too_many_args);
}

TEST_F(Commands, Execute_MissingArgs) {
	auto cmd = registry.search("root").command;

	std::array<commands::ArgumentValue, 1> args {
		"jumped",
	};

	ASSERT_EQ(cmd->execute(), commands::Result::missing_args);
}

TEST_F(Commands, Execute_RequiredArgs_Success) {
	auto cmd = registry.search("root").command;

	std::array<commands::ArgumentValue, 2> args {
		"over", "the",
	};

	ASSERT_EQ(cmd->execute(args), commands::Result::success);
	ASSERT_TRUE(success);
}

TEST_F(Commands, Execute_OptionalArgs_Success) {
	auto cmd = registry.search("root").command;

	std::array<commands::ArgumentValue, 3> args {
		"lazy", "dog", std::uint32_t(42)
	};

	ASSERT_EQ(cmd->execute(args), commands::Result::success);
	ASSERT_TRUE(success);
}

TEST_F(Commands, Execute_AllArgTypes_Success) {
	auto cmd = registry.search("root").command;
	cmd->clear_arguments();

	cmd->argument("arg1", commands::ArgumentType::at_string)
		->argument("arg2", commands::ArgumentType::at_char)
		->argument("arg3", commands::ArgumentType::at_double)
		->argument("arg4", commands::ArgumentType::at_float)
		->argument("arg5", commands::ArgumentType::at_int16)
		->argument("arg6", commands::ArgumentType::at_int32)
		->argument("arg7", commands::ArgumentType::at_int64)
		->argument("arg8", commands::ArgumentType::at_int8)
		->argument("arg9", commands::ArgumentType::at_uint16)
		->argument("arg10", commands::ArgumentType::at_uint32)
		->argument("arg11", commands::ArgumentType::at_uint64)
		->argument("arg12", commands::ArgumentType::at_uint8);

	std::array<commands::ArgumentValue, 12> args {
		"Hello, world", 'c', 1.0, 1.0f,
		std::int16_t(0), std::int32_t(0), std::int64_t(0), std::int8_t(0),
		std::uint16_t(0), std::uint32_t(0), std::uint64_t(0), std::uint8_t(0)
	};

	ASSERT_EQ(cmd->execute(args), commands::Result::success);
	ASSERT_TRUE(success);
}