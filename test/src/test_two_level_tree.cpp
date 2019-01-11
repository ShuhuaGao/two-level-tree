#include <catch2/catch.hpp>
#include <numeric>
#include <vector>
#include <cmath>
#include <random>
#include "two_level_tree.h"

// whether a and b are neighbors and a is before b on a forward tour
static bool is_ordered_neighbor(const tsp::TwoLevelTree& tree, int a, int b)
{
	return tree.get_next(tree.get_node(a)) == tree.get_node(b) &&
		tree.get_node(a) == tree.get_prev(tree.get_node(b));
}

static bool is_between(const tsp::TwoLevelTree& tree, int a, int b, int c)
{
	return tree.is_between(tree.get_node(a), tree.get_node(b), tree.get_node(c)); 
}

static std::vector<int> get_tour_via_parents(tsp::TwoLevelTree& tree,  int start_city)
{
	std::vector<int> ans;
	auto p = tree.get_parent_node(start_city);
	do
	{
		auto q = p->forward_begin_node();
		while (q != p->forward_end_node())
		{
			ans.push_back(q->city);
			q = tree.get_next(q);
		}
		ans.push_back(q->city);
		p = p->next;
	} while (p != tree.get_parent_node(start_city));
	return ans;
}

void move_2opt(tsp::TwoLevelTree& tree, int t1, int t2, int t3, int t4)
{
	tree.flip(t1, t2, t4, t3);
}

void undo_2opt_move(tsp::TwoLevelTree& tree, int t1, int t2, int t3, int t4)
{
	//move_2opt(tree, t2, t3, t4, t1);
	tree.flip(t2, t3, t1, t4);
}

TEST_CASE("Build tree from an ordered list of cities", "[two level tree]")
{
	int n_cities = 67, start_city = 2;
	std::vector<int> order(n_cities);
	std::iota(order.begin(), order.end(), start_city);
	std::shuffle(order.begin(), order.end(), std::mt19937{ 123 });
	tsp::TwoLevelTree tree{ n_cities, start_city };
	tree.set_raw_tour(order);

	REQUIRE(tree.n_cities() == n_cities);
	REQUIRE(tree.n_segments() == (int)std::sqrt(n_cities) + 1);

	// traverse the tour
	for (int i = 0; i < n_cities; i++)
	{
		auto city = order[i];
		auto node = tree.get_node(city);
		REQUIRE(node->city == city);
		if (i + 1 < n_cities)
		{
			REQUIRE(node->next->city == order[i + 1]);
		}
		else
		{
			REQUIRE(node->next->city == order[0]);
		}
		if (i - 1 >= 0)
			REQUIRE(node->prev->city == order[i - 1]);
		else
			REQUIRE(node->prev->city == order.back());
	}
	

	// a tour is a cycle
	// (1) next orientation
	int count_city = 0;
	auto node = tree.origin_city_node();
	do
	{
		node = node->next;
		count_city++;
	} while (node != tree.origin_city_node());
	REQUIRE(count_city == n_cities);
	// (2) prev orientation
	node = tree.origin_city_node();
	count_city = 0;
	do
	{
		node = node->prev;
		count_city++;
	} while (node != tree.origin_city_node());
	REQUIRE(count_city == n_cities);
	
	// check each segment
	count_city = 0;
	auto parent_node = tree.head_parent_node();
	do
	{
		REQUIRE(parent_node->segment_end_node->next == parent_node->next->segment_begin_node);
		REQUIRE(parent_node->segment_begin_node->prev == parent_node->prev->segment_end_node);
		count_city += parent_node->size;
		parent_node = parent_node->next;

	} while (parent_node != tree.head_parent_node());
	REQUIRE(count_city == n_cities);

	// // in the initial tour, all segments are splitted in order
	auto first_city = order.front(), last_city = order.back();
	REQUIRE(tree.get_node(first_city)->parent == tree.head_parent_node());
	REQUIRE(tree.get_node(last_city)->parent == tree.tail_parent_node());
	REQUIRE(tree.tail_parent_node()->next == tree.head_parent_node());
	REQUIRE(tree.head_parent_node()->prev == tree.tail_parent_node());
}

