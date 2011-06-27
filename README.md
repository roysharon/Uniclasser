The uniclasser generator receives a Unicode general category name, and produces a classifier for this category:

		./uniclasser Lu

This will create five files:

 * uniclasser_Lu.cpp - the classifier itself.
 * uniclasser.hpp - a header file for the classifier.
 * test_uniclasser_Lu.cpp - test suite for the classifier.
 * text_uniclasser.hpp - a header file for the test suite.
 * main.cpp - a simple file that runs the classifier against any supplied character, or runs the test suite if no characters were supplied.

The generated classifier performs a simple task: it takes a single Unicode character and returns a boolean that indicates whether or not this character is part of the classifier's character set:

		bool uniclasser_Lu(codevalue c) { .... }
		
If the supplied Unicode character `c` is included in the Lu ("Letter, Uppercase") general category, the classifier returns true.


There are several options to control how the classifier files are created:

 * `-t` causes the generator to not create the test suite files.
 * `-c` causes the generator to create the files in C rather than in C++.
 * `-u <path>` tells the generator to read the unicode data from the specified path (default: ./UnicodeData.txt). You can download the unicode data of the latest unicode version from <http://www.unicode.org/Public/UNIDATA/UnicodeData.txt>.

If you specify several categories seperated by commas, the created classifier will include all characters within any of these categories. For example:

		./uniclasser Lu,Ll
		
will produce a classifier that identifies whether a given character belongs either to the "Letter, Uppercase" or to the "Letter, Lowercase" general categories.

If you specify several arguments seperated by spaces, several classifiers will be created. For example:

		./uniclasser Lu Ll

will produce two classifiers: one for the "Letter, Uppercase" category and one for the "Letter, Lowercase" category. By performing a logical OR between these two classifiers you could of course create the Lu,Ll classifier mentioned in the previous example. However, this combined classifier will be less efficient than the auto-generated one, and perform more JNE's (jump on not equal) than actually needed.

Using or modifying this project is governed by the [MIT License](http://creativecommons.org/licenses/MIT/).
 