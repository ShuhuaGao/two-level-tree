#include <cmath>
#include <cassert>
#include <functional>
#include "two_level_tree.h"

namespace tsp
{
	TwoLevelTree::TwoLevelTree(int n_cities, int origin_city)
		: _nodes(n_cities + origin_city), _parent_nodes(static_cast<int>(std::sqrt(n_cities)) + 1),
			_n_cities{n_cities}, _origin_city{origin_city}
	{
		assert(n_cities > 0);
		assert(origin_city >= 0);
		assert(n_segments() > 1); // we didn't handle the case where only one segment exists
		_nominal_segment_length = n_cities / n_segments();
	}

	TwoLevelTree::TwoLevelTree(const TwoLevelTree & other)
		: _n_cities{other._n_cities}, _origin_city{other._origin_city}, 
		_nodes(other._nodes.size()), 
		_parent_nodes(other._parent_nodes.size()),
		_nominal_segment_length{other._nominal_segment_length}
	{
		set_raw_tour(other.get_raw_tour());
	}

	TwoLevelTree & TwoLevelTree::operator=(const TwoLevelTree & other)
	{
		if (this == &other)
			return *this;
		_n_cities = other._n_cities;
		_origin_city = other._origin_city;
		_nodes.clear();
		_nodes.resize(other._nodes.size());
		_parent_nodes.clear();
		_parent_nodes.resize(other._parent_nodes.size());
		_nominal_segment_length = other._nominal_segment_length;
		set_raw_tour(other.get_raw_tour());
		_temp_nodes.clear();
		_temp_parent_nodes.clear();
		return *this;
	}


	void TwoLevelTree::set_raw_tour(const std::vector<int>& order)
	{
		assert(order.size() == _n_cities);
		int n = n_segments();
		int segment_length = _n_cities / n;
		int first_city = order.front();
		int last_city = order.back();

		for (int current_segment = 0; current_segment < n; current_segment++)
		{
			// first build the parent for this segment
			auto parent = &_parent_nodes[current_segment];
			parent->id = current_segment;
			parent->prev = current_segment > 0 ? &_parent_nodes[current_segment - 1] : tail_parent_node();
			parent->next = current_segment + 1 < n ? &_parent_nodes[current_segment + 1] : head_parent_node();
			parent->reverse = false;
			// this segment range in the given order tour (the end excluded)
			int i_begin = current_segment * segment_length;
			int i_end = i_begin + segment_length;
			// the last segment takes all the remaining cities
			if (current_segment == n - 1)
				i_end = _n_cities;
			parent->segment_begin_node = &_nodes[order[i_begin]];
			parent->segment_end_node = &_nodes[order[i_end - 1]];
			parent->size = i_end - i_begin;

			// build the segment node one by one
			for (int i = i_begin; i < i_end; i++)
			{
				int city = order[i];
				assert(is_city_valid(city));
				auto node = get_node(city);
				node->city = city;
				node->parent = parent;
				// cycle tour
				node->prev = i == 0 ? get_node(last_city) : get_node(order[i - 1]);
				node->next = i + 1 == _n_cities ? get_node(first_city) : get_node(order[i + 1]);
				node->id = i - i_begin;
			}
		}
	}

	bool TwoLevelTree::is_between(const Node * a, const Node * b, const Node * c) const
	{
		assert(a != b && a != c && b != c);
		auto pa = a->parent, pb = b->parent, pc = c->parent;
		// all same parents: in a single segment
		if (pa == pb && pb == pc)
		{
			if (pa->reverse)
			{
				if (c->id < a->id)
					return b->id < a->id && b->id > c->id;
				else
					return b->id < a->id || b->id > c->id;
			}
			else
			{
				if (c->id > a->id)
					return b->id > a->id && b->id < c->id;
				else
					return b->id > a->id || b->id < c->id;

			}
		}
		// all three parents are distinct
		if (pa != pb && pa != pc && pb != pc)
		{
			// note that the parents are in a cyclcal list
			if (pc->id > pa->id)
			{
				return pb->id > pa->id && pb->id < pc->id;
			}
			else
			{
				return pb->id > pa->id || pb->id < pc->id;
			}

		}
		// now: two nodes share the parent, one different
		// only true if we can reach v from u before leaving a the segment
		auto can_reach_in_current_segment = [](const Node* u, const Node* v) {
			assert(u->parent == v->parent);
			if (u->parent->reverse)
				return v->id < u->id;
			return u->id < v->id;
		};
		if (pa == pb)
			return can_reach_in_current_segment(a, b);
		if (pb == pc)
		{
			if (pb->reverse)
				return b->id > c->id;
			return b->id < c->id;
		}
		// pa == pc
		return !can_reach_in_current_segment(a, c);
	}