TEST_CASE("Prev, Next, and Between", "[two level tree]")
{
	// no reversal yet in this test

	int n_cities = 10, origin = 1;
	std::vector<int> order = { 3, 6, 8, 4, 1, 2, 5, 9, 10, 7 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	// 1. prev and next
	for (int i : {4, 8, 2, 5})
	{
		auto city = order[i], prev_city = order[i - 1], next_city = order[i + 1];
		REQUIRE(tree.get_next(tree.get_node(city)) == tree.get_node(next_city));
		REQUIRE(tree.get_prev(tree.get_node(city)) == tree.get_node(prev_city));
	}

	auto first_city_node = tree.get_node(order.front());
	auto last_city_node = tree.get_node(order.back());
	REQUIRE(tree.get_next(last_city_node) == first_city_node);
	REQUIRE(tree.get_prev(first_city_node) == last_city_node);

	// 2. between
	auto is_between = [&tree](int a, int b, int c) {
		return tree.is_between(tree.get_node(a), tree.get_node(b), tree.get_node(c));
	};
	REQUIRE(is_between(3, 6, 8));
	REQUIRE(is_between(8, 4, 1));
	REQUIRE(is_between(3, 8, 10));
	REQUIRE(is_between(3, 5, 7));
	REQUIRE(is_between(9, 7, 3));
	REQUIRE(is_between(6, 1, 3));
	REQUIRE(is_between(10, 7, 5));
	REQUIRE(is_between(6, 8, 3));
	REQUIRE(is_between(7, 3, 6));
	REQUIRE(is_between(7, 3, 10));
	REQUIRE(is_between(5, 10, 1));
	REQUIRE(is_between(4, 1, 2));
	REQUIRE(is_between(3, 1, 7));
	REQUIRE(is_between(2, 10, 1));
	REQUIRE(is_between(10, 4, 1));
	REQUIRE_FALSE(is_between(6, 4, 8));
	REQUIRE_FALSE(is_between(6, 4, 8));
	REQUIRE_FALSE(is_between(10, 3, 7));
	REQUIRE_FALSE(is_between(10, 1, 8));
	REQUIRE_FALSE(is_between(3, 7, 9));
	REQUIRE_FALSE(is_between(1, 4, 2));
	REQUIRE_FALSE(is_between(6, 3, 10));
}

TEST_CASE("Reverse exactly a complete segment.", "[two level tree]")
{
	int n_cities = 14, origin = 1;
	std::vector<int> order = { 11, 13, 6, 8, 4, 1, 2, 5, 9, 10, 7, 12, 14, 3};
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	REQUIRE(tree.get_node(11)->parent->id == 0);
	REQUIRE(tree.get_node(13)->parent->id == 0);
	REQUIRE(tree.get_node(6)->parent->id == 0);
	REQUIRE(tree.get_node(8)->parent->id == 1);
	REQUIRE(tree.get_node(14)->parent->id == 3);
	REQUIRE(tree.get_node(7)->parent->id == 3);
	REQUIRE(tree.get_node(3)->parent->id == 3);

	tree.reverse(tree.get_node(8), tree.get_node(1));  //[8, 4, 1]
	REQUIRE(tree.get_parent_node(8)->reverse);
	REQUIRE(tree.get_parent_node(4)->reverse);
	REQUIRE(tree.get_parent_node(1)->reverse);
	REQUIRE(tree.get_next(tree.get_node(6)) == tree.get_node(1));
	REQUIRE(tree.get_next(tree.get_node(8)) == tree.get_node(2));
	REQUIRE(tree.get_next(tree.get_node(4)) == tree.get_node(8));
	REQUIRE(tree.get_next(tree.get_node(1)) == tree.get_node(4));
	REQUIRE(tree.is_between(tree.get_node(1), tree.get_node(4), tree.get_node(8)));
	// only change the reversal bit: the beginning and ending nodes remain unchanged
	REQUIRE(tree.get_parent_node(4)->segment_begin_node == tree.get_node(8));
	REQUIRE(tree.get_parent_node(4)->segment_end_node == tree.get_node(1));
	std::vector<int> ans = { 11, 13, 6, 1, 4, 8, 2, 5, 9, 10, 7, 12, 14, 3 };
	REQUIRE(tree.get_raw_tour(11) == ans);
	auto node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_next(node);
	} while (node != tree.origin_city_node());
	REQUIRE_FALSE(tree.is_between(tree.get_node(6), tree.get_node(13), tree.get_node(1)));

	tree.reverse(tree.get_node(11), tree.get_node(6));  //[11, 13, 6]
	REQUIRE(tree.get_parent_node(11)->reverse);
	REQUIRE(tree.get_next(tree.get_node(11)) == tree.get_node(1));
	REQUIRE(tree.get_prev(tree.get_node(13)) == tree.get_node(6));
	REQUIRE(tree.get_next(tree.get_node(13)) == tree.get_node(11));
	REQUIRE(tree.get_prev(tree.get_node(6)) == tree.get_node(3));
	REQUIRE(tree.is_between(tree.get_node(6), tree.get_node(13), tree.get_node(1)));
	ans = { 6, 13, 11, 1, 4, 8, 2, 5, 9, 10, 7, 12, 14, 3 };
	REQUIRE(tree.get_raw_tour(6) == ans);
	node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_prev(node);
	} while (node != tree.origin_city_node());

	tree.reverse(tree.get_node(10), tree.get_node(3)); // [10, 7, 12, 14, 3]
	REQUIRE(tree.get_prev(tree.get_node(3)) == tree.get_node(9));
	REQUIRE(tree.get_prev(tree.get_node(10)) == tree.get_node(7));
	REQUIRE(tree.get_next(tree.get_node(14)) == tree.get_node(12));
	ans = { 6, 13, 11, 1, 4, 8, 2, 5, 9, 3, 14, 12, 7, 10 };
	REQUIRE(tree.get_raw_tour(6) == ans);
	node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_prev(node);
	} while (node != tree.origin_city_node());

	tree.reverse(tree.get_node(6), tree.get_node(11)); // [6, 13, 11]
	REQUIRE(tree.get_parent_node(11)->reverse == false);
	REQUIRE(tree.get_prev(tree.get_node(11)) == tree.get_node(10));
	REQUIRE(tree.get_prev(tree.get_node(13)) == tree.get_node(11));
	REQUIRE(tree.get_next(tree.get_node(6)) == tree.get_node(1));
	ans = { 11, 13, 6, 1, 4, 8, 2, 5, 9, 3, 14, 12, 7, 10 };
	REQUIRE(tree.get_raw_tour(11) == ans);
	node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_prev(node);
	} while (node != tree.origin_city_node());
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));
}

