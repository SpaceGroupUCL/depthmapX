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



// this includes helpers for idepthmap

#include <time.h>

#include <genlib/paftl.h>
#include <salalib/mgraph.h>
#include <salalib/ngraph.h>

#include <salalib/idepthmap.h>
#include <salalib/idepthmapx.h>

//////////////////////////////////////////////////////////////////////

IGraphOrganizer::IGraphOrganizer()
{ 
   m_graph = NULL; 
   m_delete_flag = false;
   m_vga_map_cursor = -1;
   m_shape_map_cursor = -1;
}

IGraphOrganizer::~IGraphOrganizer()
{ 
   if (m_delete_flag) {
      delete m_graph;
   }
}

void IGraphOrganizer::setDeleteFlag()
{
   m_delete_flag = true;
}

IVGAMap *IGraphOrganizer::getFirstIVGAMap()
{
   m_vga_map_cursor = 0;
   IVGAMap *ret = getIVGAMap(m_vga_map_cursor);
   if (ret == NULL) {
      m_vga_map_cursor = -1;
   }
   return ret;
}

IVGAMap *IGraphOrganizer::getNextIVGAMap()
{
   m_vga_map_cursor++;
   IVGAMap *ret = getIVGAMap(m_vga_map_cursor);
   if (ret == NULL) {
      m_vga_map_cursor = -1;
   }
   return ret;
}

IVGAMap *IGraphOrganizer::getIVGAMap(PointMap *map)
{
   if (map == NULL) {
      return NULL;
   }
   size_t n = m_vga_maps.searchindex(map);
   if (n == paftl::npos) {
      n = m_vga_maps.add(map,IVGAMap(),paftl::ADD_HERE);
      m_vga_maps[n].setData((void *)map);
      m_ivga_maps.add(&(m_vga_maps[n]),map);   // also reverse lookup
   }
   return &(m_vga_maps[n]);
}

// INTERNAL ONLY -- others use cursor version or pointer version above
IVGAMap *IGraphOrganizer::getIVGAMap(int i)
{
   IVGAMap *p_ivga = NULL;
   if (i >= 0 && size_t(i) < m_graph->PointMaps::maps_vector.size()) {
      p_ivga = getIVGAMap(&(m_graph->PointMaps::maps_vector.at(i)));
   }
   return p_ivga;
}

IShapeMap *IGraphOrganizer::getFirstIShapeMap()
{
   m_shape_map_cursor = 0;
   IShapeMap *ret = getIShapeMap(m_shape_map_cursor);
   if (ret == NULL) {
      m_shape_map_cursor = -1;
   }
   return ret;
}

IShapeMap *IGraphOrganizer::getNextIShapeMap()
{
   m_shape_map_cursor++;
   IShapeMap *ret = getIShapeMap(m_shape_map_cursor);
   if (ret == NULL) {
      m_vga_map_cursor = -1;
   }
   return ret;
}

IShapeMap *IGraphOrganizer::getIShapeMap(ShapeMap *map)
{
   if (map == NULL) {
      return NULL;
   }
   int n = m_shape_maps.searchindex(map);
   if (n == -1) {
      n = m_shape_maps.add(map,IShapeMap(),paftl::ADD_HERE);
      m_shape_maps[n].setData((void *)map);
      m_ishape_maps.add(&(m_shape_maps[n]),map);   // also reverse lookup
   }
   return &(m_shape_maps[n]);
}


// INTERNAL ONLY -- others use cursor version above or by pointer version above
IShapeMap *IGraphOrganizer::getIShapeMap(int i) 
{
   IShapeMap *p_layer = NULL;
   int shape_graphs = m_graph->getShapeGraphs().getMapCount();
   int data_maps = m_graph->getDataMaps().getMapCount();
   if (i >= 0 && i < shape_graphs) {
      p_layer = getIShapeMap(&(m_graph->getShapeGraphs().getMap(i)));
   }
   else if (i >= shape_graphs && i < shape_graphs + data_maps) {
      p_layer = getIShapeMap(&(m_graph->getDataMaps().getMap(i-shape_graphs)));
   }
   return p_layer;
}

