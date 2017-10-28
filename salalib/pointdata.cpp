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

#include <math.h>
#include <genlib/paftl.h>
#include <genlib/comm.h>  // for communicator

#include <salalib/mgraph.h>
#include <salalib/spacepix.h>
#include <salalib/datalayer.h>
#include <salalib/pointdata.h>
#include "MapInfoData.h"
#include "isovist.h"

// Metagraphs are used...
#include <salalib/mgraph.h>
#include <salalib/ngraph.h>

#include "genlib/stringutils.h"
/////////////////////////////////////////////////////////////////////////////////

Point::~Point()
{
   if (m_node) {
      delete m_node;
      m_node = NULL;
      delete m_attributes;
      m_attributes = NULL;
   }
}

float Point::getBinDistance(int i)
{
   return m_node->bindistance(i);
}

ifstream& Point::read(ifstream& stream, int version, int attr_count)
{
   if (m_node) {
      delete m_node;
      m_node = NULL;
      delete m_attributes;
      m_attributes = NULL;
   }
   stream.read( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // (note block NO LONGER used!)
   stream.read( (char *) &m_block, sizeof(m_block) );
   if (version < VERSION_BLOCKEDQUAD) {
      m_block = 0;
   }
   stream.read( (char *) &m_misc, sizeof(m_misc) );
   if (version >= VERSION_GRID_CONNECTIONS) {
      stream.read( (char *) &m_grid_connections, sizeof(m_grid_connections) );
   }
   if (version >= VERSION_NGRAPH_INTROD) {
      stream.read( (char *) &m_merge, sizeof(m_merge) );
      bool ngraph;
      stream.read( (char *) &ngraph, sizeof(ngraph) );
      if (ngraph) {
         m_node = new Node;
         m_node->read(stream, version);
         if (version < VERSION_ATTRIBUTES_TABLE) {
            // don't deal with this here, just get the attributes into memory
            m_attributes = new AttrBody(-1,g_attr_header);
            m_attributes->read( stream, attr_count );
         }
      }
   }
   if (version >= VERSION_POINT_LOCATIONS) {
      stream.read((char *) &m_location, sizeof(m_location));
   }
   if (version < VERSION_SHAPE_MAPS) {
      // old layer information
      m_data_objects.read(stream);
   }
   if (version >= VERSION_BOUNDARYGRAPH && version < VERSION_NEWBOUNDARYGRAPH) {
      // dummy pvecint to hold old format boundary nodes
      pvecint old_boundary_nodes;
      old_boundary_nodes.read(stream);
   }
   return stream;
}

ofstream& Point::write(ofstream& stream, int version)
{
   stream.write( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // note block is no longer used at all
   stream.write( (char *) &m_block, sizeof(m_block) );
   stream.write( (char *) &m_misc, sizeof(m_misc) );
   stream.write( (char *) &m_grid_connections, sizeof(m_grid_connections) );
   stream.write( (char *) &m_merge, sizeof(m_merge) );
   bool ngraph;
   if (m_node) {
      ngraph = true;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
      m_node->write(stream, version);
   }
   else {
      ngraph = false;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
   }
   stream.write((char *) &m_location, sizeof(m_location));
   return stream;
}

/////////////////////////////////////////////////////////////////////////////////

int PointMaps::addNewMap(const std::string& name)
{
   std::string myname = name;
   int counter = 1;
   bool duplicate = true;
   while (duplicate) {
      duplicate = false;
      for (size_t i = 0; i < maps_vector.size(); i++) {
         if (maps_vector.at(i).getName() == myname) {
            duplicate = true;
            myname = dXstring::formatString(counter++,name + " %d");
            break;
         }  
      }
   }
   maps_vector.push_back(PointMap(myname)); 
   maps_vector.back().setSpacePixel(m_spacepix); 
   m_displayed_map = maps_vector.size() - 1; 
   return maps_vector.size() - 1; 
}

bool PointMaps::read(ifstream& stream, int version)
{
   stream.read((char *) &m_displayed_map, sizeof(m_displayed_map));
   int count;
   stream.read((char *) &count, sizeof(count));
   for (int i = 0; i < count; i++) {
      maps_vector.push_back(PointMap());
      maps_vector.back().setSpacePixel( (SuperSpacePixel *) this );
      maps_vector.back().read( stream, version );
   }
   return true;
}

bool PointMaps::write(ofstream& stream, int version, bool displayedmaponly)
{
   if (!displayedmaponly) {
      stream.write((char *) &m_displayed_map, sizeof(m_displayed_map));
      int count = maps_vector.size();
      stream.write((char *) &count, sizeof(count));
      for (int i = 0; i < count; i++) {
         maps_vector.at(i).write( stream, version );
      }
   }
   else {
      int dummy;
      // displayed map is 0:
      dummy = 0;
      stream.write((char *) &dummy, sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *) &dummy, sizeof(dummy));
      //
      maps_vector.at(m_displayed_map).write(stream, version);
   }
   return true;
}

/////////////////////////////////////////////////////////////////////////////////

PointMap::PointMap(const std::string& name)
{
   m_name = name;

   m_points = NULL;
   m_cols = 0;
   m_rows = 0;
   m_point_count = 0;

   m_spacepix = NULL;
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

PointMap::~PointMap()
{
   if (m_points) {
      // Trying to clear out the memory quicker -> predelete nodes and bins
      for (int j = 0; j < m_cols; j++) {
         for (int k = 0; k < m_rows; k++) {
            if (m_points[j][k].m_node) {
               delete m_points[j][k].m_node;
               m_points[j][k].m_node = NULL;
            }
         }
      }
      for (int i = 0; i < m_cols; i++) {
         delete [] m_points[i];
      }
      delete [] m_points;
      m_points = NULL;
      m_cols = 0;
      m_rows = 0;
      m_point_count = 0;
   }
}

PointMap::PointMap(const PointMap& pointdata)
{
   construct(pointdata);
}

PointMap& PointMap::operator = (const PointMap& pointdata)
{
   if (this != &pointdata) {

      if (m_points) {
         for (int i = 0; i < m_cols; i++) {
            delete [] m_points[i];
         }
         delete [] m_points;
         m_points = NULL;
         m_cols = 0;
         m_rows = 0;         
      }
      construct(pointdata);
   }
   return *this;
}

// Technically should not call a function from constructor!

void PointMap::construct(const PointMap& pointdata)
{
   // NB Does not set SpacePixel 
   // You *must* set SpacePixel manually

   m_name = pointdata.m_name;

   m_cols = pointdata.m_cols;
   m_rows = pointdata.m_rows;

   if (m_cols) {
      m_points = new Point *[m_cols];
      for (int i = 0; i < m_cols; i++) {
         m_points[i] = new Point [m_rows];
         for (int j = 0; j < m_rows; j++) {
            m_points[i][j] = pointdata.m_points[i][j];
         }
      }
   }
   else {
      m_points = NULL;

   }

   m_point_count = pointdata.m_point_count;

   // You *must* set SpacePixel manually
   m_spacepix = NULL;
   m_spacing = pointdata.m_point_count;

   m_initialised = pointdata.m_initialised;
   m_blockedlines = false;       // <- always false for a new point data
   m_processed = pointdata.m_processed;
   m_boundarygraph = pointdata.m_boundarygraph;

   // All selection is turned off in new pointdata layer:
   m_selection = NO_SELECTION;
   m_undocounter = 0;

   // screen: all default params
   m_viewing_deprecated = -1;
   m_draw_step = 1;

   s_bl = NoPixel;
   s_tr = NoPixel;
   curmergeline = -1;

   m_displayed_attribute = pointdata.m_displayed_attribute;
}


// Quick mod - TV
#if defined(_WIN32)
void PointMap::communicate( __time64_t& atime, Communicator *comm, int record )
#else
void PointMap::communicate( time_t& atime, Communicator *comm, int record )
#endif
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

bool PointMap::setSpacePixel(const SuperSpacePixel *spacepix)
{
   m_spacepix = (SuperSpacePixel *) spacepix;

   return true;
}

bool PointMap::setGrid(double spacing, const Point2f& offset)
{
   if (!m_spacepix) {
      return false;
   }

   m_spacing = spacing;
   // note, the internal offset is the offset from the bottom left
   double xoffset = fmod(m_spacepix->m_region.bottom_left.x + offset.x,m_spacing);
   double yoffset = fmod(m_spacepix->m_region.bottom_left.y + offset.y,m_spacing);
   if (xoffset < m_spacing / 2.0)
      xoffset += m_spacing;
   if (xoffset > m_spacing / 2.0)
      xoffset -= m_spacing;
   if (yoffset < m_spacing / 2.0)
      yoffset += m_spacing;
   if (yoffset > m_spacing / 2.0)
      yoffset -= m_spacing;

   m_offset = Point2f(-xoffset, -yoffset);

   if (m_points) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_points[i];
      }
      delete [] m_points;
      m_points = NULL;
      m_point_count = 0;
   }
   m_undocounter = 0;  // <- reset the undo counter... sorry... once you've done this you can't undo

   // A grid at the required spacing:
   m_cols = (int) floor((xoffset + m_spacepix->m_region.width()) / m_spacing + 0.5) + 1;
   m_rows = (int) floor((yoffset + m_spacepix->m_region.height()) / m_spacing + 0.5) + 1;

   m_bottom_left = Point2f(m_spacepix->m_region.bottom_left.x + m_offset.x,
                           m_spacepix->m_region.bottom_left.y + m_offset.y);

   m_region = QtRegion(
      Point2f(m_bottom_left.x-m_spacing/2.0, m_bottom_left.y-m_spacing/2.0), 
      Point2f(m_bottom_left.x+double(m_cols-1)*m_spacing + m_spacing/2.0,
              m_bottom_left.y+double(m_rows-1)*m_spacing + m_spacing/2.0) );

   m_points = new Point *[m_cols];
   for (int j = 0; j < m_cols; j++) {
      m_points[j] = new Point [m_rows];
      for (int k = 0; k < m_rows; k++) {
         m_points[j][k].m_location = depixelate(PixelRef(j,k));
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
   if (!m_point_count) {
      return false;
   }

   // This function is a bit messy... 
   // each is a slight variation (saves a little time when there's a single selection as opposed to a compound selection
   // someday clean up

   m_undocounter++;
   if (m_selection == NO_SELECTION) {
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].filled()) {
               m_points[i][j].set( Point::EMPTY, m_undocounter );
            }
         }
      }
      m_point_count = 0;
      m_merge_lines.clear();
   }
   else if (m_selection & SINGLE_SELECTION) {
      m_undocounter++;
      for (int i = s_bl.x; i <= s_tr.x; i++) {
         for (int j = s_bl.y; j <= s_tr.y; j++) {
            if (m_points[i][j].m_state & (Point::SELECTED | Point::FILLED)) {
               m_points[i][j].set( Point::EMPTY, m_undocounter );
               if (!m_points[i][j].m_merge.empty()) {
                  PixelRef p = m_points[i][j].m_merge;
                  m_merge_lines.remove_at(m_merge_lines.searchindex(PixelRefPair(PixelRef(i,j),p)));
                  getPoint(p).m_merge = NoPixel;
                  getPoint(p).m_state &= ~Point::MERGED;
               }
               m_point_count--;
            }
         }
      }
   }
   else { // COMPOUND_SELECTION (note, need to test bitwise now)
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_state & (Point::SELECTED | Point::FILLED)) {
               m_points[i][j].set( Point::EMPTY, m_undocounter );
               if (!m_points[i][j].m_merge.empty()) {
                  PixelRef p = m_points[i][j].m_merge;
                  m_merge_lines.remove_at(m_merge_lines.searchindex(PixelRefPair(PixelRef(i,j),p)));
                  getPoint(p).m_merge = NoPixel;
                  getPoint(p).m_state &= ~Point::MERGED;
               }
               m_point_count--;
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
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {

         if ( m_points[i][j].m_misc == m_undocounter) {
            Point& p = m_points[i][j];
            if (p.m_state & Point::FILLED) {
               p.m_state &= ~Point::FILLED;
               p.m_state |= Point::EMPTY;
               p.m_misc = 0; // probably shouldn't set to 0 (can't undo)  Eventually will implement 'redo' counter as well
               m_point_count--;
            }
            else if (p.m_state & Point::EMPTY) {
               p.m_state |= Point::FILLED;
               p.m_state &= ~Point::EMPTY;
               p.m_misc = 0; // probably shouldn't set to 0 (can't undo)  Eventually will implement 'redo' counter as well
               m_point_count++;
            }
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
      else if (ref.x >= m_cols * scalefactor) 
         ref.x = (m_cols * scalefactor) - 1;
      if (ref.y < 0) 
         ref.y = 0;
      else if (ref.y >= m_rows * scalefactor) 
         ref.y = (m_rows * scalefactor) - 1;
   }

   return ref;
}

PixelRefList PointMap::getLayerPixels(int layer) 
{
   PixelRefList pixels;

   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         size_t obj = m_points[i][j].m_data_objects.searchindex(layer);
         if (obj != paftl::npos) {
            pixels.push_back( PixelRef(i,j) );
         }
      }
   }

   return pixels;
}

