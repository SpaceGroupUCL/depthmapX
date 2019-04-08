#pragma once

#include "mgraph440/pixelbase.h"
#include "mgraph440/mapinfodata.h"
#include "mgraph440/attributes.h"
#include "mgraph440/connector.h"
#include "mgraph440/paftl.h"
#include "mgraph440/mgraph_consts.h"
#include "mgraph440/stringutils.h"
#include "mgraph440/bspnode.h"

namespace mgraph440 {

class SalaShape : public pqvector<Point2f>
{
public:
   enum {SHAPE_POINT = 0x01, SHAPE_LINE = 0x02, SHAPE_POLY = 0x04, SHAPE_CIRCLE = 0x08, SHAPE_TYPE = 0x0f, SHAPE_CLOSED = 0x40, SHAPE_CCW = 0x80 };
   friend class ShapeMap;

   unsigned char m_type;
   Point2f m_centroid; // centre of mass, but also used as for point if object is a point
   Line m_region; // bounding box, but also used as a line if object is a line, hence type
   double m_area;
   double m_perimeter;
   // these are all temporary data which are recalculated on reload
   mutable bool m_selected;
   mutable float m_color;
   mutable int m_draworder;

   SalaShape(unsigned char type = 0)
   { m_type = type; m_draworder = -1; m_selected = false; m_area = 0.0; m_perimeter = 0.0; }
   SalaShape(const Point2f& point)
   { m_type = SHAPE_POINT; m_draworder = -1; m_selected = false; m_region = Line(point,point); m_centroid = point; m_area = 0.0; m_perimeter = 0.0; }
   SalaShape(const Line& line)
   { m_type = SHAPE_LINE; m_draworder = -1; m_selected = false; m_region = line; m_centroid = m_region.getCentre(); m_area = 0.0; m_perimeter = m_region.length(); }
   //
   bool isOpen() const
   { return (m_type & SHAPE_CLOSED) == 0; }
   bool isClosed() const
   { return (m_type & SHAPE_CLOSED) == SHAPE_CLOSED; }
   bool isPoint() const
   { return (m_type == SHAPE_POINT); }
   bool isLine() const
   { return (m_type == SHAPE_LINE); }
   bool isPolyLine() const
   { return (m_type & (SHAPE_POLY | SHAPE_CLOSED)) == SHAPE_POLY; }
   bool isPolygon() const
   { return (m_type & (SHAPE_POLY | SHAPE_CLOSED)) == (SHAPE_POLY | SHAPE_CLOSED); }
//   bool isCCW() const
//   { return (m_type & SHAPE_CCW) == SHAPE_CCW; }
//   //
   const Point2f& getPoint() const
   { return m_centroid; }
   const Line& getLine() const
   { return m_region; }
//   const QtRegion& getBoundingBox() const
//   { return m_region; }
//   //
//   double getArea() const
//   { return m_area; }
//   double getPerimeter() const
//   { return m_perimeter; }
//   // duplicate function, but easier to understand naming convention
   double getLength() const
   { return m_perimeter; }
//   //
   void setCentroidAreaPerim();
//   void setCentroid(const Point2f& p);
//   // duplicate function, but easier to understand naming convention
//   const Point2f& getCentroid() const
//   { return m_centroid; }
//   //
//   double getAngDev() const;
//   //
//   pqvector<SalaEdgeU> getClippingSet(QtRegion& clipframe) const;
//   //
   bool read(std::ifstream& stream, int version);
   bool write(std::ostream& stream);
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class SalaObject : public pvecint
{
   friend class ShapeMap;
protected:
   Point2f m_centroid;
public:
   SalaObject() {;}
   //
   bool read(std::ifstream& stream, int version);
   bool write(std::ostream& stream);
};
inline bool SalaObject::read(std::ifstream& stream, int)
{
   stream.read((char *)&m_centroid,sizeof(m_centroid));
   pvecint::read(stream);
   return true;
}
inline bool SalaObject::write(std::ostream& stream)
{
   stream.write((char *)&m_centroid,sizeof(m_centroid));
   pvecint::write(stream);
   return true;
}
struct SalaEvent
{
   enum { SALA_NULL_EVENT, SALA_CREATED, SALA_DELETED, SALA_MOVED };
   int m_action;
   int m_shape_ref;
   SalaShape m_geometry;
   SalaEvent(int action = SALA_NULL_EVENT, int shape_ref = -1) { m_action = action; m_shape_ref = shape_ref; }
};

struct ShapeRef
{
   enum {SHAPE_REF_NULL = 0xFFFFFFFF};
   enum {SHAPE_L = 0x01, SHAPE_B = 0x02, SHAPE_R = 0x04, SHAPE_T = 0x08 };
   enum {SHAPE_EDGE = 0x0f, SHAPE_INTERNAL_EDGE = 0x10, SHAPE_CENTRE = 0x20, SHAPE_OPEN = 0x40 };
   unsigned char m_tags;
   unsigned int m_shape_ref;
   psubvec<short> m_polyrefs;
   ShapeRef( unsigned int sref = SHAPE_REF_NULL, unsigned char tags = 0x00 )
      { m_shape_ref = sref; m_tags = tags; }
   friend bool operator == (const ShapeRef& a, const ShapeRef& b);
   friend bool operator != (const ShapeRef& a, const ShapeRef& b);
   friend bool operator < (const ShapeRef& a, const ShapeRef& b);
   friend bool operator > (const ShapeRef& a, const ShapeRef& b);
};
inline bool operator == (const ShapeRef& a, const ShapeRef& b)
{ return a.m_shape_ref == b.m_shape_ref; }
inline bool operator != (const ShapeRef& a, const ShapeRef& b)
{ return a.m_shape_ref != b.m_shape_ref; }
inline bool operator < (const ShapeRef& a, const ShapeRef& b)
{ return a.m_shape_ref < b.m_shape_ref; }
inline bool operator > (const ShapeRef& a, const ShapeRef& b)
{ return a.m_shape_ref > b.m_shape_ref; }

class ShapeMap : public PixelBase
{
public:
    ShapeMap(const std::string& name = std::string(),int type = EMPTYMAP);
    virtual ~ShapeMap();

