#include "mgraph440/pointmap.h"
#include "mgraph440/containerutils.h"

namespace mgraph440 {

bool PointMap::read(ifstream& stream, int version )
{
   if (version >= VERSION_POINT_MAP_NAMES) {
      m_name = dXstring440::readString(stream);
   }
   else {
      m_name = "VGA Map";
   }

   // NOTE: You MUST set m_spacepix manually!
   m_displayed_attribute = -1;

   if (m_points) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_points[i];
      }
      delete [] m_points;
      m_points = NULL;
   }

   stream.read( (char *) &m_spacing, sizeof(m_spacing) );

   if (version < VERSION_STATE_RECORDED) {
      if (version >= VERSION_SPARK_GRAPH_INTROD) {
         bool redundant;
         stream.read( (char *) &redundant, sizeof(redundant) );
      }
      else {
         double redundant;
         stream.read( (char *) &redundant, sizeof(redundant) );
      }
   }

   stream.read( (char *) &m_rows, sizeof(m_rows) );
   stream.read( (char *) &m_cols, sizeof(m_cols) );

   stream.read( (char *) &m_point_count, sizeof(m_point_count) );

   stream.read( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   m_region = QtRegion(
      Point2f(m_bottom_left.x-m_spacing/2.0, m_bottom_left.y-m_spacing/2.0),
      Point2f(m_bottom_left.x+double(m_cols-1)*m_spacing + m_spacing/2.0,
              m_bottom_left.y+double(m_rows-1)*m_spacing + m_spacing/2.0) );

   // for old data versions:
   int attr_count = -1, which_attributes = -1;

   int displayed_attribute;  // n.b., temp variable necessary to force recalc below
   if (version >= VERSION_ATTRIBUTES_TABLE) {
      // our data read
      stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));
      m_attributes.read( stream, version );
   }
   else if (version >= VERSION_NGRAPH_INTROD) {
      // ick, a very specific subset have this file format:
      stream.read( (char *) &which_attributes, sizeof(which_attributes) );
      stream.read( (char *) &attr_count, sizeof(int) );
   }

   m_points = new Point *[m_cols];

   for (int j = 0; j < m_cols; j++) {
      m_points[j] = new Point [m_rows];
      // ...and read...
      if (version >= VERSION_LAYERS_INTROD) {
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].read(stream,version,attr_count);

            // check if occdistance of any pixel's bin is set, meaning that
            // the isovist analysis was done
            if(!m_hasIsovistAnalysis) {
                for(int b = 0; b < 32; b++) {
                   if(m_points[j][k].m_node && m_points[j][k].m_node->occdistance(b) > 0) {
                       m_hasIsovistAnalysis = true;
                       break;
                   }
                }
            }
         }
      }
      else if (version >= VERSION_EXTRA_POINT_DATA_INTROD) {
         // Hmm... more untidiness from a previous incarnation
         OldPoint2 *oldpoints = new OldPoint2 [m_rows];
         stream.read( (char *) oldpoints, sizeof(OldPoint2) * m_rows );
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].m_block = oldpoints[k].m_noderef;  // <- block is actually for something else!
            m_points[j][k].m_state = oldpoints[k].m_state;
            m_points[j][k].m_misc = oldpoints[k].m_misc;
         }
      }
      else {
         // Hmm... more untidiness from a previous incarnation
         OldPoint1 *oldpoints = new OldPoint1 [m_rows];
         stream.read( (char *) oldpoints, sizeof(OldPoint1) * m_rows );
         for (int k = 0; k < m_rows; k++) {
            m_points[j][k].m_block= oldpoints[k].m_noderef;  // <- block is actually for something else!
            m_points[j][k].m_state = oldpoints[k].m_state;
         }
      }

      for (int k = 0; k < m_rows; k++) {
         // Old style point node reffing and also unselects selected nodes which would otherwise be difficult
         if (version >= VERSION_QUICK_GRAPH_INTROD) {
            // would soon be better simply to turn off the select flag....
            m_points[j][k].m_state &= ( Point::EMPTY | Point::FILLED | Point::MERGED | Point::BLOCKED | Point::CONTEXTFILLED | Point::EDGE);
         }
         else if (m_points[j][k].m_state == -1) {
            m_points[j][k].m_state = Point::EMPTY;
         }
         else {
            m_points[j][k].m_state = Point::FILLED;
         }
         // Set the node pixel if it exists:
         if (m_points[j][k].m_node) {
            m_points[j][k].m_node->setPixel(PixelRef(j,k));
         }
         // Set the point location if required:
         if (version < VERSION_POINT_LOCATIONS) {
            m_points[j][k].m_location = depixelate(PixelRef(j,k));
         }
         // Add merge line if merged:
         if (!m_points[j][k].m_merge.empty()) {
             depthmapX440::addIfNotExists(m_merge_lines, PixelRefPair(PixelRef(j,k),m_points[j][k].m_merge));
         }
      }
   }

   // this is my attempt to enter these into the new attribute table:
   if (version >= VERSION_NGRAPH_INTROD && version < VERSION_ATTRIBUTES_TABLE) {
      if (which_attributes != GraphVertexList::NONE) {
         displayed_attribute = 0;
         convertAttributes(which_attributes);
      }
      else {
         displayed_attribute = -1;
      }
   }

   // and if the attribute tables are ready, calculate the direct grid connections:
   if (version >= VERSION_NGRAPH_INTROD && version < VERSION_GRID_CONNECTIONS) {
      addGridConnections();
   }

   m_selection = NO_SELECTION;
   m_pinned_selection = false;

   m_initialised = true;
   m_blockedlines = false;

   if (version >= VERSION_POINT_MAPS) {
      stream.read((char *) &m_processed, sizeof(m_processed));
      stream.read((char *) &m_boundarygraph, sizeof(m_boundarygraph));
   }

   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

