#pragma once

#include "mgraph440/shapemap.h"
#include "mgraph440/pafcolor.h"
#include "mgraph440/paftl.h"
#include "mgraph440/p2dpoly.h"

namespace mgraph440 {

struct LineTest {
   Line line;
   unsigned int test;
   LineTest(const Line& l = Line(), int t = -1)
   { line = l; test = t; }
};

class SpacePixel : public PixelBase
{
   friend class PointMap;
   friend class AxialMaps;
   friend class AxialPolygons;
   friend class ShapeMap; // for transfer to everything being ShapeMaps
public:
//protected:
   PafColor m_color;
   std::string m_name;
   bool m_show;
   bool m_edit;
   pvecint **m_pixel_lines;
   int m_ref;
   pmap<int,LineTest> m_lines;
   //
   // for screen drawing
   mutable int *m_display_lines;
   PixelRef pixelate( const Point2f& p, bool constrain = true, int = 1 ) const;
   const pmap<int,LineTest>& getAllLines() const // Danger! Use solely to look at the raw line data
      { return m_lines; }
   std::string getName()
      { return m_name; }
   virtual bool read( std::ifstream& stream, int version );
};

// simply check they are the same name... useful for findindex from the group
inline bool operator == (const SpacePixel& a, const SpacePixel& b)
{
   return a.m_name == b.m_name;
}

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
#if !defined(_MSC_VER)
   size_t size() const
   { return pmemvec<T>::size(); }

   T& at(size_t pos)
   { return  prefvec<T>::at(pos); }

   T& tail()
   { return prefvec<T>::tail(); }

#endif
#endif

public:
   bool read(std::ifstream& stream, int version, bool = true);
   bool write(std::ostream &stream, int version );
};

template <class T>
bool SpacePixelGroup<T>::read( std::ifstream& stream, int version, bool )
{
   if (version >= VERSION_SPACEPIXELGROUPS) {
      m_name = dXstring440::readString(stream);
      stream.read( (char *) &m_region, sizeof(m_region) );
      int count;
      stream.read( (char *) &count, sizeof(count) );
      for (int i = 0; i < count; i++) {
         SpacePixelGroup<T>::push_back(T());
         prefvec<T>::tail().read(stream,version,true);
      }
   }
   else {
      m_name = "<unknown>";
      SpacePixelGroup<T>::push_back(T());
      prefvec<T>::tail().read(stream,version,true);
      m_region = prefvec<T>::tail().getRegion();
   }
   if (m_name.empty()) {
      m_name = "<unknown>";
   }
   return true;
}

template <class T>
bool SpacePixelGroup<T>::write( std::ostream& stream, int version )
{
   dXstring440::writeString(stream, m_name);
   stream.write( (char *) &m_region, sizeof(m_region) );

   // Quick mod - TV
   int count = prefvec<T>::size();
   stream.write( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
      prefvec<T>::at(i).write(stream,version);
   }
   return true;
}
typedef SpacePixelGroup < ShapeMap> SpacePixelFile;
typedef SpacePixelGroup<SpacePixelFile > SuperSpacePixel;

}
