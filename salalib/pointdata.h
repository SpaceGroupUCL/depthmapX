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


#ifndef __POINTDATA_H__
#define __POINTDATA_H__

#include "genlib/exceptions.h"
#include "vertex.h"
#include <vector>

class MetaGraph;
class PointMap;
class PafAgent;
class DataLayers; // deprecated, but still required for data conversion
class ShapeMap;

class OldPoint1 {
   friend class PointMap;
protected:
   int m_noderef;
   int m_state; 
};

class OldPoint2 {
   friend class PointMap;
protected:
   int m_noderef;
   int m_state;
   int m_misc;
};

class Bin;
class Node;
class Isovist;

struct PixelVec;

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
   pqmap<int,Line> m_lines; 
   // and when dynamic lines are being used, the process flag tells you which q octants to reprocess:
   //
   // Deprecated, kept for compatibility with previous versions:
   AttrBody *m_attributes;  // deprecated: now PointMap has an attribute table to handle this
   pmap<int,int> m_data_objects; // deprecated: (first int is data layer -- presumably the KEY not the index, second int is object ref)
   //
   int m_processflag;
public:
   Point()
      { m_state = EMPTY; m_block = 0; m_misc = 0; m_grid_connections = 0; m_node = NULL; m_attributes = NULL; m_processflag = 0; m_merge = NoPixel; m_user_data = NULL; }
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
   int getDataObject( int layer ) { 
      size_t var = m_data_objects.searchindex( layer );
      if (var != paftl::npos) 
         return m_data_objects.at(var);
      return -1;  // note: not paftl::npos
   }
   // note -- set merge pixel should be done only through merge pixels
   PixelRef getMergePixel() {
      return m_merge;
   }
   Node& getNode()
      { return *m_node; }
   AttrBody& getAttributes()
      { return *m_attributes; }
   void setAttributes(const AttrBody& attr)
      { if (m_attributes) delete m_attributes;
        m_attributes = new AttrBody(attr); }
   char getGridConnections() const
      { return m_grid_connections; }
   float getBinDistance(int i);
public:
   ifstream& read(ifstream& stream, int version, int attr_count);
   ofstream& write(ofstream& stream, int version);
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

class sparkSieve2;

namespace depthmapX
{
    enum PointMapExceptionType{NO_ISOVIST_ANALYSIS};
    class PointMapException: public depthmapX::RuntimeException {
    private:
        PointMapExceptionType m_errorType;
    public:
        PointMapException(PointMapExceptionType errorType, std::string message) : depthmapX::RuntimeException(message)
        {
            m_errorType = errorType;
        }
        PointMapExceptionType getErrorType() const {
            return m_errorType;
        }
    };
}

