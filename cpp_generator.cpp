// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#include <iostream>
#include "cpp_generator.hpp"

using namespace std;


//----- visit() methods -------------------------------------------------------

bool cpp_profile;

void CppGenerator::visit(IPredicate &predicate)
{
	predicate.accept(*this); // cause predicate to call visit() again with the proper class as argument
}

void CppGenerator::visit(TerminalPredicate &predicate)
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

#define PROFILE(p, x) if (cpp_profile) out << #p << '('; (x); if (cpp_profile) out << ')';

void CppGenerator::visit(AndPredicate &predicate)
{
	PROFILE(JA, predicate.lhs->accept(*this))
	out << "&&" << endl << prefix;
	PROFILE(JR, predicate.rhs->accept(*this))
}

void CppGenerator::visit(OrPredicate &predicate)
{
	prefix += '\t';
	out << "(	";
	PROFILE(JO, predicate.lhs->accept(*this))
	out << endl << prefix.substr(0, prefix.length()-1) << "||\t";
	PROFILE(JR, predicate.rhs->accept(*this))
	prefix.erase(prefix.length()-1);
	out << endl << prefix << ')';
}

void CppGenerator::visit(TernaryPredicate &predicate)
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

void CppGenerator::generate_main(bool test, bool profiler)
{
	out << "#include <iostream>" << endl
		<< "#include \"uniclasser.hpp\"" << endl;
	if (test)
		out << "#include \"test_uniclasser.hpp\"" << endl;
	out	<< endl
		<< "int main (int argc, char * const argv[])" << endl
		<< '{'
		<< "	" << endl
		<< "	std::cout << std::boolalpha << std::hex << std::showbase;" << endl
		<< "	if (argc > 1) for (int i = 1; i < argc; ++i)" << endl
		<< "	{" << endl
		<< "		wchar_t c;" << endl
		<< "		if (mbtowc(&c, argv[i], strlen(argv[i])) > 0)" << endl
		<< "		{" << endl
		<< "			std::cout << \"Checking c='\" << *argv[i] << \"' (\" << c << ')' << std::endl;" << endl;
	
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "			std::cout << \"" << *i << "(c)==\" << " << *i << "(c) << std::endl;" << endl;
	
	out	<< "		}" << endl
		<< "		std::cout << std::endl;" << endl 
		<< "	}" << endl;

	if (test) 
		out << "	else" << endl
			<< "	{" << endl
			<< "		test();" << endl
			<< "		std::cout << std::endl << \"You can also enter some characters to check (separated by spaces).\" << std::endl;" << endl
			<< "	}" << endl;
	else
		out << "	else std::cout << \"Please enter some characters to check (separated by spaces).\" << std::endl;" << endl;
	
	out	<< '}' << endl << endl;
}

#define QUOTEMACRO_(x) #x
#define QUOTEMACRO(x) QUOTEMACRO_(x)
#define QCODEVALUE QUOTEMACRO(CODEVALUE)

void CppGenerator::generate_classer(string classer_name, IPredicate &predicate, bool profiler)
{
	out << "#include \"uniclasser.hpp\"" << endl << endl << showbase << boolalpha;
	
	if (profiler)
		out << "#define JA(x) Profiler::inct() && (x) && Profiler::dect()" << endl
			<< "#define JO(x) Profiler::incf() || (x) || Profiler::decf()" << endl
			<< "#define JR(x) (Profiler::inct() && (x))" << endl
			<< endl;

	out << "bool " << classer_name << '(' << QCODEVALUE << " c)" << endl
		<< '{' << endl;
	if (profiler) out << "	Profiler::reset();" << endl;
	out	<< "	return" << endl
		<< "		" << hex;
	
	prefix = "\t\t";
	predicate.accept(*this);
	out << endl 
		<< "	;" << endl
		<< '}' << endl
		<< endl;
}

