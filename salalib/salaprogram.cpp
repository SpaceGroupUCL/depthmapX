// sala - a component of the depthmapX - spatial network analysis platform
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

// salaprogram.cpp - a component of the depthmapX - spatial network analysis platform
// SalaScripting language


/////////////////////////////////////////////////////////////////////////

// SalaScripting language

// A "Pythonesque" language, which is pre-interpretted, and thus generally
// should run fairly fast

// The class implementation is very much hardcoded for built-in classes,
// so user defined classes will be difficult to implement
// (but would you really want classes in an inbuilt scripting language?! -- I guess some people would)

// User defined functions are not included yet, but should be fairly easy using a global function stack
// alongside the global variable stack

#include <math.h>
#include <float.h>
#include <time.h>

#include <salalib/mgraph.h>
#include <salalib/ngraph.h>
#include <salalib/salaprogram.h>

///////////////////////////////////////////////////////////////////////////////////////////////

// Assign and list access rather incongruently in math ops, but never mind:

bool g_sala_loaded = false;

prefvec<SalaFuncLabel> g_sala_math_ops;
prefvec<SalaFuncLabel> g_sala_comp_ops;
prefvec<SalaFuncLabel> g_sala_logical_ops;
prefvec<SalaFuncLabel> g_sala_global_funcs;
prefvec<SalaMemberFuncLabel> g_sala_member_funcs;

void loadSalaProgram()
{
	// math ops
	g_sala_math_ops.push_back( SalaFuncLabel( SalaObj::S_ADD, "+", "add" ) );
	g_sala_math_ops.push_back( SalaFuncLabel( SalaObj::S_SUBTRACT, "-", "subtract" ) );
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_MINUS, "-", "negative" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_PLUS, "+", "positive" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_MULTIPLY, "*", "multiply" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_DIVIDE, "/", "divide" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_MODULO, "%", "modulo" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_POWER, "^", "power" ));
	g_sala_math_ops.push_back(    SalaFuncLabel( SalaObj::S_ASSIGN, "=", "assignment" ));
	g_sala_math_ops.push_back(SalaFuncLabel( SalaObj::S_LIST_ACCESS, "[]", "list access" )); // list access included even though not parsed directly like this

	// comp ops
   g_sala_comp_ops.push_back( SalaFuncLabel( SalaObj::S_GT, ">", "greater than" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_LT, "<", "less than" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_GEQ, ">=", "greater than or equal to" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_LEQ, "<=", "less than or equal to" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_NEQ, "!=", "not equal to" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_EQ, "==", "equal to" ));
   g_sala_comp_ops.push_back(SalaFuncLabel( SalaObj::S_IS, "is", "is the same object as" ));

   // logical ops
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_NOT, "not", "logical not" ));
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_NOT, "!", "logical not" ));
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_AND, "and", "logical and" ));
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_AND, "&&", "logical and" ));
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_OR, "or", "logical or" ));
   g_sala_logical_ops.push_back(SalaFuncLabel( SalaObj::S_OR, "||", "logical or" ));

   // global functions
	g_sala_global_funcs.push_back(   SalaFuncLabel( SalaObj::S_SQRT, "sqrt", "square root" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_LOG, "log", "log base 10" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_LN, "ln", "natural logarithm" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_RAND, "random", "random number (0.0 to 1.0)" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_SIN, "sin", "sine" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_COS, "cos", "cosine" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_TAN, "tan", "tangent" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_ASIN, "asin", "inverse sine" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_ACOS, "acos", "inverse cosine" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_ATAN, "atan", "inverse tangent" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_LEN, "len", "array or string length" ));
   g_sala_global_funcs.push_back(SalaFuncLabel( SalaObj::S_RANGE, "range", "set of integers" ));

   // member functions
	g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_LIST, SalaObj::S_FAPPEND, "append", "append item" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_LIST, SalaObj::S_FEXTEND, "extend", "extend by list" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_LIST, SalaObj::S_FPOP, "pop", "pop (last) item" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_LIST, SalaObj::S_FCLEAR, "clear", "clear contents" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_GRAPHOBJ, SalaObj::S_FVALUE, "value", "get attribute value" ));
	g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_GRAPHOBJ, SalaObj::S_FSETVALUE, "setvalue", "set attribute value" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_GRAPHOBJ, SalaObj::S_FMARK, "mark", "get node mark" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_GRAPHOBJ, SalaObj::S_FSETMARK, "setmark", "set node mark" ));
   g_sala_member_funcs.push_back(SalaMemberFuncLabel( SalaObj::S_GRAPHOBJ, SalaObj::S_FCONNECTIONS, "connections", "get list of connections" ));

	g_sala_loaded = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

SalaProgram::SalaProgram(SalaObj context)
{
	if (!g_sala_loaded) {
	   loadSalaProgram();
	}

   // col is used when run in update mode, it does not form part of the program:
   m_col = -1;
   m_thisobj = context;
}

SalaProgram::~SalaProgram()
{
}

// use istrstream to make an istream from a string:
// istrstream file(char *);

bool SalaProgram::parse(std::istream& program)
{
   m_var_stack.clear();
   m_error_stack.clear();

   // this ensures wipe of any pre-existing variables in the global context:
   m_root_command = SalaCommand(this,NULL,-1,SalaCommand::SC_ROOT);

   int line = 0;

   SalaCommand *parent = &m_root_command;

   while (!program.eof()) {

      // the problem with a language being "Pythonesque" is that the "end" of 
      // any control is implicit through the amount of indentation
      // Thus, the parser eats the white space, only handing of control
      // to a function when it is ready to parse, and knows its parent
      int indent = 0;
      bool endloop = false;
      while (!endloop) {
         char ch = program.peek();
         switch (ch) {
            case ' ':
               indent++; break;
            case 13:
               break; // ignore
            case '\n':
               line++; indent = 0; break;
            case '#':
               // hit comment, read to end of line:
               while (ch != EOF && ch != '\n') 
                  { program.get(); ch = program.peek(); }
               line++;
               break;
            case '\\':
               // hit line continuation, ignore everything after it:
               while (ch != EOF && ch != '\n') 
                  { program.get(); ch = program.peek(); }
               line++;
               break;
            case EOF:
               program.get(); // actually shift onto the eof character
               endloop = true;
               break;
            default:
               endloop = true;
               break;
            }
            if (!endloop) {
               program.get();
            }
      }

      // okay, we now know indent level, and we are ready to parse:
      if (!program.eof()) { 
         while (indent <= parent->m_indent) {
            parent = parent->m_parent;
         }

         parent->m_children.push_back(SalaCommand(this,parent,indent));

         SalaCommand &thiscommand = parent->m_children.tail();

         try {
            line = thiscommand.parse(program,line);
         }
         catch (SalaError e) {
            if (e.lineno == -1)
               e.lineno = line;
            m_error_stack.push_back(e);
            return false;
         }

         // sort out commands capable of having children:
         if (thiscommand.m_command == SalaCommand::SC_FOR || thiscommand.m_command == SalaCommand::SC_IF || thiscommand.m_command == SalaCommand::SC_WHILE) {
            parent = &thiscommand;
         }
         else if (thiscommand.m_command == SalaCommand::SC_ELSE) {
            if (parent->m_children.size() < 2) {
               m_error_stack.push_back(SalaError("'Else' must be preceded by an 'if','for' or 'while'", thiscommand.m_line));
               return false;
            }
            int command = parent->m_children[parent->m_children.size()-2].m_command;
            if (command != SalaCommand::SC_IF && command != SalaCommand::SC_ELIF && command != SalaCommand::SC_FOR && command != SalaCommand::SC_WHILE) {
               m_error_stack.push_back(SalaError("'Else' must be preceded by an 'if','for' or 'while'", thiscommand.m_line));
               return false;
            }
            parent = &thiscommand;
         }
         else if (thiscommand.m_command == SalaCommand::SC_ELIF) {
            if (parent->m_children.size() < 2) {
               m_error_stack.push_back(SalaError("'Elif' must be preceded by an 'if' condition", thiscommand.m_line));
               return false;
            }
            int command = parent->m_children[parent->m_children.size()-2].m_command;
            if (command != SalaCommand::SC_IF && command != SalaCommand::SC_ELIF) {
               m_error_stack.push_back(SalaError("'Elif' must be preceded by an 'if' condition", thiscommand.m_line));
               return false;
            }
            parent = &thiscommand;
         }
      }
   }

   // do a quick check that all 'for', 'if' and 'elif' have children:
   // TO DO!

   return true;
}

