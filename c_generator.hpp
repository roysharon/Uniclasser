// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#ifndef C_GENERATOR_H
#define C_GENERATOR_H

#include <string>
#include <fstream>
#include "generator.hpp"

struct CGenerator : public IGenerator
{
	CGenerator(std::string output_dir) : output_dir(output_dir) {}
	
	virtual void visit(IPredicate &predicate);
	virtual void visit(TerminalPredicate &predicate);
	virtual void visit(AndPredicate &predicate);
	virtual void visit(OrPredicate &predicate);
	virtual void visit(TernaryPredicate &predicate);
	
	virtual void generate(std::string classer_name, Predicate &p, codevalue_vector *test_codes = 0, bool profiler = false);
	void generate_main(bool test, bool profiler);
	void generate_classer(std::string classer_name, IPredicate &predicate, bool profiler);
	void generate_test(std::string classer_name, codevalue_vector &codes, bool profiler);
	void generate_header(bool profiler);
	void generate_test_header();
	void generate_profiler();
	virtual void finalize(bool test, bool profiler);
	
	void out_open(std::string filename, char * const what = 0);
	void out_close();
	
	std::string output_dir, prefix;
	std::vector<std::string> classers;
	std::ofstream out;
};

#endif