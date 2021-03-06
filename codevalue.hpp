// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#ifndef CODEVALUE_H
#define CODEVALUE_H

#include <string>
#include <utility>
#include <vector>


//----- codevalue -------------------------------------------------------------

#define CODEVALUE wchar_t

typedef CODEVALUE codevalue;

typedef std::basic_string<codevalue> ustring;

typedef std::vector<codevalue> codevalue_vector;


//----- Version ---------------------------------------------------------------

#define VERSION "1.0"
#define URL "https://github.com/roysharon/Uniclasser"

#endif