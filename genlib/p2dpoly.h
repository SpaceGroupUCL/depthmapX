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


// 2d poly (own format, adapted from the original Sala libraries

// The idea is that from this format,
// we can read into Cosmo3d as well as Sala based applications

#ifndef __P2DPOLY_H__
#define __P2DPOLY_H__

// Using doubles right the way through can really eat memory for isovist
// polygon files, thus we use a defined type, change as appropriate:

#include <algorithm>
#include <genlib/pafmath.h>
#include <genlib/comm.h> // communicator used in BSP tree construction

// Note: code depends on XAXIS being 0 and YAXIS being 1 --- do not change
enum {
   NOAXIS = -1, XAXIS = 0, YAXIS = 1
};

class  Point2f;
bool approxeq(const Point2f& p1, const Point2f& p2, double tolerance = 0.0);
class  QtRegion;
bool intersect_region(const QtRegion& a, const QtRegion& b, double tolerance = 0.0);
bool overlap_x(const QtRegion& a, const QtRegion& b, double tolerance = 0.0);
bool overlap_y(const QtRegion& a, const QtRegion& b, double tolerance = 0.0);
class Line;
bool intersect_line(const Line& a, const Line& b, double tolerance = 0.0);
bool intersect_line_no_touch(const Line& a, const Line& b, double tolerance = 0.0);
int  intersect_line_distinguish(const Line& a, const Line& b, double tolerance = 0.0);
int  intersect_line_b(const Line& a, const Line& b, double tolerance = 0.0);
Point2f intersection_point(const Line& a, const Line& b, double tolerance = 0.0);

// NaN on Intel:
// Quick mod - TV
// const double P2DNULL = (const double)0xFFFFFFFF7FF7FFFF;
// for non-Intel:  0x7FF7FFFFFFFFFFFF

// Point

class Point2f
{
public:
   double x;
   double y;
   Point2f()
//      { x = P2DNULL; y = P2DNULL; }
      { x = 0.0; y = 0.0; }
   Point2f(double a, double b)
      { x = a; y = b; }
   bool atZero() const
//      { return x == P2DNULL || y == P2DNULL; }
      { return x == 0.0 && y == 0.0; }
   void normalScale( const QtRegion& );  // inline function: below region
   void denormalScale( const QtRegion& );
   void operator += (const Point2f& p)
      { x += p.x; y += p.y; }
   void operator -= (const Point2f& p)
      { x -= p.x; y -= p.y; }
   void operator *= (const double s)
      { x *= s; y *= s; }
   void operator /= (const double s)
      { x /= s; y /= s; }
   double& operator [] (int i)
      { return (i == XAXIS) ? x : y; }
   const double& operator [] (int i) const
      { return (i == XAXIS) ? x : y; }
   friend Point2f operator - (Point2f& p);
   friend Point2f operator + (const Point2f& p1, const Point2f& p2);
   friend Point2f operator - (const Point2f& p1, const Point2f& p2);
   friend bool operator == (const Point2f& p1, const Point2f& p2);
   friend bool operator != (const Point2f& p1, const Point2f& p2);
   friend bool operator > (const Point2f& a, const Point2f& b);
   friend bool operator < (const Point2f& a, const Point2f& b);
   friend Point2f operator * (const double s, const Point2f& p);
   friend Point2f operator / (const Point2f& p, const double s);
   friend double dot(const Point2f& p1, const Point2f& p2);
   friend double det(const Point2f& p1, const Point2f& p2);
   friend double dist(const Point2f& p1, const Point2f& p2);
   friend double dist(const Point2f& point, const Line& line);
   friend double angle(const Point2f& p1, const Point2f& p2, const Point2f& p3);
   friend bool approxeq(const Point2f& p1, const Point2f& p2, double tolerance);
   friend Point2f pointfromangle(double angle);
   // a couple of useful tests
   bool intriangle(const Point2f& p1, const Point2f& p2, const Point2f& p3);
   bool insegment(const Point2f& key, const Point2f& p2, const Point2f& p3, double tolerance = 0.0);
   // for OS transformation (note: accurate only to 5 metres according to OS)
   Point2f longlat2os(const Point2f& p);
public:
   // A few simple vector ops:
   double length() const
      { return (double) sqrt(x * x + y * y); }
   Point2f& scale(const double scalar)
      { x *= scalar; y *= scalar; return *this; }
   Point2f& scale(const Point2f& scalevec)
      { x *= scalevec.x; y *= scalevec.y; return *this; }
   Point2f& normalise()
      { return scale( 1.0 / length() ); }
   Point2f& rotate(const double angle)
      { double t = x;
        x =  x * cos(angle) - y * sin(angle);
        y =  y * cos(angle) + t * sin(angle);
        return *this; }
   double angle() const
      { return (y < 0) ? (2.0 * M_PI - acos(x)) : acos(x); }
};

