#pragma once

#include "mgraph440/spacepix.h"
#include "mgraph440/point.h"
#include "mgraph440/pixelref.h"
#include "mgraph440/attributes.h"

namespace mgraph440 {

class PointMap : public PixelBase
{
public:
    enum { NO_SELECTION = 0, SINGLE_SELECTION = 1, COMPOUND_SELECTION = 2, LAYER_SELECTION = 4, OVERRIDE_SELECTION = 8 };

    bool m_processed;
    bool m_boundarygraph;
    std::string m_name;
    SuperSpacePixel *m_spacepix;
    Point2f m_bottom_left;
    double m_spacing;
    mutable int m_displayed_attribute;
    AttributeTable m_attributes;
    int m_point_count;
    bool m_hasIsovistAnalysis = false;
    int m_selection;
    bool m_pinned_selection;
    bool m_initialised;
    bool m_blockedlines;
    mutable DisplayParams m_display_params;
    std::set<int> m_selection_set;      // n.b., m_selection_set stored as int for compatibility with other map layers

    Point **m_points;    // will contain the graph reference when created
    std::vector<PixelRefPair> m_merge_lines;

    Point& getPoint(const PixelRef& p) const
       { return m_points[p.x][p.y]; }
    bool setSpacePixel(const SuperSpacePixel *spacepix)
    {m_spacepix = (SuperSpacePixel *) spacepix; return true;}
    bool read( ifstream& stream, int version );
    Point2f depixelate( const PixelRef& p, double scalefactor = 1.0 ) const;   // Inlined below
    void convertAttributes( int which_attributes );
    void addGridConnections(); // adds grid connections where graph does not include them
    void setDisplayedAttribute( int col );
    PixelRef pixelate(const Point2f &p, bool constrain, int scalefactor) const;
    AttributeTable& getAttributeTable()
    { return m_attributes; }
    bool overrideSelPixel(PixelRef pix);
    double getSpacing() const
    { return m_spacing; }
    bool clearSel(); // clear the current selection
    // Note: passed by ref, use with care in multi-threaded app
    const std::set<int>& getSelSet() const
       { return m_selection_set; }
    bool write(ostream &stream, int version );
};

inline Point2f PointMap::depixelate( const PixelRef& p, double scalefactor ) const
{
   return Point2f( m_bottom_left.x + m_spacing * scalefactor * double(p.x),
                   m_bottom_left.y + m_spacing * scalefactor * double(p.y) );
}

class PointMaps : public prefvec<PointMap>
{
public:
   int m_displayed_map;
   SuperSpacePixel *m_spacepix;
   pmap<int,int> m_data_objects; // deprecated: (first int is data layer -- presumably the KEY not the index, second int is object ref)

   void setDisplayedPointMapRef(int i)
   { m_displayed_map = i; }
   PointMap& getDisplayedPointMap()
   { return at(m_displayed_map); }
   bool overrideSelPixel(PixelRef pix);    // set a pixel to selected: careful!
   void setSpacePixel(SuperSpacePixel *spacepix)
   { m_spacepix = spacepix; for (size_t i = 0; i < size(); i++) at(i).setSpacePixel(spacepix); }
   int getDataObject( int layer ) {
      size_t var = m_data_objects.searchindex( layer );
      if (var != paftl::npos)
         return m_data_objects.at(var);
      return -1;  // note: not paftl::npos
   }
   bool read(ifstream& stream, int version)
   {
      stream.read((char *) &m_displayed_map, sizeof(m_displayed_map));
      int count;
      stream.read((char *) &count, sizeof(count));
      for (int i = 0; i < count; i++) {
         push_back(PointMap());
         tail().setSpacePixel( (SuperSpacePixel *) this );
         tail().read( stream, version );
      }
      return true;
   }
   bool write(ostream &stream, int version, bool displayedmaponly = false );
};

}
