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



// Point data


#include "salalib/pointdata.h"
#include "salalib/parsers/mapinfodata.h" // for mapinfo interface
#include "salalib/isovist.h"
#include "salalib/mgraph.h" // Metagraphs are used...
#include "salalib/ngraph.h"
#include "salalib/attributetable.h"
#include "salalib/attributetablehelpers.h"

#include "genlib/comm.h"  // for communicator
#include "genlib/stringutils.h"
#include "genlib/containerutils.h"

#include <math.h>
#include <unordered_set>
#include <numeric>


/////////////////////////////////////////////////////////////////////////////////

PointMap::PointMap(const QtRegion& parentRegion, const std::vector<SpacePixelFile>& drawingFiles, const std::string& name):
    m_parentRegion(&parentRegion), m_drawingFiles(&drawingFiles), m_points(0,0),
    m_attributes(new AttributeTable()), m_attribHandle(new AttributeTableHandle(*m_attributes))
{
   m_name = name;

   m_cols = 0;
   m_rows = 0;
   m_filled_point_count = 0;

   m_spacing = 0.0;

   m_initialised = false;
   m_blockedlines = false;
   m_processed = false;
   m_boundarygraph = false;

   m_selection = NO_SELECTION;
   m_pinned_selection = false;
   m_undocounter = 0;

   // screen
   m_viewing_deprecated = -1;
   m_draw_step = 1;

   s_bl = NoPixel;
   s_tr = NoPixel;
   curmergeline = -1;

   // -2 follows axial map convention, where -1 is the reference number
   m_displayed_attribute = -2;
}

void PointMap::copy(const PointMap& other)
{
   m_name = other.getName();
   m_region = other.getRegion();

   m_cols = other.getCols();
   m_rows = other.getRows();
   m_filled_point_count = other.getFilledPointCount();

   m_spacing = other.getSpacing();

   m_initialised = other.m_initialised;
   m_blockedlines = other.m_blockedlines;
   m_processed = other.m_processed;
   m_boundarygraph = other.m_boundarygraph;

   m_selection = other.m_selection;
   m_pinned_selection = other.m_pinned_selection;
   m_undocounter = other.m_undocounter;

   // screen
   m_viewing_deprecated = other.m_viewing_deprecated;
   m_draw_step = other.m_draw_step;

   s_bl = other.s_bl;
   s_tr = other.s_tr;
   curmergeline = other.curmergeline;
   m_offset = other.m_offset;
   m_bottom_left = other.m_bottom_left;

   // -2 follows axial map convention, where -1 is the reference number
   m_displayed_attribute = other.m_displayed_attribute;
}

void PointMap::communicate( time_t& atime, Communicator *comm, int record )
{
   if (comm) {
      if (qtimer( atime, 500 )) {
         if (comm->IsCancelled()) {
            throw Communicator::CancelledException();
         }
         comm->CommPostMessage( Communicator::CURRENT_RECORD, record);
      }         
   }
}

bool PointMap::setGrid(double spacing, const Point2f& offset)
{
   m_spacing = spacing;
   // note, the internal offset is the offset from the bottom left
   double xoffset = fmod(m_parentRegion->bottom_left.x + offset.x,m_spacing);
   double yoffset = fmod(m_parentRegion->bottom_left.y + offset.y,m_spacing);
   if (xoffset < m_spacing / 2.0)
      xoffset += m_spacing;
   if (xoffset > m_spacing / 2.0)
      xoffset -= m_spacing;
   if (yoffset < m_spacing / 2.0)
      yoffset += m_spacing;
   if (yoffset > m_spacing / 2.0)
      yoffset -= m_spacing;

   m_offset = Point2f(-xoffset, -yoffset);

   if (m_points.size() != 0) {
      m_filled_point_count = 0;
   }
   m_undocounter = 0;  // <- reset the undo counter... sorry... once you've done this you can't undo

   // A grid at the required spacing:
   m_cols = (int) floor((xoffset + m_parentRegion->width()) / m_spacing + 0.5) + 1;
   m_rows = (int) floor((yoffset + m_parentRegion->height()) / m_spacing + 0.5) + 1;

   m_bottom_left = Point2f(m_parentRegion->bottom_left.x + m_offset.x,
                           m_parentRegion->bottom_left.y + m_offset.y);

   m_region = QtRegion(
      Point2f(m_bottom_left.x-m_spacing/2.0, m_bottom_left.y-m_spacing/2.0), 
      Point2f(m_bottom_left.x+double(m_cols-1)*m_spacing + m_spacing/2.0,
              m_bottom_left.y+double(m_rows-1)*m_spacing + m_spacing/2.0) );

   m_points = depthmapX::ColumnMatrix<Point>(m_rows, m_cols);
   for (size_t j = 0; j < m_cols; j++) {
      for (size_t k = 0; k < m_rows; k++) {
         m_points(k,j).m_location = depixelate(PixelRef(static_cast<short>(j),static_cast<short>(k)));
      }
   }

   m_initialised = true;
   m_blockedlines = false;
   m_processed = false;
   m_boundarygraph = false;

   m_merge_lines.clear();

   return true;
}

bool PointMap::clearPoints()
{
   if (!m_filled_point_count) {
      return false;
   }

   // This function is a bit messy... 
   // each is a slight variation (saves a little time when there's a single selection as opposed to a compound selection
   // someday clean up

   m_undocounter++;
   if (m_selection == NO_SELECTION) {
      for(auto& point: m_points) {
         if(point.filled()) {
             point.set( Point::EMPTY, m_undocounter );
         }
      }
      m_filled_point_count = 0;
      m_merge_lines.clear();
   }
   else if (m_selection & SINGLE_SELECTION) {
      m_undocounter++;
      for (int i = s_bl.x; i <= s_tr.x; i++) {
         for (int j = s_bl.y; j <= s_tr.y; j++) {
            Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
            if (pnt.m_state & (Point::SELECTED | Point::FILLED)) {
               pnt.set( Point::EMPTY, m_undocounter );
               if (!pnt.m_merge.empty()) {
                  PixelRef p = pnt.m_merge;
                  depthmapX::findAndErase(m_merge_lines, PixelRefPair(PixelRef(i,j),p));
                  getPoint(p).m_merge = NoPixel;
                  getPoint(p).m_state &= ~Point::MERGED;
               }
               m_filled_point_count--;
            }
         }
      }
   }
   else { // COMPOUND_SELECTION (note, need to test bitwise now)
      for (size_t i = 0; i < m_cols; i++) {
         for (size_t j = 0; j < m_rows; j++) {
            Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
            if (pnt.m_state & (Point::SELECTED | Point::FILLED)) {
               pnt.set( Point::EMPTY, m_undocounter );
               if (!pnt.m_merge.empty()) {
                  PixelRef p = pnt.m_merge;
                  depthmapX::findAndErase(m_merge_lines, PixelRefPair(PixelRef(i,j),p));
                  getPoint(p).m_merge = NoPixel;
                  getPoint(p).m_state &= ~Point::MERGED;
               }
               m_filled_point_count--;
            }
         }
      }
   }

   m_selection_set.clear();
   m_selection = NO_SELECTION;

   return true;
}