// PROTECTED: only use this internally in IGraphFile * 
int IGraphOrganizer::getMapRef(IShapeMap *map)
{
   size_t n = m_ishape_maps.searchindex(map);
   if (n == paftl::npos) {
      return -1;
   }
   ShapeMap *basemap = m_ishape_maps[n];
   //
   if (!map->isDataMap()) {
      // look for it...
      for (size_t i = 0; i < m_graph->m_shape_graphs.getMapCount(); i++) {
         if (&(m_graph->m_shape_graphs.getMap(i)) == basemap) {
            return i;
         }
      }
   }
   else {
      // look for it...
      for (size_t i = 0; i < m_graph->m_data_maps.getMapCount(); i++) {
         if (&(m_graph->m_data_maps.getMap(i)) == basemap) {
            return i;
         }
      }
   }
   return -1;
}

// PROTECTED: only use this internally in IGraphFile *
int IGraphOrganizer::getMapRef(IVGAMap *map)
{
   size_t n = m_ivga_maps.searchindex(map);
   if (n == paftl::npos) {
      return -1;
   }
   PointMap *basemap = m_ivga_maps[n];
   //
   // look for it...
   for (size_t i = 0; i < m_graph->PointMaps::maps_vector.size(); i++) {
      if (&(m_graph->PointMaps::maps_vector.at(i)) == basemap) {
         return i;
      }
   }
   return -1;
}

////////////////////////////////////////////////////////////////

// these three are tough as they all require adding maps into the lookup tables

IShapeMap *IGraphOrganizer::addShapeMap(const char *name, int type) 
{
   ShapeMap *p_map = NULL;
   IShapeMap *p_imap = NULL;

   if (type == IShapeMap::IDATAMAP) {
      int mapref = m_graph->addShapeMap(name);
      p_map = &(m_graph->getDataMaps().getMap(mapref));
   }
   else {
      // I'm assuming here that the new map type is already checked and allowable...
      int mapref = m_graph->addShapeGraph(name,type);
      p_map = &(m_graph->getShapeGraphs().getMap(mapref));
   }

   if (p_map) {
      p_imap = getIShapeMap(p_map);
      
      // don't know why, but it looks like we need to do this here:
      QtRegion r = m_graph->getBoundingBox();
      if (r.isNull()) {
         r = QtRegion(Point2f(-50.0,-50.0),Point2f(50.0,50.0));
      }
      p_map->init(0,r);
      p_map->setEditable(true);
   }

   return p_imap;
}

IShapeMap *IGraphOrganizer::makeAxialMapFromBaseMap(IComm *comm, IShapeMap *basemap, const char *newmapname)
{ 
   IShapeMap *newmap = NULL;

   size_t n = m_ishape_maps.searchindex(basemap);
   if (n == paftl::npos) {
      return NULL;
   }
   ShapeMap& truebasemap = *(m_ishape_maps[n]);

   bool copydata = true;

   try {
      int mapref = m_graph->getShapeGraphs().convertDataToAxial((Communicator *)(comm->getData()), newmapname, truebasemap, copydata);
      ShapeGraph *p_map = &(m_graph->getShapeGraphs().getMap(mapref));
      newmap = getIShapeMap(p_map);
   }
   catch (Communicator::CancelledException)
   {
	  newmap = NULL;
   }

   return newmap;
}


IShapeMap *IGraphOrganizer::makeSegmentMapFromAxialMap(IComm *comm, IShapeMap *axialmap, const char *newmapname, double stubremoval) 
{
   IShapeMap *newmap = NULL;
   int mapref = -1;

   bool keeporiginal = true;
   bool copydata = true;

   if (!axialmap->isAxialMap()) {
      return NULL;
   }

   // set the displayed map to the axial map (I know, it's all a bit UI focused...)
   int basemapref = getMapRef(axialmap);
   if (basemapref == -1) {
      return NULL;
   }
   m_graph->m_shape_graphs.setDisplayedMapRef( basemapref );

   try {
	   mapref = m_graph->m_shape_graphs.convertAxialToSegment((Communicator *)(comm->getData()), newmapname, keeporiginal, copydata, stubremoval);
      ShapeGraph *p_map = &(m_graph->getShapeGraphs().getMap(mapref));
      newmap = getIShapeMap(p_map);
   }
   catch (Communicator::CancelledException)
   {
	  newmap = NULL;
   }

   return newmap;
}

/////////////////////////////////////////////////////////////////////////////////////////

