#pragma once

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
#include <list>

namespace tsp
{
	class Node;

	/**
	 * Representing a parent node in the two-level tree structure. 
	 */
	class ParentNode
	{
	public:
		bool reverse = false;
		int size = 0;			// number of (segment) nodes under its control
		int id = 0;				// a sequence number in the list where it resides

		ParentNode* prev = nullptr;
		ParentNode* next = nullptr;

		Node* segment_begin_node = nullptr;
		Node* segment_end_node = nullptr;

		/**
		 * Get the last node in this segment in a forward traversal.
		 */
		Node* forward_end_node()
		{
			return reverse ?  segment_begin_node : segment_end_node;
		}

		/**
		 * Get the first node in this segment in a forward traversal.
		 */
		Node* forward_begin_node()
		{
			return reverse ? segment_end_node : segment_begin_node;
		}

		/**
		 * Get the first node in this segment in a backward traversal.
		 */
		Node* backward_begin_node()
		{
			return reverse ? segment_begin_node : segment_end_node;
		}

		/**
		 * Get the last node in this segment in a backward traversal.
		 */
		Node* backward_end_node()
		{
			return reverse ? segment_end_node : segment_begin_node;
		}
	};


	/**
	 * A segment node in the two-level tree representation.
	 */
	class Node
	{
	public:
		int id = 0;				// a sequence number in the list where it resides
		int city = -1;			// the city bound to this node

		Node* prev = nullptr;
		Node* next = nullptr;
		ParentNode* parent = nullptr;
	};

}