class PointMap : public PixelBase
{
   friend class PointMaps;
   friend class MapInfoData; // <- for mapinfo export
   // MetaGraph is a friend for two functions:
   // convertAttributes: this really should not be at the metagraph level!  Fix!
   // pushValuesToLayer: this swaps values from a PointMap to a DataLayer, and it needs to be changed in the future
   // (e.g., when making DataLayers into ShapeMaps)
   friend class MetaGraph;
private:
   bool m_hasIsovistAnalysis = false;
protected:
   std::string m_name;
   Point **m_points;    // will contain the graph reference when created
   //int m_rows;
   //int m_cols;
   int m_point_count;
   double m_spacing;
   Point2f m_offset;
   Point2f m_bottom_left;
   SuperSpacePixel *m_spacepix;
   bool m_initialised;
   bool m_blockedlines;
   bool m_processed;
   bool m_boundarygraph;
   int m_undocounter;
   pqvector<PixelRefPair> m_merge_lines;
   // The attributes table replaces AttrHeader / AttrRow data format
   AttributeTable m_attributes;
public:
   PointMap(const std::string& name = std::string("VGA Map"));
   PointMap(const PointMap& pointdata);
   PointMap& operator = (const PointMap& pointdata);
   void construct( const PointMap& pointdata );
   virtual ~PointMap();
   const std::string& getName() const
   { return m_name; }
   //
   // Quick mod - TV
#if defined(_WIN32)
   void communicate( __time64_t& atime, Communicator *comm, int record );
#else
   void communicate( time_t& atime, Communicator *comm, int record );
#endif
   // constrain is constrain to existing rows / cols
   PixelRef pixelate( const Point2f& p, bool constrain = true, int scalefactor = 1 ) const;
   Point2f depixelate( const PixelRef& p, double scalefactor = 1.0 ) const;   // Inlined below 
   QtRegion regionate( const PixelRef& p, double border ) const;     // Inlined below
   bool setSpacePixel(const SuperSpacePixel *spacepix);  // (so different threads can use it... dangermouse!)
   bool setGrid(double spacing, const Point2f& offset = Point2f());
   //
   bool isProcessed() const
   { return m_processed; }
   bool isBoundaryGraph() const
   { return m_boundarygraph; }
   //
   bool fillLines();
   void fillLine(const Line& li);
   bool blockLines();
   void blockLine(int key, const Line& li);
   void unblockLines(bool clearblockedflag = true);
   void addLineDynamic(LineKey ref,const Line& line);
   void removeLineDynamic(LineKey ref,const Line& line);
   bool fillPoint(const Point2f& p, bool add = true); // use add = false for remove point
   //bool blockPoint(const Point2f& p, bool add = true); // no longer used
   //
   bool makePoints(const Point2f& seed, int fill_type, Communicator *comm = NULL); // Point2f non-reference deliberate
   bool clearPoints();  // Clear *selected* points
   bool undoPoints();
   bool canUndo() const
      { return !m_processed && m_undocounter != 0; }
   //
   bool importPoints(istream& stream);
   void outputPoints( ostream& stream, char delim );
   void outputMergeLines(ostream& stream, char delim);
   //
   void makeConstants();
   int  tagState(bool settag, bool sparkgraph = false);
   bool binMap( Communicator *comm );
   bool sparkGraph( Communicator *comm );
   bool sparkGraph2( Communicator *comm, bool boundarygraph, double maxdist );
   bool dynamicSparkGraph2();
   bool sparkPixel2(PixelRef curs, int make, double maxdist = -1.0);
   bool sieve2(sparkSieve2& sieve, pvector<PixelRef>& addlist, int q, int depth, PixelRef curs);
   // bool makeGraph( Graph& graph, int optimization_level = 0, Communicator *comm = NULL);
   //
   bool binDisplay(Communicator *comm);
   bool analyseIsovist(Communicator *comm, MetaGraph& mgraph, bool simple_version);
   bool analyseVisual(Communicator *comm, Options& options, bool simple_version);
   bool analyseVisualPointDepth(Communicator *comm);
   bool analyseMetric(Communicator *comm, Options& options);
   bool analyseMetricPointDepth(Communicator *comm);
   bool analyseAngular(Communicator *comm, Options& options);
   bool analyseAngularPointDepth(Communicator *comm);
   bool analyseThruVision(Communicator *comm);
   bool mergePoints(const Point2f& p);
   bool unmergePoints();
   bool mergePixels(PixelRef a, PixelRef b);
   void mergeFromShapeMap(const ShapeMap& shapemap);
   bool isPixelMerged(const PixelRef &a);
   //
   void outputSummary(ostream& myout, char delimiter = '\t');
   void outputMif( ostream& miffile, ostream& midfile );
   void outputNet( ostream& netfile );
   void outputConnections(ostream& myout);
   void outputBinSummaries(ostream& myout);
   //
   // scruffy little helper functions
   int u(int i, int x, int s) const
      { return i + (x * s); }
   int v(int j, int y, int s) const
      { return j + (y * s); }
   int remaining(int i, int j, int x, int y, int q);
   //
   Point& getPoint(const PixelRef& p) const
      { return m_points[p.x][p.y]; }
   const int& pointState( const PixelRef& p ) const
      { return m_points[p.x][p.y].m_state; }
   const int pointObject( const PixelRef& p, int layer_ref ) const
      { size_t layer_index = m_points[p.x][p.y].m_data_objects.searchindex(layer_ref);
        if (layer_index != paftl::npos)
           return m_points[p.x][p.y].m_data_objects.at(layer_index);
        return -1; }
   // to be phased out
   bool blockedAdjacent( const PixelRef p ) const;
   //
   int getPointCount() const
      { return m_point_count; }
   //
   void requireIsovistAnalysis()
   {
       if(!m_hasIsovistAnalysis) {
           throw depthmapX::PointMapException(depthmapX::PointMapExceptionType::NO_ISOVIST_ANALYSIS, "Current pointmap does not contain isovist analysis");
       }
   }
protected:
   int expand( const PixelRef p1, const PixelRef p2, PixelRefList& list, int filltype );
   //
   //void walk( PixelRef& start, int steps, Graph& graph, 
   //           int parity, int dominant_axis, const int grad_pair[] );