SalaObj SalaProgram::evaluate()
{
   for (size_t i = 0; i < m_var_stack.size(); i++) {
      // uninitialise all variables:
      m_var_stack[i].uninit();
   }
   m_marked = false;

   // run the program
   SalaObj obj;
   bool ret = false, ifhandled = false;
   m_root_command.evaluate(obj,ret,ifhandled);

   // clear marks if they've been used:
   if (m_marked) {
      AttributeTable *table = m_thisobj.getTable();
      for (int i = 0; i < table->getRowCount(); i++) {
	 // Quick mod - TV
#if defined(_WIN32)	 
         table->setMark(i,SalaObj());
#else
	 SalaObj objTmp = SalaObj();
         table->setMark(i,objTmp);
#endif         
      }
      m_marked = false;
   }

   return obj;
}

// this function is called by depthmapX to run a script to update a column
// the operation is on a single node / row of the database combination

bool SalaProgram::runupdate(int col, const std::set<int> &selset)
{
   AttributeTable *table = m_thisobj.getTable();
   bool pointmap = (m_thisobj.type & SalaObj::S_POINTMAP) ? true : false;
   //
   // note: reference, will change object directly, which is important for commands running the program
   int& row = m_thisobj.data.graph.node;
   m_col = col;
   if (selset.size()) {
      for (auto& sel: selset) {
         row = sel;  // *** NB! selsets for vga store pixelrefs keys, *all* others use rowids directly ***
         try {
            SalaObj val = evaluate();
            float v = (float) val.toDouble();   // note, toDouble will type check and throw if there's a problem
            // Quick mod - TV
#if defined(_WIN32)            
            if (!_finite(v)) {
#else
            if (!finite(v)) {
#endif            
               v = -1.0f;
            }
            table->changeValue(pointmap ? table->getRowid(row) : row,m_col,v);
         }
         catch (SalaError e) {
            // error
            m_error_stack.push_back(e);
            return false;
         }
      }
   }
   else {
      for (int i = 0; i < table->getRowCount(); i++) {
         row = pointmap ? table->getRowKey(i) : i; // *** NB! selsets for vga store pixelrefs keys, *all* others use rowids directly ***
         try {
            SalaObj val = evaluate();
            float v = (float) val.toDouble();   // note, toDouble will type check and throw if there's a problem
            // Quick mod - TV
#if defined(_WIN32)            
            if (!_finite(v)) {
#else
            if (!finite(v)) {
#endif            
               v = -1.0f;
            }
            table->changeValue(i,m_col,v);
         }
         catch (SalaError e) {
            // error
            m_error_stack.push_back(e);
            return false;
         }
      }
   }
   return true;
}

// this function is called by depthmapX to run a script to select values
// the operation is on a single node / row of the database combination

bool SalaProgram::runselect(std::vector<int> &selsetout, const std::set<int>& selsetin)
{
   AttributeTable *table = m_thisobj.getTable();
   bool pointmap = (m_thisobj.type & SalaObj::S_POINTMAP) ? true : false;
   //
   int& row = m_thisobj.data.graph.node;
   if (selsetin.size()) {
      for (auto& sel: selsetin) {
         row = sel;  // *** NB! selsets for vga store pixelrefs keys, *all* others use rowids directly ***
         try {
            SalaObj val = evaluate();
            bool v = val.toBool();   // note, toBool will type check and throw if there's a problem
            if (v) {
               selsetout.push_back(sel);
            }
         }
         catch (SalaError e) {
            // error
            m_error_stack.push_back(e);
            return false;
         }
      }
   }
   else {
      for (int i = 0; i < table->getRowCount(); i++) {
         row = pointmap ? table->getRowKey(i) : i; // *** NB! selsets for vga store pixelrefs keys, *all* others use rowids directly ***
         try {
            SalaObj val = evaluate();
            bool v = val.toBool();   // note, toBool will type check and throw if there's a problem
            if (v) {
               selsetout.push_back(row);
            }
         }
         catch (SalaError e) {
            // error
            m_error_stack.push_back(e);
            return false;
         }
      }
   }
   return true;
}

std::string SalaProgram::getLastErrorMessage() const
{ 
   const SalaError& error = m_error_stack.tail();
   if (error.lineno == -1) {
      return error.message; 
   }
   else {
      return error.message + " on line " + dXstring::formatString(error.lineno+1,"%d");
   }
}

////////////////////////////////////////////////////////////////////////////

SalaCommand::SalaCommand(SalaProgram *program, SalaCommand *parent, int indent, Command command)
{
   m_program = program;
   m_parent = parent;
   m_indent = indent;
   m_command = command;
   m_line = 0;
}

int SalaCommand::parse(std::istream& program, int line)
{
   m_func_stack.clear();
   m_eval_stack.clear();
   m_var_names.clear();

   m_command = SC_NONE;

   // useful to know which line the command starts on for debugging purposes
   m_line = line;

   int last = SP_FUNCTION;
   bool endloop = false;
   bool overridecache = false;
   bool firstword = true;
   SalaBuffer buffer;
   char cache = ' ';
   //
   while (!endloop && !program.eof()) {
      char alpha = program.get();
      switch (alpha) {
      // string constant
      case '\"': case '\'': // variants: either delimit with single or double quotes
         if (!buffer.empty()) {
            decode(buffer);
            buffer.clear();
         }
         {
            char delim = alpha;
            char beta = program.peek();
            while (beta != EOF && beta != '\n' && (beta != delim || alpha == '\\')) {
               alpha = program.get();
               beta = program.peek();
               buffer.add(alpha);
            }
            if (beta == EOF || beta == '\n') {
               throw SalaError("No closing quote",m_line);
            }
            else {
               program.get(); // take off closing quote and discard
            }
            // add even if the string constant is empty:
            m_eval_stack.push_back( std::string(buffer) );
            buffer.clear();
            last = SP_DATA;
         }
         break;
      // operator stack
      case '+': case '-':
         if (!buffer.empty()) {
            last = decode(buffer);
            if (last & SP_NUMBER && cache == 'e') {
               // check for 9.999e+99...
               // decode will handle later:
               buffer.add(alpha);
               break;
            }
            // otherwise handled, clear the buffer:
            buffer.clear();
         }
         if (last == SP_FUNCTION || last == SP_COMMAND) {
            pushFunc( alpha == '+' ? SalaObj(SalaObj::S_PLUS) : SalaObj(SalaObj::S_MINUS) );
         }
         else {
            pushFunc( alpha == '+' ? SalaObj(SalaObj::S_ADD) : SalaObj(SalaObj::S_SUBTRACT) );
         }
         last = SP_FUNCTION;
         break;
      case '=':
         if (!buffer.empty()) {
            // n.b., this will catch '>=', '<=', '==' and '!='
            if (strchr("><=!",cache) != NULL) {
               buffer.add(alpha);
               last = decode(buffer);
               buffer.clear();
               overridecache = true;
               // handled next step (see default clause below)
               break;
            }
            else {
               last = decode(buffer);
               buffer.clear();
            }
         }
         buffer.add(alpha); // <- '=' decoded later
         break;
      case '!': case '<': case '>':
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         // note: this looks a little odd, simply adding to the buffer, but these
         // are handled by the default function next step if still hanging on the buffer
         buffer.add(alpha);
         break;
      case '/': case '*':  case '%':  case '^':
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         last = decode(std::string(1,alpha));
         break;
      case '(':
         // note: the opening bracket forms a function
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         if (last == SP_DATA) {
            // whatever that just went onto the eval stack, the user thought it was a function...
            // alert them: 
            throw SalaError(m_last_string + " is not a known function name", m_line);
            // (in the future, we may well want to transfer an object hashed function name to the func stack instead)
         }
         else if (last == SP_NUMBER) {
            throw SalaError("Cannot treat a number as if it were a function", m_line);
         }
         // check for pair of open / close brackets: () or (  ) -- this is a null value
         {
            char beta = program.peek();
            while (beta != EOF && beta == ' ') {
               alpha = program.get();
               beta = program.peek();
            }
            if (beta == ')') {
               alpha = program.get();
               m_eval_stack.push_back(SalaObj());
               last = SP_DATA;
            }
            else {
               pushFunc( SalaObj::S_OPEN_BRACKET );
               last = SP_FUNCTION;
            }
         }
         break;
      case ')':
         // note: the closing bracket forms a data packet:
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         pushFunc( SalaObj::S_CLOSE_BRACKET );
         last = SP_DATA;
         break;
      case '[':
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         // check for pair of open / close brackets: [] or [  ] -- this is a null value or empty list depending on context
         {
            char beta = program.peek();
            while (beta != EOF && beta == ' ') {
               alpha = program.get();
               beta = program.peek();
            }
            if (beta == ']') {
               alpha = program.get();
               if (last == SP_DATA) {
                  throw SalaError("Accessor operator ('[]') requires a parameter", m_line);
               }
               else {
                  // put an empty list on the stack
                  m_eval_stack.push_back( SalaObj(SalaObj::S_CONST_LIST, 0) );
               }
               last = SP_DATA;
            }
            else {
               if (last == SP_DATA) {
                  // list accessor function
                  pushFunc( SalaObj::S_LIST_ACCESS );
                  pushFunc( SalaObj::S_OPEN_SQR_BRACKET_ACCESS );
               }
               else {
                  // making an list...
                  pushFunc( SalaObj::S_OPEN_SQR_BRACKET_LIST );
               }
               last = SP_FUNCTION;
            }
         }
         break;
      case ']':
         // note: the closing bracket forms a data packet:
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         pushFunc( SalaObj::S_CLOSE_SQR_BRACKET );
         last = SP_DATA;
         break;
      case ',':
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         pushFunc( SalaObj::S_COMMA );
         last = SP_FUNCTION;
         break;
      case ':':
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         // end of command (def, if, else, elif and for)
         if (m_command == SC_FOR || m_command == SC_WHILE || m_command == SC_IF || m_command == SC_ELIF || m_command == SC_ELSE) {
            bool commentfound = false;
            alpha = program.get();
            while (!program.eof() && alpha != '\n') {
               // continue to end of line, only comments allowed though!
               if (!commentfound) {
                  if (alpha == '#') {
                     commentfound = true;
                  }
                  else if (alpha != ' ' && alpha != 13) { // 13 ignored, as it appears \n is 10 in this stream... (so 13,10 can be found)
                     throw SalaError("'For', 'if', 'else', etc cannot have execution part on same line; insert a new line after ':'", m_line);
                  }
               }
               alpha = program.get();
            }
            line++;
            endloop = true;
         }
         else {
            throw SalaError("Unexpected colon ':' in expression",m_line);
         }
         break;
      // end of line:
      case '\\':
         // hit line continuation, read to end of line:
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         while (!program.eof() && program.get() != '\n');
         // note, end loop is not set, this is a continuation character
         line++; // line is incremented, although it this command will still start on the original line
         break;
      case '#':
         // loop through until hit \n or end
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         while (!program.eof() && program.get() != '\n');
         line++;  // should have hit a line end (or if it's end of file, it doesn't matter)
         endloop = true;
         break;
      case '\n':
         // force end of command parse:
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         line++;  // hit a line end
         endloop = true;
         break;
      case ' ':
         // white space: read word
         if (!buffer.empty()) {
            last = decode(buffer);
            buffer.clear();
         }
         break;
      case '.':
         // currently handled inelegantly through decode for either number (1.002) or member access (blah.x())
         buffer.add('.');
         break;
      case '\t':
         throw SalaError("Tab character found: please use only spaces to indent lines",m_line);
         break;
      default:
         if (strchr("<>=!",cache)) {
            // >, <, = and ! are held as next step operators
            last = decode(buffer);
            buffer.clear();
         }
         if (alpha != EOF && alpha != 13) {  // 13 ignored, as it appears \n is 10 in this stream...
            if (!isalphanum_(alpha) && alpha != '&' && alpha != '|') { // include & and | for and and or
               throw SalaError("Unrecognised symbol ('" + std::string(1,alpha) + "')",m_line);
            }
            buffer.add(alpha);
         }
         break;
      }
      if (overridecache) {
         cache = ' ';
         overridecache = false;
      }
      else {
         cache = alpha;
      }
      if (last == SP_COMMAND) {
         if (m_command == SC_FOR) {
            // check the name of the for variable:
            alpha = program.get();
            while (alpha == ' ') {
               alpha = program.get();
            }
            if (!isalpha_(alpha)) {
               throw SalaError("'For' command expecting variable name",m_line);
            }
            while (isalphanum_(alpha)) {
               buffer.add(alpha);
               alpha = program.get();
            }
            if (alpha != ' ') {
               throw SalaError("Command expecting syntax 'for xyz in'...",m_line);
            }
            // add the for iterator variable:
            m_program->m_var_stack.push_back(SalaObj());
            int x = m_program->m_var_stack.size() - 1;
            m_var_names.insert(std::make_pair(buffer,x));
            m_for_iter = SalaObj( SalaObj::S_VAR, x);
            // now check for 'in'
            while (alpha == ' ') {
               alpha = program.get();
            }
            if (alpha != 'i' || program.get() != 'n') {
               throw SalaError("Command expecting syntax 'for xyz in'...",m_line);
            }
         }
         last = SP_FUNCTION;
      }
   }
   if (!buffer.empty()) {
      decode(buffer);
      buffer.clear();
      last = SP_DATA;
   }
   // push remaining functions onto eval stack:
   while (m_func_stack.size()) {
      if (m_func_stack.tail().type & SalaObj::S_BRACKET) {
         throw SalaError("Unmatched brackets",m_line);
      }
      m_eval_stack.push_back(m_func_stack.tail());
      m_func_stack.pop_back();
   }

   if (m_eval_stack.size() == 0 && m_command != SC_ELSE) { // note, else is by definition empty
      throw SalaError("Partial or missing command",m_line);
   }
   return line;
}

int SalaCommand::decode(std::string string)   // string copied as makelower applied
{
   // ideally, some form of hashing the string should be performed so that
   // functions can be found quicker than a long list of "else ifs"
   int retvar = SP_NONE;
   dXstring::toLower(string);

   if (m_command == SC_NONE) {
      if (string == "return") {
         m_command = SC_RETURN;
         retvar = SP_COMMAND;
      }
      else if (string == "for") {
         m_command = SC_FOR;  // n.b. will still need a variable name and "in": for x in ...
         retvar = SP_COMMAND;
      }
      else if (string == "while") {
         m_command = SC_WHILE;
         retvar = SP_COMMAND;
      }
      else if (string == "if") {
         m_command = SC_IF;
         retvar = SP_COMMAND;
      }
      else if (string == "elif") {
         m_command = SC_ELIF;
         retvar = SP_COMMAND;
      }
      else if (string == "else") {
         m_command = SC_ELSE;
         retvar = SP_COMMAND;
      }
   }
   if (retvar == SP_COMMAND) {
      // 
      m_last_string = string; // make a copy for debugging purposes
      return retvar;
   }

   // numeric constant
   if (isdigit(string[0]) || (string.length() > 1 && string[0] == '.' && isdigit(string[1]))) {
      if (string[string.length()-1] == 'e') {
         // handle later... at the moment we have hit + or - in 9.999e+99 or 9.999e-99
         m_last_string = string; // make a copy for debugging purposes
         return SP_NUMBER;
      }
      if (string.find_first_of('.') != std::string::npos || string.find_first_of('e') != std::string::npos) {
         m_eval_stack.push_back( atof(string.c_str()) );
      }
      else {
         m_eval_stack.push_back( atoi(string.c_str()) );
      }
      retvar = SP_NUMBER;
   }
   // this is a different 'e' to the 'e' above -> natural logarithm
   else if (string == "e") {
      m_eval_stack.push_back( 2.7182818284590452353602874713527 );
      retvar = SP_NUMBER;
   }
   else if (string == "pi") {
      m_eval_stack.push_back( 3.1415926535897932384626433832795 );
      retvar = SP_NUMBER;
   }
   // boolean constants
   else if (string == "true") {
      m_eval_stack.push_back( bool(true) );
      retvar = SP_NUMBER;
   }
   else if (string == "false") {
      m_eval_stack.push_back( bool(false) );
      retvar = SP_NUMBER;
   }
   // this
   else if (string == "this") {
      m_eval_stack.push_back( SalaObj(SalaObj::S_THIS) );
      retvar = SP_DATA;
   }
   else if (string == "none") {
      m_eval_stack.push_back(SalaObj());
      retvar = SP_DATA;
   }
   else {
      // everything else should be in one of the operator / func lists:
      size_t i;
      if (retvar == SP_NONE) {
         // note, math ops include assignment
         for (i = 0 ; i < g_sala_math_ops.size(); i++) {
            if (string == g_sala_math_ops[i].name) {
               pushFunc( g_sala_math_ops[i].func );
               retvar = SP_FUNCTION;
               break;
            }
         }
      }
      if (retvar == SP_NONE) {
         for (i = 0 ; i < g_sala_comp_ops.size(); i++) {
            if (string == g_sala_comp_ops[i].name) {
               pushFunc( g_sala_comp_ops[i].func );
               retvar = SP_FUNCTION;
               break;
            }
         }
      }
      if (retvar == SP_NONE) {
         for (i = 0 ; i < g_sala_logical_ops.size(); i++) {
            if (string == g_sala_logical_ops[i].name) {
               pushFunc( g_sala_logical_ops[i].func );
               retvar = SP_FUNCTION;
               break;
            }
         }
      }
      if (retvar == SP_NONE) {
         for (i = 0 ; i < g_sala_global_funcs.size(); i++) {
            if (string == g_sala_global_funcs[i].name) {
               pushFunc( g_sala_global_funcs[i].func );
               retvar = SP_FUNCTION;
            }
         }
      }
   }

   if (retvar == SP_NONE) {
      size_t n = string.find_first_of(".");
      if (n != std::string::npos) {
         if (n > 0) {
            decode(string.substr(0,n));
         }
         if (decode_member(string.substr(n+1),false) == SP_NONE) {
            throw SalaError("There is no known member function called " + string.substr(n+1),m_line);
         }
         retvar = SP_FUNCTION;
      }
      else {
         // see if it's a member function of 'this':
         retvar = decode_member(string,true);

         if (retvar == SP_NONE) {
            // see if it exists in the variable stack (walk up scope)
            SalaCommand *parent = m_parent;
            auto n = parent->m_var_names.end();
            int x = -1;
            while (parent != NULL && n == parent->m_var_names.end()) {
               n = parent->m_var_names.find(string);
               if (n != parent->m_var_names.end()) {
                  x = n->second;
               }
               parent = parent->m_parent;
            }
            if (x != -1) {
               m_eval_stack.push_back( SalaObj( SalaObj::S_VAR, x) );
               retvar = SP_DATA;
            }
            else {
               m_program->m_var_stack.push_back(SalaObj());
               x = m_program->m_var_stack.size() - 1;
               // note: attach simply to your m_parent, not parent variable, which has walked up the stack
               m_parent->m_var_names.insert(std::make_pair(string,x));
               m_eval_stack.push_back( SalaObj( SalaObj::S_VAR, x) );
               retvar = SP_DATA;
            }
         }
      }
   }

   if (retvar == SP_NONE) {
      // should never reach this point
      throw SalaError("There is no known function or variable called " + string,m_line);
   }


   if (m_command == SC_NONE) {
      m_command = SC_EXPR;
   }

   m_last_string = string; // make a copy for debugging purposes

   return retvar;
}

// note, thisobj not usually known (type S_NALL),
// but, depending where SalaScript is called from, it may be:
//    a graph node / table row (for "select by query" and "edit connections")
//    a map (not yet implemented, but intended for scripting agents)

int SalaCommand::decode_member(const std::string& string, bool apply_to_this)
{
   int retvar = SP_NONE;

   // note, all hardcoded for built in classes:
   // string classes:
   for (size_t i = 0; i < g_sala_member_funcs.size(); i++) {
      // note '&' in the type -- essentially allows for inheritance between objects (tuple is type of list, etc)
      if (!apply_to_this || (m_program->m_thisobj.type & g_sala_member_funcs[i].type) != 0) {
         if (string == g_sala_member_funcs[i].name) {
            pushFunc( g_sala_member_funcs[i].func );
            retvar = SP_FUNCTION;
            break;
         }
      }
   }
   if (retvar == SP_FUNCTION && apply_to_this) {
      m_eval_stack.push_back(SalaObj(SalaObj::S_THIS));
   }
   return retvar;
}

void SalaCommand::pushFunc(const SalaObj& func)
{
   // note comma is part of the "Bracket" class of things:
   if (func.type & SalaObj::S_BRACKET) {
      if (func.type == SalaObj::S_CLOSE_BRACKET) {
         while (m_func_stack.size() && m_func_stack.tail().type != SalaObj::S_OPEN_BRACKET) {
            m_eval_stack.push_back(m_func_stack.tail());
            m_func_stack.pop_back();
         }
         if (m_func_stack.size()) {
            // don't necessarily pop it... if it's a group marker, we want to hang onto it:
            if (m_func_stack.tail().data.count > 1) {
               m_func_stack.tail().type = SalaObj::S_CONST_TUPLE;
               m_eval_stack.push_back(m_func_stack.tail());
            }
            m_func_stack.pop_back(); // remove opening bracket
         }
      }
      else if (func.type == SalaObj::S_CLOSE_SQR_BRACKET) {
         while (m_func_stack.size() && (m_func_stack.tail().type & SalaObj::S_OPEN_SQR_BRACKET) == 0) {
            m_eval_stack.push_back(m_func_stack.tail());
            m_func_stack.pop_back();
         }
         if (m_func_stack.size()) {
            // don't pop it, always make a list from a make list command, even if it's only one item long:
            if (m_func_stack.tail().type == SalaObj::S_OPEN_SQR_BRACKET_LIST || m_func_stack.tail().data.count > 1) {
               m_func_stack.tail().type = SalaObj::S_CONST_LIST;
               m_eval_stack.push_back(m_func_stack.tail());
            }
            m_func_stack.pop_back();
         }
      }
      else if (func.type == SalaObj::S_COMMA) {
         // go and increment your associated group / list
         while (m_func_stack.size() && m_func_stack.tail().type != SalaObj::S_OPEN_BRACKET && (m_func_stack.tail().type & SalaObj::S_OPEN_SQR_BRACKET) == 0) {
            m_eval_stack.push_back(m_func_stack.tail());
            m_func_stack.pop_back();
         }
         if (m_func_stack.size()) {
            m_func_stack.tail().data.count++;
         }
      }      
      else {
         m_func_stack.push_back( func );
      }
   }
   else if (!m_func_stack.size() || func.precedence() > m_func_stack.tail().precedence()) {    // original: >
      m_func_stack.push_back(func);
   }
   else {
      while (m_func_stack.size() && func.precedence() <= m_func_stack.tail().precedence()) {     // original <=
         m_eval_stack.push_back(m_func_stack.tail());
         m_func_stack.pop_back();
      }
      m_func_stack.push_back(func);
   }
}

void SalaCommand::evaluate(SalaObj& obj, bool& ret, bool& ifhandled)
{
   int begin = m_eval_stack.size()-1;
   SalaObj *p_obj = NULL;
   switch (m_command) {
   case SC_EXPR:
      obj = evaluate(begin,p_obj);
      break;
   case SC_RETURN:
      ret = true;
      obj = evaluate(begin,p_obj);
      break;
   case SC_ROOT:
      {
         for (size_t i = 0; i < m_children.size(); i++) {
            m_children[i].evaluate(obj,ret,ifhandled);
            if (ret)
               break;
         }
      }
      break;
   case SC_IF:
      {
         SalaObj test = evaluate(begin,p_obj);
         if (test.toBool() == true) {
            for (size_t i = 0; i < m_children.size(); i++) {
               m_children[i].evaluate(obj,ret,ifhandled);
               if (ret)
                  break;
            }
            ifhandled = true;
         }
         else {
            ifhandled = false;
         }
      }
      break;
   case SC_ELIF:
      if (!ifhandled) {
         SalaObj test = evaluate(begin,p_obj);
         if (test.toBool() == true) {
            for (size_t i = 0; i < m_children.size(); i++) {
               m_children[i].evaluate(obj,ret,ifhandled);
               if (ret)
                  break;
            }
            ifhandled = true;
         }
      }
      break;
   case SC_ELSE:
      if (!ifhandled) {
         for (size_t i = 0; i < m_children.size(); i++) {
            m_children[i].evaluate(obj,ret,ifhandled);
            if (ret)
               break;
         }
      }
      break;
   case SC_FOR:
      {
         // eventually I'd like to do this with generators / iterators rather than constructing a list each time
         SalaObj list = evaluate(begin,p_obj);
         if (list.type == SalaObj::S_LIST) {
            int len = list.data.list.list->size();
            if (len != 0) {
               for (int i = 0; i < len; i++) {
                  // reset all my stack (actually, all parent functions should do this!)
                  for (auto varName: m_var_names) {
                     m_program->m_var_stack[varName.second].uninit();
                  }
                  m_program->m_var_stack[m_for_iter.data.var] = list.data.list.list->at(i);
                  for (size_t k = 0; k < m_children.size(); k++) {
                     m_children[k].evaluate(obj,ret,ifhandled);
                     if (ret)
                        break;
                  }
                  if (ret) 
                     break;
               }
               ifhandled = true;
            }
            else {
               ifhandled = false;
            }
         }
         else {
            ifhandled = false;
         }
      }
      break;
   case SC_WHILE:
      {
         int counter = 0;
         while (evaluate(begin,p_obj).toBool()) {
            for (size_t k = 0; k < m_children.size(); k++) {
               m_children[k].evaluate(obj,ret,ifhandled);
               if (ret)
                  break;
            }
            if (ret)
               break;
            if (++counter == 0x04000000) { // <- an arbitrary big number
               throw SalaError("Infinite loop",m_line);
            }
            begin = m_eval_stack.size()-1;
         }
         if (counter) {
            ifhandled = true;
         }
         else {
            ifhandled = false;
         }
      }
      break;
   default:
      throw SalaError("Unknown command",m_line);
      break;
   }
}

SalaObj SalaCommand::evaluate(int& pointer, SalaObj* &p_obj)
{
   if (pointer < 0) {
      throw SalaError("Missing argument",m_line);
   }
   register SalaObj data = m_eval_stack[pointer];
   pointer--;
   if (data.type == SalaObj::S_FUNCTION) {
      SalaObj::Func func = data.data.func;
      int group = (func & SalaObj::S_GROUP);
      if (group == SalaObj::S_MATH_OPS) {
         try {
            switch (func) {
            case SalaObj::S_ADD:
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) + evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    SalaObj tmp2 = evaluate(pointer,p_obj);
            	    data = tmp1 + tmp2;
               }
#endif               
               break;
            case SalaObj::S_SUBTRACT:
    	       // Quick mod - TV
#if defined(_WIN32)    	       
               data = evaluate(pointer,p_obj) - evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    SalaObj tmp2 = evaluate(pointer,p_obj);
            	    data = tmp1 - tmp2;
               }
#endif               
               break;
            case SalaObj::S_PLUS:
               data = evaluate(pointer,p_obj);  // just ignore it
               break;
            case SalaObj::S_MINUS:
    	       // Quick mod - TV
#if defined(_WIN32)    	       
               data = -evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
            	    data = -tmp1;
               }
#endif               
               break;
            case SalaObj::S_MULTIPLY:
    	       // Quick mod - TV
#if defined(_WIN32)    	       
               data = evaluate(pointer,p_obj) * evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    SalaObj tmp2 = evaluate(pointer,p_obj);
            	    data = tmp1 * tmp2;
               }
#endif               
               break;
            case SalaObj::S_DIVIDE:
    	       // Quick mod - TV
#if defined(_WIN32)    	       
               data = evaluate(pointer,p_obj) / evaluate(pointer,p_obj);
#else
	       {
                SalaObj tmp1 = evaluate(pointer,p_obj);
                SalaObj tmp2 = evaluate(pointer,p_obj);
                    data = tmp2 / tmp1;
               }
#endif               
               break;
            case SalaObj::S_MODULO:
               data = evaluate(pointer,p_obj);
               
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) % data;   // reverse order
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer, p_obj);
	    	    data = tmp1 % data;
	       }