inline Point2f operator - (Point2f& p)
{
   return Point2f(-p.x, -p.y);
}

inline Point2f operator + (const Point2f& p1, const Point2f& p2)
{
   return Point2f(p1.x + p2.x, p1.y + p2.y);
}

inline Point2f operator - (const Point2f& p1, const Point2f& p2)
{
   return Point2f(p1.x - p2.x, p1.y - p2.y);
}

inline bool operator == (const Point2f& p1, const Point2f& p2)
{ return (p1.x == p2.x && p1.y == p2.y); }
inline bool operator != (const Point2f& p1, const Point2f& p2)
{ return (p1.x != p2.x || p1.y != p2.y); }
inline bool operator > (const Point2f& p1, const Point2f& p2)
{ return (p1.x > p2.x || (p1.x == p2.x && p1.y > p2.y)); }
inline bool operator < (const Point2f& p1, const Point2f& p2)
{ return (p1.x < p2.x || (p1.x == p2.x && p1.y < p2.y)); }

inline Point2f operator * (const double s, const Point2f& p)
{
   return Point2f(s * p.x, s * p.y);
}

inline Point2f operator / (const Point2f& p, const double s)
{
   return Point2f(p.x / s, p.y / s);
}

inline double dot(const Point2f& p1, const Point2f& p2)
{
   return (p1.x * p2.x + p1.y * p2.y);
}

// greater than 0 => p2 left (anticlockwise) of p1, less than 0 => p2 right (clockwise) of p1
inline double det(const Point2f& p1, const Point2f& p2)
{
   return (p1.x * p2.y - p1.y * p2.x);
}

inline double dist(const Point2f& p1, const Point2f& p2)
{
   return sqrt(sqr(p1.x - p2.x) + sqr(p1.y - p2.y));
}

inline double angle(const Point2f& p1, const Point2f& p2, const Point2f& p3)
{
   Point2f a = p1 - p2;
   Point2f b = p3 - p2;
   a.normalise();
   b.normalise();
   // ensure in range (f.p. error can throw out)
   double d = std::min<double>(std::max<double>(dot(a,b),-1.0),1.0);
   return (sgn(det(a,b)) == 1) ? acos(d) : 2.0 * M_PI - acos(d);
}

inline bool approxeq(const Point2f& p1, const Point2f& p2, double tolerance)
{
   return (fabs(p1.x - p2.x) <= tolerance && fabs(p1.y - p2.y) <= tolerance);
}

inline bool Point2f::insegment(const Point2f& key, const Point2f& p2, const Point2f& p3, double tolerance)
{
   Point2f va = p2 - key;
   Point2f vb = p3 - key;
   Point2f vp = *this - key;
   double ap = det(va,vp);
   double bp = det(vb,vp);
   if ((dot(va,vp) > 0 && dot(vb,vp) > 0) && (sgn(ap) != sgn(bp) || fabs(ap) < tolerance || fabs(bp) < tolerance) ) {
      return true;
   }
   return false;
}