bool PointMap::undoPoints()
{
   if (!m_undocounter) {
      return false;
   }
   for (auto& p: m_points) {
        if ( p.m_misc == m_undocounter) {
            if (p.m_state & Point::FILLED) {
                p.m_state &= ~Point::FILLED;
                p.m_state |= Point::EMPTY;
                p.m_misc = 0; // probably shouldn't set to 0 (can't undo)  Eventually will implement 'redo' counter as well
                m_filled_point_count--;
            }
            else if (p.m_state & Point::EMPTY) {
                p.m_state |= Point::FILLED;
                p.m_state &= ~Point::EMPTY;
                p.m_misc = 0; // probably shouldn't set to 0 (can't undo)  Eventually will implement 'redo' counter as well
                m_filled_point_count++;
            }
        }
   }
   m_undocounter--;  // reduce undo counter

   return true;
}

// constrain is used to constrain to existing rows / cols 
// (not quite the same as constraining to bounding box due to spacing offsets)
PixelRef PointMap::pixelate( const Point2f& p, bool constrain, int scalefactor ) const
{
   PixelRef ref;
 
   double spacing = m_spacing / double(scalefactor);
   ref.x = int(floor( (p.x - m_bottom_left.x + (m_spacing / 2.0)) / spacing ));
   ref.y = int(floor( (p.y - m_bottom_left.y + (m_spacing / 2.0)) / spacing ));

   if (constrain) {
      if (ref.x < 0) 
         ref.x = 0;
      else if (ref.x >= static_cast<short>(m_cols * scalefactor))
         ref.x = (m_cols * scalefactor) - 1;
      if (ref.y < 0) 
         ref.y = 0;
      else if (ref.y >= static_cast<short>(m_rows * scalefactor))
         ref.y = (m_rows * scalefactor) - 1;
   }

   return ref;
}

void PointMap::fillLine(const Line& li)
{
   PixelRefVector pixels = pixelateLine( li, 1 );
   for (size_t j = 0; j < pixels.size(); j++) {
      if (getPoint(pixels[j]).empty()) {
         getPoint(pixels[j]).set( Point::FILLED, m_undocounter );
         m_filled_point_count++;
      }
   }
}

bool PointMap::blockLines()
{
   if (!m_initialised || m_points.size() == 0) {
      return false;
   }
   if (m_blockedlines) {
      return true;
   }
   // just ensure lines don't exist to start off with (e.g., if someone's been playing with the visible layers)
   unblockLines();

   // This used to use a packed Linekey (file, layer, line), but
   // would require a key with (file, layer, shaperef, seg) when used with shaperef,
   // so just switched to an integer key:

   for (const auto& pixelGroup: *m_drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         // chooses the first editable layer it can find:
         if (pixel.isShown()) {
             std::vector<SimpleLine> newLines = pixel.getAllShapesAsLines();
             for (const auto& line: newLines) {
                blockLine(Line(line.start(), line.end()));
             }
         }
      }
   }

   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         Point& pt = getPoint( curs );
         QtRegion viewport = regionate( curs, 1e-10 );
         std::vector<Line>::iterator iter = pt.m_lines.begin(), end = pt.m_lines.end();
         for(; iter != end; ) {
             if (!iter->crop( viewport )) {
                 // the pixelation is fairly rough to make sure that no point is missed: this just
                 // clears up if any point has been added in error:
                 iter = pt.m_lines.erase(iter);
                 end = pt.m_lines.end();
             } else {
                 ++iter;
             }
         }
      }
   }

   m_blockedlines = true;

   return true;
}

void PointMap::blockLine(const Line& li)
{
   std::vector<PixelRef> pixels = pixelateLineTouching(li,1e-10);
   // touching is generally better for ensuring lines pixelated completely, 
   // although it may catch extra points...
   for (size_t n = 0; n < pixels.size(); n++)
   {
      getPoint(pixels[n]).m_lines.push_back(li);
      getPoint(pixels[n]).setBlock(true);
   }
}

void PointMap::unblockLines(bool clearblockedflag)
{
   // just ensure lines don't exist to start off with (e.g., if someone's been playing with the visible layers)
   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef(i,j);
         getPoint(curs).m_lines.clear();
         if (clearblockedflag) {
            getPoint(curs).setBlock(false);
         }
      }
   }
}

// still used through pencil tool

bool PointMap::fillPoint(const Point2f& p, bool add)
{
   // "false" is do not constrain to bounding box, includes() must be used before getPoint
   PixelRef pix = pixelate(p,false);
   if (!includes(pix)) {
      return false;
   }
   Point& pt = getPoint(pix);
   if (add && !pt.filled()) {
      m_filled_point_count++;
      pt.set( Point::FILLED, ++m_undocounter );
   }
   else if (!add && (pt.m_state & Point::FILLED)) {
      m_filled_point_count--;
      pt.set( Point::EMPTY, ++m_undocounter );
      if (pt.m_merge != NoPixel) {
          unmergePixel(pix);
      }
   }
   return true;
}

/////////////////////////////////////////////////////////////////////////////////

// NB --- I've returned to original

