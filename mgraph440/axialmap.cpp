#include "mgraph440/mgraph_consts.h"
#include "mgraph440/axialmap.h"
#include "mgraph440/stringutils.h"

namespace mgraph440 {

bool ShapeGraph::read( ifstream& stream, int version )
{
   m_attributes.clear();
   m_connectors.clear();
   m_selection = false;
   m_map_type = ShapeMap::EMPTYMAP;

   // the old version used SpacePixel as base class
   // actually easiest to read and translate, rather than try to use new method
   if (version < VERSION_AXIAL_SHAPES) {
      readold(stream, version);
   }
   else {
      bool segmentmap = false;
      if (version < VERSION_MAP_TYPES) {
         // axial specific reads -- segment map flag and keyvertices (part of all line map functionality)
         // note, now stored in the "map_type", and read / written with shape map
         char segmentmapc = stream.get();
         if (segmentmapc == '1') {
            segmentmap = true;
         }
      }
      // note that keyvertexcount and keyvertices are different things! (length keyvertices not the same as keyvertexcount!)
      stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
      int size;
      stream.read((char *)&size,sizeof(size));
      for (int i = 0; i < size; i++) {
         m_keyvertices.push_back(pvecint());
         m_keyvertices[i].read(stream);
      }
      // now base class read:
      ShapeMap::read(stream,version);
      //
      // override shapemap map type designation if necessary:
      if (version < VERSION_MAP_TYPES) {
         if (segmentmap) {
            m_map_type = ShapeMap::SEGMENTMAP;
         }
         else {
            m_map_type = ShapeMap::AXIALMAP;
         }
      }
   }

   return true;
}

bool ShapeGraph::readold( ifstream& stream, int version )
{
   // read in from old base class
   SpacePixel linemap;
   linemap.read(stream, version);
   const pmap<int,LineTest>& lines = linemap.getAllLines();

   m_name = linemap.getName();

   // now copy to new base class:
   init(lines.size(),linemap.getRegion());
   for (size_t i = 0; i < lines.size(); i++) {
      makeLineShape(lines[i].line);
   }
   // n.b., we now have to reclear attributes!
   m_attributes.clear();

   // continue old read:
   int pushmap = -1;
   if (version >= VERSION_SEGMENT_MAPS) {
      char segmentmapc = stream.get();
      if (segmentmapc == '1') {
         m_map_type = ShapeMap::SEGMENTMAP;
      }
      else {
         m_map_type = ShapeMap::AXIALMAP;
      }
   }
   if (version >= VERSION_GATE_MAPS) {
      char gatemapc = stream.get();
      if (gatemapc == '1') {
         m_map_type = ShapeMap::DATAMAP;
      }
      stream.read((char *)&pushmap,sizeof(pushmap));
   }

   int displayed_attribute;  // n.b., temp variable necessary to force recalc below
   stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));

   m_attributes.read(stream,version);
   int size;
   stream.read((char *)&size,sizeof(size));
   for (int j = 0; j < size; j++) {
      m_keyvertices.push_back(pvecint());    // <- these were stored with the connector
      int key;
      stream.read((char *)&key,sizeof(key)); // <- key deprecated
      m_connectors.push_back(Connector());
      m_connectors[j].read(stream,version,&(m_keyvertices[j]));
   }
   stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));

   if (version >= VERSION_AXIAL_LINKS) {
      m_links.read(stream);
      m_unlinks.read(stream);
   }
   // some miscellaneous extra data for mapinfo files
   if (m_mapinfodata) {
      delete m_mapinfodata;
      m_mapinfodata = NULL;
   }
   if (version >= VERSION_MAPINFO_DATA) {
      char x = stream.get();
      if (x == 'm') {
         m_mapinfodata = new MapInfoData;
         m_mapinfodata->read(stream,version);
      }
   }

   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

bool ShapeGraphs::read( ifstream& stream, int version )
{
   // base class read
   if (version >= VERSION_AXIAL_SHAPES) {
      ShapeMaps<ShapeGraph>::read(stream,version);
   }
   else {
      readold(stream,version);
   }

   // these are additional essentially for all line axial maps
   // should probably be kept *with* the all line axial map...
   m_poly_connections.clear();
   m_poly_connections.read(stream);
   m_radial_lines.clear();
   m_radial_lines.read(stream);

   // this is an index to look up the all line map, used by UI to determine if can make fewest line map
   // note: it is not saved for historical reasons
   // will get confused by more than one all line map
   m_all_line_map = getMapRef("All-Line Map");
   if (m_all_line_map == -1) {
      // used to be called All Line Map
      m_all_line_map = getMapRef("All Line Map");
   }
   if (m_all_line_map != -1) {
      at(m_all_line_map).m_map_type = ShapeMap::ALLLINEMAP;
   }

   // VERSION_AXIAL_REGION_FIX -- this fix is now deprecated
   // some awful things could have gone wrong in the past, but the shapemap read should fix automatically

   return true;
}

// for backward compatibility only:
bool ShapeGraphs::readold( ifstream& stream, int version )
{
   // this read is based on SpacePixelGroup<ShapeGraph>::read(stream, version);
   dXstring440::readString(stream);
   QtRegion dummyregion;
   stream.read( (char *) &dummyregion, sizeof(dummyregion) );
   int count;
   stream.read( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
      push_back(ShapeGraph());
      tail().read(stream,version);
   }
   stream.read((char *)&m_displayed_map,sizeof(m_displayed_map));

   return true;
}

bool ShapeGraphs::write(ostream &stream, int version, bool displayedmaponly )
{
   // base class write
   ShapeMaps<ShapeGraph>::write(stream, version, displayedmaponly);

   m_poly_connections.write(stream);
   m_radial_lines.write(stream);

   return true;
}

bool ShapeGraph::write( ostream& stream, int version )
{
   // note keyvertexcount and keyvertices are different things!  (length keyvertices not the same as keyvertexcount!)
   stream.write((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
   int size = m_keyvertices.size();
   stream.write((char *)&size,sizeof(size));
   for (size_t i = 0; i < m_keyvertices.size(); i++) {
      m_keyvertices[i].write(stream);
   }

   // now simply run base class write:
   ShapeMap::write(stream,version);

   return true;
}

AxialPolygons::AxialPolygons()
{
   m_pixel_polys = NULL;
}

AxialPolygons::~AxialPolygons()
{
   if (m_pixel_polys) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_polys[i];
      }
      delete [] m_pixel_polys;
      m_pixel_polys = NULL;
   }
}

ShapeGraph::ShapeGraph(const std::string& name, int type) : ShapeMap(name,type)
{
   m_keyvertexcount = 0;
   m_hasgraph = true;
}

}