    enum {EMPTYMAP = 0x0000, SEGMENTMAP = 0x0040, AXIALMAP = 0x0020, ALLLINEMAP = 0x0010, DRAWINGMAP = 0x0001, DATAMAP = 0x0002};
    int m_map_type;
    std::string m_name;
    bool m_show;              // used when shape map is a drawing layer
    bool m_editable;
    bool m_selection;
    mutable BSPNode *m_bsp_root;
    mutable bool m_bsp_tree;
    // quick grab for shapes
    pqvector<ShapeRef> **m_pixel_shapes;    // i rows of j columns
    // for screen drawing
    mutable int *m_display_shapes;
    pqmap<int,SalaShape> m_shapes;
    pqmap<int,SalaObject> m_objects;   // THIS IS UNUSED! Meant for each object to have many shapes
    prefvec<SalaEvent> m_undobuffer;
    AttributeTable m_attributes;
    // for graph functionality
    // Note: this list is stored PACKED for optimal performance on graph analysis
    // ALWAYS check it is in the same order as the shape list and attribute table
    prefvec<Connector> m_connectors;
    int m_shape_ref;
    pqvector<OrderedIntPair> m_links;
    pqvector<OrderedIntPair> m_unlinks;
    double m_tolerance;
    int m_obj_ref;
    mutable bool m_invalidate;
    mutable int m_displayed_attribute;
    MapInfoData *m_mapinfodata;
    std::set<int> m_selection_set;   // note: uses rowids not keys
    mutable bool m_show_lines;
    mutable bool m_show_fill;
    mutable bool m_show_centroids;
    bool m_hasgraph;
    mutable bool m_newshape;   // if a new shape has been added

