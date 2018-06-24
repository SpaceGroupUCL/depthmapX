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

#include "p2dpoly.h"
#include <memory>

// Binary Space Partition

struct BSPNode
{

private:
   Line m_line;
   int m_tag;

public:
   enum { BSPLEFT, BSPRIGHT };
   std::unique_ptr<BSPNode> m_left;
   std::unique_ptr<BSPNode> m_right;
   BSPNode *m_parent;

   BSPNode(BSPNode *parent = NULL) : m_left(nullptr), m_right(nullptr)
   {m_parent = parent; m_tag = -1; }
   //
   bool isLeaf() {
      return m_left == nullptr && m_right == nullptr;
   }
   int classify(const Point2f& p) {
       Point2f v0 = m_line.end() - m_line.start();
       v0.normalise();
       Point2f v1 = p - m_line.start();
       v1.normalise();
       if (det(v0,v1) >= 0) {
          return BSPLEFT;
       }
       else {
          return BSPRIGHT;
       }
   }
   const Line& getLine() const { return m_line; }
   void setLine(const Line& line) { m_line = line; }
   int getTag() const { return m_tag; }
   void setTag(const int tag) { m_tag = tag; }
};

namespace BSPTree {
void make(Communicator *communicator, time_t atime, const std::vector<TaggedLine> &lines, BSPNode *root);
int pickMidpointLine(const std::vector<TaggedLine> &lines, BSPNode *par);
std::pair<std::vector<TaggedLine>, std::vector<TaggedLine> > makeLines(Communicator *communicator,
                                                                       time_t atime,
                                                                       const std::vector<TaggedLine>& lines,
                                                                       BSPNode *base);
}
