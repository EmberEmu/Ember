/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PatchGraph.h"
#include <boost/optional.hpp>
#include <algorithm>
#include <queue>
#include <set>

namespace ember {

class NodeComp {
public:
	bool operator()(PatchGraph::Node& lhs, PatchGraph::Node& rhs) {
		if(lhs.weight > rhs.weight)
			return true;
		else
			return false;
	}
};

void PatchGraph::build_graph(const std::vector<PatchMeta>& patches) {
	for(auto& patch : patches) {
		adjacency_[patch.build_from].emplace_back(Edge {patch.file_meta.name, patch.build_to, patch.file_meta.size});
	}
}

bool PatchGraph::edge_test(const std::vector<Edge>& edges, std::uint16_t to) const {
	for(auto& e : edges) {
		if(e.build_to == to) {
			return true;
		}
		
		auto it = adjacency_.find(e.build_to);

		if(it != adjacency_.end()) {
			if(edge_test(it->second, to)) {
				return true;
			}
		}
	}

	return false;
}

bool PatchGraph::is_path(std::uint16_t from, std::uint16_t to) const {
	for(auto& entry : adjacency_) {
		if(entry.first <= from) { // todo, need to test for rollup vs incremental
			if(edge_test(entry.second, to)) {
				return true;
			}
		}
	}

	return false;
}

std::deque<PatchGraph::Node> PatchGraph::path(std::uint16_t from, std::uint16_t to) const {
	std::unordered_map<std::uint16_t, std::uint64_t> distance;
	std::unordered_map<std::uint16_t, boost::optional<Node>> prev;
	std::priority_queue<Node, std::vector<Node>, NodeComp> queue;

	for(auto& v : adjacency_) {
		distance[v.first] = -1;
	
		for(auto e : v.second) {
			distance[e.build_to] = -1;
		}
	}

	distance[from] = 0;
	queue.emplace(Node{from, 0});

	while(!queue.empty()) {
		auto next = queue.top();
		queue.pop();

		if(next.from == to) {
			break; // terminate search, found the path we care about
		}

		auto it = adjacency_.find(next.from);

		if(it == adjacency_.end()) {
			continue; // no adjacent nodes
		}

		for(auto& e : adjacency_.at(next.from)) {
			if(distance.at(next.from) + e.filesize < distance.at(e.build_to)) {
				distance[e.build_to] = distance.at(next.from) + e.filesize;
				prev[e.build_to] = next;
				queue.emplace(Node{e.build_to, distance.at(e.build_to)});
			}
		}
	}

	// perform reverse search
	std::deque<Node> path;

	auto it = prev.find(to);

	while(it != prev.end()) {
		auto node = *prev[to];
		path.push_front(node);
		to = node.from;
		it = prev.find(to);
	}

	return path;
}

} // ember