#endif               
               break;
            case SalaObj::S_POWER:
               data = evaluate(pointer,p_obj);   // reverse order
               data = pow(evaluate(pointer,p_obj).toDouble(),data.toDouble());
               break;
            case SalaObj::S_ASSIGN:
               data = evaluate(pointer,p_obj);  // reverse order
               evaluate(pointer,p_obj);
               if ((unsigned long)p_obj > 1) {
                  *p_obj = data;
               }
               else {
                  throw SalaError("Cannot assign to constant, function or none",m_line);
               }
               data = SalaObj(); // assign returns nil value
               break;
            case SalaObj::S_LIST_ACCESS:
               {
                  int x = evaluate(pointer,p_obj).toInt(); 
                  data = evaluate(pointer,p_obj);
                  if (data.type == SalaObj::S_LIST) {
                     // setting p_obj allows things above this in the stack to modify it
                     p_obj = &(data.list_at(x));
                     return *p_obj;
                  }
                  else if (data.type == SalaObj::S_STRING) {
                     // but n.b., strings cannot be modified, keep p_obj as null
                     p_obj = NULL;
                     return data.char_at(x);
                  }
                  else 
                     throw SalaError("Cannot be applied to " + data.getTypeIndefArt() + data.getTypeStr(),m_line);
               }
               break;
            }
         }
         catch (SalaError e)
         {
            // slow to go through one by one, but this is an exception...
            for (size_t i = 0; i < g_sala_math_ops.size(); i++) {
               if (g_sala_math_ops[i].func == func) {
                  e.message = "In '" + g_sala_math_ops[i].name + "' operator: " + e.message;
                  break;
               }
            }
            e.lineno = m_line; throw e;
         }
      }
      else if (group == SalaObj::S_LOGICAL_OPS) {
         try {
            switch (func) {
            case SalaObj::S_OR:
               // note: you cannot simply say evaluate(x) || evaluate(y) because if evaluate(x) is true,
               // the in-built || operator will not evaluate(y)
               // but... it's on the eval stack... it would be nice simply to pop the eval stack at
               // this point if the first half evaluates to true, thus emulating C... but it's in reverse order too!
               data = evaluate(pointer,p_obj);
               data = evaluate(pointer,p_obj).toBool() || data.toBool();
               break;
            case SalaObj::S_AND:
               data = evaluate(pointer,p_obj).toBool() && evaluate(pointer,p_obj).toBool();
               break;
            case SalaObj::S_NOT:
               data = !evaluate(pointer,p_obj).toBool();
               break;
            case SalaObj::S_EQ:
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) == evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer, p_obj);
	    	    SalaObj tmp2 = evaluate(pointer, p_obj);
	    	    data = (tmp1 == tmp2);
	       }