PixelRefList PointMap::getDataObjectPixels(int layer, int object) 
{
   PixelRefList pixels;

   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         size_t obj = m_points[i][j].m_data_objects.searchindex(layer);
         if (paftl::npos != -1 && m_points[i][j].m_data_objects[obj] == object) {
            pixels.push_back( PixelRef(i,j) );
         }
      }
   }

   return pixels;
}

// I've reverted to actually filling the lines, provided there's space to put a filled point in

bool PointMap::fillLines()
{
   if (!m_spacepix || !m_initialised || !m_points) {
      return false;
   }
   m_undocounter++;
   for (size_t file = 0; file < m_spacepix->size(); file++) {
      for (size_t layer = 0; layer < m_spacepix->at(file).size(); layer++) {
         if (m_spacepix->at(file).at(layer).isShown()) {
            for (size_t k = 0; k < m_spacepix->at(file).at(layer).getAllShapes().size(); k++) {
               SalaShape& shape = m_spacepix->at(file).at(layer).getAllShapes().at(k);
               if (shape.isLine()) {
                  fillLine(shape.getLine());
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     fillLine(Line(shape[n],shape[n+1]));
                  }
                  if (shape.isPolygon()) {
                     fillLine( Line(shape.tail(),shape.head()));
                  }
               }
            }
         }
      }
   }
   return true;
}

void PointMap::fillLine(const Line& li)
{
   PixelRefList pixels = pixelateLine( li, 1 );
   for (size_t j = 0; j < pixels.size(); j++) {
      if (getPoint(pixels[j]).empty()) {
         getPoint(pixels[j]).set( Point::FILLED, m_undocounter );
         m_point_count++;
      }
   }
}

bool PointMap::blockLines()
{
   if (!m_spacepix || !m_initialised || !m_points) {
      return false;
   }
   if (m_blockedlines) {
      return true;
   }
   // just ensure lines don't exist to start off with (e.g., if someone's been playing with the visible layers)
   unblockLines();

   size_t count = 0;

   // This used to use a packed Linekey (file, layer, line), but
   // would require a key with (file, layer, shaperef, seg) when used with shaperef,
   // so just switched to an integer key:

   for (size_t i = 0; i < m_spacepix->size(); i++) {
      for (size_t j = 0; j < m_spacepix->at(i).size(); j++) {
         // chooses the first editable layer it can find:
         if (m_spacepix->at(i).at(j).isShown()) {
            for (size_t k = 0; k < m_spacepix->at(i).at(j).getAllShapes().size(); k++) {
               SalaShape& shape = m_spacepix->at(i).at(j).getAllShapes().at(k);
               if (shape.isLine()) {
                  blockLine(count++,shape.getLine());
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     blockLine(count++,Line(shape[n],shape[n+1]));
                  }
                  if (shape.isPolygon()) {
                     blockLine(count++,Line(shape.tail(),shape.head()));
                  }
               }
            }
         }
      }
   }

   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         Point& pt = getPoint( curs );
         QtRegion viewport = regionate( curs, 1e-10 );
         for (size_t k = pt.m_lines.size() - 1; k != paftl::npos; k--) {
            if (!pt.m_lines.value(k).crop( viewport )) {
               // the pixelation is fairly rough to make sure that no point is missed: this just
               // clears up if any point has been added in error:
               pt.m_lines.remove_at(k);
            }
         }
      }
   }

   m_blockedlines = true;

   return true;
}

void PointMap::blockLine(int key, const Line& li)
{
   pvector<PixelRef> pixels = pixelateLineTouching(li,1e-10); 
   // touching is generally better for ensuring lines pixelated completely, 
   // although it may catch extra points...
   for (size_t n = 0; n < pixels.size(); n++)
   {
      getPoint(pixels[n]).m_lines.add(key,li);
      getPoint(pixels[n]).setBlock(true);
   }
}

void PointMap::unblockLines(bool clearblockedflag)
{
   // just ensure lines don't exist to start off with (e.g., if someone's been playing with the visible layers)
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef(i,j);
         getPoint(curs).m_lines.clear();
         if (clearblockedflag) {
            getPoint(curs).setBlock(false);
         }
      }
   }
}

// New blockLines code replaces:
   
/*   
   char blah[64];
   sprintf(blah,"times %f\nsubtimes %f\n", times, subtimes);
   MessageBox(NULL,blah,"Blah",MB_OK);
   
   for (int file = 0; file < m_spacepix->size(); file++) {
      for (int layer = 0; layer < m_spacepix->at(file).size(); layer++) {
         if (m_spacepix->at(file).at(layer).isShown()) {
            const prefvec<Line>& lineset = m_spacepix->at(file).at(layer).getAllLines();
            for (int i = 0; i < lineset.size(); i++) {
               Line line = lineset[i];
               // line.denormalScale( m_spacepix->m_region ); <- no longer required since standardised PointMap and SpacePixel
               PixelRefList pixels = pixelateLine( line, 4 ); // pixelate at quadruple precision
               for (int j = 0; j < pixels.size(); j++) {
                  PixelRef actual = pixels[j] / 4;
                  // now which bits a filled:
                  int block = 1;
                  block <<= (pixels[j].x % 4);
                  block <<= 4 * (pixels[j].y % 4);
                  getPoint(actual).addBlock( block );
                  if (getPoint(actual).filled() && getPoint(actual).fillBlocked() ) {
                     m_point_count--;
                     getPoint(actual).set( Point::EMPTY, 0 ); // <- note, can't undo this
                  }
               }
            }
         }
      }
   }
   return true;
}
*/

/////////////////////////////////////////////////////////////////////

/*
// These have not been recoded to work with shape maps rather than space pixels
// -- note that LineKey cannot be recoded due to possible changes in line mode

// Instead use blocklines each time a line is added, removed or changed
// (That could be speeded up somewhat with a pixelate of the old and new positions of the line)

void PointMap::addLineDynamic(LineKey ref,const Line& line)
{
   if (!m_spacepix || !m_initialised || !m_points) {
      return;
   }

   pvector<PixelRef> pixels = pixelateLineTouching(line,1e-10);
   for (int n = 0; n < pixels.size(); n++)
   {
      // right... we have a line over us... we need to tell everyone we can see that there's a change taking place
      Point& pt = getPoint(pixels[n]);
      if (pt.filled()) {
         // just go through our current viewshed
         for (int i = 0; i < 32; i++) {
            pt.m_node->bin(i).first();
            while (!pt.m_node->bin(i).is_tail()) {
               getPoint(pt.m_node->bin(i).cursor()).m_processflag |= q_opposite(i);
               pt.m_node->bin(i).next();
            }
         }
         // and we'll have to reprocess ourselves:
         pt.m_processflag = 0x00FF;
      }
      else {
         // slightly trickier... we don't know who can see us!  Go forth and find out:
         sparkPixel2(pixels[n],2);  // <- 2 means tell q_opposites that can see you to reprocess
      }
   }
   // now everything's marked, we can go and add the line:
   for (n = 0; n < pixels.size(); n++) 
   {
      Point& pt = getPoint(pixels[n]);
      int pos = pt.m_lines.add(ref,line);
      QtRegion viewport = regionate( pixels[n], 1e-10 );
      if (!pt.m_lines[pos].crop( viewport )) {
         // the pixelation is fairly rough to make sure that no point is missed: this just
         // clears up if any point has been added in error:
         pt.m_lines.remove_at(pos);
      }
   }
}

void PointMap::removeLineDynamic(LineKey ref, const Line& line)
{
   if (!m_spacepix || !m_initialised || !m_points) {
      return;
   }

   pvector<PixelRef> pixels = pixelateLineTouching(line,1e-10);
   for (int n = 0; n < pixels.size(); n++)
   {
      Point& pt = getPoint(pixels[n]);
      size_t pos = pt.m_lines.searchindex(ref);
      if (pos != paftl::npos) {
         pt.m_lines.remove_at(pos);
      }
   }
   for (n = 0; n < pixels.size(); n++) {
      // okay, line removed, now reprocess everything that can see us:
      Point& pt = getPoint(pixels[n]);
      pt.m_processflag = 0x00FF;
      if (pt.filled()) {
         sparkPixel2(pixels[n],3);  // <- 3 means both reprocess us, and tell q_opposites that can see you to reprocess
      }
      else {
         sparkPixel2(pixels[n],2);  // <- 2 means tell q_opposites that can see you to reprocess
      }
   }
   for (n = 0; n < pixels.size(); n++) {
      // the pixels in the line will automatically flag *each other* as needing reprocessing:
      // cancel this out since we've just done it!
      Point& pt = getPoint(pixels[n]);
      pt.m_processflag = 0x0000;
   }
}
*/
/////////////////////////////////////////////////////////////////////////////////

// no longer used -- block points through block lines only
/*
bool PointMap::blockPoint(const Point2f& p, bool add)
{
   PixelRef pix = pixelate(p,true,4);
   PixelRef actual = pix / 4;
   // now which bits a filled:
   int block = 1;
   block <<= (pix.x % 4);
   block <<= 4 * (pix.y % 4);
   if (add) {
      getPoint(actual).addBlock( block );
      if (getPoint(actual).fillBlocked()) {
         if (getPoint(actual).filled()) {
            m_point_count--;
            getPoint(actual).set( Point::EMPTY, 0 ); // <- note, can't undo this
         }
      }
   }
   else {
      int newblock = getPoint(actual).getBlock() & ~block;
      getPoint(actual).setBlock( newblock );
   }

   return true;
}
*/
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
      m_point_count++;
      pt.set( Point::FILLED, ++m_undocounter );
   }
   else if (!add && (pt.m_state & Point::FILLED)) {
      m_point_count--;
      pt.set( Point::EMPTY, ++m_undocounter );
   }
   return true;
}

/////////////////////////////////////////////////////////////////////////////////

// NB --- I've returned to original

//AV TV // semifilled
bool PointMap::makePoints(const Point2f& seed, int fill_type, Communicator *comm)
{
   if (!m_spacepix) {
      return false;
   }
   if (!m_initialised || !m_points) {
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
   m_point_count++;

   // Now... start making lines:
   pflipper<PixelRefList> surface;

   surface.a().push_back( seedref );

   int added = 0;

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   qtimer( atime, 0 );

   while (surface.a().size() > 0) {
      PixelRef& currpix = surface.a().tail();
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

int PointMap::expand( const PixelRef p1, const PixelRef p2, PixelRefList& list, int filltype)
{
   if (p2.x < 0 || p2.x >= m_cols || p2.y < 0 || p2.y >= m_rows) {
      // 1 = off edge
      return 1;
   }
   if (getPoint(p2).getState() & Point::FILLED) {
      // 2 = already filled
      return 2;
   }
   Line l(depixelate(p1),depixelate(p2));
   for (size_t i = 0; i < getPoint(p1).m_lines.size(); i++)
   {
      if (intersect_region(l, getPoint(p1).m_lines[i], m_spacing * 1e-10) && intersect_line(l, getPoint(p1).m_lines[i], m_spacing * 1e-10)) {
         // 4 = blocked
         return 4;
      }
   }
   for (size_t j = 0; j < getPoint(p2).m_lines.size(); j++)
   {
      if (intersect_region(l, getPoint(p2).m_lines[j], m_spacing * 1e-10) && intersect_line(l, getPoint(p2).m_lines[j], m_spacing * 1e-10)) {
         // 4 = blocked
         return 4;
      }
   }
   getPoint(p2).set( filltype, m_undocounter );
   m_point_count++;
   list.push_back( p2 ); 

   // 8 = success
   return 8;
}


void PointMap::outputPoints(ostream& stream, char delim)
{
   stream << "Ref" << delim << "x" << delim << "y" << endl;
   stream.precision(12);

   int count = 0;
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint(curs).filled() ) {

            Point2f p = depixelate(curs);
            stream << curs << delim << p.x << delim << p.y << endl;
            count++;
         }
      }
   }
}

