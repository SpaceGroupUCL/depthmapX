#include "mgraph440/shapemap.h"
#include "mgraph440/pointmap.h"
#include "mgraph440/spacepix.h"
#include "mgraph440/exceptions.h"

namespace mgraph440 {

static const double TOLERANCE_A = 1e-9;

// import TOLERANCE_B from axial map...
static const double TOLERANCE_B = 1e-12;

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

bool ShapeMap::read( ifstream& stream, int version, bool drawinglayer )
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

   // read in an old file:
   if (drawinglayer && version < VERSION_DRAWING_SHAPES) {
      // the data in the file is for a SpacePixel
      // the easiest solution, although not the most memory effective, is to read in the space pixel,
      // and convert to a shape map:
      SpacePixel layer;
      layer.read(stream,version);
      m_name = layer.m_name;
      m_show = layer.m_show;
      m_editable = false;  // <- don't take from spacepixel in case conflicts in some way
      m_region = layer.m_region;
      m_rows = layer.m_rows;
      m_cols = layer.m_cols;
      m_tolerance = __max(m_region.width(), m_region.height()) * TOLERANCE_A;
      m_shape_ref = -1;
      for (size_t i = 0; i < layer.m_lines.size(); i++) {
         m_shape_ref++;
         int index = m_shapes.add(m_shape_ref, SalaShape(layer.m_lines[i].line));
         // insert a dummy attribute row:
         m_attributes.insertRow(m_shape_ref);
         // note: as this is always a drawing layer, no need to set shape attributes
      }
      // prepare pixel map (using same number of cols and rows as the original spacepixel for convenience)
      m_pixel_shapes = new pqvector<ShapeRef> *[m_cols];
      for (int j = 0; j < m_cols; j++) {
         m_pixel_shapes[j] = new pqvector<ShapeRef>[m_rows];
      }
      // Now add the pixel shapes pixel map:
      // pixelate all polys in the pixel structure:
      for (size_t k = 0; k < m_shapes.size(); k++) {
         makePolyPixels(m_shapes.key(k));
      }
      // set to read in display attribute:
      // note that even though it's only a drawing layer, this still needs to be done
      invalidateDisplayedAttribute();
      setDisplayedAttribute(-1);

      // all done
      return true;
   }

   // name
   m_name = dXstring440::readString(stream);

   if (version >= VERSION_MAP_TYPES) {
      stream.read( (char *) &m_map_type, sizeof(m_map_type));
   }
   else {
      // old versions data maps or drawing maps,
      // the axial reader will override this with its own map type designation if necessary
      if (drawinglayer) {
         m_map_type = DRAWINGMAP;
      }
      else {
         m_map_type = DATAMAP;
      }
   }

   if (version >= VERSION_DRAWING_SHAPES_B) {
      stream.read( (char *) &m_show, sizeof(m_show) );
      stream.read( (char *) &m_editable, sizeof(m_editable) );
   }

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
   if (version < VERSION_SHAPE_CENTROIDS) {
      // manually set centroid according to type:
      for (size_t i = 0; i < m_shapes.size(); i++) {
         switch (m_shapes[i].m_type & SalaShape::SHAPE_TYPE) {
         case SalaShape::SHAPE_POINT:
            m_shapes[i].m_centroid = m_shapes[i].m_region.bottom_left;
            break;
         case SalaShape::SHAPE_LINE:
            m_shapes[i].m_centroid = m_shapes[i].m_region.getCentre();
            break;
         case SalaShape::SHAPE_POLY:
            m_shapes[i].setCentroidAreaPerim();
            break;
         default:
            break;
         }
      }
   }

   // read object data (currently unused)
   stream.read((char *) &count, sizeof(count));
   for (int k = 0; k < count; k++) {
      int key;
      stream.read((char *) &key, sizeof(key));
      int index = m_objects.add(key, SalaObject());
      m_objects.value(index).read(stream,version);
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

   // later versions can have shape connections:
   if (version >= VERSION_AXIAL_SHAPES) {
      int count;
      stream.read((char *)&count,sizeof(count));
      for (int i = 0; i < count; i++) {
         m_connectors.push_back(Connector());
         m_connectors[i].read(stream,version);
         if (version < VERSION_NO_SELF_CONNECTION) {
            size_t self = m_connectors[i].m_connections.searchindex(i);
            if (self != paftl::npos) {
               m_connectors[i].m_connections.remove_at(self);
            }
         }
      }
      m_links.read(stream);
      m_unlinks.read(stream);
   }

   // some miscellaneous extra data for mapinfo files
   if (m_mapinfodata) {
      delete m_mapinfodata;
      m_mapinfodata = NULL;
   }
   if (version >= VERSION_MAPINFO_SHAPES) {
      char x = stream.get();
      if (x == 'm') {
         m_mapinfodata = new MapInfoData;
         m_mapinfodata->read(stream,version);
      }
   }

   invalidateDisplayedAttribute();
   setDisplayedAttribute(m_displayed_attribute);

   return true;
}
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

bool SalaShape::read(ifstream& stream, int version)
{
   // defaults
   m_draworder = -1;
   m_selected = false;

   stream.read((char *)&m_type,sizeof(m_type));

   int sss = sizeof(m_region);
   stream.read((char *)&m_region,sizeof(m_region));

   if (version >= VERSION_SHAPE_CENTROIDS) {
      stream.read((char *)&m_centroid,sizeof(m_centroid));
      if (version >= VERSION_SHAPE_AREA_PERIMETER) {
         stream.read((char *)&m_area,sizeof(m_area));
         stream.read((char *)&m_perimeter,sizeof(m_perimeter));
      }
   }
   else {
      // old types were simply 1,2,3... these are now labelled using bits:
      if (m_type == 3) {
         m_type = SHAPE_POLY;
      }
      else if (m_type == 4) {
         m_type = SHAPE_POLY | SHAPE_CLOSED;
      }
   }
   pqvector<Point2f>::read(stream);

   if (version < VERSION_SHAPE_AREA_PERIMETER) {
      if (m_type & SHAPE_POLY) {
         setCentroidAreaPerim();
      }
      else if (m_type & SHAPE_LINE) {
         m_perimeter = m_region.length();
      }
   }

   return true;
}

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
      throw depthmapX440::RuntimeException("this function is to be used for polygons only");
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

bool ShapeMap::write(ostream &stream, int version )
{
   // name
   dXstring440::writeString(stream, m_name);

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
   for (int k = 0; k < count; k++) {
      int key = m_objects.key(k);
      stream.write((char *) &key, sizeof(key));
      m_objects.value(k).write(stream);
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

bool SalaShape::write(ostream &stream)
{
   stream.write((char *)&m_type,sizeof(m_type));
   stream.write((char *)&m_region,sizeof(m_region));
   stream.write((char *)&m_centroid,sizeof(m_centroid));
   stream.write((char *)&m_area,sizeof(m_area));
   stream.write((char *)&m_perimeter,sizeof(m_perimeter));
   pqvector<Point2f>::write(stream);
   return true;
}

}
