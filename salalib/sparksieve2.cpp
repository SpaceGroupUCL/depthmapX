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

/////////////////////////////////////////////////////////////////////////////////

// New spark sieve implemementation (more accurate)

#include <math.h>

#include <salalib/mgraph.h>
#include <salalib/spacepix.h>
#include <salalib/pointdata.h>

#include "sparksieve2.h"

sparkSieve2::sparkSieve2( const Point2f& centre, double maxdist )
{
   m_centre = centre;
   m_maxdist = maxdist;

   m_gaps.push_back( sparkZone2(0.0, 1.0) );
}

sparkSieve2::~sparkSieve2()
{
}

bool sparkSieve2::testblock( const Point2f& point, const std::vector<Line>& lines, double tolerance )
{
   Line l(m_centre, point);

   // maxdist is to construct graphs with a maximum visible distance: (-1.0 is infinite)
   if (m_maxdist != -1.0 && l.length() > m_maxdist) {
      return true;
   }

   for (auto line: lines)
   {
      // Note: must check regions intersect before using this intersect_line test -- see notes on intersect_line
      if (intersect_region(l,line,tolerance) && intersect_line(l,line,tolerance)) {
         return true;
      }
   }

   return false;
}

//

void sparkSieve2::block( const std::vector<Line>& lines, int q )
{
   for (auto line: lines) {
      double a = tanify(line.start(), q);
      double b = tanify(line.end(), q);

      sparkZone2 block;
      if (a < b) {
         block.start = a - 1e-10;   // 1e-10 required for floating point error
         block.end = b + 1e-10;
      }
      else {
         block.start = b - 1e-10;   // 1e-10 required for floating point error
         block.end = a + 1e-10;
      }
      // this creates a list of blocks sorted by start location
      m_blocks.push_back(block);
   }
   std::sort( m_blocks.begin(), m_blocks.end() );
   m_blocks.erase( std::unique( m_blocks.begin(), m_blocks.end() ), m_blocks.end() );
}

void sparkSieve2::collectgarbage()
{
   auto iter = m_gaps.begin();
   auto blockIter = m_blocks.begin();

   for (; blockIter != m_blocks.end() && iter != m_gaps.end();)
   {
      if (blockIter->end < iter->start) {
         blockIter++;
         continue;
      }
      bool create = true;
      if (blockIter->start <= iter->start) {
         create = false;
         if (blockIter->end > iter->start) {
            // simply move the start in front of us
            iter->start = blockIter->end;
         }
      }
      if (blockIter->end >= iter->end) {
         create = false;
         if (blockIter->start < iter->end) {
            // move the end behind us
            iter->end = blockIter->start;
         }
      }
      if (iter->end <= iter->start + 1e-10) { // 1e-10 required for floating point error
         iter = m_gaps.erase(iter);
         continue;  // on the next iteration, stay with this block
      }
      else if (blockIter->end > iter->end) {
         ++iter;
         continue; // on the next iteration, stay with this block
      }
      else if (create) {
         // add a new gap (has to be behind us), and move the start in front of us
         m_gaps.insert(iter, sparkZone2( iter->start, blockIter->start ) );
         iter->start = blockIter->end;
      }
      blockIter++;
   }
   // reset blocks for next row:
   m_blocks.clear();
}

// q quadrants:
//
//      \ 6 | 7 /
//      0 \ | / 1
//      - -   - -
//      2 / | \ 3
//      / 4 | 5 \
//

double sparkSieve2::tanify( const Point2f& point, int q )
{
   switch (q)
   {
   case 0:
      return (point.y - m_centre.y) / (m_centre.x - point.x);
      break;
   case 1:
      return (point.y - m_centre.y) / (point.x - m_centre.x);
      break;
   case 2:
      return (m_centre.y - point.y) / (m_centre.x - point.x);
      break;
   case 3:
      return (m_centre.y - point.y) / (point.x - m_centre.x);
      break;
   case 4:
      return (m_centre.x - point.x) / (m_centre.y - point.y);
      break;
   case 5:
      return (point.x - m_centre.x) / (m_centre.y - point.y);
      break;
   case 6:
      return (m_centre.x - point.x) / (point.y - m_centre.y);
      break;
   case 7:
      return (point.x - m_centre.x) / (point.y - m_centre.y);
      break;
   }
   return -1.0;
}
