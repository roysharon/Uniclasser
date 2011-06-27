#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>

struct IGenerator;

struct IGeneratable
{
	virtual void accept(IGenerator &generator) = 0;
};

#include "predicate.hpp"

struct IGenerator
{
	virtual void visit(IPredicate &predicate) = 0;
	virtual void visit(TerminalPredicate &predicate) = 0;
	virtual void visit(AndPredicate &predicate) = 0;
	virtual void visit(OrPredicate &predicate) = 0;
	virtual void visit(TernaryPredicate &predicate) = 0;
	
	virtual void generate(std::string classer_name, Predicate &p, codevalue_vector *test_codes, bool profiler) = 0;
	virtual void finalize(bool test, bool profiler) = 0;
};

#endif