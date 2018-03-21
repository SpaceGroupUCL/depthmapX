#pragma once

#include "mgraph440/spacepix.h"
#include "mgraph440/shapemap.h"
#include "mgraph440/connector.h"

namespace mgraph440 {

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
struct PolyConnector {
   Line line;
   RadialKey key;
   PolyConnector(const Line& l = Line(), const RadialKey& k = RadialKey())
   { line = l; key = k; }
};
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

class AxialPolygons : public SpacePixel
{
   friend class ShapeGraphs;
public:
   pqmap<Point2f,pqvector<Point2f>> m_vertex_possibles;
   pvecint m_vertex_polys;
   pvecint **m_pixel_polys;
   pqvector<AxialVertex> m_handled_list;
   AxialPolygons();
   virtual ~AxialPolygons();

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

class ShapeGraph : public ShapeMap
{
   friend class ShapeGraphs;
   friend class AxialMinimiser;
   friend class MapInfoData;
public:
   ShapeGraph(const std::string& name = "<axial map>", int type = ShapeMap::AXIALMAP);
   virtual ~ShapeGraph() {;}

   prefvec<pvecint> m_keyvertices;       // but still need to return keyvertices here
   int m_keyvertexcount;
   bool outputMifPolygons(ostream& miffile, ostream& midfile) const;
   void outputNet(ostream& netfile) const;
   virtual bool read( ifstream& stream, int version );
   bool readold( ifstream& stream, int version );
   virtual bool write(ostream &stream, int version );
};

class ShapeGraphs : public ShapeMaps<ShapeGraph>
{
public:
   // helpful to know this for creating fewest line maps, although has to be reread at input
   int m_all_line_map;
   // For all line map work:
   AxialPolygons m_polygons;
   prefvec<PolyConnector> m_poly_connections;
   pqvector<RadialLine> m_radial_lines;
   ShapeGraphs() { m_all_line_map = -1; }
   virtual ~ShapeGraphs() {;}
   bool read( ifstream& stream, int version );
   bool readold( ifstream& stream, int version );
   bool write( ostream& stream, int version, bool displayedmaponly = false );
};

}
