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

#ifndef __SHAPEMAP_H__
#define __SHAPEMAP_H__

#include "genlib/p2dpoly.h"
#include "genlib/stringutils.h"
#include <vector>
#include <string>
#include "salalib/importtypedefs.h"
#include "genlib/bsptree.h"
#include "genlib/containerutils.h"
#include "salalib/MapInfoData.h"
#include <set>

/////////////////////////////////////////////////////////////////////////////////////////////////

#include <map>
// each pixel has various lists of information:

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

/////////////////////////////////////////////////////////////////////////////////////////////////

// this is a helper for cutting polygons to fit a viewport / cropping frame
struct SalaEdgeU : public EdgeU
{
   int index;
   bool entry; // or exit
   SalaEdgeU() : EdgeU() 
   { index = -1; entry = false; }
   SalaEdgeU(int i, bool e, const EdgeU& eu) : EdgeU(eu)
   { index = i; entry = e; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class PointMap;

class SalaShape
{
public:
   std::vector<Point2f> m_points;
   enum {SHAPE_POINT = 0x01, SHAPE_LINE = 0x02, SHAPE_POLY = 0x04, SHAPE_CIRCLE = 0x08, SHAPE_TYPE = 0x0f, SHAPE_CLOSED = 0x40, SHAPE_CCW = 0x80 };
   friend class ShapeMap;
protected:
   unsigned char m_type;
   Point2f m_centroid; // centre of mass, but also used as for point if object is a point
   Line m_region; // bounding box, but also used as a line if object is a line, hence type
   double m_area;
   double m_perimeter;
   // these are all temporary data which are recalculated on reload
   mutable bool m_selected;
   mutable float m_color;
   mutable int m_draworder;
public:
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
   bool isCCW() const
   { return (m_type & SHAPE_CCW) == SHAPE_CCW; }
   //
   const Point2f& getPoint() const
   { return m_centroid; }
   const Line& getLine() const
   { return m_region; }
   const QtRegion& getBoundingBox() const
   { return m_region; }
   //
   double getArea() const
   { return m_area; }
   double getPerimeter() const
   { return m_perimeter; }
   // duplicate function, but easier to understand naming convention
   double getLength() const
   { return m_perimeter; }
   //
   void setCentroidAreaPerim();
   void setCentroid(const Point2f& p);
   // duplicate function, but easier to understand naming convention
   const Point2f& getCentroid() const
   { return m_centroid; }  
   //
   double getAngDev() const;
   //
   pqvector<SalaEdgeU> getClippingSet(QtRegion& clipframe) const;
   //
   bool read(std::istream &stream, int version);
   bool write(std::ofstream& stream);

   std::vector<Line> getAsLines() const {
       std::vector<Line> lines;
       if (isLine()) {
          lines.push_back(getLine());
       }
       else if (isPolyLine()) {
          for (size_t j = 0; j < m_points.size() - 1; j++) {
             lines.push_back(Line(m_points[j], m_points[j+1]));
          }
       }
       return lines;
   }
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
   bool read(std::istream &stream, int version);
   bool write(std::ofstream& stream);
};
inline bool SalaObject::read(std::istream& stream, int)
{
   stream.read((char *)&m_centroid,sizeof(m_centroid));
   pvecint::read(stream);
   return true;
}
inline bool SalaObject::write(std::ofstream& stream)
{
   stream.write((char *)&m_centroid,sizeof(m_centroid));
   pvecint::write(stream);
   return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

struct Connector;

/////////////////////////////////////////////////////////////////////////////////////////////////

struct SalaEvent
{
   enum { SALA_NULL_EVENT, SALA_CREATED, SALA_DELETED, SALA_MOVED };
   int m_action;
   int m_shape_ref;
   SalaShape m_geometry;
   SalaEvent(int action = SALA_NULL_EVENT, int shape_ref = -1) { m_action = action; m_shape_ref = shape_ref; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// Quick mod - TV
class MapInfoData;

class ShapeMap : public PixelBase
{
   friend class AxialMaps;
   friend class MapInfoData;
public:
   // now shapemaps cover a multitude of different types, record here:
   // (note, allline maps are automatically generated and have extra information recorded for line reduction)
   // Do not change numeric values!  They are saved to file.
   // Note the Pesh map does auto-overlap of shape-shape (yet...), so can be used for an arbitrary shape map
   enum { EMPTYMAP = 0x0000, DRAWINGMAP = 0x0001, DATAMAP = 0x0002, POINTMAP = 0x0004, CONVEXMAP = 0x0008, 
          ALLLINEMAP = 0x0010, AXIALMAP = 0x0020, SEGMENTMAP = 0x0040, PESHMAP = 0x0080, LINEMAP = 0x0070 };
   enum { COPY_NAME = 0x0001, COPY_GEOMETRY = 0x0002, COPY_ATTRIBUTES = 0x0004, COPY_GRAPH = 0x0008, COPY_ALL = 0x000f };
protected:
   std::string m_name;
   int m_map_type;
   bool m_hasgraph;
   // counters
   int m_obj_ref;
   mutable bool m_newshape;   // if a new shape has been added
   //
   // quick grab for shapes
   std::vector<std::vector<ShapeRef> > m_pixel_shapes;    // i rows of j columns
   //
   // allow quick closest line test (note only works for a given layer, with many layers will be tricky)
   mutable BSPNode *m_bsp_root;
   mutable bool m_bsp_tree;
   //
   std::map<int,SalaShape> m_shapes;
   std::map<int,SalaObject> m_objects;   // THIS IS UNUSED! Meant for each object to have many shapes
   //
   std::vector<SalaEvent> m_undobuffer;
   //
   AttributeTable m_attributes;
   //
   // for graph functionality
   // Note: this list is stored PACKED for optimal performance on graph analysis
   // ALWAYS check it is in the same order as the shape list and attribute table
   std::vector<Connector> m_connectors;
   //
   // for geometric operations
   double m_tolerance;
   // for screen drawing
   mutable std::vector<int> m_display_shapes;
   mutable int m_current;
   mutable bool m_invalidate;
   //
public:
   ShapeMap(const std::string& name = std::string(),int type = EMPTYMAP);
   virtual ~ShapeMap();
   void copy(const ShapeMap& shapemap, int copyflags = 0);
   void clearAll();
   //
   // num objects
   const size_t getObjectCount() const
   { return m_objects.size(); }
   // num shapes total
   const size_t getShapeCount() const
   { return m_shapes.size(); }
   // num shapes for this object (note, request by object rowid
   // -- on interrogation, this is what you will usually receive)
   const size_t getShapeCount(int rowid) const
   { return depthmapX::getMapAtIndex(m_shapes, rowid)->second.m_points.size(); }
   //
   int getIndex(int rowid) const
   { return depthmapX::getMapAtIndex(m_shapes, rowid)->first; }
   //
   // add shape tools
   void makePolyPixels(int shaperef);
   void shapePixelBorder(std::map<int,int>& relations, int shaperef, int side, PixelRef currpix, PixelRef minpix, bool first);
   // remove shape tools
   void removePolyPixels(int shaperef);
   //
   //
   void init(int size, const QtRegion& r);
   int getNextShapeKey();
   // convert a single point into a shape
   int makePointShapeWithRef(const Point2f& point, int shape_ref, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
   int makePointShape(const Point2f& point, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
   // or a single line into a shape
   int makeLineShapeWithRef(const Line& line, int shape_ref, bool through_ui = false, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
   int makeLineShape(const Line& line, bool through_ui = false, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
   // or a polygon into a shape
   int makePolyShapeWithRef(const std::vector<Point2f> &points, bool open, int shape_ref, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
   int makePolyShape(const std::vector<Point2f> &points, bool open, bool tempshape = false, const std::map<int,float> &extraAttributes = std::map<int, float>());
public:
   // or make a shape from a shape
   int makeShape(const SalaShape& shape, int override_shape_ref = -1, const std::map<int,float> &extraAttributes = std::map<int,float>());
   // convert points to polygons
   bool convertPointsToPolys(double poly_radius, bool selected_only);
   // convert a selected pixels to a layer object (note, uses selection attribute on pixel, you must select to make this work):
   int makeShapeFromPointSet(const PointMap& pointmap);
   //
   // move a shape (currently only a line shape) -- in the future should use SalaShape
   bool moveShape(int shaperef, const Line& line, bool undoing = false);
   // delete selected shapes
   bool removeSelected();
   // delete a shape
   void removeShape(int shaperef, bool undoing = false);
   //
   void setShapeAttributes(int rowid, const SalaShape& shape);
   //
   // some UI polygon creation tools:
   int polyBegin(const Line& line);
   bool polyAppend(int shape_ref, const Point2f& point);
   bool polyClose(int shape_ref);
   bool polyCancel(int shape_ref);
   // some shape creation tools for the scripting language or DLL interface
protected:
   pqvector<Point2f> m_temppoints;
public:
   bool canUndo() const
   { return m_undobuffer.size() != 0; }
   void undo();
   //
   // helpers:
   Point2f pointOffset(const PointMap& pointmap, int currpix, int side);
   int moveDir(int side);
   //
   void pointPixelBorder(const PointMap& pointmap, std::map<int, int> &relations, SalaShape& shape, int side, PixelRef currpix, PixelRef minpix, bool first);
   // quick find of topmost poly from a point (bit too inaccurate!)
   int quickPointInPoly(const Point2f& p) const;
   // slower point in topmost poly test:
   int pointInPoly(const Point2f& p) const;
   // test if point is inside a particular shape
   bool pointInPoly(const Point2f& p, int shaperef) const;
   // retrieve lists of polys point intersects:
   void pointInPolyList(const Point2f& p, pvecint& shapeindexlist) const;
   void lineInPolyList(const Line& li, pvecint& shapeindexlist, int lineref = -1, double tolerance = 0.0) const;
   void polyInPolyList(int polyref, pvecint& shapeindexlist, double tolerance = 0.0) const;
   void shapeInPolyList(const SalaShape& shape, pvecint& shapeindexlist);
   // helper to make actual test of point in shape:
   int testPointInPoly(const Point2f& p, const ShapeRef& shape) const;
   // also allow look for a close polyline:
   int getClosestOpenGeom(const Point2f& p) const;
   // this version uses a BSP tree to find closest line (currently only line shapes)
   int getClosestLine(const Point2f& p) const;
   // this version simply finds the closest vertex to the point
   Point2f getClosestVertex(const Point2f& p) const;
   // Find out which shapes a line cuts through:
   void getShapeCuts(const Line& li_orig, std::vector<ValuePair> &cuts);
   // Cut a line according to the first shape it cuts
   void cutLine(Line& li);//, short dir);
   // Connect a particular shape into the graph
   int connectIntersected(int rowid, bool linegraph);
   // Get the connections for a particular line
   int getLineConnections(int lineref, pvecint& connections, double tolerance);
   // Get arbitrary shape connections for a particular shape
   int getShapeConnections(int polyref, pvecint& connections, double tolerance);
   // Make all connections
   void makeShapeConnections();
   //
   bool makeBSPtree() const;
   //
   const std::vector<Connector>& getConnections() const
   { return m_connectors; }
   //
   bool isAllLineMap() const
   { return m_map_type == ALLLINEMAP; }
   bool isSegmentMap() const
   { return m_map_type == SEGMENTMAP; }
   bool isAxialMap() const 
   { return m_map_type == ALLLINEMAP || m_map_type == AXIALMAP; }
   bool isPeshMap() const 
   { return m_map_type == PESHMAP; }
   int getMapType() const
   { return m_map_type; }
   // Attribute functionality
protected:
   // which attribute is currently displayed:
   mutable int m_displayed_attribute;
public:
   const std::string& getName() const
      { return m_name; }
   int addAttribute(const std::string& name)
      { return m_attributes.insertColumn(name); }
   void removeAttribute(int col)
      { m_attributes.removeColumn(col); }
   void setAttribute(int obj, const std::string& name, float val)
      { m_attributes.setValue(m_attributes.getRowid(obj),name,val); }
   void incrementAttribute(int obj, const std::string& name)
      { m_attributes.incrValue(m_attributes.getRowid(obj),name); }
   // I don't want to do this, but every so often you will need to update this table 
   // use const version by preference
   AttributeTable& getAttributeTable()
      { return m_attributes; }
   const AttributeTable& getAttributeTable() const
      { return m_attributes; }
public:
   // layer functionality
   bool isLayerVisible(int layerid) const
   { return m_attributes.isLayerVisible(layerid); }
   void setLayerVisible(int layerid, bool show)
   { m_attributes.setLayerVisible(layerid,show); }
   bool selectionToLayer(const std::string& name = std::string("Unnamed"));
public:
   double getDisplayMinValue() const
   { return (m_displayed_attribute != -1) ? m_attributes.getMinValue(m_displayed_attribute) : 0; } 
   double getDisplayMaxValue() const
   { return (m_displayed_attribute != -1) ? m_attributes.getMaxValue(m_displayed_attribute) : m_shapes.rbegin()->first; }
   //
   mutable DisplayParams m_display_params;
   const DisplayParams& getDisplayParams() const
   { return m_attributes.getDisplayParams(m_displayed_attribute); } 
   // make a local copy of the display params for access speed:
   void setDisplayParams(const DisplayParams& dp, bool apply_to_all = false)
   { if (apply_to_all)
        m_attributes.setDisplayParams(dp); 
     else 
        m_attributes.setDisplayParams(m_displayed_attribute, dp); 
     m_display_params = dp; }
   //
   mutable bool m_show_lines;
   mutable bool m_show_fill;
   mutable bool m_show_centroids;
   void getPolygonDisplay(bool& show_lines, bool& show_fill, bool& show_centroids)
   { show_lines = m_show_lines; show_fill = m_show_fill; show_centroids = m_show_centroids; }
   void setPolygonDisplay(bool show_lines, bool show_fill, bool show_centroids)
   { m_show_lines = show_lines; m_show_fill = show_fill; m_show_centroids = show_centroids; }
   //
public:
   void setDisplayedAttribute( int col ) const;
   // use set displayed attribute instead unless you are deliberately changing the column order:
   void overrideDisplayedAttribute(int attribute)
   { m_displayed_attribute = attribute; }
   // now, there is a slightly odd thing here: the displayed attribute can go out of step with the underlying 
   // attribute data if there is a delete of an attribute in idepthmap.h, so it just needs checking before returning!
   int getDisplayedAttribute() const
   { if (m_displayed_attribute == m_attributes.m_display_column) return m_displayed_attribute;
     if (m_attributes.m_display_column != -2) {
         m_displayed_attribute = m_attributes.m_display_column;
         m_display_params = m_attributes.getDisplayParams(m_displayed_attribute);
      }
      return m_displayed_attribute; }
   //
   void invalidateDisplayedAttribute()
      { m_invalidate = true; }
   //
   double getDisplayedAverage()
      { return m_attributes.getAvgValue( m_displayed_attribute ); }
   //
protected:
   mutable bool m_show;              // used when shape map is a drawing layer
   bool m_editable;
   bool m_selection;
   std::set<int> m_selection_set;   // note: uses rowids not keys
public:
   // Selection
   bool isSelected() const
   { return m_selection; }
   bool setCurSel( QtRegion& r, bool add = false );
   bool setCurSel(const std::vector<int> &selset, bool add = false );
   bool setCurSelDirect( const std::vector<int>& selset, bool add = false );
   bool clearSel();
   std::set<int>& getSelSet()
   { return m_selection_set; }
   const std::set<int>& getSelSet() const
   { return m_selection_set; }
   size_t getSelCount()
   { return m_selection_set.size(); }
   QtRegion getSelBounds();
   // To showing
   bool isShown() const
   { return m_show; }
   void setShow(bool on = true) const
   { m_show = on; }
   // To all editing
   bool isEditable() const
   { return m_editable; }
   void setEditable(bool on = true) 
   { m_editable = on; }
protected:
   bool m_hasMapInfoData = false;
   MapInfoData m_mapinfodata;
public:
   bool hasMapInfoData() const {return m_hasMapInfoData;}
   int loadMifMap(std::istream& miffile, std::istream& midfile);
   bool outputMifMap(std::ostream& miffile, std::ostream& midfile);
   const MapInfoData& getMapInfoData() const
   { return m_mapinfodata; }
public:
   // Screen
   void makeViewportShapes( const QtRegion& viewport ) const;
   bool findNextShape(bool& nextlayer) const;
   const SalaShape& getNextShape() const;
   const PafColor getShapeColor() const
   { return m_attributes.getDisplayColor(m_display_shapes[m_current]); }
   bool getShapeSelected() const
   { return depthmapX::getMapAtIndex(m_shapes, m_display_shapes[m_current])->second.m_selected; }
   //
   double getLocationValue(const Point2f& point) const;

   // Quick mod - TV
#if !defined(_WIN32)
#define     __max(x,y)  ((x<y) ? y: x)
#define     __min(x,y)	((x<y) ? x: y)
#endif
   //
   double getSpacing()
   { return __max(m_region.width(), m_region.height()) / (10 * log((double)10+m_shapes.size())); }
   //
   // dangerous: accessor for the shapes themselves:
   const std::map<int,SalaShape>& getAllShapes() const
   { return m_shapes; }
   std::map<int,SalaShape>& getAllShapes()
   { return m_shapes; }
   // required for PixelBase, have to implement your own version of pixelate
   PixelRef pixelate( const Point2f& p, bool constrain = true, int = 1) const;
   //
public:
   // file
   bool read(std::istream &stream, int version, bool drawinglayer = false );
   bool write( std::ofstream& stream, int version );
   //
   bool output( std::ofstream& stream, char delimiter = '\t', bool updated_only = false );
   //
   // links and unlinks
protected:
   pqvector<OrderedIntPair> m_links;
   pqvector<OrderedIntPair> m_unlinks;
   mutable int m_curlinkline;
   mutable int m_curunlinkpoint;
public:
   bool clearLinks();
   bool linkShapes(const Point2f& p);
   bool linkShapes(int index1, int index2, bool refresh = true);
   bool linkShapes(int id1, int dir1, int id2, int dir2, float weight);
   bool unlinkShapes(const Point2f& p);
   bool unlinkShapes(int index1, int index2, bool refresh = true);
   bool unlinkShapeSet(std::istream& idset, int refcol);
public:
   // generic for all types of graphs
   bool findNextLinkLine() const;
   Line getNextLinkLine() const;
   std::vector<SimpleLine> getAllLinkLines();
   // specific to axial line graphs 
   bool findNextUnlinkPoint() const;
   Point2f getNextUnlinkPoint() const;
   std::vector<Point2f> getAllUnlinkPoints();
   void outputUnlinkPoints( std::ofstream& stream, char delim );
public:
   std::vector<SimpleLine> getAllShapesAsLines() const;
   std::vector<std::pair<SimpleLine, PafColor>> getAllLinesWithColour();
   std::map<std::vector<Point2f>, PafColor> getAllPolygonsWithColour();
   bool importLines(const std::vector<Line> &lines, const depthmapX::Table &data);
   bool importLinesWithRefs(const std::map<int, Line> &lines, const depthmapX::Table &data);
   bool importPoints(const std::vector<Point2f> &points, const depthmapX::Table &data);
   bool importPointsWithRefs(const std::map<int, Point2f> &points, const depthmapX::Table &data);
   bool importPolylines(const std::vector<depthmapX::Polyline> &lines, const depthmapX::Table &data);
   bool importPolylinesWithRefs(const std::map<int, depthmapX::Polyline> &lines, const depthmapX::Table &data);
private:
   bool importData(const depthmapX::Table &data, std::vector<int> shape_refs);
};

/////////////////////////////////////////////////////////////////////////////////////////////////

// Quick mod - TV
template <class T>
class ShapeMaps : public /*protected*/ prefvec<T>
{
protected:
   size_t m_displayed_map;
public:
   ShapeMaps() { m_displayed_map = paftl::npos;}
   virtual ~ShapeMaps() {;}
   //
   size_t addMap(const std::string& name, int type);
   void removeMap(size_t map);

   //
   bool hasShapeMap()
   { return pmemvec<T>::size() != 0; }
   //
   // Simple display options
   void setDisplayedMapRef(size_t map);
   //
   T& getDisplayedMap()
   { return prefvec<T>::at(m_displayed_map); }
   const T& getDisplayedMap() const
   { return prefvec<T>::at(m_displayed_map); }
   //
   size_t getDisplayedMapRef() const
   { return m_displayed_map; }
   // Getting shape maps by name and reference
   
   // Quick mod - TV
   T& getMap(size_t index)
   { return prefvec<T>::at(index); }
   const T& getMap(size_t index) const
   { return prefvec<T>::at(index); }
   const size_t getMapCount() 
   { return prefvec<T>::size(); }
   T& getLastMap()
   { return prefvec<T>::tail(); }
   const T& getLastMap() const
   { return prefvec<T>::tail(); }
   //
   size_t getMapRef(const std::string& name) const;
   //
   size_t getObjectCount() const
   { return prefvec<T>::at(m_displayed_map).getObjectCount(); }
   const size_t getShapeCount() const
   { return prefvec<T>::at(m_displayed_map).m_shapes.size(); }
   //
   bool read(std::istream &stream, int version );
   bool write( std::ofstream& stream, int version, bool displayedmaponly = false );
   //
   const QtRegion& getBoundingBox() const
   { return prefvec<T>::at(m_displayed_map).getRegion(); }
   //
   // making links and unlinks
   bool linkShapes(const Point2f& p)
   { return prefvec<T>::at(m_displayed_map).linkShapes(p); }
   bool unlinkShapes(const Point2f& p)
   { return prefvec<T>::at(m_displayed_map).unlinkShapes(p); }
   // for displaying links and unlinks
   bool findNextLinkLine() const
   { return prefvec<T>::at(m_displayed_map).findNextLinkLine(); }
   Line getNextLinkLine() const
   { return prefvec<T>::at(m_displayed_map).getNextLinkLine(); }
   // unlinks are actually only used for special case of axial lines:
   bool findNextUnlinkPoint() const
   { return prefvec<T>::at(m_displayed_map).findNextUnlinkPoint(); }
   Point2f getNextUnlinkPoint() const
   { return prefvec<T>::at(m_displayed_map).getNextUnlinkPoint(); }
};

template <class T>
size_t ShapeMaps<T>::addMap(const std::string& name, int type)
{
   ShapeMaps<T>::push_back(T(name,type));
   setDisplayedMapRef(pmemvec<T*>::size()-1);
   return (pmemvec<T*>::size()-1);
}
template <class T>
void ShapeMaps<T>::removeMap(size_t map)
{ 
   // note: with the shape map name look up in the past, this was corrupting the 
   // lookup!  Better just to go with a simple system:
// Quick mod - TV
   prefvec<T>::remove_at(map);
   if (m_displayed_map > map || m_displayed_map >= pmemvec<T*>::size())
      m_displayed_map--; 
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
template <class T>
void ShapeMaps<T>::setDisplayedMapRef(size_t map)
{
   if (m_displayed_map != paftl::npos && m_displayed_map != map)
      prefvec<T>::at(m_displayed_map).clearSel();
   m_displayed_map = map;
}
template <class T>
bool ShapeMaps<T>::read( std::istream& stream, int version )
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

   for (size_t j = 0; j < size_t(count); j++) {
      ShapeMaps<T>::push_back(T());
      prefvec<T>::tail().read(stream,version);
   }
   return true;
}
template <class T>
bool ShapeMaps<T>::write( std::ofstream& stream, int version, bool displayedmaponly )
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

/////////////////////////////////////////////////////////////////////////////////////////////////

#endif
