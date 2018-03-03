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

#include <math.h>
#include <float.h>
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/comm.h> // for communicator
#include <genlib/stringutils.h>
#include <genlib/exceptions.h>
#include <genlib/legacyconverters.h>

#include <salalib/mgraph.h> // purely for the version info --- as phased out should replace
#include <salalib/shapemap.h>
#include "genlib/containerutils.h"

#include <stdexcept>
// for mapinfo interface
#include "MapInfoData.h"

#include <unordered_map>

#ifndef _WIN32
#define _finite finite
#endif

static const double TOLERANCE_A = 1e-9;

// import TOLERANCE_B from axial map...
static const double TOLERANCE_B = 1e-12;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SalaShape::read(istream& stream, int version)
{
   // defaults
   m_draworder = -1; 
   m_selected = false;

   stream.read((char *)&m_type,sizeof(m_type));

   stream.read((char *)&m_region,sizeof(m_region));

   stream.read((char *)&m_centroid,sizeof(m_centroid));
      
   stream.read((char *)&m_area,sizeof(m_area));
   stream.read((char *)&m_perimeter,sizeof(m_perimeter));

   pqvector<Point2f>::read(stream);

   return true;
}

bool SalaShape::write(ofstream& stream)
{
   stream.write((char *)&m_type,sizeof(m_type));
   stream.write((char *)&m_region,sizeof(m_region));
   stream.write((char *)&m_centroid,sizeof(m_centroid));
   stream.write((char *)&m_area,sizeof(m_area));
   stream.write((char *)&m_perimeter,sizeof(m_perimeter));
   pqvector<Point2f>::write(stream);
   return true;
}

void SalaShape::setCentroidAreaPerim()
{
   m_area = 0.0;
   m_perimeter = 0.0;
   m_centroid = Point2f(0,0);
   for (size_t i = 0; i < size(); i++) {
      Point2f& p1 = at(i);
      Point2f& p2 = at((i+1)%size());
      double a_i = (p1.x * p2.y - p2.x * p1.y) / 2.0;
      m_area += a_i;
      a_i /= 6.0;
      m_centroid.x += (p1.x+p2.x) * a_i;
      m_centroid.y += (p1.y+p2.y) * a_i;
      Point2f side = p2 - p1;
      m_perimeter += side.length();
   }
   m_type &= ~SHAPE_CCW;
   if (sgn(m_area) == 1) {
      m_type |= SHAPE_CCW;
   }
   m_centroid.scale(2.0/m_area); // note, *not* fabs(m_area) as it is then confused by clockwise ordered shapes
   m_area = fabs(m_area);
   if (isOpen()) {
      // take off the automatically collected final side
      Point2f side = tail() - head();
      m_perimeter -= side.length();
   }
}

// allows override of the above (used for isovists)
void SalaShape::setCentroid(const Point2f& p)
{
   m_centroid = p;
}

// get the angular deviation along the length of a poly line:
double SalaShape::getAngDev() const
{
   double dev = 0.0;
   for (size_t i = 1; i < size() - 1; i++) {
      double ang = angle(at(i-1),at(i),at(i+1));
      
      // Quick mod - TV
#if defined(_WIN32)      
      dev += abs(M_PI - ang);
#else
      (M_PI - ang) < 0.0 ? dev += (ang - M_PI) : dev += (M_PI - ang);
#endif      
   }
   // convert to Iida Hillier units (0 to 2):
   dev /= M_PI * 0.5;
   return dev;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// the replacement for datalayers

ShapeMap::ShapeMap(const std::string& name, int type) : m_attributes(name)
{
   m_name = name;
   m_map_type = type;
   m_hasgraph = false;

   // shape and object counters
   m_obj_ref = -1;
   m_shape_ref = -1;
   // pixel map
   m_pixel_shapes = NULL;
   //
   // -1 is the shape ref column (which will be shown by default)
   m_displayed_attribute = -1; 
   m_display_shapes = NULL;
   m_invalidate = false;
   // for polygons:
   m_show_lines = true;
   m_show_fill = true;
   m_show_centroids = false;

   // data (MUST be set before use)
   m_tolerance = 0.0;

   m_selection = false;

   // note show is 
   m_show = true; 
   m_editable = false;

   m_bsp_tree = false;
   m_bsp_root = NULL;
   //
   m_mapinfodata = NULL;
}

ShapeMap::~ShapeMap()
{
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   if (m_pixel_shapes) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_shapes[i];
      }
      delete [] m_pixel_shapes;
      m_pixel_shapes = NULL;
   }
   if (m_display_shapes) {
      delete [] m_display_shapes;
      m_display_shapes = NULL;
   }
   if (m_mapinfodata) {
      delete m_mapinfodata;
      m_mapinfodata = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////

// this can be reinit as well

void ShapeMap::init(int size, const QtRegion &r)
{
   if (m_pixel_shapes) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_shapes[i];
      }
      delete [] m_pixel_shapes;
      m_pixel_shapes = NULL;
   }
   if (m_display_shapes) {
      delete [] m_display_shapes;
      m_display_shapes = NULL;
   }
   m_rows = __min(__max(20,(int)sqrt((double)size)),32768);
   m_cols = __min(__max(20,(int)sqrt((double)size)),32768);
   if (m_region.atZero()) {
      m_region = r;
   }
   else {
      m_region = runion(m_region,r);
   }
   // calculate geom data:
   m_tolerance = __max(m_region.width(), m_region.height()) * TOLERANCE_A;
   //
   m_pixel_shapes = new pqvector<ShapeRef> *[m_cols];
   for (int i = 0; i < m_cols; i++) {
      m_pixel_shapes[i] = new pqvector<ShapeRef>[m_rows];
   }
}

// this makes an exact copy, keep the reference numbers and so on:

void ShapeMap::copy(const ShapeMap& sourcemap, int copyflags)
{
   if ((copyflags & ShapeMap::COPY_GEOMETRY) == ShapeMap::COPY_GEOMETRY) {
      m_shapes.clear();
      m_shape_ref = -1;
      init(sourcemap.m_shapes.size(),sourcemap.m_region);
      for (size_t i = 0; i < sourcemap.m_shapes.size(); i++) {
         // using makeShape is actually easier than thinking about a total copy:
         makeShape(sourcemap.m_shapes.value(i),sourcemap.m_shapes.key(i));
         // note that addShape automatically adds the attribute row
      }
   }

   if ((copyflags & ShapeMap::COPY_ATTRIBUTES) == ShapeMap::COPY_ATTRIBUTES) {
      // assumes attribute rows are filled in already
      for (int incol = 0; incol < sourcemap.m_attributes.getColumnCount(); incol++) {
         int outcol = m_attributes.insertColumn(sourcemap.m_attributes.getColumnName(incol));
         // n.b. outcol not necessarily the same as incol, although row position in table (j) should match
         for (int j = 0; j < sourcemap.m_attributes.getRowCount(); j++) {
            m_attributes.setValue(j,outcol,sourcemap.m_attributes.getValue(j,incol));
         }
      }
   }

   if ((copyflags & ShapeMap::COPY_ATTRIBUTES) == ShapeMap::COPY_GRAPH) {
      if (sourcemap.m_hasgraph) {
         m_hasgraph = true;
         // straight copy:
         m_connectors = sourcemap.m_connectors;
         m_links = sourcemap.m_links;
         m_unlinks = sourcemap.m_unlinks;
      }
   }

   // copies mapinfodata (projection data) regardless of copy flags
   if (sourcemap.getMapInfoData()) {
      if (m_mapinfodata != NULL) {
         delete m_mapinfodata;
      }
      m_mapinfodata = new MapInfoData;
      m_mapinfodata->m_coordsys = sourcemap.getMapInfoData()->m_coordsys;
      m_mapinfodata->m_bounds = sourcemap.getMapInfoData()->m_bounds;
   }
}

// Zaps all memory structures, apart from mapinfodata
void ShapeMap::clearAll()
{
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   if (m_pixel_shapes) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_shapes[i];
      }
      delete [] m_pixel_shapes;
      m_pixel_shapes = NULL;
   }
   if (m_display_shapes) {
      delete [] m_display_shapes;
      m_display_shapes = NULL;
   }

   m_shapes.clear();
   m_objects.clear();
   m_undobuffer.clear();
   m_connectors.clear();
   m_attributes.clear();
   m_links.clear();
   m_unlinks.clear();
   m_region = QtRegion();

   m_obj_ref = -1;
   m_shape_ref = -1;
   m_displayed_attribute = -1; 
}


///////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::makePointShape(const Point2f& point, bool tempshape)
{
   bool bounds_good = true;

   if (!m_region.contains_touch(point)) {
      bounds_good = false;
      init(m_shapes.size(),QtRegion(point,point));
   }

   m_shape_ref++;
   int x = m_shapes.add(m_shape_ref,SalaShape(point));

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   if (!tempshape) {
      int rowid = m_attributes.insertRow(m_shape_ref);
      m_newshape = true;
   }

   return m_shape_ref;
}

int ShapeMap::makeLineShape(const Line& line, bool through_ui, bool tempshape)
{
   // note, map must have editable flag on if we are to make a shape through the user interface:
   if (through_ui && !m_editable) {
      return -1;
   }

   bool bounds_good = true;

   if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
      bounds_good = false;
      init(m_shapes.size(),line);
   }
  
   m_shape_ref++;
   // note, shape constructor sets centroid, length etc
   int rowid = m_shapes.add(m_shape_ref,SalaShape(line));

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   if (!tempshape) {
      m_attributes.insertRow(m_shape_ref);
      m_newshape = true;
   }

   if (through_ui) {
      //
      // manually add connections:
      if (m_hasgraph) {
         if (isAxialMap()) {
            connectIntersected(rowid,true); // "true" means line-line intersections only will be applied
         }
         else {
            connectIntersected(rowid,false);
         }
      }
      // if through ui, set undo counter:
      m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED,m_shape_ref));
      // update displayed attribute if through ui:
      invalidateDisplayedAttribute();
      setDisplayedAttribute(m_displayed_attribute);
   }

   return m_shape_ref;
}

int ShapeMap::makePolyShape(const pqvector<Point2f>& points, bool open, bool tempshape)
{
   bool bounds_good = true;

   switch (points.size()) {
   case 0:
      return -1;
   case 1:
      return makePointShape(points[0],tempshape);
   case 2:
      return makeLineShape(Line(points[0],points[1]),false,tempshape);  // false is not through ui: there really should be a through ui here?
   }

   QtRegion region(points[0],points[0]);
   size_t i;
   for (i = 1; i < points.size(); i++) {
      region.encompass(points[i]);
   }
   if (!m_region.contains_touch(region.bottom_left) || !m_region.contains_touch(region.top_right)) {
      bounds_good = false;
      init(m_shapes.size(),region);
   }

   m_shape_ref++;

   size_t len = points.size();
   // NOTE: This is commented out deliberately
   // Sometimes you really do want a polyline that forms a loop
   /*
   if (points.head() == points.tail()) {
      len--;
      open = false;
   }
   */

   // not sure if it matters if the polygon is clockwise or anticlockwise... we'll soon tell!

   if (open) {
      m_shapes.add(m_shape_ref,SalaShape(SalaShape::SHAPE_POLY));
   }
   else {
      m_shapes.add(m_shape_ref,SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED));
   }
   for (i = 0; i < len; i++) {
      m_shapes.tail().push_back(points[i]);
   }

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   if (!tempshape) {
      // set centroid now also adds a few other things: as well as area, perimeter
      m_shapes.tail().setCentroidAreaPerim();
      m_attributes.insertRow(m_shape_ref);
      m_newshape = true;
   }

   return m_shape_ref;
}

int ShapeMap::makeShape(const SalaShape& poly, int override_shape_ref)
{
   // overridden shape cannot exist:
   if (override_shape_ref != -1 && m_shapes.searchindex(override_shape_ref) != paftl::npos) {
      return -1;  // failure!
   }

   bool bounds_good = true;

   if (!m_region.contains_touch(poly.m_region.bottom_left) || !m_region.contains_touch(poly.m_region.top_right)) {
      bounds_good = false;
      init(m_shapes.size(),poly.m_region);
   }

   int shape_ref;
   if (override_shape_ref == -1) {
      m_shape_ref++;
      shape_ref = m_shape_ref;
   }
   else {
      if (override_shape_ref > m_shape_ref) {
         m_shape_ref = override_shape_ref;
      }
      shape_ref = override_shape_ref;
   }

   int rowid1 = m_shapes.add(shape_ref,poly);

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   int rowid2 = m_attributes.insertRow(shape_ref);

#ifdef _DEBUG
   if (rowid1 != rowid2) {
      // rowids should match, they're both pqmaps, but if someone is stupid enough to change it, they'll know pretty quickly:
      throw depthmapX::RuntimeException("Arrrrgghhh: important! insertRow does not index in the same way as add shapes, this will badly mess up the system!");
   }
#endif

   m_newshape = true;

   return shape_ref;
}


//////////////////////////////////////////////////////////////////////////////////////////

// n.b., only works from current selection (and uses point selected attribute)