TEST_CASE("Reverse a partial segment with no split-and-merge", "[two level tree]")
{
	int n_cities = 23, origin = 1;
	std::vector<int> order = { 11, 13, 6, 8, 4, 1, 2, 5, 9, 10, 7, 12, 14, 3, 
		15, 16, 17, 18, 20, 19, 23, 22, 21};
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);
	// assert segment 
	std::vector<int> expected_segment_sizes{ 4, 4, 4, 4, 7 };
	REQUIRE(tree.actual_segment_sizes() == expected_segment_sizes);
	// partial reverse. The nominal segment length is 4, thus if the partial segment has a 
	// length <= 3, it is reversed in the segment with not split-merge.
	tree.reverse(tree.get_node(4), tree.get_node(2)); // reverse [4, 1, 2]
	REQUIRE(is_ordered_neighbor(tree, 8, 2));
	REQUIRE(is_ordered_neighbor(tree, 4, 5));
	REQUIRE(is_ordered_neighbor(tree, 2, 1));
	REQUIRE(is_ordered_neighbor(tree, 1, 4));
	REQUIRE_FALSE(is_ordered_neighbor(tree, 4, 1));
	REQUIRE(tree.get_parent_node(1)->segment_begin_node == tree.get_node(2));
	REQUIRE(tree.get_parent_node(1)->segment_end_node == tree.get_node(5));
	std::vector<int> expected_tour = { 11, 13, 6, 8, 2, 1, 4, 5, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21 };
	REQUIRE(tree.get_raw_tour(11) == expected_tour);
	auto node = tree.get_parent_node(1)->segment_begin_node;
	auto end = tree.get_parent_node(1)->segment_end_node;
	while (node != end)
	{
		REQUIRE(node->next->id - node->id == 1);
		node = node->next;
	}

	tree.reverse(tree.get_node(20), tree.get_node(23));  // reverse [20, 19, 23]
	REQUIRE(is_ordered_neighbor(tree, 20, 22));
	REQUIRE(is_ordered_neighbor(tree, 23, 19));
	REQUIRE(is_ordered_neighbor(tree, 18, 23));
	REQUIRE(tree.get_parent_node(17)->segment_begin_node == tree.get_node(17));
	REQUIRE(tree.get_parent_node(20)->segment_end_node == tree.get_node(21));
	// check IDs
	node = tree.get_parent_node(17)->segment_begin_node;
	end = tree.get_parent_node(17)->segment_end_node;
	while (node != end)
	{
		REQUIRE(node->next->id - node->id == 1);
		node = node->next;
	}
	expected_tour = { 11, 13, 6, 8, 2, 1, 4, 5, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 23, 19, 20, 22, 21 };
	REQUIRE(tree.get_raw_tour(11) == expected_tour);


	// let's check first reverse a whole segment
	tree.reverse(tree.get_node(17), tree.get_node(21));  // reverse [17, 18, 23, 19, 20, 22, 21]
	expected_tour = { 11, 13, 6, 8, 2, 1, 4, 5, 9, 10, 7, 12, 14, 3,
		15, 16,    21, 22, 20, 19, 23, 18, 17 };
	REQUIRE(tree.get_raw_tour(11) == expected_tour);
	REQUIRE(tree.get_parent_node(23)->reverse);
	tree.reverse(tree.get_node(23), tree.get_node(17)); // reverse [23, 18, 17]
	expected_tour = { 11, 13, 6, 8, 2, 1, 4, 5, 9, 10, 7, 12, 14, 3,
		15, 16,    21, 22, 20, 19, 17, 18, 23 };
	REQUIRE(tree.get_raw_tour(11) == expected_tour);
	REQUIRE(is_ordered_neighbor(tree, 17, 18));
	REQUIRE(is_ordered_neighbor(tree, 23, 11));
	REQUIRE(is_ordered_neighbor(tree, 19, 17));
	REQUIRE(tree.get_next(tree.get_node(21)) == tree.get_node(22));
	REQUIRE(tree.get_prev(tree.get_node(11)) == tree.get_node(23));
	REQUIRE(is_between(tree, 11, 22, 23));
	REQUIRE(is_between(tree, 18, 23, 1));
	REQUIRE(is_between(tree, 5, 7, 3));
	REQUIRE_FALSE(is_between(tree, 15, 18, 22));
	// check IDs
	node = tree.get_parent_node(22)->segment_begin_node;
	end = tree.get_parent_node(22)->segment_end_node;
	while (node != end)
	{
		REQUIRE(node->next->id - node->id == 1);
		node = node->next;
	}
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));
}

