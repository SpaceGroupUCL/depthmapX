// salaprogram.h - a component of the depthmapX - spatial network analysis platform
// SalaScripting language
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


#ifndef __SALAPROGRAM_H__
#define __SALAPROGRAM_H__

#include "mgraph440/stringutils.h"
#include "mgraph440/paftl.h"
#include <vector>
#include <set>
#include <cmath>

namespace mgraph440 {

class AttributeTable;
class PointMap;
class ShapeMap;

inline bool isalphanum_(char c)
{
   if (isalnum(c) || c == '_')
      return true;
   else
      return false;
}

inline bool isalpha_(char c)
{
   if (isalpha(c) || c == '_')
      return true;
   else
      return false;
}

struct SalaError
{
   int lineno;
   std::string message;
   SalaError(const std::string& m = std::string(), int li = -1)
   { message = m; lineno = li; }
};

/////////////////////////////////////////////////////////////////////////////////////////

// A series of 8-byte types to go in the SalaObj data union
// note, they cannot cannot instantiate a copy constructor as it is used as
// a member of the union in SalaObj

class SalaObj;

struct SalaStr
{
public:
   int *refcount;
   std::string *string;
public:
   friend bool operator == (const SalaStr& a, const SalaStr& b);
   friend bool operator != (const SalaStr& a, const SalaStr& b);
   friend bool operator < (const SalaStr& a, const SalaStr& b);
   friend bool operator > (const SalaStr& a, const SalaStr& b);
   // operator const std::string&() { return *string; }
   char char_at(size_t i) const
   { return string->operator[](i); }
   size_t length() const
   { return string->length(); }
};
inline bool operator == (const SalaStr& a, const SalaStr& b)
{ return *(a.string) == *(b.string); }
inline bool operator != (const SalaStr& a, const SalaStr& b)
{ return *(a.string) != *(b.string); }
inline bool operator < (const SalaStr& a, const SalaStr& b)
{ return *(a.string) < *(b.string); }
inline bool operator > (const SalaStr& a, const SalaStr& b)
{ return *(a.string) > *(b.string); }

struct SalaList {
   int *refcount;
   pvector<SalaObj> *list;
public:
   friend bool operator == (const SalaList& a, const SalaList& b);
   friend bool operator != (const SalaList& a, const SalaList& b);
   // inlines below
};

struct SalaGrf {
   int node;
   union Map {
      PointMap *point;  // vga
      ShapeMap *shape;  // everything else
   };
   Map map;
};

// SalaObj is 16 bytes, which is larger than I intended, but it appears
// when you put both a double (8 bytes) and an int (4 bytes) into a class, it pads
// to 16 bytes rather than the 12 you would expect

// union members aren't allow copy constructors, so the list functionality
// is built directly into the SalaObj, making it no more inefficient than if it
// were to reference directly to another object to find, e.g., length or refcount

// note lists are stored by reference.  I'm not sure if this is a good idea!

class SalaObj
{
   friend class SalaProgram;
   friend class SalaCommand;
   friend class SalaArray;
public:
   // Object types
   enum Type { S_BRACKET = 0x0000003f, S_OPEN_SQR_BRACKET = 0x0000000c,
               S_OPEN_BRACKET = 0x00000001, S_CLOSE_BRACKET = 0x00000002,
               S_OPEN_SQR_BRACKET_LIST = 0x00000004, S_OPEN_SQR_BRACKET_ACCESS = 0x00000008,
               S_CLOSE_SQR_BRACKET = 0x00000010, S_COMMA = 0x00000020, // bracket includes comma for checking purposes
               S_NONE = 0x00000100, S_UNINIT = 0x00000200, S_FUNCTION = 0x00000400,
               S_BOOL = 0x00001000, S_CHAR = 0x00002000,
               S_INT = 0x00004000, S_DOUBLE = 0x00008000, S_NUMBER = 0x0000c000,
               S_STRING = 0x00010000, S_VAR = 0x00020000,
               S_CONST_LIST = 0x00100000, S_CONST_TUPLE = 0x00300000, // tuple is a type of list
               S_LIST = 0x00400000, S_TUPLE = 0x00500000,  // tuple is a type of list
               // maps are bitwise 'or'ed to node to make appropriate node type for each map
               S_GRAPHOBJ = 0x01000000, S_MAP = 0x06000000, S_POINTMAP = 0x02000000, S_SHAPEMAP = 0x04000000,
               // however, as the variable is uses the typename of the enum, each must be filled in explicitly:
               S_POINTMAPOBJ = 0x03000000, S_SHAPEMAPOBJ = 0x05000000,
               S_THIS = 0x10000000
         };
   // Built-in Functions, note, some of the groupings contain other operations (eg., math ops includes assign, and logical ops includes both comparators and logical ops)
   enum Func {
          S_FNULL = 0x00000000, S_GROUP = 0xf0000000,
          S_MATH_OPS = 0x10000000, S_LOGICAL_OPS = 0x20000000, S_GLOBAL_FUNCS = 0x30000000, S_MEMBER_FUNCS = 0x40000000,
          S_ADD = 0x10000001, S_SUBTRACT = 0x10000002, S_MINUS = 0x10000003, S_PLUS = 0x10000004,
          S_MULTIPLY = 0x10000005, S_DIVIDE = 0x10000006, S_MODULO = 0x10000007, S_POWER = 0x10000008,
          S_ASSIGN = 0x10000009, S_LIST_ACCESS = 0x1000000a,
          S_LT  = 0x20000001, S_GT = 0x20000002, S_LEQ = 0x20000003, S_GEQ = 0x20000004, S_EQ = 0x20000005,
          S_NEQ = 0x20000006, S_AND = 0x20000007, S_OR = 0x20000008, S_NOT = 0x20000009, S_IS = 0x2000000a,
          S_LEN = 0x30000001, S_RANGE = 0x30000002,
          S_SQRT = 0x30000003, S_LOG = 0x30000004, S_LN = 0x30000005, S_RAND = 0x30000006,
          S_SIN = 0x30000007, S_COS = 0x30000008, S_TAN = 0x30000009,
          S_ASIN = 0x3000000a, S_ACOS = 0x3000000b, S_ATAN = 0x3000000c,
          S_FPOP = 0x40000001, S_FAPPEND = 0x40000002, S_FEXTEND = 0x40000003, S_FCLEAR = 0x40000004,
          S_FVALUE = 0x40000011, S_FSETVALUE = 0x40000012, S_FCONNECTIONS = 0x40000013,
          S_FMARK = 0x40000014, S_FSETMARK = 0x40000015
   };
protected:
   union Data {
      bool   b;
      char   ch;
      int    i;
      double f;
      SalaList list;
      SalaStr str;
      SalaGrf graph;
      Func   func;
      int    var;
      int    count;  // used by brackets to count how many objects they have
   };
   Data data;
   Type type;
public:
   SalaObj() { type = S_NONE; }
   // Two usages: (a) used for brackets (=groups of things, hence the count) and commas
   //             (b) used for lists
   SalaObj(Type t)
   {
      type = t;
      if (t & S_LIST) {
         data.list.refcount = new int(1);
         data.list.list = new pvector<SalaObj>;
      }
      else {
         data.count = 1;
      }
   }
   // Two usages: (a) used to address variable or user function tables
   //             (b) used for lists
   SalaObj(Type t, int v)
   {
      type = t;
      if (t & S_LIST) {
         data.list.refcount = new int(1);
         data.list.list = new pvector<SalaObj>;
         data.list.list->set(v); // set blanks
      }
      else {
         data.var = v;
      }
   }
   // other constructors
   SalaObj(bool a) { type = S_BOOL; data.b = a; }
   SalaObj(int a) { type = S_INT; data.i = a; }
   SalaObj(double a) { type = S_DOUBLE; data.f = a; }
   SalaObj(Func f) { type = S_FUNCTION; data.func = f; }
   SalaObj(const std::string& a) { type = S_STRING; data.str.refcount = new int(1); data.str.string = new std::string(a); }
   // note, type required here as sometimes this will be an axial map, sometimes segment map, sometimes point map,
   // also not fully filled in until runtime, but still required by parse
   SalaObj(Type t, SalaGrf graph)
   { type = t; data.graph = graph; }
   //
   SalaObj(const SalaObj& obj);
   SalaObj& operator = (const SalaObj& obj);
   ~SalaObj();
   void reset();
   void uninit() { reset(); type = S_UNINIT; }  // <- used to uninitialise variables before running program, thus they give nice error messages if used before initialisation
   int func() const { return data.func; }
   int precedence() const;
   bool toBool() const;
   int toInt() const;
   double toDouble() const;
   std::string toString() const;
   const std::string& toStringRef() const;
   friend SalaObj op_is(SalaObj& a, SalaObj& b);
   friend SalaObj operator - (SalaObj& a);
   friend SalaObj operator + (SalaObj& a, SalaObj& b);
   friend SalaObj operator - (SalaObj& a, SalaObj& b);
   friend SalaObj operator / (SalaObj& a, SalaObj& b);
   friend SalaObj operator * (SalaObj& a, SalaObj& b);
   friend SalaObj operator % (SalaObj& a, SalaObj& b);
   friend bool operator || (SalaObj& a, SalaObj& b);
   friend bool operator && (SalaObj& a, SalaObj& b);
   friend bool operator ! (SalaObj& a);
   friend bool operator == (SalaObj& a, SalaObj& b);
   friend bool operator != (SalaObj& a, SalaObj& b);
   friend bool operator > (SalaObj& a, SalaObj& b);
   friend bool operator < (SalaObj& a, SalaObj& b);
   friend bool operator >= (SalaObj& a, SalaObj& b);
   friend bool operator <= (SalaObj& a, SalaObj& b);
   // operations for lists:
   SalaObj& list_at(int i);
   SalaObj char_at(int i); // actually returns a string of the char -- note constant
   int length();
   // check for no parameters
   void ensureNone()
   { if (type != SalaObj::S_NONE) throw SalaError("Does not take any parameters"); }
   //
   // operations for graphs / graph nodes:
   AttributeTable *getTable();
   //
   const std::string getTypeStr() const;
   const std::string getTypeIndefArt() const;
};

// Quick mod - TV
class SalaProgram;

class SalaCommand
{
   friend class SalaProgram;
   //
   enum Command { SC_NONE, SC_ROOT, SC_EXPR, SC_RETURN, SC_FOR, SC_WHILE, SC_IF, SC_ELIF, SC_ELSE };
   enum { SP_NONE, SP_DATA, SP_NUMBER, SP_FUNCTION, SP_COMMAND }; // used while calculating what is on eval stack
protected:
   //
   SalaProgram *m_program; // information about the running program (in particular, the global variable and error stack)
   SalaCommand *m_parent;
   prefvec<SalaCommand> m_children;
   //
   pqmap<std::string,int> m_var_names;
   //
   Command m_command;
   int m_indent;  // vital for program flow due to Pythonesque syntax
   pvector<SalaObj> m_eval_stack;
   pvector<SalaObj> m_func_stack;
   //
   SalaObj m_for_iter;  // object used in a for loop
   //
   int m_line; // useful for debugging to know which line this command starts on
   std::string m_last_string; // occassionally useful in debugging if the user does something unsyntactical
   //
public:
   SalaCommand() { m_program = NULL; m_parent = NULL; m_indent = 0; m_command = SC_NONE; }
   SalaCommand(SalaProgram *program, SalaCommand *parent, int indent, Command command = SC_NONE);
protected:
   int parse(::std::istream& program, int line);
   int decode(std::string string);
   int decode_member(const std::string& string, bool apply_to_this);
   void pushFunc(const SalaObj& func);
   //
   void evaluate(SalaObj& obj, bool& ret, bool& ifhandled);
   SalaObj evaluate(int& pointer, SalaObj* &p_obj);
   SalaObj connections(SalaObj graphnode, SalaObj param);
};

class SalaProgram
{
   friend class SalaCommand;
   //
   SalaCommand m_root_command;
   pvector<SalaObj> m_var_stack;
   prefvec<SalaError> m_error_stack;
   //
   // column is stored away from the context, as it's not actually passed to the program itself, just used to update a column
   int m_col;
   // m_thisobj stores contextual information (which attribute table, node etc)
   // NB ! -- this can be messed with by SalaCommand!
   SalaObj m_thisobj;
   //
   bool m_marked; // this is used to tell the program that a node has been "marked" -- all marks are cleared at the end of the execution
   //
public:
   SalaProgram(SalaObj context);
   ~SalaProgram();
   bool parse(::std::istream& program);
   SalaObj evaluate();
   bool runupdate(int col, const std::set<int> &selset = std::set<int>());
   bool runselect(std::vector<int>& selsetout, const std::set<int> &selsetin = std::set<int>());
   std::string getLastErrorMessage() const;
};

inline SalaObj::SalaObj(const SalaObj& obj)
{
   type = obj.type;
   switch(obj.type) {
      case S_FUNCTION: data.func = obj.data.func; break;
      case S_BOOL: data.b = obj.data.b; break;
      case S_INT: data.i = obj.data.i; break;
      case S_DOUBLE: data.f = obj.data.f; break;
      case S_VAR: data.var = obj.data.var; break;
      case S_STRING:
         data.str.string = obj.data.str.string;
         data.str.refcount = obj.data.str.refcount;
         *(data.str.refcount) += 1;
         break;
      case S_LIST: case S_TUPLE:
         data.list.list = obj.data.list.list;
         data.list.refcount = obj.data.list.refcount;
         *(data.list.refcount) += 1;
         break;
      case S_NONE: case S_UNINIT: case S_THIS:
         break;
      case S_SHAPEMAPOBJ: case S_SHAPEMAP:
         data.graph.map.shape = obj.data.graph.map.shape;
         data.graph.node = obj.data.graph.node;
         break;
      case S_POINTMAPOBJ: case S_POINTMAP:
         data.graph.map.point = obj.data.graph.map.point;
         data.graph.node = obj.data.graph.node;
         break;
      case S_OPEN_BRACKET: case S_CLOSE_BRACKET: case S_OPEN_SQR_BRACKET_LIST: case S_OPEN_SQR_BRACKET_ACCESS:
      case S_CLOSE_SQR_BRACKET: case S_COMMA: case S_CONST_LIST: case S_CONST_TUPLE:
         data.count = obj.data.count; break;
      default: throw SalaError("Cannot instantiate unknown type");
   }
}
inline SalaObj& SalaObj::operator = (const SalaObj& obj)
{
   if (this != &obj) {
      reset();
      type = obj.type;
      switch(obj.type) {
         case S_FUNCTION: data.func = obj.data.func; break;
         case S_BOOL: data.b = obj.data.b; break;
         case S_INT: data.i = obj.data.i; break;
         case S_DOUBLE: data.f = obj.data.f; break;
         case S_VAR: data.var = obj.data.var; break;
         case S_STRING:
            data.str.string = obj.data.str.string;
            data.str.refcount = obj.data.str.refcount;
            *(data.str.refcount) += 1;
            break;
         case S_LIST: case S_TUPLE:
            data.list.list = obj.data.list.list;
            data.list.refcount = obj.data.list.refcount;
            *(data.list.refcount) += 1;
            break;
         case S_NONE: case S_UNINIT: case S_THIS:
            break;
         case S_SHAPEMAPOBJ: case S_SHAPEMAP:
            data.graph.map.shape = obj.data.graph.map.shape;
            data.graph.node = obj.data.graph.node;
            break;
         case S_POINTMAPOBJ: case S_POINTMAP:
            data.graph.map.point = obj.data.graph.map.point;
            data.graph.node = obj.data.graph.node;
            break;
         case S_OPEN_BRACKET: case S_CLOSE_BRACKET: case S_OPEN_SQR_BRACKET_LIST: case S_OPEN_SQR_BRACKET_ACCESS:
         case S_CLOSE_SQR_BRACKET: case S_COMMA: case S_CONST_LIST: case S_CONST_TUPLE:
            data.count = obj.data.count; break;
         default: throw SalaError("Cannot instantiate unknown type");
      }
   }
   return *this;
}
inline SalaObj::~SalaObj()
{
   reset();
}
inline void SalaObj::reset()
{
   if (type & S_STRING) {
      *(data.str.refcount) -= 1;
      if (*(data.str.refcount) == 0) {
         delete data.str.refcount;
         delete data.str.string;
      }
      data.str.refcount = NULL;
      data.str.string = NULL;
   }
   else if (type & S_LIST)
   {
      *(data.list.refcount) -= 1;
      if (*(data.list.refcount) == 0) {
         delete data.str.refcount;
         delete data.list.list;
      }
      data.str.refcount = NULL;
      data.list.list = NULL;
   }
   type = S_NONE;
}
inline bool SalaObj::toBool() const
{
   switch(type) {
      case S_BOOL: return data.b;
      case S_INT: return data.i != 0;
      case S_DOUBLE: return data.f != 0.0;
      default:
         throw SalaError(std::string("Cannot convert ") + getTypeIndefArt() + getTypeStr() + std::string(" to a boolean value"));
   }
   return false;
}
inline int SalaObj::toInt() const
{
   switch(type) {
      case S_BOOL: return data.b ? 1 : 0;
      case S_INT: return data.i;
      case S_DOUBLE: return int(floor(data.f)); // ensure properly implemented
      default: throw SalaError(std::string("Cannot convert ") + getTypeIndefArt() + getTypeStr() + std::string(" to an integer value"));
   }
   return 0;
}
inline double SalaObj::toDouble() const
{
   switch(type) {
      case S_BOOL: return data.b ? 1.0 : 0.0;
      case S_INT: return double(data.i);
      case S_DOUBLE: return data.f;
      default: throw SalaError(std::string("Cannot convert ") + getTypeIndefArt() + getTypeStr() + std::string(" to a floating point number"));
   }
   return 0.0;
}
inline std::string SalaObj::toString() const
{
   switch(type) {
      case S_INT: return dXstring440::formatString(data.i);
      case S_DOUBLE: return dXstring440::formatString(data.f);
      case S_STRING: return *(data.str.string);
      default: throw SalaError(std::string("Cannot convert ") + getTypeIndefArt() + getTypeStr() + std::string(" to a string"));
   }
   return std::string();
}
inline const std::string& SalaObj::toStringRef() const
{
   if (type != S_STRING) {
      throw SalaError(std::string("Cannot convert ") + getTypeIndefArt() + getTypeStr() + std::string(" to a string reference"));
   }
   return *(data.str.string);
}

inline SalaObj operator + (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: throw SalaError("Cannot add booleans");
      case SalaObj::S_INT: return SalaObj(a.data.i + b.data.i);
      case SalaObj::S_DOUBLE: return SalaObj(a.data.f + b.data.f);
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) + b.data.f) : (a.data.f + double(b.data.i));
      case SalaObj::S_STRING: return SalaObj(*(a.data.str.string) + *(b.data.str.string));
      default: throw SalaError(std::string("Cannot add ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" to ")  + b.getTypeIndefArt() + b.getTypeStr());
   }
   return SalaObj();
}
inline SalaObj operator - (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: throw SalaError("Cannot subtract booleans");
      case SalaObj::S_INT: return SalaObj(a.data.i - b.data.i);
      case SalaObj::S_DOUBLE: return SalaObj(a.data.f - b.data.f);
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) - b.data.f) : (a.data.f - double(b.data.i));
      default: throw SalaError(std::string("Cannot subtract ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" from ") + a.getTypeIndefArt() + a.getTypeStr());
   }
   return SalaObj();
}
inline SalaObj operator - (SalaObj& a)
{
   switch (a.type) {
      case SalaObj::S_BOOL: throw SalaError("Cannot minus booleans");
      case SalaObj::S_INT: return SalaObj(-a.data.i);
      case SalaObj::S_DOUBLE: return SalaObj(-a.data.f);
      default: throw SalaError(std::string("Cannot minus ") + a.getTypeIndefArt() + a.getTypeStr());
   }
   return SalaObj();
}
inline SalaObj operator * (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_INT: return SalaObj(a.data.i * b.data.i);
      case SalaObj::S_DOUBLE: return SalaObj(a.data.f * b.data.f);
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) * b.data.f) : (a.data.f * double(b.data.i));
      default: throw SalaError(std::string("Cannot multiply ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" by ") + b.getTypeIndefArt() + b.getTypeStr());
   }
   return SalaObj();
}
inline SalaObj operator % (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_INT: return SalaObj(a.data.i % b.data.i);
      case SalaObj::S_DOUBLE: return SalaObj(::std::fmod(a.data.f,b.data.f));
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? fmod(double(a.data.i),b.data.f) : fmod(a.data.f,double(b.data.i));
      default: throw SalaError(std::string("Cannot multiply ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" by ") + b.getTypeIndefArt() + b.getTypeStr());
   }
   return SalaObj();
}
inline SalaObj operator / (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_INT: if (b.data.i != 0) return SalaObj(a.data.i / b.data.i); else throw SalaError("Integer divide by zero error");
      case SalaObj::S_DOUBLE: return SalaObj(a.data.f / b.data.f);
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) / b.data.f) : (a.data.f / double(b.data.i));
      default: throw SalaError(std::string("Cannot divide ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" by ")  + a.getTypeIndefArt() + b.getTypeStr());
   }
   return SalaObj();
}
// assume already bools (use convert to bool first)
inline bool operator && (SalaObj& a, SalaObj& b)
{
   return a.data.b && b.data.b;
}
// assume already bools (use convert to bool first)
inline bool operator || (SalaObj& a, SalaObj& b)
{
   return a.data.b || b.data.b;
}
// assume already bools (use convert to bool first)
inline bool operator ! (SalaObj& a)
{
   return !a.data.b;
}
inline bool operator == (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_NONE: return true; // none == none
      case SalaObj::S_BOOL: return a.data.b == b.data.b;
      case SalaObj::S_INT: return a.data.i == b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f == b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) == b.data.f) : (a.data.f == double(b.data.i));
      case SalaObj::S_STRING: return a.data.str == b.data.str;
      case SalaObj::S_LIST: return a.data.list == b.data.list;
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '=='"));
   }
   return false;
}
inline SalaObj op_is(SalaObj& a, SalaObj& b)
{
   // note, op_is is forgiving: does not complain if cannot compare, just returns false
   switch (a.type & b.type) {
      case SalaObj::S_NONE: return true; // none is none
      case SalaObj::S_BOOL: return a.data.b == b.data.b;
      case SalaObj::S_INT: return a.data.i == b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f == b.data.f;
      // n.b., no number! int is not double and v.v.
      case SalaObj::S_STRING: return a.data.str.string == b.data.str.string;  // n.b.: pointer compare!
      case SalaObj::S_LIST: return a.data.list.list == b.data.list.list;      // n.b.: pointer compare!
   }
   return false;
}

