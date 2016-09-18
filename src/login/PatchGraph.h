/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/objects/PatchMeta.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ember {

struct Edge {
	std::uint16_t build_to;
	std::uint64_t filesize;
};

struct Node {
	std::vector<Edge> edges;
	std::uint32_t cost;
};

class PatchGraph {
	std::unordered_map<std::uint16_t, Node> adjacency_;

	void build_graph(const std::vector<PatchMeta>& patches);

public:
	PatchGraph(const std::vector<PatchMeta>& patches) {
		build_graph(patches);
	}

	bool edge_test(const std::vector<Edge>& edges, std::uint16_t to);
	bool is_path(std::uint16_t from, std::uint16_t to);
};

} // ember