inline bool Point2f::intriangle(const Point2f& p1, const Point2f& p2, const Point2f& p3)
{
   // touching counts
   int test = sgn(det(p2-p1,*this-p1));
   if (test == sgn(det(p3-p2,*this-p2)) && test == sgn(det(p1-p3,*this-p3))) {
      return true;
   }
   return false;
}

inline Point2f pointfromangle(double angle)
{
   Point2f p;
   p.x = cos(angle);
   p.y = sin(angle);
   return p;
}

Point2f gps2os(const Point2f& p);

// an event is a point plus time (as in spacetime technical language)
class Event2f : public Point2f
{
public:
   double t; // time in seconds
   Event2f() : Point2f()
   { t = 0.0; }
   Event2f(double _x, double _y, double _t) : Point2f(_x,_y)
   { t = _t; }
   Event2f(Point2f& _p) : Point2f(_p)
   { t = 0.0; }
   Event2f(Point2f& _p, double _t) : Point2f(_p)
   { t = _t; }
};

///////////////////////////////////////////////////////////////////////////////////////////

class Point3f
{
public:
   double x;
   double y;
   double z;
   Point3f(double a = 0.0, double b = 0.0, double c = 0.0)
      { x = a; y = b; z = c;}
   Point3f(const Point2f& p)
      { x = p.x; y = 0.0; z = p.y; } // Note! not z = -y (due to an incosistency earlier...)
   bool inside( const Point3f& bl, const Point3f& tr )   // now inclusive (...)
      { return (x >= bl.x && y >= bl.y && z >= bl.z && x <= tr.x && y <= tr.y && z <= tr.z); }
   operator Point2f()
      { return Point2f( x, z ); } // Note! not x, -z (due to an inconsistency earlier...)
   Point2f xy()
      { return Point2f(x, y); }  // From the x, y plane
   // A few simple vector ops:
   double length() const
      { return (double) sqrt(x * x + y * y + z * z); }
   Point3f& scale(const double scalar)
      { x *= scalar; y *= scalar; z *= scalar; return *this; }
   Point3f& normalise()
      { return scale( 1.0 / length() ); }
   Point3f& rotate(double theta, double phi)
      { double t = x;
        x = t * cos(theta) - y * sin(theta);
        y = y * cos(theta) + t * sin(theta);
        t = x;
        x = t * cos(phi) - z * sin(phi);
        z = z * cos(phi) - t * sin(phi);
        return *this; }
   //
   friend double dot(const Point3f& a, const Point3f& b);
   friend Point3f cross(const Point3f& a, const Point3f& b);
};

inline double dot(const Point3f& a, const Point3f& b)
{
   return (a.x * b.x + a.y * b.y + a.z * b.z);
}
inline Point3f cross(const Point3f& a, const Point3f& b)
{
   return Point3f( a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y );
}

//

//////////////////////////////////////////////////////////////////////////////

// used for clipping of polygons to regions

struct EdgeU {
   int edge;
   double u;
   EdgeU(int e = -1, double _u = 0.0)
   { edge = e; u = _u; }
   EdgeU(const EdgeU& eu)
   { edge = eu.edge; u = eu.u; }
   friend bool ccwEdgeU(const EdgeU& a, const EdgeU& b, const EdgeU& c);
};

// QtRegion

