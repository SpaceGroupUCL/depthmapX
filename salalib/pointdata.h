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

#include "salalib/spacepixfile.h"
#include "genlib/exceptions.h"
#include "salalib/point.h"
#include "salalib/options.h"
#include "salalib/attributetable.h"
#include <vector>
#include <set>
#include <deque>

class MetaGraph;
class PointMap;
class PafAgent;
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
class Isovist;

struct PixelVec;



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
public:
   bool m_hasIsovistAnalysis = false;
   const std::vector<SpacePixelFile>& getDrawingFiles() { return *m_drawingFiles; }
protected:
   std::string m_name;
   const QtRegion* m_parentRegion;
   const std::vector<SpacePixelFile>* m_drawingFiles;
   depthmapX::ColumnMatrix<Point> m_points;    // will contain the graph reference when created
   int m_filled_point_count;
   double m_spacing;
   Point2f m_offset;
   Point2f m_bottom_left;
   bool m_initialised;
   bool m_blockedlines;
   bool m_processed;
   bool m_boundarygraph;
   int m_undocounter;
   std::vector<PixelRefPair> m_merge_lines;
   // The attributes table replaces AttrHeader / AttrRow data format
   AttributeTable m_attributes;
public:
   PointMap(const QtRegion& parentRegion, const std::vector<SpacePixelFile>& drawingFiles, const std::string& name = std::string("VGA Map"));
   const std::string& getName() const
   { return m_name; }

   void communicate( time_t& atime, Communicator *comm, int record );
   // constrain is constrain to existing rows / cols
   PixelRef pixelate( const Point2f& p, bool constrain = true, int scalefactor = 1 ) const;
   Point2f depixelate( const PixelRef& p, double scalefactor = 1.0 ) const;   // Inlined below
   QtRegion regionate( const PixelRef& p, double border ) const;     // Inlined below
   bool setGrid(double spacing, const Point2f& offset = Point2f());
   std::vector<std::pair<PixelRef, PixelRef>> getMergedPixelPairs()
   {
       // unnecessary converter until the m_merge_lines variable is
       // replaced with a std container
       std::vector<std::pair<PixelRef, PixelRef>> mergedPixelPairs;
       for (size_t i = 0; i < m_merge_lines.size(); i++) {
           mergedPixelPairs.push_back(std::make_pair(m_merge_lines[i].a, m_merge_lines[i].b));
       }
       return mergedPixelPairs;
   }
   //
   bool isProcessed() const
   { return m_processed; }
   void fillLine(const Line& li);
   bool blockLines();
   void blockLine(const Line& li);
   void unblockLines(bool clearblockedflag = true);
   bool fillPoint(const Point2f& p, bool add = true); // use add = false for remove point
   //bool blockPoint(const Point2f& p, bool add = true); // no longer used
   //
   bool makePoints(const Point2f& seed, int fill_type, Communicator *comm = nullptr); // Point2f non-reference deliberate
   bool clearPoints();  // Clear *selected* points
   bool undoPoints();
   bool canUndo() const
      { return !m_processed && m_undocounter != 0; }
   void outputPoints(std::ostream& stream, char delim );
   void outputMergeLines(std::ostream& stream, char delim);
   int  tagState(bool settag, bool sparkgraph = false);
   bool sparkGraph2(Communicator *comm, bool boundarygraph, double maxdist );
   bool sparkPixel2(PixelRef curs, int make, double maxdist = -1.0);
   bool sieve2(sparkSieve2& sieve, std::vector<PixelRef>& addlist, int q, int depth, PixelRef curs);
   // bool makeGraph( Graph& graph, int optimization_level = 0, Communicator *comm = NULL);
   //
   bool binDisplay(Communicator *comm);
   bool mergePoints(const Point2f& p);
   bool unmergePoints();
   bool mergePixels(PixelRef a, PixelRef b);
   void mergeFromShapeMap(const ShapeMap& shapemap);
   bool isPixelMerged(const PixelRef &a);

   void outputSummary(std::ostream& myout, char delimiter = '\t');
   void outputMif(std::ostream& miffile, std::ostream& midfile );
   void outputNet(std::ostream& netfile );
   void outputConnections(std::ostream& myout);
   void outputBinSummaries(std::ostream& myout);

   const Point& getPoint(const PixelRef& p) const
      { return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x)); }
   Point& getPoint(const PixelRef& p)
      { return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x)); }
   depthmapX::BaseMatrix<Point>& getPoints() { return m_points; }
   const int& pointState( const PixelRef& p ) const
      { return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x)).m_state; }
   // to be phased out
   bool blockedAdjacent( const PixelRef p ) const;
   //
   int getFilledPointCount() const
      { return m_filled_point_count; }
   //
   void requireIsovistAnalysis()
   {
       if(!m_hasIsovistAnalysis) {
           throw depthmapX::PointMapException(depthmapX::PointMapExceptionType::NO_ISOVIST_ANALYSIS, "Current pointmap does not contain isovist analysis");
       }
   }
protected:
   int expand( const PixelRef p1, const PixelRef p2, PixelRefVector& list, int filltype );
   //
   //void walk( PixelRef& start, int steps, Graph& graph,
   //           int parity, int dominant_axis, const int grad_pair[] );

   // Selection functionality
protected:
   enum { NO_SELECTION = 0, SINGLE_SELECTION = 1, COMPOUND_SELECTION = 2, LAYER_SELECTION = 4, OVERRIDE_SELECTION = 8 };
   int m_selection;
   bool m_pinned_selection;
   std::set<int> m_selection_set;      // n.b., m_selection_set stored as int for compatibility with other map layers
   mutable PixelRef s_bl;
   mutable PixelRef s_tr;
public:
   bool isSelected() const                              // does a selection exist
      { return m_selection != NO_SELECTION; }
   bool clearSel(); // clear the current selection
   bool setCurSel( QtRegion& r, bool add = false ); // set current selection
   bool setCurSel(const std::vector<int> &selset, bool add = false );
   // Note: passed by ref, use with care in multi-threaded app
   std::set<int>& getSelSet()
      { return m_selection_set; }
   const std::set<int>& getSelSet() const
      { return m_selection_set; }
   //
   PixelRefVector getLayerPixels(int layer);

   // Attribute functionality
protected:
   // which attribute is currently displayed:
   mutable int m_displayed_attribute;
public:
   int addAttribute(const std::string& name)
      { return m_attributes.insertColumn(name); }
   void removeAttribute(int col)
      { m_attributes.removeColumn(col); }
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
   PafColor getPointColor(PixelRef pixelRef) const;
   PafColor getCurrentPointColor() const;
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
   bool read(std::istream &stream, int version );
   bool write(std::ofstream& stream, int version );
   void addGridConnections(); // adds grid connections where graph does not include them
   void outputConnectionsAsCSV(std::ostream &myout, std::string delim = ",");
   void outputLinksAsCSV(std::ostream &myout, std::string delim = ",");
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
