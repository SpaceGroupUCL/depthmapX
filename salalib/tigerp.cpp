// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2018, Petros Koutsolampros

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

// Quick Tiger line parser (type 1 records)

#include "salalib/tigerp.h"
#include "genlib/comm.h"
#include <iostream>
#include <fstream>


// at some point will need to extend to parsing record type 2 (chains) as well as record type 1 (node to node)

// Thank you US Census Bureau -- this is a great easy flat file format:

void TigerMap::parse(const std::vector<std::string>& fileset, Communicator *comm)
{

   time_t atime = 0;

   qtimer( atime, 0 );
     
   for (size_t i = 0; i < fileset.size(); i++) {
      std::ifstream stream(fileset[i].c_str());
      while (!stream.eof())
      {
         std::string line;
         std::getline(stream, line);

         if (line.length()) {
            // grab major code:
            std::string code = line.substr(55,2);
            if (code[0] == 'A' || code[0] == 'B') {
               auto iter = m_categories.insert(std::make_pair(code,TigerCategory())).first;
               int long1 = stoi(line.substr(190,10));
               int lat1  = stoi(line.substr(200,9));
               int long2 = stoi(line.substr(209,10));
               int lat2  = stoi(line.substr(219,9));
               Point2f p1(double(long1)/1e6,double(lat1)/1e6);
               Point2f p2(double(long2)/1e6,double(lat2)/1e6);
               Line li(p1,p2);
               iter->second.chains.push_back(TigerChain());
               iter->second.chains.back().lines.push_back(li);
               if (!m_init) {
                  m_region = li;
                  m_init = true;
               }
               else {
                  m_region = runion(m_region,li);
               }
            }
         }
         if (comm)
         {
            if (qtimer( atime, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
            }
         }
      }
   }
}