AttributeTable *IGraphOrganizer::getAttributeTable(IVGAMap *ivga) 
{
    AttributeTable *p_table = NULL;
    for (size_t i = 0; i < m_vga_maps.size(); i++) {
       if (&(m_vga_maps.value(i)) == ivga) {
          PointMap *p_map = m_vga_maps.key(i);
          p_table = &(p_map->getAttributeTable());
          break;
       }
    }
    return p_table;
}

AttributeTable *IGraphOrganizer::getAttributeTable(IShapeMap *ishape) 
{
    AttributeTable *p_table = NULL;
    for (size_t i = 0; i < m_shape_maps.size(); i++) {
       if (&(m_shape_maps.value(i)) == ishape) {
          ShapeMap *p_map = m_shape_maps.key(i);
          p_table = &(p_map->getAttributeTable());
          break;
       }
    }
    return p_table;
}

/////////////////////////////////////////////////////////////////////////////////////////


IAttributes *IGraphOrganizer::getIAttributes(AttributeTable *p_table, int analysis_type)
{
   IAttributes *p_attr = NULL;
   size_t n = m_attribute_tables.searchindex(p_table);
   if (n == paftl::npos) {
      n = m_attribute_tables.add(p_table,IAttributes(),paftl::ADD_HERE);
      m_attribute_tables[n].setData((void *)p_table, analysis_type);
   }
   p_attr = &(m_attribute_tables[n]);
   return p_attr;
}

IAttributes *IGraphOrganizer::getIAttributes(IVGAMap *ivga) 
{
    IAttributes *p_attr = NULL;
    for (size_t i = 0; i < m_vga_maps.size(); i++) {
       if (&(m_vga_maps.value(i)) == ivga) {
          PointMap *p_map = m_vga_maps.key(i);
          AttributeTable *p_table = &(p_map->getAttributeTable());
          p_attr = getIAttributes(p_table, DLL_VGA_ANALYSIS);
          break;
       }
    }
    return p_attr;
}

IAttributes *IGraphOrganizer::getIAttributes(IShapeMap *ishape) 
{
    IAttributes *p_attr = NULL;
    for (size_t i = 0; i < m_shape_maps.size(); i++) {
       if (&(m_shape_maps.value(i)) == ishape) {
          ShapeMap *p_map = m_shape_maps.key(i);
          AttributeTable *p_table = &(p_map->getAttributeTable());
          p_attr = getIAttributes(p_table, DLL_AXIAL_ANALYSIS | DLL_SEGMENT_ANALYSIS);
          break;
       }
    }
    return p_attr;
}

/////////////////////////////////////////////////////

int IGraphOrganizer::getViewOrder(IVGAMap *ivga)
{
   int order = -1;
   int view = m_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA);
   if (view != 0) { 
      size_t n = m_ivga_maps.searchindex(ivga);
      if (n == paftl::npos) {
         return -1;
      }
      else if (m_ivga_maps[n] == &(m_graph->getDisplayedPointMap())) {
         if (view == MetaGraph::VIEWVGA) {
            order = 0;
         }
         else {
            order = 1;
         }
      }
   }
   return order;
}
int IGraphOrganizer::getViewOrder(IShapeMap *ishape)
{
   int order = -1;
   if (!ishape->isDataMap()) { // assume everything else is a graph
      int view = m_graph->getViewClass() & (MetaGraph::VIEWAXIAL | MetaGraph::VIEWBACKAXIAL);
      if (view != 0) { 
         size_t n = m_ishape_maps.searchindex(ishape);
         if (n == paftl::npos) {
            return -1;
         }
         else if (m_ishape_maps[n] == &(m_graph->getDisplayedShapeGraph())) {
            if (view == MetaGraph::VIEWAXIAL) {
               order = 0;
            }
            else {
               order = 1;
            }
         }
      }
   }
   else {
      int view = m_graph->getViewClass() & (MetaGraph::VIEWDATA | MetaGraph::VIEWBACKDATA);
      if (view != 0) {
         size_t n = m_ishape_maps.searchindex(ishape);
         if (n == paftl::npos) {
            return -1;
         }
         else if (m_ishape_maps[n] == &(m_graph->getDisplayedDataMap())) {
            if (view == MetaGraph::VIEWDATA) {
               order = 0;
            }
            else {
               order = 1;
            }
         }
      }
   }
   return order;
}