inline bool operator != (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: return a.data.b != b.data.b;
      case SalaObj::S_INT: return a.data.i != b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f != b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) != b.data.f) : (a.data.f != double(b.data.i));
      case SalaObj::S_STRING: return a.data.str != b.data.str;
      case SalaObj::S_LIST: return a.data.list != b.data.list;
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '!='"));
   }
   return false;
}
inline bool operator < (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: return a.data.b < b.data.b;
      case SalaObj::S_INT: return a.data.i < b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f < b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) < b.data.f) : (a.data.f < double(b.data.i));
      case SalaObj::S_STRING: return a.data.str < b.data.str;
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '<'"));
   }
   return false;
}
inline bool operator > (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: return a.data.b > b.data.b;
      case SalaObj::S_INT: return a.data.i > b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f > b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) > b.data.f) : (a.data.f > double(b.data.i));
      case SalaObj::S_STRING: return a.data.str > b.data.str;
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '>'"));
   }
   return false;
}
inline bool operator <= (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: return a.data.b <= b.data.b;
      case SalaObj::S_INT: return a.data.i <= b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f <= b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) <= b.data.f) : (a.data.f <= double(b.data.i));
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '<='"));
   }
   return false;
}
inline bool operator >= (SalaObj& a, SalaObj& b)
{
   switch (a.type | b.type) {
      case SalaObj::S_BOOL: return a.data.b >= b.data.b;
      case SalaObj::S_INT: return a.data.i >= b.data.i;
      case SalaObj::S_DOUBLE: return a.data.f >= b.data.f;
      case SalaObj::S_NUMBER:
         return (a.type == SalaObj::S_INT) ? (double(a.data.i) >= b.data.f) : (a.data.f >= double(b.data.i));
      default: throw SalaError(std::string("Cannot compare ") + a.getTypeIndefArt() + a.getTypeStr() + std::string(" with ") + b.getTypeIndefArt() + b.getTypeStr() + std::string(" using '>='"));
   }
   return false;
}
// list operations: note -> precheck in program and sort into list and string
inline SalaObj& SalaObj::list_at(int i)
{
   if (i < 0)
      i += (int)data.list.list->size();
   if (i < 0 || size_t(i) >= data.list.list->size())
      throw SalaError("Index out of range");
   return data.list.list->at(i);
}
inline SalaObj SalaObj::char_at(int i) // actually returns a string of the char
{
   if (i < 0)
      i += data.str.length();
   if (i < 0 || i >= static_cast<int>(data.str.length()))
      throw SalaError("String index out of range");
   return SalaObj(std::string(1,data.str.char_at(i)));
}
inline int SalaObj::length()
{
   if (type & S_LIST)
      return (int)data.list.list->size();
   else if (type == S_STRING)
      return (int)data.str.length();
   throw SalaError("Cannot get the length of " + getTypeIndefArt() + getTypeStr());
}

