#include "mgraph440/point.h"

namespace mgraph440 {

Point::~Point()
{
   if (m_node) {
      delete m_node;
      m_node = NULL;
      delete m_attributes;
      m_attributes = NULL;
   }
}

std::ifstream& Point::read(std::ifstream& stream, int version, int attr_count)
{
   if (m_node) {
      delete m_node;
      m_node = NULL;
      delete m_attributes;
      m_attributes = NULL;
   }
   stream.read( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // (note block NO LONGER used!)
   stream.read( (char *) &m_block, sizeof(m_block) );
   if (version < VERSION_BLOCKEDQUAD) {
      m_block = 0;
   }
   stream.read( (char *) &m_misc, sizeof(m_misc) );
   if (version >= VERSION_GRID_CONNECTIONS) {
      stream.read( (char *) &m_grid_connections, sizeof(m_grid_connections) );
   }
   if (version >= VERSION_NGRAPH_INTROD) {
      stream.read( (char *) &m_merge, sizeof(m_merge) );
      bool ngraph;
      stream.read( (char *) &ngraph, sizeof(ngraph) );
      if (ngraph) {
         m_node = new Node;
         m_node->read(stream, version);
         if (version < VERSION_ATTRIBUTES_TABLE) {
            // don't deal with this here, just get the attributes into memory
            m_attributes = new AttrBody(-1,g_attr_header);
            m_attributes->read( stream, attr_count );
         }
      }
   }
   if (version >= VERSION_POINT_LOCATIONS) {
      stream.read((char *) &m_location, sizeof(m_location));
   }
   if (version < VERSION_SHAPE_MAPS) {
      // old layer information
      m_data_objects.read(stream);
   }
   if (version >= VERSION_BOUNDARYGRAPH && version < VERSION_NEWBOUNDARYGRAPH) {
      // dummy pvecint to hold old format boundary nodes
      pvecint old_boundary_nodes;
      old_boundary_nodes.read(stream);
   }
   return stream;
}

ostream& Point::write(ostream& stream, int version)
{
   stream.write( (char *) &m_state, sizeof(m_state) );
   // block is the same size as m_noderef used to be for ease of replacement:
   // note block is no longer used at all
   stream.write( (char *) &m_block, sizeof(m_block) );
   stream.write( (char *) &m_misc, sizeof(m_misc) );
   stream.write( (char *) &m_grid_connections, sizeof(m_grid_connections) );
   stream.write( (char *) &m_merge, sizeof(m_merge) );
   bool ngraph;
   if (m_node) {
      ngraph = true;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
      m_node->write(stream, version);
   }
   else {
      ngraph = false;
      stream.write( (char *) &ngraph, sizeof(ngraph) );
   }
   stream.write((char *) &m_location, sizeof(m_location));
   return stream;
}

}
