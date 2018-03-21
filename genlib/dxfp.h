// genlib - a component of the depthmapX - spatial network analysis platform
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

// DXF parser header file

#ifndef __DXFP_H__
#define __DXFP_H__

///////////////////////////////////////////////////////////////////////////////

// DXF parser reads in DXF files
// So far very simple:
// The parser reads in vertices, lines and polylines, and stores them in the
// defined layers.  It also reads in any line types defined.

#include <map>

class DxfToken;

class DxfTableRow;
class DxfEntity;

class DxfVertex;
class DxfLine;
class DxfPolyLine;
class DxfArc;
class DxfCircle;
class DxfSpline;

class DxfInsert;

class DxfLineType;
class DxfLayer;
class DxfBlock;

class DxfParser;

#include <fstream>

using namespace std;

const double DXF_PI = 3.1415926535897932384626433832795;

///////////////////////////////////////////////////////////////////////////////

// Tokens read from file

class DxfToken {
public:
   int code;
   int size;
   std::string data;
   //
   DxfToken();
   friend istream& operator >> (istream& stream, DxfToken& token);
};

///////////////////////////////////////////////////////////////////////////////

// Table (row) base classes

class DxfTableRow
{
   friend class DxfParser;
protected:
   std::string m_name;
public:
   DxfTableRow( const std::string& name = "" );
   const std::string& getName() const
      { return m_name; }
   virtual ~DxfTableRow(){}
protected:
   virtual bool parse( const DxfToken& token, DxfParser *Parser );
public:
   // for hash table storage
   friend bool operator > (const DxfTableRow&, const DxfTableRow& );
   friend bool operator < (const DxfTableRow&, const DxfTableRow& );
   friend bool operator == (const DxfTableRow&, const DxfTableRow& );
};

// Entity base class

class DxfEntity
{
   friend class DxfParser;
protected:
   // Reference data
   int m_tag;
   DxfLineType *m_p_line_type;
   DxfLayer *m_p_layer;
public:
   DxfEntity( int tag = -1 );
   void clear();  // for reuse when parsing
   virtual ~DxfEntity(){}
protected:
   virtual bool parse( const DxfToken& token, DxfParser *parser );
};

// Three very simple 'entities'...

// Vertex