	bool TwoLevelTree::is_between(int a, int b, int c) const
	{
		return is_between(get_node(a), get_node(b), get_node(c));
	}

	// To facilitate implementation, we use implicit rebalance here, because in practice the 
	// complicated full rebalance is empricially unnecessary.
	void TwoLevelTree::reverse(Node * a, Node * b)
	{
		if (a == b || get_next(b) == a)
			return;
		// (1) the path is contained in a single segment
		if (is_path_in_single_segment(a, b))
		{
			reverse_segment(a, b);
		}
		else  // (2) multiple segments are involved, we simply split and merge to make complete segments
		{
			// split and merge the smaller half heuristically
			auto split_and_merge_a = [a, this]() {
				if (a == a->parent->forward_begin_node())
					return;
				auto a_forward_end = a->parent->forward_end_node();
				int	a_forward_half_length = std::abs(a_forward_end->id - a->id) + 1;
				if (a_forward_half_length <= a->parent->size / 2)
					split_and_merge(a, true, Direction::forward);
				else
					split_and_merge(a, false, Direction::backward);
			};
			auto split_and_merge_b = [a, b, this]() {
				if (b == b->parent->backward_begin_node())
					return;
				// to handle the special cases: [......b..] -> [a......] (i.e., reverse almost a full circle)
				if (b->parent->next == a->parent)
				{
					split_and_merge(b, true, Direction::backward);
					return;
				}
				auto b_backward_end = b->parent->backward_end_node();
				auto b_backward_half_length = std::abs(b_backward_end->id - b->id) + 1;
				if (b_backward_half_length <= b->parent->size / 2)
					split_and_merge(b, true, Direction::backward);
				else
					split_and_merge(b, false, Direction::forward);
			};
			
			split_and_merge_a();
			// whether a and b are in a single segment now
			if (is_path_in_single_segment(a, b))
			{
				reverse_segment(a, b);
				return;
			}
			// split and merge b similarly
			split_and_merge_b();
			// whether a and b are in a single segment now
			if (is_path_in_single_segment(a, b))
			{
				reverse_segment(a, b);
				return;
			}
			// now the forward path a ----> b contains multiple complete segments even after merge
			// suppose s1 [a...] [....] [....] [....b] s2
			// note that in a forward path, we always go to .next for the parent node
			assert((!a->parent->reverse && a == a->parent->segment_begin_node)
				|| (a->parent->reverse && a == a->parent->segment_end_node));
			assert((!b->parent->reverse && b == b->parent->segment_end_node)
				|| (b->parent->reverse && b == b->parent->segment_begin_node));
			_temp_parent_nodes.clear();
			// (a) each segment between a and b should be reversed
			auto s1 = a->parent->prev;
			auto s2 = b->parent->next;
			_temp_parent_nodes.push_back(s2);
			auto p = a->parent;
			while (p != s2)
			{
				p->reverse = !p->reverse;
				_temp_parent_nodes.push_back(p);
				p = p->next;
			}
			// (b) reverse the positions of the segments and reconnect them between s1 and s2
			// we also need to update the ID and adjust the connections in the ends of each segment
			p = s1;
			int n_parents = static_cast<int>(_parent_nodes.size());
			while (!_temp_parent_nodes.empty())
			{
				auto q = _temp_parent_nodes.back();
				_temp_parent_nodes.pop_back();
				// p -> q forward
				p->next = q;
				q->prev = p;
				q->id = (p->id + 1) % n_parents; // all parents nodes are placed in a cyclic list
				// the neighbor nodes of p and q segments should be connected properly
				auto p_last = p->forward_end_node();
				auto q_first = q->forward_begin_node();
				connect_arc_forward(p_last, q_first);
				p = q;
			}
			// now p is s2
			assert((p->id + 1) % n_parents == p->next->id);
		}
	}

