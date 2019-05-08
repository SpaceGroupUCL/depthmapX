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

#pragma once

// Modifed by Dream
#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fstream>

#include "mgraph440/paftl.h"
#include "mgraph440/p2dpoly.h"
#include "mgraph440/mgraph_consts.h"

namespace mgraph440 {

// Pretty much all of this is now deprecated...
// the attrs will be replaced by the easier to use AttributeTables in the future...

// ...and so it now is:

union AttrVal {
   int   intval;
   float floatval;
};

const int NUM_PHYSICAL_ATTRIBUTES = 25;     // 25 physical attributes: see below

// Note: also see AttrMap at bottom of file.

// THIS ENTIRE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

struct AttrHeader {
   enum { NEIGHBOURHOOD_SIZE =   0,
          GRAPH_SIZE         =   1,
          KERNEL_SIZE        =   2,
          CLIQUE_SIZE        =   3,
          TOTAL_DEPTH        =   4,
          MEAN_DEPTH         = 101,
          INTEGRATION_RA     = 102,
          INTEGRATION_RRA    = 103,
          INTEGRATION_HILL   = 104,
          INTEGRATION_TEKL   = 105,
          ENTROPY            =   5,
          REL_ENTROPY        =   6,
          CLUSTER            =   7,
          UNUSED             =   8,
          POINT_DEPTH        =   9,
          CONTROL_HILL       =  10,
          CONTROL_TURN       =  11,
          MEDIAN_ANGLE       =  12,
          FAR_NODE           =  13, // not output in text file
          FAR_DIST           =  14,
          AGENT_COUNT        =  15,
          AGENT_COLL_COUNT   =  16, // collisions
          TOTAL_DIST         =  17,
          AVG_DIST           = 107, // derived
          TOTAL_METRIC_DEPTH =  18,
          METRIC_GRAPH_SIZE  =  19,
          METRIC_MEAN_DEPTH  = 108, // derived
          DECENTRAL_INTEG    = 109, // derived
          METRIC_POINT_DEPTH =  20,
          TOTAL_EUCLID_DIST  =  21,
          POINT_EUCLID_DIST  =  22,
          MEAN_PENN_DIST     = 110, // derived
          POINT_PENN_DIST    = 111, // derived
          TOTAL_METRIC_ANGLE =  23,
          METRIC_POINT_ANGLE =  24,
          METRIC_MEAN_ANGLE  = 112  // derived
   };

   int   m_attr_count;      // number of attributes: for storage