int ShapeMap::makeShapeFromPointSet(const PointMap& pointmap)
{
   bool bounds_good = true;
   PixelRefVector selset;
   Point2f offset = Point2f(pointmap.getSpacing()/2,pointmap.getSpacing()/2);
   for (auto &sel: pointmap.getSelSet()) {
      selset.push_back(sel);
      if (!m_region.contains_touch(pointmap.depixelate(sel)-offset) || !m_region.contains_touch(pointmap.depixelate(sel)+offset)) {
         bounds_good = false;
      }
   }
   if (!bounds_good) {
      QtRegion r(pointmap.getRegion().bottom_left - offset,pointmap.getRegion().top_right + offset);
      init(m_shapes.size(),r);
   }
   pmap<int,int> relations;
   for (size_t j = 0; j < selset.size(); j++) {
      PixelRef pix = selset[j];
      int x = relations.add(pix,ShapeRef::SHAPE_EDGE);
      if (pointmap.includes(pix.right()) && pointmap.getPoint(pix.right()).selected()) {
         relations.value(x) &= ~ShapeRef::SHAPE_R;
      }
      if (pointmap.includes(pix.up()) && pointmap.getPoint(pix.up()).selected()) {
         relations.value(x) &= ~ShapeRef::SHAPE_T;
      }
      if (pointmap.includes(pix.down()) && pointmap.getPoint(pix.down()).selected()) {
         relations.value(x) &= ~ShapeRef::SHAPE_B;
      }
      if (pointmap.includes(pix.left()) && pointmap.getPoint(pix.left()).selected()) {
         relations.value(x) &= ~ShapeRef::SHAPE_L;
      }
   }
   // now find pixel with SHAPE_B | SHAPE_L
   PixelRef minpix = NoPixel;
   size_t k;
   for (k = 0; k < relations.size(); k++) {
      if ((relations.value(k) & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) == (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
         if ((minpix == NoPixel) || (relations.key(k) < (int)minpix)) {
            minpix = relations.key(k);
         }
      }
   }
   // now follow round anticlockwise...
   SalaShape poly(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
   pointPixelBorder(pointmap,relations,poly,ShapeRef::SHAPE_L,minpix,minpix,true);

   bool retvar = true;

   for (k = 0; k < relations.size(); k++) {
      if (relations[k] != 0) {
         // more than one shape!
         return -1;
      }
   }
   poly.setCentroidAreaPerim();

   m_shape_ref++;
   int rowid = m_shapes.add(m_shape_ref,poly);

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }
   
   m_attributes.insertRow(m_shape_ref);
   m_newshape = true;
   
   return m_shape_ref;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::convertPointsToPolys(double poly_radius, bool selected_only)
{
   // I'm not sure quite how easy this will be...
   QtRegion region;

   bool done_something = false;

   // replace the points with polys
   for (size_t i = 0; i < m_shapes.size(); i++) {
      if (selected_only && !m_attributes.isSelected(i)) {
         continue;
      }
      if (m_shapes[i].isPoint()) {
         done_something = true;
         // remove old spatial index
         removePolyPixels(m_shapes.key(i));
         // construct a poly from the point:
         Point2f p = m_shapes[i].getCentroid();
         //
         if (region.atZero()) {
            region = QtRegion(p,p);
         }
         // replace with a polygon:
         m_shapes[i] = SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
         for (int k = 0; k < 8 ; k++) {
            Point2f poly_p;
            if (k == 0)      { poly_p.x = p.x + poly_radius;              poly_p.y = p.y; }
            else if (k == 1) { poly_p.x = p.x + poly_radius * M_ROOT_1_2; poly_p.y = p.y + poly_radius * M_ROOT_1_2; } 
            else if (k == 2) { poly_p.x = p.x;                            poly_p.y = p.y + poly_radius; } 
            else if (k == 3) { poly_p.x = p.x - poly_radius * M_ROOT_1_2; poly_p.y = p.y + poly_radius * M_ROOT_1_2; } 
            else if (k == 4) { poly_p.x = p.x - poly_radius;              poly_p.y = p.y; }
            else if (k == 5) { poly_p.x = p.x - poly_radius * M_ROOT_1_2; poly_p.y = p.y - poly_radius * M_ROOT_1_2; } 
            else if (k == 6) { poly_p.x = p.x;                            poly_p.y = p.y - poly_radius; } 
            else if (k == 7) { poly_p.x = p.x + poly_radius * M_ROOT_1_2; poly_p.y = p.y - poly_radius * M_ROOT_1_2; } 
            region.encompass(poly_p);
            m_shapes[i].push_back(poly_p);
         }
         m_shapes[i].setCentroidAreaPerim();
      }
   }

   if (done_something) {
      // spatially reindex (simplest just to redo everything)
      init(m_shapes.size(),region);

      for (size_t j = 0; j < m_shapes.size(); j++) {
         makePolyPixels(m_shapes.key(j));
      }
   }

   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::moveShape(int shaperef, const Line& line, bool undoing)
{
   bool bounds_good = true;

   size_t rowid = m_shapes.searchindex(shaperef);
   if (rowid == paftl::npos) {
      return false;
   }

   // remove shape from the pixel grid
   removePolyPixels(shaperef); // done first, as all interface references use this list

   if (!undoing) {
      // set undo counter, but only if this is not an undo itself:
      m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_MOVED,shaperef));
      m_undobuffer.tail().m_geometry = m_shapes.value(rowid);
      m_undobuffer.tail().m_geometry.m_selected = false; // <- this m_selected really shouldn't be used -- should use attributes, but for some reason it is!
   }

   if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
      bounds_good = false;
      init(m_shapes.size(),line);
   }

   m_shapes.value(rowid) = SalaShape(line);

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(shaperef);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   // change connections:
   if (m_hasgraph) {
      //
      pvecint oldconnections = m_connectors[rowid].m_connections;
      m_connectors[rowid].m_connections.clear();
      pvecint& newconnections = m_connectors[rowid].m_connections;
      //
      int conn_col = m_attributes.getOrInsertLockedColumnIndex("Connectivity");
      int leng_col = -1;
      //
      int connectivity = 0;
      if (isAxialMap()) {
         // line connections optimised for line-line intersection
         connectivity = getLineConnections( shaperef, newconnections, TOLERANCE_B*__max(m_region.height(),m_region.width()));
      }
      else {
         connectivity = getShapeConnections( shaperef, newconnections, TOLERANCE_B*__max(m_region.height(),m_region.width()));
      }
      m_attributes.setValue(rowid, conn_col, (float) connectivity );
      if (isAxialMap()) {
         leng_col = m_attributes.getOrInsertLockedColumnIndex("Line Length");
         m_attributes.setValue(rowid, leng_col, (float) m_shapes[rowid].getLength() );
      }
      //
      size_t k = 0;
      // now go through our old connections, and remove ourself:
      for (k = 0; k < oldconnections.size(); k++) {
         int myplace = oldconnections[k];
         if (myplace != rowid) { // <- exclude self!
            m_connectors[myplace].m_connections.remove(rowid);
            m_attributes.decrValue(myplace,conn_col);
         }
      }
      // now go through our new connections, and add ourself:
      for (k = 0; k < newconnections.size(); k++) {
         int myplace = newconnections[k];
         if (myplace != rowid) { // <- exclude self!
            m_connectors[myplace].m_connections.add(rowid);
            m_attributes.incrValue(myplace,conn_col);
         }
      }
      // now check any unlinks still exist in our newconnections are unlinked again (argh...)
      for (k = m_unlinks.size() - 1; k != paftl::npos; k--) {
         int connb = -1;
         if (m_unlinks[k].a == rowid)
            connb = m_unlinks[k].b;
         else if (m_unlinks[k].b == rowid) 
            connb = m_unlinks[k].a;
         if (connb != -1) {
            if (newconnections.searchindex(connb) == paftl::npos) {
               // no longer required:
               m_unlinks.remove_at(k);
            }
            else {
               // enforce:
               newconnections.remove(connb);
               m_connectors[connb].m_connections.remove(rowid);
               m_attributes.decrValue(connb,conn_col);
               m_attributes.decrValue(rowid,conn_col);
            }
         }
      }
      // now check any links are actually required (argh...)
      for (k = m_links.size() - 1; k != paftl::npos; k--) {
         int connb = -1;
         if (m_links[k].a == rowid)
            connb = m_links[k].b;
         else if (m_links[k].b == rowid) 
            connb = m_links[k].a;
         if (connb != -1) {
            if (newconnections.searchindex(connb) != paftl::npos) {
               // no longer required:
               m_links.remove_at(k);
            }
            else {
               // enforce:
               newconnections.add(connb);
               m_connectors[connb].m_connections.add(rowid);
               m_attributes.incrValue(connb,conn_col);
               m_attributes.incrValue(rowid,conn_col);
            }
         }
      }
      // update displayed attribute for any changes:
      invalidateDisplayedAttribute();
      setDisplayedAttribute(m_displayed_attribute);
   }

   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// some functions to make a polygon from the UI

int ShapeMap::polyBegin(const Line& line)
{
   // add geometry
   bool bounds_good = true;
   if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
      bounds_good = false;
      init(m_shapes.size(),line);
   }
   m_shape_ref++;
   int rowid = m_shapes.add(m_shape_ref,SalaShape(line));
   m_shapes.tail().m_centroid = line.getCentre();

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   // insert into attributes
   m_attributes.insertRow(m_shape_ref);
   // would usually set attributes here, but actually, really want
   // to set the attributes only when the user completes the drawing

   // change connections:
   if (m_hasgraph) {
      // dummy for now to ensure there is a row in the connector table
      // so all indices match...
      m_connectors.push_back( Connector() );
   }

   // flag new shape
   m_newshape = true;

   // set undo counter:
   m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED,m_shape_ref));

   // update displayed attribute
   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);

   return m_shape_ref;
}

bool ShapeMap::polyAppend(const Point2f& point)
{
   // don't do anything too complex:
   SalaShape& shape = m_shapes.tail();

   // check you can actually do this first
   if (!(shape.isLine() || shape.isPolyLine())) {
      return false;
   }

   // junk the old shape pixels:
   removePolyPixels(m_shape_ref);
   
   bool bounds_good = true;
   if (!m_region.contains_touch(point)) {
      bounds_good = false;
      init(m_shapes.size(),QtRegion(point,point));
   }

   if (shape.m_type == SalaShape::SHAPE_LINE) {
      // convert it to a poly line:
      shape.m_type = SalaShape::SHAPE_POLY;
      shape.push_back(shape.m_region.t_start());
      shape.push_back(shape.m_region.t_end());
   }
   // add new point:
   shape.push_back(point);

   if (bounds_good) {
      // note: also sets polygon bounding box:
      makePolyPixels(m_shape_ref);
   }
   else {
      // pixelate all polys in the pixel new structure:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         makePolyPixels(m_shapes.key(i));
      }
   }

   shape.setCentroidAreaPerim();

   return true;
}

bool ShapeMap::polyClose()
{
   // don't do anything too complex:
   SalaShape& shape = m_shapes.tail();

   // check you can actually do this first
   if (!shape.isPolyLine()) {
      return false;
   }

   // junk the old shape pixels:
   removePolyPixels(m_shape_ref);

   shape.m_type |= SalaShape::SHAPE_CLOSED;

   makePolyPixels(m_shape_ref);

   return true;
}

bool ShapeMap::polyCancel()
{
   // don't do anything too complex:
   SalaShape& shape = m_shapes.tail();

   // check you can actually do this first
   if (!(shape.isLine() || shape.isPolyLine())) {
      return false;
   }

   m_undobuffer.pop_back();
   removeShape(m_shape_ref,true);

   // update displayed attribute
   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);

   return true;
}

// some functions to make a polygon from the DLL interface or scripting language

void ShapeMap::shapeBegin()
{
   m_temppoints.clearnofree();
}

void ShapeMap::shapeVertex(const Point2f& p)
{
   m_temppoints.push_back(p);
}

int ShapeMap::shapeEnd(bool open)
{
   int len = m_temppoints.size();
   if (len == 0) {
      return -1;
   }

   if (m_region.atZero()) {
      m_region = QtRegion(m_temppoints[0],m_temppoints[0]);
   }
   for (int i = 0; i < len; i++) {
      m_region.encompass(m_temppoints[i]);
   }

   m_shape_ref++;

   int rowid = -1;
   
   if (len == 1) {
      rowid = m_shapes.add(m_shape_ref,SalaShape(m_temppoints[0]));
   }
   else if (len == 2) {
      rowid = m_shapes.add(m_shape_ref,SalaShape(Line(m_temppoints[0],m_temppoints[1])));
   }
   else {
      if (open) {
         rowid = m_shapes.add(m_shape_ref,SalaShape(SalaShape::SHAPE_POLY));
      }
      else {
         rowid = m_shapes.add(m_shape_ref,SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED));
      }
      if (rowid != -1) {
         SalaShape& shape = m_shapes[rowid];
         for (int i = 0; i < len; i++) {
            shape.push_back(m_temppoints[i]);
         }
         shape.setCentroidAreaPerim();
      }
   }

   if (rowid != -1) {
      m_attributes.insertRow(m_shape_ref);

      if (m_hasgraph) {
         while (m_connectors.size() < m_shapes.size()) {
            m_connectors.push_back( Connector() );
         }
      }
   }

   m_newshape = true;

   // note: this function returns rowid *not* shaperef
   return rowid;
}

// very simple, just remake the poly pixels
void ShapeMap::shapesCommit()
{
   init(m_shapes.size(),m_region);

   for (size_t i = 0; i < m_shapes.size(); i++) {
      makePolyPixels(m_shapes.key(i));
   }

   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::removeSelected()
{
   // note, map must have editable flag on if we are to remove shape:
   if (!m_editable) {
      return false;
   }

   // pray that the selection set is in order! 
   // (it should be: code currently uses add() throughout)
   for (auto &sel: m_selection_set) {
      removeShape(m_shapes.key(sel));
   }
   m_selection_set.clear();
   m_selection = false;

   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);
   return true;
}

void ShapeMap::removeShape(int shaperef, bool undoing)
{
   // remove shape from four keys: the pixel grid, the poly list, the attributes and the connections
   removePolyPixels(shaperef); // done first, as all interface references use this list

   size_t rowid = m_shapes.searchindex(shaperef);

   if (!undoing) { // <- if not currently undoing another event, then add to the undo buffer:
      m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_DELETED,shaperef));
      m_undobuffer.tail().m_geometry = m_shapes.value(rowid);
      m_undobuffer.tail().m_geometry.m_selected = false; // <- this m_selected really shouldn't be used -- should use attributes, but for some reason it is!
   }

   if (rowid != paftl::npos) {
      m_shapes.remove_at(rowid);
   }
   
   if (m_hasgraph) {
      // note that the connections have no key for speed when processing,
      // we rely on the index order matching the index order of the shapes
      // and the attributes, and simply change all references (ick!)
      int conn_col = m_attributes.getColumnIndex("Connectivity");
      for (size_t i = m_connectors.size() - 1; i != paftl::npos; i--) {
         if (i == rowid) {
            continue;   // it's going to be removed anyway
         }
         for (size_t j = m_connectors[i].m_connections.size() - 1; j != paftl::npos; j--) {
            if (m_connectors[i].m_connections[j] == rowid) {
               m_connectors[i].m_connections.remove_at(j);
               if (conn_col != -1) {
                  m_attributes.decrValue(i,conn_col);
               }
            }
            else if (m_connectors[i].m_connections[j] > (int)rowid) {
               m_connectors[i].m_connections[j] -= 1;
            }
         }
         // note, you cannot delete from a segment map, it's just too messy!
      }

      m_connectors.remove_at(rowid);

      // take out explicit links and unlinks (note, undo won't restore these):
      size_t k;
      for (k = m_links.size() - 1; k != paftl::npos; k--) {
         if (m_links[k].a == rowid || m_links[k].b == rowid) {
            m_links.remove_at(k);
         }
         else {
            if (m_links[k].a > (int)rowid)
               m_links[k].a -= 1;
            if (m_links[k].b > (int)rowid)
               m_links[k].b -= 1;
         }
      }
      for (k = m_unlinks.size() - 1; k != paftl::npos; k--) {
         if (m_unlinks[k].a == rowid || m_unlinks[k].b == rowid) {
            m_unlinks.remove_at(k);
         }
         else {
            if (m_unlinks[k].a > (int)rowid)
               m_unlinks[k].a -= 1;
            if (m_unlinks[k].b > (int)rowid)
               m_unlinks[k].b -= 1;
         }
      }
   }

   // n.b., shaperef should have been used to create the row in the first place:
   m_attributes.removeRow(shaperef);

   // if undoing the final shape, might as well reuse the shape ref:
   if (undoing && shaperef == m_shape_ref) {
      m_shape_ref--;
   }

   m_newshape = true;
   m_invalidate = true;
}