void CppGenerator::generate_test(string classer_name, codevalue_vector &codes, bool profiler)
{
	out << "#include <iostream>" << endl
		<< "#include \"uniclasser.hpp\"" << endl << endl << showbase << boolalpha
		<< "#define AVG(s,n) (n==0? 0 : ((s)*10/(n)/10.0))" << endl << endl 
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
	out	<< "	std::cout << std::hex << std::noshowbase << std::endl << \"Testing " << classer_name << " (matching " << codes.size() << "):\" << std::endl;" << endl
		<< "	for (i = 0, j = 0; i <= " << hex << max_codevalue << "; ++i, ++c)" << endl 
		<< "	{" << endl 
		<< "#ifdef TEST_ONLY_MATCHES" << endl
		<< "		if (j < n) c = codes[j]; else break;" << endl
		<< "#endif" << endl
		<< "		bool b = j < n && c == codes[j];" << endl 
		<< "		if (b) ++j;" << endl;
	out	<< "		if (" << classer_name << "(c) != b)" << endl 
		<< "		{" << endl 
		<< "			std::cout << \"Failed test: U+\" << c << \" should \" << (b?\"\":\"not \") << \"match\" << std::endl;" << endl
		<< "			++failed;" << endl 
		<< "		}" << endl;
	if (profiler) out << "		if (b) match_jumps += Profiler::jumps(); else unmatched_jumps += Profiler::jumps();" << endl
					  << "		if (c < 128) ascii_jumps += Profiler::jumps();" << endl
					  << "		if (Profiler::jumps() > max_jumps) max_jumps = Profiler::jumps();" << endl;
	out	<< "	}" << endl
		<< "	if (failed == 0) std::cout << \"All \" << std::dec << i << \" tests passed!\" << std::endl;" << endl 
		<< "	else std::cout << \"Failed \" << std::dec << failed << \" out of \" << i << \" tests!\" << std::endl;" << endl;
	if (profiler) out << "	std::cout << \"Jumps per codevalue: total=\" << AVG(match_jumps+unmatched_jumps,i)" << endl
					  << "			  << \", matched=\" << AVG(match_jumps,n) << \", unmatched=\" << AVG(unmatched_jumps,i-n)" << endl
					  << "			  << \", ascii=\" << AVG(ascii_jumps,128) << \", max=\" << max_jumps << std::endl;" << endl;
	out	<< '}' << endl
		<< endl;
}

void CppGenerator::generate_header(bool profiler)
{
	out << "#ifndef UNICLASSER_H" << endl 
		<< "#define UNICLASSER_H" << endl 
		<< endl;
	
	for (vector<string>::const_iterator i = classers.begin(), e = classers.end(); i != e; ++i)
		out << "bool " << *i << '(' << QCODEVALUE << " c);" << endl;
	out << endl;
	
	if (profiler) 
		out << "struct Profiler" << endl 
			<< '{' << endl 
			<< "	static void reset();" << endl
			<< "	static bool inct();" << endl
			<< "	static bool incf();" << endl
			<< "	static bool dect();" << endl
			<< "	static bool decf();" << endl
			<< "	static int jumps();" << endl
			<< "};" << endl << endl;
	
	out << "#endif";
}

#undef QCODEVALUE
#undef QUOTEMACRO
#undef QUOTEMACRO_

void CppGenerator::generate_test_header()
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

void CppGenerator::generate_profiler()
{
	out << "#include \"uniclasser.hpp\"" << endl << endl 
		<< "int j;" << endl << endl 
		<< "void Profiler::reset() { j = 0; }" << endl
		<< "bool Profiler::inct() { ++j; return true; }" << endl
		<< "bool Profiler::incf() { ++j; return false; }" << endl
		<< "bool Profiler::dect() { --j; return true; }" << endl
		<< "bool Profiler::decf() { --j; return false; }" << endl
		<< "int Profiler::jumps() { return j; }" << endl;
}

#pragma GCC diagnostic ignored "-Wwrite-strings"  // remove "Deprecated conversion from string constant to 'char*'"

void CppGenerator::out_open(string filename, char * const what)
{
	if (what != 0) cout << "Writing " << what << " to " << output_dir << filename << endl;
	out.open((output_dir + filename).c_str(), ios_base::out);
	
	out << "// Autogenerated by the uniclasser generator. See " << URL << endl 
		<< "// Permission is hereby granted to include, modify, republish and resell this code for any purpose." << endl << endl;
}

void CppGenerator::out_close()
{
	out.close();
}

void CppGenerator::generate(string classer_name, Predicate &p, codevalue_vector *test_codes, bool profiler)
{
	classers.push_back(classer_name);
	cpp_profile = profiler;
	
	out_open(classer_name + ".cpp", "classifier");
	generate_classer(classer_name, p, profiler);
	out_close();
	
	if (test_codes != 0)
	{
		out_open("test_" + classer_name + ".cpp", "classifier test");
		generate_test(classer_name, *test_codes, profiler);
		out_close();
	}
}

void CppGenerator::finalize(bool test, bool profiler)
{
	out_open("uniclasser.hpp", "a combined header file");
	generate_header(profiler);
	out_close();
	
	if (test)
	{
		out_open("test_uniclasser.hpp", "a combined test header file");
		generate_test_header();
		out_close();
	}
	
	if (profiler)
	{
		out_open("uniclasser_profiler.cpp", "profiler");
		generate_profiler();
		out_close();
	}
	
	out_open("main.cpp", "main");
	generate_main(test, profiler);
	out_close();
}