   AttrHeader(int n = NUM_PHYSICAL_ATTRIBUTES)
      { m_attr_count = n; }
   void reset(AttrVal attributes[]);
   double getAttr(int attr, const AttrVal attributes[]) const;
};

const AttrHeader g_attr_header;

class PafAgent;

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

struct AttrBody {
   //
   // stored values
   int         pos;
   int         ref;
   Point3f     origin;        // storing origin unnecessary, may revise later
   AttrHeader *header;
   AttrVal    *attributes;
   // colour and highlight are not stored
   float       color;
   bool        highlight;
   //
   // you can have agents *in* the graph too (helps with looking them up!)
   PafAgent   *myagent;
   //
   AttrBody(std::streampos p = -1, const AttrHeader& h = AttrHeader() );
   AttrBody(const AttrBody& attr);
   ~AttrBody();
   void reset();
   //
   double getAttr(int attr) const
      { return header->getAttr(attr, attributes); }
   //
   void write(std::ostream &stream ) const;
   void read(  std::ifstream& stream, int attr_count );
   //
   // Save a bit of memory, use integer / floating as appropriate
   int& intval(int i)
      { return attributes[i].intval; }
   float& floatval(int i)
      { return attributes[i].floatval; }
   //
   friend std::ostream& operator << (std::ostream& stream, const AttrBody& attr);
   //
   friend bool operator == (const AttrBody& a, const AttrBody& b);
   friend bool operator != (const AttrBody& a, const AttrBody& b);
   friend bool operator < (const AttrBody& a, const AttrBody& b);
   friend bool operator > (const AttrBody& a, const AttrBody& b);
};

// These allow an order list of attributes (used by the conversion routine)
inline bool operator == (const AttrBody& a, const AttrBody& b)
{
   return a.ref == b.ref;
}
inline bool operator != (const AttrBody& a, const AttrBody& b)
{
   return a.ref != b.ref;
}
inline bool operator > (const AttrBody& a, const AttrBody& b)
{
   return a.ref > b.ref;
}
inline bool operator < (const AttrBody& a, const AttrBody& b)
{
   return a.ref < b.ref;
}

///////////////////////////////////////////////////////////////////////////////

// a graph vertex

/*
class GraphVertex {
   friend class ArVertexList;
protected:
   pvecint m_connections;
public:
   GraphVertex()
   {}
   ~GraphVertex()
   {}
public:
   pvecint& get_connections() {
      return m_connections;
   }
   int count_connections() {
      return m_connections.size();
   }
   int& operator [] (int index) {
      return m_connections[index];
   }
   void add_connection(int index) {
#ifdef _DEBUG
      if (m_connections.findindex(index) != paftl::npos) {
         cerr << "oops" << endl;
      }
#endif
      m_connections.push_back(index);
   }
};
*/

///////////////////////////////////////////////////////////////////////////////

// new angular member

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

inline short& loword(int& a) {return (short&) *((short *)&a);}
inline short& hiword(int& a) {return (short&) *((short *)&a + 1); }

class ArVertex {
   friend class ArVertexList;
protected:
   pvecint m_nodes;
   pvecint m_bins;
public:
   class Iterator {
   protected:
      ArVertex *m_vertex;
      short m_current;
      short m_last;
      short m_top;
   public:
      Iterator() {
         m_vertex = NULL;
      }
      Iterator(ArVertex *vertex, int bin) {
         m_vertex  = vertex;
         m_current = loword(m_vertex->m_bins[bin]);
         m_last    = hiword(m_vertex->m_bins[bin]);
         m_top     = (short)m_vertex->m_nodes.size();
         if (m_current == m_last)
            m_current = -1;
      }
      Iterator(ArVertex *vertex, short current, short last) {
         m_vertex  = vertex;
         m_current = current;
         m_last    = last;
         m_top     = (short)m_vertex->m_nodes.size();
         if (m_current == m_last)
            m_current = -1;
         else if (m_current == m_top)
            m_current = 0;
      }
      int& operator *() { return m_vertex->m_nodes[m_current]; }
      operator short() { return m_current; }
      virtual Iterator& operator ++(int) {
         if ((++m_current) - m_last == 0) // <- might jump, and then whoops
            m_current = -1;
         else if (m_current >= m_top)     // <- although this just loops back
            m_current = 0;
         return *this;
      }
   };
   friend class Iterator;
   Iterator fovealView(int bin) {
      Iterator i( this, loword(m_bins[(bin+30)%32]), hiword(m_bins[(bin+2)%32]) );
      return i;
   }
   Iterator peripheralViewFemale(int bin) {
      // a fudge follows...
      // essentially, the iterator can't determine the difference between 6,6 and 6,7...5,6
      bool found = false;
      for (int i = bin - 8; i <= bin + 8; i++) {
         if (loword(m_bins[(i+32)%32]) != hiword(m_bins[(i+32)%32]))
            found = true;
      }
      if (loword(m_bins[(bin+24)%32]) == hiword(m_bins[(bin+8)%32]) && found)
         return Iterator( this, 0, (short)m_nodes.size() );
      else
         return Iterator( this, loword(m_bins[(bin+24)%32]), hiword(m_bins[(bin+8)%32]) );
   }
   Iterator peripheralViewMale(int bin) {
      Iterator i( this, loword(m_bins[(bin+25)%32]), hiword(m_bins[(bin+7)%32]) );
      return i;
   }
public:
   enum { bin_count = 32 };
   ArVertex( const pvecint& nodes = pvecint(), const pvecint& bins = pvecint() )
      { m_nodes = nodes; m_bins = bins; }
   ArVertex( const ArVertex& v )
      { m_nodes = v.m_nodes; m_bins = v.m_bins; }
   ArVertex& operator = (const ArVertex& v )
   {  if (&v != this) {
         m_nodes = v.m_nodes; m_bins = v.m_bins;
      }
      return *this; }
   void make( pvecint *bin_list ) {
      // If you're a clever bunny using the sparkGraph algorithm, the bins come presorted
      // ...this code is the same as the one below... tidy at some point!
      int bin_marker;
      loword(bin_marker) = 0;
      hiword(bin_marker) = 0;
      for (int i = 0; i < bin_count; i++) {
         for (size_t j = 0; j < bin_list[i].size(); j++) {
            m_nodes.push_back( bin_list[i][j] );
         }
         hiword(bin_marker) += (short)bin_list[i].size();
         m_bins.push_back( bin_marker );
         loword(bin_marker) = hiword(bin_marker);
         bin_list[i].clear();    // <- NOTE: useful to clear this here
      }
   }
   void make( pqmap<double,int> *bin_list ) {
      // Same as above
      int bin_marker;
      loword(bin_marker) = 0;
      hiword(bin_marker) = 0;
      for (int i = 0; i < bin_count; i++) {
         for (size_t j = 0; j < bin_list[i].size(); j++) {
            m_nodes.push_back( bin_list[i][j] );
         }
         hiword(bin_marker) += (short)bin_list[i].size();
         m_bins.push_back( bin_marker );
         loword(bin_marker) = hiword(bin_marker);
         bin_list[i].clear();    // <- NOTE: useful to clear this here
      }
   }
   void clear()
   {
      m_nodes.clear();
      m_bins.clear();
   }
   // Old version compatibility:
   int& operator [] (int i) { return m_nodes[i]; }
   int size() const { return (int)m_nodes.size(); }
   //
   // All connected vertices
   pvecint& edgeset() { return m_nodes; }
   // Edges from a particular bin
   Iterator binset(int binnum) {
      return Iterator( this, binnum );
   }
   int binsize(int bin) {
      // NB --- bin with all nodes will give erroneous zero!
      return (int)((m_nodes.size() + hiword(m_bins[bin]) - loword(m_bins[bin])) % m_nodes.size());
   }
   //
   std::ifstream& read( std::ifstream& stream, std::streampos offset, int metagraph_version )
   {
      if (metagraph_version >= mgraph440::VERSION_BINS_INTROD) {
         m_nodes.read(stream,offset);  // read from offset...
         m_bins.read(stream);          // <- and now read bins straight away
      }
      else {
         m_nodes.read(stream,offset);  // no angular before version 30
      }
      return stream;
   }
   std::ofstream& write(std::ofstream& stream )
   {
      m_nodes.write(stream);
      m_bins.write(stream);
      return stream;
   }
};

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

class ArVertexList {
public:
   enum {   NONE              = 0x0000,
            BASIC             = 0x0001,
            LOCAL             = 0x0002,
            GLOBAL            = 0x0004,
            POINTDEPTH        = 0x0008,
            ANGULAR           = 0x0010,
            METRIC            = 0x0020,
            METRICPOINTDEPTH  = 0x0040,
            ANGULARPOINTDEPTH = 0x0080    // for historical reasons (which was implemented when) these are in a strange order
   };    // which attributes have been calculated
protected:
   // stored here for ease of use
   int m_metagraph_version;
   ::std::string m_filename;
   std::fstream *m_stream;
   AttrHeader m_attr_header;
   prefvec<AttrBody> m_attributes;
   int m_cache_ref;
   ArVertex m_cache_data;         // either cached...
   bool m_mem_loaded;
   prefvec<ArVertex> m_mem_data;  // ...or all in memory
   int m_which_attributes;
public:
   ArVertexList( const ::std::string& filename = ::std::string() )   {
      m_metagraph_version = mgraph440::METAGRAPH_VERSION;
      m_filename = filename;
      m_stream = NULL;
      m_cache_ref = -1;
      m_mem_loaded = false;
      m_which_attributes = ArVertexList::NONE;
   }
   virtual ~ArVertexList() {
      close(); // <-- used to be remove: ensure MetaGraph removes if a temporary file
   }
   int size() const {
      return (int)m_attributes.size();
   }
   ArVertex& operator [] (int i) {
      if (m_mem_loaded) {
         return m_mem_data[i];
      }
      else if (i != m_cache_ref && m_stream) { // <- just make sure this doesn't crash
         m_cache_data.read( (std::ifstream&) *m_stream, m_attributes[i].pos, m_metagraph_version );
         m_cache_ref = i;
      }
      return m_cache_data;
   }
   //
   long memsize() const {
      return (m_attributes.tail().pos );  // roughly right : excludes final node
   }
   // file must be open to do this
   void loadmem() {
      for (size_t i = 0; i < m_attributes.size(); i++) {
         m_mem_data.push_back( ArVertex() );
         m_mem_data.tail().read( (std::ifstream&) *m_stream, m_attributes[i].pos, m_metagraph_version );
      }
      m_mem_loaded = true;
   }
   void unloadmem() {
      m_mem_data.clear();
      m_mem_loaded = false;
   }
   //
   void setFilename( const std::string& filename ) {
      m_filename = filename;
   }
   const std::string& getFilename() const {
      return m_filename;
   }
   //
   void setWhichAttributes(int which_attributes) {
      m_which_attributes |= which_attributes;
   }
   int getWhichAttributes() const {
      return m_which_attributes;
   }
   void clearAttributes() {
      m_attributes.clear();
      m_which_attributes = ArVertexList::NONE;
   }
   //
   bool openwrite(int nodes);
   void openread();
   void close();
   void remove();
   void add( int ref, const ArVertex& node );
   void commit();                                                 // when copying
   void commit( const Point2f& p, int far_node, float far_dist, float total_dist ); // when creating new node
   //
   bool read( std::ifstream& stream, int metagraph_version );
   bool write(std::ostream &stream );
   //
   const AttrBody& getAttributes(int i) const {
      return m_attributes[i];
   }
   AttrBody& getAttributes(int i) {
      return m_attributes[i];
   }
};

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

///////////////////////////////////////////////////////////////////////////////

// depthmap: file based graph vertex lists:
typedef ArVertex     GraphVertex;
typedef ArVertexList GraphVertexList;

// standard: memory resident graph vertex lists:
//   typedef Vertex                    GraphVertex;
//   typedef pqmap<int,GraphVertex>    GraphVertexList;
// --- now sadly dead and buried
///////////////////////////////////////////////////////////////////////////////



// Exception to be thrown if the thread is cancelled
/*
class thread_cancelled_exception {
public:
   thread_cancelled_exception()
   {;}
};
*/


// Attribute map is for choosing attributes by description

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

struct AttrMap {
   enum  { ATTR_INT, ATTR_FLOAT };
   int   ref;
   char *desc;
   int   attr_set;
   int   modules_req;
   int   type;
   AttrMap(int r, char *d, int a, int m, int t) { ref = r; desc = d; attr_set = a; modules_req = m; type = t; }
   int   usable() const {return true; }
   bool  intval() const {return type == ATTR_INT;}
   bool  floatval() const {return type == ATTR_FLOAT;}
};

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

// This *must* match number of attributes listed below!
const int NUM_DISPLAYABLE_ATTRIBUTES = 26;

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

const AttrMap g_attr_display_map[] = {
   // 0
   AttrMap(AttrHeader::NEIGHBOURHOOD_SIZE, (char *)"Neighbourhood Size",  GraphVertexList::BASIC,      0,  AttrMap::ATTR_INT),
   AttrMap(AttrHeader::MEAN_DEPTH,         (char *)"Mean Depth",          GraphVertexList::GLOBAL,     0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::INTEGRATION_RA,     (char *)"Relative Asymmetry",  GraphVertexList::GLOBAL,     0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::INTEGRATION_RRA,    (char *)"Real Relative Asymmetry*", GraphVertexList::GLOBAL,0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::INTEGRATION_HILL,   (char *)"Integration (Hillier/Hanson)*",  GraphVertexList::GLOBAL, 0, AttrMap::ATTR_FLOAT),
   // 5
   AttrMap(AttrHeader::INTEGRATION_TEKL,   (char *)"Integration (Teklenburg et al.)",  GraphVertexList::GLOBAL, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::GRAPH_SIZE,         (char *)"Graph Size",          GraphVertexList::GLOBAL,     0,  AttrMap::ATTR_INT),
   AttrMap(AttrHeader::ENTROPY,            (char *)"Entropy",             GraphVertexList::GLOBAL,     0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::REL_ENTROPY,        (char *)"Relativised Entropy", GraphVertexList::GLOBAL,     0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::CLUSTER,            (char *)"Clustering Coefficient", GraphVertexList::LOCAL,   0,  AttrMap::ATTR_FLOAT),
   // 10
   AttrMap(AttrHeader::CONTROL_HILL,       (char *)"Control (Hillier/Hanson)", GraphVertexList::LOCAL, 0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::CONTROL_TURN,       (char *)"Control (Turner)",    GraphVertexList::LOCAL,      0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::POINT_DEPTH,        (char *)"Point Depth",         GraphVertexList::POINTDEPTH, 0,  AttrMap::ATTR_INT),
   AttrMap(AttrHeader::MEDIAN_ANGLE,       (char *)"Mean Angle",          GraphVertexList::ANGULAR,    0,AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::FAR_DIST,           (char *)"Far Neighbour Distance",     GraphVertexList::BASIC,      0,  AttrMap::ATTR_FLOAT),
   // 15
   AttrMap(AttrHeader::TOTAL_DIST,         (char *)"Total Neighbour Distance",   GraphVertexList::BASIC,      0,    AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::AVG_DIST,           (char *)"Average Neighbour Distance", GraphVertexList::BASIC,      0,    AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::AGENT_COUNT,        (char *)"Agent Trails",        GraphVertexList::BASIC,      0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::AGENT_COLL_COUNT,   (char *)"Agent Collisions",    GraphVertexList::BASIC,      0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::METRIC_MEAN_DEPTH,  (char *)"Metric Mean Depth",   GraphVertexList::METRIC,     0,    AttrMap::ATTR_FLOAT),
   // 20
   AttrMap(AttrHeader::METRIC_MEAN_ANGLE,  (char *)"Metric Mean Angle",      GraphVertexList::METRIC,     0,    AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::MEAN_PENN_DIST,     (char *)"Mean Penn Distance",        GraphVertexList::METRIC, 0,  AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::DECENTRAL_INTEG,    (char *)"Decentralised Integration", GraphVertexList::METRIC, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::METRIC_POINT_DEPTH, (char *)"Metric Point Depth",        GraphVertexList::METRICPOINTDEPTH, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::METRIC_POINT_ANGLE, (char *)"Metric Point Angle",        GraphVertexList::METRICPOINTDEPTH, 0, AttrMap::ATTR_FLOAT),
   // 25
   AttrMap(AttrHeader::POINT_PENN_DIST,    (char *)"Point Penn Distance",       GraphVertexList::METRICPOINTDEPTH, 0, AttrMap::ATTR_FLOAT)
};

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

// This *must* match number of attributes listed below!
const int NUM_SUMMARISABLE_ATTRIBUTES = 23;

// THIS PART OF THE FILE IS DEPRECATED
// ATTRIBUTE TABLES ARE USED INSTEAD

const AttrMap g_attr_summary_map[] = {
   // 0
   AttrMap(AttrHeader::NEIGHBOURHOOD_SIZE, (char *)"Neighbourhood Size",     1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::FAR_DIST,           (char *)"Far Neighbour Distance", 1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::TOTAL_DIST,         (char *)"Total Neighbour Distance", 1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::GRAPH_SIZE,         (char *)"Graph Size",             1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::KERNEL_SIZE,        (char *)"Kernel Size",            1, 0, AttrMap::ATTR_INT),
   // 5
   AttrMap(AttrHeader::CLIQUE_SIZE,        (char *)"Clique Size",            1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::TOTAL_DEPTH,        (char *)"Total Visual Depth",     1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::ENTROPY,            (char *)"Entropy",                1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::REL_ENTROPY,        (char *)"Relativised Entropy",    1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::CLUSTER,            (char *)"Clustering Coefficient", 1, 0, AttrMap::ATTR_FLOAT),
   // 10
   AttrMap(AttrHeader::POINT_DEPTH,        (char *)"Point Depth",            1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::CONTROL_HILL,       (char *)"Control (Hillier/Hanson)", 1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::CONTROL_TURN,       (char *)"Control (Turner)",       1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::MEDIAN_ANGLE,       (char *)"Mean Angle",             1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::AGENT_COUNT,        (char *)"Agent Trails",           1, 0, AttrMap::ATTR_INT),
   // 15
   AttrMap(AttrHeader::AGENT_COLL_COUNT,   (char *)"Agent Collisions",       1, 0, AttrMap::ATTR_INT),
   AttrMap(AttrHeader::TOTAL_METRIC_DEPTH, (char *)"Total Metric Depth",     1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::TOTAL_METRIC_ANGLE, (char *)"Total Metric Angle",     1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::TOTAL_EUCLID_DIST,  (char *)"Total Euclidean Distance", 1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::METRIC_GRAPH_SIZE,  (char *)"Metric Graph Size",      1, 0, AttrMap::ATTR_INT),
   // 20
   AttrMap(AttrHeader::METRIC_POINT_DEPTH, (char *)"Metric Point Depth",     1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::METRIC_POINT_ANGLE, (char *)"Metric Point Angle",     1, 0, AttrMap::ATTR_FLOAT),
   AttrMap(AttrHeader::POINT_EUCLID_DIST,  (char *)"Euclidean Point Depth",  1, 0, AttrMap::ATTR_FLOAT)
};

}