#endif               
               break;
            case SalaObj::S_IS:
               // Quick mod - TV
#if defined(_WIN32)               
               data = op_is(evaluate(pointer,p_obj),evaluate(pointer,p_obj));
#else
	       {
		    SalaObj tmp1 = evaluate(pointer, p_obj);
		    SalaObj tmp2 = evaluate(pointer, p_obj);
		    data = op_is(tmp1, tmp2);
	       }
#endif               
               break;
            case SalaObj::S_NEQ:
    	       // Quick mod - TV
#if defined(_WIN32)    	       
               data = evaluate(pointer,p_obj) != evaluate(pointer,p_obj);
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    SalaObj tmp2 = evaluate(pointer,p_obj);
	    	    data = (tmp1 != tmp2);
	       }
#endif               
               break;
            case SalaObj::S_GT:
               data = evaluate(pointer,p_obj);
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) > data;   // revese order
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    data = (tmp1 > data);
	       }
#endif               
               break;
            case SalaObj::S_LT:
               data = evaluate(pointer,p_obj);
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) < data;   // revese order
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    data = (tmp1 < data);
	       }
#endif               
               break;
            case SalaObj::S_GEQ:
               data = evaluate(pointer,p_obj);
               
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) >= data;   // revese order
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer,p_obj);
	    	    data = (tmp1 >= data);
	       }
