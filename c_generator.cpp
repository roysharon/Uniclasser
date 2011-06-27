#include <iostream>
#include "c_generator.hpp"

using namespace std;


//----- visit() methods -------------------------------------------------------

bool c_profile;

void CGenerator::visit(IPredicate &predicate)
{
	predicate.accept(*this); // cause predicate to call visit() again with the proper class as argument
}

void CGenerator::visit(TerminalPredicate &predicate)
{
	bool eq = predicate.should_succeed;
	codevalue m = predicate.tested_bits, v = predicate.tested_value & m;
	
	if (m == 0)
	{
		out << eq;
	}
	else if (v == 0)
	{
		if (eq) out << '!';
		if (m == -1) out << "(c";
		else out << "(c&" << m;
		out << ')';
	}
	else
	{
		if (m == -1) out << "(c";
		else out << "((c&" << m << ')';
		out << (eq ? "==" : "!=") << v << ')';
	}
}

#define PROFILE(p, x) if (c_profile) out << #p << '('; (x); if (c_profile) out << ')';

void CGenerator::visit(AndPredicate &predicate)
{
	PROFILE(JA, predicate.lhs->accept(*this))
	out << "&&" << endl << prefix;
	PROFILE(JR, predicate.rhs->accept(*this))
}

void CGenerator::visit(OrPredicate &predicate)
{
	prefix += '\t';
	out << "(	";
	PROFILE(JO, predicate.lhs->accept(*this))
	out << endl << prefix.substr(0, prefix.length()-1) << "||\t";
	PROFILE(JR, predicate.rhs->accept(*this))
	prefix.erase(prefix.length()-1);
	out << endl << prefix << ')';
}

void CGenerator::visit(TernaryPredicate &predicate)
{
	out << '(';
	PROFILE(JA, predicate.predicate->accept(*this))
	out << endl << prefix << "?\t";
	prefix += '\t';
	predicate.on->accept(*this);
	out << endl << prefix.substr(0, prefix.length()-1) << ":\t";
	predicate.off->accept(*this);
	prefix.erase(prefix.length()-1);
	out << endl << prefix << ')';
}


//----- generate() methods ----------------------------------------------------

void CGenerator::generate_main(bool test, bool profiler)
{
	out << "#include <stdio.h>" << endl
		<< "#include <string.h>" << endl
		<< "#include \"uniclasser.h\"" << endl;
	if (test)
		out << "#include \"test_uniclasser.h\"" << endl;
	out	<< endl
		<< "int main (int argc, char * const argv[])" << endl
		<< '{'
		<< "	" << endl
		<< "	if (argc > 1) for (int i = 1; i < argc; ++i)" << endl
		<< "	{" << endl
		<< "		wchar_t c;" << endl
		<< "		if (mbtowc(&c, argv[i], strlen(argv[i])) > 0)" << endl
		<< "		{" << endl
		<< "			printf(\"Checking c='%c' (%#x)\\n\", *argv[i], c);" << endl;
	
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "			printf(\"" << *i << "(c)==%s\\n\", " << *i << "(c) ? \"true\" : \"false\");" << endl;
	
	out	<< "		}" << endl
		<< "		printf(\"\\n\");" << endl 
		<< "	}" << endl;
	
	if (test) 
		out << "	else" << endl
		<< "	{" << endl
		<< "		test();" << endl
		<< "		printf(\"\\nYou can also enter some characters to check (separated by spaces).\\n\");" << endl
		<< "	}" << endl;
	else
		out << "	else printf(\"Please enter some characters to check (separated by spaces).\\n\");" << endl;
	
	out	<< '}' << endl << endl;
}

#define QUOTEMACRO_(x) #x
#define QUOTEMACRO(x) QUOTEMACRO_(x)
#define QCODEVALUE QUOTEMACRO(CODEVALUE)

void CGenerator::generate_classer(string classer_name, IPredicate &predicate, bool profiler)
{
	out << "#include \"uniclasser.h\"" << endl << endl << showbase << boolalpha;
	
	if (profiler)
		out << "#define JA(x) inct() && (x) && dect()" << endl
			<< "#define JO(x) incf() || (x) || decf()" << endl
			<< "#define JR(x) (inct() && (x))" << endl
			<< endl;
	
	out << "int " << classer_name << '(' << QCODEVALUE << " c)" << endl
	<< '{' << endl;
	if (profiler) out << "	profiler_reset();" << endl;
	out	<< "	return" << endl
		<< "		" << hex;
	
	prefix = "\t\t";
	predicate.accept(*this);
	out << endl 
		<< "	;" << endl
		<< '}' << endl
		<< endl;
}

