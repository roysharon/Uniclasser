#include <iostream>
#include <cassert>
#include <sstream>
#include "unistd.h"
#include "unicode_data.hpp"
#include "match_tree.hpp"
#include "predicate.hpp"
#include "cpp_generator.hpp"
#include "c_generator.hpp"

using namespace std;


void short_help_message()
{
	cout << "usage: uniclasser [-tp] [-u path] categories" << endl
		 << "type uniclasser without any arguments for help." << endl;
}

void help_message()
{
	cout << "generates highly efficient classifiers for Unicode characters based" << endl 
		 << "on their properties." << endl 
		 << "usage:  uniclasser [-tp] [-u path] categories" << endl
		 << "options:" << endl 
		 << "  -t        do not generate classifier test functions." << endl 
		 << "  -p        generate profiling information (not suitable for production code)." << endl 
		 << "  -c        generator C code (instead of the default C++)." << endl 
		 << "  -u path   read unicode data from specified path (default: ./UnicodeData.txt)." << endl 
		 << "            You can download the unicode data of the latest unicode version from:" << endl
		 << "            http://www.unicode.org/Public/UNIDATA/UnicodeData.txt" << endl;
}

int main (int argc, char * const argv[])
{
	cout << "uniclasser v" << VERSION << " built " << __DATE__ << endl << "See " << URL << endl;
	
	bool test = true, profiler = false;
	string data_filename("./UnicodeData.txt"), output_dir("./");

	auto_ptr<IGenerator> generator(new CppGenerator(output_dir));
	
	opterr = 0;
	int c;
	while ((c = getopt(argc, argv, ":tpcu:")) != -1)
	{
		switch (c)
		{
			case 't':
				test = false;
				break;
			case 'p':
				profiler = true;
				break;
			case 'c':
				generator.reset(new CGenerator(output_dir));
				break;
			case 'u':
				data_filename = optarg;
				break;
			case ':':
				cerr << "Option -" << (char)optopt << " requires an argument." << endl;
				short_help_message();
				return 1;
			case '?':
				if (isprint(optopt))
					cerr << "Unknown option '-" << (char)optopt << "'." << endl;
				else
					cerr << "Unknown option character '" << hex << showbase << optopt << "'." << endl;
				short_help_message();
				return 1;
			default:
				short_help_message();
				abort();
		}
	}
	if (optind >= argc)
	{
		if (optind > 1)
		{
			cerr << "No general catergories were supplied. Nothing to create." << endl;
			short_help_message();
		}
		else help_message();
		return 1;
	}
	
	cout << "Reading " << data_filename << " file..." << endl;
	UnicodeData unicode(data_filename);
	if (unicode.count() == 0) return 1;
	cout << "Read " << unicode.count() << " codevalues." << endl;

	for (int i = optind; i < argc; ++i) {
		auto_ptr<codevalue_vector> codes(unicode.filter_multiple_gc(argv[i]));
		cout << endl << "General Category '" << argv[i] << "' matched " << codes->size() << " codevalues." << endl;
		
		cout << "Building match tree..." << endl;
		MatchTree tree(*codes);
		cout << "Built match tree with " << dec << tree.count << " nodes." << endl;
		
		cout << "Building classifier predicate..." << endl;
		Predicate predicate;
		int compare_jump = tree.create_predicate(predicate);
		cout << "Created a predicate with " << dec << compare_jump << " compare/jumps." << endl;
		assert(tree.count == 0); // should consume all tree nodes
		
		string classer_name("uniclasser_");
		classer_name += argv[i];
		replace(classer_name.begin(), classer_name.end(), ',', '_');
		generator->generate(classer_name, predicate, test ? codes.get() : 0, profiler);
		
		cout << endl;
	}
	
	generator->finalize(test, profiler);
	
	cout << "Finished!" << endl;
	
	return 0;
}

/* ToDo:
 
	Can predicate hold the number of compare/jumps? If so, then change MatchTree.create_predicate() to return a Predicate*
	Check why generated code does not handle multibyte characters input
	Decide on license
	Change general category property to be a bitfield
	Add further properties
	Add timing
	
 */
