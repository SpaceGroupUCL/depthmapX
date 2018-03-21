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

#include "mgraph440/mgraph_consts.h"
#include "mgraph440/stringutils.h"

namespace mgraph440 {

// imported and exported data
// note: this is very basic and designed for axial line import / export only

// MapInfoData is stored with axial map data

class MapInfoData
{
    friend class ShapeGraph;
    friend class ShapeGraphs;
    friend class ShapeMap;

public:
   std::string m_version;
   std::string m_charset;
   char m_delimiter;
   std::string m_index;
   std::string m_coordsys;
   std::string m_bounds;

   std::istream& read(std::istream& stream, int version)
   {
      m_version = dXstring440::readString(stream);
      m_charset = dXstring440::readString(stream);
      m_delimiter = stream.get();
      m_index = dXstring440::readString(stream);
      m_coordsys = dXstring440::readString(stream);
      m_bounds = dXstring440::readString(stream);
      //
      // this is no longer used: just a dummy read:
      if (version < mgraph440::VERSION_MAPINFO_SHAPES) {
         int columns, rows;
         std::vector<std::string> columnheads;
         std::vector<std::string> table;
         stream.read((char *) &columns, sizeof(int));
         for (int i = 0; i < columns; i++) {
             columnheads.push_back(dXstring440::readString(stream));
         }
         stream.read((char *) &rows, sizeof(int));
         for (int j = 0; j < rows; j++) {
            table.push_back(dXstring440::readString(stream));
         }
      }

      return stream;
   }
   std::ostream& write(std::ostream& stream)
   {
      dXstring440::writeString(stream, m_version);
      dXstring440::writeString(stream, m_charset);
      stream.put(m_delimiter);
      dXstring440::writeString(stream, m_index);
      dXstring440::writeString(stream, m_coordsys);
      dXstring440::writeString(stream, m_bounds);
      /*
      // No longer used as of VERSION_MAPINFO_SHAPES
      int columns = m_columnheads.size();
      int rows = m_table.size();
      stream.write((char *)&columns, sizeof(columns));
      for (int i = 0; i < m_columnheads.size(); i++) {
         m_columnheads[i].write(stream);
      }
      stream.write((char *)&rows, sizeof(rows));
      for (int j = 0; j < m_table.size(); j++) {
         m_table[j].write(stream);
      }
      */
      return stream;
   }
};

}

#endif
