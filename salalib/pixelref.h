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

#include <vector>

class PixelRef
{
public:
   short x;
   short y;
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
   bool isodd()
   { return x % 2 == 1 && y % 2 == 1; }
   bool iseven()
   { return x % 2 == 0 && y % 2 == 0; }
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

inline double dist(const PixelRef a, const PixelRef b)
{
   return sqrt(sqr(a.x - b.x) + sqr(a.y - b.y));
}

inline double angle(const PixelRef a, const PixelRef b, const PixelRef c)
{
   if (c == NoPixel) {
      return 0.0;
   }
   else {
      // n.b. 1e-12 required for floating point error
      return acos( double((a.x - b.x) * (b.x - c.x) + (a.y - b.y) * (b.y - c.y)) /
                   (sqrt(sqr(a.x - b.x) + sqr(a.y - b.y)) * sqrt(sqr(b.x - c.x) + sqr(b.y - c.y)) + 1e-12) );
   }
}

// Now sizeof(PixelRef) == sizeof(int) better stored directly:
typedef std::vector<PixelRef> PixelRefVector;

/////////////////////////////////////////////////////////////////////////////////////////////////

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