	// TODO : optimize by using a raw_tour cache
	std::vector<int> TwoLevelTree::get_raw_tour(int start_city, Direction direction) const
	{
		if (start_city < 0)
			start_city = origin_city();
		assert(start_city >= origin_city() && start_city < _nodes.size());
		auto node = get_node(start_city);
		std::vector<int> raw_tour;
		raw_tour.resize(_n_cities);
		for (int i = 0; i < _n_cities; ++i)
		{
			raw_tour[i] = node->city;
			if (direction == Direction::forward)
				node = get_next(node);
			else
				node = get_prev(node);
		}
		return raw_tour;
	}

	void TwoLevelTree::to_raw_tour(std::vector<int>& raw_tour, int start_city, Direction direction) const
	{
		if (start_city < 0)
			start_city = origin_city();
		assert(start_city >= origin_city() && start_city < _nodes.size());
		auto node = get_node(start_city);
		raw_tour.resize(_n_cities);
		for (int i = 0; i < _n_cities; ++i)
		{
			raw_tour[i] = node->city;
			if (direction == Direction::forward)
				node = get_next(node);
			else
				node = get_prev(node);
		}
	}

	std::vector<int> TwoLevelTree::actual_segment_sizes(int start_city) const
	{
		if (is_city_valid(start_city))
		{
			std::vector<int> ans;
			ans.reserve(_parent_nodes.size());
			auto start_parent = get_parent_node(start_city);
			auto p = start_parent;
			do
			{
				ans.push_back(p->size);
				p = p->next;
			} while (p != start_parent);
			return ans;
		}
		else
		{
			std::vector<int> ans(_parent_nodes.size());
			for (std::size_t i = 0; i < _parent_nodes.size(); i++)
			{
				ans[i] = _parent_nodes[i].size;
			}
			return ans;
		}
		
	}

	bool TwoLevelTree::is_approximately_shorter(Node * a, Node * b, Node * c, Node * d) const
	{
		
		int n_segments_ab = count_n_segments(a, b);
		int n_segments_cd = count_n_segments(c, d);
		if (n_segments_ab != n_segments_cd)
			return n_segments_ab < n_segments_cd;
		int excluded_length_a = std::abs(a->id - a->parent->forward_begin_node()->id);
		int excluded_length_b = std::abs(b->id - b->parent->forward_end_node()->id);
		int excluded_length_c = std::abs(c->id - c->parent->forward_begin_node()->id);
		int excluded_length_d = std::abs(d->id - d->parent->forward_end_node()->id);
		return excluded_length_a + excluded_length_b > excluded_length_c + excluded_length_d;
	}

	// forward path a --> b
	int TwoLevelTree::count_n_segments(Node * a, Node * b) const
	{
		int n = n_segments();
		auto apid = a->parent->id, bpid = b->parent->id;
		// how many segments are involved in the forward path a --> b
		if (apid == bpid)
		{
			// whether the forward a --> b is in the single segment
			auto ap = a->parent;
			if ((!ap->reverse && a->id < b->id) || (ap->reverse && a->id > b->id))
				return 1;
			return n;
		}
		if (bpid > apid)
			return bpid - apid + 1;
		return bpid + n - apid + 1;
	}

	bool TwoLevelTree::has_edge(int city1, int city2) const
	{
		auto a = get_node(city1);
		auto b = get_next(a);
		if (b->city == city2)
			return true;
		b = get_prev(a);
		if (b->city == city2)
			return true;
		return false;
	}

	bool TwoLevelTree::has_edge(const Node * a, const Node * b) const
	{
		return get_next(a) == b || get_prev(a) == b;
	}

	std::pair<int, int> TwoLevelTree::turn_forward(int city1, int city2) const
	{
		assert(get_next(city1) == city2 || get_prev(city1) == city2);
		if (get_next(city1) == city2)
			return { city1, city2 };
		return { city2, city1 };
	}