//AV TV // semifilled
bool PointMap::makePoints(const Point2f& seed, int fill_type, Communicator *comm)
{
   if (!m_initialised || m_points.size() == 0) {
      return false;
   }
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_RECORDS, (m_rows * m_cols));
   }
   
   // Snap to existing grid
   // "false" is does not constrain: must use includes() before getPoint
   PixelRef seedref = pixelate( seed, false );

   if (!includes(seedref) || getPoint(seedref).filled()) {
      return false;
   }

   // check if seed point is actually visible from the centre of the cell
   std::vector<Line>& linesTouching = getPoint(seedref).m_lines;
   for(auto line: linesTouching) {
       if(intersect_line_no_touch(line, Line(seed, getPoint(seedref).m_location))) {
           return false;
       }
   }

   if (!m_blockedlines) {
      blockLines();
   }

   m_undocounter++; // undo counter increased ready for fill...

   // AV TV
   //int filltype = fill_type ? Point::FILLED | Point::CONTEXTFILLED : Point::FILLED;
   int filltype;
   if( fill_type == 0 ) // FULLFILL
       filltype = Point::FILLED;
   else if( fill_type == 1 ) // SEMIFILL
       filltype = Point::FILLED | Point::CONTEXTFILLED;
   else // AUGMENT
       filltype = Point::AUGMENTED;

   getPoint(seedref).set( filltype, m_undocounter );
   m_filled_point_count++;

   // Now... start making lines:
   pflipper<PixelRefVector> surface;

   surface.a().push_back( seedref );

   int added = 0;

   time_t atime = 0;
   qtimer( atime, 0 );

   while (surface.a().size() > 0) {
      PixelRef& currpix = surface.a().back();
      int result = 0;
      result |= expand( currpix, currpix.up(), surface.b(), filltype );
      result |= expand( currpix, currpix.down(), surface.b(), filltype );
      result |= expand( currpix, currpix.left(), surface.b(), filltype );
      result |= expand( currpix, currpix.right(), surface.b(), filltype );
      result |= expand( currpix, currpix.up().left(), surface.b(), filltype );
      result |= expand( currpix, currpix.up().right(),  surface.b(), filltype );
      result |= expand( currpix, currpix.down().left(), surface.b(), filltype );
      result |= expand( currpix, currpix.down().right(), surface.b(), filltype );
      // if there is a block, mark the currpix as an edge
      if ((result & 4) || getPoint(currpix).blocked()) {
         getPoint(currpix).setEdge();
      }
      //
      surface.a().pop_back();
      if (surface.a().size() == 0) {
         surface.flip();
      }
      added++;
      communicate( atime, comm, added );
   }

   return true;
}

int PointMap::expand( const PixelRef p1, const PixelRef p2, PixelRefVector& list, int filltype)
{
   if (p2.x < 0 || p2.x >= static_cast<short>(m_cols) || p2.y < 0 || p2.y >= static_cast<short>(m_rows)) {
      // 1 = off edge
      return 1;
   }
   if (getPoint(p2).getState() & Point::FILLED) {
      // 2 = already filled
      return 2;
   }
   Line l(depixelate(p1),depixelate(p2));
   for (auto& line: getPoint(p1).m_lines)
   {
      if (intersect_region(l, line, m_spacing * 1e-10) && intersect_line(l, line, m_spacing * 1e-10)) {
         // 4 = blocked
         return 4;
      }
   }
   for (auto& line: getPoint(p2).m_lines)
   {
      if (intersect_region(l, line, m_spacing * 1e-10) && intersect_line(l, line, m_spacing * 1e-10)) {
         // 4 = blocked
         return 4;
      }
   }
   getPoint(p2).set( filltype, m_undocounter );
   m_filled_point_count++;
   list.push_back( p2 ); 

   // 8 = success
   return 8;
}


void PointMap::outputPoints(std::ostream& stream, char delim)
{
   stream << "Ref" << delim << "x" << delim << "y" << std::endl;
   stream.precision(12);

   int count = 0;
   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint(curs).filled() ) {

            Point2f p = depixelate(curs);
            stream << curs << delim << p.x << delim << p.y << std::endl;
            count++;
         }
      }
   }
}

void PointMap::outputMergeLines(std::ostream& stream, char delim)
{
   stream << "x1" << delim << "y1" << delim << "x2" << delim << "y2" << std::endl;

   stream.precision(12);
   for (size_t i = 0; i < m_merge_lines.size(); i++) {

      Line li(depixelate(m_merge_lines[i].a),depixelate(m_merge_lines[i].b));

      stream << li.start().x << delim << li.start().y << delim
             << li.end().x << delim << li.end().y << std::endl;
   }
}

/////////////////////////////////////////////////////////////////////////////////

void PointMap::outputSummary(std::ostream& myout, char delimiter)
{
   myout << "Ref" << delimiter << "x" << delimiter << "y";

   // TODO: For compatibility write the columns in alphabetical order
   // but the physical columns in the order inserted

   std::vector<size_t> indices(m_attributes->getNumColumns());
   std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

   std::sort(indices.begin(), indices.end(),
       [&](size_t a, size_t b) {
       return m_attributes->getColumnName(a) < m_attributes->getColumnName(b);
   });
   for (int idx: indices) {
       myout << delimiter << m_attributes->getColumnName(idx);
   }

   myout << std::endl;
   myout.precision(8);

   for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
       PixelRef pix = iter->getKey().value;
      if (isObjectVisible(m_layers, iter->getRow())) {
         myout << pix << delimiter;
         Point2f p = depixelate(pix);
         myout << p.x << delimiter << p.y;
         for (int idx: indices) {
             myout << delimiter << iter->getRow().getValue(idx);
         }
         myout << std::endl;
      }
   }
}

void PointMap::outputMif( std::ostream& miffile, std::ostream& midfile )
{
   MapInfoData mapinfodata;
   mapinfodata.exportFile(miffile, midfile, *this);
}

void PointMap::outputNet(std::ostream& netfile)
{
   // this is a bid of a faff, as we first have to get the point locations, 
   // then the connections from a lookup table... ickity ick ick...
   std::map<PixelRef,PixelRefVector> graph;
   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {
         Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
         if (pnt.filled() && pnt.m_node) {
            PixelRef pix(i,j);
            PixelRefVector connections;
            pnt.m_node->contents(connections);
            graph.insert(std::make_pair(pix,connections));
         }
      }
   }
   netfile << "*Vertices " << graph.size() << std::endl;
   double maxdim = __max(m_region.width(),m_region.height());
   Point2f offset = Point2f((maxdim - m_region.width())/(2.0*maxdim),(maxdim - m_region.height())/(2.0*maxdim));
   size_t j = 0;
   for (auto& iter: graph) {
      auto graphKey = iter.first;
      Point2f p = depixelate(graphKey);
      p.x = offset.x + (p.x - m_region.bottom_left.x) / maxdim;
      p.y = 1.0 - (offset.y + (p.y - m_region.bottom_left.y) / maxdim);
      netfile << (j+1) << " \"" << graphKey << "\" " << p.x << " " << p.y << std::endl;
      j++;
   }
   netfile << "*Edges" << std::endl;
   size_t k = 0;
   for (auto& iter: graph) {
      PixelRefVector& list = iter.second;
      for (size_t m = 0; m < list.size(); m++) {
         size_t n = depthmapX::findIndexFromKey(graph, list[m]);
         if (static_cast<int>(n) != -1 && k < n) {
            netfile << (k+1) << " " << (n+1) << " 1" << std::endl;
         }
      }
   }
}

