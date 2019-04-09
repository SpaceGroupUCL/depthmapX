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

#include "salalib/displayparams.h"
#include "genlib/p2dpoly.h"

// For my colour scheme... some parameters to pass, and my own colour class

// Converts everything to safe HTML colours

struct PafColor
{
   unsigned int m_color;
   unsigned char redb() const
   { return (unsigned char) (m_color >> 16); }
   unsigned char greenb() const
   { return (unsigned char) (m_color >> 8); }
   unsigned char blueb() const
   { return (unsigned char) (m_color);  }
   unsigned char alphab() const
   { return (unsigned char) (m_color >> 24); }
   // Quick mod - TV
   void setr(unsigned char r)
   { m_color &= 0xff00ffff; m_color |= (((unsigned int)r) << 16);}
   // Quick mod - TV
   void setg(unsigned char g)
   { m_color &= 0xffff00ff; m_color |= (((unsigned int)g) << 8);}
   // Quick mod - TV
   void setb(unsigned char b)
   { m_color &= 0xffffff00; m_color |= ((unsigned int)b);}
   float redf() const
   { return float(redb()) / 255.0f; }
   float greenf() const
   { return float(greenb()) / 255.0f; }
   float bluef() const
   { return float(blueb()) / 255.0f; }
   PafColor()
   { m_color = 0x00000000; }
   PafColor(unsigned int rgb) // color in 0x00rrggbb format
   { m_color = 0xff000000 | rgb; }
   PafColor( double r, double g, double b, double a = 1.0 )
   {
      m_color = 0x00000000 |
      (((unsigned char) (a * 255.0)) << 24) |
      (((unsigned char) (r * 255.0)) << 16) |
      (((unsigned char) (g * 255.0)) << 8) |
      (((unsigned char) (b * 255.0)));
   }

   PafColor( const Point2f& vec, double a = 1.0 )
   {
       m_color = 0x00000000 |
      (((unsigned char) (a * 255.0)) << 24) |
      (((unsigned char) (dot(vec,Point2f(1.0, 0.0)) * 255.0)) << 16) |
      (((unsigned char) (dot(vec,Point2f(-0.5,0.86602540378443864676372317075294)) * 255.0)) << 8) |
      (((unsigned char) (dot(vec,Point2f(-0.5,-0.86602540378443864676372317075294)) * 255.0)));
   }

   operator unsigned int ()
      { return m_color & 0x00ffffff; }
   friend bool operator == (const PafColor& a, const PafColor& b);
   friend bool operator != (const PafColor& a, const PafColor& b);
   PafColor& makeAxmanesque( double field);
   PafColor& makeHueOnlyAxmanesque( double field);
   PafColor& makePurpleOrange( double field );
   PafColor& makeBlueRed( double field );
   PafColor& makeGreyScale( double field );
   PafColor& makeMonochrome( double field );
   PafColor& makeDepthmapClassic( double field, double blue, double red );
   PafColor& makeColor(double field, DisplayParams dp);  // <- note, make copy to play around with
};
inline bool operator == (const PafColor& a, const PafColor& b)
{
   return (a.m_color == b.m_color);
}
inline bool operator != (const PafColor& a, const PafColor& b)
{
   return (a.m_color != b.m_color);
}
