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


// This is my code to make a set of axial lines from a set of boundary lines

#ifndef __SPATIALDATA_H__
#define __SPATIALDATA_H__

/////////////////////////////////////////////////////////////////////////////////////////////////
#include "genlib/pafmath.h"
#include "genlib/p2dpoly.h"

#include "genlib/stringutils.h"
#include "salalib/pixelref.h"
#include "salalib/pafcolor.h"
#include "genlib/paftl.h"
#include <map>

class SalaShape;

class PixelBase
{
protected:
   int m_rows;
   int m_cols;
   QtRegion m_region;
public:
   PixelBase() {;}
   // constrain is constrain to bounding box (i.e., in row / col bounds)
   virtual PixelRef pixelate(const Point2f&, bool constrain = true, int scalefactor = 1 ) const = 0;
   PixelRefVector pixelateLine( Line l, int scalefactor = 1 ) const;
   PixelRefVector pixelateLineTouching( Line l, double tolerance ) const;
   PixelRefVector quickPixelateLine(PixelRef p, PixelRef q);
   bool includes(const PixelRef pix) const {
      return (pix.x >= 0 && pix.x < m_cols && pix.y >= 0 && pix.y < m_rows);
   }
   int getCols() const {
      return m_cols;
   }
   int getRows() const {
      return m_rows;
   }
   const QtRegion& getRegion() const {
      return m_region;
   }
};

/////////////////////////////////////////////

// couple of quick helpers

struct LineTest {
   Line line;
   unsigned int test;
   LineTest(const Line& l = Line(), int t = -1)
   { line = l; test = t; }
// operator Line() {return line;}
};
/*
struct LineTest : public Line 
{
public:
   // Inside / outside system... sides refer to the side hit when line runs from A to B
   enum { NEITHER = 0, LEFT = 1, RIGHT = 2 };
   unsigned short inside;
   //
   unsigned int test;
public:
   LineTest(const Line& l = Line(), int t = -1) : Line(l)
   { test = t; inside = NEITHER; }
};
*/
struct LineKey {
   unsigned int file : 4;
   unsigned int layer : 6;
   unsigned int lineref : 20;
   operator int() { return *(int *)this; }
   LineKey(int value = 0) { *(int *)this = value; }
   friend bool operator < (LineKey a, LineKey b);
   friend bool operator > (LineKey a, LineKey b);
   friend bool operator == (LineKey a, LineKey b);
};
inline bool operator < (LineKey a, LineKey b)
{ return int(a) < int(b); }
inline bool operator > (LineKey a, LineKey b)
{ return int(a) > int(b); }
inline bool operator == (LineKey a, LineKey b)
{ return int(a) == int(b); }

/////////////////////////////////////////////

class SpacePixel : public PixelBase
{
   friend class PointMap;
   friend class AxialMaps;
   friend class AxialPolygons;
   friend class ShapeMap; // for transfer to everything being ShapeMaps
protected:
   bool m_lock;
   mutable bool m_newline;
protected:
   PafColor m_color;
   int m_style;   // allows for bold / dotted lines etc
   std::string m_name;
   bool m_show;
   bool m_edit;
   pvecint **m_pixel_lines;
//   int m_rows;
//   int m_cols;
//
//   double m_pixel_height;
//   double m_pixel_width;
   //
   int m_ref;
   std::map<int,LineTest> m_lines;
   //
   // for screen drawing
   mutable int *m_display_lines;
   mutable int m_current;
   //
   // for line testing
   mutable unsigned int m_test;
   //
public:
   SpacePixel(const std::string& name = std::string("Default"));
   virtual ~SpacePixel();
   //
   SpacePixel(const SpacePixel& spacepixel);
   SpacePixel& operator = (const SpacePixel& spacepixel);
   void construct(const SpacePixel& spacepixel);
   //
   PixelRef pixelate( const Point2f& p, bool constrain = true, int = 1 ) const;
//   PixelRefVector pixelate( const Line& l ) const;
   //
   void initLines(int size, const Point2f& min, const Point2f& max, double density = 1.0);
   void reinitLines(double density);   // just reinitialises pixel lines, keeps lines, current ref and test setting
   //
   void addLine(const Line& l);
   void sortPixelLines();
   //
   int addLineDynamic(const Line& l);

   // Quick mod - TV
#if defined(_WIN32)
   bool removeLineDynamic(int ref, Line& line = Line() ); // fills in line if it finds it
#else
   bool removeLineDynamic(int ref, Line& line); // fills in line if it finds it
#endif
   //
   virtual void makeViewportLines( const QtRegion& viewport ) const;
   virtual bool findNextLine(bool&) const;
   virtual const Line& getNextLine() const;
   //
   bool intersect(const Line& l, double tolerance = 0.0);
   bool intersect_exclude(const Line& l, double tolerance = 0.0);
   //
   // Point2f getFirstCrossingPoint(const Line& l, int fromend, pvecint& ignorelist = pvecint());
   void cutLine(Line& l, short dir);
   // Quick mod - TV
#if defined(_WIN32)
   pvecdouble getCrossingPoints(const Line& l, int ignore1, pvecint& ignorelist = pvecint() );
#else
   pvecdouble getCrossingPoints(const Line& l, int ignore1, pvecint& ignorelist);
#endif
   //
   QtRegion& getRegion() const
      { return (QtRegion&) m_region; }
   //
   const std::map<int,LineTest>& getAllLines() const // Danger! Use solely to look at the raw line data
      { return m_lines; }
   //
   // For easy layer manipulation:
   void setName(const std::string& name)
      { m_name = name; }
   std::string getName()
      { return m_name; }
   void setShow(bool show = true)
      { m_show = show; }
   bool isShown() const
      { return m_show; }
   void setEditable(bool edit = true)
      { m_edit = edit; }
   bool isEditable() const
      { return m_edit; }
public:
   // for screen drawing:
   PafColor getLineColor(bool in_color = true) const
      { return m_color; }
   int getLineStyle() const
      { return m_style; }
   const bool getLineSelected() const
      { return false; } // selection not enabled as yet for non-axial lines -- note: major change around when it happens
   void setColor(const PafColor& color)
      { m_color = color; }
   void setStyle(const int style)
      { m_style = style; }
   // just to know...
   int getLineCount() const
      { return (int)m_lines.size(); }
public:
   virtual bool read(istream &stream, int version );
   virtual bool write( ofstream& stream );
   friend bool operator == (const SpacePixel& a, const SpacePixel& b); 
};