void PointMap::outputMergeLines(ostream& stream, char delim)
{
   stream << "x1" << delim << "y1" << delim << "x2" << delim << "y2" << endl;

   stream.precision(12);
   for (size_t i = 0; i < m_merge_lines.size(); i++) {

      Line li(depixelate(m_merge_lines[i].a),depixelate(m_merge_lines[i].b));

      stream << li.start().x << delim << li.start().y << delim
             << li.end().x << delim << li.end().y << endl;
   }
}

/////////////////////////////////////////////////////////////////////////////////

void PointMap::outputSummary(ostream& myout, char delimiter)
{
   myout << "Ref" << delimiter << "x" << delimiter << "y";

   m_attributes.outputHeader(myout, delimiter);
   myout.precision(12);

   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      if (m_attributes.isVisible(i)) {
         PixelRef pix = m_attributes.getRowKey(i);
         myout << pix << delimiter;
         Point2f p = depixelate(pix);
         myout << p.x << delimiter << p.y;
         m_attributes.outputRow(i, myout, delimiter); // , update_only = false
      }
   }
}

void PointMap::outputMif( ostream& miffile, ostream& midfile )
{
   MapInfoData mapinfodata;
   mapinfodata.exportFile(miffile, midfile, *this);
}

void PointMap::outputNet(ostream& netfile)
{
   // this is a bid of a faff, as we first have to get the point locations, 
   // then the connections from a lookup table... ickity ick ick...
   pmap<PixelRef,PixelRefList> graph;
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         if (m_points[i][j].filled() && m_points[i][j].m_node) {
            PixelRef pix(i,j);
            PixelRefList connections;
            m_points[i][j].m_node->contents(connections);
            graph.add(pix,connections);
         }
      }
   }
   netfile << "*Vertices " << graph.size() << endl;
   double maxdim = __max(m_region.width(),m_region.height());
   Point2f offset = Point2f((maxdim - m_region.width())/(2.0*maxdim),(maxdim - m_region.height())/(2.0*maxdim));
   for (size_t j = 0; j < graph.size(); j++) {
      Point2f p = depixelate(graph.key(j));
      p.x = offset.x + (p.x - m_region.bottom_left.x) / maxdim;
      p.y = 1.0 - (offset.y + (p.y - m_region.bottom_left.y) / maxdim);
      netfile << (j+1) << " \"" << graph.key(j) << "\" " << p.x << " " << p.y << endl;
   }
   netfile << "*Edges" << endl;
   for (size_t k = 0; k < graph.size(); k++) {
      PixelRefList& list = graph.value(k);
      for (size_t m = 0; m < list.size(); m++) {
         size_t n = graph.searchindex(list[m]);
         if (n != paftl::npos && k < n) {
            netfile << (k+1) << " " << (n+1) << " 1" << endl;
         }
      }
   }
}

void PointMap::outputConnections(ostream& myout)
{
   myout << "#graph v1.0" << endl;
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         if (m_points[i][j].filled() && m_points[i][j].m_node) {
            PixelRef pix(i,j);
            Point2f p = depixelate(pix);
            myout << "node {\n" 
                  << "  ref    " << pix << "\n" 
                  << "  origin " << p.x << " " << p.y << " " << 0.0 << "\n"
                  << "  connections [" << endl;
            myout << *(m_points[i][j].m_node);
            myout << "  ]\n}" << endl;
         }
      }
   }
}

void PointMap::outputConnectionsAsCSV(ostream& myout, std::string delim)
{
    myout << "RefFrom" << delim << "RefTo";
    std::vector<PixelRef> seenPix;
    for (int i = 0; i < m_cols; i++)
    {
        for (int j = 0; j < m_rows; j++)
        {
            if (m_points[i][j].filled() && m_points[i][j].m_node)
            {
                PixelRef pix(i,j);
                seenPix.push_back(pix);
                for (int b = 0; b < 32; b++)
                {
                    PixelRefList hood;
                    m_points[i][j].m_node->bin(b).contents(hood);
                    for(size_t p = 0; p < hood.size(); p++)
                    {
                        if(!(std::find(seenPix.begin(), seenPix.end(), hood[p]) != seenPix.end()))
                        {
                            myout << std::endl << pix << delim << hood[p];
                        }
                    }
                }
            }
        }
    }
}

void PointMap::outputBinSummaries(ostream& myout)
{
   myout << "cols " << m_cols << " rows " << m_rows << endl;

   myout << "x\ty";
   for (int i = 0; i < 32; i++) {
      myout << "\tbin" << i;
   }
   myout << endl;

   int count = 0;
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {

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

         myout << endl;
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
   // make a local copy of the display params for access speed:
   m_display_params = m_attributes.getDisplayParams(m_displayed_attribute);

   m_attributes.setDisplayColumn(m_displayed_attribute,true);
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

PafColor PointMap::getPointColor() const
{
   PafColor color;
   int state = pointState( cur );
   if (state & Point::HIGHLIGHT) {
      return PafColor( SALA_HIGHLIGHTED_COLOR ); 
   }
   else if (state & Point::SELECTED) {
      return PafColor( SALA_SELECTED_COLOR );
   }
   else {
      if (state & Point::FILLED) {
         if (m_processed) {
            return m_attributes.getDisplayColorByKey( cur );
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

/////////////////////////////////////////////////////////////////////////////////

// Selection stuff

// eventually we will use returned info to draw the selected point quickly

bool PointMap::clearSel()
{
   if (m_selection == NO_SELECTION) {
      return false;
   }
   for (size_t i = 0; i < m_selection_set.size(); i++) {
      getPoint(m_selection_set[i]).m_state &= ~Point::SELECTED;
   }
   m_selection_set.clear();
   m_selection = NO_SELECTION;
   m_attributes.deselectAll();
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
         if ((m_points[i][j].m_state & mask) && (~m_points[i][j].m_state & Point::SELECTED)) {
            m_points[i][j].m_state |= Point::SELECTED;
            m_selection_set.push_back( PixelRef(i,j) );
            if (add) {
               m_selection &= ~SINGLE_SELECTION;
               m_selection |= COMPOUND_SELECTION;
            }
            else {
               m_selection |= SINGLE_SELECTION;
            }
            if (m_points[i][j].m_node) {
               m_attributes.selectRowByKey(PixelRef(i,j));
            }
         }
      }
   }

   // Set the region to our actual region:
   r = QtRegion( depixelate(s_bl), depixelate(s_tr) );

   return true;
}

bool PointMap::setCurSel(const pvecint& selset, bool add)
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
         int row = m_attributes.getRowid(pix);
         if (row != -1) {
            m_points[pix.x][pix.y].m_state |= Point::SELECTED;
            if (m_attributes.selectRowByKey(pix)) {
               m_selection_set.push_back(pix);
            }
         }
      }
   }
   return true;
}

// dangerous: used only for making a false selection set
bool PointMap::overrideSelPixel(PixelRef pix)
{
   m_selection = OVERRIDE_SELECTION;
   if (!(m_points[pix.x][pix.y].m_state & Point::SELECTED)) {
      m_points[pix.x][pix.y].m_state |= Point::SELECTED;
      m_selection_set.push_back(pix);
   }
   return true;
}

/*
bool PointMap::togglePin()
{
   // Just a keep it simple stupid version of this... won't happen that often
   if (m_pinned_selection) {
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if ( m_points[i][j].m_state & Point::PINNED ) {
               m_points[i][j].m_state &= ~Point::PINNED;
            }
         }
      }
      m_pinned_set.clear();
      m_pinned_selection = false;
   }
   else if (m_selection != NO_SELECTION) {
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if ( m_points[i][j].m_state & Point::SELECTED ) {
               m_points[i][j].m_state &= ~Point::SELECTED;
               m_points[i][j].m_state |= Point::PINNED;
            }
         }
      }
      m_pinned_set = m_selection_set;
      m_selection_set.clear();
      m_selection = NO_SELECTION;
      m_pinned_selection = true;
   }
   else {
      return false;
   }
   return true;
}
*/

/*
// DEPRECATED: REPLACED BY SHAPEMAP FUNCTION 21.08.05

bool PointMap::convertSelToDataObject( MetaGraph& meta_graph )
{
   // Just a keep it simple stupid version of this too...
   // will certainly happen more often, but why not? --- no brainer computing from now on

   if (m_selection != NO_SELECTION) {

      int layer_ref = meta_graph.MetaGraph::getCurrentLayerRef();
      int object_ref;
      try {
         object_ref = meta_graph.MetaGraph::getCurrentLayer().addObject();
      }
      catch (DataException e) {
         if (e.errorCode() == DataException::LAYER_HAS_COLUMNS) {
            return false;
         }
         else {
            throw e;
         }
      }

      double pointcount = 0.0;
      Point2f centroid;

      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if ( m_points[i][j].m_state & Point::SELECTED ) {
               m_points[i][j].m_state &= ~Point::SELECTED;
               m_points[i][j].m_layer_objects.add(layer_ref, object_ref);
               centroid += depixelate( PixelRef(i,j) );
               pointcount += 1.0;
            }
         }
      }
      m_selection_set.clear();
      m_selection = NO_SELECTION;

      centroid.scale( 1.0 / pointcount );
      meta_graph.MetaGraph::getCurrentLayer()[object_ref].setCentroid( centroid );

      return true;
   }
   else {
      
      return true; // <- even though didn't make layer, there wasn't an error, just no points selected
   }
   return false;
}
*/

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

bool PointMap::read(ifstream& stream, int version )
{
   if (version >= VERSION_POINT_MAP_NAMES) {
      m_name = dXstring::readString(stream);
   }
   else {
      m_name = "VGA Map";
   }

   // NOTE: You MUST set m_spacepix manually!
   m_displayed_attribute = -1;

   if (m_points) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_points[i];
      }
      delete [] m_points;
      m_points = NULL;
   }

   stream.read( (char *) &m_spacing, sizeof(m_spacing) );

   if (version < VERSION_STATE_RECORDED) {
      if (version >= VERSION_SPARK_GRAPH_INTROD) {
         bool redundant;
         stream.read( (char *) &redundant, sizeof(redundant) );
      }
      else {
         double redundant;
         stream.read( (char *) &redundant, sizeof(redundant) );
      }
   }

   stream.read( (char *) &m_rows, sizeof(m_rows) );
   stream.read( (char *) &m_cols, sizeof(m_cols) );

   stream.read( (char *) &m_point_count, sizeof(m_point_count) );

   stream.read( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   m_region = QtRegion(
      Point2f(m_bottom_left.x-m_spacing/2.0, m_bottom_left.y-m_spacing/2.0), 
      Point2f(m_bottom_left.x+double(m_cols-1)*m_spacing + m_spacing/2.0,
              m_bottom_left.y+double(m_rows-1)*m_spacing + m_spacing/2.0) );

   // for old data versions:
   int attr_count = -1, which_attributes = -1;

   int displayed_attribute;  // n.b., temp variable necessary to force recalc below
   if (version >= VERSION_ATTRIBUTES_TABLE) {
      // our data read
      stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));
      m_attributes.read( stream, version );
   }
   else if (version >= VERSION_NGRAPH_INTROD) {
      // ick, a very specific subset have this file format:
      stream.read( (char *) &which_attributes, sizeof(which_attributes) );
      stream.read( (char *) &attr_count, sizeof(int) );
   }

   m_points = new Point *[m_cols];
   
   for (int j = 0; j < m_cols; j++) {
      m_points[j] = new Point [m_rows];
      // ...and read...
      if (version >= VERSION_LAYERS_INTROD) {
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].read(stream,version,attr_count);

            // check if occdistance of any pixel's bin is set, meaning that
            // the isovist analysis was done
            if(!m_hasIsovistAnalysis) {
                for(int b = 0; b < 32; b++) {
                   if(m_points[j][k].m_node && m_points[j][k].m_node->occdistance(b) > 0) {
                       m_hasIsovistAnalysis = true;
                       break;
                   }
                }
            }
         }
      }
      else if (version >= VERSION_EXTRA_POINT_DATA_INTROD) {
         // Hmm... more untidiness from a previous incarnation
         OldPoint2 *oldpoints = new OldPoint2 [m_rows];
         stream.read( (char *) oldpoints, sizeof(OldPoint2) * m_rows );
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].m_block = oldpoints[k].m_noderef;  // <- block is actually for something else!
            m_points[j][k].m_state = oldpoints[k].m_state;
            m_points[j][k].m_misc = oldpoints[k].m_misc;
         }
      }
      else {
         // Hmm... more untidiness from a previous incarnation
         OldPoint1 *oldpoints = new OldPoint1 [m_rows];
         stream.read( (char *) oldpoints, sizeof(OldPoint1) * m_rows );
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].m_block= oldpoints[k].m_noderef;  // <- block is actually for something else!
            m_points[j][k].m_state = oldpoints[k].m_state;
         }
      }

      for (int k = 0; k < m_rows; k++) {
         // Old style point node reffing and also unselects selected nodes which would otherwise be difficult
         if (version >= VERSION_QUICK_GRAPH_INTROD) {
            // would soon be better simply to turn off the select flag....
            m_points[j][k].m_state &= ( Point::EMPTY | Point::FILLED | Point::MERGED | Point::BLOCKED | Point::CONTEXTFILLED | Point::EDGE);
         }
         else if (m_points[j][k].m_state == -1) {
            m_points[j][k].m_state = Point::EMPTY;
         }
         else {
            m_points[j][k].m_state = Point::FILLED;
         }
         // Set the node pixel if it exists:
         if (m_points[j][k].m_node) {
            m_points[j][k].m_node->setPixel(PixelRef(j,k));
         }
         // Set the point location if required:
         if (version < VERSION_POINT_LOCATIONS) {
            m_points[j][k].m_location = depixelate(PixelRef(j,k));
         }
         // Add merge line if merged:
         if (!m_points[j][k].m_merge.empty()) {
            m_merge_lines.add(PixelRefPair(PixelRef(j,k),m_points[j][k].m_merge));
         }
      }
   }

   // this is my attempt to enter these into the new attribute table:
   if (version >= VERSION_NGRAPH_INTROD && version < VERSION_ATTRIBUTES_TABLE) {
      if (which_attributes != GraphVertexList::NONE) {
         displayed_attribute = 0;
         convertAttributes(which_attributes);
      }
      else {
         displayed_attribute = -1;
      }
   }

   // and if the attribute tables are ready, calculate the direct grid connections:
   if (version >= VERSION_NGRAPH_INTROD && version < VERSION_GRID_CONNECTIONS) {
      addGridConnections();
   }

   m_selection = NO_SELECTION;
   m_pinned_selection = false;

   m_initialised = true;
   m_blockedlines = false;

   if (version >= VERSION_POINT_MAPS) {
      stream.read((char *) &m_processed, sizeof(m_processed));
      stream.read((char *) &m_boundarygraph, sizeof(m_boundarygraph));
   }

   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