class DxfVertex : public DxfEntity
{
   friend class DxfParser;
   friend class DxfLine;
   friend class DxfPolyLine;
   friend class DxfLwPolyLine;
public:
   double x;
   double y;
   double z;
public:
   DxfVertex( int tag = -1 );
   void clear();  // for reuse when parsing
   // some simple manipulation
   // note, all ops are 2d...
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { x = (x - base_vertex.x) * scale.x + base_vertex.x;
     y = (y - base_vertex.y) * scale.y + base_vertex.y; }
   // note, rotation is 2d op, angle in degrees, ccw
   void rotate(const DxfVertex& base_vertex, double angle)
   { DxfVertex reg; double ang = (2.0 * DXF_PI * angle / 360.0);
     reg.x = (x - base_vertex.x) * cos(ang) - (y - base_vertex.y) * sin(ang);
     reg.y = (y - base_vertex.y) * cos(ang) + (x - base_vertex.x) * sin(ang);
     x = reg.x + base_vertex.x; y = reg.y + base_vertex.y; }
   void translate(const DxfVertex& translation)
   { x += translation.x;
     y += translation.y; }
   //
   friend bool operator == (const DxfVertex& a, const DxfVertex& b);
   friend bool operator != (const DxfVertex& a, const DxfVertex& b);
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

// Helper: a bounding box region (only 2D at present)

class DxfRegion {
protected:
   bool m_first;
   DxfVertex m_min;
   DxfVertex m_max;
public:
   DxfRegion()
      { m_first = true; }
   void add(const DxfVertex& v)
   {
      if (m_first) {
         m_min = v; m_max = v;
         m_first = false;
      }
      if (v.x < m_min.x)
         m_min.x = v.x;
      if (v.x > m_max.x)
         m_max.x = v.x;
      if (v.y < m_min.y)
         m_min.y = v.y;
      if (v.y > m_max.y)
         m_max.y = v.y;
   }
   void merge(const DxfVertex& point)
      { add(point); }
   void merge(const DxfRegion& region)
      { add(region.m_min); add(region.m_max); }
   const DxfVertex& getExtMin() const
      { return m_min; }
   const DxfVertex& getExtMax() const
      { return m_max; }
   void clear()
      { m_first = true; }
   bool empty() const
      { return m_first; }
   //
   // some simple manipulations
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { m_min.scale(base_vertex, scale); m_max.scale(base_vertex, scale); }
   // rotate tricky...
   void rotate(const DxfVertex& base_vertex, double angle)
   { ; }
   void translate(const DxfVertex& translation)
   { m_min.translate(translation); m_max.translate(translation); }
};

///////////////////////////////////////////////////////////////////////////////

// Line

class DxfLine : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
protected:
   DxfVertex m_start;
   DxfVertex m_end;
public:
   DxfLine( int tag = -1 );
   void clear();  // for reuse when parsing
   //
   DxfVertex& getStart() const;
   DxfVertex& getEnd() const;
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { m_start.scale(base_vertex, scale); m_end.scale(base_vertex, scale);
     DxfRegion::scale(base_vertex, scale); }
   void rotate(const DxfVertex& base_vertex, double angle)
   { m_start.rotate(base_vertex, angle); m_end.rotate(base_vertex, angle);
     DxfRegion::rotate(base_vertex, angle); }
   void translate(const DxfVertex& translation)
   { m_start.translate(translation); m_end.translate(translation);
     DxfRegion::translate(translation); }
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

// PolyLine

class DxfPolyLine : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
public:
   enum { CLOSED = 1 }; // CLOSED = closed polygon
protected:
   int m_attributes;
   int m_vertex_count;
   std::vector<DxfVertex> m_vertices;
public:
   DxfPolyLine( int tag = -1 );
   void clear();  // for reuse when parsing
   //
   int numVertices() const;
   const DxfVertex& getVertex(int i) const;
   int getAttributes() const;
   const DxfRegion& getBoundingBox();
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { for (int i = 0; i < m_vertex_count; i++)
        m_vertices[i].scale(base_vertex, scale);
     DxfRegion::scale(base_vertex, scale); }
   void rotate(const DxfVertex& base_vertex, double angle)
   { for (int i = 0; i < m_vertex_count; i++)
        m_vertices[i].rotate(base_vertex, angle);
     DxfRegion::rotate(base_vertex, angle); }
   void translate(const DxfVertex& translation)
   { for (int i = 0; i < m_vertex_count; i++)
        m_vertices[i].translate(translation);
     DxfRegion::translate(translation); }
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

// LwPolyLine --- just inherit from DxfPolyLine

class DxfLwPolyLine : public DxfPolyLine
{
   friend class DxfParser;
   //
protected:
   int m_expected_vertex_count;
public:
   DxfLwPolyLine( int tag = -1 );
   void clear();  // for reuse when parsing
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

// Arcs and Cicles

class DxfArc : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
   DxfVertex m_centre;
   double m_radius;
   mutable double m_start;
   double m_end;
public:
   DxfArc( int tag = -1 );
   void clear();  // for reuse when parsing
   // getVertex splits into number of segments
   int numSegments(int segments) const;
   DxfVertex getVertex(int i, int segments) const;
   const DxfVertex& getCentre() const
   { return m_centre; }
   const double& getRadius() const
   { return m_radius; }
   int getAttributes() const;
   const DxfRegion& getBoundingBox();
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { m_centre.scale(base_vertex, scale); m_radius *= (fabs(scale.x) + fabs(scale.y)) / 2.0;
     // this is rather tricky to do, need to think more than just reflect around 0,0,0
     if (m_start != m_end && (scale.x < 0 || scale.y < 0)) {
        reflect(scale.x, scale.y);
     }
     DxfRegion::scale(base_vertex, scale);
   }
   void reflect(double x, double y);
   void rotate(const DxfVertex& base_vertex, double angle)
   { m_centre.rotate(base_vertex, angle);
     // this is rather tricky to do, need to think more than just rotate around 0,0,0
     if (m_start != m_end) {
        m_start += angle; m_end += angle;
     }
     DxfRegion::rotate(base_vertex, angle);
   }
   void translate(const DxfVertex& translation)
   { m_centre.translate(translation);
     DxfRegion::translate(translation); }
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};


class DxfEllipse : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
   DxfVertex m_centre;
   DxfVertex m_majorAxisEndPoint;
   DxfVertex m_extrusionDirection;
   double m_minorMajorAxisRatio;
   mutable double m_start;
   double m_end;
public:
   DxfEllipse( int tag = -1 );
   void clear();  // for reuse when parsing
   // getVertex splits into number of segments
   int numSegments(int segments) const;
   DxfVertex getVertex(int i, int segments) const;
   const DxfVertex& getCentre() const
   { return m_centre; }
   const double& getMinorMajorAxisRatio() const
   { return m_minorMajorAxisRatio; }
   int getAttributes() const;
   const DxfRegion& getBoundingBox();
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { m_centre.scale(base_vertex, scale);

       m_majorAxisEndPoint.x *= scale.x;
       m_majorAxisEndPoint.y *= scale.y;

     // this is rather tricky to do, need to think more than just reflect around 0,0,0
     if (m_start != m_end && (scale.x < 0 || scale.y < 0)) {
        reflect(scale.x, scale.y);
     }
     DxfRegion::scale(base_vertex, scale);
   }
   void reflect(double x, double y);
   void rotate(const DxfVertex& base_vertex, double angle)
   { m_centre.rotate(base_vertex, angle);
     // this is rather tricky to do, need to think more than just rotate around 0,0,0
     if (m_start != m_end) {
        m_start += angle; m_end += angle;
     }
     DxfRegion::rotate(base_vertex, angle);
   }
   void translate(const DxfVertex& translation)
   { m_centre.translate(translation);
     DxfRegion::translate(translation); }
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

class DxfCircle : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
   DxfVertex m_centre;
   double m_radius;
public:
   DxfCircle( int tag = -1 );
   void clear();  // for reuse when parsing
   DxfVertex getVertex(int i, int segments) const;
   const DxfVertex& getCentre() const
   { return m_centre; }
   const double& getRadius() const
   { return m_radius; }
   int getAttributes() const;
   const DxfRegion& getBoundingBox();
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { m_centre.scale(base_vertex, scale); m_radius *= (fabs(scale.x) + fabs(scale.y)) / 2.0;
     DxfRegion::scale(base_vertex, scale);
   }
   void reflect(double x, double y);
   void rotate(const DxfVertex& base_vertex, double angle)
   {
      DxfRegion::rotate(base_vertex, angle);
   }
   void translate(const DxfVertex& translation)
   { m_centre.translate(translation);
     DxfRegion::translate(translation); }
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

// Spline
// n.b. currently just linear interpolation between control points -
// not good, but whatever method will have to make some sort of approximation at some point

class DxfSpline : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
public:
   enum { CLOSED = 1 }; // CLOSED = closed spline
protected:
   int m_xyz;
   int m_attributes;
   int m_ctrl_pt_count;
   int m_knot_count;
   std::vector<DxfVertex> m_ctrl_pts;
   std::vector<double> m_knots;
public:
   DxfSpline( int tag = -1 );
   void clear();  // for reuse when parsing
   //
   int numVertices() const;
   const DxfVertex& getVertex(int i) const;
   int getAttributes() const;
   //
   // some basic manipulation
   void scale(const DxfVertex& base_vertex, const DxfVertex& scale)
   { for (int i = 0; i < m_ctrl_pt_count; i++)
        m_ctrl_pts[i].scale(base_vertex, scale);
     DxfRegion::scale(base_vertex, scale); }
   void rotate(const DxfVertex& base_vertex, double angle)
   { for (int i = 0; i < m_ctrl_pt_count; i++)
        m_ctrl_pts[i].rotate(base_vertex, angle);
     DxfRegion::rotate(base_vertex, angle); }
   void translate(const DxfVertex& translation)
   { for (int i = 0; i < m_ctrl_pt_count; i++)
        m_ctrl_pts[i].translate(translation);
     DxfRegion::translate(translation); }
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

// Inserts... these are flattened at parse-time and not retained in layers

class DxfInsert : public DxfEntity, public DxfRegion
{
   friend class DxfParser;
   friend class DxfLayer;
protected:
   std::string m_blockName;
   DxfVertex m_translation;
   DxfVertex m_scale;
   double m_rotation;
public:
   DxfInsert( int tag = -1 );
   void clear();  // for reuse when parsing
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

// Two very simple 'table' entries:

// Line types

class DxfLineType : public DxfTableRow
{
   friend class DxfParser;
public:
   DxfLineType( const std::string& name = "" );
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

// Layers

class DxfLayer : public DxfTableRow, public DxfRegion
{
   friend class DxfParser;
protected:
   // Originally was going to be clever, but it's far easier to have a list for each type:
   std::vector<DxfVertex>   m_points;
   std::vector<DxfLine>     m_lines;
   std::vector<DxfPolyLine> m_poly_lines;
   std::vector<DxfArc>      m_arcs;
   std::vector<DxfEllipse>  m_ellipses;
   std::vector<DxfCircle>   m_circles;
   std::vector<DxfSpline>   m_splines;
   std::vector<DxfInsert>   m_inserts;
   int                  m_total_point_count = 0;
   int                  m_total_line_count = 0;
public:
   DxfLayer( const std::string& name = "" );
   //
   const DxfVertex& getPoint( int i ) const;
   const DxfLine& getLine( int i ) const;
   const DxfPolyLine& getPolyLine( int i ) const;
   const DxfArc& getArc( int i ) const;
   const DxfEllipse& getEllipse( int i ) const;
   const DxfCircle& getCircle( int i ) const;
   const DxfSpline& getSpline( int i ) const;
   //
   int numPoints() const;
   int numLines() const;
   int numPolyLines() const;
   int numArcs() const;
   int numEllipses() const;
   int numCircles() const;
   int numSplines() const;
   //
   int numTotalPoints() const
      { return m_total_point_count; }
   int numTotalLines() const
      { return m_total_line_count; }
   //
   // this merges an insert (so the insert remains flattened)
   void insert(DxfInsert& insert, DxfParser *parser);
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

class DxfBlock : public DxfLayer
{
   friend class DxfParser;
   friend class DxfLayer;
protected:
   DxfVertex m_base_point;
public:
   DxfBlock( const std::string& name = "" );
   //
protected:
   bool parse( const DxfToken& token, DxfParser *parser );
};

///////////////////////////////////////////////////////////////////////////////

class Communicator;

class DxfParser {
   friend class DxfInsert;
   friend class DxfLayer;
public:
   enum token_t { UNIDENTIFIED = -2, ZEROTOKEN = -1 };
   enum section_t { HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS, _EOF };
   enum subsection_t { EXTMIN, EXTMAX,
                       LTYPE_TABLE, LTYPE_ROW, LAYER_TABLE, LAYER_ROW, BLOCK,
                       POINT, LINE, POLYLINE, LWPOLYLINE, ARC, ELLIPSE, CIRCLE, SPLINE, INSERT, VERTEX,
                       ENDSEC };
protected:
   time_t            m_time;
protected:
   DxfRegion              m_region;
   std::map<std::string, DxfLayer>     m_layers;
   std::map<std::string, DxfBlock>     m_blocks;
   std::map<std::string, DxfLineType>  m_line_types;
   //
   long m_size;
   Communicator *m_communicator;
public:
   DxfParser(Communicator *comm = NULL);
   //
   istream& open( istream& stream );
   //
   void openHeader( istream& stream );
   void openTables( istream& stream );
   void openBlocks( istream& stream );
   void openEntities( istream& stream, DxfToken& token, DxfBlock *block = NULL ); // cannot have a default token: it's a reference.  Removed default to DxfToken() AT 29.04.11
   //
   const DxfVertex& getExtMin() const;
   const DxfVertex& getExtMax() const;
   DxfLayer *getLayer( const std::string& layer_name ); // const; <- removed as will have to add layer when DXF hasn't declared one
   DxfLineType *getLineType( const std::string& line_type_name ); // const;
   //
   int numLayers() const;
   int numLineTypes() const;
   //
   friend istream& operator >> (istream& stream, DxfParser& dxfp);

   std::map<std::string, DxfLayer> getLayers() { return m_layers; }
};

///////////////////////////////////////////////////////////////////////////////

#endif