	bool TwoLevelTree::is_path_in_single_segment(const Node * a, const Node * b) const
	{
		if (a->parent == b->parent)
		{
			if (a->parent->reverse)
				return a->id > b->id;
			return a->id < b->id;
		}
		return false;
	}

	int TwoLevelTree::forward_distance(const Node * a, const Node * b) const
	{
		int count = 1;
		while (a != b)
		{
			count++;
			a = get_next(a);
		}
		return count;
	}

	void TwoLevelTree::reverse_segment(Node * a, Node * b)
	{
		assert(a->parent == b->parent);
		auto parent = a->parent;
		// if exactly a complete segment
		if ((a == parent->segment_begin_node && b == parent->segment_end_node) ||
			(b == parent->segment_begin_node && a == parent->segment_end_node))
		{
			reverse_complete_segment(a, b);
		}
		else  // only a part of the segment
		{
			auto path_length = std::abs(a->id - b->id) + 1;  // IDs are consecutive
			if (path_length <= _nominal_segment_length * 3 / 4)
			{
				reverse_partial_segment(a, b);
			}
			else  // split at a and b and merge with their neighbors
			{
				// leave a and b in the original segment to make a complete segment for reversion
				split_and_merge(a, false, Direction::backward);
				split_and_merge(b, false, Direction::forward);
				reverse_complete_segment(a, b);
			}
		}
	}

	void TwoLevelTree::reverse_complete_segment(Node * a, Node * b)
	{
		assert(a->parent == b->parent);
		auto parent = a->parent;
		bool reverse = parent->reverse;
		assert((parent->reverse && a == parent->segment_end_node && b == parent->segment_begin_node)
			|| (!parent->reverse && b == parent->segment_end_node && a == parent->segment_begin_node));
		// the following line cannot work directly in the flip method when (b, c) is a single segment.
		//auto prev_a = get_prev(a), next_b = get_next(b);
		auto prev_a = a->parent->prev->forward_end_node();
		auto next_b = b->parent->next->forward_begin_node();
		parent->reverse = !parent->reverse;
		// repair the 4 connections to the neighbor segments
		// pre_a now should go to b
		if (prev_a->parent->reverse)
			prev_a->prev = b;
		else
			prev_a->next = b;
		// a should now go to next_b
		if (parent->reverse)
			a->prev = next_b;
		else
			a->next = next_b;
		// next_b should go back to a
		if (next_b->parent->reverse)
			next_b->next = a;
		else
			next_b->prev = a;
		// b should go back to pre_a
		if (parent->reverse)
			b->next = prev_a;
		else
			b->prev = prev_a;
	}

	void TwoLevelTree::reverse_partial_segment(Node * a, Node * b)
	{
		assert(a->parent == b->parent);
		auto parent = a->parent;
		// we need change the connections and the IDs, and possibly the segment endpoints
		auto prev_a = get_prev(a), next_b = get_next(b);
		auto partial_segment_length = std::abs(a->id - b->id) + 1;
		// first store a and the internal nodes between a and b
		_temp_nodes.clear();
		_temp_nodes.reserve(partial_segment_length + 1);
		_temp_nodes.push_back(next_b);
		_temp_nodes.push_back(a);
		auto p = get_next(a);
		while (p != b)
		{
			_temp_nodes.push_back(p);
			p = get_next(p);
		}
		_temp_nodes.push_back(b);
		
		// now we reconstruct the connections from prev_a -> b .. -> a -> next_b along the forward direction
		p = prev_a;
		while (!_temp_nodes.empty())
		{
			auto q = _temp_nodes.back();
			_temp_nodes.pop_back();
			// connection p with q, p -> q on forward tour
			connect_arc_forward(p, q);
			p = q;
		}
		
		// if one of them is originally an endpoint (at most one can be)
		if (a == parent->segment_begin_node)
			parent->segment_begin_node = b;
		else if (a == parent->segment_end_node)
			parent->segment_end_node = b;
		else if (b == parent->segment_begin_node)
			parent->segment_begin_node = a;
		else if (b == parent->segment_end_node)
			parent->segment_end_node = a;

		// relabel the IDs for the forward path b --> a. Note ID is numbered according to node.next.
		if (parent->reverse)  // a --next-- --next-- b
		{
			int a_id = 0;
			if (a == parent->segment_begin_node)
				a_id = b->next->id - partial_segment_length;
			else
				a_id = a->prev->id + 1;
			relabel_id(a, b, a_id);
		}
		else  // b --next-- --next-- a
		{
			int b_id = 0;
			if (b == parent->segment_begin_node)
				b_id = a->next->id - partial_segment_length;
			else
				b_id = b->prev->id + 1;
			relabel_id(b, a, b_id);
		}
	}

