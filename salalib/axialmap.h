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


#ifndef __SHAPEGRAPH_H__
#define __SHAPEGRAPH_H__
#include "spacepix.h"
#include "connector.h"

struct AxialVertex;
struct AxialVertexKey;
struct RadialLine;
struct PolyConnector;

struct ValuePair;
struct ValueTriplet;

class AxialPolygons : public SpacePixel
{
   friend class ShapeGraphs;
protected:
   std::map<Point2f,pqvector<Point2f>> m_vertex_possibles;
   pvecint m_vertex_polys;
   pvecint **m_pixel_polys;
   pqvector<AxialVertex> m_handled_list;
public:
   AxialPolygons();
   virtual ~AxialPolygons();
   //
   void clear();
   void init(prefvec<Line>& lines, const QtRegion& region);
   void makeVertexPossibles(const prefvec<Line>& lines, const prefvec<Connector>& connectionset);
   void makePixelPolys();
   //
   AxialVertex makeVertex(const AxialVertexKey& vertexkey, const Point2f& openspace);
   // find a polygon corner visible from seed:
   AxialVertexKey seedVertex(const Point2f& seed);
   // make axial lines from corner vertices, visible from openspace
   void makeAxialLines(pqvector<AxialVertex>& openvertices, prefvec<Line>& lines, prefvec<pvecint>& keyvertices, prefvec<PolyConnector>& poly_connections, pqvector<RadialLine>& radial_lines);
   // extra: make all the polygons possible from the set of m_vertex_possibles
   void makePolygons(prefvec<pqvector<Point2f>>& polygons);
};

struct AxialVertexKey
{
   int m_ref_key;
   short m_ref_a;
   short m_ref_b;
   AxialVertexKey(int ref = -1, short a = -1, short b = -1)
   { m_ref_key = ref; m_ref_a = a; m_ref_b = b; }
   friend bool operator == (const AxialVertexKey& a, const AxialVertexKey& b);
   friend bool operator != (const AxialVertexKey& a, const AxialVertexKey& b);
   friend bool operator > (const AxialVertexKey& a, const AxialVertexKey& b);
   friend bool operator < (const AxialVertexKey& a, const AxialVertexKey& b);
};
inline bool operator == (const AxialVertexKey& a, const AxialVertexKey& b)
{ return (a.m_ref_key == b.m_ref_key && a.m_ref_a == b.m_ref_a && a.m_ref_b == b.m_ref_b); }
inline bool operator != (const AxialVertexKey& a, const AxialVertexKey& b)
{ return (a.m_ref_key != b.m_ref_key || a.m_ref_a != b.m_ref_a || a.m_ref_b != b.m_ref_b); }
inline bool operator > (const AxialVertexKey& a, const AxialVertexKey& b)
{ return (a.m_ref_key > b.m_ref_key || (a.m_ref_key == b.m_ref_key && (a.m_ref_a > b.m_ref_a || (a.m_ref_a == b.m_ref_a && a.m_ref_b > b.m_ref_b)))); }
inline bool operator < (const AxialVertexKey& a, const AxialVertexKey& b)
{ return (a.m_ref_key < b.m_ref_key || (a.m_ref_key == b.m_ref_key && (a.m_ref_a < b.m_ref_a || (a.m_ref_a == b.m_ref_a && a.m_ref_b < b.m_ref_b)))); }

const AxialVertexKey NoVertex(-1,-1,-1);

struct AxialVertex : public AxialVertexKey
{
   Point2f m_point;
   Point2f m_openspace;
   Point2f m_a;
   Point2f m_b;
   bool m_clockwise;
   bool m_convex;
   bool m_initialised;
   bool m_axial;
   AxialVertex(const AxialVertexKey& vertex_key = NoVertex, const Point2f& point = Point2f(), const Point2f& openspace = Point2f()) : AxialVertexKey(vertex_key)
   { m_point = point; m_openspace = openspace; m_initialised = false; m_axial = false; } 
};

struct RadialKey {
   AxialVertexKey vertex;
   float ang;
   bool segend;
   // padding the remaining three bytes behind the bool - don't use int : 24 as this will grab the next 4 byte block
   char pad1 : 8;
   short pad2 : 16;