TEST_CASE("Split and merge", "[two level tree]")
{
	int n_cities = 23, origin = 1;
	std::vector<int> order = { 11, 13, 6, 8, 4, 1, 2, 5, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);
	// assert segment 
	std::vector<int> expected_segment_sizes{ 4, 4, 4, 4, 7 };
	REQUIRE(tree.actual_segment_sizes() == expected_segment_sizes);

	tree.split_and_merge(tree.get_node(6), true, tsp::Direction::forward);
	REQUIRE(tree.get_parent_node(6) == tree.get_parent_node(4));
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{2, 6, 4, 4, 7});
	REQUIRE(tree.get_raw_tour(6) == std::vector<int>{6, 8, 4, 1, 2, 5, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21, 11, 13});
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13, 6, 8, 4, 1, 2, 5, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21});

	// reverse the 2nd segment [6, 8, 4, 1, 2, 5]
	tree.reverse(tree.get_node(6), tree.get_node(5));
	REQUIRE(tree.get_raw_tour(10, tsp::Direction::backward) == std::vector<int>{10, 9, 6, 8, 4, 1, 2, 5,
		13, 11, 21, 22, 23, 19, 20, 18, 17, 16, 15, 3, 14, 12, 7});
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13, 5, 2, 1, 4, 8, 6, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21});
	REQUIRE(tree.get_parent_node(4)->reverse);
	// split and merge will not change the tour
	tree.split_and_merge(tree.get_node(4), true, tsp::Direction::forward);
	REQUIRE(tree.get_raw_tour(1) == std::vector<int>{1, 4, 8, 6, 9, 10, 7, 12, 14, 3,
		15, 16, 17, 18, 20, 19, 23, 22, 21, 11, 13, 5, 2});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{2, 3, 7, 4, 7});
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13,  5, 2, 1,  4, 8, 6, 9, 10, 7, 12,  
		14, 3, 15, 16,  17, 18, 20, 19, 23, 22, 21});
	REQUIRE(tree.get_parent_node(2)->reverse);
	REQUIRE_FALSE(tree.get_parent_node(4)->reverse);

	// try backward merge
	tree.split_and_merge(tree.get_node(19), false, tsp::Direction::backward); //doesn't include 19
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13,   5, 2, 1,   4, 8, 6, 9, 10, 7, 12,  
		14, 3, 15, 16, 17, 18, 20,   19, 23, 22, 21});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{2, 3, 7, 7, 4});
	REQUIRE(tree.get_parent_node(19)->segment_begin_node == tree.get_node(19));
	REQUIRE(tree.get_parent_node(16)->segment_end_node == tree.get_node(20));
	REQUIRE(tree.get_parent_node(2)->reverse);

	// try another backward merge, note that the segment for [5, 2, 1] has the reverse bit
	tree.split_and_merge(tree.get_node(10), true, tsp::Direction::backward);
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13,   5, 2, 1, 4, 8, 6, 9, 10,   7, 12,  
		14, 3, 15, 16, 17, 18, 20,   19, 23, 22, 21});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{2, 8, 2, 7, 4});
	REQUIRE(tree.get_parent_node(9)->reverse);
	REQUIRE(tree.get_parent_node(9)->segment_end_node == tree.get_node(5));
	REQUIRE(tree.get_parent_node(7)->segment_end_node == tree.get_node(12));
	REQUIRE(tree.get_parent_node(9)->segment_begin_node == tree.get_node(10));
	REQUIRE(tree.get_parent_node(12)->segment_begin_node == tree.get_node(7));

	// another one, here the segment containing 5 has the reverse bit
	tree.split_and_merge(tree.get_node(2), true, tsp::Direction::forward);
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{11, 13,   5,    2, 1, 4, 8, 6, 9, 10, 7, 12,  
		14, 3, 15, 16, 17, 18, 20,   19, 23, 22, 21});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{2, 1, 9, 7, 4});
	REQUIRE(tree.get_parent_node(5)->reverse);
	REQUIRE_FALSE(tree.get_parent_node(1)->reverse);
	REQUIRE(tree.get_parent_node(5)->segment_begin_node == tree.get_node(5));
	REQUIRE(tree.get_parent_node(5)->segment_end_node == tree.get_node(5));
	REQUIRE(tree.get_parent_node(12)->segment_begin_node == tree.get_node(2));
	REQUIRE(tree.get_parent_node(2)->segment_end_node == tree.get_node(12));
	REQUIRE(tree.get_raw_tour(2, tsp::Direction::backward) == std::vector<int>{2, 5, 13, 11, 21, 22, 23, 
		19, 20, 18, 17, 16, 15, 3, 14, 12, 7, 10, 9, 6, 8, 4, 1});
}