/////////////////////////////////////////////////////////////////////////////////////

inline const std::string SalaObj::getTypeStr() const
{
   switch(type) {
   case S_NONE:
      return "none";
   case S_UNINIT:
      return "uninitialised variable";
   case S_FUNCTION:
      return "function";
   case S_BOOL:
      return "boolean";
   case S_INT:
      return "integer";
   case S_DOUBLE:
      return "float";
   case S_STRING:
      return "string";
   case S_LIST:
      return "list";
   case S_TUPLE:
      return "tuple";
   case S_THIS:
      return "this";
   default:
       break;
   }
   if (type & S_GRAPHOBJ) {
      return "graph object";
   }
   else if (type & S_MAP) {
      return "graph";
   }
   return "unknown type";
}

inline const std::string SalaObj::getTypeIndefArt() const
{
   switch(type & ~S_GRAPHOBJ) {
   case S_FUNCTION: case S_BOOL: case S_DOUBLE: case S_STRING: case S_TUPLE: case S_LIST:
   case S_SHAPEMAP: case S_POINTMAP:
      return "a ";
   case S_INT: case S_UNINIT:
      return "an ";
   case S_NONE: case S_THIS:
      return "";
   default:
      return "an ";  // unknown type
   }
   return std::string();
}

/////////////////////////////////////////////////////////////////////////////////////

