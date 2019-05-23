// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2018, Petros Koutsolampros

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

#include "salalib/axialmodules/axialstepdepth.h"

#include "genlib/pflipper.h"
#include "genlib/stringutils.h"

bool AxialStepDepth::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    std::string stepdepth_col_text = std::string("Step Depth");
    int stepdepth_col = attributes.insertOrResetColumn(stepdepth_col_text.c_str());

    bool *covered = new bool [map.getConnections().size()];
    for (size_t i = 0; i < map.getConnections().size(); i++) {
       covered[i] = false;
    }
    pflipper<std::vector<int> > foundlist;
    for(auto& lineindex: map.getSelSet()) {
       foundlist.a().push_back(lineindex);
       covered[lineindex] = true;
       map.getAttributeRowFromShapeIndex(lineindex).setValue(stepdepth_col,0.0f);
    }
    int depth = 1;
    while (foundlist.a().size()) {
       Connector& line = map.getConnections()[foundlist.a().back()];
       for (size_t k = 0; k < line.m_connections.size(); k++) {
          if (!covered[line.m_connections[k]]) {
             covered[line.m_connections[k]] = true;
             foundlist.b().push_back(line.m_connections[k]);
             map.getAttributeRowFromShapeIndex(line.m_connections[k]).setValue(stepdepth_col,float(depth));
          }
       }
       foundlist.a().pop_back();
       if (!foundlist.a().size()) {
          foundlist.flip();
          depth++;
       }
    }
    delete [] covered;

    map.setDisplayedAttribute(-1); // <- override if it's already showing
    map.setDisplayedAttribute(stepdepth_col);

    return true;
}
