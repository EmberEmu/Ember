/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/objects/PatchMeta.h>
#include <deque>
#include <string>
#include <unordered_map>
#include <span>
#include <vector>
#include <cstdint>

namespace ember {

struct Edge {
	std::uint16_t build_to;
	std::uint64_t filesize;
};

class PatchGraph final {
	std::unordered_map<std::uint16_t, std::vector<Edge>> adjacency_;

	void build_graph(std::span<const PatchMeta> patches);
	bool edge_test(std::span<const Edge> edges, std::uint16_t to) const;

public:
	struct Node {
		std::uint16_t from;
		std::uint64_t weight;
	};

	explicit PatchGraph(std::span<const PatchMeta> patches) {
		build_graph(patches);
	}

	std::deque<Node> path(std::uint16_t from, std::uint16_t to) const;
	bool is_path(std::uint16_t from, std::uint16_t to) const;
};

} // ember