// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#include <ostream>
#include <cassert>
#include "match_tree.hpp"

using namespace std;

int MatchTree::prune_base(IPredicate &predicate, Node* &bottom, codevalue &mask, codevalue &val)
{
	int height = 0;
	
	// how high does the bottom grow as a single branch?
	Node *node = bottom;
	bool top_trimmed = false;
	while ((node->on == 0 || node->off == 0) && node->on != node->off)
	{
		++height;
		mask |= node->pos;
		if (node->on != 0)
		{
			val |= node->pos;
			if (TRIMMED(node->on)) { top_trimmed = true; break; }
			node = node->on;
		}
		else
		{
			if (TRIMMED(node->off)) { top_trimmed = true; break; }
			node = node->off;
		}
	}

	// if bottom splits to two branches immediately, there's nothing to prune
	if (height == 0) return 0;
	
	// create a test of the joint base branch
	IPredicate *p = new TerminalPredicate(true, mask, val);

	if (top_trimmed)
	{
		// removing the trimmed top will also cause the removal of its pedgree
		// including bottom, so we need to indicate this to caller
		remove_trimmed_child(node);
		bottom = 0;
	}
	else
	{
		p = new AndPredicate(p);
	
		// pruning the base should move the bottom upward in the tree,
		// to the node with the two children
		bottom = node;
		mask |= bottom->pos;
	}
	
	// all this pruning cost us only one comparison/jump!
	assert(predicate.push(p));
	return 1;
}

MatchTree::Node* MatchTree::check_branch(MatchTree::Node* bottom, codevalue &mask, codevalue &val)
{
	// we are going to prune the top only if it will cause its parent to be
	// removed, which will happen only if this top is its parent's only child.
	if (bottom->on != 0 && bottom->off != 0) return 0;

	Node* node;
	if (bottom->on != 0)
	{
		node = bottom->on;
		val |= bottom->pos;
	}
	else node = bottom->off;
	
	// if this single child is already trimmed, we are done
	if (TRIMMED(node)) return bottom;
	
	// otherwise, continue checking in child
	mask |= mask >> 1;
	return check_branch(node, mask, val);
}

int MatchTree::prune_top(IPredicate &predicate, Node* &bottom, codevalue mask, codevalue val)
{
	// we got here after prune_base() was called, so we are guaranteed to have
	// two children at the bottom
	assert(bottom->on != 0 && bottom->off != 0);
	
	// when we trim a node that is the only child of its parent,
	// then removing it from the MatchTree leaves its parent childless,
	// which means we can remove the parent too. This may cause the parent's
	// parent to be removed as well, etc all the way down to the bottom.
	// When the bottom (which is guaranteed to have two children) becomes
	// a single-child-parent it is fit for base prunning, which lowers
	// succeeding compare/jumps for other nodes. This method tries to find
	// the lowest branch that fulfills these requirements.

	Node *lowest = 0;
	if (TRIMMED(bottom->on) || TRIMMED(bottom->off))
	{
		// the tree mechanics should have taken care of node with a couple of
		// trimmed children, so we should be here only with one!
		assert(!(TRIMMED(bottom->on) && TRIMMED(bottom->off)));
		
		// we found our match right here at bottom
		lowest = bottom;
		if (TRIMMED(bottom->on)) val |= bottom->pos;
	}
	else 
	{
		// find_trimmed_branch() checks whether a specific branch fullfills
		// the requirements, returning the trimmed node's parent if so.
		
		codevalue onMask = mask | mask >> 1, offMask = onMask, onVal = val | bottom->pos, offVal = val;
		Node *onBranch = check_branch(bottom->on, onMask, onVal);
		Node *offBranch = check_branch(bottom->off, offMask, offVal);
		if (onBranch != 0 || offBranch != 0)
		{
			bool prefer_on_branch = 
				onBranch != 0 && offBranch != 0 ? 
			
				// if both branches offer candidates, then choose the lowest
				(offBranch->pos / onBranch->pos == 0) :
			
				// otherwise, take the available candidate
				(onBranch != 0)
			;
			lowest = prefer_on_branch ? onBranch : offBranch;
			val = prefer_on_branch ? onVal : offVal;
			mask = prefer_on_branch ? onMask : offMask; 
		}
	}
	
	
	if (lowest != 0)
	{
		// we have a winner! we can prune and rip the benefits.
		IPredicate *p;
		Node *node = TRIMMED(lowest->on) ? lowest->off : lowest->on;
		if (!node == 0 && !TRIMMED(node) && (node->off == 0 && TRIMMED(node->on) || node->on == 0 && TRIMMED(node->off)))
		{
			// in the special case when lowest covers three out of four options
			// we can create a single compare/jump that would cover all of them
			bool onTrimmed = TRIMMED(lowest->on), offTrimmed = TRIMMED(lowest->off);
			val ^= lowest->pos;
			if (onTrimmed && TRIMMED(lowest->off->off)) val |= lowest->off->pos;
			else if (offTrimmed && TRIMMED(lowest->on->off)) val |= lowest->on->pos;
			p = new TerminalPredicate(false, mask | mask >> 1, val);
			
			// we can now remove the non-trimmed child, which is already included in the predicate.
			// But first check if this causes the removal of our bottom
			if (lowest == bottom) bottom = 0;
			if (onTrimmed) lowest->removeOff();
			else lowest->removeOn();
			--count;
		}
		else
		{
			// otherwise, we need to create an OR predicate
			p = new OrPredicate(new TerminalPredicate(true, mask, val));
		}
		
		// now kick off the removal process
		remove_trimmed_child(lowest);
		
		// and finally add the additional compare/jump to the predicate, and report it back
		assert(predicate.push(p));
		return 1;
	}
	else return 0;
}