// comparisons for lists (must be after the associated SalaObj comparisons have been declared)

inline bool operator == (const SalaList& a, const SalaList& b)
{
   if (a.list->size() != a.list->size())
      return false;
   for (size_t i = 0; i < a.list->size(); i++) {
      if (a.list->at(i) != b.list->at(i))
         return false;
   }
   return true;
}
inline bool operator != (const SalaList& a, const SalaList& b)
{
   if (a.list->size() != a.list->size())
      return true;
   for (size_t i = 0; i < a.list->size(); i++) {
      if (a.list->at(i) != b.list->at(i))
         return true;
   }
   return false;
}

/////////////////////////////////////////////

// helpers for parser:

struct SalaBuffer
{
   int bufpos;
   char buffer[128];
   SalaBuffer()
   { bufpos = -1; buffer[0] = '\0'; }
   void add(char c)
   { bufpos++; if (bufpos > 127) throw SalaError("Overlong string of characters");
     buffer[bufpos] = c; }
   void clear()
   { bufpos = -1; buffer[0] = '\0'; }
   operator std::string()
   { buffer[bufpos + 1] = '\0'; return std::string(buffer); }
   bool empty()
   { return bufpos == -1; }
};

///////////////////////////////////////////////////

/////////////////////////////////////////////

// Operator and function names

struct SalaFuncLabel
{
   SalaObj::Func func;
   std::string name;
   std::string desc;
   SalaFuncLabel(SalaObj::Func f = SalaObj::S_FNULL, const std::string& str = std::string(), const std::string& des = std::string()) {
      func = f; name = str; desc = des;
   }
};

struct SalaMemberFuncLabel : public SalaFuncLabel
{
   SalaObj::Type type;
   SalaMemberFuncLabel(SalaObj::Type t = SalaObj::S_NONE, SalaObj::Func f = SalaObj::S_FNULL, const std::string& str = std::string(), const std::string& des = std::string()) {
      type = t; func = f; name = str; desc = des;
   }
};


/////////////////////////////////////////////////////////////////////////////////////

}

#endif
