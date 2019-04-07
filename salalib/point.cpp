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

#include "salalib/point.h"
#include "salalib/ngraph.h"

float Point::getBinDistance(int i)
{
   return m_node->bindistance(i);
}

std::istream& Point::read(std::istream& stream)
{
   stream.read( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // (note block NO LONGER used!)
   stream.read( (char *) &m_block, sizeof(m_block) );

   int dummy = 0;
   stream.read( reinterpret_cast<char *>(&dummy), sizeof(dummy) );

   stream.read( (char *) &m_grid_connections, sizeof(m_grid_connections) );


   stream.read( (char *) &m_merge, sizeof(m_merge) );
   bool ngraph;
   stream.read( (char *) &ngraph, sizeof(ngraph) );
   if (ngraph) {
       m_node = std::unique_ptr<Node>(new Node());
       m_node->read(stream);
   }

   stream.read((char *) &m_location, sizeof(m_location));

   return stream;
}

std::ostream &Point::write(std::ostream& stream)
{
   stream.write( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // note block is no longer used at all
   stream.write( (char *) &m_block, sizeof(m_block) );
   int dummy = 0;
   stream.write( (char *) &dummy, sizeof(dummy) );
   stream.write( (char *) &m_grid_connections, sizeof(m_grid_connections) );
   stream.write( (char *) &m_merge, sizeof(m_merge) );
   bool ngraph;
   if (m_node) {
      ngraph = true;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
      m_node->write(stream);
   }
   else {
      ngraph = false;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
   }
   stream.write((char *) &m_location, sizeof(m_location));
   return stream;
}
