// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <iterator>
#include <vector>
#include <istream>
#include <string>
#include <map>
#include "codevalue.hpp"

typedef std::vector<ustring> line_t;


//----- Parser for UnicodeData.txt file ---------------------------------------

struct UnicodeDataParser : std::iterator<std::input_iterator_tag, line_t>
{
	UnicodeDataParser(std::istream &file) : in(file), eofbit(false), buf(L""), line_number(0) {}
	UnicodeDataParser(const UnicodeDataParser& p) : in(p.in), eofbit(p.eofbit), buf(p.buf), cur(p.cur), line_number(p.line_number) {}
	
	void fill_buf();
	line_t& parse_buf();
	
	UnicodeDataParser& operator++()
	{
		buf.clear();
		parse_buf();	// fills both buf and cur
		return *this;
	}
	
	UnicodeDataParser operator++(int)
	{
		UnicodeDataParser tmp(*this);
		operator++();
		return tmp;
	}
	
	line_t& operator*() { return parse_buf(); }
	
	bool operator==(const UnicodeDataParser& rhs) { return in == rhs.in; }
	bool operator!=(const UnicodeDataParser& rhs) { return in != rhs.in; }
	
	bool eof() { return eofbit; }
	
	std::istream &in;
	bool eofbit;
	ustring buf;
	line_t cur;
	unsigned line_number;
};


//----- UnicodeData -----------------------------------------------------------

struct UnicodeData
{
	UnicodeData(std::string filename);
	
	unsigned count() { return codevalues.size(); }
	
	typedef unsigned char property_t;

	inline property_t propval(property_t mask, property_t shift, property_t val) { return (val << shift) & mask; }

	codevalue_vector* filter(property_t mask, property_t val);
	
	void add_codevalue(codevalue c);
	void set_property(property_t mask, property_t shift, property_t val);
	
	std::vector<property_t> properties;
	codevalue_vector codevalues;

	// General Category property
	std::map<std::wstring, property_t> gc_map;
	property_t gc_count;
	static const property_t gc_mask = 0x1F, gc_shift = 0;
	codevalue_vector* filter_gc(const char * const gc);
	codevalue_vector* filter_multiple_gc(const char * const mgc);
};



#endif