// simply check they are the same name... useful for findindex from the group
inline bool operator == (const SpacePixel& a, const SpacePixel& b)
{
   return a.m_name == b.m_name;
}

// Two levels of space pixel layering (file, and layers in files)...

template <class T> 
class SpacePixelGroup : public pqvector<T>
{
protected:
   std::string m_name;   // <- file name
   mutable int m_current_layer;
public:
   QtRegion m_region;  // easier public for now
   //
   SpacePixelGroup(const std::string& name = std::string())
   { m_name = name; m_current_layer = -1; }
   void setName(const std::string& name)
   { m_name = name; }
   const std::string& getName() const
   { return m_name; }
   //
   QtRegion& getRegion() const
      { return (QtRegion&) m_region; }
   //
   // Screen functionality:
   void makeViewportShapes( const QtRegion& viewport = QtRegion() ) const;
   bool findNextShape(bool& nextlayer) const;

   // Quick mod - TV
#if 0
#if !defined(_WIN32)
   size_t size() const
   { return pmemvec<T>::size(); }

   T& at(size_t pos)
   { return  prefvec<T>::at(pos); }

   T& tail()
   { return prefvec<T>::tail(); }

#endif
#endif

   const SalaShape& getNextShape() const
      { return prefvec<T>::at(m_current_layer).getNextShape(); }
   const PafColor getLineColor() const
      { return prefvec<T>::at(m_current_layer).getLineColor(); }
   const int getLineStyle() const
      { return prefvec<T>::at(m_current_layer).getLineStyle(); }
   const bool getLineSelected() const
      { return  prefvec<T>::at(m_current_layer).getLineSelected(); }

   //
   void cutLine(Line& l);//, short dir);
   //
   // Is any one sublayer shown?

   bool isShown() const
      { for (size_t i = 0; i < prefvec<T>::size(); i++) if (prefvec<T>::at(i).isShown()) return true; return false; }
   //
public:
   bool read(std::istream &stream, int version, bool drawinglayer = true );
   bool write( std::ofstream& stream, int version );
};

template <class T> 
void SpacePixelGroup<T>::cutLine(Line& l)//, short dir)
{
   for (size_t i = 0; i < prefvec<T>::size(); i++) {
      if (prefvec<T>::at(i).isShown()) {
         prefvec<T>::at(i).cutLine(l); //,dir);
      }
   }
}

template <class T> 
void SpacePixelGroup<T>::makeViewportShapes( const QtRegion& viewport ) const
{
   m_current_layer = -1;
   for (size_t i = prefvec<T>::size() - 1; i != paftl::npos; i--) {
      if (prefvec<T>::at(i).isShown()) {
         m_current_layer = (int) i;
         prefvec<T>::at(i).makeViewportShapes( (viewport.atZero() ? m_region : viewport) );
      }
   }
}
template <class T>
bool SpacePixelGroup<T>::findNextShape(bool& nextlayer) const
{
   if (m_current_layer == -1) 
      return false;
   while (!prefvec<T>::at(m_current_layer).findNextShape(nextlayer)) {
      while (++m_current_layer < (int)prefvec<T>::size() && !prefvec<T>::at(m_current_layer).isShown());
      if (m_current_layer == prefvec<T>::size()) {
         m_current_layer = -1;
         return false;
      }
   }
   return true;
}
template <class T>
bool SpacePixelGroup<T>::read( istream& stream, int version, bool drawinglayer )
{
   m_name = dXstring::readString(stream);
   stream.read( (char *) &m_region, sizeof(m_region) );
   int count;
   stream.read( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
       SpacePixelGroup<T>::push_back(T());
       prefvec<T>::tail().read(stream,version,true);
   }

   if (m_name.empty()) {
      m_name = "<unknown>";
   }
   return true;
}
template <class T>
bool SpacePixelGroup<T>::write( ofstream& stream, int version )
{
   dXstring::writeString(stream, m_name);
   stream.write( (char *) &m_region, sizeof(m_region) );
   
   // Quick mod - TV
   int count = prefvec<T>::size();
   stream.write( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
      prefvec<T>::at(i).write(stream,version);
   }
   return true;
}

// (Currently) two layers of SpacePixeling, the file, and the whole lot (in SuperSpacePixel)
// (I aim to split this into a set of ShapeMaps with the version 9 layer system for each subset)
#include <salalib/attributes.h>
#include <salalib/shapemap.h>
typedef SpacePixelGroup < ShapeMap> SpacePixelFile;
//typedef SpacePixelGroup<SpacePixel> SpacePixelFile;
typedef SpacePixelGroup<SpacePixelFile > SuperSpacePixel;

#endif