   RadialKey(const AxialVertexKey& v = NoVertex, float a = -1.0f, bool se = false) : pad1(0), pad2(0)
   { vertex = v; ang = a; segend = se; }
   RadialKey(const RadialKey& rk) : pad1(0), pad2(0)
   { vertex = rk.vertex; ang = rk.ang; segend = rk.segend; }
   friend bool operator < (const RadialKey& a, const RadialKey& b);
   friend bool operator > (const RadialKey& a, const RadialKey& b);
   friend bool operator == (const RadialKey& a, const RadialKey& b);
};
inline bool operator < (const RadialKey& a, const RadialKey& b)
{ return a.vertex < b.vertex || (a.vertex == b.vertex && (a.ang < b.ang || (a.ang == b.ang && a.segend < b.segend))); }
inline bool operator > (const RadialKey& a, const RadialKey& b)
{ return a.vertex > b.vertex || (a.vertex == b.vertex && (a.ang > b.ang || (a.ang == b.ang && a.segend > b.segend))); }
inline bool operator == (const RadialKey& a, const RadialKey& b)
{ return a.vertex == b.vertex && a.ang == b.ang && a.segend == b.segend; }

struct RadialLine : public RadialKey
{
   Point2f openspace;
   Point2f keyvertex;
   Point2f nextvertex;
   RadialLine(const RadialKey& rk = RadialKey()) : RadialKey(rk) {;}
   RadialLine(const AxialVertexKey& v, bool se, const Point2f& o, const Point2f& k, const Point2f& n)
   { vertex = v; ang = (float) angle(o,k,n); segend = se; openspace = o; keyvertex = k; nextvertex = n; }
   RadialLine(const RadialLine& rl) : RadialKey(rl)
   { openspace = rl.openspace; keyvertex = rl.keyvertex; nextvertex = rl.nextvertex; }
   bool cuts(const Line& l) const;
};

typedef pqvector<RadialKey> RadialKeyList;

struct RadialSegment : public pvecint
{
   RadialKey radial_b;

   // Quick mod - TV
#if defined(_WIN32)
   RadialSegment(RadialKey& rb = RadialKey()) : pvecint()
   { radial_b = rb; }
#else
   RadialSegment(RadialKey& rb /*= RadialKey()*/) : pvecint()
   { radial_b = rb; }
#endif

   // Quick mod - TV
   RadialSegment() : pvecint()
   {
       radial_b = RadialKey();
   }

};

struct PolyConnector {
   Line line;
   RadialKey key;
   PolyConnector(const Line& l = Line(), const RadialKey& k = RadialKey())
   { line = l; key = k; }
};

// used during angular analysis
struct AnalysisInfo
{
   // lists used for multiple radius analysis
   bool leaf;
   bool choicecovered;
   SegmentRef previous;
   int depth;
   double choice;            
   double weighted_choice;
   double weighted_choice2; //EFEF
   AnalysisInfo() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; choice = 0.0; weighted_choice = 0.0; weighted_choice2 = 0.0; 
   }
   void clearLine() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; // choice values are cummulative and not cleared
   }
};

class MapInfoData;

class ShapeGraph : public ShapeMap
{
   friend class ShapeGraphs;
   friend class AxialMinimiser;
   friend class MapInfoData;
protected:
   prefvec<pvecint> m_keyvertices;       // but still need to return keyvertices here
   int m_keyvertexcount;
protected:
public:
   bool outputMifPolygons(ostream& miffile, ostream& midfile) const;
   void outputNet(ostream& netfile) const;
public:
   ShapeGraph(const std::string& name = "<axial map>", int type = ShapeMap::AXIALMAP);
   virtual ~ShapeGraph() {;}
   void makeConnections(const prefvec<pvecint>& keyvertices = prefvec<pvecint>());
   //void initAttributes();
   void makeDivisions(const prefvec<PolyConnector>& polyconnections, const pqvector<RadialLine>& radiallines, std::map<RadialKey, pvecint> &radialdivisions, std::map<int,pvecint>& axialdividers, Communicator *comm);
   bool integrate(Communicator *comm = NULL, const pvecint& radius = pvecint(), bool choice = false, bool local = false, bool fulloutput = false, int weighting_col = -1, bool simple_version = true);
   bool stepdepth(Communicator *comm = NULL);
   bool analyseAngular(Communicator *comm, const pvecdouble& radius);
   // extra parameters for selection_only and interactive are for parallel process extensions
   int analyseTulip(Communicator *comm, int tulip_bins, bool choice, int radius_type, const pvecdouble& radius, int weighting_col, int weighting_col2 = -1, int routeweight_col = -1, bool selection_only = false, bool interactive = true);
   bool angularstepdepth(Communicator *comm);
   // the two topomet analyses can be found in topomet.cpp:
   bool analyseTopoMet(Communicator *comm, int analysis_type, double radius, bool sel_only);
   bool analyseTopoMetPD(Communicator *comm, int analysis_type);
   // lineset and connectionset are filled in by segment map
   void makeNewSegMap();
   void makeSegmentMap(prefvec<Line>& lineset, prefvec<Connector>& connectionset, double stubremoval);
   void initSegmentAttributes(prefvec<Connector>& connectionset);
   void makeSegmentConnections(prefvec<Connector>& connectionset);
   void pushAxialValues(ShapeGraph& axialmap);
   //
   virtual bool read( istream& stream, int version );
   bool readold( istream& stream, int version );
   virtual bool write( ofstream& stream, int version );
   void writeAxialConnectionsAsDotGraph(ostream &stream);
   void writeAxialConnectionsAsPairsCSV(ostream &stream);
   void writeSegmentConnectionsAsPairsCSV(ostream &stream);
   //
   void unlinkFromShapeMap(const ShapeMap& shapemap);
};

