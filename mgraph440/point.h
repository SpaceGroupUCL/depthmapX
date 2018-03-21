#pragma once

#include "mgraph440/attr.h"
#include "mgraph440/ngraph.h"

namespace mgraph440 {

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
   bool filled() const
      { return (m_state & FILLED) == FILLED; }
   bool selected() const
      { return (m_state & SELECTED) == SELECTED; }
   int getDataObject( int layer ) {
      size_t var = m_data_objects.searchindex( layer );
      if (var != paftl::npos)
         return m_data_objects.at(var);
      return -1;  // note: not paftl::npos
   }
   AttrBody& getAttributes()
      { return *m_attributes; }
   void setAttributes(const AttrBody& attr)
      { if (m_attributes) delete m_attributes;
        m_attributes = new AttrBody(attr); }
   ifstream& read(ifstream& stream, int version, int attr_count);
   ostream &write(ostream &stream, int version);
   void *m_user_data;
   Node& getNode()
      { return *m_node; }
};

}