void ShapeMap::undo()
{
   if (m_undobuffer.size() == 0) {
      return;
   }

   SalaEvent& event = m_undobuffer.tail();

   if (event.m_action == SalaEvent::SALA_CREATED) {
   
      removeShape(event.m_shape_ref, true);  // <- note, must tell remove shape it's an undo, or it will add this remove to the undo stack!

   }
   else if (event.m_action == SalaEvent::SALA_DELETED) {

      makeShape(event.m_geometry,event.m_shape_ref);
      int rowid = m_attributes.getRowid(event.m_shape_ref);

      if (rowid != -1 && m_hasgraph) {
         // redo connections... n.b. TO DO this is intended to use the slower "any connection" method, so it can handle any sort of graph
         // ...but that doesn't exist yet, so for the moment do lines:
         //
         // insert new connector at the row:
         m_connectors.insert_at(rowid,Connector());
         //
         // now go through all connectors, ensuring they're reindexed above this one:
         // Argh!  ...but, remember the reason we're doing this is for fast processing elsewhere
         // -- this is a user triggered *undo*, they'll just have to wait:
         for (size_t i = 0; i < m_connectors.size(); i++) {
            for (size_t j = 0; j < m_connectors[i].m_connections.size(); j++) {
               if (m_connectors[i].m_connections[j] >= rowid) {
                  m_connectors[i].m_connections[j] += 1;
               }
            }
         }
         // it gets worse, the links and unlinks will also be all over the shop due to the inserted row:
         size_t j;
         for (j = 0; j < m_links.size(); j++) {
            if (m_links[j].a >= rowid)
               m_links[j].a += 1;
            if (m_links[j].b >= rowid)
               m_links[j].b += 1;
         }
         for (j = 0; j < m_unlinks.size(); j++) {
            if (m_unlinks[j].a >= rowid)
               m_unlinks[j].a += 1;
            if (m_unlinks[j].b >= rowid)
               m_unlinks[j].b += 1;
         }
         //
         // calculate this line's connections
         int connectivity = getLineConnections(event.m_shape_ref,m_connectors[rowid].m_connections,TOLERANCE_B*__max(m_region.height(),m_region.width()));
         // update:
         int conn_col = m_attributes.getOrInsertLockedColumnIndex("Connectivity");
         m_attributes.setValue(rowid, conn_col, (float) connectivity );
         //
         if (event.m_geometry.isLine()) {
            int leng_col = m_attributes.getOrInsertLockedColumnIndex("Line Length");
            m_attributes.setValue(rowid, leng_col, (float) m_shapes[rowid].getLength() );
         }
         //
         // now go through our connections, and add ourself:
         for (size_t k = 0; k < m_connectors[rowid].m_connections.size(); k++) {
            int myplace = m_connectors[rowid].m_connections[k];
            if (myplace != rowid) { // <- exclude self!
               m_connectors[myplace].m_connections.add(rowid);
               m_attributes.incrValue(myplace,conn_col);
            }
         }
      }
   }
   else if (event.m_action == SalaEvent::SALA_MOVED) {

      moveShape(event.m_shape_ref, event.m_geometry.getLine(), true);  // <- note, must tell remove shape it's an undo, or it will add this remove to the undo stack!

   }

   m_undobuffer.pop_back();

   m_newshape = true;

   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::makePolyPixels(int polyref)
{
   // first add into pixels, and ensure you have a bl, tr for the set (useful for testing later)
   SalaShape& poly = m_shapes.search(polyref);
   if (poly.isClosed()) {
      pmap<int,int> relations;
      for (size_t k = 0; k < poly.size(); k++) {
         int nextk = (k + 1) % poly.size();
         Line li(poly[k],poly[nextk]);
         if (k == 0) {
            poly.m_region = li;
         }
         else {
            poly.m_region = runion(poly.m_region,li);
         }
         PixelRefVector pixels = pixelateLine(li);
         // debug
         // int duplicate_shaperefs = 0;
         // end debug
         for (size_t i = 0; i < pixels.size(); i++) {
            PixelRef pix = pixels[i];
            size_t x = m_pixel_shapes[pix.x][pix.y].searchindex(ShapeRef(polyref));
            if (x == paftl::npos) {
               x = m_pixel_shapes[pix.x][pix.y].add(ShapeRef(polyref),paftl::ADD_HERE);
            }
            m_pixel_shapes[pix.x][pix.y][x].m_polyrefs.push_back(k);
            relations.add(pixels[i],ShapeRef::SHAPE_EDGE);
         }
      }
      // erase joined sides, and look for min:
      PixelRef minpix = NoPixel; 
      for (size_t j = 0; j < relations.size(); j++) {
         PixelRef pix = relations.key(j);
         PixelRef nextpix;
         nextpix = pix.right();
         if (includes(nextpix) && m_pixel_shapes[nextpix.x][nextpix.y].searchindex(ShapeRef(polyref)) != paftl::npos) {
            relations.value(j) &= ~ShapeRef::SHAPE_R;
         }
         nextpix = pix.up();
         if (includes(nextpix) && m_pixel_shapes[nextpix.x][nextpix.y].searchindex(ShapeRef(polyref)) != paftl::npos) {
            relations.value(j) &= ~ShapeRef::SHAPE_T;
         }
         nextpix = pix.down();
         if (includes(nextpix) && m_pixel_shapes[nextpix.x][nextpix.y].searchindex(ShapeRef(polyref)) != paftl::npos) {
            relations.value(j) &= ~ShapeRef::SHAPE_B;
         }
         nextpix = pix.left();
         if (includes(nextpix) && m_pixel_shapes[nextpix.x][nextpix.y].searchindex(ShapeRef(polyref)) != paftl::npos) {
            relations.value(j) &= ~ShapeRef::SHAPE_L;
         }
         if ((relations.value(j) & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) == (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
            if ((minpix == NoPixel) || (relations.key(j) < (int)minpix)) {
               minpix = relations.key(j);
            }
         }
      }
      shapePixelBorder(relations,polyref,ShapeRef::SHAPE_L,minpix,minpix,true);
      // go through any that aren't on the outer border: this will be internal edges, and will cause problems
      // for point in polygon algorithms!
      size_t i;
      for (i = 0; i < relations.size(); i++) {
         PixelRef pix = relations.key(i);
         unsigned char& tags = m_pixel_shapes[pix.x][pix.y].search(polyref).m_tags;
         if (tags == 0x00) {
            tags |= ShapeRef::SHAPE_INTERNAL_EDGE;
         }
      }
      // now, any remaining tags are internal sides, and need to be cleared through fill
      // we could go either direction, but we just go left to right:
      for (i = 0; i < relations.size(); i++) {
         PixelRef pix = relations.key(i);
         if (relations.value(i) & ShapeRef::SHAPE_R) {
            int pos = 0;
            do {
               PixelRef nextpix = pix.right();
               if (!includes(nextpix)) {
                  // this shouldn't happen
                  break;
               }
               // returns -1 if cannot add due to already existing:
               pos = m_pixel_shapes[nextpix.x][nextpix.y].add(ShapeRef(polyref,ShapeRef::SHAPE_CENTRE));
               pix = nextpix;
            } while (pos != -1);
         }
      }
      // Done...! This polygon is registered in the pixel polygon structure
   }
   else {
      // Open shapes much easier!
      switch (poly.m_type & SalaShape::SHAPE_TYPE)
      { 
      case SalaShape::SHAPE_POINT:
         {
            PixelRef pix = pixelate(poly.m_centroid);
            size_t x = m_pixel_shapes[pix.x][pix.y].searchindex(ShapeRef(polyref));
            if (x == paftl::npos) {
               x = m_pixel_shapes[pix.x][pix.y].add(ShapeRef(polyref,ShapeRef::SHAPE_OPEN),paftl::ADD_HERE);
            }
         }
         break;
      case SalaShape::SHAPE_LINE:
         {
            PixelRefVector pixels = pixelateLine(poly.m_region);
            for (size_t i = 0; i < pixels.size(); i++) {
               PixelRef pix = pixels[i];
               size_t x = m_pixel_shapes[pix.x][pix.y].searchindex(ShapeRef(polyref));
               if (x == paftl::npos) {
                  x = m_pixel_shapes[pix.x][pix.y].add(ShapeRef(polyref,ShapeRef::SHAPE_OPEN),paftl::ADD_HERE);
               }
            }
         }
         break;
      case SalaShape::SHAPE_POLY:
         for (size_t k = 0; k < poly.size() - 1; k++) {
            int nextk = (k + 1);
            Line li(poly[k],poly[nextk]);
            if (k == 0) {
               poly.m_region = li;
            }
            else {
               poly.m_region = runion(poly.m_region,li);
            }
            PixelRefVector pixels = pixelateLine(li);
            for (size_t i = 0; i < pixels.size(); i++) {
               PixelRef pix = pixels[i];
               size_t x = m_pixel_shapes[pix.x][pix.y].searchindex(ShapeRef(polyref));
               if (x == paftl::npos) {
                  x = m_pixel_shapes[pix.x][pix.y].add(ShapeRef(polyref,ShapeRef::SHAPE_OPEN),paftl::ADD_HERE);
               }
               m_pixel_shapes[pix.x][pix.y][x].m_polyrefs.push_back(k);
            }
         }
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::shapePixelBorder(pmap<int,int>& relations, int polyref, int side, PixelRef currpix, PixelRef minpix, bool first) 
{
   if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
      // looped:
      return;
   }
   size_t rel = relations.searchindex(currpix);
   if (relations[rel] & side) {
      m_pixel_shapes[currpix.x][currpix.y].search(polyref).m_tags |= side;
      relations[rel] &= ~side;   // <- clear to check all have been done later
      side <<= 1;
      if (side > ShapeRef::SHAPE_T) {
         side = ShapeRef::SHAPE_L;
      }
      shapePixelBorder(relations,polyref,side,currpix,minpix,false);
   }
   else {
      currpix.move( moveDir(side) );
      side >>= 1;
      if (side < ShapeRef::SHAPE_L) {
         side = ShapeRef::SHAPE_T;
      }
      shapePixelBorder(relations,polyref,side,currpix,minpix,false);
   }
}

// note that this is almost exactly the same as shapePixelBorder
void ShapeMap::pointPixelBorder(const PointMap& pointmap, pmap<int,int>& relations, SalaShape& poly, int side, PixelRef currpix, PixelRef minpix, bool first) 
{
   if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
      // looped:
      return;
   }
   size_t rel = relations.searchindex(currpix);
   if (relations[rel] & side) {
      poly.push_back(pointmap.depixelate(currpix)+pointOffset(pointmap,currpix,side));
      relations[rel] &= ~side;   // <- clear to check all have been done later
      side <<= 1;
      if (side > ShapeRef::SHAPE_T) {
         side = ShapeRef::SHAPE_L;
      }
      pointPixelBorder(pointmap,relations,poly,side,currpix,minpix,false);
   }
   else {
      currpix.move( moveDir(side) );
      side >>= 1;
      if (side < ShapeRef::SHAPE_L) {
         side = ShapeRef::SHAPE_T;
      }
      pointPixelBorder(pointmap,relations,poly,side,currpix,minpix,false);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::removePolyPixels(int polyref)
{
   size_t index = m_shapes.searchindex(polyref);
   if (index == paftl::npos) {
      return;
   }
   SalaShape& poly = m_shapes[index];
   if (poly.isClosed()) {
      // easiest just to use scan lines to find internal pixels rather than trace a complex border:
      PixelRef minpix = pixelate(poly.m_region.bottom_left);
      PixelRef maxpix = pixelate(poly.m_region.top_right);
      for (int x = minpix.x; x <= maxpix.x; x++) {
         for (int y = minpix.y; y <= maxpix.y; y++) {
            size_t pos = m_pixel_shapes[x][y].searchindex(polyref);
            if (pos != paftl::npos) {
               m_pixel_shapes[x][y].remove_at(pos);
            }
         }
      }
   }
   else {
     // open shapes easier still, as no need to find internal pixels:
     switch (poly.m_type & SalaShape::SHAPE_TYPE)
      { 
      case SalaShape::SHAPE_POINT:
         {
            PixelRef pix = pixelate(poly.m_centroid);
            int pos = m_pixel_shapes[pix.x][pix.y].searchindex(polyref);
            if (pos != -1) {
               m_pixel_shapes[pix.x][pix.y].remove_at(pos);
            }
         }
         break;
      case SalaShape::SHAPE_LINE:
         {
            PixelRefVector list = pixelateLine(poly.m_region);
            for (size_t i = 0; i < list.size(); i++) {
               size_t pos = m_pixel_shapes[list[i].x][list[i].y].searchindex(polyref);
               if (pos != paftl::npos) {
                  m_pixel_shapes[list[i].x][list[i].y].remove_at(pos);
               }
            }
         }
         break;
      case SalaShape::SHAPE_POLY:
         for (size_t k = 0; k < poly.size() - 1; k++) {
            size_t nextk = (k + 1);
            Line li(poly[k],poly[nextk]);
            PixelRefVector list = pixelateLine(li);
            for (size_t i = 0; i < list.size(); i++) {
               size_t pos = m_pixel_shapes[list[i].x][list[i].y].searchindex(polyref);
               if (pos != paftl::npos) {
                  m_pixel_shapes[list[i].x][list[i].y].remove_at(pos);
               }
            }
         }
         break;
      }
 
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::moveDir(int side)
{
   int dir;
   switch (side)
   {
   case ShapeRef::SHAPE_L:
      dir = PixelRef::NEGHORIZONTAL;
      break;
   case ShapeRef::SHAPE_B:
      dir = PixelRef::NEGVERTICAL;
      break;
   case ShapeRef::SHAPE_R:
      dir = PixelRef::HORIZONTAL;
      break;
   case ShapeRef::SHAPE_T:
      dir = PixelRef::VERTICAL;
      break;
   }
   return dir;
}

Point2f ShapeMap::pointOffset(const PointMap& pointmap, int currpix, int side) 
{
   Point2f p;
   switch (side)
   {
   case ShapeRef::SHAPE_L:
      p = Point2f(-pointmap.getSpacing()/2,0.0);
      break;
   case ShapeRef::SHAPE_B:
      p = Point2f(0.0,-pointmap.getSpacing()/2);
      break;
   case ShapeRef::SHAPE_R:
      p = Point2f(pointmap.getSpacing()/2,0.0);
      break;
   case ShapeRef::SHAPE_T:
      p = Point2f(0.0,pointmap.getSpacing()/2);
      break;
   }
   return p;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// returns poly key for an attributes call / select command

int ShapeMap::quickPointInPoly(const Point2f& p) const
{
   if (!m_region.contains(p)) {
      return -1;
   }
   PixelRef pix = pixelate(p);
   pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
   int drawlast = -1;
   int draworder = -1;
   for (size_t i = 0; i < shapes.size(); i++) {
      size_t x = m_shapes.searchindex(shapes[i].m_shape_ref);
      if (x != paftl::npos && m_attributes.getDisplayPos(x) > draworder) {
         drawlast = x;
         draworder = m_attributes.getDisplayPos(x);
      }
   }
   return drawlast;
}

// Point in poly testing (returns topmost displayed poly)

int ShapeMap::pointInPoly(const Point2f& p) const
{
   if (!m_region.contains(p)) {
      return -1;
   }
   pvecint testedshapes;
   pvecint testnodes;
   PixelRef pix = pixelate(p);
   pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
   int drawlast = -1;
   int draworder = -1;
   for (size_t i = 0; i < shapes.size(); i++) {
      const ShapeRef& shape = shapes[i]; 
      if (testedshapes.searchindex(shape.m_shape_ref) != paftl::npos) {
         continue;
      }
      testedshapes.add(shape.m_shape_ref,paftl::ADD_HERE);

      int shapeindex = testPointInPoly(p,shape);

      // if there's a shapeindex, then add:
      if (shapeindex != -1 && m_attributes.getDisplayPos(shapeindex) > draworder) {
         drawlast = shapeindex;
         draworder = m_attributes.getDisplayPos(shapeindex);
      }
   }
   return drawlast;
}

// Point in specific poly (by reference)

bool ShapeMap::pointInPoly(const Point2f& p, int polyref) const
{
   PixelRef pix = pixelate(p);
   pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
   size_t x = shapes.searchindex(ShapeRef(polyref));
   if (x != paftl::npos) {
      return (testPointInPoly(p,shapes[x]) != -1);
   }
   return false;
}

// similar to above, but builds a list

void ShapeMap::pointInPolyList(const Point2f& p, pvecint& shapeindexlist) const
{
   if (!m_region.contains(p)) {
      return ;
   }
   pvecint testedshapes;
   PixelRef pix = pixelate(p);
   pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
   for (size_t i = 0; i < shapes.size(); i++) {
      const ShapeRef& shape = shapes[i]; 
      if (testedshapes.searchindex(shape.m_shape_ref) != paftl::npos) {
         continue;
      }
      testedshapes.add(shape.m_shape_ref,paftl::ADD_HERE);

      int shapeindex = testPointInPoly(p,shape);

      // if there's a shapeindex, then add (note it is an add -- you may be passed a list again to expand)
      if (shapeindex != -1) {
         shapeindexlist.add(shapeindex);
      }
   }
}

// note, lineref is only used as an "exclude self" test when called from getShapeConnections
void ShapeMap::lineInPolyList(const Line& li_orig, pvecint& shapeindexlist, int lineref, double tolerance) const
{
   if (!intersect_region(m_region,li_orig)) {
      return;
   }
   Line li = li_orig;
   if (!m_region.contains(li.start()) || !m_region.contains(li.end())) {
      li.crop(m_region);
   }

   pointInPolyList(li.start(),shapeindexlist);
   pointInPolyList(li.end(),shapeindexlist);

   // only now pixelate and test for any other shapes:
   PixelRefVector list = pixelateLine(li);
   for (size_t i = 0; i < list.size(); i++) {
      PixelRef pix = list[i];
      if (includes(pix)) {
         pqvector<ShapeRef>& shapes = m_pixel_shapes[pix.x][pix.y];
         for (size_t j = 0; j < shapes.size(); j++) {
            const ShapeRef& shape = shapes[j]; 
            // slow to do this as it can repeat -- really need to use a linetest like structure to avoid retest of 
            // polygon lines
            if (shape.m_shape_ref != lineref && shape.m_tags & (ShapeRef::SHAPE_EDGE | ShapeRef::SHAPE_INTERNAL_EDGE | ShapeRef::SHAPE_OPEN)) {
               const SalaShape& poly = m_shapes.search(shape.m_shape_ref);
               switch (poly.m_type & (SalaShape::SHAPE_LINE | SalaShape::SHAPE_POLY)) {
               case SalaShape::SHAPE_LINE:
                  if (intersect_region(li,poly.m_region)) {
                     // note: in this case m_region is stored as a line:
                     if (intersect_line(li,poly.m_region,tolerance)) {
                        shapeindexlist.add(m_shapes.searchindex(shape.m_shape_ref));
                     }
                  }
                  break;
               case SalaShape::SHAPE_POLY:
                  {
                     for (int k = 0; k < shape.m_polyrefs.size(); k++) {
                        Line lineb = Line(poly[shape.m_polyrefs[k]],poly[((shape.m_polyrefs[k]+1)%poly.size())]);
                        if (intersect_region(li,lineb)) {
                           if (intersect_line(li,lineb,tolerance)) {
                              shapeindexlist.add(m_shapes.searchindex(shape.m_shape_ref));
                           }
                        }
                     }
                  }
                  break;
               default:
                  break;
               }
            }
         }
      }
   }
}

void ShapeMap::polyInPolyList(int polyref, pvecint& shapeindexlist, double tolerance) const
{
   size_t index = m_shapes.searchindex(polyref);
   if (index == paftl::npos) {
      return;
   }
   const SalaShape& poly = m_shapes[index];
   if (poly.isClosed()) { // <- it ought to be, you shouldn't be using this function if not!
      pvecint testedlist;
      // easiest just to use scan lines to find internal pixels rather than trace a complex border:
      PixelRef minpix = pixelate(poly.m_region.bottom_left);
      PixelRef maxpix = pixelate(poly.m_region.top_right);
      // pass one: shape centre of either object coincident automatically adds
      int x;
      for (x = minpix.x; x <= maxpix.x; x++) {
         for (int y = minpix.y; y <= maxpix.y; y++) {
            size_t pos = m_pixel_shapes[x][y].searchindex(polyref);
            if (pos != paftl::npos) {
               pqvector<ShapeRef>& shaperefs = m_pixel_shapes[x][y];
               // this has us in it, now looked through everything else:
               for (size_t i = 0; i < shaperefs.size(); i++) {
                  ShapeRef& shaperef = shaperefs[i];
                  if (i != pos && ((shaperefs[pos].m_tags & ShapeRef::SHAPE_CENTRE) || (shaperef.m_tags & ShapeRef::SHAPE_CENTRE))) {
                     if (testedlist.add(shaperef.m_shape_ref) != -1) {
                        shapeindexlist.add(m_shapes.searchindex(shaperef.m_shape_ref));
                     }
                  }
               }
            }
         }
      }
      // that was the easy bit... now, pass 2, for non centre things:
      for (x = minpix.x; x <= maxpix.x; x++) {
         for (int y = minpix.y; y <= maxpix.y; y++) {
            size_t pos = m_pixel_shapes[x][y].searchindex(polyref);
            if (pos != paftl::npos) {
               pqvector<ShapeRef>& shaperefs = m_pixel_shapes[x][y];
               ShapeRef& shaperef = shaperefs[pos];
               if ((shaperef.m_tags & ShapeRef::SHAPE_CENTRE) == 0) {
                  // this has us in it, now looked through everything else:
                  for (size_t i = 0; i < shaperefs.size(); i++) {
                     ShapeRef& shaperefb = shaperefs[i];
                     if (i != pos && testedlist.searchindex(shaperefb.m_shape_ref) == paftl::npos) {
                        size_t indexb = m_shapes.searchindex(shaperefb.m_shape_ref);
                        const SalaShape& polyb = m_shapes[indexb];
                        if (polyb.isPoint()) {
                           if (testPointInPoly(polyb.getPoint(),shaperef) != -1) {
                              shapeindexlist.add((int)indexb);
                           }
                        }
                        else if (polyb.isLine()) {
                           if (testPointInPoly(polyb.getLine().start(),shaperef) != -1 || testPointInPoly(polyb.getLine().end(),shaperef) != -1) {
                              testedlist.add(shaperefb.m_shape_ref,paftl::ADD_HERE);
                              shapeindexlist.add((int)indexb);
                           }
                           else {
                              for (int k = 0; k < shaperef.m_polyrefs.size(); k++) {
                                 Line line = Line(poly[shaperef.m_polyrefs[k]],poly[((shaperef.m_polyrefs[k]+1)%poly.size())]);
                                 if (intersect_region(line,polyb.getLine())) {
                                    if (intersect_line(line,polyb.getLine(),tolerance)) {
                                       testedlist.add(shaperefb.m_shape_ref,paftl::ADD_HERE);
                                       shapeindexlist.add((int)indexb);
                                       break;
                                    }
                                 }
                              }
                           }
                        }
                        else if (polyb.isPolyLine()) {
                           if (testPointInPoly(polyb[shaperefb.m_polyrefs[0]],shaperef) != -1)  {
                              testedlist.add(shaperefb.m_shape_ref,paftl::ADD_HERE);
                              shapeindexlist.add(indexb);
                           }
                           else {
                              for (int k = 0; k < shaperef.m_polyrefs.size(); k++) {
                                 for (int kk = 0; kk < shaperefb.m_polyrefs.size(); kk++) {
                                    Line line = Line(poly[shaperef.m_polyrefs[k]],poly[((shaperef.m_polyrefs[k]+1)%poly.size())]);
                                    Line lineb = Line(polyb[shaperefb.m_polyrefs[kk]],polyb[((shaperefb.m_polyrefs[kk]+1)%polyb.size())]);
                                    if (intersect_region(line,lineb)) {
                                       if (intersect_line(line,lineb,tolerance)) {
                                          if (testedlist.add(shaperefb.m_shape_ref) != -1) {
                                             shapeindexlist.add(indexb);
                                             break;
                                          }
                                       }
                                    }
                                 }
                              }
                           }
                        }
                        else {
                           // poly to poly, ick!
                           // first test one entirely inside the other
                           // any point at all will suffice to check this: however, we need to check that the polyref point *itself* is within the 
                           // pixel, not just part of the line associated with it...
                           if ((pixelate(polyb[shaperefb.m_polyrefs[0]]) == PixelRef(x,y) && testPointInPoly(polyb[shaperefb.m_polyrefs[0]],shaperef) != -1) ||
                               (pixelate(poly[shaperef.m_polyrefs[0]]) == PixelRef(x,y) && testPointInPoly(poly[shaperef.m_polyrefs[0]],shaperefb) != -1))  {
                              testedlist.add(shaperefb.m_shape_ref,paftl::ADD_HERE);
                              shapeindexlist.add(indexb);
                           }
                           else {
                              // now check crossing
                              bool breakit = false;
                              for (int k = 0; k < shaperef.m_polyrefs.size() && !breakit; k++) {
                                 for (int kk = 0; kk < shaperefb.m_polyrefs.size(); kk++) {
                                    Line line = Line(poly[shaperef.m_polyrefs[k]],poly[((shaperef.m_polyrefs[k]+1)%poly.size())]);
                                    Line lineb = Line(polyb[shaperefb.m_polyrefs[kk]],polyb[((shaperefb.m_polyrefs[kk]+1)%polyb.size())]);
                                    if (intersect_region(line,lineb)) {
                                       if (intersect_line(line,lineb,tolerance)) {
                                          testedlist.add(shaperefb.m_shape_ref,paftl::ADD_HERE);
                                          shapeindexlist.add(indexb);
                                          breakit = true;
                                          break;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }

   }
   else {
      throw depthmapX::RuntimeException("this function is to be used for polygons only");
   }
}

void ShapeMap::shapeInPolyList(const SalaShape& shape, pvecint& shapeindexlist) // note: no const due to poly in poly testing
{
   if (!intersect_region(m_region,shape.m_region)) {
      // quick test that actually coincident
      return;
   }
   if (shape.isPoint()) {
      pointInPolyList(shape.getPoint(),shapeindexlist);
   }
   else if (shape.isLine()) {
      lineInPolyList(shape.getLine(),shapeindexlist);
   }
   else if (shape.isPolyLine()) {
      for (size_t i = 1; i < shape.size() - 1; i++) {
         Line li(shape[i], shape[i-1]);
         lineInPolyList(li,shapeindexlist);
      }
   }
   else {
      // first *add* the poly temporarily (note this may grow pixel set):
      int ref = makePolyShape(shape, false, true); // false is closed poly, true is temporary shape
      // do test:
      polyInPolyList(ref, shapeindexlist);
      // clean up:
      removePolyPixels(ref);
      size_t rowid = m_shapes.searchindex(ref);
      m_shapes.remove_at(rowid);
      m_shape_ref--; // also undo temporary shaperef, or it could get out of hand with lots of tests
   }
}

// helper for point in poly -- 
// currently needs slight rewrite to avoid problem if point is in line with a vertex 
// (counter incremented twice on touching implies not in poly when is)

int ShapeMap::testPointInPoly(const Point2f& p, const ShapeRef& shape) const
{
   size_t shapeindex = paftl::npos;
   // simplist: in shape centre
   if (shape.m_tags & ShapeRef::SHAPE_CENTRE) {
      shapeindex = m_shapes.searchindex(shape.m_shape_ref);
   }
   // check not an open shape (cannot be inside)
   else if ((shape.m_tags & ShapeRef::SHAPE_OPEN) == 0) {
      const SalaShape& poly = m_shapes.search(shape.m_shape_ref);
      if (poly.m_region.contains_touch(p)) {
         // next simplest, on the outside border:
         int alpha = 0;
         int counter = 0;
         int parity = 0;
         if (shape.m_tags & ShapeRef::SHAPE_EDGE) {
            // run a test line to the edge:
            if (shape.m_tags & (ShapeRef::SHAPE_L | ShapeRef::SHAPE_R)) {
               if (shape.m_tags & ShapeRef::SHAPE_L) {
                  parity = -1;
               }
               else if (shape.m_tags & ShapeRef::SHAPE_R) {
                  parity = +1;
               }
               for (int j = 0; j < shape.m_polyrefs.size(); j++) {
                  Line lineb = Line(poly[shape.m_polyrefs[j]],poly[((shape.m_polyrefs[j]+1)%poly.size())]);
                  if (lineb.bottom_left.y <= p.y && lineb.top_right.y >= p.y) {
                     // crosses or touches... but we need to check
                     // touching exception:
                     if (lineb.t_start().y == p.y) {
                        if (parity * lineb.t_start().x >= parity * p.x) {
                           alpha -= 1;
                           counter++;
                        }
                     }
                     // the other touching exception
                     else if (lineb.t_end().y == p.y) {
                        if (parity * lineb.t_end().x >= parity * p.x) {
                           alpha += 1;
                           // n.b., no counter here
                        }
                     }
                     // at this stage we know the line isn't horizontal, so we can find the intersection point:
                     else if (parity * (lineb.grad(XAXIS)*(p.y-lineb.ay()) + lineb.ax()) >= parity * p.x) {
                        counter++;
                     }
                  }
               }
            }
            else {
               if (shape.m_tags & ShapeRef::SHAPE_B) {
                  parity = -1;
               }
               else if (shape.m_tags & ShapeRef::SHAPE_T) {
                  parity = +1;
               }
               for (int j = 0; j < shape.m_polyrefs.size(); j++) {
                  Line lineb = Line(poly[shape.m_polyrefs[j]],poly[((shape.m_polyrefs[j]+1)%poly.size())]);
                  if (lineb.bottom_left.x <= p.x && lineb.top_right.x >= p.x) {
                     // crosses or touches... but we need to check
                     // touching exception:
                     if (lineb.top_right.x == p.x) {
                        if (parity * lineb.by() >= parity * p.y) {
                           alpha -= 1;
                           counter++;
                        }
                     }
                     // the other touching exception
                     else if (lineb.bottom_left.x == p.x) {
                        if (parity * lineb.ay() >= parity * p.y) {
                           alpha += 1;
                           // n.b., no counter here
                        }
                     }
                     // at this stage we know the line isn't vertical, so we can find the intersection point:
                     else if (parity * (lineb.grad(YAXIS)*(p.x-lineb.ax()) + lineb.ay()) >= parity * p.y) {
                        counter++;
                     }
                  }
               }
            }
            if (counter % 2 != 0 && alpha == 0) {
               shapeindex = m_shapes.searchindex(shape.m_shape_ref);
            }
         }
         // and now the pig -- it's somewhere in the middle of the poly:
         else if (shape.m_tags & ShapeRef::SHAPE_INTERNAL_EDGE) {
            pvecint testnodes;
            size_t j;
            for (j = 0; j < size_t(shape.m_polyrefs.size()); j++) { // <- note, polyrefs is a subvec and has maximum number according to sizeof(T)
               testnodes.add(shape.m_polyrefs[j]);
            }
            PixelRef pix2 = pixelate(p);
            // bit of code duplication like this, but easier on params to this function:
            pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this shape...
            size_t nextindex = m_pixel_shapes[pix2.x][pix2.y].searchindex(shape.m_shape_ref);
            while (nextindex != paftl::npos) {
               const ShapeRef& shape2 = m_pixel_shapes[pix2.x][pix2.y][nextindex]; 
               for (int k = 0; k < shape2.m_polyrefs.size(); k++) {
                  testnodes.add(shape2.m_polyrefs[k]);
               }
               pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this shape...
               if (includes(pix2)) {
                  nextindex = m_pixel_shapes[pix2.x][pix2.y].searchindex(shape.m_shape_ref);
               }
               else {
                  nextindex = paftl::npos;
               }
            }
            int alpha = 0;
            int counter = 0;
            int parity = -1;
            for (j = 0; j < testnodes.size(); j++) {
               Line lineb = Line(poly[testnodes[j]],poly[((testnodes[j]+1)%poly.size())]);
               if (lineb.bottom_left.x <= p.x && lineb.top_right.x >= p.x) {
                  // crosses or touches... but we need to check
                  // touching exception:
                  if (lineb.top_right.x == p.x) {
                     if (parity * lineb.by() >= parity * p.y) {
                        alpha -= 1;
                        counter++;
                     }
                  }
                  // the other touching exception
                  else if (lineb.bottom_left.x == p.x) {
                     if (parity * lineb.ay() >= parity * p.y) {
                        alpha += 1;
                        // n.b., no counter here
                     }
                  }
                  // at this stage we know the line isn't vertical, so we can find the intersection point:
                  else if (parity * (lineb.grad(YAXIS)*(p.x-lineb.ax()) + lineb.ay()) >= parity * p.y) {
                     counter++;
                  }
               }
            }
            if (counter % 2 != 0 && alpha == 0) {
               shapeindex = m_shapes.searchindex(shape.m_shape_ref);
            }
         }
      }
   }
   return (shapeindex == paftl::npos) ? -1 : int(shapeindex); // note convert to -1
}

// also note that you may want to find a close poly line or point
// if you can't find a point in poly (or even if you can)

// (see also getClosestVertex below)

// returns a rowid *not* a shape key

int ShapeMap::getClosestOpenGeom(const Point2f& p) const
{
   if (!m_region.contains(p)) {
      return -1;
   }

   PixelRef pix = pixelate(p);

   size_t index = paftl::npos;
   double mindist = -1;
   for (size_t i = 0; i < m_pixel_shapes[pix.x][pix.y].size(); i++) {
      ShapeRef& ref = m_pixel_shapes[pix.x][pix.y][i];
      if (ref.m_tags & ShapeRef::SHAPE_OPEN) {
         double thisdist = -1.0;
         const SalaShape& poly = m_shapes.search(ref.m_shape_ref);
         switch (poly.m_type) {
         case SalaShape::SHAPE_POINT:
            thisdist = dist(p,poly.m_centroid);
         case SalaShape::SHAPE_LINE:
            thisdist = dist(p,poly.m_region);  // note, in this case m_region is a line
            break;
         case SalaShape::SHAPE_POLY: case SalaShape::SHAPE_POLY | SalaShape::SHAPE_CCW: // note CCW should never have happened, but it has
            for (int j = 0; j < ref.m_polyrefs.size(); j++) {
               Line line(poly[ref.m_polyrefs[j]],poly[ref.m_polyrefs[j]+1]);
               double tempthisdist = dist(p,line);
               if (tempthisdist != -1 && (thisdist == -1 || tempthisdist < thisdist)) {
                  thisdist = tempthisdist;
               }
            }
            break;
         }
         if (thisdist != -1.0 && (mindist == -1 || thisdist < mindist)) {
            mindist = thisdist;
            index = m_shapes.searchindex(ref.m_shape_ref);
         }
      }
   }

   return (index == paftl::npos) ? -1 : int(index); // note conversion to -1
}

Point2f ShapeMap::getClosestVertex(const Point2f& p) const
{
   Point2f vertex; // null by default

   if (!m_region.contains(p)) {
      return vertex; // will be null in this case
   }

   PixelRef pix = pixelate(p);

   double mindist = -1.0;
   for (size_t i = 0; i < m_pixel_shapes[pix.x][pix.y].size(); i++) {
      ShapeRef& ref = m_pixel_shapes[pix.x][pix.y][i];
      double thisdist = -1.0;
      Point2f thisvertex;
      const SalaShape& poly = m_shapes.search(ref.m_shape_ref);
      switch (poly.m_type) {
         case SalaShape::SHAPE_POINT:
            thisvertex = poly.m_centroid;
            thisdist = dist(p,thisvertex);
            break;
         case SalaShape::SHAPE_LINE:
            {
               double d1 = dist(p,poly.m_region.start());
               double d2 = dist(p,poly.m_region.end());
               if (d1 < d2) {
                  thisvertex = poly.m_region.start();
                  thisdist = d1;
               }
               else {
                  thisvertex = poly.m_region.end();
                  thisdist = d2;
               }
            }
            break;
         default: // either a poly line or a polygon
            for (int j = 0; j < ref.m_polyrefs.size(); j++) {
               double d1 = dist(p,poly[ref.m_polyrefs[j]]);
               // note this can be used for both open / closed with the % poly.size()
               double d2 = dist(p,poly[(ref.m_polyrefs[j]+1)%poly.size()]);
               if (thisdist == -1 || d1 < thisdist) {
                  thisvertex = poly[ref.m_polyrefs[j]];
                  thisdist = d1;
               }
               if (d2 < thisdist) {
                  thisvertex = poly[(ref.m_polyrefs[j]+1)%poly.size()];
                  thisdist = d2;
               }
            }
            break;
      }
      if (thisdist != -1.0 && (mindist == -1.0 || thisdist < mindist)) {
          mindist = thisdist;
          vertex = thisvertex;
      }
   }

   return vertex;
}

// nb, uses full BSP tree test:

#include "isovist.h"

int ShapeMap::getClosestLine(const Point2f& p) const
{
   // not the best place to check this, but we must all the same:
   if (m_newshape) {
      m_bsp_tree = false;
   }

   if (!m_bsp_tree) {
      makeBSPtree();
   }

   int index = -1;

   if (m_bsp_tree) { // <- check there is actually something in the tree!
      Isovist iso;
      index = iso.getClosestLine(m_bsp_root, p);
   }

   return index;
}

void ShapeMap::getShapeCuts(const Line& li_orig, pvector<ValuePair>& cuts)
{
   if (!intersect_region(li_orig,m_region)) {
      return;
   }
   Line li = li_orig;
   if (!m_region.contains(li.start()) || !m_region.contains(li.end())) {
      li.crop(m_region);
   }
   int axis = YAXIS;
   if (li.width() > li.height()) {
      axis = XAXIS;
   }
   PixelRefVector pixels = pixelateLine(li);
   pvector<IntPair> tested;
   for (size_t i = 0; i < pixels.size(); i++) {
      PixelRef& pix = pixels[i];
      if (includes(pix)) { // <- note, for some reason, this pixel may be off edge (line crop problem?)
         for (size_t j = 0; j < m_pixel_shapes[pix.x][pix.y].size(); j++) {
            ShapeRef& shaperef = m_pixel_shapes[pix.x][pix.y][j];
            if (!shaperef.m_polyrefs.isEmpty()) {
               int len = shaperef.m_polyrefs.size(); 
               for (int k = 0; k < len; k++) {
                  int x = shaperef.m_polyrefs[k];
                  if (tested.searchindex(IntPair(shaperef.m_shape_ref,x)) == paftl::npos) {
                     SalaShape& poly = m_shapes.search(shaperef.m_shape_ref);
                     
                     // Quick mod - TV
#if defined(_WIN32)                     
                     Line& li2 = Line(poly[x],poly[(x+1)%poly.size()]);
#else
		     Line li2(poly[x], poly[(x+1) % poly.size()]);
#endif                     
                     if (intersect_region(li,li2)) {
                        // note: in this case m_region is stored as a line:
                        if (intersect_line(li,li2)) {
                           // find intersection point and add:
                           cuts.add(ValuePair(shaperef.m_shape_ref,li.intersection_point(li2,axis,1e-9)),paftl::ADD_DUPLICATE);  // note: added key not rowid
                        }
                     }
                     tested.add(IntPair(shaperef.m_shape_ref,x),paftl::ADD_HERE);
                  }
               }
            }
            else {
               // this is a non-poly, so check just the shape_ref:
               if (tested.searchindex(IntPair(shaperef.m_shape_ref,-1)) == paftl::npos) {
                  SalaShape& poly = m_shapes.search(shaperef.m_shape_ref);
                  // n.b. points cannot be intersected (and since we won't return to the pix, don't need to be added to the tested list
                  if (poly.isLine()) { 
                     if (intersect_region(li,poly.m_region)) {
                        // note: in this case m_region is stored as a line:
                        if (intersect_line(li,poly.m_region)) {
                           // find intersection point and add:
                           cuts.add(ValuePair(shaperef.m_shape_ref,li.intersection_point(poly.m_region,axis,1e-9)),paftl::ADD_DUPLICATE);  // note: added key not rowid
                        }
                     }
                     tested.add(IntPair(shaperef.m_shape_ref,-1),paftl::ADD_HERE);
                  }
               }
            }
         }
      }
   }
}

void ShapeMap::cutLine(Line& li) //, short dir)
{
   pvector<ValuePair> cuts;
   getShapeCuts(li,cuts);
/*
   bool same = false;
   if (dir == li.direction()) {
      same = true;
   }

   if (cuts.size()) {
      if (li.width() > li.height()) {
         // these are xaxis cuts
         if (li.rightward() == same) {
            // in the correct order:
            li = Line(li.point_on_line(cuts.head().value,XAXIS),li.end());
         }
         else {
            // in reverse order:
            li = Line(li.start(),li.point_on_line(cuts.tail().value,XAXIS));
         }
      }
      else {
         if (li.upward()) {
            li = Line(li.point_on_line(cuts.head().value,YAXIS),li.end());
         }
         else {
            // in reverse order:
            li = Line(li.start(),li.point_on_line(cuts.tail().value,YAXIS));
         }
      }
   }
   */

   if (cuts.size()) {
      if (li.width() > li.height()) {
         if (li.rightward()) {
            li = Line(li.start(),li.point_on_line(cuts.head().value,XAXIS));
         }
         else {
            li = Line(li.point_on_line(cuts.tail().value,XAXIS),li.end());
         }
      }
      else {
         if (li.upward()) {
            li = Line(li.t_start(),li.point_on_line(cuts.head().value,YAXIS));
         }
         else {
            li = Line(li.t_start(),li.point_on_line(cuts.tail().value,YAXIS));
         }
      }
   }

}

/////////////////////////////////////////////////////////////////////////////////

// buffering type function, just for points for the time being

int ShapeMap::withinRadius(const Point2f& pt, double radius, std::vector<int>& bufferset)
{
   // first, get all the pixels within the radius (using square as simpler)
   PixelRef bl = pixelate( Point2f(pt.x - radius, pt.y - radius) );
   PixelRef tr = pixelate( Point2f(pt.x + radius, pt.y + radius) );
   // go through testing line distances to each shape:
   pvector<IntPair> tested;
   for (int i = bl.x; i <= tr.x; i++) {
      for (int j = bl.y; j <= tr.y; j++) {
         for (size_t k = 0; k < m_pixel_shapes[i][j].size(); k++) {
            ShapeRef& shaperef = m_pixel_shapes[i][j][k];
            int len = shaperef.m_polyrefs.size();
            if (len == 0) {
               // this is a non-poly, so check just the shape_ref:
               if (tested.searchindex(IntPair(shaperef.m_shape_ref,-1)) == paftl::npos) {
                  size_t shapeindex = m_shapes.searchindex(shaperef.m_shape_ref);
                  SalaShape& poly = m_shapes[shapeindex];
                  if (poly.isPoint() && dist(pt,poly.getPoint()) < radius) {
                     depthmapX::addIfNotExists(bufferset, int(shapeindex));
                  }
                  else if (dist(pt,poly.getLine()) < radius) { // if poly is line
                      depthmapX::addIfNotExists(bufferset, int(shapeindex));
                  }
                  tested.add(IntPair(shaperef.m_shape_ref,-1),paftl::ADD_HERE);
               }
            }
            else {
               for (int p = 0; p < len; p++) {
                  int q = shaperef.m_polyrefs[p];
                  if (tested.searchindex(IntPair(shaperef.m_shape_ref,q)) == paftl::npos) {
                     size_t shapeindex = m_shapes.searchindex(shaperef.m_shape_ref);
                     SalaShape& poly = m_shapes[shapeindex];
                     Line li( poly[q], poly[(q+1)%poly.size()] );
                     if (dist(pt,li) < radius) {
                         depthmapX::addIfNotExists(bufferset, int(shapeindex));
                     }
                     tested.add(IntPair(shaperef.m_shape_ref,q),paftl::ADD_HERE);
                  }
               }
            }
         }
      }
   }
   // finally, do a quick point in poly test for any polygons in the centre pixel
   // only need to check the shape ref as everything else will have been picked up above
   PixelRef centre = pixelate(pt);
   for (size_t k = 0; k < m_pixel_shapes[centre.x][centre.y].size(); k++) {
      ShapeRef& shaperef = m_pixel_shapes[centre.x][centre.y][k];
      if (shaperef.m_tags & ShapeRef::SHAPE_CENTRE) {
          int shapeindex = m_shapes.searchindex(shaperef.m_shape_ref);
          depthmapX::addIfNotExists(bufferset, shapeindex);
      }
   }
   return bufferset.size();
}

/////////////////////////////////////////////////////////////////////////////////

// code to add intersections when shapes are added to the graph one by one:
int ShapeMap::connectIntersected(int rowid, bool linegraph)
{
   int shaperef = m_shapes.key(rowid);
   int conn_col = m_attributes.getOrInsertColumnIndex("Connectivity");
   m_attributes.setColumnLock(conn_col);
   int leng_col = -1;
   if (linegraph) {
      // historically line length has always been added at this point
      leng_col = m_attributes.getOrInsertLockedColumnIndex("Line Length");
   }
   // all indices should match... this grows connectors if necessary to same length as shapes
   while (m_connectors.size() < m_shapes.size()) {
      m_connectors.push_back( Connector() );
   }
   int connectivity = linegraph ? 
      getLineConnections( shaperef, m_connectors[rowid].m_connections, TOLERANCE_B*__max(m_region.height(),m_region.width())) :
      getShapeConnections( shaperef, m_connectors[rowid].m_connections, TOLERANCE_B*__max(m_region.height(),m_region.width()));
   m_attributes.setValue(rowid, conn_col, (float) connectivity );
   if (linegraph) {
      m_attributes.setValue(rowid, leng_col, (float) m_shapes[rowid].getLength() );
   }
   // now go through our connections, and add ourself:
   for (size_t k = 0; k < m_connectors[rowid].m_connections.size(); k++) {
      int myplace = m_connectors[rowid].m_connections[k];
      if (myplace != rowid) { // <- exclude self!
         m_connectors[myplace].m_connections.add(rowid);
         m_attributes.incrValue(myplace,conn_col);
      }
   }
   return connectivity;
}

// this assumes this is a line map (to speed up axial map creation)
// use the other version, getShapeConnections for arbitrary shape-shape connections
// note, connections are listed by rowid in list, *not* reference number
// (so they may vary: must be checked carefully when shapes are removed / added)
int ShapeMap::getLineConnections(int lineref, pvecint& connections, double tolerance)
{
   SalaShape& poly = m_shapes.search(lineref);
   if (!poly.isLine()) {
      return 0;
   }
   const Line& l = poly.getLine();

   pvecint testedshapes;
   
   // As of version 10, self-connections are *not* added
   // In the past:
   // <exclude> it's useful to have yourself in your connections list
   // (apparently! -- this needs checking, as most of the time it is then checked to exclude self again!) </exclude> 
   // <exclude> connections.add(m_shapes.searchindex(lineref)); </exclude>

   testedshapes.add(lineref);

   int num_intersections = 0;

   PixelRefVector list = pixelateLine( l );

   for (size_t i = 0; i < list.size(); i++) {
      pqvector<ShapeRef>& shapes = m_pixel_shapes[ list[i].x ][ list[i].y ];
      for (size_t j = 0; j < shapes.size(); j++) {
         ShapeRef& shape = shapes[j];
         if (testedshapes.searchindex(shape.m_shape_ref) != paftl::npos) {
            continue;
         }
         testedshapes.add(shape.m_shape_ref,paftl::ADD_HERE);
         if ((shape.m_tags & ShapeRef::SHAPE_OPEN) == ShapeRef::SHAPE_OPEN) {
            try {
               const Line& line = m_shapes.search(shape.m_shape_ref).getLine();
               if ( intersect_region(line, l, line.length() * tolerance) ) {
                  // n.b. originally this followed the logic that we must normalise intersect_line properly: tolerance * line length one * line length two
                  // in fact, works better if it's just line.length() * tolerance...
                  if ( intersect_line(line, l, line.length() * tolerance) ) {
                     connections.add(m_shapes.searchindex(shape.m_shape_ref));
                     num_intersections++;
                  }
               }
            }
            catch (pexception) {
               // the lineref may have been deleted -- this is supposed to be tidied up
               // just ignore...
            }
         }
      }
   }

   return num_intersections;
}

// this is only problematic as there is lots of legacy code with shape-in-shape testing,
int ShapeMap::getShapeConnections(int shaperef, pvecint& connections, double tolerance)
{
   // In versions prior to 10, note that unlike getLineConnections, self-connection is excluded by all of the following functions
   // As of version 10, both getShapeConnections and getLineConnections exclude self-connection

   size_t index = m_shapes.searchindex(shaperef);
   if (index != paftl::npos) {
      SalaShape& shape = m_shapes[index];
      if (shape.isPoint()) {
         // a point is simple, it never intersects itself:
         pointInPolyList(shape.getPoint(),connections);
      }
      else if (shape.isPolygon()) {
         // a closed poly is actually quite simple too as we already have code using a polyref:
         polyInPolyList(shaperef,connections,tolerance);
      }
      else if (shape.isLine()) {
         // line is a bit slow because there's no tested shape as in getLineConnections, but similar:
         lineInPolyList(shape.getLine(),connections,shaperef,tolerance);
      }
      else if (shape.isPolyLine()) {
         // this is the worst for efficiency: potential for many possible retries of the same shape:
         for (size_t i = 1; i < shape.size() - 1; i++) {
            Line li(shape[i-1], shape[i]);
            lineInPolyList(li,connections,shaperef,tolerance);
         }
      }
   }

   return connections.size();
}

// for any geometry, not just line to lines
void ShapeMap::makeShapeConnections()
{
   if (m_hasgraph) {
      m_connectors.clear();
      m_attributes.clear();
      m_links.clear();
      m_unlinks.clear();

      // note, expects these to be numbered 0, 1...
      int conn_col = m_attributes.insertLockedColumn("Connectivity");

      for (size_t i = 0; i < m_shapes.size(); i++) {
         int key = m_shapes.key(i);
         int rowid = m_attributes.insertRow(key);
         // all indices should match...
         m_connectors.push_back( Connector() );
         int connectivity = getShapeConnections( key, m_connectors[i].m_connections, TOLERANCE_B*__max(m_region.height(),m_region.width()));
         m_attributes.setValue(rowid, conn_col, (float) connectivity );
      }

      m_displayed_attribute = -1; // <- override if it's already showing
      setDisplayedAttribute(conn_col);
   }
}


/////////////////////////////////////////////////////////////////////////////////

// note: uses rowid not key
double ShapeMap::getLocationValue(const Point2f& point) const
{
   double val = -1.0;
   int x = pointInPoly(point);
   if (x == -1) {
      // try looking for a polyline instead
      x = getClosestOpenGeom(point);
   }
   if (x != -1) {
     val = m_attributes.getValue(x,m_displayed_attribute);
   }
   return (x == -1) ? -2.0 : val;  // -2.0 is returned when point cannot be associated with a poly
}

bool ShapeMap::setCurSel( QtRegion& r, bool add )
{
   if (add == false) {
      clearSel();
   }

   if (r.bottom_left == r.top_right) {
      // note: uses index not key
      int index = pointInPoly(r.bottom_left);
      if (index == -1) {
         // try looking for a polyline instead
         index = getClosestOpenGeom(r.bottom_left);
      }
      if (index != -1) {
         // relies on indices of shapes and attributes being aligned
         if(m_selection_set.insert(index).second) {
            m_attributes.selectRowByIndex(index);
         }
         m_shapes.value(index).m_selected = true;
         m_selection = true;
      }
   }
   else {
      PixelRef bl = pixelate(r.bottom_left);
      PixelRef tr = pixelate(r.top_right);
      for (int i = bl.x; i <= tr.x; i++) {
         for (int j = bl.y; j <= tr.y; j++) {
            for (size_t k = 0; k < m_pixel_shapes[i][j].size(); k++) {
               // relies on indices of shapes and attributes being aligned
               size_t x = m_shapes.searchindex(m_pixel_shapes[i][j][k].m_shape_ref);
               if (x != paftl::npos && intersect_region(r,m_shapes.value(x).m_region)) {
                  m_shapes.value(x).m_selected = true;
                  m_selection = true;
               }
            }
         }
      }
      // actually probably often faster to set flag and later record list:
      for (size_t x = 0; x < m_shapes.size(); x++) {
         if (m_shapes.value(x).m_selected) {
             if(m_selection_set.insert(x).second) {
               m_attributes.selectRowByIndex(x);
            }
         }
      }
   }

   return m_selection;
}

// this version is used by setSelSet in MetaGraph, ultimately called from CTableView and PlotView
bool ShapeMap::setCurSel(const std::vector<int>& selset, bool add)
{
   // note: override cursel, can only be used with analysed pointdata:
   if (!add) {
      clearSel();
   }
   for (size_t i = 0; i < selset.size(); i++) {
      size_t index = m_shapes.searchindex(selset[i]);  // relies on aligned indices for attributes and shapes
      if (index != paftl::npos) {
         if(m_selection_set.insert(index).second) {
            m_attributes.selectRowByIndex(int(index));
         }
         m_shapes[index].m_selected = true;
         m_selection = true;
      }
   }
   return m_selection;
}

// this version is used when setting a selection set via the scripting language
bool ShapeMap::setCurSelDirect(const std::vector<int> &selset, bool add)
{
   // note: override cursel, can only be used with analysed pointdata:
   if (!add) {
      clearSel();
   }
   for (size_t i = 0; i < selset.size(); i++) {
      int index = selset[i];  // relies on aligned indices for attributes and shapes
      if (index != -1) {
         if(m_selection_set.insert(index).second) {
            m_attributes.selectRowByIndex(index);
         }
         m_shapes[index].m_selected = true;
         m_selection = true;
      }
   }
   return m_selection;
}


bool ShapeMap::clearSel()
{
   // note, only clear if need be, as m_attributes.deselectAll is slow
   if (m_selection_set.size()) {
      m_attributes.deselectAll();
      m_selection = false;
      for (auto& sel: m_selection_set) {
         m_shapes.value(sel).m_selected = false;
      }
      m_selection_set.clear();
   }
   return true;
}

QtRegion ShapeMap::getSelBounds()
{
   QtRegion r;
   if (m_selection_set.size()) {
       for (auto& sel: m_selection_set) {
         r = runion(r, m_shapes.value(sel).getBoundingBox());
      }
   }
   return r;
}

bool ShapeMap::selectionToLayer(const std::string& name)
{
   bool retvar = false;
   if (m_selection_set.size()) {
      retvar = m_attributes.selectionToLayer(name);
      if (retvar) {
         clearSel();
      }
   }
   return retvar;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::read( istream& stream, int version, bool drawinglayer )
{
   // turn off selection / editable etc
   m_selection = false;
   m_editable = false;
   m_show = true; // <- by default show
   m_map_type = ShapeMap::EMPTYMAP;

   // clear old BSP tree (if exists)
   m_bsp_tree = false;
   m_bsp_root = NULL;

   // clear old:
   if (m_pixel_shapes) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_shapes[i];
      }
      delete [] m_pixel_shapes;
      m_pixel_shapes = NULL;
   }
   if (m_display_shapes) {
      delete [] m_display_shapes;
      m_display_shapes = NULL;
   }
   m_objects.clear();
   m_shapes.clear();
   m_attributes.clear();
   m_connectors.clear();
   m_links.clear();
   m_unlinks.clear();
   m_undobuffer.clear();

   // name
   m_name = dXstring::readString(stream);


   stream.read( (char *) &m_map_type, sizeof(m_map_type));

   stream.read( (char *) &m_show, sizeof(m_show) );
   stream.read( (char *) &m_editable, sizeof(m_editable) );


   // PixelBase read
   // read extents:
   stream.read( (char *) &m_region, sizeof(m_region) );
   // read rows / cols
   stream.read( (char *) &m_rows, sizeof(m_rows) );
   stream.read( (char *) &m_cols, sizeof(m_cols) );
   // calculate geom data:
   m_tolerance = __max(m_region.width(), m_region.height()) * TOLERANCE_A;

   // read next object ref to be used:
   stream.read((char *) &m_obj_ref, sizeof(m_obj_ref));
   stream.read((char *) &m_shape_ref, sizeof(m_shape_ref));

   // read shape data
   int count = 0;
   stream.read((char *) &count, sizeof(count));
   for (int j = 0; j < count; j++) {
      int key;
      stream.read((char *) &key, sizeof(key));
      int index = m_shapes.add(key, SalaShape());
      m_shapes.value(index).read(stream,version);
   }

   // read object data (currently unused)
   stream.read((char *) &count, sizeof(count));
   for (int k = 0; k < count; k++) {
      int key;
      stream.read((char *) &key, sizeof(key));
      m_objects.insert(std::make_pair(key, SalaObject()));
      m_objects.end()->second.read(stream,version);
   }
   // read attribute data
   m_attributes.read(stream,version);
   stream.read((char *)&m_displayed_attribute,sizeof(m_displayed_attribute));

   // prepare pixel map:
   m_pixel_shapes = new pqvector<ShapeRef> *[m_cols];
   int i;
   for (i = 0; i < m_cols; i++) {
      m_pixel_shapes[i] = new pqvector<ShapeRef>[m_rows];
   }
   // Now add the pixel shapes pixel map:
   // pixelate all polys in the pixel structure:
   for (size_t j = 0; j < m_shapes.size(); j++) {
      makePolyPixels(m_shapes.key(j));
   }

   // shape connections:
   count = 0;
   stream.read((char *)&count,sizeof(count));
   for (int i = 0; i < count; i++) {
      m_connectors.push_back(Connector());
      m_connectors[i].read(stream,version);
   }
   m_links.read(stream);
   m_unlinks.read(stream);

   // some miscellaneous extra data for mapinfo files
   if (m_mapinfodata) {
      delete m_mapinfodata;
      m_mapinfodata = NULL;
   }
   char x = stream.get();
   if (x == 'm') {
      m_mapinfodata = new MapInfoData;
      m_mapinfodata->read(stream,version);
   }


   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);

   return true;
}

bool ShapeMap::write( ofstream& stream, int version )
{
   // name
   dXstring::writeString(stream, m_name);

   stream.write( (char *) &m_map_type, sizeof(m_map_type));
   stream.write( (char *) &m_show, sizeof(m_show) );
   stream.write( (char *) &m_editable, sizeof(m_editable) );

   // PixelBase write
   // write extents:
   stream.write( (char *) &m_region, sizeof(m_region) );
   // write rows / cols
   stream.write( (char *) &m_rows, sizeof(m_rows) );
   stream.write( (char *) &m_cols, sizeof(m_cols) );

   // write next object ref to be used:
   stream.write((char *) &m_obj_ref, sizeof(m_obj_ref));
   stream.write((char *) &m_shape_ref, sizeof(m_shape_ref));

   // write shape data
   int count = m_shapes.size();
   stream.write((char *) &count, sizeof(count));
   for (int j = 0; j < count; j++) {
      int key = m_shapes.key(j);
      stream.write((char *) &key, sizeof(key));
      m_shapes.value(j).write(stream);
   }
   // write object data (currently unused)
   count = m_objects.size();
   stream.write((char *) &count, sizeof(count));
   for (auto obj: m_objects) {
      int key = obj.first;
      stream.write((char *) &key, sizeof(key));
      obj.second.write(stream);
   }
   // write attribute data
   m_attributes.write(stream,version);
   stream.write((char *)&m_displayed_attribute,sizeof(m_displayed_attribute));

   // write connections data
   count = m_connectors.size();
   stream.write((char *)&count,sizeof(count)); 

   for (int i = 0; i < count; i++) {
      m_connectors[i].write(stream);
   }
   m_links.write(stream);
   m_unlinks.write(stream);

   // some miscellaneous extra data for mapinfo files
   if (m_mapinfodata) {
      stream.put('m');
      m_mapinfodata->write(stream);
   }
   else {
      stream.put('x');
   }

   return true;
}

bool ShapeMap::output( ofstream& stream, char delimiter, bool updated_only )
{
   stream << "Ref";
   if ((m_map_type & LINEMAP) == 0) {
      stream << delimiter << "cx" << delimiter << "cy";
   }
   else {
      stream << delimiter << "x1" << delimiter << "y1" << delimiter << "x2" << delimiter << "y2";
   }
   m_attributes.outputHeader(stream, delimiter, updated_only);

   stream.precision(12);
   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      if (m_attributes.isVisible(i)) {
         stream << m_attributes.getRowKey(i);
         if ((m_map_type & LINEMAP) == 0) {
            stream << delimiter << m_shapes[i].m_centroid.x << delimiter << m_shapes[i].m_centroid.y;
         }
         else {
            const Line& li = m_shapes[i].getLine();
            stream << delimiter << li.start().x << delimiter << li.start().y << delimiter << li.end().x << delimiter << li.end().y;
         }
         m_attributes.outputRow(i,stream,delimiter,updated_only);
      }
   }
   return true;
}

bool ShapeMap::importPoints(const std::vector<Point2f> &points,
                            const depthmapX::Table &data)
{
    //assumes that points and data come in the same order

    std::vector<int> indices;

    for(auto& point: points) {
        indices.push_back(makePointShape(point));
    }

    bool dataImported = importData(data, indices);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importLines(const std::vector<Line> &lines,
                           const depthmapX::Table &data)
{
    //assumes that lines and data come in the same order

    std::vector<int> indices;

    for(auto& line: lines) {
        indices.push_back(makeLineShape(line));
    }

    bool dataImported = importData(data, indices);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importPolylines(const std::vector<depthmapX::Polyline> &polylines,
                           const depthmapX::Table &data)
{
    //assumes that lines and data come in the same order

    std::vector<int> indices;

    for(auto& polyline: polylines) {
        indices.push_back(makePolyShape(genshim::toPQVector(polyline.m_vertices), !polyline.m_closed));
    }

    bool dataImported = importData(data, indices);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}


bool ShapeMap::importData(const depthmapX::Table &data, std::vector<int> indices)
{
    for (auto& column: data) {
        std::string colName = column.first;
        std::replace(colName.begin(), colName.end(), '_',' ');
        dXstring::makeInitCaps(colName);

        if (colName.empty()) continue;

        int colIndex = m_attributes.insertColumn(colName);

        if( colIndex == -1 ) {
            // error adding column (e.g., duplicate column names)
            continue;
        }

        std::unordered_map<std::string, size_t> colcodes;

        for (size_t i = 0; i < column.second.size(); i++) {
            std::string cellValue = column.second[i];
            double value = 0;
            if (dXstring::isDouble(cellValue)) {
                value = stod(cellValue);
            }
            else {
                std::unordered_map<std::string, size_t>::iterator cellAt = colcodes.find( cellValue );
                if(cellAt == colcodes.end()) {

                    // It seems that the original intention here was that if we are past 32 unique
                    // values, we should stop trying to make the column categorical and fill the rest
                    // of the values with -1.0f. It's not possible to test the original implementation
                    // because the app crashes if we load a file with more than 32 unique values. When
                    // and if we have a robust implementation of an attribute table that allows for
                    // both categorical and plain string attributes this should be re-examined for a
                    // better way to classify the column as either. Meanwhile after this threshold (32)
                    // we set the whole column to -1 so that it does not give the impression it worked
                    // when it's actually half-baked

                    if(colcodes.size() >= 32) {
                        for (size_t j = 0; j < column.second.size(); j++) {
                            m_attributes.setValue(indices[j], colIndex, -1.0f);
                        }
                        continue;
                    } else {
                        value = colcodes.size();
                        colcodes.insert(std::make_pair(cellValue, colcodes.size()));
                    }
                } else {
                    value = cellAt->second;
                }
            }
            m_attributes.setValue(indices[i], colIndex, value);
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////

void ShapeMap::setDisplayedAttribute(int col) const
{
   if (!m_invalidate && m_displayed_attribute == col) {
      return;
   }
   m_displayed_attribute = col;
   m_invalidate = true;

   // always override at this stage:
   m_attributes.setDisplayColumn(m_displayed_attribute,true);

   m_invalidate = false;
}

//////////////////////////////////////////////////////////////////////////////////

// this is all very similar to spacepixel, apart from a few minor details

void ShapeMap::makeViewportShapes( const QtRegion& viewport ) const
{
   if (m_invalidate) {
      return;
   }

   if (!m_display_shapes || m_newshape) {
      if (m_display_shapes) 
         delete [] m_display_shapes;
      m_display_shapes = new int [m_shapes.size()];
      m_newshape = false;
      for (size_t i = 0; i < m_shapes.size(); i++) {
         m_display_shapes[i] = -1;
      }
   }

   m_current = -1;   // note: findNext expects first to be labelled -1

   PixelRef bl = pixelate( viewport.bottom_left );
   PixelRef tr = pixelate( viewport.top_right );

   for (int i = bl.x; i <= tr.x; i++) {
      for (int j = bl.y; j <= tr.y; j++) {
         for (size_t k = 0; k < m_pixel_shapes[i][j].size(); k++) {
            // copy the index to the correct draworder position (draworder is formatted on display attribute)
            int x = m_attributes.getRowid(m_pixel_shapes[i][j][k].m_shape_ref);
            if (m_attributes.isVisible(x)) {
               m_display_shapes[m_attributes.getDisplayPos(x)] = x;
            }
         }
      }
   }

   m_curlinkline = -1;
   m_curunlinkpoint = -1;
}

bool ShapeMap::findNextShape(bool& nextlayer) const
{
   // note: will not work immediately after a new poly has been added: makeViewportShapes first
   if (m_invalidate || m_newshape) {
      return false;
   }

   while (++m_current < (int)m_shapes.size() && m_display_shapes[m_current] == -1 );
   
   if (m_current < (int)m_shapes.size()) {
      return true;
   }
   else {
      m_current = (int)m_shapes.size();
      nextlayer = true;
      return false;
   }
}

const SalaShape& ShapeMap::getNextShape() const
{
   int x = m_display_shapes[m_current];    // x has display order in it
   m_display_shapes[m_current] = -1;       // you've drawn it
   return m_shapes[x];
}

///////////////////////////////////////////////////////////////////////////////////

pqvector<SalaEdgeU> SalaShape::getClippingSet(QtRegion& clipframe) const
{
   pqvector<SalaEdgeU> edgeset;
   bool last_inside = (clipframe.contains_touch(at(0))) ? true : false;
   bool found_inside = last_inside;
   for (size_t i = 1; i < size(); i++) {
      bool next_inside = (clipframe.contains_touch(at(i))) ? true : false;
      found_inside |= next_inside;
      if (last_inside != next_inside) {
         if (last_inside) {
            EdgeU eu = clipframe.getCutEdgeU(at(i-1),at(i));
            edgeset.push_back(SalaEdgeU(i,false,eu));
         }
         else {
            EdgeU eu = clipframe.getCutEdgeU(at(i),at(i-1));
            edgeset.push_back(SalaEdgeU(i-1,true,eu));
         }
      }
      last_inside = next_inside;
   }
   if (!found_inside) {
      // note: deliberately add a single empty SalaEdgeU if this polygon is never inside the frame
      edgeset.push_back(SalaEdgeU());
   }
   return edgeset;
}


///////////////////////////////////////////////////////////////////////////////////

// copied from SpacePixel

PixelRef ShapeMap::pixelate( const Point2f& p, bool constrain, int ) const
{
   PixelRef r;

   Point2f p1 = p;
   p1.normalScale(m_region);  

   if (constrain) {
      if (p1.x <= 0.0) {
         r.x = 0;
      }
      else if (p1.x >= 1.0) {
         r.x = m_cols - 1;
      }
      else {
         r.x = short(floor(p1.x * m_cols));
      }
   }
   else {
      r.x = short(floor(p1.x * m_cols));
   }

   if (constrain) {
      if (p1.y <= 0.0) {
         r.y = 0;
      }
      else if (p1.y >= 1.0) {
         r.y = m_rows - 1;
      }
      else {
         r.y = short(floor(p1.y * m_rows));
      }
   }
   else {
      r.y = short(floor(p1.y * m_rows));
   }

   return r;
}

///////////////////////////////////////////////////////////////////////////////////

int ShapeMap::loadMifMap(istream& miffile, istream& midfile)
{
   m_mapinfodata = new MapInfoData;

   return m_mapinfodata->import(miffile, midfile, *this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::outputMifMap(ostream& miffile, ostream& midfile) const
{
   if (!m_mapinfodata) {
      MapInfoData mapinfodata;
      mapinfodata.exportFile(miffile, midfile, *this);
   }
   else {
      m_mapinfodata->exportFile(miffile, midfile, *this);
   }

   return true;
}


/////////////////////////////////////////////////////////////////////////////////

// Code for explicit linking / unlinking

bool ShapeMap::linkShapes(const Point2f& p)
{
   if (m_selection_set.size() != 1) {
      return false;
   }
   int index1 = *m_selection_set.begin();
   // note: uses rowid not key
   int index2 = pointInPoly(p);
   if (index2 == -1) {
      // try looking for a polyline instead
      index2 = getClosestOpenGeom(p);
   }
   if (index2 == -1) {
      return false;
   }
   clearSel();

   linkShapes(index1, index2);

   return true;
}

bool ShapeMap::linkShapes(int index1, int index2, bool refresh)
{
   int conn_col = m_attributes.getOrInsertLockedColumnIndex("Connectivity");
   bool update = false;

   if (index1 != index2) {
      // link these lines...
      // first look for explicit unlinks and clear
      OrderedIntPair link(index1,index2);
      size_t unlinkindex = m_unlinks.searchindex(link);
      if (unlinkindex != paftl::npos) {
         m_unlinks.remove_at(unlinkindex);
         update = true;
      }
      else {
         // then check not linked already
         size_t linkindex1 = m_connectors[index1].m_connections.searchindex(index2);
         size_t linkindex2 = m_connectors[index2].m_connections.searchindex(index1);
         if (linkindex1 == paftl::npos && linkindex2 == paftl::npos) {
            // finally, link the two lines
            m_links.add(link);
            update = true;
         }
      }
   }

   if (update) {
      m_connectors[index1].m_connections.add(index2);
      m_connectors[index2].m_connections.add(index1);
      m_attributes.incrValue(index1, conn_col);
      m_attributes.incrValue(index2, conn_col);
      if (refresh && getDisplayedAttribute() == conn_col) {
         invalidateDisplayedAttribute();
         setDisplayedAttribute(conn_col);  // <- reflect changes to connectivity counts
      }
   }

   return update;
}

// this version is used to link segments in segment analysis
// note it only links one way!
bool ShapeMap::linkShapes(int id1, int dir1, int id2, int dir2, float weight)
{
   int success = -1;
   if (dir1 == 1) {
      success = m_connectors[id1].m_forward_segconns.add(SegmentRef(dir2,id2),weight);
   }
   else {
      success = m_connectors[id1].m_back_segconns.add(SegmentRef(dir2,id2),weight);
   }

   // checking success != -1 avoids duplicate entries adding to connectivity
   if (success != -1) {
      int conn_col = m_attributes.getOrInsertLockedColumnIndex("Connectivity");
      m_attributes.incrValue(id1, conn_col);
      int weight_col = m_attributes.getOrInsertLockedColumnIndex("Weighted Connectivity");
      m_attributes.incrValue(id1, weight_col, weight);
   }

   return true;
}

bool ShapeMap::unlinkShapes(const Point2f& p)
{
   if (m_selection_set.size() != 1) {
      return false;
   }
   int index1 = *m_selection_set.begin();
   int index2 = pointInPoly(p);
   if (index2 == -1) {
      // try looking for a polyline instead
      index2 = getClosestOpenGeom(p);
   }
   if (index2 == -1) {
      return false;
   }
   clearSel();

   unlinkShapes(index1,index2);

   return true;
}

// note: uses rowids rather than shape key
bool ShapeMap::unlinkShapes(int index1, int index2, bool refresh)
{
   int conn_col = m_attributes.getColumnIndex("Connectivity");
   bool update = false;

   if (index1 != index2) {
      // unlink these shapes...
      // first look for explicit links and clear
      OrderedIntPair unlink(index1,index2);
      size_t linkindex = m_links.searchindex(unlink);
      if (linkindex != paftl::npos) {
         m_links.remove_at(linkindex);
         update = true;
      }
      else {
         // then check if linked already
         size_t linkindex1 = m_connectors[index1].m_connections.searchindex(index2);
         size_t linkindex2 = m_connectors[index2].m_connections.searchindex(index1);
         if (linkindex1 != paftl::npos && linkindex2 != paftl::npos) {
            // finally, unlink the two shapes
            m_unlinks.add(unlink);
            update = true;
         }
      }
   }

   if (update && conn_col != -1) {
      m_connectors[index1].m_connections.remove(index2);
      m_connectors[index2].m_connections.remove(index1);
      m_attributes.decrValue(index1, conn_col);
      m_attributes.decrValue(index2, conn_col);
      if (refresh && getDisplayedAttribute() == conn_col) {
         invalidateDisplayedAttribute();
         setDisplayedAttribute(conn_col);  // <- reflect changes to connectivity counts
      }
   }
   return update;
}

bool ShapeMap::clearLinks()
{
   for (size_t i = 0; i < m_unlinks.size(); i++) {
      OrderedIntPair link = m_unlinks[i];
      m_connectors[link.a].m_connections.add(link.b);
      m_connectors[link.b].m_connections.add(link.a);
   }
   m_unlinks.clear();

   for (size_t j = 0; j < m_links.size(); j++) {
      OrderedIntPair link = m_links[j];
      m_connectors[link.a].m_connections.remove(link.b);
      m_connectors[link.b].m_connections.remove(link.a);
   }
   m_links.clear();

   return true;
}

bool ShapeMap::unlinkShapeSet(istream& idset, int refcol)
{
   std::string line;
   prefvec<IntPair> unlinks;
   do {
      IntPair unlink;
      idset >> line;
      if (!line.empty()) {
         auto tokens = dXstring::split(line, '\t');
         if (tokens.size() < 2) {
            return false;
         }
         try {
            unlink.a = stoi(tokens[0]);
            unlink.b = stoi(tokens[1]);
            unlinks.push_back(unlink);
         }
         catch (pexception) {;}   // don't do anything if it can't parse the numbers, just ignore (e.g., first line)
      }
   } while (!idset.eof());

   AttributeIndex idx;
   if (refcol != -1) {
      idx.makeIndex(m_attributes,refcol,false);
   }

   for (size_t i = 0; i < unlinks.size(); i++) {
      if (refcol != -1) {
         size_t x;
         x = idx.searchindex(ValuePair(-1,unlinks[i].a));
         unlinks[i].a = (x == paftl::npos) ? -1 : idx[x].index;
         x = idx.searchindex(ValuePair(-1,unlinks[i].b));
         unlinks[i].b = (x == paftl::npos) ? -1 : idx[x].index;
      }
      if (unlinks[i].a != paftl::npos && unlinks[i].b != paftl::npos) {
         unlinkShapes(unlinks[i].a,unlinks[i].b,false);
      }
   }

   int conn_col = m_attributes.getColumnIndex("Connectivity");
   if (getDisplayedAttribute() == conn_col) {
      invalidateDisplayedAttribute();
      setDisplayedAttribute(conn_col);  // <- reflect changes to connectivity counts
   }

   return true;
}



/////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::findNextLinkLine() const
{
   if (m_curlinkline < (int)m_links.size()) {
      m_curlinkline++;
   }
   return (m_curlinkline < (int)m_links.size());
}

Line ShapeMap::getNextLinkLine() const
{
   // note, links are stored directly by rowid, not by key:
   if (m_curlinkline < (int)m_links.size()) {
      return Line(m_shapes.value(m_links[m_curlinkline].a).getCentroid(), 
                  m_shapes.value(m_links[m_curlinkline].b).getCentroid() );
   }
   return Line();
}

std::vector<SimpleLine> ShapeMap::getAllLinkLines()
{
    std::vector<SimpleLine> linkLines;
    for(size_t i = 0; i < m_links.size(); i++)
    {
        linkLines.push_back(SimpleLine(
                                m_shapes.value(m_links[i].a).getCentroid(),
                                m_shapes.value(m_links[i].b).getCentroid()
                                ));
    }
    return linkLines;
}

// note: these functions would need slight work for arbitrary shape overlaps

bool ShapeMap::findNextUnlinkPoint() const
{
   if (m_curunlinkpoint < (int)m_unlinks.size()) {
      m_curunlinkpoint++;
   }
   return (m_curunlinkpoint < (int)m_unlinks.size());
}

Point2f ShapeMap::getNextUnlinkPoint() const
{
   // note, links are stored directly by rowid, not by key:
   if (m_curunlinkpoint < (int)m_unlinks.size()) {
      return intersection_point(m_shapes.value(m_unlinks[m_curunlinkpoint].a).getLine(), 
                                m_shapes.value(m_unlinks[m_curunlinkpoint].b).getLine(), TOLERANCE_A);
   }
   return Point2f();
}
std::vector<Point2f> ShapeMap::getAllUnlinkPoints()
{
    std::vector<Point2f> unlinkPoints;
    for(size_t i = 0; i < m_unlinks.size(); i++)
    {
        unlinkPoints.push_back(intersection_point(m_shapes.value(m_unlinks[i].a).getLine(),
                                                  m_shapes.value(m_unlinks[i].b).getLine(), TOLERANCE_A));
    }
    return unlinkPoints;

}

void ShapeMap::outputUnlinkPoints( ofstream& stream, char delim )
{
   stream << "x" << delim << "y" << endl;

   stream.precision(12);
   for (size_t i = 0; i < m_unlinks.size(); i++) {
      // note, links are stored directly by rowid, not by key:
      Point2f p = intersection_point(m_shapes.value(m_unlinks[i].a).getLine(), 
                                     m_shapes.value(m_unlinks[i].b).getLine(), TOLERANCE_A);
      stream << p.x << delim << p.y << endl;
   }
}



/////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::makeBSPtree() const
{
   if (m_bsp_tree) {
      return true;
   }

   std::vector<TaggedLine> partitionlines;
   for (size_t i = 0; i < m_shapes.size(); i++) {
      if (m_shapes[i].isLine()) {
         partitionlines.push_back(TaggedLine(m_shapes.value(i).getLine(),m_shapes.key(i)));
      }
   }

   if (partitionlines.size()) {
      //
      // Now we'll try the BSP tree:
      //
      if (m_bsp_root) {
         delete m_bsp_root;
         m_bsp_root = NULL;
      }
      m_bsp_root = new BSPNode();

      BSPTree::make(NULL,0,partitionlines,m_bsp_root);
      m_bsp_tree = true;
   }

   partitionlines.clear();

   return m_bsp_tree;
}

/////////////////////////////////////////////////////////////////////////////////

// SPECIALS BELOW

////////////////////////////////////////////////////////////////////////////////////////////////

int findwinner(double *bins, int bincount, int& difficult, int& impossible)
{
   difficult = 0;
   impossible = 0;
   //
   double total = 0.0;
   //
   double maxvalue = -1.0;
   int maxbin = -1;
   int i;
   for (i = 0; i < bincount; i++) {
      if (i == 0 || bins[i] > maxvalue) {
         maxvalue = bins[i];
         maxbin = i;
      }
      total += bins[i];
   }
   if (maxvalue > total * 0.8) {
      return maxbin;
   }
   int lastwinner = maxbin;
   // no immediate clear winner, so see if across two adjacent bins:
   double savebin = bins[bincount-1];
   double savebins0 = bins[0];
   for (i = 0; i < bincount - 1; i++) {
      double lastbin = savebin;
      savebin = bins[i];
      bins[i] += bins[i+1] + lastbin;
   }
   bins[bincount-1] += savebins0 + savebin;

   // now check again for a clear winner:
   maxvalue = -1.0;
   maxbin = -1;
   for (i = 0; i < bincount; i++) {
      if (i == 0 || bins[i] > maxvalue) {
         maxvalue = bins[i];
         maxbin = i;
      }
   }
   // if it's a tie, the last winner wins it:
   if (maxbin != lastwinner && maxvalue == bins[lastwinner]) {
      maxbin = lastwinner;
   }
   //
   if (maxvalue > total * 0.8) {
      return maxbin;
   }
   //
   // now it's at least hard:
   if (maxvalue > total * 0.6) {
      difficult = 1;
      return maxbin;
   }
   //
   // if not even this is true, it's really a guess in the dark:
   impossible = 1;
   return maxbin;
}


// Quick mod - TV
#if defined(_WIN32)
#include <windows.h>
#endif

// this is similar to the ozlemSpecial version below, but puts axes into a shape layer rather than exporting
void ShapeMap::ozlemSpecial(ShapeMap& output)
{
   // output needs 4 lines per building, but this uses too much mem -- so just initialise as per buildings:
   output.init(m_shapes.size(),QtRegion(m_region.bottom_left+Point2f(-50.0,-50.0),m_region.top_right+Point2f(50.0,50.0)));

   int inrefcol = m_attributes.getColumnIndex("PolyID");
   int exclcol = m_attributes.insertColumn("Poss problem");
   int outrefcol = output.m_attributes.insertColumn("BuildingPolyID");
   int outorientcol = output.m_attributes.insertColumn("Orientation");
   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      SalaShape& poly = m_shapes[i];
      if (!pointInPoly(poly.getCentroid(),m_shapes.key(i))) {
         m_attributes.setValue(i,exclcol,1);
      }
      else {
         m_attributes.setValue(i,exclcol,0);
      }
      // buildintid just used in output as key for MapInfo table
      int buildintid = (int) m_attributes.getValue(i,inrefcol);
      //
      // put the poly edges into bins:
      double bins64[16];
      size_t j;
      for (j = 0; j < 16; j++) {
         bins64[j] = 0;
      }
      Point2f start = poly[0];
      for (j = 1; j < poly.size() + 1; j++) {
         Point2f end = (j != poly.size()) ? poly[j] : poly[0];
         {
            Point2f vec = end - start;
            double len = vec.length();
            vec.normalise();
            double ang = vec.angle(); // test angle okay
            ang /= (M_PI * 0.5);
            while (ang > 1) {
               ang -= 1;
            }
            
      	    bins64[(int)(round(ang * 16)) % 16] += len;
         }
         start = end;
      }
      int difficult = 0, impossible = 0;
      int winner64 = findwinner(bins64,16,difficult,impossible);
      double ang = M_PI * 0.5 * double(winner64) / 16;
      Point2f axis(cos(ang),sin(ang));
      axis.scale(75.0);
      for (j = 0; j < 4; j++) {
         output.makeLineShape(Line(poly.getCentroid(),poly.getCentroid()+axis));
         // n.b., should be last one added, but this code is not strictly safe:
         output.m_attributes.setValue(output.m_attributes.getRowCount()-1,outrefcol,(float)buildintid);
         output.m_attributes.setValue(output.m_attributes.getRowCount()-1,outorientcol,(float)j);

         axis.rotate(M_PI * 0.5);
      }
   }
   
   output.invalidateDisplayedAttribute();
   output.setDisplayedAttribute(outrefcol);
}

// expects to be the rays layer... check intersection of each ray with building polys
void ShapeMap::ozlemSpecial2(ShapeMap& buildings)
{
   int myrefcol = m_attributes.getColumnIndex("BuildingPolyID");
   int itsrefcol = buildings.m_attributes.getColumnIndex("PolyID");

   pvecint removelist;

   size_t i;
   for (i = 0; i < m_shapes.size(); i++) {
      Line& li = m_shapes[i].m_region;
      int buildingid = (int) m_attributes.getValue(i,myrefcol);
      pvector<ValuePair> cuts;
      buildings.getShapeCuts(li, cuts);
      Point2f mepoint;
      bool meencountered = false, chopped = false;
      if (li.width() > li.height()) {
         // these are xaxis cuts
         if (li.rightward()) {
            // in the correct order:
            for (size_t j = 0; j < cuts.size(); j++) {
               int itsrowid = buildings.m_attributes.getRowid(cuts[j].index);
               if (buildingid != buildings.m_attributes.getValue(itsrowid,itsrefcol)) {
                  Point2f newend = li.point_on_line(cuts[j].value + 0.1,XAXIS);  // + 0.1 metres for tolerance later
                  if (!meencountered || dist(newend,mepoint) < 1.5) {
                     removelist.push_back(i);
                  }
                  else {
                     // section it here!
                     removePolyPixels(m_shapes.key(i));
                     m_shapes[i].m_region = Line(mepoint,newend);
                     makePolyPixels(m_shapes.key(i));
                  }
                  chopped = true;
                  break;
               }
               else {
                  mepoint = li.point_on_line(cuts[j].value - 0.1,XAXIS);   // -0.1 metres for tolerance later
                  meencountered = true;
               }
            }
         }
         else {
            // in reverse order:
            for (size_t j = cuts.size() - 1; j != paftl::npos; j--) {
               int itsrowid = buildings.m_attributes.getRowid(cuts[j].index);
               if (buildingid != buildings.m_attributes.getValue(itsrowid,itsrefcol)) {
                  Point2f newend = li.point_on_line(cuts[j].value - 0.1,XAXIS);   // -0.1 metres for tolerance later
                  if (!meencountered || dist(newend,mepoint) < 1.5) {
                     removelist.push_back(i);
                  }
                  else {
                     // section it here!
                     removePolyPixels(m_shapes.key(i));
                     m_shapes[i].m_region = Line(mepoint,newend);
                     makePolyPixels(m_shapes.key(i));
                     chopped = true;
                  }
                  chopped = true;
                  break;
               }
               else {
                  mepoint = li.point_on_line(cuts[j].value + 0.1,XAXIS);   // +0.1 metres for tolerance later
                  meencountered = true;
               }
            }
         }
      }
      else {
         if (li.upward()) {
            for (size_t j = 0; j < cuts.size(); j++) {
               int itsrowid = buildings.m_attributes.getRowid(cuts[j].index);
               if (buildingid != buildings.m_attributes.getValue(itsrowid,itsrefcol)) {
                  Point2f newend = li.point_on_line(cuts[j].value + 0.1,YAXIS);   // +0.1 metres for tolerance later
                  if (!meencountered || dist(newend,mepoint) < 1.5) {
                     removelist.push_back(i);
                  }
                  else {
                     // section it here!
                     removePolyPixels(m_shapes.key(i));
                     m_shapes[i].m_region = Line(mepoint,newend);
                     makePolyPixels(m_shapes.key(i));
                  }
                  chopped = true;
                  break;
               }
               else {
                  mepoint = li.point_on_line(cuts[j].value - 0.1,YAXIS);   // -0.1 metres for tolerance later
                  meencountered = true;
               }
            }
         }
         else {
            // in reverse order:
            for (size_t j = cuts.size() - 1; j != paftl::npos; j--) {
               int itsrowid = buildings.m_attributes.getRowid(cuts[j].index);
               if (buildingid != buildings.m_attributes.getValue(itsrowid,itsrefcol)) {
                  Point2f newend = li.point_on_line(cuts[j].value - 0.1,YAXIS);   // -0.1 metres for tolerance later
                  if (!meencountered || dist(newend,mepoint) < 1.5) {
                     removelist.push_back(i);
                  }
                  else {
                     // section it here!
                     removePolyPixels(m_shapes.key(i));
                     m_shapes[i].m_region = Line(mepoint,newend);
                     makePolyPixels(m_shapes.key(i));
                  }
                  chopped = true;
                  break;
               }
               else {
                  mepoint = li.point_on_line(cuts[j].value + 0.1,YAXIS);   // +0.1 metres for tolerance later
                  meencountered = true;
               }
            }
         }
      }
      if (!chopped && meencountered) {
         // section it here!
         removePolyPixels(m_shapes.key(i));
         m_shapes[i].m_region = Line(mepoint,li.t_end());
         makePolyPixels(m_shapes.key(i));
      }
   }

   for (i = 0; i < removelist.size(); i++) {
      removePolyPixels(m_shapes.key(removelist[i]));
   }
   m_shapes.remove_at(removelist);
   m_attributes.removeRowids(removelist);

   m_newshape = true;
   m_displayed_attribute = -2;
   setDisplayedAttribute(-1);
}

// expects to be the rays layer... check intersection of each ray with *all* O.S. polys
void ShapeMap::ozlemSpecial3(ShapeMap& all)
{
   pvecint removelist;

   int intid = m_attributes.insertColumn("ExposurePolyID");
   int featcode = m_attributes.insertColumn("FeatureCode");

   IntPair lookupcols;
   lookupcols.a = all.m_attributes.getColumnIndex("PolyID");
   lookupcols.b = all.m_attributes.getColumnIndex("FeatureCode");

   size_t i;
   for (i = 0; i < m_shapes.size(); i++) {
      Line& li = m_shapes[i].m_region;
      pvector<ValuePair> cuts;
      all.getShapeCuts(li, cuts);
      Point2f endpoint;
      int state = 0;
      IntPair previous;
      if (li.width() > li.height()) {
         // these are xaxis cuts
         if (li.rightward()) {
            // in the correct order:
            for (size_t j = 0; j < cuts.size() && state < 2; j++) {
               if (ozlemSpecial4(cuts[j],previous,state,all.m_attributes,lookupcols)) { // something of note encountered tag location:
                  endpoint = li.point_on_line(cuts[j].value,XAXIS);
               }
            }
         }
         else {
            // in reverse order:
            for (size_t j = cuts.size() - 1; j != paftl::npos && state < 2; j--) {
               if (ozlemSpecial4(cuts[j],previous,state,all.m_attributes,lookupcols)) { // something of note encountered tag location:
                  endpoint = li.point_on_line(cuts[j].value,XAXIS);
               }
            }
         }
      }
      else {
         if (li.upward()) {
            for (size_t j = 0; j < cuts.size() && state < 2; j++) {
               if (ozlemSpecial4(cuts[j],previous,state,all.m_attributes,lookupcols)) { // something of note encountered tag location:
                  endpoint = li.point_on_line(cuts[j].value,YAXIS);
               }
            }
         }
         else {
            // in reverse order:
            for (size_t j = cuts.size() - 1; j != paftl::npos && state < 2; j--) {
               if (ozlemSpecial4(cuts[j],previous,state,all.m_attributes,lookupcols)) { // something of note encountered tag location:
                  endpoint = li.point_on_line(cuts[j].value,YAXIS);
               }
            }
         }
      }
      if (state != 0) {
         // section it here!
         removePolyPixels(m_shapes.key(i));
         m_shapes[i].m_region = Line(li.t_start(),endpoint);
         makePolyPixels(m_shapes.key(i));
         m_attributes.setValue(i,intid,(float)previous.a);
         m_attributes.setValue(i,featcode,(float)previous.b);
      }
      else {
         // didn't encounter anything, remove it:
         removelist.push_back(i);
      }
   }

   for (i = 0; i < removelist.size(); i++) {
      removePolyPixels(m_shapes.key(removelist[i]));
   }
   m_shapes.remove_at(removelist);
   m_attributes.removeRowids(removelist);

   m_newshape = true;
   m_displayed_attribute = -2;
   setDisplayedAttribute(-1);
}

bool ShapeMap::ozlemSpecial4(ValuePair& cut, IntPair& previous, int& state, AttributeTable& table, IntPair& lookupcols)
{
   int rowid = table.getRowid(cut.index);
   IntPair current;
   current.a = (int)table.getValue(rowid,lookupcols.a);
   current.b = (int)table.getValue(rowid,lookupcols.b);
   if (current.b == 10021 || current.b == 10062 || current.b == 10185 || current.b == 10187) {
      // building, glasshouse, or other building structures -- ignore, and don't alter the status:
      return false;
   }
   if (current.b == 10053) {
      if (previous.b == 10053) {
         if (current.a != previous.a) {
            // two adjacent parcels: finish up, and mark location:
            state = 2;
            return true;
         }
         // otherwise it's the same parcel: just ignore it
         return false;
      }
      if (previous.b == -1) {
         // first parcel cut, record and continue:
         previous = current;
         return false;
      }
      // otherwise there's been a previous feature:- only cut it if you have entered and exited the previous feature:
      if (state == -1) {
         state = 2;
         return false;  // <- cut at previous feature encounter, not this one
      }
      // otherwise continue as before
      return false;
   }
   if (current.b == 10172 || current.b == 10167) {
      // major feature type -- show this one, and cut here:
      previous = current;
      state = 2;
      return true;
   }
   // any other code
   if (previous.b == -1 || previous.b == 10053) {
      // record if nothing in buffer or last was a land parcel and mark as found
      previous = current;
      state = 1;
      return true;
   }
   else if (current.a == previous.a) {
      state = -1; // exiting a feature: mark in case 10053 found
      return false;
   }
   return false;
}




// expects to be the rays layer... check intersection of each ray with *all* O.S. polys
void ShapeMap::ozlemSpecial5(ShapeMap& buildings)
{
   pmap<int,int> exposureranks;
   exposureranks.add(10172,10);
   exposureranks.add(10183,9);
   exposureranks.add(10056,8);
   exposureranks.add(10054,7);
   exposureranks.add(10123,6);
   exposureranks.add(10119,5);
   exposureranks.add(10111,4);
   exposureranks.add(10217,3);
   exposureranks.add(10167,2);
   exposureranks.add(10089,1);
   exposureranks.add(10053,0);
   exposureranks.add(-1,-1);

   int inorientcol = m_attributes.getColumnIndex("Orientation");
   int incodecol = m_attributes.getColumnIndex("FeatureCode");
   //
   for (int ii = 0; ii < m_attributes.getRowCount(); ii++) {
      int featcode = (int)m_attributes.getValue(ii,incodecol);
      if (exposureranks.searchindex(featcode) == paftl::npos) {
         std::string blah = dXstring::formatString(featcode,"Error: wasn't expecting feature code %d");
         // Quick mod - TV
         // MessageBox(NULL,blah.c_str(),"Error: ozlemSpecial5",MB_OK|MB_ICONEXCLAMATION);
         fprintf(stderr, "%s --- %s\n", blah.c_str(), "Error: ozlemSpecial5");
         return;
      }
   }

   int inbuildcol = m_attributes.getColumnIndex("BuildingPolyID");
   AttributeIndex idxlines;
   if (inbuildcol != -1) {
      idxlines.makeIndex(m_attributes,inbuildcol,false);
   }
   else {
      // Quick mod - TV
      // MessageBox(NULL,"Error: no PolyID column found in the lines table","Error: ozlemSpecial5",MB_OK|MB_ICONEXCLAMATION);
      fprintf(stderr, "%s --- %s\n", "Error: no PolyID column found in the lines table", "Error: ozlemSpecial5");
      return;
   }
   //
   int outtypecol = buildings.m_attributes.insertColumn("ExposureType");
   int outfacecol = buildings.m_attributes.insertColumn("FacesExposed");
   int outprimcol = buildings.m_attributes.insertColumn("PrimaryExposure");
   int outsecocol = buildings.m_attributes.insertColumn("SecondaryExposure");

   int outbuildcol = buildings.m_attributes.getColumnIndex("PolyID");
   AttributeIndex idxbuildings;
   if (outbuildcol != -1) {
      idxbuildings.makeIndex(buildings.m_attributes,outbuildcol,false);
   }
   else {
      // Quick mod - TV
      // MessageBox(NULL,"Error: no PolyID column found in the buildings table","Error: ozlemSpecial5",MB_OK|MB_ICONEXCLAMATION);
      fprintf(stderr, "%s --- %s\n", "Error: no PolyID column found in the buildings table","Error: ozlemSpecial5");
      return;
   }


   // Quick mod - TV
   // MessageBox(NULL,"Message: about to start main loop","Message: ozlemSpecial5",MB_OK|MB_ICONINFORMATION);
   fprintf(stderr, "%s --- %s\n", "Message: about to start main loop","Message: ozlemSpecial5");

   int start = 0;
   for (size_t i = 1; i <= idxlines.size(); i++) {
      if (idxlines.size() == i || idxlines[i].value != idxlines[start].value) {
         ValuePair buildinglkup;
         buildinglkup.value = idxlines[start].value;
         size_t idxb = idxbuildings.searchindex(buildinglkup);
         if (idxb == paftl::npos) {
            std::string blah = dXstring::formatString(idxlines[start].value,"Error: couldn't find PolyID %d in Buildings table");
            // Quick mod - TV
            // MessageBox(NULL,blah.c_str(),"Error: ozlemSpecial5",MB_OK|MB_ICONEXCLAMATION);
    	    fprintf(stderr, "%s --- %s\n",blah.c_str(),"Error: ozlemSpecial5");
            return;
         }
         int browid = idxbuildings[idxb].index;
         // process: stage 1, orientation -- note orient is *ordered*
         pvector<IntPair> exposurelist;
         int risks = 0;
         for (size_t j = start; j < i; j++) {
            int feature = (int)m_attributes.getValue(idxlines[j].index,incodecol);
            int orientation = (int)m_attributes.getValue(idxlines[j].index,inorientcol);
            if (feature != 10053) { // don't count parcel boundaries towards exposure
               exposurelist.add(IntPair(orientation,feature));
            }
         }
         if (exposurelist.size() == 0) {
            buildings.m_attributes.setValue(browid,outfacecol,0);
            buildings.m_attributes.setValue(browid,outtypecol,0);
         }
         else {
            switch (exposurelist.size()) {
            case 1:
               buildings.m_attributes.setValue(browid,outfacecol,1);
               buildings.m_attributes.setValue(browid,outtypecol,1);
               buildings.m_attributes.setValue(browid,outprimcol,(float)exposurelist[0].b);
               break;
            case 2:
               buildings.m_attributes.setValue(browid,outfacecol,2);
               if ((exposurelist[0].a + 2) % 4 == exposurelist[1].a) {
                  buildings.m_attributes.setValue(browid,outtypecol,2);
               }
               else {
                  buildings.m_attributes.setValue(browid,outtypecol,3);
               }
               if (exposureranks.search(exposurelist[0].b) > exposureranks.search(exposurelist[1].b)) {
                  buildings.m_attributes.setValue(browid,outprimcol,(float)exposurelist[0].b);
                  buildings.m_attributes.setValue(browid,outsecocol,(float)exposurelist[1].b);
               }
               else {
                  buildings.m_attributes.setValue(browid,outprimcol,(float)exposurelist[1].b);
                  buildings.m_attributes.setValue(browid,outsecocol,(float)exposurelist[0].b);
               }
               break;
            case 3:
               {
                  buildings.m_attributes.setValue(browid,outfacecol,3);
                  bool rearexposed = false;
                  int prim = -1, seco = -1;
                  for (int i = 0; i < 3; i++) {
                     if (exposurelist[i].b == 10172) {
                        if (exposurelist[(i+1)%3].a == (exposurelist[i].a + 2) % 2 || 
                            exposurelist[(i+2)%3].a == (exposurelist[i].a + 2) % 2) {
                           rearexposed = true;
                        }
                     }
                     if (exposureranks.search(exposurelist[i].b) > exposureranks.search(prim)) {
                        seco = prim;
                        prim = exposurelist[i].b;
                     }
                     else if (exposureranks.search(exposurelist[i].b) > exposureranks.search(seco)) {
                        seco = exposurelist[i].b;
                     }
                  }
                  buildings.m_attributes.setValue(browid,outtypecol,(float)(rearexposed?4:3));
                  buildings.m_attributes.setValue(browid,outprimcol,(float)prim);
                  buildings.m_attributes.setValue(browid,outsecocol,(float)seco);
               }
               break;
            case 4:
               {
                  buildings.m_attributes.setValue(browid,outfacecol,4);
                  buildings.m_attributes.setValue(browid,outtypecol,4);
                  int prim = -1, seco = -1;
                  for (int i = 0; i < 4; i++) {
                     if (exposureranks.search(exposurelist[i].b) > exposureranks.search(prim)) {
                        seco = prim;
                        prim = exposurelist[i].b;
                     }
                     else if (exposureranks.search(exposurelist[i].b) > exposureranks.search(seco)) {
                        seco = exposurelist[i].b;
                     }
                  }
                  buildings.m_attributes.setValue(browid,outprimcol,(float)prim);
                  buildings.m_attributes.setValue(browid,outsecocol,(float)seco);
               }
               break;
            }
         }
         start = i;
      }
   }

   // Quick mod - TV
   // MessageBox(NULL,"Message: finished main loop","Message: ozlemSpecial5",MB_OK|MB_ICONINFORMATION);
   fprintf(stderr, "%s --- %s\n","Message: finished main loop","Message: ozlemSpecial5");

   buildings.m_displayed_attribute = -2;
   buildings.setDisplayedAttribute(outtypecol);
}

void ShapeMap::ozlemSpecial6() // ShapeMap& border)
{
   int count = 0;
   int delcol = m_attributes.insertColumn("Delete");
   int dupcol = m_attributes.insertColumn("Duplicate");
   for (size_t i = 0; i < m_shapes.size(); i++) {
      bool duplicate = false;
      bool tag_delete = false;
      PixelRef p = pixelate(m_shapes[i].getPoint());
      pqvector<ShapeRef>& sr = m_pixel_shapes[p.x][p.y];
      for (size_t j = 0; j < sr.size(); j++) {
         if (sr[j].m_shape_ref != m_shapes.key(i)) {
            if (m_shapes[i].getPoint() == m_shapes.search(sr[j].m_shape_ref).getPoint()) {
               duplicate = true;

               // Quick mod - TV
               if ((unsigned int)m_shapes.key(i) > sr[j].m_shape_ref) {
                  tag_delete = true;
               }
            }
         }
      }
      if (duplicate) {
         m_attributes.setValue(i,dupcol,1.0);
         count++;
      }
      else {
         m_attributes.setValue(i,dupcol,0.0);
      }
      if (tag_delete) {
         m_attributes.setValue(i,delcol,1.0);
      }
      else {
         m_attributes.setValue(i,delcol,0.0);
      }
   }
   std::string blah;
   blah = dXstring::formatString(count,"%d duplicates found");

   // Quick mod - TV
   // MessageBox(NULL,blah.c_str(),"Message: ozlemSpecial6",MB_OK|MB_ICONINFORMATION);
   fprintf(stderr, "%s --- %s\n",blah.c_str(),"Message: ozlemSpecial6");

}


// expects to be the points layer... expects passed map to be lines layer
void ShapeMap::ozlemSpecial7(ShapeMap& linemap)
{
   int linerefcol = m_attributes.insertColumn("Line Ref");
   
   for (size_t i = 0; i < m_shapes.size(); i++) {
      Point2f p = m_shapes[i].getPoint();
      int index = linemap.getClosestLine(p);
      m_attributes.setValue(i,linerefcol,(float)index);
   }

   m_displayed_attribute = -2;
   setDisplayedAttribute(linerefcol);
}

std::vector<SimpleLine> ShapeMap::getAllShapesAsLines() {
    std::vector<SimpleLine> lines;
    pqmap<int,SalaShape>& allShapes = getAllShapes();
    for (size_t k = 0; k < allShapes.size(); k++) {
        SalaShape& shape = allShapes[k];
        if (shape.isLine()) {
            lines.push_back(SimpleLine(shape.getLine()));
        }
        else if (shape.isPolyLine() || shape.isPolygon()) {
            for (size_t n = 0; n < shape.size() - 1; n++) {
                lines.push_back(SimpleLine(shape[n],shape[n+1]));
            }
            if (shape.isPolygon()) {
                lines.push_back(SimpleLine(shape.tail(),shape.head()));
            }
        }
    }
    return lines;
}

std::vector<std::pair<SimpleLine, PafColor>> ShapeMap::getAllLinesWithColour() {
    std::vector<std::pair<SimpleLine, PafColor>> colouredLines;
    const AttributeTable &attributeTable = getAttributeTable();
    pqmap<int,SalaShape>& allShapes = getAllShapes();
    for (size_t k = 0; k < allShapes.size(); k++) {
        SalaShape& shape = allShapes[k];
        PafColor color(attributeTable.getDisplayColor(k));
        if (shape.isLine()) {
            colouredLines.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(shape.getLine()), color));
        }
        else if (shape.isPolyLine()) {
            for (size_t n = 0; n < shape.size() - 1; n++) {
                colouredLines.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(shape[n],shape[n+1]), color));
            }
        }
    }
    return colouredLines;
}

std::map<std::vector<Point2f>, PafColor> ShapeMap::getAllPolygonsWithColour() {
    std::map<std::vector<Point2f>, PafColor> colouredPolygons;
    const AttributeTable &attributeTable = getAttributeTable();
    pqmap<int,SalaShape>& allShapes = getAllShapes();
    for (size_t k = 0; k < allShapes.size(); k++) {
        SalaShape& shape = allShapes[k];
        if (shape.isPolygon()) {
            std::vector<Point2f> vertices;
            for (size_t n = 0; n < shape.size(); n++) {
                vertices.push_back(shape[n]);
            }
            vertices.push_back(shape.tail());
            PafColor colour(attributeTable.getDisplayColor(k));
            colouredPolygons.insert(std::make_pair(vertices, colour));
        }
    }
    return colouredPolygons;
}
