#pragma once

#include "mgraph440/p2dpoly.h"

namespace mgraph440 {

class PixelRef
{
public:
   short x = 0;
   short y = 0;
   PixelRef( short ax = -1, short ay = -1 )
      { x = ax; y = ay; }
   PixelRef( int i )
      { x = short(i >> 16); y = short(i & 0xffff); }
   bool empty()
      { return x == -1 && y == -1; }
   PixelRef up() const
      { return PixelRef(x, y + 1); }
   PixelRef left() const
      { return PixelRef(x - 1, y); }
   PixelRef right() const
      { return PixelRef(x + 1, y); }
   PixelRef down() const
      { return PixelRef(x, y - 1); }
   short& operator [] (int i)
      { return (i == XAXIS) ? x : y; }
   bool within( const PixelRef bl, const PixelRef tr ) const
      { return (x >= bl.x && x <= tr.x && y >= bl.y && y <= tr.y); }
   bool encloses( PixelRef testpoint ) const
      { return (testpoint.x >= 0 && testpoint.x < x && testpoint.y >= 0 && testpoint.y < y);}
   // directions for the ngraph:
   enum {NODIR = 0x00, HORIZONTAL = 0x01, VERTICAL = 0x02, POSDIAGONAL = 0x04, NEGDIAGONAL = 0x08, DIAGONAL = 0x0c, NEGHORIZONTAL = 0x10, NEGVERTICAL = 0x20};
   short& row(char dir)
      { return (dir & VERTICAL) ? x : y; }
   short& col(char dir)
      { return (dir & VERTICAL) ? y : x; }
   const short& row(char dir) const
      { return (dir & VERTICAL) ? x : y; }
   const short& col(char dir) const
      { return (dir & VERTICAL) ? y : x; }
   PixelRef& move(char dir)
      { switch (dir)
        {
           case POSDIAGONAL: x++; y++; break;
           case NEGDIAGONAL: x++; y--; break;
           case HORIZONTAL: x++; break;
           case VERTICAL: y++; break;
           case NEGHORIZONTAL: x--; break;
           case NEGVERTICAL: y--; break;
        }
        return *this; }
   friend bool operator == (const PixelRef a, const PixelRef b);
   friend bool operator != (const PixelRef a, const PixelRef b);
   friend bool operator < (const PixelRef a, const PixelRef b);
   friend bool operator > (const PixelRef a, const PixelRef b);
   friend PixelRef operator + (const PixelRef a, const PixelRef b);
   friend PixelRef operator - (const PixelRef a, const PixelRef b);
   friend PixelRef operator / (const PixelRef a, const int factor);
   friend double dist(const PixelRef a, const PixelRef b);
   friend double angle(const PixelRef a, const PixelRef b, const PixelRef c);
   operator int() const
   { return ((int(x) << 16) + (int(y) & 0xffff)); }
};

const PixelRef NoPixel( -1, -1 );
typedef std::vector<PixelRef> PixelRefVector;


inline bool operator == (const PixelRef a, const PixelRef b)
{
   return (a.x == b.x) && (a.y == b.y);
}
inline bool operator != (const PixelRef a, const PixelRef b)
{
   return (a.x != b.x) || (a.y != b.y);
}
inline bool operator < (const PixelRef a, const PixelRef b)
{
   return (a.x < b.x) || (a.x == b.x && a.y < b.y);
}
inline bool operator > (const PixelRef a, const PixelRef b)
{
   return (a.x > b.x) || (a.x == b.x && a.y > b.y);
}
inline PixelRef operator + (const PixelRef a, const PixelRef b)
{
   return PixelRef(a.x + b.x, a.y + b.y);
}
inline PixelRef operator - (const PixelRef a, const PixelRef b)
{
   return PixelRef(a.x - b.x, a.y - b.y);
}
inline PixelRef operator / (const PixelRef a, const int factor)
{
   return PixelRef(a.x / factor, a.y / factor);
}


struct PixelRefPair
{
   PixelRef a;
   PixelRef b;
   PixelRefPair(const PixelRef x = NoPixel, const PixelRef y = NoPixel)
   {
      a =  x < y ? x : y;
      b =  x < y ? y : x;
   }
   friend bool operator == (const PixelRefPair& x, const PixelRefPair& y);
   friend bool operator != (const PixelRefPair& x, const PixelRefPair& y);
   friend bool operator <  (const PixelRefPair& x, const PixelRefPair& y);
   friend bool operator >  (const PixelRefPair& x, const PixelRefPair& y);

};

// note: these are made with a is always less than b
inline bool operator == (const PixelRefPair& x, const PixelRefPair& y)
{
   return (x.a == y.a && x.b == y.b);
}
inline bool operator != (const PixelRefPair& x, const PixelRefPair& y)
{
   return (x.a != y.a || x.b != y.b);
}
inline bool operator < (const PixelRefPair& x, const PixelRefPair& y)
{
   return ( (x.a == y.a) ? x.b < y.b : x.a < y.a );
}
inline bool operator > (const PixelRefPair& x, const PixelRefPair& y)
{
   return ( (x.a == y.a) ? x.b > y.b : x.a > y.a );
}

}
