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



#ifndef __MAPINFODATA_H__
#define __MAPINFODATA_H__

// imported and exported data
// note: this is very basic and designed for axial line import / export only

// MapInfoData is stored with axial map data

class MapInfoData
{
friend class ShapeGraph;
friend class ShapeGraphs;
friend class ShapeMap;
   //
protected:
   std::string m_version;
   std::string m_charset;
   char m_delimiter;
   std::string m_index;
   std::string m_coordsys;
   std::string m_bounds;
   //
   // no longer use columnheads and table
   // -- where possible, added directly to the data
   // pvecstring m_columnheads; // <- original mapinfo column headers
   // pvecstring m_table;       // <- original mapinfo table (stored as a flat text file!)
   //
public:
   MapInfoData();
   // 
   int import(istream& miffile, istream& midfile, ShapeMap& map);
   //bool exportFile(ostream& miffile, ostream& midfile, const ShapeGraph& map);   // n.b., deprecated: use shapemap instead
   bool exportFile(ostream& miffile, ostream& midfile, const PointMap& points);
   bool exportFile(ostream& miffile, ostream& midfile, const ShapeMap& map);
   bool exportPolygons(ostream& miffile, ostream& midfile, const prefvec<pqvector<Point2f>>& polygons, const QtRegion& region);
   //
   bool readheader(istream& miffile);
   bool readcolumnheaders(istream& miffile, istream& midfile, std::vector<std::string>& columnheads);
   void writeheader(ostream& miffile);
   void writetable(ostream& miffile, ostream& midfile, const AttributeTable& attributes);
   //
   istream& read(istream& stream, int version);
   ostream& write(ostream& stream);
};

#endif
