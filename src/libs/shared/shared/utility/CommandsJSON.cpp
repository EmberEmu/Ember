/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandsJSON.h"
#include <commands/Command.h>
#include <nlohmann/json.hpp>
#include <ranges>

namespace ember::utility {

nlohmann::ordered_json command_to_json_recurse(const commands::Command& root) {
    nlohmann::ordered_json json;

    json["name"] = root.name();
    json["description"] = root.description();
	json["arguments"] = nlohmann::ordered_json::array(); // make no args explicit in the output

    for(const auto& arg : root.arguments()) {
        json["arguments"].push_back({
            { "name", arg.name },
            { "required", arg.required },
            { "type", arg.type.get().name() }
        });
    }

    for(const auto& cmd : root.view() | std::views::values) {
        json["subcommands"].push_back(
            command_to_json_recurse(*cmd)
        );
    }

    return json;
}

std::string command_to_json(const commands::Command& root) {
    return command_to_json_recurse(root).dump(4);
}

} // utility, ember