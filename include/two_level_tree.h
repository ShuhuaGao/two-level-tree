#pragma once
#include <vector>
#include "node.h"

/**
 * References for the two-level tree structure.
 * [1] Fredman, Michael L., David S. Johnson, Lyle A. McGeoch, and Gretchen Ostheimer.
 *	"Data structures for traveling salesmen." Journal of Algorithms 18, no. 3 (1995): 432-479.
 * [2] Helsgaun, Keld. "An effective implementation of the Lin–Kernighan traveling salesman heuristic.
 *	" European Journal of Operational Research 126, no. 1 (2000): 106-130.
 * [3] Osterman, Colin, and César Rego.
 *	"A k-level data structure for large-scale traveling salesman problems."
 *	Annals of Operations Research 244, no. 2 (2016): 583-601.
 */



namespace tsp
{
	enum class Direction
	{
		forward,
		backward
	};

#define CONST_THIS static_cast<const TwoLevelTree*>(this)
	/**
	 * A two-level tree structure as an efficient tour representation.
	 *
	 * Definition:
	 *	- A forward tour is defined by calling a.next on a segment node `a` if the segment is not reversed, 
	 *		otherwise calling a.prev. For the parent nodes, to get a forward toure, always call .next.
	 *	- A forward path is obtained by calling \ref get_next;
	 *	- A backward path is obtained by calling \ref get_prev.
	 *  - Supposing the beginning node of a segment is b and the ending node is e, then by 
	 *		calling b.next, we can traverse the segment from b to e.
	 *
	 * Invariants:
	 *	- ID (sequence number) for segment nodes
	 *		-# If node a and a.next reside in the same segment, then a.next.id = a.id + 1.
	 *		-# If two nodes a and b reside in different segments, there is no relation between their IDs.
	 *		-# The begin node of a segment has the minimum ID in this segment. Similarly, the end node has 
	 *			the maximum ID. All IDs in a segment are unique and contiguous.
	 *	- ID (sequence number) for parent nodes
	 *		-# If a parent node p is NOT the tail (\ref tail_parent_node()), then p.next.id = p.id + 1.
	 *		-# If a parent node p is the tail (\ref tail_parent_node()), then p.next.id is in fact the id
	 *			of the head parent node (\ref head_parent_node()) and we have p.next.id < p.id.
	 */
	class TwoLevelTree
	{
	private:
		// Since the double linked list contains no node creation/deletion operations, we can store
		// them in an array for fast access. However, they will behave just like a doubly-linked list.
		std::vector<ParentNode> _parent_nodes;
		std::vector<Node> _nodes;
		int _n_cities = 0;
		int _origin_city = -1;
		int _nominal_segment_length = 0;

		std::vector<Node*> _temp_nodes;
		std::vector<ParentNode*> _temp_parent_nodes;
	public:
		/**
		 * An empty two level tree, which is meaningless, but may be used as a return value to 
		 * indicate that a two level tree cannot be successfully built.
		 */
		TwoLevelTree(){}

		/**
		 * Build a two-level tree for n cities. The \p origin_city is the first number of the cities.
		 * Cities are numbered consecutively, i.e., \p origin_city, \p origin_city + 1, ...
		 * Note that the traversal order of the cities in the tour should be later specified by 
		 * the \ref set_order method.
		 */
		explicit TwoLevelTree(int n_cities, int origin_city = 0);

		TwoLevelTree(const TwoLevelTree& other);

		TwoLevelTree(TwoLevelTree&& other) noexcept = default;

		TwoLevelTree& operator= (const TwoLevelTree& other);
		TwoLevelTree& operator= (TwoLevelTree&& other) noexcept = default;
		/**
		 * Set a forward tour in specific order to be represented by this two-level tree.
		 */
		void set_raw_tour(const std::vector<int>& order);

		/**
		 * Get the node bound to the city.
		 */
		const Node* get_node(int city) const
		{
			assert(city >= _origin_city && city < _origin_city + _n_cities);
			return &_nodes[city];
		}

		Node* get_node(int city)
		{
			return const_cast<Node*>(CONST_THIS->get_node(city));
		}

		/**
		 * Get the parent node of the city's node.
		 */
		const ParentNode* get_parent_node(int city) const
		{
			return get_node(city)->parent;
		}
		
		ParentNode* get_parent_node(int city)
		{
			return const_cast<ParentNode*>(CONST_THIS->get_parent_node(city));
		}

		/**
		 * Get a parent node which can be used to start traversal. The previous one of this head 
		 * node is the tail parent node.
		 * @seealso tail_parent_node()
		 */
		ParentNode* head_parent_node()
		{
			return &_parent_nodes.front();
		}

		const ParentNode* head_parent_node() const
		{
			return &_parent_nodes.front();
		}

		/**
		 * Get a parent node which can be used to start traversal. The next of this tail node is the 
		 * head parent node.
		 * @seealso head_parent_node()
		 */
		ParentNode* tail_parent_node()
		{
			return &_parent_nodes.back();
		}

		/**
		 * Get the node for the origin city.
		 */
		Node* origin_city_node()
		{
			return &_nodes[_origin_city];
		}

		int n_segments() const
		{
			return static_cast<int>(_parent_nodes.size());
		}

		int n_cities() const
		{
			return _n_cities;
		}

		int origin_city() const
		{
			return _origin_city;
		}

		/**
		 * Get the next node for \p current in the forward tour.
		 */
		const Node* get_next(const Node* current) const
		{
			if (current->parent->reverse)
				return current->prev;
			return current->next;
		}