TEST_CASE("Reverse a partial segment with split-and-merge", "[two level tree]")
{
	int n_cities = 23, origin = 1;
	std::vector<int> order = { 11, 13, 6, 8,   4, 1, 2, 5,   9, 10, 7, 12,   14, 3, 15, 16, 
		17, 18, 20, 19, 23, 22, 21 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);
	// assert segment 
	std::vector<int> expected_segment_sizes{ 4, 4, 4, 4, 7 };
	REQUIRE(tree.actual_segment_sizes() == expected_segment_sizes);

	// the nominal length is 4, if a part to be reversed is > 3, then split and merge is used.
	tree.reverse(tree.get_node(18), tree.get_node(23));
	REQUIRE(tree.get_raw_tour(22) == std::vector<int>{22, 21, 11, 13, 6, 8,   4, 1, 2, 5,   9, 10, 7, 12,   
		14, 3, 15, 16, 17,   23, 19, 20, 18});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{6, 4, 4, 5, 4});
	REQUIRE(tree.get_parent_node(18)->reverse);
	REQUIRE_FALSE(tree.get_parent_node(22)->reverse);

	// how about to reverse [11, 13, 6, 8], note that no forward merging is actually needed
	tree.reverse(tree.get_node(11), tree.get_node(8));
	REQUIRE(tree.get_raw_tour(8) == std::vector<int>{ 8, 6, 13, 11,   4, 1, 2, 5,   9, 10, 7, 12,   
		14, 3, 15, 16, 17,   23, 19, 20, 18, 22, 21});
	REQUIRE(tree.get_parent_node(22)->reverse);
	REQUIRE(tree.get_parent_node(21)->segment_begin_node == tree.get_node(21));
	REQUIRE(tree.get_parent_node(21)->segment_end_node == tree.get_node(23));
	REQUIRE(tree.get_parent_node(8)->reverse);
	REQUIRE(tree.get_raw_tour(12, tsp::Direction::backward) == std::vector<int>{12, 7, 10, 9, 5, 2, 1, 
		4, 11, 13, 6, 8, 21, 22, 18, 20, 19, 23, 17, 16, 15, 3, 14});

	// reverse [19, 20, 18, 22], whose reverse bit is set
	tree.reverse(tree.get_node(19), tree.get_node(22));
	REQUIRE(tree.get_raw_tour(21) == std::vector<int>{ 21, 8, 6, 13, 11,   4, 1, 2, 5,   9, 10, 7, 12,   
		14, 3, 15, 16, 17, 23,   22, 18, 20, 19});
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{5, 4, 4, 6, 4});
	REQUIRE_FALSE(tree.get_parent_node(19)->reverse);
	REQUIRE(tree.get_parent_node(19)->segment_begin_node == tree.get_node(22));
	REQUIRE(tree.get_parent_node(21)->reverse);
	REQUIRE(tree.get_parent_node(21)->segment_begin_node == tree.get_node(11));
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));
}