void PointMap::outputConnections(std::ostream& myout)
{
   myout << "#graph v1.0" << std::endl;
   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {
         Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
         if (pnt.filled() && pnt.m_node) {
            PixelRef pix(i,j);
            Point2f p = depixelate(pix);
            myout << "node {\n" 
                  << "  ref    " << pix << "\n" 
                  << "  origin " << p.x << " " << p.y << " " << 0.0 << "\n"
                  << "  connections [" << std::endl;
            myout << *(pnt.m_node);
            myout << "  ]\n}" << std::endl;
         }
      }
   }
}

void PointMap::outputConnectionsAsCSV(std::ostream& myout, std::string delim)
{
    myout << "RefFrom" << delim << "RefTo";
    std::unordered_set<PixelRef, hashPixelRef> seenPix;
    for (size_t i = 0; i < m_cols; i++)
    {
        for (size_t j = 0; j < m_rows; j++)
        {
            Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
            if (pnt.filled() && pnt.m_node)
            {
                PixelRef pix(i,j);
                seenPix.insert(pix);
                PixelRefVector hood;
                pnt.m_node->contents(hood);
                for(PixelRef &p: hood)
                {
                    if(!(std::find(seenPix.begin(), seenPix.end(), p) != seenPix.end()) && getPoint(p).filled())
                    {
                        myout << std::endl << pix << delim << p;
                    }
                }
            }
        }
    }
}

void PointMap::outputLinksAsCSV(std::ostream& myout, std::string delim)
{
    myout << "RefFrom" << delim << "RefTo";
    std::unordered_set<PixelRef, hashPixelRef> seenPix;
    for (size_t i = 0; i < m_cols; i++)
    {
        for (size_t j = 0; j < m_rows; j++)
        {
            Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
            if (pnt.filled() && pnt.m_node)
            {
                PixelRef mergePixelRef = pnt.getMergePixel();
                if(mergePixelRef != NoPixel) {
                    PixelRef pix(i,j);
                    if(seenPix.insert(pix).second)
                    {
                        seenPix.insert(mergePixelRef);
                        myout << std::endl << pix << delim << mergePixelRef;
                    }
                }
            }
        }
    }
}