	void TwoLevelTree::connect_arc_forward(Node * p, Node * q)
	{
		if (p->parent->reverse)
			p->prev = q;
		else
			p->next = q;

		if (q->parent->reverse)
			q->next = p;
		else
			q->prev = p;
	}

	void TwoLevelTree::relabel_id(Node * a, Node * b, int a_id)
	{
		assert(a->parent == b->parent);
		a->id = a_id;
		while (a != b)
		{
			a->next->id = a->id + 1;
			a = a->next;
		}
	}

	bool TwoLevelTree::is_city_valid(int city) const
	{
		return city >= _origin_city && city < _origin_city + _n_cities;
	}

	void TwoLevelTree::delete_arc(Node * a, Node * b)
	{
		assert(get_next(a) == b);
		// unless a -> b is exactly between two segments, we have to split and merge to make them so
		if (a->parent == b->parent)
		{
			auto parent = a->parent;
			// split at a - b
			int b_half_length = std::abs(parent->forward_end_node()->id - b->id);
			if (b_half_length < parent->size / 2)
			{
				// move the half containing b
				split_and_merge(b, true, Direction::forward);
			}
			else
			{
				// move the half containing a
				split_and_merge(a, true, Direction::backward);
			}
		}
		// now it should be like [......a] -> [b.....] (forward)
		assert(a->parent->next == b->parent);
		assert(a == a->parent->forward_end_node());
		assert(b == b->parent->forward_begin_node());

		// remove the links between a and b
		if (a->next == b)
			a->next = nullptr;
		else
			a->prev = nullptr;
		
		if (b->next == a)
			b->next = nullptr;
		else
			b->prev = nullptr;
	}

	void TwoLevelTree::flip(Node * a, Node * b, Node * c, Node * d)
	{
		bool is_forward = get_next(a) == b;
		assert((get_next(c) == d) == is_forward);
		assert(!((a == c) && (b == d)));
		if (b == c || d == a)  // in this case, even after flip, still the same
			return;

		// reverse the old subpath (b, c) or (d, a)
		// and reconnection of (a, c)  and (b, d) is also performed automatically
		// thus, no need to delete the old arcs explicitly
		// we tend to reverse the shorter path for possibly reduced computation cost
		if (is_approximately_shorter(b, c, d, a))
		{
			if (is_forward)
				reverse(b, c);
			else
				reverse(c, b);
		}
		else
		{
			if (is_forward)
				reverse(d, a);
			else
				reverse(a, d);
		}
	}

	void TwoLevelTree::flip(int a, int b, int c, int d)
	{
		flip(get_node(a), get_node(b), get_node(c), get_node(d));
	}

	void TwoLevelTree::double_bridge_move(Node * a, Node * b, Node * c, Node * d)
	{
		assert(is_between(a, b, c));
		assert(is_between(b, c, d));
		assert(is_between(c, d, a));
		assert(is_between(d, a, b));
		assert(a->parent != b->parent);
		assert(a->parent != c->parent);
		assert(a->parent != d->parent);
		assert(b->parent != c->parent);
		assert(b->parent != d->parent);
		assert(c->parent != d->parent);
		auto an = get_next(a), bn = get_next(b), cn = get_next(c), dn = get_next(d);
		// (1) split and merge to make all the above segment boundaries
		auto sam = [this](Node* p) {  
			// special case: p and pn are already boundary nodes, i.e., [.....p] -> [pn....]
			if (p->parent == get_next(p)->parent)
			{
				split_and_merge(p, false, Direction::forward);
			}
		};

		for (auto p : { a, b, c, d })
		{
			sam(p);
			
#ifndef NDEBUG
			assert((p == p->parent->segment_begin_node || p == p->parent->segment_end_node));
			auto q = get_next(p);
			assert((q == q->parent->segment_begin_node || q == q->parent->segment_end_node));
			assert(p->parent->next == q->parent);
#endif
		}
		
		// (2) reconnect. Note that p and q are both segment boundary nodes.
		auto connect_forward = [this](Node* p, Node* q) {
			connect_arc_forward(p, q);
			p->parent->next = q->parent;
			q->parent->prev = p->parent;
		};
		// must be connected in the right order
		connect_forward(a, cn);
		connect_forward(d, bn);
		connect_forward(c, an);
		connect_forward(b, dn);
		
		
		// (3) each segment itself is not changed due to reconnection
		// However, the order of the segments is changed and re-id is needed
		auto p = head_parent_node();
		int id = 0;
		do
		{
			p->id = id;
			id++;
			p = p->next;
		} while (p != head_parent_node());
	}