void CGenerator::generate_test(string classer_name, codevalue_vector &codes, bool profiler)
{
	out << "#include <stdio.h>" << endl
		<< "#include \"uniclasser.h\"" << endl << endl << showbase << boolalpha
		<< "#define AVG(s,n) (n==0? 0 : ((float)(s)/(n)))" << endl << endl 
		<< "void test_" << classer_name << "()" << endl
		<< '{' << endl
		<< "	" << QCODEVALUE << " codes[] = {" << hex;
	
	int h = 999;
	string d;
	for (codevalue_vector::iterator i = codes.begin(), s = i, e = codes.end(); i != e; ++i)
	{
		if (h >= 72)
		{
			out << d << endl << "\t\t" << *i;
			h = 8;
		}
		else
		{
			out << d << *i;
			h += 8;
		}
		d = ",";
	}
	
	unsigned max_codevalue = min((unsigned)0x10FFFF, (unsigned)-1 >> CHAR_BIT*(sizeof(unsigned)-sizeof(codevalue)));
		out << endl
		<< "	};" << endl
		<< endl << dec
		<< "	" << QCODEVALUE << " c = 0;" << endl
		<< "	unsigned failed = 0, i, j, n = sizeof(codes)/sizeof(" << QCODEVALUE << ");" << endl;
	if (profiler) out << "	unsigned match_jumps = 0, unmatched_jumps = 0, ascii_jumps = 0, max_jumps = 0;" << endl;
	out	<< "	printf(\"\\nTesting " << classer_name << " (matching " << codes.size() << "):\\n\");" << endl
		<< "	for (i = 0, j = 0; i <= " << hex << max_codevalue << "; ++i, ++c)" << endl 
		<< "	{" << endl 
		<< "#ifdef TEST_ONLY_MATCHES" << endl
		<< "		if (j < n) c = codes[j]; else break;" << endl
		<< "#endif" << endl
		<< "		int b = j < n && c == codes[j];" << endl 
		<< "		if (b) ++j;" << endl;
	out	<< "		if (" << classer_name << "(c) != b)" << endl 
		<< "		{" << endl 
		<< "			printf(\"Failed test: U+%04x should %smatch\\n\", c, b?\"\":\"not \");" << endl
		<< "			++failed;" << endl 
		<< "		}" << endl;
	if (profiler)
		out << "		if (b) match_jumps += jumps(); else unmatched_jumps += jumps();" << endl
			<< "		if (c < 128) ascii_jumps += jumps();" << endl
			<< "		if (jumps() > max_jumps) max_jumps = jumps();" << endl;
	out	<< "	}" << endl
		<< "	if (failed == 0) printf(\"All %d tests passed!\\n\", i);" << endl 
		<< "	else printf(\"Failed %d out of %d tests!\\n\", failed, i);" << endl;
	if (profiler)
		out << "	printf(\"Jumps per codevalue: total=%.1f, matched=%.1f, unmatched=%.1f, ascii=%.1f, max=%d\\n\", AVG(match_jumps+unmatched_jumps,i), AVG(match_jumps,n), AVG(unmatched_jumps,i-n), AVG(ascii_jumps,128), max_jumps);" << endl;
	out	<< '}' << endl
		<< endl;
}

void CGenerator::generate_header(bool profiler)
{
	out << "#ifndef UNICLASSER_H" << endl 
		<< "#define UNICLASSER_H" << endl 
		<< endl 
		<< "#include <stdlib.h>" << endl 
		<< endl;
	
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "int " << *i << '(' << QCODEVALUE << " c);" << endl;
	out << endl;
	
	if (profiler) 
		out << "// Profiler methods:" << endl 
			<< "void profiler_reset();" << endl
			<< "int inct();" << endl
			<< "int incf();" << endl
			<< "int dect();" << endl
			<< "int decf();" << endl
			<< "int jumps();" << endl << endl;
	
	out << "#endif";
}

#undef QCODEVALUE
#undef QUOTEMACRO
#undef QUOTEMACRO_

void CGenerator::generate_test_header()
{
	out << "#ifndef TEST_UNICLASSER_H" << endl 
		<< "#define TEST_UNICLASSER_H" << endl 
		<< endl;
	
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "void test_" << *i << "();" << endl;
	out << endl;
	
	out << "void test()" << endl 
		<< '{' << endl;
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "	test_" << *i << "();" << endl;
	out << '}' << endl << endl;
	
	out << "#endif";
}

void CGenerator::generate_profiler()
{
	out << "#include \"uniclasser.h\"" << endl << endl 
	<< "int j;" << endl << endl 
	<< "void profiler_reset() { j = 0; }" << endl
	<< "int inct() { ++j; return 1; }" << endl
	<< "int incf() { ++j; return 0; }" << endl
	<< "int dect() { --j; return 1; }" << endl
	<< "int decf() { --j; return 0; }" << endl
	<< "int jumps() { return j; }" << endl;
}

#pragma GCC diagnostic ignored "-Wwrite-strings"  // remove "Deprecated conversion from string constant to 'char*'"

void CGenerator::out_open(string filename, char * const what)
{
	if (what != 0) cout << "Writing " << what << " to " << output_dir << filename << endl;
	out.open((output_dir + filename).c_str(), ios_base::out);
	
	out << "// Autogenerated by the uniclasser generator. See " << URL << endl 
		<< "// Permission is hereby granted to include, modify, republish and resell this code for any purpose." << endl << endl;
}

void CGenerator::out_close()
{
	out.close();
}

void CGenerator::generate(string classer_name, Predicate &p, codevalue_vector *test_codes, bool profiler)
{
	classers.push_back(classer_name);
	c_profile = profiler;
	
	out_open(classer_name + ".c", "classifier");
	generate_classer(classer_name, p, profiler);
	out_close();
	
	if (test_codes != 0)
	{
		out_open("test_" + classer_name + ".c", "classifier test");
		generate_test(classer_name, *test_codes, profiler);
		out_close();
	}
}

void CGenerator::finalize(bool test, bool profiler)
{
	out_open("uniclasser.h", "a combined header file");
	generate_header(profiler);
	out_close();
	
	if (test)
	{
		out_open("test_uniclasser.h", "a combined test header file");
		generate_test_header();
		out_close();
	}
	
	if (profiler)
	{
		out_open("uniclasser_profiler.c", "profiler");
		generate_profiler();
		out_close();
	}
	
	out_open("main.c", "main");
	generate_main(test, profiler);
	out_close();
}
