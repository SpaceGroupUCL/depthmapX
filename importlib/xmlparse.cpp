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


#include <importlib/xmlparse.h>

using namespace std;

enum { STEP_START, STEP_ELEMENT_NAME, 
                   STEP_ATTRIBUTE_NAME,
                   STEP_START_ATTRIBUTE_VALUE, STEP_ATTRIBUTE_VALUE,
                   STEP_CLOSING };

bool iscrlf(char c)
{
   // \n is MAC = 13, UNIX = 10, MS = 13,10
   return (c == 10 || c == 13);
}

bool xmlelement::parse(ifstream& stream, bool parsesubelements)
{
   bool closed = false;
   bool complete = false;
   int step = STEP_START;
   string buffer;
   string attribute;
   string value;
   while (!complete && stream) {
      char c = stream.get();
      if (stream) {
         switch (step) {
         case STEP_START:
            if (c == '<') {
               step = 1;
               buffer.clear();
            }
            break;
         case STEP_ELEMENT_NAME:
            if (isspace(c) || iscrlf(c)) {
               name = buffer;
               buffer.clear();
               step = STEP_ATTRIBUTE_NAME;
            }
            else if (c == '/') {
               if (buffer.empty()) {
                  closetag = true;
               }
               else {
                  name = buffer;
                  buffer.clear();
                  step = STEP_CLOSING;
               }
            }
            else if (c == '>') {
               if (buffer.empty()) {
                  throw xmlerror("No element name");
               }
               else {
                  name = buffer;
                  buffer.clear();
                  complete = true;
                  if (closetag) {
                     closed = true;
                  }
                  else {
                     if (parsesubelements) {
                        closed = subparse(stream);
                     }
                  }
               }
            }
            else if (isalnum(c) || ispunct(c)) {
               buffer += c;
            }
            else {
               badcharacter(c,"parsing element name");
            }
            break;
         case STEP_CLOSING:
            if (c == '>') {
               // only get here if midway through tag
               closed = true;
               complete = true;
            }
            else if (!isspace(c) && !iscrlf(c)) {
               badcharacter(c,"closing element tag");
            }
            break;
         case STEP_ATTRIBUTE_NAME:
            if (isspace(c) || iscrlf(c)) {
               if (attribute.empty()) {
                  attribute = buffer;
                  buffer.clear();
               }
            }
            else if (c == '=') {
               if (attribute.empty()) {
                  attribute = buffer;
                  if (attribute.empty()) {
                     throw xmlerror("No attribute name");
                  }
               }
               buffer.clear();
               step = STEP_START_ATTRIBUTE_VALUE;
            }
            else if (c == '/') {
               // could be closing a tag without specifying attribute value, but we'll be okay:
               step = STEP_CLOSING;
            }
            else if (c == '>') {
               complete = true;
               // could be closing a tag without specifying attribute value, but we'll be okay:
               if (parsesubelements) {
                  closed = subparse(stream);
               }
            }
            else if (isalnum(c) || ispunct(c)) {
               buffer += c;
            }
            else {
               badcharacter(c,"reading attribute name");
            }
            break;
         case STEP_START_ATTRIBUTE_VALUE:
            if (c == '\"') {
               step = STEP_ATTRIBUTE_VALUE;
            }
            else if (!isspace(c) && !iscrlf(c)) {
               badcharacter(c,"expecting opening '\"'");
            }
            break;
         case STEP_ATTRIBUTE_VALUE:
            if (c == '\"') {
               attributes[attribute] = buffer;
               buffer.clear();
               attribute.clear();
               step = STEP_ATTRIBUTE_NAME;
            }
            else {
               // there was a bad char test for 'isprint(c)', but it didn't like certain characters!
               buffer += c;
            }
            break;
         }
      }
   }
   if (!complete && step != STEP_START) {
      throw xmlerror(string("Unexpected end of element ") + name);
   }
   return closed;
}

void xmlelement::badcharacter(char c, const string& location)
{
   if (isprint(c)) {
      throw (string("Found '") + c + string("' while ") + location);
   }
   else {
      stringstream s;
      s << "Found character " << int(c) << " while " << location;
      throw (s.str());
   }
}

bool xmlelement::subparse(ifstream& stream)
{
   bool complete = false;
   while (!complete && stream) {
      subelements.push_back(xmlelement());
      if (!subelements.back().parse(stream,true)) {
         throw xmlerror(string("Unexplained error"));
      }
      if (subelements.back().closetag) {
         if (subelements.back().name == name) {
            subelements.pop_back();
            complete = true;
         }
         else {
            throw xmlerror(string("Element <") + name + string("> closed with </") + subelements.back().name + string(">"));
         }
      }
   }
   if (!complete) {
      throw xmlerror(string("Unexpected end of element ") + name);
   }
   return true;
}

ostream& operator << (ostream& stream, const xmlelement& elem)
{
   stream << "<" << elem.name;
   map<string,string>::const_iterator it;
   for (it = elem.attributes.begin(); it != elem.attributes.end(); it++) {
     stream << " " << it->first << "=\"" << it->second << "\" ";
   }
   if (elem.subelements.size() == 0) {
      stream << " />" << endl;
   }
   else {
      stream << ">" << endl;
      for (size_t i = 0; i < elem.subelements.size(); i++) {
         stream << elem.subelements[i];
      }
      stream << "</" << elem.name << ">" << endl;
   }
   return stream;
}