class QtRegion
{
public:
   Point2f bottom_left;
   Point2f top_right;
   QtRegion(const Point2f& bl = Point2f(), const Point2f& tr = Point2f())
      { bottom_left = bl; top_right = tr; }
   QtRegion(const QtRegion& r)
      { bottom_left = r.bottom_left; top_right = r.top_right; }
   QtRegion& operator = (const QtRegion& r)
      { bottom_left = r.bottom_left; top_right = r.top_right; return *this; }
   //
   double height() const
      { return top_right.y - bottom_left.y; }
   double width() const
   // The assumption that top_right.x is always > bottom_left.x is not always true.
   // Returning a negative value here causes an infinite loop at axialmap.cpp line 3106
   // after overlapdist is assigned a negative value at axialmap.cpp line 3084.
   // height() above could also be changed for this reason, but this is a band-aid
   // fix for the real problem, which is why the top_right > bottom_left assumption
   // is assumed to be 100% valid but is, in some instances, not valid.
   // { return top_right.x - bottom_left.x; }
      { return fabs(top_right.x - bottom_left.x); }
   double area() const
      { return height() * width(); }
   void normalScale( const QtRegion& r )
      { top_right.normalScale(r); bottom_left.normalScale(r); }
   void denormalScale( const QtRegion& r )
      { top_right.denormalScale(r); bottom_left.denormalScale(r); }
   void scale( const Point2f& scalevec )
      { top_right.scale(scalevec); bottom_left.scale(scalevec); }
   void offset( const Point2f& offset )
      { top_right += offset; bottom_left += offset; }
   Point2f getCentre() const
      { return Point2f( (bottom_left.x + top_right.x) / 2.0,
                        (bottom_left.y + top_right.y) / 2.0 ); }
   //
   bool contains ( const Point2f& p ) const
   { return (p.x > bottom_left.x && p.x < top_right.x && p.y > bottom_left.y && p.y < top_right.y); }
   bool contains_touch ( const Point2f& p ) const
   { return (p.x >= bottom_left.x && p.x <= top_right.x && p.y >= bottom_left.y && p.y <= top_right.y); }
   void encompass( const Point2f& p )
   { if (p.x < bottom_left.x) bottom_left.x = p.x; if (p.x > top_right.x) top_right.x = p.x;
     if (p.y < bottom_left.y) bottom_left.y = p.y; if (p.y > top_right.y) top_right.y = p.y; }
   //
   bool atZero() const
   { return bottom_left.atZero() || top_right.atZero(); }
   //
   Point2f getEdgeUPoint(const EdgeU& eu);
   EdgeU getCutEdgeU(const Point2f& inside, const Point2f& outside);
   //
   friend bool intersect_region(const QtRegion& a, const QtRegion& b, double tolerance);
   friend bool overlap_x(const QtRegion& a, const QtRegion& b, double tolerance);
   friend bool overlap_y(const QtRegion& a, const QtRegion& b, double tolerance);
   //
   // set functions
   friend QtRegion runion(const QtRegion& a, const QtRegion& b);
   friend QtRegion rintersect( const QtRegion& a, const QtRegion& b); // undefined?
   //
   void grow(const double scalar)
   { Point2f dim = top_right - bottom_left;
     dim.scale(scalar - 1.0);
     top_right += dim;
     bottom_left -= dim; }
};

// First time we have a region available to use...
inline void Point2f::normalScale( const QtRegion& r )
{
   if(r.width()) x = (x - r.bottom_left.x) / r.width();
   else x = 0.0;
   if(r.height()) y = (y - r.bottom_left.y) / r.height();
   else y = 0.0;
}

inline void Point2f::denormalScale( const QtRegion& r )
{
   x = x * r.width() + r.bottom_left.x;
   y = y * r.height() + r.bottom_left.y;
}

// Lines are stored left to right as regions,
// the parity tells us whether the region should be inverted
// top to bottom to get the line

class Line : public QtRegion
{
protected:
   struct Bits {
      Bits() : x_dummy(0), y_dummy(0), z_dummy(0){}
      char parity      : 8;  // 1 ... positive, 0 ... negative
      char direction   : 8;  // 1 ... positive, 0 ... negative