	void TwoLevelTree::double_bridge_move(int a, int b, int c, int d)
	{
		double_bridge_move(get_node(a), get_node(b), get_node(c), get_node(d));
	}

	void TwoLevelTree::split_and_merge(Node * s, bool include_self, Direction direction)
	{
		auto parent = s->parent;
		ParentNode* neighbor_parent = direction == Direction::forward ? parent->next : parent->prev;
		// get the nodes that need to be merged to the neighbor
		_temp_nodes.clear();
		if (include_self)
			_temp_nodes.push_back(s);
		Node* boundary = nullptr;  // the new boundary of the parent segment after being splitted
		if (direction == Direction::forward)
		{
			auto p = get_next(s);
			while (p->parent == s->parent)
			{
				_temp_nodes.push_back(p);
				p = get_next(p);
			}
			boundary = include_self ? get_prev(s) : s;
		}
		else
		{
			auto p = get_prev(s);
			while (p->parent == s->parent)
			{
				_temp_nodes.push_back(p);
				p = get_prev(p);
			}
			boundary = include_self ? get_next(s) : s;
		}

		if (_temp_nodes.empty())  // no split and merge is needed 
			return;

		Node* q = nullptr; // the node of neighbor segment to be connected to _temp_vector
		neighbor_parent->size += (int)_temp_nodes.size();
		parent->size -= (int)_temp_nodes.size();
		assert(parent->size > 0); // we cannot leave an empty segment
		// we first merge these nodes to the neighbor
		if (direction == Direction::forward)
		{
			q = neighbor_parent->reverse ? neighbor_parent->segment_end_node : neighbor_parent->segment_begin_node;
			int delta_id = neighbor_parent->reverse ? 1 : -1;
			while (!_temp_nodes.empty())
			{
				auto p = _temp_nodes.back();
				_temp_nodes.pop_back();
				p->parent = neighbor_parent;
				connect_arc_forward(p, q);
				p->id = q->id + delta_id; // relabel the newly merged part in the neighbor segment
				q = p;
			}

			if (neighbor_parent->reverse)
				neighbor_parent->segment_end_node = q;
			else
				neighbor_parent->segment_begin_node = q;

			// repair the boundary of the old segment
			connect_arc_forward(boundary, q);
			if (parent->reverse)
				parent->segment_begin_node = boundary;
			else
				parent->segment_end_node = boundary;
		}
		else
		{
			q = neighbor_parent->reverse ? neighbor_parent->segment_begin_node : neighbor_parent->segment_end_node;
			int delta_id = neighbor_parent->reverse ? -1 : 1;
			while (!_temp_nodes.empty())
			{
				auto p = _temp_nodes.back();
				_temp_nodes.pop_back();
				p->parent = neighbor_parent;
				connect_arc_forward(q, p);
				p->id = q->id + delta_id; // relabel the newly merged part in the neighbor segment
				q = p;
			}
			if (neighbor_parent->reverse)
				neighbor_parent->segment_begin_node = q;
			else
				neighbor_parent->segment_end_node = q;
			// repair the boundary of the old segment
			connect_arc_forward(q, boundary);
			if (parent->reverse)
				parent->segment_end_node = boundary;
			else
				parent->segment_begin_node = boundary;
		}

	}
}