class ShapeGraphs : public ShapeMaps<ShapeGraph>
{
protected:
   // helpful to know this for creating fewest line maps, although has to be reread at input
   int m_all_line_map;
   //
   // For all line map work:
   AxialPolygons m_polygons;
   prefvec<PolyConnector> m_poly_connections;
   pqvector<RadialLine> m_radial_lines;
public:
   ShapeGraphs() { m_all_line_map = -1; }
   virtual ~ShapeGraphs() {;}
   //
   // ShapeGraphs just have extra functionality over ShapeMaps here:
   bool makeAllLineMap(Communicator *comm, SuperSpacePixel& superspacepix, const Point2f& seed);
   bool makeFewestLineMap(Communicator *comm, bool replace_existing);
   int convertDrawingToAxial(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix);
   int convertDataToAxial(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
   int convertDrawingToConvex(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix);
   int convertDataToConvex(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
   int convertDrawingToSegment(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix);
   int convertDataToSegment(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata = false);
   int convertAxialToSegment(Communicator *comm, const std::string& name, bool keeporiginal = true, bool pushvalues = false, double stubremoval = 0.0);
   //
   bool hasAllLineMap()
   { return m_all_line_map != -1; }
   //
   bool read(istream &stream, int version );
   bool readold( istream& stream, int version );
   bool write( ofstream& stream, int version, bool displayedmaponly = false );
};

// helpers... a class to tidy up ugly maps people may give me...

class TidyLines : public SpacePixel
{
public:
   TidyLines() {;}
   virtual ~TidyLines() {;}
   void tidy(prefvec<Line>& lines, const QtRegion& region);  
   void quicktidy(std::map<int, Line> &lines, const QtRegion& region);
};

// helpers... a class to reduce all line maps to fewest line maps

class AxialMinimiser
{
protected:
   ShapeGraph *m_alllinemap;
   //
   ValueTriplet *m_vps;
   bool *m_removed;
   bool *m_affected;
   bool *m_vital;
   int *m_radialsegcounts;
   int *m_keyvertexcounts;
   prefvec<Connector> m_axialconns; // <- uses a copy of axial lines as it will remove connections
public:
   AxialMinimiser(const ShapeGraph& alllinemap, std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs);
   ~AxialMinimiser();
   void removeSubsets(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts);
   void fewestLongest(std::map<int,pvecint>& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts);
   // advanced topological testing:
   bool checkVital(int checkindex,pvecint& axsegcuts, std::map<RadialKey,RadialSegment>& radialsegs, std::map<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines);
   //
   bool removed(int i) const
   { return m_removed[i]; }
};

struct ValueTriplet
{
   int value1;
   float value2;
   int index;
};

// note: these are unordered, but in 'a' takes priority over 'b'
inline bool operator == (const IntPair& x, const IntPair& y)
{
   return (x.a == y.a && x.b == y.b);
}
inline bool operator != (const IntPair& x, const IntPair& y)
{
   return (x.a != y.a || x.b != y.b);
}
inline bool operator < (const IntPair& x, const IntPair& y)
{
   return ( (x.a == y.a) ? x.b < y.b : x.a < y.a );
}
inline bool operator > (const IntPair& x, const IntPair& y)
{
   return ( (x.a == y.a) ? x.b > y.b : x.a > y.a );
}

// note: these are made with a is always less than b
inline bool operator == (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return (x.a == y.a && x.b == y.b);
}
inline bool operator != (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return (x.a != y.a || x.b != y.b);
}
inline bool operator < (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return ( (x.a == y.a) ? x.b < y.b : x.a < y.a );
}
inline bool operator > (const OrderedIntPair& x, const OrderedIntPair& y)
{
   return ( (x.a == y.a) ? x.b > y.b : x.a > y.a );
}

#endif
