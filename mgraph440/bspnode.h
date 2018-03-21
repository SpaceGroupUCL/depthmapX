// genlib - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010 University College London, Alasdair Turner
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

#pragma once

#include "mgraph440/paftl.h"
#include "mgraph440/p2dpoly.h"

// Binary Space Partition

namespace mgraph440 {

struct BSPNode
{
public:
   enum { BSPLEFT, BSPRIGHT };
   BSPNode *left;
   BSPNode *right;
   BSPNode *parent;
   Line line;
   int m_tag;
   int m_count;
   //
public:
   BSPNode()
   { left = NULL; right = NULL; parent = NULL; m_count = 0; m_tag = -1; }
   virtual ~BSPNode()
   { if (left) delete left; left = NULL; if (right) delete right; right = NULL; }
   //
   bool isLeaf() {
      return left == NULL && right == NULL;
   }
   void make(Communicator *communicator, time_t atime, const prefvec<TaggedLine>& lines, BSPNode *par);
   int classify(const Point2f& p);
   const Line& getLine() const { return line; }
   const int getTag() const { return m_tag; }
};

}
