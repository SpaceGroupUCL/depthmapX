// genlib - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef __XMLPARSE_H__
#define __XMLPARSE_H__

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

inline string tolower(const string& str)
{
   string s = str;
   for (size_t i = 0; i < s.length(); i++) {
      s[i] = tolower(s[i]);
   }
   return s;
}

struct xmlelement
{
   string name;
   bool closetag;
   map<string,string> attributes;
   vector<xmlelement> subelements;
   xmlelement()
   { closetag = false;}
   bool parse(ifstream& stream, bool parsesubelements = false);
   friend ostream& operator << (ostream& stream, const xmlelement& elem);
protected:
   bool subparse(ifstream& stream);
   void badcharacter(char c, const string& location);
};

struct xmlerror
{
   string error;
   xmlerror(const string& e = string())
   { error = e; }
};

#endif