bool PointMap::write( ofstream& stream, int version )
{
   dXstring::writeString(stream, m_name);

   stream.write( (char *) &m_spacing, sizeof(m_spacing) );

   stream.write( (char *) &m_rows, sizeof(m_rows) );
   stream.write( (char *) &m_cols, sizeof(m_cols) );

   stream.write( (char *) &m_point_count, sizeof(m_point_count) );

   stream.write( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   stream.write( (char *) &m_displayed_attribute, sizeof(m_displayed_attribute) );
   m_attributes.write( stream, version );
   
   for (int j = 0; j < m_cols; j++) {
      for (int k = 0; k < m_rows; k++) {
         m_points[j][k].write( stream, version );
      }
   }

   stream.write((char *) &m_processed, sizeof(m_processed));
   stream.write((char *) &m_boundarygraph, sizeof(m_boundarygraph));

   return false;
}

////////////////////////////////////////////////////////////////////////////////

// A horrible piece of code: this is to convert a file from the old format
// AttrHeader / AttrBody to the new format Attributes Table

// This code converts attributes for all versions
// between VERSION_NGRAPH_INTROD and VERSION_ATTRIBUTES_TABLE

void PointMap::convertAttributes(int which_attributes)
{
   if (which_attributes & GraphVertexList::BASIC) {
      int connectivity_col = m_attributes.insertLockedColumn("Connectivity");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               // insert row...
               int row = m_attributes.insertRow(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::NEIGHBOURHOOD_SIZE);
               m_attributes.setValue(row, connectivity_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::LOCAL) {
      int cluster_col = m_attributes.insertColumn("Visual Clustering Coefficient");
      int control_col = m_attributes.insertColumn("Visual Control");
      int controllability_col = m_attributes.insertColumn("Visual Controllability");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CLUSTER);
               m_attributes.setValue(row, cluster_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CONTROL_HILL);
               m_attributes.setValue(row, control_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CONTROL_TURN);
               m_attributes.setValue(row, controllability_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::GLOBAL) {
      int entropy_col = m_attributes.insertColumn("Visual Entropy");
      int integ_hh_col = m_attributes.insertColumn("Visual Integration [HH]");
      int integ_tk_col = m_attributes.insertColumn("Visual Integration [Tekl]");
      int depth_col = m_attributes.insertColumn("Visual Mean Depth");
      int count_col = m_attributes.insertColumn("Visual Node Count");
      int rel_entropy_col = m_attributes.insertColumn("Visual Relativised Entropy");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::ENTROPY);
               m_attributes.setValue(row, entropy_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::REL_ENTROPY);
               m_attributes.setValue(row, rel_entropy_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::INTEGRATION_HILL);
               m_attributes.setValue(row, integ_hh_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::INTEGRATION_TEKL);
               m_attributes.setValue(row, integ_tk_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::MEAN_DEPTH);
               m_attributes.setValue(row, depth_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::GRAPH_SIZE);
               m_attributes.setValue(row, count_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::POINTDEPTH) {
      int col = m_attributes.insertColumn("Visual Step Depth");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::POINT_DEPTH);
               m_attributes.setValue(row, col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::METRIC) {
      int mspa_col = m_attributes.insertColumn("Metric Mean Shortest-Path Angle");
      int mspl_col = m_attributes.insertColumn("Metric Mean Shortest-Path Distance");
      int dist_col = m_attributes.insertColumn("Metric Mean Straight-Line Distance");
      int count_col = m_attributes.insertColumn("Metric Node Count");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               double val, total = m_points[i][j].getAttributes().getAttr(AttrHeader::METRIC_GRAPH_SIZE);
               //
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_METRIC_ANGLE) / total;
               m_attributes.setValue(row, mspa_col, float(val));
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_METRIC_DEPTH) / total;
               m_attributes.setValue(row, mspl_col, float(val));
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_EUCLID_DIST) / total;
               m_attributes.setValue(row, dist_col, float(val));
               //
               m_attributes.setValue(row, count_col, float(total));
            }
         }
      }
   }
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         if (m_points[i][j].m_attributes) {
            delete m_points[i][j].m_attributes;
            m_points[i][j].m_attributes = NULL;
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

// Now what this class is actually for: making a visibility graph!

// Visibility graph construction constants

pvecint g_primes;
prefvec<Grad> g_gradients;

int PointMap::remaining(int i, int j, int x, int y, int q) 
{
   int x1,y1,ret = 0;
   if (x > 0) {
      if (q % 2 == 0) {
         x1 = i / x;
      }
      else {
         x1 = (m_cols - 1 - i) / x; 
      }
   }
   else {
      ret = 1;
   }
   if (y > 0) {
      if ((q / 2) % 2 == 0) {
         y1 = j / y;
      }
      else {
         y1 = (m_rows - 1 - j) / y;
      }
   }
   else {
      ret = -1;
   }
   if (ret == 1) {
      return y1;
   }
   if (ret == -1) {
      return x1;
   }
   return __min(x1, y1);
}

// Helper for for making graph

int PointMap::tagState(bool settag, bool sparkgraph)
{
   m_selection_set.clear();
   m_selection = NO_SELECTION;

   int count = 0;

   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {

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

//////////////////////////////////////////////////////////////////////////////////

// calculate 32 distances by explicit angle...

bool PointMap::binMap( Communicator *comm )
{
   // Note, graph must be fixed (i.e., having blocking pixels filled in)
   if (!m_spacepix) {
      return false;
   }

   // start the timer when you know the true count including fixed points

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_rows * m_cols );
   }

   int i = 0;

   float far_bin_dists[32];
   Point2f bin_vectors[32];
   Point2f diagonal = m_spacepix->m_region.top_right - m_spacepix->m_region.bottom_left;
   Point2f east = diagonal.length() * Point2f(1.0,0.0);
   for (i = 0; i < 32; i++) {
      bin_vectors[i] = east;
      east.rotate(0.0625 * M_PI);
   }

   int count = 0;

   for (i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint( curs ).getState() & Point::FILLED ) {

            if (!getPoint(curs).m_node) {
               getPoint( curs ).m_node = new Node;
               m_attributes.insertRow( curs );
            }

            Point2f point = depixelate(curs);

            for (int k = 0; k < 32; k++) {
               Line l(point, point + bin_vectors[k]);
               l.crop(m_spacepix->m_region);
               m_spacepix->cutLine(l);//,1);
               far_bin_dists[k] = (float) l.length();
            }

            Point& pt = getPoint( curs );
            pt.getNode().setbindistances( far_bin_dists );

            count++;    // <- increment count

            if (comm) {
               if (qtimer( atime, 500 )) {
                  if (comm->IsCancelled()) {
                     tagState( false, true );         // <- the state field has been used for tagging visited nodes... set back to a state variable
                     //
                     throw Communicator::CancelledException();
                  }
                  comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
               }         
            }
         }
      }
   }

   return true;
}


//////////////////////////////////////////////////////////////////////////////////

// The fast way to generate graphs!

// #include "sparksieve.h"

