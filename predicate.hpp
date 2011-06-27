#ifndef PREDICATE_H
#define PREDICATE_H

#include "codevalue.hpp"

struct IPredicate;
struct Predicate;
struct TerminalPredicate;
struct AndPredicate;
struct OrPredicate;
struct TernaryPredicate;

#include "generator.hpp"


struct IPredicate : public IGeneratable
{
	virtual bool push(IPredicate *p) = 0;
};


struct Predicate : public IPredicate
{
	Predicate() : predicate(0) {}
	virtual ~Predicate() { delete predicate; }
	
	virtual bool push(IPredicate *p);
	virtual void accept(IGenerator &generator);
	
	IPredicate *predicate;
};


struct TerminalPredicate : public IPredicate
{
	TerminalPredicate(bool should_succeed, codevalue tested_bits = 0, codevalue tested_value = 0)
		: should_succeed(should_succeed), tested_bits(tested_bits), tested_value(tested_value) {}
	
	virtual bool push(IPredicate *p) { return false; }
	
	virtual void accept(IGenerator &generator);
	
	bool should_succeed;
	codevalue tested_bits, tested_value;
};



struct AndPredicate : public IPredicate
{
	AndPredicate(IPredicate *lhs) : complete(false), lhs(lhs), rhs(0) {}
	virtual ~AndPredicate() { delete lhs; delete rhs; }
	
	virtual bool push(IPredicate *p);
	virtual void accept(IGenerator &generator);
	
	bool complete;
	IPredicate *lhs, *rhs;
};


struct OrPredicate : public IPredicate
{
	OrPredicate(IPredicate *lhs) : complete(false), lhs(lhs), rhs(0) {}
	virtual ~OrPredicate() { delete lhs; delete rhs; }
	
	virtual bool push(IPredicate *p);
	virtual void accept(IGenerator &generator);
	
	bool complete;
	IPredicate *lhs, *rhs;
};


struct TernaryPredicate : public IPredicate
{
	TernaryPredicate(codevalue tested_bit)
		// since the ascii range is more common in usage, we want to prefer (i.e., avoid
		// the compare/jump in) the off branch. So we create a negative test, and switch
		// the on and off branch precedence in the push() method
		: predicate(new TerminalPredicate(true, tested_bit, 0)), complete(false), off(0), on(0) {}
	virtual ~TernaryPredicate() { delete predicate; delete on; delete off; }
	
	virtual bool push(IPredicate *p);
	virtual void accept(IGenerator &generator);
	
	bool complete;
	IPredicate *predicate, *on, *off;
};

#endif