TEST_CASE("Reverse multiple segments with split-and-merge", "[two level tree]")
{
	int n_cities = 23, origin = 1;
	std::vector<int> order = { 11, 13, 6, 8,   4, 1, 2, 5,   9, 10, 7, 12,   14, 3, 15, 16,
		17, 18, 20, 19, 23, 22, 21 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);
	// assert segment 
	std::vector<int> expected_segment_sizes{ 4, 4, 4, 4, 7 };
	REQUIRE(tree.actual_segment_sizes() == expected_segment_sizes);

	// though a and b are not in the same segment, after split-and-merge, they are.
	tree.reverse(tree.get_node(6), tree.get_node(4));
	REQUIRE(tree.get_raw_tour(11) == std::vector<int>{ 11, 13,   4, 8, 6, 1, 2, 5,   9, 10, 7, 12,   
		14, 3, 15, 16,   17, 18, 20, 19, 23, 22, 21 });
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{ 2, 6, 4, 4, 7 });
	REQUIRE(tree.get_parent_node(4)->segment_begin_node == tree.get_node(4));

	tree.reverse(tree.get_node(22), tree.get_node(8));
	REQUIRE(tree.get_raw_tour(8) == std::vector<int>{8, 4, 13, 11, 21, 22,   6, 1, 2, 5,   9, 10, 7, 12,
		14, 3, 15, 16,   17, 18, 20, 19, 23 });
	REQUIRE(tree.get_parent_node(8)->reverse);
	REQUIRE(tree.get_parent_node(22)->reverse);
	REQUIRE(tree.get_parent_node(8)->segment_end_node == tree.get_node(8));
	REQUIRE_FALSE(tree.get_parent_node(23)->reverse);
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{ 6, 4, 4, 4, 5 });

	// reverse multiple segments
	tree.reverse(tree.get_node(13), tree.get_node(5));  // now [8, 4] are deprived
	REQUIRE(tree.get_raw_tour(5) == std::vector<int>{5, 2, 1, 6,   22, 21, 11, 13,   9, 10, 7, 12,
		14, 3, 15, 16,    17, 18, 20, 19, 23, 8, 4});
	REQUIRE_FALSE(tree.get_parent_node(22)->reverse);
	REQUIRE(tree.get_parent_node(2)->reverse);
	REQUIRE(tree.actual_segment_sizes() == std::vector<int>{ 4, 4, 4, 4, 7 });

	tree.reverse(tree.get_node(6), tree.get_node(14));
	REQUIRE(tree.get_raw_tour(13) == std::vector<int>{13, 11, 21, 22, 6, 
		3, 15, 16,    17, 18, 20, 19, 23, 8, 4,    5, 2, 1,   14, 12, 7, 10, 9});
	REQUIRE(tree.actual_segment_sizes(13) == std::vector<int>{ 5, 3, 7, 3, 5 });

	// let's traverse via the parents
	auto p_parent = tree.head_parent_node();
	do
	{
		REQUIRE(p_parent->next->prev == p_parent);
		REQUIRE(p_parent->prev->next == p_parent);
		REQUIRE((p_parent->id + 1) % tree.n_segments() == p_parent->next->id);
		p_parent = p_parent->next;
	} while (p_parent != tree.head_parent_node());

	std::vector<int> segment_size_res, node_res;
	p_parent = tree.get_parent_node(13);
	do
	{
		segment_size_res.push_back(p_parent->size);
		auto p = p_parent->forward_begin_node();
		while (p != p_parent->forward_end_node())
		{
			node_res.push_back(p->city);
			p = tree.get_next(p);
		}
		node_res.push_back(p->city);
		p_parent = p_parent->next;
	} while (p_parent != tree.get_parent_node(13));
	REQUIRE(segment_size_res == std::vector<int>{ 5, 3, 7, 3, 5 });
	REQUIRE(node_res == tree.get_raw_tour(13));

	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));
}