// A horrible piece of code: this is to convert a file from the old format
// AttrHeader / AttrBody to the new format Attributes Table

// This code converts attributes for all versions
// between VERSION_NGRAPH_INTROD and VERSION_ATTRIBUTES_TABLE

void PointMap::convertAttributes(int which_attributes)
{
   if (which_attributes & GraphVertexList::BASIC) {
      int connectivity_col = m_attributes.insertLockedColumn("Connectivity");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               // insert row...
               int row = m_attributes.insertRow(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::NEIGHBOURHOOD_SIZE);
               m_attributes.setValue(row, connectivity_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::LOCAL) {
      int cluster_col = m_attributes.insertColumn("Visual Clustering Coefficient");
      int control_col = m_attributes.insertColumn("Visual Control");
      int controllability_col = m_attributes.insertColumn("Visual Controllability");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CLUSTER);
               m_attributes.setValue(row, cluster_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CONTROL_HILL);
               m_attributes.setValue(row, control_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::CONTROL_TURN);
               m_attributes.setValue(row, controllability_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::GLOBAL) {
      int entropy_col = m_attributes.insertColumn("Visual Entropy");
      int integ_hh_col = m_attributes.insertColumn("Visual Integration [HH]");
      int integ_tk_col = m_attributes.insertColumn("Visual Integration [Tekl]");
      int depth_col = m_attributes.insertColumn("Visual Mean Depth");
      int count_col = m_attributes.insertColumn("Visual Node Count");
      int rel_entropy_col = m_attributes.insertColumn("Visual Relativised Entropy");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::ENTROPY);
               m_attributes.setValue(row, entropy_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::REL_ENTROPY);
               m_attributes.setValue(row, rel_entropy_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::INTEGRATION_HILL);
               m_attributes.setValue(row, integ_hh_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::INTEGRATION_TEKL);
               m_attributes.setValue(row, integ_tk_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::MEAN_DEPTH);
               m_attributes.setValue(row, depth_col, val);
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::GRAPH_SIZE);
               m_attributes.setValue(row, count_col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::POINTDEPTH) {
      int col = m_attributes.insertColumn("Visual Step Depth");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               float val;
               val = (float) m_points[i][j].getAttributes().getAttr(AttrHeader::POINT_DEPTH);
               m_attributes.setValue(row, col, val);
            }
         }
      }
   }
   if (which_attributes & GraphVertexList::METRIC) {
      int mspa_col = m_attributes.insertColumn("Metric Mean Shortest-Path Angle");
      int mspl_col = m_attributes.insertColumn("Metric Mean Shortest-Path Distance");
      int dist_col = m_attributes.insertColumn("Metric Mean Straight-Line Distance");
      int count_col = m_attributes.insertColumn("Metric Node Count");
      for (int i = 0; i < m_cols; i++) {
         for (int j = 0; j < m_rows; j++) {
            if (m_points[i][j].m_attributes) {
               int row = m_attributes.getRowid(PixelRef(i,j));
               double val, total = m_points[i][j].getAttributes().getAttr(AttrHeader::METRIC_GRAPH_SIZE);
               //
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_METRIC_ANGLE) / total;
               m_attributes.setValue(row, mspa_col, float(val));
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_METRIC_DEPTH) / total;
               m_attributes.setValue(row, mspl_col, float(val));
               val = m_points[i][j].getAttributes().getAttr(AttrHeader::TOTAL_EUCLID_DIST) / total;
               m_attributes.setValue(row, dist_col, float(val));
               //
               m_attributes.setValue(row, count_col, float(total));
            }
         }
      }
   }
   for (int i = 0; i < m_cols; i++) {
      for (int j = 0; j < m_rows; j++) {
         if (m_points[i][j].m_attributes) {
            delete m_points[i][j].m_attributes;
            m_points[i][j].m_attributes = NULL;
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////
// Update connections will load an old graph and add char information

void PointMap::addGridConnections()
{
   for (int i = 0; i < m_attributes.getRowCount(); i++) {
      PixelRef curs = m_attributes.getRowKey(i);
      PixelRef node = curs.right();
      Point& point = getPoint(curs);
      point.m_grid_connections = 0;
      for (int i = 0; i < 32; i += 4) {
         Bin& bin = point.m_node->bin(i);
         bin.first();
         while (!bin.is_tail()) {
            if (node == bin.cursor()) {
               point.m_grid_connections |= (1 << (i / 4));
               break;
            }
            bin.next();
         }
         char dir;
         if (i == 0) {
            dir = PixelRef::VERTICAL;
         }
         else if (i == 4 || i == 8) {
            dir = PixelRef::NEGHORIZONTAL;
         }
         else if (i == 12 || i == 16) {
            dir = PixelRef::NEGVERTICAL;
         }
         else if (i == 20 || i == 24) {
            dir = PixelRef::HORIZONTAL;
         }
         node.move(dir);
      }
   }
}

void PointMap::setDisplayedAttribute(int col)
{
   if (m_displayed_attribute == col) {
      return;
   }
   else {
      m_displayed_attribute = col;
   }
   // make a local copy of the display params for access speed:
   m_display_params = m_attributes.getDisplayParams(m_displayed_attribute);

   m_attributes.setDisplayColumn(m_displayed_attribute,true);
}

// constrain is used to constrain to existing rows / cols
// (not quite the same as constraining to bounding box due to spacing offsets)
PixelRef PointMap::pixelate( const Point2f& p, bool constrain, int scalefactor ) const
{
   PixelRef ref;

   double spacing = m_spacing / double(scalefactor);
   ref.x = int(floor( (p.x - m_bottom_left.x + (m_spacing / 2.0)) / spacing ));
   ref.y = int(floor( (p.y - m_bottom_left.y + (m_spacing / 2.0)) / spacing ));

   if (constrain) {
      if (ref.x < 0)
         ref.x = 0;
      else if (ref.x >= m_cols * scalefactor)
         ref.x = (m_cols * scalefactor) - 1;
      if (ref.y < 0)
         ref.y = 0;
      else if (ref.y >= m_rows * scalefactor)
         ref.y = (m_rows * scalefactor) - 1;
   }

   return ref;
}

// dangerous: used only for making a false selection set
bool PointMap::overrideSelPixel(PixelRef pix)
{
   m_selection = OVERRIDE_SELECTION;
   if (!(m_points[pix.x][pix.y].m_state & Point::SELECTED)) {
      m_points[pix.x][pix.y].m_state |= Point::SELECTED;
      m_selection_set.insert(pix);
   }
   return true;
}
bool PointMap::clearSel()
{
   if (m_selection == NO_SELECTION) {
      return false;
   }
   for (auto& sel: m_selection_set) {
      getPoint(sel).m_state &= ~Point::SELECTED;
   }
   m_selection_set.clear();
   m_selection = NO_SELECTION;
   m_attributes.deselectAll();
   return true;
}

bool PointMaps::write(ostream& stream, int version, bool displayedmaponly)
{
   if (!displayedmaponly) {
      stream.write((char *) &m_displayed_map, sizeof(m_displayed_map));
      int count = size();
      stream.write((char *) &count, sizeof(count));
      for (int i = 0; i < count; i++) {
         at(i).write( stream, version );
      }
   }
   else {
      int dummy;
      // displayed map is 0:
      dummy = 0;
      stream.write((char *) &dummy, sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *) &dummy, sizeof(dummy));
      //
      at(m_displayed_map).write(stream, version);
   }
   return true;
}
bool PointMap::write( ostream& stream, int version )
{
   dXstring440::writeString(stream, m_name);

   stream.write( (char *) &m_spacing, sizeof(m_spacing) );

   stream.write( (char *) &m_rows, sizeof(m_rows) );
   stream.write( (char *) &m_cols, sizeof(m_cols) );

   stream.write( (char *) &m_point_count, sizeof(m_point_count) );

   stream.write( (char *) &m_bottom_left, sizeof(m_bottom_left) );

   stream.write( (char *) &m_displayed_attribute, sizeof(m_displayed_attribute) );
   m_attributes.write( stream, version );

   for (int j = 0; j < m_cols; j++) {
      for (int k = 0; k < m_rows; k++) {
         m_points[j][k].write( stream, version );
      }
   }

   stream.write((char *) &m_processed, sizeof(m_processed));
   stream.write((char *) &m_boundarygraph, sizeof(m_boundarygraph));

   return false;
}
}