void MatchTree::remove_trimmed_child(Node *node)
{
	// we should reach here with a single trimmed child
	assert(TRIMMED(node->on) ^ TRIMMED(node->off));
	
	if (TRIMMED(node->on)) node->on = 0;
	else if (TRIMMED(node->off)) node->off = 0;
	count += node->removeUp();
}

int MatchTree::create_predicate(IPredicate &predicate, Node* bottom, codevalue mask, codevalue val)
{
	int compare_jump = 0, n;
	do
	{
		// in all likelihood the base of the tree is single branched,
		// which we can prune in a single comparison/jump
		compare_jump += prune_base(predicate, bottom, mask, val);
		if (bottom == 0) break;
		
		// the top of the tree is probably crowded with ripe, full branches.
		// these branches will be marked as trimmed by MatchTree, so all we
		// need is to find the lowest (i.e., heaviest) one and prune it
		// in a single comparison/jump
		compare_jump += (n = prune_top(predicate, bottom, mask, val));
		if (bottom == 0) break;
	}
	// if we were able to prune at the top, then we now can prune the base
	// as well, which in turn may allow us to perform yet another prune at the top...
	while (n > 0);
	
	// if the bottom still has both the on and the off kids, then we have
	// no choice but to add a ternary test
	if (bottom != 0 && bottom->on != 0 && bottom->off != 0)
	{
		IPredicate *ternary = new TernaryPredicate(bottom->pos);
		++compare_jump;
		
		codevalue m = mask | bottom->on->pos;
		compare_jump += create_predicate(*ternary, bottom->on, m, val | bottom->pos);
		compare_jump += create_predicate(*ternary, bottom->off, m, val);
		
		assert(predicate.push(ternary));
	}
	
	return compare_jump;
}

int MatchTree::create_predicate(IPredicate &predicate)
{
	// first handle two special cases
	if (root.on == 0 && root.off == 0)
	{
		assert(predicate.push(new TerminalPredicate(false)));	// empty tree matches nothing
		return 0;
	}
	else if (TRIMMED(root.on) && TRIMMED(root.off))
	{	
		assert(predicate.push(new TerminalPredicate(true)));	// full tree matches everything
		return 0;
	}
	
	return create_predicate(predicate, &root, LASTBIT(codevalue), 0);
}