TEST_CASE("flip", "[two level tree]")
{
	int n_cities = 12, origin = 1;
	std::vector<int> order = { 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	tree.flip(tree.get_node(3), tree.get_node(6), tree.get_node(10), tree.get_node(7));
	REQUIRE(tree.get_raw_tour(6) == std::vector<int>{6, 8, 4, 1, 12, 2, 5, 9, 10, 3, 11, 7});
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));

	tree.reverse(tree.get_node(4), tree.get_node(10));
	REQUIRE(tree.get_raw_tour(6) == std::vector<int>{6, 8, 10, 9, 5, 2, 12, 1, 4, 3, 11, 7});
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));

	tree.flip(tree.get_node(8), tree.get_node(10), tree.get_node(7), tree.get_node(6));
	REQUIRE(tree.get_raw_tour(10) == std::vector<int>{10, 9, 5, 2, 12, 1, 4, 3, 11, 7, 8, 6});
	REQUIRE(get_tour_via_parents(tree, 1) == tree.get_raw_tour(tree.get_parent_node(1)->forward_begin_node()->city));

	auto node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_prev(node);
	} while (node != tree.origin_city_node());

	// check ID of the 2 segment
	node = tree.get_parent_node(2)->segment_begin_node;
	auto end = tree.get_parent_node(2)->segment_end_node;
	while (node != end)
	{
		REQUIRE(node->next->id - node->id == 1);
		node = node->next;
	}

	// backward
	REQUIRE(tree.get_raw_tour(10) == std::vector<int>{10, 9, 5, 2, 12, 1, 4, 3, 11, 7, 8, 6});
	tree.flip(1, 12, 9, 10);
	REQUIRE(( tree.get_raw_tour(1) == std::vector<int>{1, 9, 5, 2, 12, 10, 6, 8, 7, 11, 3, 4 }));

	tree.flip(10, 6, 8, 7);
	REQUIRE(tree.get_raw_tour(10) == std::vector<int>{10, 8, 6, 7, 11, 3, 4, 1, 9, 5, 2, 12});
}

TEST_CASE("(t1, t2, t3, t4) 2-opt move and undo", "[two level tree]")
{
	int n_cities = 12, origin = 1;
	std::vector<int> order = { 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	move_2opt(tree, 5, 9, 3, 11);
	REQUIRE(tree.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 11, 7, 10, 9});
	// undo
	undo_2opt_move(tree, 5, 9, 3, 11);
	REQUIRE(tree.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11});
	REQUIRE(tree.get_raw_tour(12) == std::vector<int>{12, 2, 5, 9, 10, 7, 11, 3, 6, 8, 4, 1});

	move_2opt(tree, 12, 2, 7, 10);
	REQUIRE((tree.get_raw_tour(3) == std::vector<int>{3, 11, 7, 2, 5, 9, 10, 12, 1, 4, 8, 6} 
	|| tree.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 10, 9, 5, 2, 7, 11}));;
	// undo
	undo_2opt_move(tree, 12, 2, 7, 10);
	REQUIRE(tree.get_raw_tour(12) == std::vector<int>{12, 2, 5, 9, 10, 7, 11, 3, 6, 8, 4, 1});
	REQUIRE(tree.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11});
}

