#include <fstream>
#include <sstream>
#include "unicode_data.hpp"

using namespace std;

//----- Parser for UnicodeData.txt file ---------------------------------------

void UnicodeDataParser::fill_buf()
{
	while (!in.eof())
	{
		char tmp[1024];
		in.getline(tmp, sizeof(tmp));
		if (in.gcount() == 0) break;
		
		++line_number;
		
		wchar_t wtmp[sizeof(tmp)];
		int n = mbstowcs(wtmp, tmp, sizeof(wtmp));
		if (n < 0)
		{
			cerr << "Error while reading line " << line_number << ". Skipping line." << endl;
			continue;
		}
		
		wtmp[n] = 0;
		buf += wtmp;
		break;
	}
}

line_t& UnicodeDataParser::parse_buf()
{
	if (buf.empty())
	{
		fill_buf();
		cur.clear();
	}
	if (cur.empty())
	{
		ustring s;
		for (ustring::const_iterator i = buf.begin(), e = buf.end(); i != e; ++i)
		{
			switch (*i) {
				case ' ':	// ignore whitespace: space
				case '\t':	// ignore whitespace: tab
				case '\v':	// ignore whitespace: vertial tab
					continue;
				case ';':	// field delimiter
					cur.push_back(s);
					s.clear();
					continue;
				case '#':	// rest of line is a comment, so ignore it
					break;
				default:
					s += *i;
					break;
			}
		}
		if (!s.empty()) cur.push_back(s);
	}
	eofbit = in.eof();
	return cur;
}


//----- UnicodeData -----------------------------------------------------------

UnicodeData::UnicodeData(std::string filename) : gc_count(0)
{
	ifstream data(filename.c_str(), ios_base::in);
	if (!data)
	{
		cerr << "Error: Could not open file " << filename << endl
			 << "You can download the latest version of this file from http://www.unicode.org/Public/UNIDATA/UnicodeData.txt" << endl;
		return;
	}
    data.unsetf(ios::skipws); // No white space skipping!
	
	UnicodeDataParser parser(data);
	while (!parser.eof())
	{
		line_t l = *parser;
		++parser;
		
		codevalue c;
		swscanf(l[0].c_str(), L"%x", &c);
		add_codevalue(c);
		
		// parse General Category (second field in each line in UnicodeData.txt)
		wstring gc(l[2]);
		map<wstring, property_t>::iterator i = gc_map.find(gc);
		if (i == gc_map.end())
		{
			property_t v = ++gc_count;
			if (v > (gc_mask >> gc_shift))
			{
				char tmp[256];
				wcstombs(tmp, gc.c_str(), sizeof(tmp));
				cerr << "Error: More General Categories than " << (gc_mask >> gc_shift) << ". General Category '" << tmp << "' ignored." << endl;
				v = 0;
			}
			i = gc_map.insert(pair<wstring, property_t>(gc, v)).first;
		}
		set_property(gc_mask, gc_shift, i->second);
		
		if (l[1].rfind(L"First>") != wstring::npos && !parser.eof())
		{
			l = *parser;
			++parser;
			
			codevalue d;
			swscanf(l[0].c_str(), L"%x", &d);
			while (++c <= d)
			{
				add_codevalue(c);
				set_property(gc_mask, gc_shift, i->second);
			}
		}
	}
	
	data.close();
}

void UnicodeData::add_codevalue(codevalue c)
{
	codevalues.push_back(c);
	properties.push_back(0);
}

void UnicodeData::set_property(property_t mask, property_t shift, property_t val) // always set the property of the last codevalue added by add_codevalue()
{
	properties.back() &= ~mask;
	properties.back() |= (val << shift) & mask;
}

codevalue_vector* UnicodeData::filter(property_t mask, property_t val)
{
	auto_ptr<codevalue_vector> codes(new codevalue_vector);
	
	for (unsigned i = 0, n = count(); i < n; ++i)
	{
		if ((properties[i] & mask) == val) codes->push_back(codevalues[i]);
	}
	sort(codes->begin(), codes->end());
	codes->resize(unique_copy(codes->begin(), codes->end(), codes->begin()) - codes->begin());

	return codes.release();
}

codevalue_vector* UnicodeData::filter_gc(const char * const gc)
{
	wstringstream wgc;
	wgc << gc;
	map<wstring, property_t>::iterator i = gc_map.find(wgc.str());
	if (i == gc_map.end() || i->second == 0)
	{
		cerr << "Error: General Category '" << gc << "' is undefined. Ignoring." << endl;
		return new codevalue_vector;
	}
	else return filter(gc_mask, propval(gc_mask, gc_shift, i->second));
}

codevalue_vector* UnicodeData::filter_multiple_gc(const char * const mgc)
{
	string m(mgc);
	auto_ptr<codevalue_vector> codes(new codevalue_vector);
	size_t p = 0, n;
	do
	{
		n = m.find(',', p);
		string gc = m.substr(p, n == m.npos ? m.npos : n-p);
		p = n+1;
		auto_ptr<codevalue_vector> more(filter_gc(gc.c_str()));
		auto_ptr<codevalue_vector> merged(new codevalue_vector);
		merged->reserve(codes->size() + more->size());
		merge(codes->begin(), codes->end(), more->begin(), more->end(), back_inserter(*merged));
		codes.reset(merged.release());
	} 
	while (n != m.npos);
	return codes.release();
}