      // dummy variables as it seems to be necessary that the width of this struct is 8 bytes
      // and I don't want any uninitialised memory that gets written to file accidentally
      char x_dummy : 8;
      char y_dummy : 8;
      int  z_dummy : 32;
   };
   Bits bits;
public:
   Line();
   Line(const Point2f& a, const Point2f& b);
   Line(const QtRegion& r) : QtRegion(r)
      { bits.parity = 1; bits.direction = 1; }
   Line(const Line& l) : QtRegion(l)
      { bits = l.bits; }
   Line& operator = (const Line& l)
      { this->QtRegion::operator = (l); bits = l.bits; return *this; }
   //
   friend bool intersect_line(const Line& a, const Line& b, double tolerance);
   friend bool intersect_line_no_touch(const Line& a, const Line& b, double tolerance);
   friend int  intersect_line_distinguish(const Line& a, const Line& b, double tolerance);
   friend int  intersect_line_b(const Line& a, const Line& b, double tolerance);
   //
   // fills in the location along the axis where the intersection happens
   bool intersect_line(const Line& l, int axis, double& loc) const;
   double intersection_point(const Line& l, int axis, double tolerance = 0.0) const;
   // this converts a loc retrieved from intersect line or intersection point back into a point:
   Point2f point_on_line(double loc, int axis) const;
   // ...and a quick do it all in one go:
   friend Point2f intersection_point(const Line& a, const Line& b, double tolerance);
   //
   bool crop(const QtRegion& r);
   void ray(short dir, const QtRegion& r);
   //
   friend double dot(const Line& a, const Line& b);
   //
   double ax() const { return bottom_left.x; }
   double& ax() { return bottom_left.x; }
   double bx() const { return top_right.x; }
   double& bx() { return top_right.x; }
   double ay() const { return bits.parity ? bottom_left.y : top_right.y; }
   double& ay() { return bits.parity ? bottom_left.y : top_right.y; }
   double by() const { return bits.parity ? top_right.y : bottom_left.y; }
   double& by() { return bits.parity ? top_right.y : bottom_left.y; }
   //
   const Point2f start() const
      { return Point2f( bottom_left.x, (bits.parity ? bottom_left.y : top_right.y) ); }
   const Point2f end() const
      { return Point2f( top_right.x,   (bits.parity ? top_right.y : bottom_left.y) ); }
   const Point2f midpoint() const
      { return Point2f( (start() + end()) / 2); }
   //
   // helpful to have a user friendly indication of direction:
   bool rightward() const
      { return bits.direction == 1; }
   bool upward() const
      { return bits.direction == bits.parity; }
   //
   const Point2f t_start() const
      { return Point2f( (rightward() ? bottom_left.x : top_right.x), (upward() ? bottom_left.y : top_right.y) ); }
   const Point2f t_end() const
      { return Point2f( (rightward() ? top_right.x : bottom_left.x), (upward() ? top_right.y : bottom_left.y) ); }
   //
   short sign() const
      { return bits.parity ? 1 : -1; }
   //
   double grad(int axis) const {
      return (axis == YAXIS) ? sign() * height() / width() : sign() * width() / height();
   }
   double constant(int axis) const {
      return (axis == YAXIS) ? ay() - grad(axis) * ax() : ax() - grad(axis) * ay();
   }
   //
   double length() const
      { return (double) sqrt((top_right.x - bottom_left.x) * (top_right.x - bottom_left.x) +
                             (top_right.y - bottom_left.y) * (top_right.y - bottom_left.y)); }
   //
   short direction() const
      { return bits.direction; }
   Point2f vector() const
      { return t_end() - t_start(); }
};

inline Point2f intersection_point(const Line& a, const Line& b, double tolerance)
{
   int axis = (a.width() >= a.height()) ? XAXIS : YAXIS;
   return a.point_on_line(a.intersection_point(b,axis,tolerance),axis);
}

////////////////////////////////////////////////////////////////////////////////////////

struct TaggedLine
{
   Line line;
   int tag;
   TaggedLine(const Line& l = Line(),int t = -1) { line = l; tag = t; }
};