TEST_CASE("Tree deep copy/move and independency", "[two level tree]")
{
	int n_cities = 12, origin = 1;
	std::vector<int> order = { 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	auto tree2 = tree;
	REQUIRE(tree2.get_raw_tour(3) == std::vector<int>{ 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 });

	move_2opt(tree, 5, 9, 3, 11);
	REQUIRE(tree.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 11, 7, 10, 9});
	REQUIRE(tree2.get_raw_tour(3) == std::vector<int>{ 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 });
	REQUIRE(tree2.get_raw_tour(5) == std::vector<int>{5, 9, 10, 7, 11, 3, 6, 8, 4, 1, 12, 2});

	tsp::TwoLevelTree tree3 = std::move(tree);
	REQUIRE(tree3.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 11, 7, 10, 9});
	undo_2opt_move(tree3, 5, 9, 3, 11);
	REQUIRE(tree3.get_raw_tour(3) == std::vector<int>{3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11});
}

TEST_CASE("Double bridge move", "[two level tree]")
{
	int n_cities = 12, origin = 1;
	std::vector<int> order = { 3, 6, 8, 4, 1, 12, 2, 5, 9, 10, 7, 11 };
	tsp::TwoLevelTree tree{ n_cities, origin };
	tree.set_raw_tour(order);

	tree.double_bridge_move(12, 5, 11, 8);
	REQUIRE(tree.get_raw_tour(2) == std::vector<int>{2, 5, 4, 1, 12, 3, 6, 8, 9, 10, 7, 11});
	// let's traverse via the parents
	auto p_parent = tree.head_parent_node();
	int id = 0;
	do
	{
		REQUIRE(p_parent->id == id);
		REQUIRE(p_parent->next->prev == p_parent);
		REQUIRE(p_parent->prev->next == p_parent);
		REQUIRE((p_parent->id + 1) % tree.n_segments() == p_parent->next->id);
		p_parent = p_parent->next;
		id++;
	} while (p_parent != tree.head_parent_node());

	tree.double_bridge_move(3, 9, 2, 4);
	REQUIRE(tree.get_raw_tour(2) == std::vector<int>{2, 6, 8, 9, 1, 12, 3, 5, 4, 10, 7, 11});
	auto node = tree.origin_city_node();
	do
	{
		REQUIRE(tree.get_next(tree.get_prev(node)) == node);
		REQUIRE(tree.get_prev(tree.get_next(node)) == node);
		node = tree.get_next(node);
	} while (node != tree.origin_city_node());

	p_parent = tree.head_parent_node();
	id = 0;
	do
	{
		REQUIRE(p_parent->id == id);
		REQUIRE(p_parent->next->prev == p_parent);
		REQUIRE(p_parent->prev->next == p_parent);
		REQUIRE((p_parent->id + 1) % tree.n_segments() == p_parent->next->id);
		p_parent = p_parent->next;
		id++;
	} while (p_parent != tree.head_parent_node());

	tree.double_bridge_move(5, 11, 6, 1);
	REQUIRE(tree.get_raw_tour(4) == std::vector<int>{4, 10, 7, 11, 12, 3, 5, 8, 9, 1, 2, 6});
	p_parent = tree.head_parent_node();
	int size = 0;
	do
	{
		REQUIRE(p_parent->next->prev == p_parent);
		REQUIRE(p_parent->prev->next == p_parent);
		REQUIRE((p_parent->id + 1) % tree.n_segments() == p_parent->next->id);
		if (!p_parent->reverse)
		{
			if (p_parent->next->reverse)
				REQUIRE(tree.get_next(p_parent->segment_end_node) == p_parent->next->segment_end_node);
			else
				REQUIRE(tree.get_next(p_parent->segment_end_node) == p_parent->next->segment_begin_node);
		}
		size += p_parent->size;
		p_parent = p_parent->next;
	} while (p_parent != tree.head_parent_node());
	assert(size == 12);
}