    void makePolyPixels(int shaperef);
    // required for PixelBase, have to implement your own version of pixelate
    PixelRef pixelate( const Point2f& p, bool constrain = true, int = 1) const;
    const std::string& getName() const
       { return m_name; }
    bool read( std::ifstream& stream, int version, bool drawinglayer = false );
    bool write( std::ostream& stream, int version );
    void invalidateDisplayedAttribute()
       { m_invalidate = true; }
    void setDisplayedAttribute( int col ) const;
    void shapePixelBorder(pmap<int,int>& relations, int shaperef, int side, PixelRef currpix, PixelRef minpix, bool first);
    int moveDir(int side);
    // convert a selected pixels to a layer object (note, uses selection attribute on pixel, you must select to make this work):
    int makeShapeFromPointSet(const PointMap& pointmap);
    AttributeTable& getAttributeTable()
       { return m_attributes; }
    // use set displayed attribute instead unless you are deliberately changing the column order:
    void overrideDisplayedAttribute(int attribute)
    { m_displayed_attribute = attribute; }
    const prefvec<Connector>& getConnections() const
    { return m_connectors; }
    bool isSegmentMap() const
    { return m_map_type == SEGMENTMAP; }
    void pointPixelBorder(const PointMap& pointmap, pmap<int,int>& relations, SalaShape& shape, int side, PixelRef currpix, PixelRef minpix, bool first);
    bool clearSel();
    void init(int size, const QtRegion& r);
    Point2f pointOffset(const PointMap& pointmap, int currpix, int side);
    int makeLineShape(const Line& line, bool through_ui = false, bool tempshape = false);
    bool isAxialMap() const
    { return m_map_type == ALLLINEMAP || m_map_type == AXIALMAP; }
    // Connect a particular shape into the graph
    int connectIntersected(int rowid, bool linegraph);
    // Get the connections for a particular line
    int getLineConnections(int lineref, pvecint& connections, double tolerance);
    // Get arbitrary shape connections for a particular shape
    int getShapeConnections(int polyref, pvecint& connections, double tolerance);
    // retrieve lists of polys point intersects:
    void pointInPolyList(const Point2f& p, pvecint& shapeindexlist) const;
    void lineInPolyList(const Line& li, pvecint& shapeindexlist, int lineref = -1, double tolerance = 0.0) const;
    void polyInPolyList(int polyref, pvecint& shapeindexlist, double tolerance = 0.0) const;
    // helper to make actual test of point in shape:
    int testPointInPoly(const Point2f& p, const ShapeRef& shape) const;
};

// Quick mod - TV
template <class T>
class ShapeMaps : public /*protected*/ prefvec<T>
{
public:
   size_t m_displayed_map;
   ShapeMaps() { m_displayed_map = paftl::npos;}
   virtual ~ShapeMaps() {;}
   //
   size_t addMap(const std::string& name, int type);
   void setDisplayedMapRef(size_t map);
   // Quick mod - TV
   T& getMap(size_t index)
   { return prefvec<T>::at(index); }
   size_t getMapRef(const std::string& name) const;
   bool read( std::ifstream& stream, int version );
   bool write(::std::ostream &stream, int version, bool displayedmaponly = false );
};
template <class T>
void ShapeMaps<T>::setDisplayedMapRef(size_t map)
{
   if (m_displayed_map != paftl::npos && m_displayed_map != map)
      prefvec<T>::at(m_displayed_map).clearSel();
   m_displayed_map = map;
}
template <class T>
size_t ShapeMaps<T>::addMap(const std::string& name, int type)
{
   ShapeMaps<T>::push_back(T(name,type));
   setDisplayedMapRef(pmemvec<T*>::size()-1);
   return (pmemvec<T*>::size()-1);
}
template <class T>
bool ShapeMaps<T>::read( std::ifstream& stream, int version )
{
    prefvec<T>::clear(); // empty existing data
   // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
   unsigned int displayed_map;
   stream.read((char *)&displayed_map,sizeof(displayed_map));
   m_displayed_map = size_t(displayed_map);
   // read maps
   // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
   unsigned int count = 0;
   stream.read((char *) &count, sizeof(count));
   if (version < VERSION_NO_SHAPEMAP_NAME_LOOKUP) {
      for (size_t i = 0; i < size_t(count); i++) {
         // dummy name lookup (now simply creates on fly, as the name lookup may be corrupted in earlier versions)
         std::string name = dXstring440::readString(stream);
         int number;
         stream.read((char *)&number,sizeof(number));
      }
   }
   for (size_t j = 0; j < size_t(count); j++) {
      ShapeMaps<T>::push_back(T());
      prefvec<T>::tail().read(stream,version);
   }
   return true;
}
template <class T>
bool ShapeMaps<T>::write( ::std::ostream& stream, int version, bool displayedmaponly )
{
   if (!displayedmaponly) {
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int displayed_map = (unsigned int)(m_displayed_map);
      stream.write((char *)&displayed_map,sizeof(displayed_map));
      // write maps
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int count = (unsigned int) pmemvec<T*>::size();
      stream.write((char *) &count, sizeof(count));
      for (size_t j = 0; j < count; j++) {
         prefvec<T>::at(j).write(stream,version);
      }
   }
   else {
      unsigned int dummy;
      // displayed map is 0
      dummy = 0;
      stream.write((char *)&dummy,sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *)&dummy,sizeof(dummy));
      // write map:
      prefvec<T>::at(m_displayed_map).write(stream,version);
   }
   return true;
}
template <class T>
size_t ShapeMaps<T>::getMapRef(const std::string& name) const
{
   // note, only finds first map with this name
   for (size_t i = 0; i < pmemvec<T*>::size(); i++) {
      if (prefvec<T>::at(i).getName() == name)
         return i;
   }
   return -1;
}

}
