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


#ifndef __IDEPTHMAPX_H__
#define __IDEPTHMAPX_H__

///////////////////////////////////////////////////////////////////////////////////////
// Extra functionality specifically for the depthmapX DLL interface
// Helper for IGraphFile

class IGraphOrganizer {
   friend class IGraphFile;
protected:
   MetaGraph *m_graph;
private:
   // You MUST use pqmap here!
   // It is necessary so the IVGAMaps and IShapeMaps do not get reallocated, and that
   // both IVGAMap* and IShapeMap* remain valid after adding new maps
   pqmap<PointMap *, IVGAMap> m_vga_maps;
   pqmap<ShapeMap *, IShapeMap> m_shape_maps;
   pqmap<AttributeTable *, IAttributes> m_attribute_tables;
   // in addition to look up that way, we also need to look up the other way, because IShapeMap and IVGAMap won't allow us to get the data back!
   pqmap<IVGAMap *, PointMap *> m_ivga_maps;
   pqmap<IShapeMap *, ShapeMap *> m_ishape_maps;
   //
protected:
   bool m_delete_flag;
   int m_vga_map_cursor;
   int m_shape_map_cursor;
public:
   IGraphOrganizer();
   virtual ~IGraphOrganizer();
   void setDeleteFlag();
   //
public:
   //
   IShapeMap *addShapeMap(const char *name, int type);
   IShapeMap *makeAxialMapFromBaseMap(IComm *comm, IShapeMap *basemap, const char *newmapname);
   IShapeMap *makeSegmentMapFromAxialMap(IComm *comm, IShapeMap *axialmap, const char *newmapname, double stubremoval);
   //
   int getIVGAMapCount() const
   { return m_graph->PointMaps::maps_vector.size(); }
   IVGAMap *getFirstIVGAMap();
   IVGAMap *getNextIVGAMap();
   //
   // note shape graphs and data maps are chained together
   int getIShapeMapCount() const
   { return m_graph->getShapeGraphs().getMapCount() + m_graph->getDataMaps().getMapCount(); }
   IShapeMap *getFirstIShapeMap();
   IShapeMap *getNextIShapeMap();
   //
   IAttributes *getIAttributes(AttributeTable *p_table, int analysis_type);
   IAttributes *getIAttributes(IVGAMap *ivga);
   IAttributes *getIAttributes(IShapeMap *ishape);
   //
   int getViewOrder(IVGAMap *ivga);
   int getViewOrder(IShapeMap *ishape);
   //
   IVGAMap *getIVGAMap(PointMap *);
   IShapeMap *getIShapeMap(ShapeMap *);
   //
//   PointMap *getPointMap(IVGAMap *);
//   ShapeMap *getShapeMap(IShapeMap *);
   //
protected:
   // protected as the return is mutable -- much better for everyone else to use the fixed IShapeMap pointers
   int getMapRef(IShapeMap *map);
   int getMapRef(IVGAMap *map);
   //
   // Note: shape graphs and data maps are chain together!  Do not forget!
   IShapeMap *getIShapeMap(int n);
   IVGAMap *getIVGAMap(int n);
   //
   // I am not sure if I really need this, but it certainly helps when writing
   // "pushValues" in IGraphFile
   AttributeTable *getAttributeTable(IShapeMap *map);
   AttributeTable *getAttributeTable(IVGAMap *map);
};

#endif