// plain 2-point line without regions
struct SimpleLine
{
public:
    SimpleLine(const Line& line)
    {
        m_start.x = line.start().x;
        m_start.y = line.start().y;
        m_end.x = line.end().x;
        m_end.y = line.end().y;
    }
    SimpleLine(const Point2f& a, const Point2f& b)
    {
        m_start.x = a.x;
        m_start.y = a.y;
        m_end.x = b.x;
        m_end.y = b.y;
    }
    SimpleLine(double x1, double y1, double x2, double y2)
    {
        m_start.x = x1;
        m_start.y = y1;
        m_end.x = x2;
        m_end.y = y2;
    }
    const Point2f& start() const { return m_start; }
    const Point2f& end() const { return m_end; }
private:
    Point2f m_start;
    Point2f m_end;
};

////////////////////////////////////////////////////////////////////////////////////////

// not sure if this code is used any more:

// Now the difficult bit: making the line segments into polygons...
// The polygons are stored in a tree format so that intersection testing is easier

class RegionTree
{
friend class Poly;
protected:
   Line       *m_p_region;
   RegionTree *m_p_left;
   RegionTree *m_p_right;
public:
   RegionTree()
      { m_p_region = NULL; m_p_left = this; m_p_right = this; }
   virtual ~RegionTree()
      { if (m_p_region) delete m_p_region; }
   //
   virtual bool is_leaf() const = 0;
   //
   RegionTree& left() const
      { return *m_p_left; }
   RegionTree& right() const
      { return *m_p_right; }
   //
   operator QtRegion() const
      { return *(QtRegion *)m_p_region; }
   operator Line() const
      { return *(Line *)m_p_region; }
   //
   friend bool intersect(const RegionTree& a, const RegionTree& b);
   friend bool subintersect(const RegionTree& a, const RegionTree& b);
   friend int  intersections(const RegionTree& a, const Line& b);
};

// Branch on a region tree...

class RegionTreeBranch : public RegionTree
{
public:
   RegionTreeBranch() : RegionTree() {;}
   RegionTreeBranch( const Line& r,
                     const RegionTree& a,
                     const RegionTree& b )
   {
      m_p_left   = (RegionTree *) &a;
      m_p_right  = (RegionTree *) &b;
      m_p_region = new Line(r);  // copy
   }
   virtual bool is_leaf() const
      { return false; }
};

// Leaf on a region tree...

class RegionTreeLeaf : public RegionTree
{
public:
   RegionTreeLeaf() : RegionTree() {;}
   RegionTreeLeaf(const Line &l)
   {
      // no subnodes (but nice recursive properties)
      m_p_left = this;
      m_p_right = this;
      m_p_region = new Line(l);
   }
   virtual bool is_leaf() const
      { return true; }
};

class Poly
{
protected:
   int m_line_segments;
   RegionTree *m_p_root;
public:
   Poly()
   {
      m_p_root = NULL; m_line_segments = 0;
   }
   Poly( const Poly& p )
   {
      m_line_segments = p.m_line_segments;
      m_p_root = copy_region_tree( p.m_p_root );
   }
   Poly& operator = (const Poly& p)
   {
      if (this != &p) {
         m_line_segments = p.m_line_segments;
         m_p_root = copy_region_tree( p.m_p_root );
      }
      return *this;
   }
   virtual ~Poly()
   {
      destroy_region_tree();
   }
   // essentially, the copy constructor...
   RegionTree *copy_region_tree( const RegionTree* tree );
   // essentially, the destructor...
   void destroy_region_tree();
   //
   RegionTree& get_region_tree() const
   {
      return *m_p_root;
   }
   //
   void add_line_segment(const Line& l);
   //
   int get_line_segments()
      { return m_line_segments; }
   QtRegion get_bounding_box()
      { return *(QtRegion *)(m_p_root->m_p_region); }
   //
   bool contains( const Point2f& p );
   friend bool intersect( const Poly& a, const Poly& b );
};

#endif
