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



// This is my code to make a set of axial lines from a set of boundary lines

#ifndef __SPARKSIEVE2_H__
#define __SPARKSIEVE2_H__

#include <float.h>
#include "genlib/p2dpoly.h"
#include "genlib/paftl.h"
#include <list>
#include <map>

class sparkSieve2
{
public:
   struct sparkZone2{
      double start;
      double end;
      bool remove;
      sparkZone2( double s = 0.0, double e = 0.0 )
      { start = s; end = e; remove = false; }
      // to allow ordered lists:
      friend bool operator == (const sparkZone2& a, const sparkZone2& b);
      friend bool operator != (const sparkZone2& a, const sparkZone2& b);
      friend bool operator < (const sparkZone2& a, const sparkZone2& b);
      friend bool operator > (const sparkZone2& a, const sparkZone2& b);
   };
private:
   Point2f m_centre;
   double m_maxdist; // for creating graphs that only see out a certain distance: set to -1.0 for infinite
   std::vector<sparkZone2> m_blocks;
public:
   std::list<sparkZone2> m_gaps;
public:
   sparkSieve2( const Point2f& centre, double maxdist = -1.0 );
   ~sparkSieve2();
   bool testblock(const Point2f& point, const std::vector<Line> &lines, double tolerance );
   void block(const std::vector<Line> &lines, int q );
   void collectgarbage();
   double tanify( const Point2f& point, int q );
   //
   bool hasGaps() const 
   {
      return (!m_gaps.empty());
   }
};

inline bool operator == (const sparkSieve2::sparkZone2& a, const sparkSieve2::sparkZone2& b)
{
   return (a.start == b.start && a.end == b.end);
}
inline bool operator != (const sparkSieve2::sparkZone2& a, const sparkSieve2::sparkZone2& b)
{
   return (a.start != b.start || a.end != b.end);
}
inline bool operator < (const sparkSieve2::sparkZone2& a, const sparkSieve2::sparkZone2& b)
{
   return (a.start == b.start) ? (a.end > b.end) : (a.start < b.start);
}
inline bool operator > (const sparkSieve2::sparkZone2& a, const sparkSieve2::sparkZone2& b)
{
   return (a.start == b.start) ? (a.end < b.end) : (a.start > b.start);
}


#endif