#endif               
               break;
            case SalaObj::S_LEQ:
               data = evaluate(pointer,p_obj);
               
               // Quick mod - TV
#if defined(_WIN32)               
               data = evaluate(pointer,p_obj) <= data;   // revese order
#else
	       {
	    	    SalaObj tmp1 = evaluate(pointer, p_obj);
	    	    data = (tmp1 <= data);
	       }
#endif               
               break;
            }
         }
         catch (SalaError e)
         {
            // slow to go through one by one, but this is an exception...
            for (size_t i = 0; i < g_sala_logical_ops.size(); i++) {
               if (g_sala_logical_ops[i].func == func) {
                  e.message = "In '" + g_sala_logical_ops[i].name + "' operator: " + e.message;
                  break;
               }
            }
            for (size_t j = 0; j < g_sala_comp_ops.size(); j++) {
               if (g_sala_comp_ops[j].func == func) {
                  e.message = "In '" + g_sala_comp_ops[j].name + "' operator: " + e.message;
                  break;
               }
            }
            e.lineno = m_line; throw e;
         }
      }
      else if (group == SalaObj::S_GLOBAL_FUNCS) {
         try {
            switch (func) {
            case SalaObj::S_LEN:
               data = evaluate(pointer,p_obj);
               data = SalaObj( data.length() );
               break;
            case SalaObj::S_RANGE:
               data = evaluate(pointer,p_obj);
               { 
                  int len = data.length();
                  if (len != 2 && len != 3) {
                     throw SalaError("Range takes either 2 or 3 parameters",m_line);
                  }
                  int start = data.data.list.list->at(0).toInt();
                  int end = data.data.list.list->at(1).toInt();
                  int step = (len == 3) ? data.data.list.list->at(2).toInt() : 1;
                  if (step == 0) {
                     throw SalaError("Range cannot have a step of 0",m_line);
                  }
                  int listlen = (int) ceil(float(end - start) / float(step)); 
                  if (listlen <= 0) {
                     data = SalaObj( SalaObj::S_LIST );
                  }
                  else {
                     data = SalaObj( SalaObj::S_LIST, listlen );
                     for (int i = start, j = 0; i < end; i += step, j++) {
                        data.data.list.list->at(j) = i;
                     }
                  }
               }
               break;
            case SalaObj::S_SQRT:
               data = sqrt(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_LOG:
               data = log10(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_LN:
               data = ln(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_RAND:
               data = evaluate(pointer,p_obj);
               data.ensureNone();
               data = SalaObj(prandom());
               break;
            case SalaObj::S_SIN:
               data = sin(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_COS:
               data = cos(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_TAN:
               data = tan(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_ASIN:
               data = asin(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_ACOS:
               data = acos(evaluate(pointer,p_obj).toDouble());
               break;
            case SalaObj::S_ATAN:
               data = atan(evaluate(pointer,p_obj).toDouble());
               break;
            }
         }
         catch (SalaError e)
         {
            // slow to go through one by one, but this is an exception...
            for (size_t i = 0; i < g_sala_global_funcs.size(); i++) {
               if (g_sala_global_funcs[i].func == func) {
                  e.message = "In '" + g_sala_global_funcs[i].name + "' function: " + e.message;
                  break;
               }
            }
            e.lineno = m_line; throw e;
         }
      }
      else if (group == SalaObj::S_MEMBER_FUNCS) {
         try {
            SalaObj param = evaluate(pointer,p_obj);
            SalaObj obj = evaluate(pointer,p_obj);
            switch (obj.type) {
            case SalaObj::S_LIST: case SalaObj::S_TUPLE:
               switch (func) {
               case SalaObj::S_FAPPEND:
                  obj.data.list.list->push_back(param);
                  data = SalaObj(); // returns none
                  break;
               case SalaObj::S_FEXTEND:
                  if (param.type & SalaObj::S_LIST) {
                     int count = param.data.list.list->size();
                     for (int i = 0; i < count; i++) {
                        obj.data.list.list->push_back(param.data.list.list->at(i));
                     }
                  }
                  else {
                     throw SalaError("Parameter must be a list not "  + param.getTypeIndefArt() + param.getTypeStr(),m_line);
                  }
                  data = SalaObj(); // returns none
                  break;
               case SalaObj::S_FPOP:
                  if (obj.data.list.list->size() == 0) {
                     throw SalaError("List is empty", m_line);
                  }
                  if (param.type == SalaObj::S_NONE) {
                     data = obj.data.list.list->tail();
                     obj.data.list.list->pop_back();
                  }
                  else {
                     pvector<SalaObj>& list = *(obj.data.list.list);
                     int i = param.toInt();
                     if (i < 0) 
                        i += list.size();
                     if (i < 0 || i >= (int)list.size()) 
                        throw SalaError("Index out of range");
                     data = list.at(i);
                     list.remove_at(i);
                  }
                  break;
               case SalaObj::S_FCLEAR:
                  param.ensureNone();
                  obj.data.list.list->clear();
                  obj = SalaObj();
                  break;
               default:
                  throw SalaError("Not a member function of " + obj.getTypeStr(),m_line);
               }
               break;
            case SalaObj::S_SHAPEMAPOBJ: case SalaObj::S_POINTMAPOBJ:
               switch (func) {
               case SalaObj::S_FVALUE:
                  {
                     const std::string& str = param.toStringRef();
                     AttributeTable *table = obj.getTable();
                     int col = -1;
                     if (str != "Ref Number") {
                        col = table->getColumnIndex(str);
                        if (col == -1) {
                           throw SalaError(str + " is an unknown column",m_line);
                        }
                     }
                     // *** NB! vga is keyed off pixelrefs *all* others use rowids directly ***
                     // (this may seem an odd way to do it, but it speeds up connection hunting to use whichever method the map uses -- note idepthmap uses the method to access attributes)
                     data = SalaObj(table->getValue((obj.type == SalaObj::S_POINTMAPOBJ) ? table->getRowid(obj.data.graph.node) : obj.data.graph.node,col));
                  }
                  break;
               case SalaObj::S_FSETVALUE:
                  {
                     if (param.length() != 2) {
                        throw SalaError("Function takes 2 parameters");
                     }
                     const std::string& str = param.list_at(0).toStringRef();
                     float val = (float) param.list_at(1).toDouble();
                     AttributeTable *table = obj.getTable();
                     int col = -1;
                     if (str != "Ref Number") {
                        col = table->getColumnIndex(str);
                        if (col == -1) {
                           throw SalaError(str + " is an unknown column",m_line);
                        }
                     }
                     // *** NB! vga is keyed off pixelrefs *all* others use rowids directly ***
                     // (this may seem an odd way to do it, but it speeds up connection hunting to use whichever method the map uses -- note idepthmap uses the method to access attributes)
                     table->setValue((obj.type == SalaObj::S_POINTMAPOBJ) ? table->getRowid(obj.data.graph.node) : obj.data.graph.node, col, val);
                     data = SalaObj(); // returns none
                  }
                  break;
               case SalaObj::S_FCONNECTIONS:
                  {
                     data = connections(obj, param);
                  }
                  break;
               case SalaObj::S_FMARK:
                  {
                     param.ensureNone();
                     AttributeTable *table = obj.getTable();
                     data = table->getMark((obj.type == SalaObj::S_POINTMAPOBJ) ? table->getRowid(obj.data.graph.node) : obj.data.graph.node);
                  }
                  break;
               case SalaObj::S_FSETMARK:
                  {
                     AttributeTable *table = obj.getTable();
                     table->setMark((obj.type == SalaObj::S_POINTMAPOBJ) ? table->getRowid(obj.data.graph.node) : obj.data.graph.node, param);
                     m_program->m_marked = true;   // <- this tells the program to tidy up marks between executions
                     data = SalaObj(); // returns none
                  }
                  break;
               default:
                  throw SalaError("Not a member function of " + obj.getTypeStr(),m_line);
               }
               break;
            default:
               throw SalaError("Not a member function of " + obj.getTypeStr(), m_line);
            }
         }
         catch (SalaError e)
         {
            // slow to go through one by one, but this is an exception...
            for (size_t i = 0; i < g_sala_member_funcs.size(); i++) {
               if (g_sala_member_funcs[i].func == func) {
                  SalaObj type = SalaObj(g_sala_member_funcs[i].type);
                  e.message = "In " + type.getTypeStr() + " '" + g_sala_member_funcs[i].name + "' function: " + e.message;
                  break;
               }
            }
            e.lineno = m_line; throw e;
         }
      }
   }
   else if (data.type == SalaObj::S_THIS) {
      p_obj = &(m_program->m_thisobj);
      return *p_obj;
   }
   else if (data.type & SalaObj::S_VAR) {
      // retrieve value from variable stack (keeping in a variable stack means it can be reassigned dynamically)
      p_obj = &(m_program->m_var_stack[data.data.var]);
      return *p_obj;
   }
   else if (data.type & SalaObj::S_CONST_LIST) {
      // build an list from either a const tuple or const list:
      int x = data.data.count;
      data = SalaObj((data.type == SalaObj::S_CONST_LIST) ? SalaObj::S_LIST : SalaObj::S_TUPLE, x);
      for (--x; x >= 0; x--) {
         data.data.list.list->at(x) = evaluate(pointer,p_obj);  // n.b., direct access to the list
      }
   }
   p_obj = NULL;
   return data;
}

/////////////////////////////////////////////////////////////////////////////////

SalaObj SalaCommand::connections(SalaObj graphobj, SalaObj param)
{
   // now, depending on type of object, it may or may not be allowed parameters:
   // for point maps it can be none (all connections) or a bin number (0-32)
   // for segment maps it can be 'all' or 'forward' or 'back' (a string -- no parameters is excluded due to potential for errors)
   // for axial maps it must be none
   SalaObj list;
   if ((graphobj.type & SalaObj::S_MAP) == SalaObj::S_POINTMAP) {
      // point map version
      Node& node = graphobj.data.graph.map.point->getPoint(graphobj.data.graph.node).getNode();
      if (param.type == SalaObj::S_NONE) {
         int count = node.count();
         list = SalaObj( SalaObj::S_LIST, count);
         node.first();
         for (int i = 0; i < count; i++) {
            graphobj.data.graph.node = node.cursor();
            list.data.list.list->at(i) = graphobj;
            node.next();
         }
      }
      else {
         int b = param.toInt(); // note, will throw if it's not the right type
         if (b < 0 || b > 31) {
            throw SalaError("Bin must be in range 0 to 31");
         }
         Bin& bin = node.bin(b);
         int count = bin.count();
         list = SalaObj( SalaObj::S_LIST, count);
         bin.first();
         for (int i = 0; i < count; i++) {
            graphobj.data.graph.node = bin.cursor();
            list.data.list.list->at(i) = graphobj;
            bin.next();
         }
      }
   }
   else {
      const Connector& connector = graphobj.data.graph.map.shape->getConnections().at(graphobj.data.graph.node);
      int mode = Connector::CONN_ALL;
      if (graphobj.data.graph.map.shape->isSegmentMap()) {
         const std::string& str = param.toStringRef();
         if (str == "forward") {
            mode = Connector::SEG_CONN_FW;
         }
         else if (str == "back") {
            mode = Connector::SEG_CONN_BK;
         }
         else if (str == "all") {
            mode = Connector::SEG_CONN_ALL;
         }  
      }
      else {
         param.ensureNone();
      }
      int count = connector.count(mode);
      list = SalaObj( SalaObj::S_LIST, count);
      connector.first();
      for (int i = 0; i < count; i++) {
         graphobj.data.graph.node = connector.cursor(mode);
         list.data.list.list->at(i) = graphobj;
         connector.next();
      }
   }
   return list;
}

/////////////////////////////////////////////////////////////////////////////////

AttributeTable *SalaObj::getTable()
{
   if ((type & SalaObj::S_MAP) == SalaObj::S_POINTMAP) {
      return &(data.graph.map.point->getAttributeTable());
   }
   else {
      return &(data.graph.map.shape->getAttributeTable());
   }
}

int SalaObj::precedence() const
{
   int prec = 0;
   if ((type & S_BRACKET) == 0) { // preserve bracket on func stack until after close bracket, remember to strip any at end
      switch (func()) {
         case S_ASSIGN:
            prec = 1; // do absolutely last!
            break;
         case S_AND:
            prec = 2;
            break;
         case S_OR:
            prec = 3;
            break;
         case S_NOT:
            prec = 4;
            break;
         case S_EQ: case S_IS: case S_LT: case S_GT: case S_LEQ: case S_GEQ: case S_NEQ:
            prec = 5;
            break;
         case S_ADD: case S_SUBTRACT:
            prec = 6;
            break;
         case S_MULTIPLY: case S_DIVIDE: case S_MODULO:
            prec = 7;
            break;
         case S_POWER:
            prec = 8;
            break;
         default: // function -- place straight on eval stack when you meet next operator
            prec = 9;
            break;
      }
   }
   return prec;
}
