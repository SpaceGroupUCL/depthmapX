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


// ngraph.h

#ifndef __NGRAPH_H__
#define __NGRAPH_H__

#include "mgraph440/pixelref.h"
#include "mgraph440/paftl.h"
#include <set>

namespace mgraph440 {

class PointMap;
struct MetricPair;
struct MetricTriple;

struct PixelVec
{
   PixelRef m_start;
   PixelRef m_end;
   PixelVec(const PixelRef start = NoPixel, const PixelRef end = NoPixel)
   { m_start = (int) start; m_end = (int) end; };
   PixelRef start() const
   { return m_start; }
   PixelRef end() const
   { return m_end; }
   //
   std::ifstream& read(std::ifstream& stream, int version, const char dir);
   std::ifstream& read(std::ifstream& stream, int version, const char dir, const PixelVec& context);
   std::ostream &write(std::ostream &stream, const char dir);
   std::ostream& write(std::ostream& stream, const char dir, const PixelVec& context);
};

class Bin
{
   friend class Node;
protected:
   char m_dir;
   unsigned short m_length;
   unsigned short m_node_count;
   float m_distance;
   float m_occ_distance;
   PixelVec *m_pixel_vecs;
public:
   Bin()
   { m_dir = PixelRef::NODIR; m_length = 0; m_pixel_vecs = NULL; m_node_count = 0; m_distance = 0.0f; m_occ_distance = 0.0f; }
   Bin(const Bin&)
   { throw 1; }
   Bin& operator = (const Bin&)
   { throw 1; }
   ~Bin()
   { if (m_pixel_vecs) delete [] m_pixel_vecs; m_pixel_vecs = NULL; }
   //
   void make(const PixelRefVector& pixels, char m_dir);
   //
   int count() const
   { return m_node_count; }
   float distance() const
   { return m_distance; }
   float occdistance() const
   { return m_occ_distance; }
   //
   void setOccDistance(float d)
   { m_occ_distance = d; }
   //
   bool containsPoint(const PixelRef p) const;
   //
   // Iterator
protected:
   // Conversion back to old fashioned schema:
   mutable int m_curvec;
   mutable PixelRef m_curpix;
public:
//   void contents(PixelRefVector& hood);
   void first() const;
   void next() const;
   bool is_tail() const;
   PixelRef cursor() const;
   //
   std::ifstream& read(std::ifstream& stream, int version);
   std::ostream &write(std::ostream &stream, int version);
};

class Node
{
protected:
   PixelRef m_pixel;
   Bin m_bins[32];
public:
   // testing some agent stuff:
   std::vector<PixelRef> m_occlusion_bins[32];
public:
   Node()
   { ; }
   Node(const Node&)
   { throw 1; }
   Node& operator = (const Node&)
   { throw 1; }
   ~Node()
   { ; }
   // Conversion back to old fashioned schema:
   mutable int m_curbin;
   // Note: this function clears the bins as it goes
   void make(const PixelRef pix, PixelRefVector *bins, float *bin_far_dists, int q_octants);
   //
   void setPixel(const PixelRef& pixel)
   { m_pixel = pixel; }
   Bin& bin(int i)
   { return m_bins[i]; }
   float occdistance(int i)
   { return m_bins[i].occdistance(); }
   int count()
   { int c = 0; for (int i = 0; i < 32; i++) c += m_bins[i].count(); return c; }
   void first() const;
   void next() const;
   PixelRef cursor() const;
   std::ifstream& read(std::ifstream& stream, int version);
   std::ostream &write(std::ostream &stream, int version);
};

// Two little helpers:

class PixelRefH : public PixelRef
{
public:
   PixelRefH() : PixelRef()
   {;}
   PixelRefH(const PixelRef& p) : PixelRef(p)
   {;}
   friend bool operator > (const PixelRefH& a, const PixelRefH& b);
   friend bool operator < (const PixelRefH& a, const PixelRefH& b);
};
inline bool operator > (const PixelRefH& a, const PixelRefH& b)
{
   return (a.y > b.y || (a.y == b.y && a.x > b.x));
}
inline bool operator < (const PixelRefH& a, const PixelRefH& b)
{
   return (a.y < b.y || (a.y == b.y && a.x < b.x));
}
class PixelRefV : public PixelRef
{
public:
   PixelRefV() : PixelRef()
   {;}
   PixelRefV(const PixelRef& p) : PixelRef(p)
   {;}
   friend bool operator > (const PixelRefV& a, const PixelRefV& b);
   friend bool operator < (const PixelRefV& a, const PixelRefV& b);
};
inline bool operator > (const PixelRefV& a, const PixelRefV& b)
{
   return (a.x > b.x || (a.x == b.x && a.y > b.y));
}
inline bool operator < (const PixelRefV& a, const PixelRefV& b)
{
   return (a.x < b.x || (a.x == b.x && a.y < b.y));
}

}

#endif