// DEPRECATED!!!!!!
/*
bool PointMap::sparkGraph( Communicator *comm )
{
   // Note, graph must be fixed (i.e., having blocking pixels filled in)
   if (!m_spacepix) {
      return false;
   }

   // pre-label --- allows faster node access later on
   int count = tagState( true, true );

   // start the timer when you know the true count including fixed points
   int atime = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, count );
   }

   // attributes table set up
   // n.b. these must be entered in alphabetical order to preserve col indexing:
   int connectivity_col = m_attributes.insertLockedColumn("Connectivity");
   int maxradial_col = m_attributes.insertColumn("Isovist Maximum Radial");
   //int maxradial_node_col = m_attributes.insertColumn("Maximum Radial (Node ID)");
   int moment_col = m_attributes.insertColumn("Isovist Moment of Inertia");
   //
   pvecint bins[ GraphVertex::bin_count ];
   pvector<PixelRef> bins_b[32];
   static float far_bin_dists[32];
   for (int ii = 0; ii < 32; ii++) {
      far_bin_dists[ii] = 0.0f;
   }

   count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint( curs ).getState() & Point::FILLED ) {

            if (getPoint(curs).m_node) {
               delete getPoint(curs).m_node;
            }
            getPoint( curs ).m_node = new Node;

            m_attributes.insertRow( curs );

            int neighbourhood_size = 0;
            int max_depth = 0;
            int far_node = -1;
            double far_dist = 0.0;
            double total_dist_sqr = 0.0;

            // note: blocked points (for now) do not see out
            for (int q = 0; q < 8; q++) {

               int standardbin;
               switch (q) { 
                  case 1: standardbin =  0; break;
                  case 3: standardbin = 32; break;
                  case 0: case 2: standardbin = 16; break;
                  case 4: case 5: standardbin = 24; break;
                  case 6: case 7: standardbin =  8; break;
               }

               sparkSieve sieve( sparkSieve::rotateMask(getPoint(curs).getBlock(), q) );

               int depth = 0;

               for (depth = 1; sieve.hasGaps(); depth++) {

                  pvecint trialsetin, trialsetout;

                  for (int ind = 0; ind < depth + 1; ind++) {

                     // x and y are calculated using Grad's whichbin q quadrants

                     int x = (q >= 4 ? ind : depth);
                     int y = (q >= 4 ? depth : ind);

                     PixelRef here = PixelRef( 
                        i + (q % 2 ? x : -x), j + (q <= 1 || q >= 6 ? y : -y) );

                     if (here.x < 0 || here.y < 0 || here.x >= m_cols || here.y >= m_rows) {
                        // Outside point grid
                        trialsetin.push_back( -1 );
                        trialsetout.push_back( -1 );
                     }
                     else if (getPoint(here).getBlock()) {
                        // Blocked
                        trialsetin.push_back( sparkSieve::rotateMask(getPoint(here).getBlock(),q) );
                        trialsetout.push_back( -1 );
                     }
                     else {
                        // Empty or (space) filled
                        trialsetin.push_back( 0 );
                        trialsetout.push_back( -1 );
                     }
                  }

                  sieve.Sieve( trialsetin, trialsetout );

                  for (int tri = ((q % 4 == 0 || (q + 1) % 4 == 0) ? 1 : 0);  // avoid duplicating axes
                       tri < ((q / 4) ? depth : depth + 1);                 // avoid duplicating diagonals
                       tri++) {
                     if (trialsetout[tri] > -1) {

                        int quoi = trialsetout[tri];

                        int x = (q >= 4 ? tri : depth);
                        int y = (q >= 4 ? depth : tri);

                        PixelRef here = PixelRef( 
                           i + (q % 2 ? x : -x), j + (q <= 1 || q >= 6 ? y : -y) );

                        // oh god, yet another whichbin calculation...
                        int bin = ((q % 4 == 0 || (q + 1) % 4 == 0) ? 
                           (standardbin - trialsetout[tri]) :
                           (standardbin + trialsetout[tri]));

                        //if (!getPoint(here).empty()) {

                           if (getPoint(here).getState() & Point::FILLED) {
                              // the blocked cells shouldn't contribute to point stats
                              // note m_spacing is used to scale far_dist appropriately
                              double this_dist = dist(here,curs) * m_spacing;
                              if (this_dist > far_dist) {
                                 far_node = here;
                                 far_dist = this_dist;
                              }
                              total_dist_sqr += this_dist * this_dist;
                              neighbourhood_size++;

                              bins_b[bin].push_back( here );

                           }

                        //}
                     }
                  }
               }
            }
            Point2f point = depixelate(curs);

            // Remember to clear these bins in the make function!
            Point& pt = getPoint( curs );
            pt.m_node->make(curs, bins_b, far_bin_dists, 0x00FF);
            int row = m_attributes.insertRow( curs );
            m_attributes.setValue( row, connectivity_col, float(neighbourhood_size) );
            m_attributes.setValue( row, maxradial_col, float(far_dist) );
            //m_attributes.setValue( row, maxradial_node_col, far_node );
            m_attributes.setValue( row, moment_col, float(total_dist_sqr * m_spacing * m_spacing) );

            count++;    // <- increment count

            if (comm) {
               if (qtimer( atime, 500 )) {
                  if (comm->IsCancelled()) {
                     tagState( false, true );         // <- the state field has been used for tagging visited nodes... set back to a state variable
                     // (well, actually, no it hasn't!)
                     // Should clear all nodes and attributes here:
                     // Clear nodes
                     for (int ii = 0; ii < m_cols; ii++) {
                        for (int jj = 0; jj < m_rows; jj++) {
                           if (m_points[ii][jj].m_node) {
                              delete m_points[ii][jj].m_node;
                              m_points[ii][jj].m_node = NULL;
                           }
                        }
                     }
                     // Clear attributes
                     m_attributes.clear();
                     m_displayed_attribute = -2;
                     //
                     throw Communicator::CancelledException();
                  }
                  comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
               }         
            }
         }
      }
   }

   tagState( false, true );  // <- the state field has been used for tagging visited nodes... set back to a state variable

   // override and reset:
   m_displayed_attribute = -2;
   setDisplayedAttribute(connectivity_col);

   return true;
}
*/
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
   if (!m_spacepix) {
      return false;
   }

   if (!m_blockedlines) {
      blockLines();
   }

   if (boundarygraph) {
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef( i, j );
            if ( getPoint( curs ).filled() && !getPoint( curs ).edge()) {
               m_points[i][j].m_state &= ~Point::FILLED;
               m_point_count--;
            }
         }
      }
   }

   // attributes table set up
   // n.b. these must be entered in alphabetical order to preserve col indexing:
   int connectivity_col = m_attributes.insertLockedColumn("Connectivity");
   m_attributes.insertColumn("Point First Moment");
   m_attributes.insertColumn("Point Second Moment");

   // pre-label --- allows faster node access later on
   int count = tagState( true, true );

   // start the timer when you know the true count including fixed points
   
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, count );
   }

   count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );
   
         if ( getPoint( curs ).getState() & Point::FILLED ) {

            getPoint( curs ).m_node = new Node;
            m_attributes.insertRow( curs );

            sparkPixel2(curs,1,maxdist); // make flag of 1 suggests make this node, don't set reciprocral process flags on those you can see
                                         // maxdist controls how far to see out to

            count++;    // <- increment count

            if (comm) {
               if (qtimer( atime, 500 )) {
                  if (comm->IsCancelled()) {
                     tagState( false, true );         // <- the state field has been used for tagging visited nodes... set back to a state variable
                     // (well, actually, no it hasn't!)
                     // Should clear all nodes and attributes here:
                     // Clear nodes
                     for (int ii = 0; ii < m_cols; ii++) {
                        for (int jj = 0; jj < m_rows; jj++) {
                           if (m_points[ii][jj].m_node) {
                              delete m_points[ii][jj].m_node;
                              m_points[ii][jj].m_node = NULL;
                           }
                        }
                     }
                     // Clear attributes
                     m_attributes.clear();
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

   tagState( false, true );  // <- the state field has been used for tagging visited nodes... set back to a state variable

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

// essentially does the same as sparkGraph2, but designed to go through only
// points tagged for revision, and without a communicator

bool PointMap::dynamicSparkGraph2()
{
   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );
   
         if ( getPoint( curs ).getState() & Point::FILLED ) {

            sparkPixel2(curs,1); // make flag of 1 suggests make this node, don't set reciprocral process flags on those you can see

         }
      }
   }

   // and add grid connections
   // (this is easier than trying to work it out per pixel as we calculate visibility)
   addGridConnections();

   return true;
}

// 'make' construct types are: 
// 1 -- build this node
// 2 -- register the reciprocal q octant in nodes you can see as requiring processing

bool PointMap::sparkPixel2(PixelRef curs, int make, double maxdist)
{
   static pvector<PixelRef> bins_b[32];
   static float far_bin_dists[32];
   for (int i = 0; i < 32; i++) {
      far_bin_dists[i] = 0.0f;
   }
   int neighbourhood_size = 0;
   int max_depth = 0;
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
      pqmap<int,Line> lines0;
      for (size_t m = 0; m < getPoint(curs).m_lines.size(); m++)
      {
         int key = getPoint(curs).m_lines.key(m);
         Line l = getPoint(curs).m_lines.value(m);
         if (l.crop(viewport0)) {
            lines0.add(key,l);
         }
      }
      sieve.block(lines0, q);
      sieve.collectgarbage();

      pvector<PixelRef> addlist;

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

   Point2f point = depixelate(curs);

   if (make & 1) {
      // The bins are cleared in the make function!
      Point& pt = getPoint( curs );
      pt.m_node->make(curs, bins_b, far_bin_dists, pt.m_processflag);   // note: make clears bins!
      int row = m_attributes.getRowid( curs );
      m_attributes.setValue( row, "Connectivity", float(neighbourhood_size) );
      m_attributes.setValue( row, "Point First Moment", float(total_dist) );
      m_attributes.setValue( row, "Point Second Moment", float(total_dist_sqr) );
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

bool PointMap::sieve2(sparkSieve2& sieve, pvector<PixelRef>& addlist, int q, int depth, PixelRef curs)
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

bool PointMap::binDisplay(Communicator *comm)
{
   int bindisplay_col = m_attributes.insertColumn("Node Bins");

   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      m_attributes.setValue( i, bindisplay_col, -1 );
   }

   for (size_t k = 0; k < m_selection_set.size(); k++) {
      Point& p = getPoint(m_selection_set[k]);
      // Code for colouring pretty bins:
      int count = 1;
      int dir = 0;
      for (int i = 0; i < 32; i++) {
         Bin& b = p.m_node->bin(i);
         b.first();
         while(!b.is_tail()) {
            int row = m_attributes.getRowid( b.cursor() );
            //m_attributes.setValue( row, bindisplay_col, float((i % 8) + 1) );
            m_attributes.setValue( row, bindisplay_col, float(b.distance()) );
            b.next();   
         }
      }
   }

   // override and reset:
   m_displayed_attribute = -2;
   setDisplayedAttribute(bindisplay_col);

   return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

// Isovist analysis

bool PointMap::analyseIsovist(Communicator *comm, MetaGraph& mgraph, bool simple_version)
{
    m_hasIsovistAnalysis = false;
   // note, BSP tree plays with comm counting...
   comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
   comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   mgraph.makeBSPtree(comm);

   comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }
   int count = 0;

   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         if (getPoint( curs ).filled()) {
            count++;
            if (getPoint( curs ).contextfilled() && !curs.iseven()) {
               continue;
            }
            Isovist isovist;
            mgraph.makeIsovist(depixelate(curs),isovist);
            int row = m_attributes.getRowid(curs);

            isovist.setData(m_attributes,row, simple_version);
            Node& node = getPoint(curs).getNode();
            pvector<PixelRef> *occ = node.m_occlusion_bins;
            size_t k;
            for (k = 0; k < 32; k++) {
               occ[k].clear();
               node.bin(k).setOccDistance(0.0f);
            }
            for (k = 0; k < isovist.getOcclusionPoints().size(); k++) {
               const PointDist& pointdist = isovist.getOcclusionPoints().at(k);
               int bin = whichbin(pointdist.m_point-depixelate(curs));
               // only occlusion bins with a certain distance recorded (arbitrary scale note!)
               if (pointdist.m_dist > 1.5) {
                  PixelRef pix = pixelate(pointdist.m_point);
                  if (pix != curs) {
                     occ[bin].push_back(pix);
                  }
               }
               node.bin(bin).setOccDistance(pointdist.m_dist);
            }
         }
         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }
    m_hasIsovistAnalysis = true;
   return true;
}

// Graph analysis tools

bool PointMap::analyseVisual(Communicator *comm, Options& options, bool simple_version)
{
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }

   // dX simple version test // TV
//#define _COMPILE_dX_SIMPLE_VERSION

#ifndef _COMPILE_dX_SIMPLE_VERSION
   int cluster_col, control_col, controllability_col;
   if(!simple_version) {
       if (options.local) {
           cluster_col = m_attributes.insertColumn("Visual Clustering Coefficient");
           control_col = m_attributes.insertColumn("Visual Control");
           controllability_col = m_attributes.insertColumn("Visual Controllability");
       }
   }
#endif

   int entropy_col, rel_entropy_col, integ_dv_col, integ_pv_col, integ_tk_col, depth_col, count_col;
   if (options.global) {
      std::string radius_text;
      if (options.radius != -1) {
         radius_text = std::string(" R") + dXstring::formatString(int(options.radius),"%d");
      }

      // n.b. these must be entered in alphabetical order to preserve col indexing:
      // dX simple version test // TV
#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string entropy_col_text = std::string("Visual Entropy") + radius_text;
          entropy_col = m_attributes.insertColumn(entropy_col_text.c_str());
      }
#endif

      std::string integ_dv_col_text = std::string("Visual Integration [HH]") + radius_text;
      integ_dv_col = m_attributes.insertColumn(integ_dv_col_text.c_str());

#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string integ_pv_col_text = std::string("Visual Integration [P-value]") + radius_text;
          integ_pv_col = m_attributes.insertColumn(integ_pv_col_text.c_str());
          std::string integ_tk_col_text = std::string("Visual Integration [Tekl]") + radius_text;
          integ_tk_col = m_attributes.insertColumn(integ_tk_col_text.c_str());
          std::string depth_col_text = std::string("Visual Mean Depth") + radius_text;
          depth_col = m_attributes.insertColumn(depth_col_text.c_str());
          std::string count_col_text = std::string("Visual Node Count") + radius_text;
          count_col = m_attributes.insertColumn(count_col_text.c_str());
          std::string rel_entropy_col_text = std::string("Visual Relativised Entropy") + radius_text;
          rel_entropy_col = m_attributes.insertColumn(rel_entropy_col_text.c_str());
      }