		Node* get_next(const Node* current)
		{
			return const_cast<Node*>(CONST_THIS->get_next(current));
		}


		/**
		 * Get the next city of \p current city in the forward tour.
		 */
		int get_next(int current) const
		{
			return get_next(get_node(current))->city;
		}

		/**
		 * Get the previous node for \p current in the forward tour.
		 */
		const Node* get_prev(const Node* current) const
		{
			if (current->parent->reverse)
				return current->next;
			return current->prev;
		}

		Node* get_prev(const Node* current)
		{
			return const_cast<Node*>(CONST_THIS->get_prev(current));
		}

		/**
		 * Get the previous city of \p current city in the forward tour.
		 */
		int get_prev(int current) const
		{
			return get_prev(get_node(current))->city;
		}

		/**
		 * Whether the node \p b lies between \p a and \p c in a forward traversal.
		 * The query returns true if and only if city \p b is reached before city \p c if we start a
		 * forward traversal from \p a.
		 */
		bool is_between(const Node* a, const Node* b, const Node* c) const;

		/**
		 * Similar to is_between(const Node* a, const Node* b, const Node* c). 
		 */
		bool is_between(int a, int b, int c) const;

		/**
		 * Reverse the forward path between \p a and \p b.
		 */
		void reverse(Node* a, Node* b);

		/**
		 * Remove two arcs (a, b) and (c, d), and add two others (a, c) and (b, d).
		 * The two arcs should both be in forward or backward orientation.
		 * @note Either (a, d) or (b, c) is reversed. The smaller one is preferred.
		 */
		void flip(Node* a, Node* b, Node* c, Node* d);

		void flip(int a, int b, int c, int d);

		/**
		 * Perform a double-bridge move. Suppose the next node in a forward tour of the four arguments 
		 * are an, bn, cn and dn respectively. Then, (a, an), (b, bn), (c, cn) and (d, dn) are removed.
		 * New arcs (a, cn), (b, dn), (c, an), and (d, bn) are inserted.
		 * @note 
		 * (1) The arguments a, b, c, d should be given in a forward tour order, and there should 
		 * be at least one other node between each pairs of them.
		 * (2) Any two of a, b, c, d should lie in diffrent segments.
		 */
		void double_bridge_move(Node* a, Node* b, Node* c, Node* d);

		void double_bridge_move(int a, int b, int c, int d);

		/**
		 * Split a segment at \p s,  and merge one half to its neighbor segment specified by the 
		 *	\p direction. if \p include_self is true, then the node \p s is merged to its neighbor; 
		 * otherwise, it stays in its own segment.
		 */
		void split_and_merge(Node* s, bool include_self, Direction direction);

		
		/**
		 * Get the tour encoded by this two-level tree. If a negative number is given for the 
		 * \p start_city (default -1), then the tour starts at the origin city. 
		 */
		std::vector<int> get_raw_tour(int start_city = -1, Direction direction = Direction::forward) const;

		/**
		 * Output the raw tour to a given vector \param v.
		 * @note The original contents in \param v, if any, will be cleaned.
		 */
		void to_raw_tour(std::vector<int>& v, int start_city = -1, Direction direction = Direction::forward) const;

		/**
		 * Get the lengths of each segment. Note that the result may change after tree operations.
		 * If a valid \p start_city is given, then the first segment in the returned result is the segment
		 * that contains this \p start_city. Otherwise, segment sizes are given in a random order.
		 */
		std::vector<int> actual_segment_sizes(int start_city = -1) const;

		/**
		 * Whether the length of the first forward path a --> b is approximately shorter than the one of 
		 * the second forward path c --> d.
		 * @note If one path has more segments than the other, we consider the other path as the shorter one
		 * though its actual number of nodes may even be larger due to the imbalanced segments.
		 */
		bool is_approximately_shorter(Node* a, Node* b, Node* c, Node* d) const;

		/**
		 * Count how many segments are involved in the forward path a --> b. 
		 * (Incomplete segments are also counted.)
		 */
		int count_n_segments(Node* a, Node* b) const;

		/**
		 * Whether the edge (city1, city2) exists. The direction doesn't matter here.
		 */
		bool has_edge(int city1, int city2) const;

		/**
		 * Is (a, b) an edge in current tour? The direction doesn't matter here.
		 */
		bool has_edge(const Node* a, const Node* b) const;

		/**
		 * Given an edge's two endpoints (cities), return them in forward order.
		 */
		std::pair<int, int> turn_forward(int city1, int city2) const;
	private:
		// whether the forward path from a to b is contained in a single segment. O(1).
		bool is_path_in_single_segment(const Node* a, const Node* b) const;

		// count the number of nodes in the forward path from a to b (including both)
		int forward_distance(const Node* a, const Node* b) const;

		// reverse a single segment, either completely or partially
		void reverse_segment(Node* a, Node*b);

		// reverse a complete single segment, which is the forward path from a to b
		void reverse_complete_segment(Node* a, Node* b);

		// reverse a part of a segment, which is the forward path from a to b
		void reverse_partial_segment(Node* a, Node* b);

		// connect nodes a and b to form an arc such that a is before b on the forward tour
		void connect_arc_forward(Node* a, Node* b);

		// relabel the IDs from a to b by calling .next
		// ID of a should be relabelled to \p a_id
		void relabel_id(Node* a, Node* b, int a_id);
		
		bool is_city_valid(int city) const;

		// delete a forward arc (a -> b).
		void delete_arc(Node* a, Node* b);
	};


	static_assert(std::is_nothrow_move_constructible<TwoLevelTree>::value, "TwoLevelTree");
#undef CONST_THIS	
}