   // Selection functionality
protected:
   enum { NO_SELECTION = 0, SINGLE_SELECTION = 1, COMPOUND_SELECTION = 2, LAYER_SELECTION = 4, OVERRIDE_SELECTION = 8 };
   int m_selection;
   bool m_pinned_selection;
   pvecint m_selection_set;      // n.b., m_selection_set stored as int for compatibility with other map layers
   mutable PixelRef s_bl; 
   mutable PixelRef s_tr;
public:
   bool isSelected() const                              // does a selection exist
      { return m_selection != NO_SELECTION; }
   bool isPinned() const
      { return m_pinned_selection; }
   bool clearSel(); // clear the current selection
   bool setCurSel( QtRegion& r, bool add = false ); // set current selection
   bool setCurSel( const pvecint& selset, bool add = false );
   bool overrideSelPixel(PixelRef pix);    // set a pixel to selected: careful!
   //bool togglePin();
   //bool convertSelToDataObject( MetaGraph& meta_graph );
   // Note: passed by ref, use with care in multi-threaded app
   pvecint& getSelSet()
      { return m_selection_set; }
   const pvecint& getSelSet() const
      { return m_selection_set; }
   //
   PixelRefList getLayerPixels(int layer);
   PixelRefList getDataObjectPixels(int layer, int object);
   //
   // Attribute functionality
protected:
   // which attribute is currently displayed:
   mutable int m_displayed_attribute;
public:
   int addAttribute(const std::string& name)
      { return m_attributes.insertColumn(name); }
   void removeAttribute(int col)
      { m_attributes.removeColumn(col); }
   void setAttribute(PixelRef pix, const std::string& name, float val)
      { m_attributes.setValue(m_attributes.getRowid(pix),name,val); }
   void incrementAttribute(PixelRef pix, const std::string& name)
      { m_attributes.incrValue(m_attributes.getRowid(pix),name); }
   // I don't want to do this, but every so often you will need to update this table 
   // use const version by preference
   AttributeTable& getAttributeTable()
      { return m_attributes; }
   const AttributeTable& getAttributeTable() const
      { return m_attributes; }
public:
   double getDisplayMinValue() const
   { return (m_displayed_attribute != -1) ? m_attributes.getMinValue(m_displayed_attribute) : 0; } 

   // Quick mod - TV
#if defined(_WIN32)
   double getDisplayMaxValue() const
   { return (m_displayed_attribute != -1) ? m_attributes.getMaxValue(m_displayed_attribute) : pixelate(m_region.top_right); } 
#else
   double getDisplayMaxValue() const
   { return (m_displayed_attribute != -1) ? m_attributes.getMaxValue(m_displayed_attribute) : pixelate(m_region.top_right).x; }
#endif
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
public:
   void setDisplayedAttribute( int col );
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
   double getDisplayedAverage()
      { return m_attributes.getAvgValue( m_displayed_attribute ); }
   //
   double getLocationValue(const Point2f& point);
   //
   // Screen functionality
public:
   enum {VIEW_ATTRIBUTES, VIEW_MERGED, VIEW_LAYERS, VIEW_AGENTS};
protected:
   int m_viewing_deprecated;
   int m_draw_step;
   mutable bool m_finished;
   mutable PixelRef bl;
   mutable PixelRef cur;   // cursor for points
   mutable PixelRef rc;    // cursor for grid lines
   mutable PixelRef prc;   // cursor for point lines
   mutable PixelRef tr;
   mutable int curmergeline;
   mutable QtRegion m_sel_bounds;
public:
   void setScreenPixel( double m_unit );
   void makeViewportPoints( const QtRegion& viewport ) const;
   bool findNextPoint() const;
   Point2f getNextPointLocation() const
   { return getPoint(cur).m_location; }
   Point& getNextPoint() const
   { return getPoint(cur); }
   bool findNextRow() const;
   Line getNextRow() const;
   bool findNextPointRow() const;
   Line getNextPointRow() const;
   bool findNextCol() const;
   Line getNextCol() const;
   bool findNextPointCol() const;
   Line getNextPointCol() const;
   bool findNextMergeLine() const;
   Line getNextMergeLine() const;
   bool getPointSelected() const;
   PafColor getPointColor() const;
   int getSelCount()
      { return (int) m_selection_set.size(); }
   const QtRegion& getSelBounds() const
      { return m_sel_bounds; }
   //
   double getSpacing() const
   { return m_spacing; }
   //
public:
   // this is an odd helper function, value in range 0 to 1
   PixelRef pickPixel(double value) const;
public:
   bool read( ifstream& stream, int version );
   bool write( ofstream& stream, int version );
   void convertAttributes( int which_attributes );
   void addGridConnections(); // adds grid connections where graph does not include them
   void outputConnectionsAsCSV(ostream &myout, std::string delim = ",");
};