#endif
   }

   int count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint( curs ).filled()) {

            if ((getPoint( curs ).contextfilled() && !curs.iseven()) ||
                (options.gates_only && getPoint(curs).getDataObject(DataLayers::GATES) == -1)) {
               count++;
               continue;
            }

            if (options.global) {

               for (int ii = 0; ii < m_cols; ii++) {
                  for (int jj = 0; jj < m_rows; jj++) {
                     m_points[ii][jj].m_misc = 0;
                     m_points[ii][jj].m_extent = PixelRef(ii,jj);
                  }
               }

               int total_depth = 0;
               int total_nodes = 0;

               pvecint distribution;
               prefvec<PixelRefList> search_tree;
               search_tree.push_back(PixelRefList());
               search_tree.tail().push_back(curs);

               int level = 0;
               while (search_tree[level].size()) {
                  search_tree.push_back(PixelRefList());
                  distribution.push_back(0);
                  for (size_t n = search_tree[level].size() - 1; n != paftl::npos; n--) {
                     Point& p = getPoint(search_tree[level][n]);
                     if (p.filled() && p.m_misc != ~0) {
                        total_depth += level;
                        total_nodes += 1;
                        distribution.tail() += 1;
                        if ((int) options.radius == -1 || level < (int) options.radius &&
                            (!p.contextfilled() || search_tree[level][n].iseven())) {
                           p.m_node->extractUnseen(search_tree[level+1],this,p.m_misc);
                           p.m_misc = ~0;
                           if (!p.m_merge.empty()) {
                              Point& p2 = getPoint(p.m_merge);
                              if (p2.m_misc != ~0) {
                                 p2.m_node->extractUnseen(search_tree[level+1],this,p2.m_misc); // did say p.misc
                                 p2.m_misc = ~0;
                              }
                           }
                        }
                        else {
                           p.m_misc = ~0;
                        }
                     }
                     search_tree[level].pop_back();
                  }
                  level++;
               }
               int row = m_attributes.getRowid(curs);
               // only set to single float precision after divide
               // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of Space
               if(!simple_version) {
                    m_attributes.setValue(row, count_col, float(total_nodes) ); // note: total nodes includes this one
               }
               // ERROR !!!!!!
               if (total_nodes > 1) {
                  double mean_depth = double(total_depth) / double(total_nodes - 1);
                  if(!simple_version) {
                        m_attributes.setValue(row, depth_col, float(mean_depth) );
                  }
                  // total nodes > 2 to avoid divide by 0 (was > 3)
                  if (total_nodes > 2 && mean_depth > 1.0) {
                     double ra = 2.0 * (mean_depth - 1.0) / double(total_nodes - 2);
                     // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                     double rra_d = ra / dvalue(total_nodes);
                     double rra_p = ra / pvalue(total_nodes);
                     double integ_tk = teklinteg(total_nodes, total_depth);
                     m_attributes.setValue(row,integ_dv_col,float(1.0/rra_d));
                     if(!simple_version) {
                          m_attributes.setValue(row,integ_pv_col,float(1.0/rra_p));
                     }
                     if (total_depth - total_nodes + 1 > 1) {
                        if(!simple_version) {
                            m_attributes.setValue(row,integ_tk_col,float(integ_tk));
                        }
                     }
                     else {
                        if(!simple_version) {
                            m_attributes.setValue(row,integ_tk_col,-1.0f);
                        }
                     }
                  }
                  else {
                     m_attributes.setValue(row,integ_dv_col,(float)-1);
                     if(!simple_version) {
                        m_attributes.setValue(row,integ_pv_col,(float)-1);
                        m_attributes.setValue(row,integ_tk_col,(float)-1);
                     }
                  }
                  double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
                  // n.b., this distribution contains the root node itself in distribution[0]
                  // -> chopped from entropy to avoid divide by zero if only one node
                  for (size_t k = 1; k < distribution.size(); k++) {
                     if (distribution[k] > 0) {
                        double prob = double(distribution[k]) / double(total_nodes - 1);
                        entropy -= prob * log2(prob);
                        // Formula from Turner 2001, "Depthmap"
                        factorial *= double(k + 1);
                        double q = (pow( mean_depth, double(k) ) / double(factorial)) * exp(-mean_depth);
                        rel_entropy += (float) prob * log2( prob / q );
                     }
                  }
                  if(!simple_version) {
                    m_attributes.setValue(row, entropy_col, float(entropy) );
                    m_attributes.setValue(row, rel_entropy_col, float(rel_entropy) );
                  }
               }
               else {
                  if(!simple_version) {
                    m_attributes.setValue(row, depth_col,(float)-1);
                    m_attributes.setValue(row, entropy_col,(float)-1);
                    m_attributes.setValue(row, rel_entropy_col,(float)-1);
                  }
               }
            }
            if (options.local) {

               int row = m_attributes.getRowid(curs);

               // This is much easier to do with a straight forward list:
               PixelRefList neighbourhood;
               PixelRefList totalneighbourhood;
               getPoint(curs).m_node->contents(neighbourhood);

               int cluster = 0;
               float control = 0.0f;

               for (size_t i = 0; i < neighbourhood.size(); i++) {
                  int intersect_size = 0, retro_size = 0;
                  Point& retpt = getPoint(neighbourhood[i]);
                  if (retpt.filled() && retpt.m_node) {
                     retpt.m_node->first();
                     while (!retpt.m_node->is_tail()) {
                        retro_size++;
                        if (neighbourhood.searchindex(retpt.m_node->cursor()) != paftl::npos) {
                           intersect_size++;
                        }
                        totalneighbourhood.add(retpt.m_node->cursor()); // <- note add does nothing if member already exists
                        retpt.m_node->next();
                     }
                     control += 1.0f / float(retro_size);
                     cluster += intersect_size;
                  }
               }
#ifndef _COMPILE_dX_SIMPLE_VERSION
               if(!simple_version) {
                   if (neighbourhood.size() > 1) {
                       m_attributes.setValue(row, cluster_col, float(cluster / double(neighbourhood.size() * (neighbourhood.size() - 1.0))) );
                       m_attributes.setValue(row, control_col, float(control) );
                       m_attributes.setValue(row, controllability_col, float( double(neighbourhood.size()) / double(totalneighbourhood.size())) );
                   }
                   else {
                       m_attributes.setValue(row, cluster_col, -1 );
                       m_attributes.setValue(row, control_col, -1 );
                       m_attributes.setValue(row, controllability_col, -1 );
                   }
               }
#endif
            }

            count++;    // <- increment count
         }
         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }

   if (options.global) {
      setDisplayedAttribute(integ_dv_col);
   }
   else if (options.local) {
#ifndef _COMPILE_dX_SIMPLE_VERSION
       if(!simple_version)
           setDisplayedAttribute(cluster_col);
#endif
   }

   return true;
}

bool PointMap::analyseVisualPointDepth(Communicator *comm)
{
   // n.b., insert columns sets values to -1 if the column already exists
   int col = m_attributes.insertColumn("Visual Step Depth");

   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef pix = m_attributes.getRowKey(i);
      getPoint(pix).m_misc = 0;
      getPoint(pix).m_extent = pix;
   }

   prefvec<PixelRefList> search_tree;
   search_tree.push_back(PixelRefList());
   for (size_t j = 0; j < m_selection_set.size(); j++) {
      // need to convert from ints (m_selection_set) to pixelrefs for this op:
      search_tree.tail().push_back(m_selection_set[j]);
   }

   int level = 0;
   while (search_tree[level].size()) {
      search_tree.push_back(PixelRefList());
      for (size_t n = search_tree[level].size() - 1; n != paftl::npos; n--) {
         Point& p = getPoint(search_tree[level][n]);
         if (p.filled() && p.m_misc != ~0) {
            int row = m_attributes.getRowid(search_tree[level][n]);
            m_attributes.setValue(row,col,float(level));
            if (!p.contextfilled() || search_tree[level][n].iseven() || level == 0) {
               p.m_node->extractUnseen(search_tree[level+1],this,p.m_misc);
               p.m_misc = ~0;
               if (!p.m_merge.empty()) {
                  Point& p2 = getPoint(p.m_merge);
                  if (p2.m_misc != ~0) {
                     int row = m_attributes.getRowid(p.m_merge);
                     m_attributes.setValue(row,col,float(level));
                     p2.m_node->extractUnseen(search_tree[level+1],this,p2.m_misc); // did say p.misc
                     p2.m_misc = ~0;
                  }
               }
            }
            else {
               p.m_misc = ~0;
            }
         }
      }
      level++;
   }
   
   // force redisplay:
   m_displayed_attribute = -2;
   setDisplayedAttribute(col);
   
   return true;
}

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

bool PointMap::analyseMetric(Communicator *comm, Options& options)
{
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }

   std::string radius_text;
   if (options.radius != -1.0) {
      if (options.radius > 100.0) {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.f");
      }
      else if (m_region.width() < 1.0) {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.4f");
      }
      else {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.2f");
      }
   }
   // n.b. these must be entered in alphabetical order to preserve col indexing:
   std::string mspa_col_text = std::string("Metric Mean Shortest-Path Angle") + radius_text;
   int mspa_col = m_attributes.insertColumn(mspa_col_text.c_str());
   std::string mspl_col_text = std::string("Metric Mean Shortest-Path Distance") + radius_text;
   int mspl_col = m_attributes.insertColumn(mspl_col_text.c_str());
   std::string dist_col_text = std::string("Metric Mean Straight-Line Distance") + radius_text;
   int dist_col = m_attributes.insertColumn(dist_col_text.c_str());
   std::string count_col_text = std::string("Metric Node Count") + radius_text;
   int count_col = m_attributes.insertColumn(count_col_text.c_str());

   int count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint( curs ).filled() ) {

            if ( options.gates_only && getPoint(curs).getDataObject(DataLayers::GATES) == -1) {
               count++;
               continue;
            }

            for (int ii = 0; ii < m_cols; ii++) {
               for (int jj = 0; jj < m_rows; jj++) {
                  m_points[ii][jj].m_misc = 0;
                  m_points[ii][jj].m_dist = -1.0f;
                  m_points[ii][jj].m_cumangle = 0.0f;
               }
            }

            float euclid_depth = 0.0f;
            float total_depth = 0.0f;
            float total_angle = 0.0f;
            int total_nodes = 0;

            // note that m_misc is used in a different manner to analyseGraph / PointDepth
            // here it marks the node as used in calculation only

            pqvector<MetricTriple> search_list;
            search_list.add(MetricTriple(0.0f,curs,NoPixel));
            int level = 0;
            while (search_list.size()) {
               MetricTriple here = search_list[0];
               search_list.remove_at(0);
               if (options.radius != -1.0 && (here.dist * m_spacing) > options.radius) {
                  break;
               }
               Point& p = getPoint(here.pixel);
               // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
               if (p.filled() && p.m_misc != ~0) {
                  p.m_node->extractMetric(search_list,this,here);
                  p.m_misc = ~0;
                  if (!p.m_merge.empty()) {
                     Point& p2 = getPoint(p.m_merge);
                     if (p2.m_misc != ~0) {
                        p2.m_cumangle = p.m_cumangle;
                        p2.m_node->extractMetric(search_list,this,MetricTriple(here.dist,p.m_merge,NoPixel));
                        p2.m_misc = ~0;
                     }
                  }
                  total_depth += float(here.dist * m_spacing);
                  total_angle += p.m_cumangle;
                  euclid_depth += float(m_spacing * dist(here.pixel,curs));
                  total_nodes += 1;
               }
            }

            int row = m_attributes.getRowid(curs);
            m_attributes.setValue(row, mspa_col, float(double(total_angle) / double(total_nodes)) );
            m_attributes.setValue(row, mspl_col, float(double(total_depth) / double(total_nodes)) );
            m_attributes.setValue(row, dist_col, float(double(euclid_depth) / double(total_nodes)) );
            m_attributes.setValue(row, count_col, float(total_nodes) );

            count++;    // <- increment count
         }
         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }

   m_displayed_attribute = -2;
   setDisplayedAttribute(mspl_col);

   return true;
}