void PointMap::outputBinSummaries(std::ostream& myout)
{
   myout << "cols " << m_cols << " rows " << m_rows << std::endl;

   myout << "x\ty";
   for (int i = 0; i < 32; i++) {
      myout << "\tbin" << i;
   }
   myout << std::endl;

   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {

         Point p = getPoint(PixelRef(i, j));

         myout << i << "\t" << j;

         if (!p.filled()) {
            for (int k = 0; k < 32; k++) {
               myout << "\t" << 0;
            }
         }
         else {
            for (int k = 0; k < 32; k++) {
               myout << "\t" << p.m_node->bin(k).count();
            }
         }

         myout << std::endl;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////

// Attribute Stuff 

void PointMap::setDisplayedAttribute(int col)
{
   if (m_displayed_attribute == col) {
      return;
   }
   else {
      m_displayed_attribute = col;
   }

   m_attribHandle->setDisplayColIndex(m_displayed_attribute);
}

/////////////////////////////////////////////////////////////////////////////////

// Screen stuff 

void PointMap::setScreenPixel( double unit )
{
   if (unit / m_spacing > 1) {
      m_draw_step = int(unit / m_spacing);
   }
   else {
      m_draw_step = 1;
   }
}

void PointMap::makeViewportPoints( const QtRegion& viewport ) const
{
   // n.b., relies on "constrain" being set to true
   bl = pixelate( viewport.bottom_left, true );
   cur = bl;   // cursor for points
   cur.x -= 1; // findNext expects to find cur.x in the -1 position
   rc  = bl;   // cursor for grid lines
   prc  = bl;  // cursor for point centre grid lines
   prc.x -= 1;
   prc.y -= 1;
   // n.b., relies on "constrain" being set to true
   tr = pixelate( viewport.top_right, true );
   curmergeline = -1;

   m_finished = false;
}

bool PointMap::findNextPoint() const
{
   if (m_finished) {
      return false;
   }
   do  {
      cur.x += m_draw_step;
      if (cur.x > tr.x) {
         cur.x = bl.x;
         cur.y += m_draw_step;
         if (cur.y > tr.y) {
            cur = tr; // safety first --- this will at least return something
            m_finished = true;
            return false;
         }
      }
   } while ( !getPoint(cur).filled() && !getPoint(cur).blocked() );
   return true;
}

bool PointMap::findNextRow() const
{
   rc.y += 1;
   if (rc.y > tr.y) return false;
   return true;
}
Line PointMap::getNextRow() const
{
   Point2f offset( m_spacing / 2.0, m_spacing / 2.0 );
   return Line( depixelate( PixelRef(bl.x, rc.y)) - offset, depixelate( PixelRef(tr.x+1, rc.y)) - offset );
}
bool PointMap::findNextPointRow() const
{
   prc.y += 1;
   if (prc.y > tr.y) return false;
   return true;
}
Line PointMap::getNextPointRow() const
{
   Point2f offset( m_spacing / 2.0, 0 );
   return Line( depixelate( PixelRef(bl.x, prc.y)) - offset, depixelate( PixelRef(tr.x+1, prc.y)) - offset );
}
bool PointMap::findNextCol() const
{
   rc.x += 1;
   if (rc.x > tr.x)  return false;
   return true;
}
Line PointMap::getNextCol() const
{
   Point2f offset( m_spacing / 2.0, m_spacing / 2.0 );
   return Line( depixelate( PixelRef(rc.x, bl.y) ) - offset, depixelate( PixelRef(rc.x, tr.y+1) ) - offset );
}
bool PointMap::findNextPointCol() const
{
   prc.x += 1;
   if (prc.x > tr.x)  return false;
   return true;
}
Line PointMap::getNextPointCol() const
{
   Point2f offset( 0.0, m_spacing / 2.0 );
   return Line( depixelate( PixelRef(prc.x, bl.y) ) - offset, depixelate( PixelRef(prc.x, tr.y+1) ) - offset );
}

bool PointMap::findNextMergeLine() const
{
   if (curmergeline < (int)m_merge_lines.size()) {
      curmergeline++;
   }
   return (curmergeline < (int)m_merge_lines.size());
}

Line PointMap::getNextMergeLine() const
{
   if (curmergeline < (int)m_merge_lines.size()) {
      return Line(depixelate(m_merge_lines[curmergeline].a), 
                  depixelate(m_merge_lines[curmergeline].b));
   }
   return Line();
}

bool PointMap::getPointSelected() const
{
   int state = pointState( cur );
   if (state & Point::SELECTED) {
      return true;
   }
   return false;
}

PafColor PointMap::getPointColor(PixelRef pixelRef) const
{
   PafColor color;
   int state = pointState( pixelRef );
   if (state & Point::HIGHLIGHT) {
      return PafColor( SALA_HIGHLIGHTED_COLOR ); 
   }
   else if (state & Point::SELECTED) {
      return PafColor( SALA_SELECTED_COLOR );
   }
   else {
      if (state & Point::FILLED) {
         if (m_processed) {
            return dXreimpl::getDisplayColor(AttributeKey(pixelRef),
                                             m_attributes->getRow(AttributeKey(pixelRef)),
                                             *m_attribHandle.get(),
                                             true);
         }
         else if (state & Point::EDGE) {
            return PafColor( 0x0077BB77 );
         }
         else if (state & Point::CONTEXTFILLED) {
            return PafColor( 0x007777BB );
         }
         else {
            return PafColor( 0x00777777 );
         }
      }
      else {
         return PafColor();
      }
   }
   return PafColor();   // <- note alpha channel set to transparent - will not be drawn
}

PafColor PointMap::getCurrentPointColor() const
{
    return getPointColor( cur );
}

/////////////////////////////////////////////////////////////////////////////////

// Selection stuff

// eventually we will use returned info to draw the selected point quickly

bool PointMap::clearSel()
{
   if (m_selection == NO_SELECTION) {
      return false;
   }
   for (auto& sel: m_selection_set) {
      getPoint(sel).m_state &= ~Point::SELECTED;
   }
   m_selection_set.clear();
   m_selection = NO_SELECTION;
   m_attributes->deselectAllRows();
   return true;
}

bool PointMap::setCurSel(QtRegion &r, bool add )
{
   if (m_selection == NO_SELECTION) {
      add = false;
   }
   else if (!add) {
      // Since we started using point locations in the sel set this is a lot easier!
      clearSel();
   }

   // n.b., assumes constrain set to true (for if you start the selection off the grid)
   s_bl = pixelate(r.bottom_left, true);
   s_tr = pixelate(r.top_right, true);

   if (!add) {
      m_sel_bounds = r;
   }
   else {
      m_sel_bounds = runion(m_sel_bounds, r);
   }

   int mask = 0;
   mask |= Point::FILLED;

   for (int i = s_bl.x; i <= s_tr.x; i++) {
      for (int j = s_bl.y; j <= s_tr.y; j++) {
         Point& pnt = m_points(static_cast<size_t>(j), static_cast<size_t>(i));
         if ((pnt.m_state & mask) && (~pnt.m_state & Point::SELECTED)) {
            pnt.m_state |= Point::SELECTED;
            m_selection_set.insert( PixelRef(i,j) );
            if (add) {
               m_selection &= ~SINGLE_SELECTION;
               m_selection |= COMPOUND_SELECTION;
            }
            else {
               m_selection |= SINGLE_SELECTION;
            }
            if (pnt.m_node) {
               m_attributes->getRow(AttributeKey(PixelRef(i,j))).setSelection(true);
            }
         }
      }
   }

   // Set the region to our actual region:
   r = QtRegion( depixelate(s_bl), depixelate(s_tr) );

   return true;
}

bool PointMap::setCurSel(const std::vector<int>& selset, bool add)
{
   // note: override cursel, can only be used with analysed pointdata:
   if (!add) {
      clearSel();
   }
   m_selection = COMPOUND_SELECTION;
   // not sure what to do with m_sel_bounds (is it necessary?)
   for (size_t i = 0; i < selset.size(); i++) {
      PixelRef pix = selset[i];
      if (includes(pix)) {
            m_points(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x)).m_state |= Point::SELECTED;
            AttributeRow& row = m_attributes->getRow(AttributeKey(pix));
            if (!row.isSelected()) {
               row.setSelection(true);
               m_selection_set.insert(pix);
            }
      }
   }
   return true;
}

// Helper function: is there a blocked point next to you?
// ...rather scruffily goes round the eight adjacent points...

// This is being phased out, with the new "edge" points (which are the filled edges of the graph)

bool PointMap::blockedAdjacent( const PixelRef p ) const
{
   bool ba = false;
   PixelRef temp = p.right();
   PixelRef bounds(m_cols, m_rows);

   if (bounds.encloses(temp) && getPoint(temp).blocked()) {                      // Right
      ba = true;
   }
   else {
      temp = temp.up();
      if (bounds.encloses(temp) && getPoint(temp).blocked()) {                   // Top right
         ba = true;
      }
      else {
         temp = temp.left();
         if (bounds.encloses(temp) && getPoint(temp).blocked()) {                // Top
            ba = true;
         }
         else {
            temp = temp.left();
            if (bounds.encloses(temp) && getPoint(temp).blocked()) {             // Top Left
               ba = true;
            }
            else {
               temp = temp.down();
               if (bounds.encloses(temp) && getPoint(temp).blocked()) {          // Left
                  ba = true;
               }
               else {
                  temp = temp.down();
                  if (bounds.encloses(temp) && getPoint(temp).blocked()) {       // Bottom Left
                     ba = true;
                  }
                  else {
                     temp = temp.right();
                     if (bounds.encloses(temp) && getPoint(temp).blocked()) {    // Bottom
                        ba = true;
                     }
                     else {
                        temp = temp.right();
                        if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Bottom right
                           ba = true;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   return ba;
}


////////////////////////////////////////////////////////////////////////////////

bool PointMap::read(std::istream& stream )
{
   m_name = dXstring::readString(stream);


   // NOTE: You MUST set m_spacepix manually!
   m_displayed_attribute = -1;

   stream.read( (char *) &m_spacing, sizeof(m_spacing) );

   int rows, cols;
   stream.read( reinterpret_cast<char *>(&rows), sizeof(rows) );
   stream.read( reinterpret_cast<char *>(&cols), sizeof(cols) );
   m_rows = static_cast<size_t>(rows);
   m_cols = static_cast<size_t>(cols);

   stream.read( (char *) &m_filled_point_count, sizeof(m_filled_point_count) );

   stream.read( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   m_region = QtRegion(
      Point2f(m_bottom_left.x-m_spacing/2.0, m_bottom_left.y-m_spacing/2.0), 
      Point2f(m_bottom_left.x+double(m_cols-1)*m_spacing + m_spacing/2.0,
              m_bottom_left.y+double(m_rows-1)*m_spacing + m_spacing/2.0) );

   int displayed_attribute;  // n.b., temp variable necessary to force recalc below

   // our data read
   stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));
   m_attributes->read( stream, m_layers );

   m_points = depthmapX::ColumnMatrix<Point>(m_rows, m_cols);
   
   for (size_t j = 0; j < m_cols; j++) {
      for (size_t k = 0; k < m_rows; k++) {
         m_points(k, j).read(stream);

         // check if occdistance of any pixel's bin is set, meaning that
         // the isovist analysis was done
         if(!m_hasIsovistAnalysis) {
             for(int b = 0; b < 32; b++) {
                if(m_points(k, j).m_node && m_points(k, j).m_node->occdistance(b) > 0) {
                    m_hasIsovistAnalysis = true;
                    break;
                }
             }
         }
      }


      for (size_t k = 0; k < m_rows; k++) {
         Point& pnt = m_points(k, j);
         // Old style point node reffing and also unselects selected nodes which would otherwise be difficult

         // would soon be better simply to turn off the select flag....
         pnt.m_state &= ( Point::EMPTY | Point::FILLED | Point::MERGED | Point::BLOCKED | Point::CONTEXTFILLED | Point::EDGE);

         // Set the node pixel if it exists:
         if (pnt.m_node) {
            pnt.m_node->setPixel(PixelRef(j,k));
         }
         // Add merge line if merged:
         if (!pnt.m_merge.empty()) {
             depthmapX::addIfNotExists(m_merge_lines, PixelRefPair(PixelRef(j,k),pnt.m_merge));
         }
      }
   }

   m_selection = NO_SELECTION;
   m_pinned_selection = false;

   m_initialised = true;
   m_blockedlines = false;

   stream.read((char *) &m_processed, sizeof(m_processed));
   stream.read((char *) &m_boundarygraph, sizeof(m_boundarygraph));

   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

bool PointMap::write( std::ostream& stream )
{
   dXstring::writeString(stream, m_name);

   stream.write( (char *) &m_spacing, sizeof(m_spacing) );

   int rows = static_cast<int>(m_rows);
   int cols = static_cast<int>(m_cols);
   stream.write( reinterpret_cast<char *>(&rows), sizeof(rows) );
   stream.write( reinterpret_cast<char *>(&cols), sizeof(cols) );

   stream.write( (char *) &m_filled_point_count, sizeof(m_filled_point_count) );

   stream.write( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   // TODO: Compatibility. The attribute columns will be stored sorted alphabetically
   // so the displayed attribute needs to match that
   int sortedDisplayedAttribute = m_attributes->getColumnSortedIndex(m_displayed_attribute);
   stream.write( (char *) &sortedDisplayedAttribute, sizeof(sortedDisplayedAttribute) );

   m_attributes->write( stream, m_layers );
   
   for (auto& point: m_points) {
       point.write( stream );
   }

   stream.write((char *) &m_processed, sizeof(m_processed));
   stream.write((char *) &m_boundarygraph, sizeof(m_boundarygraph));

   return false;
}

////////////////////////////////////////////////////////////////////////////////

// Now what this class is actually for: making a visibility graph!

// Visibility graph construction constants

int PointMap::tagState(bool settag)
{
   m_selection_set.clear();
   m_selection = NO_SELECTION;

   int count = 0;

   for (size_t i = 0; i < m_cols; i++) {
      for (size_t j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         // First ensure only one of filled/empty/blocked is on:
         Point& pt = getPoint(curs);
         if (pt.filled() ) {
            if (settag) {
               pt.m_misc = count;
               pt.m_processflag = 0x00FF; // process all quadrants
            }
            else {
               pt.m_misc = 0;
               pt.m_processflag = 0x0000; // reset process flag
            }
            count++;
         }

      }
   }
   return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////

#include "sparksieve2.h"

// The fast way to generate graphs... attempt 2
// This uses the new points line segments to allow quick overlap comparisons
// The spark method uses a 1024 long bit string to check against

// writing the algo has thrown up something: it would be more appropriate to 
// have lines between the grid points, rather than centred on the grid square,
// i.e.:
//
// .-.     ---
// |X| not |.| (dots are the grid points)
// .-.     ---
//
// Then wouldn't have to 'test twice' for the grid point being blocked...
// ...perhaps a tweak for a later date!

bool PointMap::sparkGraph2( Communicator *comm, bool boundarygraph, double maxdist )
{
   // Note, graph must be fixed (i.e., having blocking pixels filled in)

   if (!m_blockedlines) {
      blockLines();
   }

   if (boundarygraph) {
      for (size_t i = 0; i < m_cols; i++) {
         for (size_t j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef( static_cast<short>(i), static_cast<short>(j) );
            if ( getPoint( curs ).filled() && !getPoint( curs ).edge()) {
               m_points(j, i).m_state &= ~Point::FILLED;
               m_filled_point_count--;
            }
         }
      }
   }

   // attributes table set up
   // n.b. these must be entered in alphabetical order to preserve col indexing:
   int connectivity_col = m_attributes->insertOrResetLockedColumn("Connectivity");
   m_attributes->insertOrResetColumn("Point First Moment");
   m_attributes->insertOrResetColumn("Point Second Moment");

   // pre-label --- allows faster node access later on
   int count = tagState( true );

   // start the timer when you know the true count including fixed points
   
   time_t atime = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, count );
   }

   count = 0;

   for (size_t i = 0; i < m_cols; i++) {

      for (size_t j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );
   
         if ( getPoint( curs ).getState() & Point::FILLED ) {

            getPoint( curs ).m_node = std::unique_ptr<Node>(new Node());
            m_attributes->addRow( AttributeKey(curs) );

            sparkPixel2(curs,1,maxdist); // make flag of 1 suggests make this node, don't set reciprocral process flags on those you can see
                                         // maxdist controls how far to see out to

            count++;    // <- increment count

            if (comm) {
               if (qtimer( atime, 500 )) {
                  if (comm->IsCancelled()) {
                     tagState( false );         // <- the state field has been used for tagging visited nodes... set back to a state variable
                     // (well, actually, no it hasn't!)
                     // Should clear all nodes and attributes here:
                     // Clear nodes
                     // Clear attributes
                     m_attributes->clear();
                     m_displayed_attribute = -2;
                     //
                     throw Communicator::CancelledException();
                  }
                  comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
               }         
            } // if (comm)
         } // if ( getPoint( curs ).getState() & Point::FILLED )
      } // rows
   } // cols

   tagState( false );  // <- the state field has been used for tagging visited nodes... set back to a state variable

   // keeping lines blocked now is wasteful of memory... free the memory involved
   unblockLines(false);

   // and add grid connections
   // (this is easier than trying to work it out per pixel as we calculate visibility)
   addGridConnections();

   // the graph is processed:
   m_processed = true;
   if (boundarygraph) {
      m_boundarygraph = true;
   }

   // override and reset:
   m_displayed_attribute = -2;
   setDisplayedAttribute(connectivity_col);

   return true;
}

bool PointMap::unmake(bool removeLinks) {
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            Point &pnt = getPoint(curs);
            if (pnt.filled()) {
                if(removeLinks) {
                    pnt.m_merge = NoPixel;
                }
                pnt.m_grid_connections = 0;
                pnt.m_node = nullptr;
                pnt.m_lines.clear();
                pnt.setBlock(false);
            }
        }
    }

    m_blockedlines = false;

    if(removeLinks) {
        m_merge_lines.clear();
    }

    m_attributes->clear();

    m_processed = false;
    m_boundarygraph = false;

    m_displayed_attribute = -2;

    return true;
}

// 'make' construct types are: 
// 1 -- build this node
// 2 -- register the reciprocal q octant in nodes you can see as requiring processing

bool PointMap::sparkPixel2(PixelRef curs, int make, double maxdist)
{
   static std::vector<PixelRef> bins_b[32];
   static float far_bin_dists[32];
   for (int i = 0; i < 32; i++) {
      far_bin_dists[i] = 0.0f;
   }
   int neighbourhood_size = 0;
   double total_dist = 0.0;
   double total_dist_sqr = 0.0;

   Point2f centre0 = depixelate(curs);

   for (int q = 0; q < 8; q++) {

      if (!((getPoint(curs).m_processflag) & (1 << q)) ) {
         continue;
      }

      sparkSieve2 sieve(centre0, maxdist);
      int depth = 0;

      // attempt 0 depth line tests by taken appropriate quadrant
      // from immediately around centre
      // note regionate border must be greater than tolerance squared used in interection testing later
      double border = m_spacing * 1e-10;
      QtRegion viewport0 = regionate(curs, 1e-10);
      switch (q) {
      case 0:
         viewport0.top_right.x = centre0.x;
         viewport0.bottom_left.y = centre0.y - border;
         break;
      case 6:
         viewport0.top_right.x = centre0.x + border;
         viewport0.bottom_left.y = centre0.y;
         break;
      case 1:
         viewport0.bottom_left.x = centre0.x;
         viewport0.bottom_left.y = centre0.y - border;
         break;
      case 7:
         viewport0.bottom_left.x = centre0.x - border;
         viewport0.bottom_left.y = centre0.y;
         break;
      case 2:
         viewport0.top_right.x = centre0.x;
         viewport0.top_right.y = centre0.y + border;
         break;
      case 4:
         viewport0.top_right.x = centre0.x + border;
         viewport0.top_right.y = centre0.y;
         break;
      case 3:
         viewport0.bottom_left.x = centre0.x;
         viewport0.top_right.y = centre0.y + border;
         break;
      case 5:
         viewport0.bottom_left.x = centre0.x - border;
         viewport0.top_right.y = centre0.y;
         break;
      }
      std::vector<Line> lines0;
      for (const Line& line: getPoint(curs).m_lines)
      {
         Line l = line;
         if (l.crop(viewport0)) {
            lines0.push_back(l);
         }
      }
      sieve.block(lines0, q);
      sieve.collectgarbage();

      std::vector<PixelRef> addlist;

      for (depth = 1; sieve.hasGaps(); depth++) {

         addlist.clear();   
         if (!sieve2(sieve, addlist, q, depth, curs))
         {
            break;
         }

         for (size_t n = 0; n < addlist.size(); n++)
         {
            if (getPoint(addlist[n]).getState() & Point::FILLED)
            {
               int bin = whichbin(depixelate(addlist[n])-centre0);
               if (make & 1) {
                  // the blocked cells shouldn't contribute to point stats
                  // note m_spacing is used to scale the moment of inertia appropriately
                  double this_dist = dist(addlist[n],curs) * m_spacing;
                  if (this_dist > far_bin_dists[bin]) {
                     far_bin_dists[bin] = (float) this_dist;
                  }
                  total_dist += this_dist;
                  total_dist_sqr += this_dist * this_dist;
                  neighbourhood_size++;

                  bins_b[bin].push_back( addlist[n] );
               }
               if (make & 2) {
                  getPoint(addlist[n]).m_processflag |= q_opposite(bin);
               }
            }
         }

      }  // <- for (depth = 1; sieve.hasgaps(); depth++)

   }  // <- for (int q = 0; q < 8; q++)

   if (make & 1) {
      // The bins are cleared in the make function!
      Point& pt = getPoint( curs );
      pt.m_node->make(curs, bins_b, far_bin_dists, pt.m_processflag);   // note: make clears bins!
      AttributeRow& row = m_attributes->getRow( AttributeKey(curs) );
      row.setValue( "Connectivity", float(neighbourhood_size) );
      row.setValue( "Point First Moment", float(total_dist) );
      row.setValue( "Point Second Moment", float(total_dist_sqr) );
   }
   else {
      // Clear bins by hand if not using them to make
      for (int i = 0; i < 32; i++) {
         bins_b[i].clear();
      }
   }

   // reset process flag
   getPoint(curs).m_processflag = 0;

   return true;
}

bool PointMap::sieve2(sparkSieve2& sieve, std::vector<PixelRef>& addlist, int q, int depth, PixelRef curs)
{
   bool hasgaps = false;
   int firstind = 0;

   for (auto iter = sieve.m_gaps.begin(); iter != sieve.m_gaps.end(); ++iter) {
      // this goes through all open points
      if (iter->remove) {
         continue;
      }
      for (int ind = (int)ceil(iter->start * (depth - 0.5) - 0.5);
               ind <= (int) floor(iter->end * (depth + 0.5) + 0.5); ind++) {
         if (ind < firstind) {
            continue;
         }
         if (ind > depth) {
            break;
         }
         // this did say first = ind + 1, but I needed to change it to cope with vertical lines
         // I have a feeling the ind + 1 was there for a reason!
         // (if there to cope with boundary graph, could easily simply use ind + 1 in the specific check)
         firstind = ind; 

         // x and y are calculated using Grad's whichbin q quadrants
         int x = (q >= 4 ? ind : depth);
         int y = (q >= 4 ? depth : ind);

         PixelRef here = PixelRef( 
                           curs.x + (q % 2 ? x : -x), curs.y + (q <= 1 || q >= 6 ? y : -y) );

         if (includes(here)) {
            hasgaps = true;
            // centre gap checks to see if the point is blocked itself
            bool centregap = (double(ind) >= (iter->start * depth) &&
                              double(ind) <= (iter->end * depth));

            if (centregap && (getPoint(here).m_state & Point::FILLED)) {
               // don't repeat axes / diagonals
               if ((ind != 0 || q == 0 || q == 1 || q == 5 || q == 6) && (ind != depth || q < 4)) {
                  // block test as usual [tested 31.10.04 -- MUST use 1e-10 for Gassin at 10 grid spacing]
                  if (!sieve.testblock(depixelate(here), getPoint(here).m_lines, m_spacing * 1e-10))  
                  {
                     addlist.push_back(here);
                  }
               }
            }
            sieve.block( getPoint(here).m_lines, q );
         }
      }
   }
   sieve.collectgarbage();

   return hasgaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool PointMap::binDisplay(Communicator *)
{
   int bindisplay_col = m_attributes->insertOrResetColumn("Node Bins");

   for (auto& sel: m_selection_set) {
      Point& p = getPoint(sel);
      // Code for colouring pretty bins:
      for (int i = 0; i < 32; i++) {
         Bin& b = p.m_node->bin(i);
         b.first();
         while(!b.is_tail()) {
            //m_attributes->setValue( row, bindisplay_col, float((i % 8) + 1) );
            m_attributes->getRow(AttributeKey(b.cursor())).setValue( bindisplay_col, float(b.distance()) );
            b.next();   
         }
      }
   }

   // override and reset:
   m_displayed_attribute = -2;
   setDisplayedAttribute(bindisplay_col);

   return true;
}


// Merge connections... very fiddly indeed... using a simple method for now...
// ...and even that's too complicated... so I'll settle for a very, very simple
// merge points (just a reference to another point --- yes, that simple!)

// the first point should have been selected, the second is chosen now:

bool PointMap::mergePoints(const Point2f& p)
{
   if (!m_selection_set.size()) {
      return false;
   }

   // note that in a multiple selection, the point p is adjusted by the selection bounds
   PixelRef bl = pixelate(m_sel_bounds.bottom_left);
   PixelRef tr = pixelate(m_sel_bounds.top_right);
   //
   PixelRef offset = pixelate(p) - PixelRef(tr.x,bl.y);
   //
   for (auto& sel: m_selection_set) {
      PixelRef a = sel;
      PixelRef b = ((PixelRef)sel) + offset;
      // check in limits:
      if (includes(b) && getPoint(b).filled()) {
         mergePixels(a,b);
      }
   }
   clearSel();
   
   return true;
}

bool PointMap::unmergePoints()
{
   if (!m_selection_set.size()) {
      return false;
   }
   for (auto& sel: m_selection_set) {
      PixelRef a = sel;
      Point p = getPoint(a);
      if(p.getMergePixel() != NoPixel) {
        unmergePixel(a);
      }
   }
   clearSel();
   return true;
}

// Either of the pixels can be given here and the other will also be unmerged
bool PointMap::unmergePixel(PixelRef a) {
    PixelRef c = getPoint(a).m_merge;
    depthmapX::findAndErase(m_merge_lines, PixelRefPair(a,c));
    getPoint(c).m_merge = NoPixel;
    getPoint(c).m_state &= ~Point::MERGED;
    getPoint(a).m_merge = NoPixel;
    getPoint(a).m_state &= ~Point::MERGED;
    return true;
}

bool PointMap::mergePixels(PixelRef a, PixelRef b)
{
   if (a == b && !getPoint(a).m_merge.empty()) {
       unmergePixel(a);
   }
   if (a != b && getPoint(a).m_merge != b) {
      if (!getPoint(a).m_merge.empty()) {
         PixelRef c = getPoint(a).m_merge;
         auto it = std::find(m_merge_lines.begin(), m_merge_lines.end(), PixelRefPair(a,c));
         if(it != m_merge_lines.end())
             m_merge_lines.erase(it);
         getPoint(c).m_merge = NoPixel;
         getPoint(c).m_state &= ~Point::MERGED;
      }
      if (!getPoint(b).m_merge.empty()) {
         PixelRef c = getPoint(b).m_merge;
         auto it = std::find(m_merge_lines.begin(), m_merge_lines.end(), PixelRefPair(b,c));
         if(it != m_merge_lines.end())
             m_merge_lines.erase(it);
         getPoint(c).m_merge = NoPixel;
         getPoint(c).m_state &= ~Point::MERGED;
      }
      getPoint(a).m_merge = b;
      getPoint(a).m_state |= Point::MERGED;
      getPoint(b).m_merge = a;
      getPoint(b).m_state |= Point::MERGED;
      m_merge_lines.push_back(PixelRefPair(a,b));
   }

   // actually this return now triggers redraw whatever
   // rather than passing back altered status (as a point must be deselected)
   return true;
}

void PointMap::mergeFromShapeMap(const ShapeMap& shapemap)
{
   const std::map<int,SalaShape>& polygons = shapemap.getAllShapes();
   for (auto polygon: polygons) {
      const SalaShape& poly = polygon.second;
      if (poly.isLine()) {
         PixelRef a = pixelate(poly.getLine().start());
         PixelRef b = pixelate(poly.getLine().end());
         if (getPoint(a).filled() && getPoint(b).filled()) {
            mergePixels(a,b);
         }
      }
   }
}

bool PointMap::isPixelMerged(const PixelRef& a)
{
    return !getPoint(a).m_merge.empty();
}


// -2 for point not in visibility graph, -1 for point has no data
double PointMap::getLocationValue(const Point2f& point)
{
   double val = -2;

   // "false" does not constrain to bounds
   PixelRef pix = pixelate(point, false);
   // quick check for outside row / col bounds:
   if (!includes(pix)) {
      return val;
   }

   if (!getPoint(pix).filled()) {
      val = -2;
   }
   else if (m_displayed_attribute == -1) {
      val = static_cast<float>(pix);
   } else {
      val = m_attributes->getRow(AttributeKey(pix)).getValue(m_displayed_attribute);
   }

   return val;
}

/////////////////////////////////////////////////////////////////////////////////
// Update connections will load an old graph and add char information

void PointMap::addGridConnections()
{
   for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
      PixelRef curs = iter->getKey().value;
      PixelRef node = curs.right();
      Point& point = getPoint(curs);
      point.m_grid_connections = 0;
      for (int i = 0; i < 32; i += 4) {
         Bin& bin = point.m_node->bin(i);
         bin.first();
         while (!bin.is_tail()) {
            if (node == bin.cursor()) {
               point.m_grid_connections |= (1 << (i / 4));
               break;
            }
            bin.next();
         }
         char dir = PixelRef::NODIR;
         if (i == 0) {
            dir = PixelRef::VERTICAL;
         }
         else if (i == 4 || i == 8) {
            dir = PixelRef::NEGHORIZONTAL;
         }
         else if (i == 12 || i == 16) {
            dir = PixelRef::NEGVERTICAL;
         }
         else if (i == 20 || i == 24) {
            dir = PixelRef::HORIZONTAL;
         }
         node.move(dir);
      }
   }
}

// value in range 0 to 1
PixelRef PointMap::pickPixel(double value) const
{
   int which = (int)ceil(value * m_rows * m_cols) - 1;
   return PixelRef(which % m_cols, which / m_cols);
}