// inlined to make thread safe

inline Point2f PointMap::depixelate( const PixelRef& p, double scalefactor ) const
{
   return Point2f( m_bottom_left.x + m_spacing * scalefactor * double(p.x), 
                   m_bottom_left.y + m_spacing * scalefactor * double(p.y) );
}

inline QtRegion PointMap::regionate( const PixelRef& p, double border ) const
{
   return QtRegion(
         Point2f( m_bottom_left.x + m_spacing * (double(p.x) - 0.5 - border),
                  m_bottom_left.y + m_spacing * (double(p.y) - 0.5 - border)),
         Point2f( m_bottom_left.x + m_spacing * (double(p.x) + 0.5 + border),
                  m_bottom_left.y + m_spacing * (double(p.y) + 0.5 + border))
      );
}

/////////////////////////////////////////////////////////////////////////////////////

class PointMaps
{
protected:
   int m_displayed_map;
   SuperSpacePixel *m_spacepix;
public:
   std::vector<PointMap> maps_vector;
   PointMaps() { m_displayed_map = -1; m_spacepix = NULL; }
   virtual ~PointMaps() {;}
   //
   void setDisplayedPointMapRef(int i) 
   { m_displayed_map = i; }
   PointMap& getDisplayedPointMap()
   { return maps_vector.at(m_displayed_map); }
   const PointMap& getDisplayedPointMap() const
   { return maps_vector.at(m_displayed_map); }
   int getDisplayedPointMapRef() const
   { return m_displayed_map; }
   int addNewMap(const std::string& name = std::string("VGA Map"));
   void removeMap(int i) 
   { if (m_displayed_map >= i) m_displayed_map--; maps_vector.erase(maps_vector.begin() + i); }
   //
   void setSpacePixel(SuperSpacePixel *spacepix)
   { m_spacepix = spacepix; for (size_t i = 0; i < maps_vector.size(); i++) maps_vector.at(i).setSpacePixel(spacepix); }
   void redoBlockLines()   // (flags blockedlines, but also flags that you need to rebuild a bsp tree if you have one)
   { for (size_t i = 0; i < maps_vector.size(); i++) { maps_vector.at(i).m_blockedlines = false; } }
   //
   bool read( ifstream& stream, int version );
   bool write( ofstream& stream, int version, bool displayedmaponly = false );
};

/////////////////////////////////////////////////////////////////////////////////////

// A helper class for metric integration

// to allow a dist / PixelRef pair for easy sorting
// (have to do comparison operation on both dist and PixelRef as 
// otherwise would have a duplicate key for pqmap / pqvector)

struct MetricTriple
{
   float dist;
   PixelRef pixel;
   PixelRef lastpixel;
   MetricTriple(float d = 0.0f, PixelRef p = NoPixel, PixelRef lp = NoPixel)
   {
      dist = d; pixel = p; lastpixel = lp;
   }
   friend bool operator == (const MetricTriple& mp1, const MetricTriple& mp2);
   friend bool operator < (const MetricTriple& mp1, const MetricTriple& mp2);
   friend bool operator > (const MetricTriple& mp1, const MetricTriple& mp2);
   friend bool operator != (const MetricTriple& mp1, const MetricTriple& mp2);
};

inline bool operator == (const MetricTriple& mp1, const MetricTriple& mp2)
{ return (mp1.dist == mp2.dist && mp1.pixel == mp2.pixel); }
inline bool operator < (const MetricTriple& mp1, const MetricTriple& mp2)
{ return (mp1.dist < mp2.dist) || (mp1.dist == mp2.dist && mp1.pixel < mp2.pixel); }
inline bool operator > (const MetricTriple& mp1, const MetricTriple& mp2)
{ return (mp1.dist > mp2.dist) || (mp1.dist == mp2.dist && mp1.pixel > mp2.pixel); }
inline bool operator != (const MetricTriple& mp1, const MetricTriple& mp2)
{ return (mp1.dist != mp2.dist) || (mp1.pixel != mp2.pixel); }