bool PointMap::analyseMetricPointDepth(Communicator *comm)
{
   // n.b., insert columns sets values to -1 if the column already exists
   int path_angle_col = m_attributes.insertColumn("Metric Step Shortest-Path Angle");
   int path_length_col = m_attributes.insertColumn("Metric Step Shortest-Path Length");
   int dist_col;
   if (m_selection_set.size() == 1) {
      // Note: Euclidean distance is currently only calculated from a single point
      dist_col = m_attributes.insertColumn("Metric Straight-Line Distance");
   }

   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef pix = m_attributes.getRowKey(i);
      getPoint(pix).m_misc = 0;
      getPoint(pix).m_dist = -1.0f;
      getPoint(pix).m_cumangle = 0.0f;
   }

   // in order to calculate Penn angle, the MetricPair becomes a metric triple...
   pqvector<MetricTriple> search_list; // contains root point

   for (size_t k = 0; k < m_selection_set.size(); k++) {
      search_list.add(MetricTriple(0.0f,m_selection_set[k],NoPixel));
   }

   // note that m_misc is used in a different manner to analyseGraph / PointDepth
   // here it marks the node as used in calculation only
   int count = 0;
   int level = 0;
   while (search_list.size()) {
      MetricTriple here = search_list[0];
      search_list.remove_at(0);
      Point& p = getPoint(here.pixel);
      // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
      if (p.filled() && p.m_misc != ~0) {
         p.m_node->extractMetric(search_list,this,here);
         p.m_misc = ~0;
         int row = m_attributes.getRowid(here.pixel);
         m_attributes.setValue(row, path_length_col, float(m_spacing * here.dist) );
         m_attributes.setValue(row, path_angle_col, float(p.m_cumangle) );
         if (m_selection_set.size() == 1) {
            // Note: Euclidean distance is currently only calculated from a single point
            m_attributes.setValue(row, dist_col, float(m_spacing * dist(here.pixel,m_selection_set[0])) );
         }
         if (!p.m_merge.empty()) {
            Point& p2 = getPoint(p.m_merge);
            if (p2.m_misc != ~0) {
               p2.m_cumangle = p.m_cumangle;
               int row = m_attributes.getRowid(p.m_merge);
               m_attributes.setValue(row, path_length_col, float(m_spacing * here.dist) );
               m_attributes.setValue(row, path_angle_col, float(p2.m_cumangle) );
               if (m_selection_set.size() == 1) {
                  // Note: Euclidean distance is currently only calculated from a single point
                  m_attributes.setValue(row, dist_col, float(m_spacing * dist(p.m_merge,m_selection_set[0])) );
               }
               p2.m_node->extractMetric(search_list,this,MetricTriple(here.dist,p.m_merge,NoPixel));
               p2.m_misc = ~0;
            }
         }
      }
   }

   m_displayed_attribute = -2;
   setDisplayedAttribute(path_length_col);
   
   return true;
}

///////////////////////////////////////////////////////////////////////////////////

// This is based on the Metric Step Depth calculation
// 

bool PointMap::analyseAngular(Communicator *comm, Options& options)
{
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }

   std::string radius_text;
   if (options.radius != -1.0) {
      if (m_region.width() > 100.0) {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.f");
      }
      else if (m_region.width() < 1.0) {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.4f");
      }
      else {
         radius_text = std::string(" R") + dXstring::formatString(options.radius,"%.2f");
      }
   }
   // n.b. these must be entered in alphabetical order to preserve col indexing:
   std::string mean_depth_col_text = std::string("Angular Mean Depth") + radius_text;
   int mean_depth_col = m_attributes.insertColumn(mean_depth_col_text.c_str());
   std::string total_detph_col_text = std::string("Angular Total Depth") + radius_text;
   int total_depth_col = m_attributes.insertColumn(total_detph_col_text.c_str());
   std::string count_col_text = std::string("Angular Node Count") + radius_text;
   int count_col = m_attributes.insertColumn(count_col_text.c_str());

   int count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( getPoint( curs ).filled() ) {

            if ( options.gates_only && getPoint(curs).getDataObject(DataLayers::GATES) == -1) {
               count++;
               continue;
            }

            for (int ii = 0; ii < m_cols; ii++) {
               for (int jj = 0; jj < m_rows; jj++) {
                  m_points[ii][jj].m_misc = 0;
                  m_points[ii][jj].m_dist = 0.0f;
                  m_points[ii][jj].m_cumangle = -1.0f;
               }
            }

            float total_angle = 0.0f;
            int total_nodes = 0;

            // note that m_misc is used in a different manner to analyseGraph / PointDepth
            // here it marks the node as used in calculation only

            pqvector<AngularTriple> search_list;
            search_list.add(AngularTriple(0.0f,curs,NoPixel));
            getPoint(curs).m_cumangle = 0.0f;
            int level = 0;
            while (search_list.size()) {
               AngularTriple here = search_list[0];
               search_list.remove_at(0);
               if (options.radius != -1.0 && here.angle > options.radius) {
                  break;
               }
               Point& p = getPoint(here.pixel);
               // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
               if (p.filled() && p.m_misc != ~0) {
                  p.m_node->extractAngular(search_list,this,here);
                  p.m_misc = ~0;
                  if (!p.m_merge.empty()) {
                     Point& p2 = getPoint(p.m_merge);
                     if (p2.m_misc != ~0) {
                        p2.m_cumangle = p.m_cumangle;
                        p2.m_node->extractAngular(search_list,this,AngularTriple(here.angle,p.m_merge,NoPixel));
                        p2.m_misc = ~0;
                     }
                  }
                  total_angle += p.m_cumangle;
                  total_nodes += 1;
               }
            }

            int row = m_attributes.getRowid(curs);
            if (total_nodes > 0) {
               m_attributes.setValue(row, mean_depth_col, float(double(total_angle) / double(total_nodes)) );
            }
            m_attributes.setValue(row, total_depth_col, total_angle );
            m_attributes.setValue(row, count_col, float(total_nodes) );

            count++;    // <- increment count
         }
         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }

   m_displayed_attribute = -2;
   setDisplayedAttribute(mean_depth_col);

   return true;
}

bool PointMap::analyseAngularPointDepth(Communicator *comm)
{
   // n.b., insert columns sets values to -1 if the column already exists
   int path_angle_col = m_attributes.insertColumn("Angular Step Depth");

   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef pix = m_attributes.getRowKey(i);
      getPoint(pix).m_misc = 0;
      getPoint(pix).m_dist = 0.0f;
      getPoint(pix).m_cumangle = -1.0f;
   }

   pqvector<AngularTriple> search_list; // contains root point

   for (size_t k = 0; k < m_selection_set.size(); k++) {
      search_list.add(AngularTriple(0.0f,m_selection_set[k],NoPixel));
      getPoint(m_selection_set[k]).m_cumangle = 0.0f;
   }

   // note that m_misc is used in a different manner to analyseGraph / PointDepth
   // here it marks the node as used in calculation only
   int count = 0;
   int level = 0;
   while (search_list.size()) {
      AngularTriple here = search_list[0];
      search_list.remove_at(0);
      Point& p = getPoint(here.pixel);
      // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
      if (p.filled() && p.m_misc != ~0) {
         p.m_node->extractAngular(search_list,this,here);
         p.m_misc = ~0;
         int row = m_attributes.getRowid(here.pixel);
         m_attributes.setValue(row, path_angle_col, float(p.m_cumangle) );
         if (!p.m_merge.empty()) {
            Point& p2 = getPoint(p.m_merge);
            if (p2.m_misc != ~0) {
               p2.m_cumangle = p.m_cumangle;
               int row = m_attributes.getRowid(p.m_merge);
               m_attributes.setValue(row, path_angle_col, float(p2.m_cumangle) );
               p2.m_node->extractAngular(search_list,this,AngularTriple(here.angle,p.m_merge,NoPixel));
               p2.m_misc = ~0;
            }
         }
      }
   }

   m_displayed_attribute = -2;
   setDisplayedAttribute(path_angle_col);
   
   return true;
}

bool PointMap::analyseThruVision(Communicator *comm)
{
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }

   /*
   // original version
   int i;
   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         getPoint( curs ).m_misc = 0;
      }
   }

   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         Point& p = getPoint(curs);
         if ( getPoint( curs ).filled() ) {
            p.m_node->first();
            while (!p.m_node->is_tail()) {
               PixelRef x = p.m_node->cursor();
               Line l(depixelate(curs),depixelate(x));
               PixelRefList pixels = pixelateLine(l,1);
               for (int k = 1; k < pixels.size() - 1; k++) {
                  getPoint(pixels[k]).m_misc += 1;
               }
               p.m_node->next();
            }
         }
      }
   }

   int col = m_attributes.insertColumn("Through vision");

   for (i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
      m_attributes.setValue(i,col,getPoint(curs).m_misc);
      getPoint(curs).m_misc = 0;
   }

   setDisplayedAttribute(col);
   */

   // current version (not sure of differences!)
   int i;
   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         getPoint( curs ).m_misc = 0;
      }
   }

   int count = 0;
   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         pvecint seengates;
         PixelRef curs = PixelRef( i, j );
         Point& p = getPoint(curs);
         if ( getPoint( curs ).filled() ) {
            p.m_node->first();
            while (!p.m_node->is_tail()) {
               PixelRef x = p.m_node->cursor();
               Line li = Line(depixelate(x),depixelate(curs));
               PixelRefList pixels = quickPixelateLine(x,curs);
               for (size_t k = 1; k < pixels.size() - 1; k++) {
                  getPoint(pixels[k]).m_misc += 1;
                  int index = m_attributes.getRowid(pixels[k]);
                  int gate = (index != -1) ? (int)m_attributes.getValue(index,g_col_gate) : -1;
                  if (gate != -1 && seengates.searchindex(gate) == paftl::npos) {
                     m_attributes.incrValue(index, g_col_gate_counts);
                     seengates.add(gate,paftl::ADD_HERE);
                  }
               }
               p.m_node->next();
            }
            // only increment count for actual filled points
            count++;
         }
         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }

   int col = m_attributes.insertColumn("Through vision");

   for (i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
      m_attributes.setValue(i,col,(float)getPoint(curs).m_misc);
      getPoint(curs).m_misc = 0;
   }

   setDisplayedAttribute(-2);
   setDisplayedAttribute(col);
/*
   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         Point& p = getPoint(curs);
         int thisrow = m_attributes.getRowIndex(curs);
         if ( getPoint( curs ).filled() ) {
            p.m_node->first();
            while (!p.m_node->is_tail()) {
               PixelRef x = p.m_node->cursor();
               Line l(depixelate(curs),depixelate(x));
               PixelRefList pixels = pixelateLine(l,1);
               for (int k = 1; k < pixels.size() - 1; k++) {
                  getPoint(pixels[k]).m_misc += m_attributes.getValue(thisrow,col) * 0.001;
               }
               p.m_node->next();
            }
         }
      }
   }

   int col2 = m_attributes.insertColumn("Through vision 2 step");

   for (i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
      m_attributes.setValue(i,col2,getPoint(curs).m_misc);
      getPoint(curs).m_misc = 0;
   }

   for (i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         PixelRef curs = PixelRef( i, j );
         Point& p = getPoint(curs);
         int thisrow = m_attributes.getRowIndex(curs);
         if ( getPoint( curs ).filled() ) {
            p.m_node->first();
            while (!p.m_node->is_tail()) {
               PixelRef x = p.m_node->cursor();
               Line l(depixelate(curs),depixelate(x));
               PixelRefList pixels = pixelateLine(l,1);
               for (int k = 1; k < pixels.size() - 1; k++) {
                  getPoint(pixels[k]).m_misc += m_attributes.getValue(thisrow,col2) * 0.001;
               }
               p.m_node->next();
            }
         }
      }
   }

   int col3 = m_attributes.insertColumn("Through vision 3 step");

   for (i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
      m_attributes.setValue(i,col3,getPoint(curs).m_misc);
      getPoint(curs).m_misc = 0;
   }
*/
   return true;
}

///////////////////////////////////////////////////////////////////////////////////

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
   for (size_t i = 0; i < m_selection_set.size(); i++) {
      PixelRef a = m_selection_set[i];
      PixelRef b = ((PixelRef)m_selection_set[i]) + offset;
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
   for (size_t i = 0; i < m_selection_set.size(); i++) {
      PixelRef a = m_selection_set[i];
      mergePixels(a,a);
   }
   clearSel();
   return true;
}

