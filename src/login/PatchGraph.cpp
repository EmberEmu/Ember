/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PatchGraph.h"

namespace ember {

void PatchGraph::build_graph(const std::vector<PatchMeta>& patches) {
	for(auto& patch : patches) {
		adjacency_[patch.build_from].edges.emplace_back(Edge {patch.build_to, patch.file_meta.size});
	}
}

bool PatchGraph::edge_test(const std::vector<Edge>& edges, std::uint16_t to) {
	for(auto& e : edges) {
		if(e.build_to == to) {
			return true;
		} else {
			auto it = adjacency_.find(e.build_to);

			if(it != adjacency_.end()) {
				if(edge_test(it->second.edges, to)) {
					return true;
				}
			}
		}
	}

	return false;
}

bool PatchGraph::is_path(std::uint16_t from, std::uint16_t to) {
	for(auto& entry : adjacency_) {
		if(entry.first <= from) { // todo, need to test for rollup vs incremental
			if(edge_test(entry.second.edges, to)) {
				return true;
			}
		}
	}

	return false;
}

} // ember