// Note: angular triple simply based on metric triple

struct AngularTriple
{
   float angle;
   PixelRef pixel;
   PixelRef lastpixel;
   AngularTriple(float a = 0.0f, PixelRef p = NoPixel, PixelRef lp = NoPixel)
   {
      angle = a; pixel = p; lastpixel = lp;
   }
   friend bool operator == (const AngularTriple& mp1, const AngularTriple& mp2);
   friend bool operator < (const AngularTriple& mp1, const AngularTriple& mp2);
   friend bool operator > (const AngularTriple& mp1, const AngularTriple& mp2);
   friend bool operator != (const AngularTriple& mp1, const AngularTriple& mp2);
};

inline bool operator == (const AngularTriple& mp1, const AngularTriple& mp2)
{ return (mp1.angle == mp2.angle && mp1.pixel == mp2.pixel); }
inline bool operator < (const AngularTriple& mp1, const AngularTriple& mp2)
{ return (mp1.angle < mp2.angle) || (mp1.angle == mp2.angle && mp1.pixel < mp2.pixel); }
inline bool operator > (const AngularTriple& mp1, const AngularTriple& mp2)
{ return (mp1.angle > mp2.angle) || (mp1.angle == mp2.angle && mp1.pixel > mp2.pixel); }
inline bool operator != (const AngularTriple& mp1, const AngularTriple& mp2)
{ return (mp1.angle != mp2.angle) || (mp1.pixel != mp2.pixel); }

//

// A scruffy little helper class for the original makeGraph

struct Grad {
   int a;
   int b;
   float length;
   float ratio;
   Grad(int u = -1, int v = -1) 
   { 
      a = u; b = v; 
      length = (float) sqrt(double(u * u) + double(v * v)); 
      ratio = float(v) / float(u);  // v is sometimes 0, thus v/u
   }
   int x(int dir) const {
      int w;
      if (dir / 4 == 0)
         w = a;
      else
         w = b;
      if (dir % 2 == 0)
         w *= -1;
      return w;
   }
   int y(int dir) const {
      int w;
      if (dir / 4 == 0)
         w = b;
      else
         w = a;
      if ((dir / 2) % 2 == 0)
         w *= -1;
      return w;
   }
            // q quadrants:
            //
            //      \ 6 | 7 /
            //      0 \ | / 1
            //      - -   - -
            //      2 / | \ 3
            //      / 4 | 5 \
            
   int whichbin(int dir) const {
      int w;
      if (ratio == 0.0f) {                      // =  0 degrees (special case) 
         switch (dir) {
         case 0:
            return 16;
         case 1:
            return 0;
         case 4:
            return 24;
         case 6:
            return 8;
         }
      }
      if (ratio < 0.2679491924311227f) {        // < 15 degrees
         w = 1;
      }
      else if (ratio < 0.5773502691896257f) {   // < 30 degrees
         w = 2;
      }
      else if (ratio < 1.0f) {                  // < 45 degrees
         w = 3;
      }
      else {                                    // = 45 degrees (special case)
         switch (dir) {
         case 0:
            return 20;
         case 1:
            return 28;
         case 2:
            return 12;
         case 3:
            return 4;
         }
      }
      switch (dir) {
      case 0:
         w = 16 + w;
         break;
      case 1:
         w = 32 - w;
         break;
      case 2:
         w = 16 - w;
         break;
      case 3:
         w = 0 + w;
         break;
      case 4:
         w = 24 - w;
         break;
      case 5:
         w = 24 + w;
         break;
      case 6:
         w = 8 + w;
         break;
      case 7:
         w = 8 - w;
         break;
      }
      return w;
   }
};


// true grads are also similar to generated grads...
// this scruffy helper function converts a true grad to a bin:

// (now corrected as of 2.1008r!)