bool PointMap::mergePixels(PixelRef a, PixelRef b)
{
   bool altered = false;
   if (a == b && !getPoint(a).m_merge.empty()) {
      PixelRef c = getPoint(a).m_merge;
      m_merge_lines.remove_at(m_merge_lines.searchindex(PixelRefPair(a,c)));
      getPoint(c).m_merge = NoPixel;
      getPoint(c).m_state &= ~Point::MERGED;
      getPoint(a).m_merge = NoPixel;
      getPoint(a).m_state &= ~Point::MERGED;
      altered = true;
   }
   if (a != b && getPoint(a).m_merge != b) {
      if (!getPoint(a).m_merge.empty()) {
         PixelRef c = getPoint(a).m_merge;
         m_merge_lines.remove_at(m_merge_lines.searchindex(PixelRefPair(a,c)));
         getPoint(c).m_merge = NoPixel;
         getPoint(c).m_state &= ~Point::MERGED;
      }
      if (!getPoint(b).m_merge.empty()) {
         PixelRef c = getPoint(b).m_merge;
         m_merge_lines.remove_at(m_merge_lines.searchindex(PixelRefPair(b,c)));
         getPoint(c).m_merge = NoPixel;
         getPoint(c).m_state &= ~Point::MERGED;
      }
      getPoint(a).m_merge = b;
      getPoint(a).m_state |= Point::MERGED;
      getPoint(b).m_merge = a;
      getPoint(b).m_state |= Point::MERGED;
      m_merge_lines.add(PixelRefPair(a,b));
      altered = true;
   }

   // actually this return now triggers redraw whatever
   // rather than passing back altered status (as a point must be deselected)
   return true; // altered;
}

void PointMap::mergeFromShapeMap(const ShapeMap& shapemap)
{
   const pqmap<int,SalaShape>& polygons = shapemap.getAllShapes();
   for (size_t i = 0; i < polygons.size(); i++) {
      const SalaShape& poly = polygons[i];
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

//////////////////////////////////////////////////////////////////////////////////

/*
bool PointMap::analyseGraph(Communicator *comm, Graph *graph)
{
   int atime = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_point_count );
   }

   int count = 0;

   SparkTree analyser_tree;

   for (int ii = 0; ii < m_cols; ii++) {
      for (int jj = 0; jj < m_rows; jj++) {
         PixelRef curs = PixelRef( ii, jj );
         if ( !getPoint( curs ).emptyorblocked() ) {
            analyser_tree.set(curs);
         }
      }
   }
   analyser_tree.invert();

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         if ( !getPoint( curs ).emptyorblocked() ) {

            SparkTree global = analyser_tree;

            int total_depth = 0;
            int total_nodes = 0;

            prefvec<PixelRefList> search_tree;

            search_tree.push_back(PixelRefList());

//            global.extractUnseen(search_tree[0], *getPoint( curs ).m_spark_tree);

            int level = 0;
            while (search_tree[level].size()) {
               search_tree.push_back(PixelRefList());
               for (int n = search_tree[level].size() - 1; n != paftl::npos; n--) {
                  Point& p = getPoint(search_tree[level][n]);
                  if (!p.emptyorblocked()) {
                     total_depth += level + 1;
                     total_nodes += 1;
//                     global.extractUnseen(search_tree[level+1],*p.m_spark_tree);
                  }
               }
               level++;
            }
            graph->m_nodes.getAttributes(getPoint(curs).m_noderef).intval(AttrHeader::DEPTH) = total_depth;
            graph->m_nodes.getAttributes(getPoint(curs).m_noderef).intval(AttrHeader::GRAPH_SIZE) = total_nodes;

            count++;    // <- increment count
         }

         if (comm) {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
               comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
            }         
         }
      }
   }

   graph->m_nodes.setWhichAttributes( GraphVertexList::GLOBAL );

   return true;
}
*/
/*
bool PointMap::analysePointDepth(Communicator *comm, Graph *graph)
{
   SparkTree analyser_tree;

   for (int ii = 0; ii < m_cols; ii++) {
      for (int jj = 0; jj < m_rows; jj++) {
         PixelRef curs = PixelRef( ii, jj );
         if ( !getPoint( curs ).emptyorblocked() ) {
            analyser_tree.set(curs);
         }
      }
   }
   analyser_tree.invert();

   PixelRefList selset;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         for (int k = 0; k < m_selection_set.size(); k++) {

            PixelRef curs(i,j);

            if (m_selection_set[k] == getPoint(curs).getRef()) {

               selset.push_back(curs);
               graph->m_nodes.getAttributes(getPoint(curs).getRef()).intval(AttrHeader::POINT_DEPTH) = 0;

            }
         }
      }
   }

   prefvec<PixelRefList> search_tree;
   search_tree.push_back(selset);
   
   int level = 0;
   while (search_tree[level].size()) {
      search_tree.push_back(PixelRefList());
      for (int n = search_tree[level].size() - 1; n != paftl::npos; n--) {
         Point& p = getPoint(search_tree[level][n]);
         if (!p.emptyorblocked()) {
            graph->m_nodes.getAttributes(p.getRef()).intval(AttrHeader::POINT_DEPTH) = level;
//            analyser_tree.extractUnseen(search_tree[level+1],*p.m_spark_tree);
         }
      }
      level++;
   }

   return true;
}
*/
/////////////////////////////////////////////////////////////////////////////////

// Optimization level:
// 0 --- fast --- 'empty' assumption and 'sensible' assumption
// 1 --- med  --- 'sensible' assumption
// 2 --- slow --- no assumptions

// I'm just going to junk this one completely!
/*
bool PointMap::makeGraph( Graph& graph, int optimization_level, Communicator *comm)
{
   makeConstants();

   if (!m_spacepix) {
      return false;
   }
   int atime = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, 8 * m_point_count );
   }

   // pre-label --- allows faster node access later on
   int count = tagState( true, false );

   // tell the graph how many nodes you're going to write... makes things easy later on...
   if (!graph.m_nodes.openwrite(count)) {
      tagState( false, false );
      return false;
   }

   // method 1b: getting a little less forceful still

   // for angular analysis:
   // each is put in a 'bin' which can be retrieved later.
   // the bins are first filled by the algorithm into separate pvectors
   // next, when storing, the points are put into a list, and the references stored
   // as a list afterwards --- this allows easy translation between straight depthmap
   // and depthmap angular / agent
   pmap<double,int> bins[ GraphVertex::bin_count ];

   count = 0;

   for (int i = 0; i < m_cols; i++) {

      for (int j = 0; j < m_rows; j++) {

         PixelRef curs = PixelRef( i, j );

         int far_node = -1;
         double far_dist = -1.0;
         double total_dist = 0.0;

         if ( !getPoint( curs ).emptyorblocked() ) {

            int n = count / 8;
            graph.m_nodes.add( n, GraphVertex() );

            // q quadrants:
            //
            //      \ 6 | 7 /
            //      0 \ | / 1
            //      - -   - -
            //      2 / | \ 3
            //      / 4 | 5 \
            //
            
            for (int q = 0; q < 8; q++) {

               for (int r = 0; r < g_gradients.size(); r++) {

                  if (r == 0 && (q == 2 || q == 3 || q == 5 || q == 7)) {  // avoid duplicating 0,1 gradients
                     r++;
                  }
                  if (r == 1 && q / 4 == 1) {                              // avoid duplicating 1,1 gradients
                     r++;
                  }

                  int x = g_gradients[r].x(q);
                  int y = g_gradients[r].y(q);

                  int top = remaining(i,j,abs(x),abs(y),q);

                  bool broken = false;
                  for (int here = 1; here <= top && !broken; here++)  {

                     PixelRef last( u(i,x,here-1), v(j,y,here-1) );
                     PixelRef next( u(i,x,here), v(j,y,here) );

                     if ( getPoint( next ).emptyorblocked() ) {
                        if ( g_gradients[r].length < 5.0f || optimization_level != 0) {
                           // test anyway, just in case there's no pixel due to a coarse flood fill
                           Line line( depixelate(last), depixelate(next) );
                           if (m_spacepix->intersect(line)) {
                              broken = true; // It really has been broken! Stop.
                           }
                        }
                        else {
                           broken = true;
                        }
                     }
                     else {
                        if (here == 1 && optimization_level != 2 && g_gradients[r].length > 25.0f) {
                           // make a resolution approximation: (code fully tested as of version 0.7b)
                           // (unfortunately, we can only do this once! 
                           //  --- otherwise pixel n - 1 is not guaranteed to have been tested)
                           PixelRef alpha, beta;
                           if (q / 4 == 0) {
                              if (q % 2 == 0)
                                 alpha = next.right();
                              else 
                                 alpha = next.left();
                              if (q / 2 == 0)
                                 beta = alpha.up();
                              else 
                                 beta = alpha.down();
                           }
                           else {
                              if (q / 2 == 2)
                                 alpha = next.up();
                              else 
                                 alpha = next.down();
                              if (q % 2 == 0)
                                 beta = alpha.right();
                              else 
                                 beta = alpha.left();
                           }
                           if (getPoint(alpha).m_misc != n && getPoint(beta).m_misc != n) {
                              broken = true;
                           }
                        }
                        if (!broken) {
                           // Standard testing
                           Line line( depixelate(last), depixelate(next) );
                           if (!m_spacepix->intersect(line)) {
                              getPoint(next).m_misc = n; // <- mark that this was included in this isovist
                              // note m_spacing is used to scale far_dist appropriately
                              double this_dist = g_gradients[r].length * double(here) * m_spacing;
                              if (this_dist > far_dist) {
                                 far_node = getPoint(here).getRef();
                                 far_dist = this_dist;
                              }
                              total_dist += this_dist;
                              // put the point in the appropriate bin ordered by distance
                              bins[ g_gradients[r].whichbin(q) ].add( this_dist, pointRef(next) );
                              // test to see if this node is the far node
                              // old depthmap:
                              // graph.m_nodes[n].push_back( pointRef(next) );
                           }
                           else {
                              broken = true;
                           }
                        }
                     }
                  }
               }
               count++;    // <- increment count (note count = nodes * q

               if (comm) {
                  if (qtimer( atime, 500 )) {
                     if (comm->IsCancelled()) {
                        tagState( false, false );        // <- the state field has been used for tagging visited nodes... set back to a state variable
                        graph.m_nodes.clearAttributes(); // <- any attributes that have been filled in are dropped
                        graph.m_nodes.remove();    // <- drop the file before throwing exception
                        throw Communicator::CancelledException();
                     }
                     comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
                  }         
               }
            }
            Point2f point = depixelate(curs);
            graph.m_nodes[n].make( bins );
            // note m_spacing is used to scale far_dist appropriately
            graph.m_nodes.commit(point, far_node, float(far_dist), float(total_dist));
         }
      }
   }

   tagState( false, false );  // <- the state field has been used for tagging visited nodes... set back to a state variable

   graph.m_nodes.close();

   graph.m_nodes.setWhichAttributes( GraphVertexList::BASIC );

   return true;
}
*/
void makePrimes()
{
   for (int i = 1; i < 200; i++) {
      bool pr = true;
      for (size_t j = 1; j < g_primes.size(); j++) {  // Note : ignore 1
         if (i % g_primes[j] == 0) {
            pr = false;
            break;
         }
         else if (g_primes[j] * g_primes[j] > i) {
            pr = true;
            break;
         }
      }
      if (pr) {
         g_primes.push_back(i);
      }
   }
}

void makeGradients()
{
   // Slow... but can't be bothered to work this out better!

   g_gradients.push_back( Grad(1,0) );
   g_gradients.push_back( Grad(1,1) );

   for (int i = 1; i < 200; i++) {
      for (int j = 1; j < i; j++) {
         if (g_primes.searchindex(i) != paftl::npos) {
            g_gradients.push_back( Grad(i, j) );
         }
         else if (j == 1) {
            g_gradients.push_back( Grad(i, j) );
         }
         else if (i % j != 0) {
            bool pr = true;
            for (size_t k = 1; k < g_primes.size(); k++) {  // Note: ignore 1
               if (i % g_primes[k] == 0 && j % g_primes[k] == 0) {
                  pr = false;
                  break;
               }
               else if (g_primes[k] > j) {
                  pr = true;
                  break;
               }
            }
            if (pr) {
               g_gradients.push_back( Grad(i, j) );
            }
         }
      }
   }
}

void PointMap::makeConstants()
{
   if (g_primes.size() == 0) {
      makePrimes();
   }
   if (g_gradients.size() == 0) {
      makeGradients();
   }
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

   int index = m_attributes.getRowid(pix);
   if (index == -1) {
      val = -2;
   }
   else {
      val = m_attributes.getValue(index,m_displayed_attribute);
   }

   return val;
}

/////////////////////////////////////////////////////////////////////////////////
// Update connections will load an old graph and add char information

void PointMap::addGridConnections()
{
   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
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
         char dir;
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
