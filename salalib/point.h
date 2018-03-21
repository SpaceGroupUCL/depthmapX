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

#pragma once

#include "genlib/p2dpoly.h"
#include "salalib/pixelref.h"
#include <map>

class Node;

class Point {
   friend class Bin;
   friend class PointMap;
   friend class MetaGraph; // <- for file conversion routines
   friend class PafAgent;
   friend class PafWalker;
public:
   enum { EMPTY = 0x0001, FILLED = 0x0002,
          BLOCKED = 0x0004, CONTEXTFILLED = 0x0008, // PARTBLOCKED = 0x0008 deprecated
          SELECTED = 0x0010, EDGE = 0x0020, MERGED = 0x0040,  // PINNED = 0x0020 deprecated
          AGENTFILLED = 0x0080, AGENTFADE = 0x0100, AGENTA = 0x0200, AGENTB = 0x0400, AGENTC = 0x0800,
          UPDATELINEADDED = 0x1000, UPDATELINEREMOVED = 0x2000, HIGHLIGHT = 0x4000,
          AUGMENTED = 0x8000 // AV TV
        };
   // note the order of these connections is important and used elsewhere:
   enum { CONNECT_E = 0x01, CONNECT_NE = 0x02, CONNECT_N = 0x04, CONNECT_NW = 0x08,
          CONNECT_W = 0x10, CONNECT_SW = 0x20, CONNECT_S = 0x40, CONNECT_SE = 0x80 };
protected:
   int m_block;   // not used, unlikely to be used, but kept for time being
   int m_state;
   int m_misc; // <- undocounter / point seen register / agent reference number, etc
   char m_grid_connections; // this is a standard set of grid connections, with bits set for E,NE,N,NW,W,SW,S,SE
   Node *m_node;            // graph links
   Point2f m_location;      // note: this is large, but it helps allow loading of non-standard grid points,
                            // whilst allowing them to be displayed as a visibility graph, also speeds up time to
                            // display
   float m_color;           // although display color for the point now introduced
   PixelRef m_merge;        // to merge with another point
   PixelRef m_extent;       // used to speed up graph analysis (not sure whether or not it breaks it!)
   float m_dist;            // used to speed up metric analysis
   float m_cumangle;        // cummulative angle -- used in metric analysis and angular analysis
   // hmm... this is for my 3rd attempt at a quick line intersect algo:
   // every line that goes through the gridsquare -- memory intensive I know, but what can you do:
   // accuracy is imperative here!  Calculated pre-fillpoints / pre-makegraph, and (importantly) it works.
   std::map<int,Line> m_lines;
   int m_processflag;
public:
   Point()
      { m_state = EMPTY; m_block = 0; m_misc = 0; m_grid_connections = 0; m_node = NULL; m_processflag = 0; m_merge = NoPixel; m_user_data = NULL; }
   Point& operator = (const Point& p)
      { throw 1; }
   Point(const Point& p)
      { throw 1; }
   ~Point();
   //
   bool empty() const
      { return (m_state & EMPTY) == EMPTY; }
   bool filled() const
      { return (m_state & FILLED) == FILLED; }
   bool blocked() const
      { return (m_state & BLOCKED) == BLOCKED; }
   bool contextfilled() const
      { return (m_state & CONTEXTFILLED) == CONTEXTFILLED; }
   bool edge() const
      { return (m_state & EDGE) == EDGE; }
   bool selected() const
      { return (m_state & SELECTED) == SELECTED; }
   //
   // Augmented Vis
   bool augmented() const
      { return (m_state & AUGMENTED) == AUGMENTED; }
   //
   void set( int state, int undocounter = 0)
   {
      m_state = state | (m_state & Point::BLOCKED);   // careful not to clear the blocked flag
      m_misc = undocounter;
   }
   void setBlock(bool blocked = true)
   {
      if (blocked)
         m_state |= Point::BLOCKED;
      else
         m_state &= ~Point::BLOCKED;
   }
   void setEdge()
   {
      m_state |= Point::EDGE;
   }
   // old blocking code
   //void addBlock( int block )
   //   { m_block |= block; }
   //void setBlock( int block )
   //   { m_block = block; }
   //int getBlock() const
   //   { return m_block & 0x0000FFFF; }
   //void addPartBlock( int block )
   //   { m_block |= (block << 16); }
   //int getPartBlock() const
   //   { return (m_block & 0xFFFF0000) >> 16; }
   //int getAllBlock() const
   //   { return m_block | (m_block >> 16); }
   //int fillBlocked() const
   //   { return m_block & 0x06600660; }
   int getState()
      { return m_state; }
   int getMisc()  // used as: undocounter, in graph construction, and an agent reference, as well as for making axial maps
      { return m_misc; }
   void setMisc(int misc)
      { m_misc = misc; }
   // note -- set merge pixel should be done only through merge pixels
   PixelRef getMergePixel() {
      return m_merge;
   }
   Node& getNode()
      { return *m_node; }
   char getGridConnections() const
      { return m_grid_connections; }
   float getBinDistance(int i);
public:
   std::istream &read(std::istream &stream, int version, int attr_count);
   std::ofstream& write(std::ofstream& stream, int version);
   //
protected:
   // for user processing, set their own data on the point:
   void *m_user_data;
public:
   void *getUserData()
   { return m_user_data; }
   void setUserData(void *user_data)
   { m_user_data = user_data; }
};