inline int whichbin( const Point2f& grad )
{
   int bin = 0;
   double ratio;


   // This is only for true gradients...
   //    ...see below for calculated gradients
   //    
   // Octant:
   //       +     -
   //    - \ 8 | 8 / +
   //      16\ | / 0
   //      ---- ----
   //      16/ | \32
   //    + /24 | 24\ -
   //      -      +    

   if (fabs(grad.y) > fabs(grad.x)) {
      bin = 1; // temporary: label y priority
   }

   if (bin == 0) {
      ratio = fabs(grad.y) / fabs(grad.x);

      // now actual bin number
      if (grad.x > 0.0) {
         if (grad.y >= 0.0) {
            bin = 0;
         }
         else {
            bin = -32;
         }
      }
      else {
         if (grad.y >= 0.0) {
            bin = -16;
         }
         else {
            bin = 16;
         }
      }
   }
   else {
      ratio = fabs(grad.x) / fabs(grad.y);

      // now actual bin number
      if (grad.y > 0.0) {
         if (grad.x >= 0.0) {
            bin = -8;
         }
         else {
            bin = 8;
         }
      }
      else {
         if (grad.x >= 0.0) {
            bin = 24;
         }
         else {
            bin = -24;
         }
      }
   }

   if (ratio < 1e-12) {
      // nop
   }
   else if (ratio < 0.2679491924311227) {   // < 15 degrees
      bin += 1;
   }
   else if (ratio < 0.5773502691896257) {   // < 30 degrees
      bin += 2;
   }
   else if (ratio < 1.0 - 1e-12) {          // < 45 degrees
      bin += 3;
   }
   else {
      bin += 4;
   }
   /*
   // True angular bins
   if (ratio <      0.0984914033571642) {   // 1/64
      // nop
   }
   else if (ratio < 0.3033466836073423) {   // 3/64
      bin += 1;
   }
   else if (ratio < 0.5345111359507916) {   // 5/64
      bin += 2;
   }
   else if (ratio < 0.8206787908286603) {   // 7/64
      bin += 3;
   }
   else {
      bin += 4;
   }
   */
   if (bin < 0) {
      bin = -bin;
   }
   // this is necessary:
   bin = bin % 32;

   return bin;
}

/////////////////////////////////

// Another helper to write down the q-octant from any bin, in shifted format
// note that sieve2 has been used to get the precise required q-octant for the bin

inline int processoctant(int bin)
{
   int q = -1;
   switch (bin) {
   case 0: case 1: case 2: case 3: case 4:
      q = 1; break;
   case 5: case 6: case 7:
      q = 7; break;
   case 8: case 9: case 10: case 11:
      q = 6; break;
   case 12: case 13: case 14: case 15: case 16:
      q = 0; break;
   case 17: case 18: case 19: case 20:
      q = 2; break;
   case 21: case 22: case 23:
      q = 4; break;
   case 24: case 25: case 26: case 27:
      q = 5; break;
   case 28: case 29: case 30: case 31:
      q = 3; break;
   }

   return (1 << q);
}

// ...but in order to determine what *needs* processing, we need this octant:

inline int flagoctant(int bin)
{
   int q = 0;
         
   // have to use two q octants if you are on diagonals or axes...
   switch (bin) {
   case 0: 
      q |= 1 << 1; q |= 1 << 3; break;
   case 1: case 2: case 3:
      q |= 1 << 1; break;
   case 4:
      q |= 1 << 1; q |= 1 << 7; break;
   case 5: case 6: case 7:
      q |= 1 << 7; break;
   case 8: 
      q |= 1 << 7; q |= 1 << 6; break;
   case 9: case 10: case 11:
      q = 1 << 6; break;
   case 12: 
      q |= 1 << 6; q |= 1 << 0; break;
   case 13: case 14: case 15:
      q |= 1 << 0; break;
   case 16:
      q |= 1 << 0; q |= 1 << 2; break;
   case 17: case 18: case 19:
      q |= 1 << 2; break;
   case 20:
      q |= 1 << 2; q |= 1 << 4; break;
   case 21: case 22: case 23:
      q |= 1 << 4; break;
   case 24: 
      q |= 1 << 4; q |= 1 << 5; break;
   case 25: case 26: case 27:
      q |= 1 << 5; break;
   case 28: 
      q |= 1 << 5; q |= 1 << 3; break;
   case 29: case 30: case 31:
      q |= 1 << 3; break;
   }

   return q;
}


// Another helper, this time to write down the q-octant for the bin opposing you

inline int q_opposite(int bin)
{
   int q = -1;
   int opposing_bin = (16 + bin) % 32;

            //      \ 6 | 7 /
            //      0 \ | / 1
            //      - -   - -
            //      2 / | \ 3
            //      / 4 | 5 \
         
   return flagoctant(opposing_bin);
}

#endif
