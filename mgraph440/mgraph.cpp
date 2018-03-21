#include "mgraph440/mgraph.h"

namespace mgraph440 {

MetaGraph::MetaGraph()
{
   m_state = 0;
   m_view_class = VIEWNONE;
   m_file_version = -1; // <- if unsaved, file version is -1

   // Standard layers no longer added: the gates layer will be initialised with the first push to layer,
   // or when made from axial lines.

   m_attr_conv_table = NULL;

   // whether or not showing text / grid saved with file:
   m_showtext = false;
   m_showgrid = false;
}

MetaGraph::~MetaGraph()
{
}

int MetaGraph::convertAttributes( ifstream& stream, int version )
{
   int ret = 1;

   // This is adapted from original ArVertexList code:

   int attr_count, which_attributes;
   stream.read( (char *) &which_attributes, sizeof(int) );
   stream.read( (char *) &attr_count, sizeof(int) );

   int size;
   stream.read( (char *) &size, sizeof(int) );

   if (m_attr_conv_table)
      delete m_attr_conv_table;
   m_attr_conv_table = new pqvector<AttrBody>;
   pqvector<AttrBody>& attrs = *m_attr_conv_table;

   size_t i;
   for (i = 0; i < size_t(size); i++) {
      AttrBody attr(-1, g_attr_header);
      attr.read(stream, attr_count);   // <- only write in saved attributes
      if (attr.ref != -1) {
         attrs.add( attr );
      }
   }

   for (size_t ii = 0; ii < attrs.size(); ii++) {
      // Check these are numbered 0 to n, and set the pos to be used later:
      if (attrs[ii].ref != ii) {
         ret = 0;
         break;
      }
      attrs[ii].pos = -1;
   }

   if (ret == 0) {
      delete m_attr_conv_table;
      m_attr_conv_table = NULL;
      return 0;
   }

   for (i = 0; i < PointMaps::size(); i++) {
      PointMap& map = PointMaps::at(i);
      for (int j = 0; j < map.m_cols; j++) {
         for (int k = 0; k < map.m_rows; k++) {
            // Note: block used to be the noderef
            if (map.m_points[j][k].filled() && map.m_points[j][k].m_block != -1) {
               AttrBody test(-1,g_attr_header);
               test.ref = map.m_points[j][k].m_block;
               size_t pos = attrs.searchindex(test);
               if (pos != paftl::npos) {
                  map.m_points[j][k].setAttributes(attrs[pos]);
                  attrs[pos].pos = (int) PixelRef(j,k);  // <- note this can be used to convert the virtual mem quickly
               }
               else {
                  // oops
                  ret = 0;
               }
            }
         }
      }
   }
   return ret;
}

int MetaGraph::convertVirtualMem( ifstream& stream, int version )
{
   if (!m_attr_conv_table) {
      return 0;
   }
   if (~m_state & ANGULARGRAPH) {
      // can't convert non-angular graphs:
      if (m_attr_conv_table)
         delete m_attr_conv_table;
      m_attr_conv_table = NULL;
      return DEPRECATED_VERSION;
   }

   // The attr conv table can help us quickly convert:
   pqvector<AttrBody>& attrs = *m_attr_conv_table;

   pvecint nodes;
   pvecint bins;

   PixelRefVector bins_b[32];
   static float far_bin_dists[32];
   for (int ii = 0; ii < 32; ii++) {
      far_bin_dists[ii] = 0.0f;
   }

   char header[3];
   stream.read( header, 3 );                             // 'grf'
   stream.read( (char *) &version, sizeof( version ) );  // version
   char type;
   stream.read( (char *) &type, sizeof( type ) );        // 'v'

   int size;
   stream.read( (char *) &size, sizeof( size ) );

   PointMap& map = PointMaps::tail();
   for (int i = 0; i < size; i++) {
      nodes.read(stream);
      bins.read(stream);
      if (attrs[i].pos != -1) {
         PixelRef curs = (PixelRef) attrs[i].pos;

         for (int j = 0; j < 32; j++) {
            for (int k = loword(bins[j]); k < hiword(bins[j]); k++) {
               int next = attrs[nodes[k]].pos;
               if (next != -1) {
                  bins_b[j].push_back( (PixelRef) next );
               }
            }
         }

         Point& pt = map.getPoint( curs );
         pt.m_node = new Node;
         pt.m_node->make( curs, bins_b, far_bin_dists, 0x00FF );
      }
      nodes.clear();
      bins.clear();
   }

   delete m_attr_conv_table;
   m_attr_conv_table = NULL;

   return WARN_CONVERTED;
}

int MetaGraph::read( const std::string& filename )
{
   m_state = 0;   // <- clear the state out

   if (filename.empty()) {
      return NOT_A_GRAPH;
   }

#ifdef _WIN32
   ifstream stream( filename.c_str(), ios::binary | ios::in );
#else
   ifstream stream( filename.c_str(), ios::in );
#endif

   char header[3];
   stream.read( header, 3 );
   if (stream.fail() || header[0] != 'g' || header[1] != 'r' || header[2] != 'f') {
      stream.close();
      return NOT_A_GRAPH;
   }
   int version;
   stream.read( (char *) &version, sizeof( version ) );
   m_file_version = version;  // <- recorded for easy debugging
   if (version > METAGRAPH_VERSION) {
      stream.close();
      return NEWER_VERSION;
   }
   if (version == VERSION_VIEW_CLASS_ADDED) {
      stream.close();
      return DEPRECATED_VERSION; // trial version no longer supported
   }

   // have to use temporary state here as redraw attempt may come too early:
   int temp_state = 0;
   if (version >= VERSION_STATE_RECORDED) {
      stream.read( (char *) &temp_state, sizeof( temp_state ) );
   }

   if (version >= VERSION_VIEW_CLASS_ADDED) {
      stream.read( (char *) &m_view_class, sizeof(m_view_class) );
   }

   if (version >= VERSION_STORE_GRIDTEXTINFO) {
      stream.read( (char *) &m_showgrid, sizeof(m_showgrid) );
      stream.read( (char *) &m_showtext, sizeof(m_showtext) );
   }

   // type codes: x --- properties
   //             v --- virtual graph (from versions below 70)
   //             n --- ngraph format
   //             l --- layer data
   //             p --- point data
   //             d --- data summary layers

   bool conversion_required = false;

   char type;
   stream.read( &type, 1 );
   if (type == 'x') {
      FileProperties::read(stream,version);
      if (stream.eof()) {
         // erk... this shouldn't happen
         stream.close();
         return DAMAGED_FILE;
      }
      else if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   else {
      FileProperties::setProperties("<unknown>","<unknown>","<unknown>","<unknown>");
   }
   if (stream.eof()) {
       // file is still ok, just empty
       stream.close();
       return OK;
   }
   if (type == 'v') {

      conversion_required = true;

      skipVirtualMem(stream,version);

      // and set our filename:
      // Graph::m_nodes.setFilename( filename );

      // and tell everyone it's been archived
      // temp_state |= GRAPHARCHIVED;

      if (stream.eof()) {
         // erk... this shouldn't happen
         stream.close();
         return DAMAGED_FILE;
      }
      else if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'l') {
      try {
         SuperSpacePixel::read( stream, version );
         temp_state |= LINEDATA;
         if (!stream.eof()) {
            stream.read( &type, 1 );
         }
      }
      catch (pexception) {
         // erk... this shouldn't happen
         stream.close();
         return DAMAGED_FILE;
      }
   }
   if (type == 'p') {
      if (version < VERSION_NGRAPH_INTROD) {
         // This hasn't been coded yet
         stream.close();
         return DEPRECATED_VERSION;
      }
      if (version < VERSION_POINT_MAPS) {
         PointMaps::push_back(PointMap());
         if (temp_state & 0x0001) { // 0x0001 was "GRAPH"
            PointMaps::tail().m_processed = true;
            temp_state &= ~0x0001;
         }
         if (temp_state & 0x0080) { // 0x0080 was "BOUNDARYGRAPH"
            PointMaps::tail().m_boundarygraph = true;
            temp_state &= ~0x0080;
         }
         PointMaps::tail().read( stream, version );
         setDisplayedPointMapRef(0);
      }
      else {
         PointMaps::read( stream, version );
      }
      PointMaps::setSpacePixel( (SuperSpacePixel *) this );
      temp_state |= POINTMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'g') {
      // Note the older version stored its attributes in a different location...
      // ...well, actually, now it's been rearranged, pretty similar to the original...
      // hasn't been coded yet, though (see above)
      if (version < VERSION_NGRAPH_INTROD) {
         if (!convertAttributes( stream, version )) {
            stream.close();
            return DAMAGED_FILE;
         }
      }
      // record on state of actual point map:
      PointMaps::tail().m_processed = true;

      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'a') {
      temp_state |= ANGULARGRAPH;
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'd') {
      // data layers are deprecated: data layers have been replaced by shape maps
      // so: first read data layers:
      DataLayers dl;
      dl.read( stream, version );
      // now replace with shape maps, but only if layer exists:
      temp_state &= ~DATAMAPS;
      // converter requires a point map to work on:
      if (PointMaps::size()) {
         // returns 0 if there are actually no objects in the shapemaps to convert,
         int conv_ok = convertDataLayersToShapeMap(dl,getDisplayedPointMap());
         if (conv_ok == 1) {
            // read objects in:
            temp_state |= DATAMAPS;
         }
         else if (conv_ok == -1) {
            // read objects in, but had trouble converting them:
            temp_state |= DATAMAPS;
            temp_state |= WARN_CONVERTED;
         }
      }
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'x') {
      m_shape_graphs.read( stream, version );
      temp_state |= SHAPEGRAPHS;
      /*
      // THIS CODE IS NO LONGER REQUIRED AS AXIAL MAPS *ARE* SHAPE MAPS -- can just be switched to shape map layer
      if (version < VERSION_SHAPE_MAPS && m_shape_graphs.m_gate_map != -1) {
         // check for a gate map:
         convertShapeGraphToShapeMap(m_shape_graphs.at(m_shape_graphs.m_gate_map));
         // delete the gate map:
         m_shape_graphs.remove_at(m_shape_graphs.m_gate_map);
         if (m_shape_graphs.m_displayed_map >= m_shape_graphs.m_gate_map) {
            int map = m_shape_graphs.m_displayed_map;
            m_shape_graphs.m_displayed_map = -1; // <- have to do this as setDisplayedShapeGraph clearSels a map that may not exist anymore
            setDisplayedShapeGraph(map-1);
         }
         m_shape_graphs.m_gate_map = -1;
         if (m_shape_graphs.size() == 0) {
            temp_state &= ~SHAPEGRAPHS;
         }
         // assume objects read in okay:
         temp_state |= DATAMAPS;
      }
      */
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 's') {
      m_data_maps.read( stream, version );
      temp_state |= DATAMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }

   stream.close();

   m_state = temp_state;

   if (version < VERSION_VIEW_CLASS_ADDED) {
      if (m_state & POINTMAPS) {
         m_view_class = VIEWVGA;
      }
      else {
         m_view_class = VIEWNONE;
      }
   }

   // Note, below version 70, the graph data must be reread:
   if (version < VERSION_NGRAPH_INTROD && conversion_required) {
      // reopen the stream and convert
      stream.open(filename.c_str(), ios::binary | ios::in );
      int ok = convertVirtualMem(stream, version);
      stream.close();
      return ok;
   }

   if (version == VERSION_EXTRA_POINT_DATA_INTROD || version == VERSION_NGRAPH_INTROD || version == VERSION_SEGMENT_MAPS) {
      m_state |= BUGGY;
      return WARN_BUGGY_VERSION;
   }

   return OK;
}
streampos MetaGraph::skipVirtualMem(ifstream& stream, int version)
{
   // it's graph virtual memory: skip it
   int nodes = -1;
   stream.read( (char *) &nodes, sizeof(nodes) );

   // in angular version, have to skip two pvectors for each node:
   if (version >= 30)
      nodes *= 2;

   for (int i = 0; i < nodes; i++) {
      int connections;
      stream.read( (char *) &connections, sizeof(connections) );
      // This relies on the pvecint storage... hope it don't change!
      stream.seekg( stream.tellg() + streamoff(connections * sizeof(connections)) );
   }
   return (stream.tellg());
}

int MetaGraph::convertDataLayersToShapeMap(DataLayers& datalayers, PointMap& pointmap)
{
   int retvar = 1;
   // check for existence of data:
   pmap<int,int> conversion_lookup;
   size_t i;
   for (i = 0; i < size_t(datalayers.getLayerCount()); i++) {
      if (datalayers[i].getObjectCount()) {
         int x = m_data_maps.addMap(datalayers[i].getLayerName(),ShapeMap::DATAMAP);
         conversion_lookup.add(i,x);
      }
   }
   // nothing to convert:
   if (!conversion_lookup.size()) {
      return 0;
   }

   for (i = 0; i < conversion_lookup.size(); i++) {
      ShapeMap& shapemap = m_data_maps.getMap(conversion_lookup.value(i));
      int layer_ref = datalayers.getLayerRef(conversion_lookup.key(i));
      pvecint *shape_pixel_lists = new pvecint [datalayers[i].getObjectCount()];
      int j;
      for (j = 0; j < pointmap.getAttributeTable().getRowCount(); j++) {
         PixelRef pix = pointmap.getAttributeTable().getRowKey(j);
         int z = pointmap.getPoint(pix).getDataObject(layer_ref);
         if (z != -1) {
            shape_pixel_lists[z].push_back(pix);
         }
      }
      // add shapes:
      pvecint row_lookup;
      for (j = 0; j < datalayers[i].getObjectCount(); j++) {
         for (size_t k = 0; k < shape_pixel_lists[j].size(); k++) {
            pointmap.overrideSelPixel(shape_pixel_lists[j][k]);
         }
         row_lookup.push_back(shapemap.makeShapeFromPointSet(pointmap));
         pointmap.clearSel();
      }
      delete [] shape_pixel_lists;
      // now add attributes:
      AttributeTable& table = shapemap.getAttributeTable();
      // add columns, note, we'll have to add and then have lookups because not necessarily in alphabetical order:
      for (j = 0; j < datalayers[i].getColumnCount(); j++) {
         table.insertColumn(datalayers[i].getColumnTitle(j));
      }
      pvecint column_lookup;
      for (j = 0; j < datalayers[i].getColumnCount(); j++) {
         column_lookup.push_back(table.getColumnIndex(datalayers[i].getColumnTitle(j)));
      }

      // now we can add the data for this horrible matrix:
      for (j = 0; j < datalayers[i].getObjectCount(); j++) {
         for (int k = 0; k < datalayers[i].getColumnCount(); k++) {
            if (row_lookup[j] != -1) {
               int row = table.getRowid(row_lookup[j]);  // row lookup should equal j since this is a new shape map, but for safety looked up
               table.setValue(row,column_lookup[k],float(datalayers[i][j][k]));
            }
            else {
               // conversion error occurred:
               retvar = -1;
            }
         }
      }

      // set the displayed attribute ready for first draw:
      shapemap.overrideDisplayedAttribute(-2);
      shapemap.setDisplayedAttribute(-1);
   }
   // the horror is over:
   return retvar;
}

int MetaGraph::writeToFile( const std::string& filename, int version, bool currentlayer )
{
   ofstream stream;

   int oldstate = m_state;
   m_state = 0;   // <- temporarily clear out state, avoids any potential read / write errors

   // As of MetaGraph version 70 the disk caching has been removed
   stream.open( filename.c_str(), ios::binary | ios::out | ios::trunc );
   if (stream.fail()) {
      if (stream.rdbuf()->is_open()) {
         stream.close();
      }
      m_state = oldstate;
      return DISK_ERROR;
   }
   m_state = oldstate;
   int result = writeToStream(stream, version, currentlayer);
   stream.close();
   return result;
}
int MetaGraph::writeToStream(std::ostream &stream, int version, bool currentlayer) {
   int oldstate = m_state;
   char type;

   stream.write("grf", 3);
   m_file_version = version; // <- note, the file may now have an updated file version
   stream.write( (char *) &version, sizeof(version) );
   if (currentlayer) {
      int tempstate, tempclass;
      if (m_view_class & VIEWVGA) {
         tempstate = POINTMAPS;
         tempclass = VIEWVGA;
      }
      else if (m_view_class & MetaGraph::VIEWAXIAL) {
         tempstate = SHAPEGRAPHS;
         tempclass = VIEWAXIAL;
      }
      else if (m_view_class & MetaGraph::VIEWDATA) {
         tempstate = DATAMAPS;
         tempclass = VIEWDATA;
      }
      stream.write( (char *) &tempstate, sizeof(tempstate) );
      stream.write( (char *) &tempclass, sizeof(tempclass) );
   }
   else {
      stream.write( (char *) &oldstate, sizeof(oldstate) );
      stream.write( (char *) &m_view_class, sizeof(m_view_class) );
   }
   stream.write( (char *) &m_showgrid, sizeof(m_showgrid) );
   stream.write( (char *) &m_showtext, sizeof(m_showtext) );

   type = 'x';
   stream.write(&type, 1);
   FileProperties::write(stream);

   if (currentlayer) {
      if (m_view_class & MetaGraph::VIEWVGA) {
         type = 'p';
         stream.write(&type, 1);
         PointMaps::write( stream, version, true );
      }
      else if (m_view_class & MetaGraph::VIEWAXIAL) {
         type = 'x';
         stream.write(&type, 1);
         m_shape_graphs.write( stream, version, true );
      }
      else if (m_view_class & MetaGraph::VIEWDATA) {
         type = 's';
         stream.write(&type, 1);
         m_data_maps.write( stream, version, true );
      }
   }
   else {
      if (oldstate & LINEDATA) {
         type = 'l';
         stream.write(&type, 1);
         SuperSpacePixel::write( stream, version );
      }
      if (oldstate & POINTMAPS) {
         type = 'p';
         stream.write(&type, 1);
         PointMaps::write( stream, version );
      }
      if (oldstate & SHAPEGRAPHS) {
         type = 'x';
         stream.write(&type, 1);
         m_shape_graphs.write( stream, version );
      }
      if (oldstate & DATAMAPS) {
         type = 's';
         stream.write(&type, 1);
         m_data_maps.write( stream, version );
      }
   }

   m_state = oldstate;
   return OK;
}

}
