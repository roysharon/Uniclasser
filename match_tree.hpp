#ifndef SEARCH_TREE_H
#define SEARCH_TREE_H

#include <vector>
#include <ostream>
#include <string>
#include "codevalue.hpp"
#include "predicate.hpp"

#define LASTBIT(t) ((t)1<<(CHAR_BIT*sizeof(t)-1))
#define TRIMMED(ptr) ((uintptr_t)(ptr)&MatchTree::Node::trimmed)

struct MatchTree
{
	

	//----- Nodes -------------------------------------------------------------
	
	struct Node
	{
		Node() : parent(0), on(0), off(0), pos(LASTBIT(codevalue)) {}
		Node(Node *parent, bool on) : parent(parent), on(0), off(0), pos((parent->pos>>1)&(~parent->pos)) {}
		
		~Node() {
			removeOn();
			removeOff();
		}
		
		void removeOn() { if (on != 0 && !TRIMMED(on)) delete on; on = 0; }
		void removeOff() { if (off != 0 && !TRIMMED(off)) delete off; off = 0; }
	
		int trimUp()
		{
			return (TRIMMED(on) && TRIMMED(off) && parent != 0) ? parent->trimDown(this) : 0;
		}
		
		int trimDown(Node *child)
		{
			int n = 0;
			if (on == child) { delete on; on = (Node*)trimmed; --n; }
			if (off == child) { delete off; off = (Node*)trimmed; --n; }
			return n + trimUp();
		}
		
		int add(codevalue x)
		{
			int n = 0;
			Node *&node = (x & pos) ? on : off;
			if (TRIMMED(node)) return n;
			
			if (node == 0 && pos != 1)
			{
				node = new Node(this, x & pos);
				++n;
			}
			
			if (pos == 1) node = (Node*)trimmed;
			else n += node->add(x);
			
			n += trimUp();
			return n;
		}
		
		int removeUp()
		{
			return (on == 0 && off == 0 && parent != 0) ? parent->removeDown(this) : 0;
		}
		
		int removeDown(Node *child)
		{
			int n = 0;
			if (on == child) { removeOn(); --n; }
			if (off == child) { removeOff(); --n; }
			return n + removeUp();
		}
		
		codevalue pos;
		Node *on;
		Node *off;
		Node *parent;
					 
		static const uintptr_t trimmed = LASTBIT(uintptr_t);
	};
	
	
	//----- MatchTree --------------------------------------------------------

	MatchTree(codevalue_vector &list) : count(0)
	{
		codevalue last = -1;
		for (codevalue_vector::iterator i = list.begin(), e = list.end(); i != e; ++i)
		{
			if (*i != last) // ignore duplicates
				count += root.add(last = *i);
		}
	}
	
	
	int create_predicate(IPredicate &predicate);
	int create_predicate(IPredicate &predicate, Node* bottom, codevalue mask, codevalue val);
	int prune_base(IPredicate &predicate, Node* &bottom, codevalue &mask, codevalue &val);
	int prune_top(IPredicate &predicate, Node* &bottom, codevalue mask, codevalue val);
	Node* check_branch(MatchTree::Node* bottom, codevalue &mask, codevalue &val);
	void remove_trimmed_child(Node* node);

	Node root;
	int count;
};


#endif