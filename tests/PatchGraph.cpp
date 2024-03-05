/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <login/PatchGraph.h>
#include <gtest/gtest.h>
#include <array>

using namespace ember;

const std::array<PatchMeta, 10> patches {
	 {{0, 1, 2, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "1_to_2.patch", {}, 1   },
	 { 1, 2, 3, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "2_to_3.patch", {}, 1   },
	 { 2, 1, 4, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "1_to_4.patch", {}, 100 },
	 { 3, 3, 4, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "3_to_4.patch", {}, 1   },
	 { 4, 5, 6, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "5_to_6.patch", {}, 1   },
	// path break
	 { 5, 6, 7, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "6_to_7.patch",   {}, 1 },
	 { 6, 7, 8, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "7_to_8.patch",   {}, 1 },
	 { 7, 8, 9, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "8_to_9.patch",   {}, 1 },
	 { 8, 9, 10, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "9_to_10.patch", {}, 1 },
	 { 9, 6, 10, 0, 0, 0, "x86", "enGB", "Win", false, false, "", "6_to_10.patch", {}, 3 }}
};

TEST(PatchGraph, NoPath) {
	PatchGraph graph(patches);
	ASSERT_FALSE(graph.is_path(1, 6));
	const auto path = graph.path(1, 6);
	ASSERT_TRUE(path.empty());
}

TEST(PatchGraph, AnyPath) {
	PatchGraph graph(patches);
	ASSERT_TRUE(graph.is_path(1, 4));
	const auto path = graph.path(1, 4);
	ASSERT_FALSE(path.empty());
}

/*
  Ensure that the graph picks the lowest weight path even if
  it is also the longest path (more files but lower download size)
*/
TEST(PatchGraph, OptimalPathLonger) {
	PatchGraph graph(patches);
	ASSERT_TRUE(graph.is_path(1, 4));
	const auto path = graph.path(1, 4);
	ASSERT_EQ(path.size(), 3);
	
	auto cost = decltype(PatchGraph::Node::weight){};

	for(auto& node : path) {
		cost += node.weight;
	}

	ASSERT_EQ(cost, 3);
}

/*
  Ensure that the graph picks the lowest weight path when it
  is also the shortest path (minimal files and minimal download size)
 */
TEST(PatchGraph, OptimalPathShorter) {
	PatchGraph graph(patches);
	ASSERT_TRUE(graph.is_path(6, 10));
	const auto path = graph.path(6, 10);
	ASSERT_EQ(path.size(), 1);

	auto cost = decltype(PatchGraph::Node::weight){};

	for(auto& node : path) {
		cost += node.weight;
	}
	
	ASSERT_EQ(cost, 0);
}