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



// This is my code to make a set of axial lines from a set of boundary lines

#include "salalib/axialmap.h"
#include "salalib/alllinemap.h"
#include "salalib/tolerances.h"
#include "salalib/pointdata.h"   // need the pointdata for the convert boundary graph to axial map routine
#include "salalib/ngraph.h"      // ditto ngraph
#include "salalib/parsers/mapinfodata.h"

#include "genlib/comm.h"  // For communicator
#include "genlib/stringutils.h"
#include "genlib/containerutils.h"
#include "genlib/readwritehelpers.h"
#include "genlib/pflipper.h"

#include <math.h>
#include <float.h>
#include <time.h>

#ifndef _WIN32
#define _finite finite
#endif

////////////////////////////////////////////////////////////////////////////////////////////

ShapeGraph::ShapeGraph(const std::string& name, int type) : ShapeMap(name,type)
{
   m_keyvertexcount = 0;
   m_hasgraph = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeGraph::initialiseAttributesAxial()
{
    m_attributes->clear();
    // note, expects these to be numbered 0, 1...
    m_attributes->insertOrResetLockedColumn("Connectivity");
    m_attributes->insertOrResetLockedColumn("Line Length");

}

void ShapeGraph::makeConnections(const KeyVertices &keyvertices)
{
   m_connectors.clear();
   m_links.clear();
   m_unlinks.clear();
   m_keyvertices.clear();

   // note, expects these to be numbered 0, 1...
   int conn_col = m_attributes->getColumnIndex("Connectivity");
   int leng_col = m_attributes->getColumnIndex("Line Length");

   int i = -1;
   for (auto shape: m_shapes) {
      i++;
      int key = shape.first;
      AttributeRow &row =
          m_attributes->getRow(AttributeKey(key));
      // all indices should match...
      m_connectors.push_back( Connector() );
      m_connectors[i].m_connections = getLineConnections( key, TOLERANCE_B*__max(m_region.height(),m_region.width()));
      row.setValue(conn_col, float(m_connectors[i].m_connections.size()) );
      row.setValue(leng_col, float(shape.second.getLine().length()) );
      if (keyvertices.size()) {
         // note: depends on lines being recorded in same order as keyvertices...
         m_keyvertices.push_back( keyvertices[i] );
      }
   }

   m_displayed_attribute = -1; // <- override if it's already showing
   setDisplayedAttribute(conn_col);
}

/////////////////////////////////////////////////////////////////////////////////////////

bool ShapeGraph::outputMifPolygons(std::ostream& miffile, std::ostream& midfile) const
{
   // take lines from lines layer and make into regions (using the axial polygons)
   std::vector<Line> lines;
   for (auto shape: m_shapes) {
      lines.push_back(shape.second.getLine());
   }
   AxialPolygons polygons;
   polygons.init(lines, m_region);

   std::vector<std::vector<Point2f>> newpolygons;
   polygons.makePolygons(newpolygons);

   MapInfoData mapinfodata;
   if (m_hasMapInfoData) {
      mapinfodata.m_coordsys = m_mapinfodata.m_coordsys;
      mapinfodata.m_bounds = m_mapinfodata.m_bounds;
   }
   mapinfodata.exportPolygons(miffile, midfile, newpolygons, m_region);

   return true;
}

void ShapeGraph::outputNet(std::ostream& netfile) const
{
   double maxdim = __max(m_region.width(),m_region.height());
   Point2f offset = Point2f((maxdim - m_region.width())/(2.0*maxdim),(maxdim - m_region.height())/(2.0*maxdim));
   if (isSegmentMap()) {
      netfile << "*Vertices " << m_shapes.size() * 2 << std::endl;
      int i = -1;
      for (auto shape: m_shapes) {
         i++;
         Line li = shape.second.getLine();
         Point2f p1 = li.start();
         Point2f p2 = li.end();
         p1.x = offset.x + (p1.x - m_region.bottom_left.x) / maxdim;
         p2.x = offset.x + (p2.x - m_region.bottom_left.x) / maxdim;
         p1.y = 1.0 - (offset.y + (p1.y - m_region.bottom_left.y) / maxdim);
         p2.y = 1.0 - (offset.y + (p2.y - m_region.bottom_left.y) / maxdim);
         netfile << (i * 2 + 1) << " \"" << i << "a\" " << p1.x << " " << p1.y << std::endl;
         netfile << (i * 2 + 2) << " \"" << i << "b\" " << p2.x << " " << p2.y << std::endl;
      }
      netfile << "*Edges" << std::endl;
      for (size_t i = 0; i < m_shapes.size(); i++) {
         netfile << (i * 2 + 1) << " " << (i * 2 + 2) << " 2" << std::endl;
      }
      netfile << "*Arcs" << std::endl;
      // this makes an assumption about which is the "start" and which is the "end"
      // it works for an automatically converted axial map, I'm not sure it works for others...
      for (size_t j = 0; j < m_connectors.size(); j++) {
         const Connector& conn = m_connectors[j];
         for (auto& segconn: conn.m_forward_segconns) {
            SegmentRef ref = segconn.first;
            float weight = segconn.second;
            netfile << (j * 2 + 1) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " " << weight << std::endl;
         }
         for (auto& segconn: conn.m_back_segconns) {
            SegmentRef ref = segconn.first;
            float weight = segconn.second;
            netfile << (j * 2 + 2) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " " << weight << std::endl;
         }
      }
   }
   else {
      netfile << "*Vertices " << m_shapes.size() << std::endl;
      int i = -1;
      for (auto shape: m_shapes) {
         i++;
         Point2f p = shape.second.getCentroid();
         p.x = offset.x + (p.x - m_region.bottom_left.x) / maxdim;
         p.y = 1.0 - (offset.y + (p.y - m_region.bottom_left.y) / maxdim);
         netfile << (i + 1) << " \"" << i << "\" " << p.x << " " << p.y << std::endl;
      }
      netfile << "*Edges" << std::endl;
      for (size_t j = 0; j < m_connectors.size(); j++) {
         const Connector& conn = m_connectors[j];
         for (size_t k = 0; k < conn.m_connections.size(); k++) {
            size_t to = conn.m_connections[k];
            if (j < to) {
               netfile << (j+1) << " " << (to + 1) << " 1" << std::endl;
            }
         }
      }
   }
}

bool ShapeGraph::read(std::istream &stream)
{
   m_attributes->clear();
   m_connectors.clear();
   m_selection = false;
   m_map_type = ShapeMap::EMPTYMAP;

   // note that keyvertexcount and keyvertices are different things! (length keyvertices not the same as keyvertexcount!)
   stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
   int size;
   stream.read((char *)&size,sizeof(size));
   for (int i = 0; i < size; i++) {
      std::vector<int> tempVec;
      dXreadwrite::readIntoVector(stream, tempVec);
      m_keyvertices.push_back(std::set<int>(tempVec.begin(), tempVec.end()));
   }
   // now base class read:
   ShapeMap::read(stream);

   return true;
}

bool ShapeGraph::readold( std::istream& stream)
{
   // read in from old base class
   SpacePixel linemap;
   linemap.read(stream);
   const std::map<int,LineTest>& lines = linemap.getAllLines();

   m_name = linemap.getName();

   // now copy to new base class:
   init(lines.size(),linemap.getRegion());
   for (auto line: lines) {
      makeLineShape(line.second.line);
   }
   // n.b., we now have to reclear attributes!
   m_attributes->clear();

   // continue old read:
   int pushmap = -1;

   char segmentmapc = stream.get();
   if (segmentmapc == '1') {
      m_map_type = ShapeMap::SEGMENTMAP;
   }
   else {
      m_map_type = ShapeMap::AXIALMAP;
   }

   char gatemapc = stream.get();
   if (gatemapc == '1') {
      m_map_type = ShapeMap::DATAMAP;
   }
   stream.read((char *)&pushmap,sizeof(pushmap));


   int displayed_attribute;  // n.b., temp variable necessary to force recalc below
   stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));

   m_attributes->read(stream, m_layers);
   int size;
   stream.read((char *)&size,sizeof(size));
   for (int j = 0; j < size; j++) {
      m_keyvertices.push_back(std::set<int>());    // <- these were stored with the connector
      int key;
      stream.read((char *)&key,sizeof(key)); // <- key deprecated
      m_connectors.push_back(Connector());
      m_connectors[size_t(j)].read(stream);
   }
   stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));


   dXreadwrite::readIntoVector(stream,m_links);
   dXreadwrite::readIntoVector(stream,m_unlinks);

   char x = stream.get();
   if (x == 'm') {
      m_mapinfodata = MapInfoData();
      m_mapinfodata.read(stream);
      m_hasMapInfoData = true;
   }


   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

bool ShapeGraph::write( std::ofstream& stream )
{
   // note keyvertexcount and keyvertices are different things!  (length keyvertices not the same as keyvertexcount!)
   stream.write((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
   int size = m_keyvertices.size();
   stream.write((char *)&size,sizeof(size));
   for (size_t i = 0; i < m_keyvertices.size(); i++) {
      dXreadwrite::writeVector(stream, std::vector<int>( m_keyvertices[i].begin(), m_keyvertices[i].end() ));
   }

   // now simply run base class write:
   ShapeMap::write(stream);

   return true;
}

void ShapeGraph::writeAxialConnectionsAsDotGraph(std::ostream &stream)
{
    const std::vector<Connector>& connectors = ShapeMap::getConnections();

    stream << "strict graph {" << std::endl;

    stream.precision(12);

    for (size_t i = 0; i < connectors.size(); i++) {
        const std::vector<int>& connections = connectors[i].m_connections;
        for (int connection: connections) {
            stream << "    " << i << " -- " << connection << std::endl;
        }
    }
    stream << "}" << std::endl;
}

void ShapeGraph::writeAxialConnectionsAsPairsCSV(std::ostream &stream)
{
    const std::vector<Connector>& connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB" << std::endl;

    for (size_t i = 0; i < connectors.size(); i++) {
        auto& connections = connectors[i].m_connections;
        if (i != 0) stream << std::endl;
        for (auto iter = connections.begin(); iter != connections.end(); ++iter) {
            if (iter != connections.begin()) stream << std::endl;
            stream << i << "," << *iter;
        }
    }
}

void ShapeGraph::writeSegmentConnectionsAsPairsCSV(std::ostream &stream)
{
    const std::vector<Connector>& connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB,ss_weight,for_back,dir";

    // directed links
    for (size_t i = 0; i < connectors.size(); i++) {
        for (auto& segconn: connectors[i].m_forward_segconns) {
            stream << std::endl;
            stream << i << "," << segconn.first.ref
                   << "," << segconn.second
                   << "," << 0 // forward
                   << "," << int(segconn.first.dir);
        }

        for (auto& segconn: connectors[i].m_back_segconns) {
            stream << std::endl;
            stream << i << "," << segconn.first.ref
                   << "," << segconn.second
                   << "," << 1 // back
                   << "," << int(segconn.first.dir);
        }
    }
}

void ShapeGraph::unlinkAtPoint(const Point2f& unlinkPoint) {
    std::vector<Point2f> closepoints;
    std::vector<std::pair<int, int>> intersections;
    PixelRef pix = pixelate(unlinkPoint);
    std::vector<ShapeRef>& pix_shapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    auto iter = pix_shapes.begin();
    for (; iter != pix_shapes.end(); ++iter) {
       for (auto jter = iter; jter != pix_shapes.end(); ++jter) {
          auto aIter = m_shapes.find(int(iter->m_shape_ref));
          auto bIter = m_shapes.find(int(jter->m_shape_ref));
          int a = int(std::distance(m_shapes.begin(), aIter));
          int b = int(std::distance(m_shapes.begin(), bIter));
          auto& connections = m_connectors[size_t(a)].m_connections;
          if (aIter != m_shapes.end() && bIter != m_shapes.end()
                  && aIter->second.isLine() && bIter->second.isLine()
                  && std::find(connections.begin(), connections.end(), b) != connections.end()) {
             closepoints.push_back( intersection_point(aIter->second.getLine(), bIter->second.getLine(), TOLERANCE_A) );
             intersections.push_back( std::pair<int, int>(a,b) );
          }
       }
    }
    double mindist = -1.0;
    int minpair = -1;
    int j = 0;
    for (auto& closepoint: closepoints) {
       if (minpair == -1 || dist(unlinkPoint,closepoint) < mindist) {
          mindist = dist(unlinkPoint,closepoint);
          minpair = j;
       }
       j++;
    }
    if (minpair != -1) {
       auto& intersection = intersections[size_t(minpair)];
       unlinkShapes(intersection.first, intersection.second, false);
    }
    else {
       std::cerr << "eek!";
    }
}
////////////////////////////////////////////////////////////////////////////

// this unlink options was originally excised on the version 7 recode
// however, it is *very specific* to axial maps, and so have been reincluded here

void ShapeGraph::unlinkFromShapeMap(const ShapeMap& shapemap)
{
   // used to make a shape map from every axial intersection,

   // find lines in rough vincinity of unlink point, and check for the closest
   // pair to unlink:

   const std::map<int,SalaShape>& polygons = shapemap.getAllShapes();
   for (auto polygon: polygons) {
      // just use the points:
      if (polygon.second.isPoint()) {
         unlinkAtPoint(polygon.second.getPoint());
      }
   }

   // reset displayed attribute if it happens to be "Connectivity":
   int conn_col = m_attributes->getColumnIndex("Connectivity");
   if (getDisplayedAttribute() == conn_col) {
      invalidateDisplayedAttribute();
      setDisplayedAttribute(conn_col);  // <- reflect changes to connectivity counts
   }
}



///////////////////////////////////////////////////////////////////////////////

// Two ways to make a segment map

// Method 1: direct linkage of endpoints where they touch

void ShapeGraph::makeNewSegMap()
{
   // now make a connection set from the ends of lines:
   std::vector<Connector> connectionset;
   std::map<int,Line> lineset;
   for (auto shape: m_shapes) {
      if (shape.second.isLine()) {
         connectionset.push_back(Connector());
         lineset[shape.first] = shape.second.getLine();
      }
   }

   double maxdim = __max(m_region.width(),m_region.height());

   int seg_a = -1;
   for (auto seg_a_line: lineset) {
       seg_a++;
      // n.b., vector() is based on t_start and t_end, so we must use t_start and t_end here and throughout
      PixelRef pix1 = pixelate(seg_a_line.second.t_start());
      std::vector<ShapeRef> &shapes1 = m_pixel_shapes(static_cast<size_t>(pix1.y),
                                                      static_cast<size_t>(pix1.x));
      for (auto& shape: shapes1) {
         auto seg_b_iter = lineset.find(int(shape.m_shape_ref));
         int seg_b = int(std::distance(lineset.begin(), seg_b_iter));
         if (seg_b_iter != lineset.end() && seg_a < seg_b) {
            Point2f alpha = seg_a_line.second.vector();
            Point2f beta  = seg_b_iter->second.vector();
            alpha.normalise();
            beta.normalise();
            if (approxeq(seg_a_line.second.t_start(),seg_b_iter->second.t_start(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
               depthmapX::addIfNotExists(connectionset[size_t(seg_a)].m_back_segconns, SegmentRef(1,seg_b), x);
               depthmapX::addIfNotExists(connectionset[size_t(seg_b)].m_back_segconns, SegmentRef(1,seg_a), x);
            }
            if (approxeq(seg_a_line.second.t_start(),seg_b_iter->second.t_end(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(alpha,-beta),-1.0),1.0)) / M_PI);
               depthmapX::addIfNotExists(connectionset[size_t(seg_a)].m_back_segconns, SegmentRef(-1,seg_b), x);
               depthmapX::addIfNotExists(connectionset[size_t(seg_b)].m_forward_segconns, SegmentRef(1,seg_a), x);
            }
         }
      }

      PixelRef pix2 = pixelate(seg_a_line.second.t_end());
      std::vector<ShapeRef> &shapes2 = m_pixel_shapes(static_cast<size_t>(pix2.y),
                                                      static_cast<size_t>(pix2.x));
      for (auto& shape: shapes2) {
         auto seg_b_iter = lineset.find(int(shape.m_shape_ref));
         int seg_b = int(std::distance(lineset.begin(), seg_b_iter));
         if (seg_b_iter != lineset.end() && seg_a < seg_b) {
            Point2f alpha = seg_a_line.second.vector();
            Point2f beta  = seg_b_iter->second.vector();
            alpha.normalise();
            beta.normalise();
            if (approxeq(seg_a_line.second.t_end(),seg_b_iter->second.t_start(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(-alpha,beta),-1.0),1.0)) / M_PI);
               depthmapX::addIfNotExists(connectionset[size_t(seg_a)].m_forward_segconns, SegmentRef(1,seg_b), x);
               depthmapX::addIfNotExists(connectionset[size_t(seg_b)].m_back_segconns, SegmentRef(-1,seg_a), x);
            }
            if (approxeq(seg_a_line.second.t_end(),seg_b_iter->second.t_end(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(-alpha,-beta),-1.0),1.0)) / M_PI);
               depthmapX::addIfNotExists(connectionset[size_t(seg_a)].m_forward_segconns, SegmentRef(-1,seg_b), x);
               depthmapX::addIfNotExists(connectionset[size_t(seg_b)].m_forward_segconns, SegmentRef(-1,seg_a), x);
            }
         }
      }
   }

   // initialise attributes now separated from making the connections
   makeSegmentConnections(connectionset);
}

// Method 2: Making a segment map (in two stages)

// One: take the original axial map and split it up
// (note: you need to start from an axial map,
//  but the map could have been created from a road-centre-line
//  graph or equivalent -- reason is that you might want to
//  preserve unlinks in your angular mapping)

// A "linetest" is used in order to use the test component to
// identify the original axial line this line segment is
// associated with

void ShapeGraph::makeSegmentMap(std::vector<Line>& lines, std::vector<Connector>& connectors, double stubremoval)
{
   // the first (key) pair is the line / line intersection, second is the pair of associated segments for the first line
   std::map<OrderedIntPair, std::pair<int, int>> segmentlist;

   // this code relies on the polygon order being the same as the connections

   auto iter = m_shapes.begin();
   for (size_t i = 0; i < m_connectors.size(); i++) {
      auto shape = iter->second;
      iter++;
      if (!shape.isLine()) {
         continue;
      }
      const Line& line = shape.getLine();
      std::vector<std::pair<double,int> > breaks; // this is a vector instead of a map because the
                                                  // original code allowed for duplicate keys
      int axis = line.width() >= line.height() ? XAXIS : YAXIS;
      // we need the breaks ordered from start to end of the line
      // this is automatic for XAXIS, but on YAXIS, need to know
      // if the line is ascending or decending
      int parity = (axis == XAXIS) ? 1 : line.sign();

      auto& connections = m_connectors[i].m_connections;
      for (size_t j = 0; j < connections.size(); j++) {
         // find the intersection point and add...
         // note: more than one break at the same place allowed
         auto shapeJ = depthmapX::getMapAtIndex(m_shapes, connections[j])->second;
         if (static_cast<int>(i) != connections[j] && shapeJ.isLine()) {
            breaks.push_back(std::make_pair(parity * line.intersection_point( shapeJ.getLine(), axis, TOLERANCE_A ),
                                         connections[j]));
         }
      }
      std::sort(breaks.begin(), breaks.end());
      // okay, now we have a list from one end of the other of lines this line connects with
      Point2f lastpoint = line.start();
      int seg_a = -1, seg_b = -1;
      double neardist;
      // TOLERANCE_C is introduced as of 01.08.2008 although it is a fix to a bug first
      // found in July 2006.  It has been set "high" deliberately (1e-6 = a millionth of the line height / width)
      // in order to catch small errors made by operators or floating point errors in other systems
      // when drawing, for example, three axial lines intersecting
      if  (stubremoval == 0.0) {
         // if 0, convert to tolerance
         stubremoval = TOLERANCE_C;
      }
      neardist = (axis == XAXIS) ? (line.width() * stubremoval) : (line.height() * stubremoval);
      double overlapdist = (axis == XAXIS) ? (line.width() * TOLERANCE_C) : (line.height() * TOLERANCE_C);
      //
      for (auto breaksIter = breaks.begin(); breaksIter != breaks.end();) {
         std::vector<int> keylist;
         if (seg_a == -1) {
            Point2f thispoint = line.point_on_line(parity * breaksIter->first,axis);
            if (fabs(parity * breaksIter->first - line.start()[axis]) < neardist) {
               seg_a = -1;
               lastpoint = thispoint;
            }
            else  {
               Line segment_a(line.start(),thispoint);
               lines.push_back(segment_a);
               connectors.push_back(Connector(i));
               seg_a = lines.size() - 1;
            }
            lastpoint = thispoint;
         }
         //
         double here = parity * breaksIter->first;
         while (breaksIter != breaks.end() && fabs(parity * breaksIter->first - here) < overlapdist) {
            keylist.push_back(breaksIter->second);
            ++breaksIter;
         }
         //
         if (breaksIter == breaks.end() && fabs(line.end()[axis] - parity * breaks.rbegin()->first) < neardist) {
            seg_b = -1;
         }
         else {
            Point2f thispoint;
            if (breaksIter != breaks.end()) {
               thispoint = line.point_on_line(parity * breaksIter->first,axis);
            }
            else {
               thispoint = line.end();
            }
            Line segment_b(lastpoint,thispoint);
            lines.push_back(segment_b);
            connectors.push_back(Connector(i));
            seg_b = lines.size() - 1;
            //
            lastpoint = thispoint;
         }
         //
         for (size_t j = 0; j < keylist.size(); j++) {
         //
            if (keylist[j] < (int)i) {
               // other line already segmented, look up in segment list,
               // and join segments together nicely
               auto segIter = segmentlist.find(OrderedIntPair(keylist[j],i));
               if (segIter != segmentlist.end()) {   // <- if it isn't -1 something has gone badly wrong!
                  int seg_1 = segIter->second.first;
                  int seg_2 = segIter->second.second;
                  if (seg_a != -1) {
                     if (seg_1 != -1) {
                        Point2f alpha = lines[size_t(seg_a)].start() - lines[size_t(seg_a)].end();
                        Point2f beta  = lines[size_t(seg_1)].start() - lines[size_t(seg_1)].end();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        depthmapX::addIfNotExists(connectors[size_t(seg_a)].m_forward_segconns, SegmentRef(-1,seg_1), x);
                        depthmapX::addIfNotExists(connectors[size_t(seg_1)].m_forward_segconns, SegmentRef(-1,seg_a), x);
                     }
                     if (seg_2 != -1) {
                        Point2f alpha = lines[size_t(seg_a)].start() - lines[size_t(seg_a)].end();
                        Point2f beta  = lines[size_t(seg_2)].end() - lines[size_t(seg_2)].start();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        depthmapX::addIfNotExists(connectors[size_t(seg_a)].m_forward_segconns, SegmentRef(1,seg_2), x);
                        depthmapX::addIfNotExists(connectors[size_t(seg_2)].m_back_segconns, SegmentRef(-1,seg_a), x);
                     }
                  }
                  if (seg_b != -1) {
                     if (seg_1 != -1) {
                        Point2f alpha = lines[size_t(seg_b)].end() - lines[size_t(seg_b)].start();
                        Point2f beta  = lines[size_t(seg_1)].start() - lines[size_t(seg_1)].end();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        depthmapX::addIfNotExists(connectors[size_t(seg_b)].m_back_segconns, SegmentRef(-1,seg_1), x);
                        depthmapX::addIfNotExists(connectors[size_t(seg_1)].m_forward_segconns, SegmentRef(1,seg_b), x);
                     }
                     if (seg_2 != -1) {
                        Point2f alpha = lines[size_t(seg_b)].end() - lines[size_t(seg_b)].start();
                        Point2f beta  = lines[size_t(seg_2)].end() - lines[size_t(seg_2)].start();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        depthmapX::addIfNotExists(connectors[size_t(seg_b)].m_back_segconns, SegmentRef(1,seg_2), x);
                        depthmapX::addIfNotExists(connectors[size_t(seg_2)].m_back_segconns, SegmentRef(1,seg_b), x);
                     }
                  }
               }
            }
            else {
               // other line still to be segmented, add ourselves to segment list
               // to be added later
               segmentlist.insert(std::make_pair( OrderedIntPair(i,keylist[j]), std::pair<int, int>(seg_a,seg_b) ));
            }
         }
         if (seg_a != -1 && seg_b != -1) {
            depthmapX::addIfNotExists(connectors[size_t(seg_a)].m_forward_segconns, SegmentRef(1,seg_b), 0.0f);
            depthmapX::addIfNotExists(connectors[size_t(seg_b)].m_back_segconns, SegmentRef(-1,seg_a), 0.0f);
         }
         seg_a = seg_b;
      }
   }
}

void ShapeGraph::initialiseAttributesSegment()
{
    m_attributes->clear();

    // note, expects these in alphabetical order to preserve numbering:
    m_attributes->insertOrResetLockedColumn("Axial Line Ref");
    m_attributes->insertOrResetLockedColumn("Segment Length");
}

// now segments and connections are listed separately...
// put them together in a new map

void ShapeGraph::makeSegmentConnections(std::vector<Connector>& connectionset)
{
   m_connectors.clear();

   // note, expects these in alphabetical order to preserve numbering:
   int w_conn_col = m_attributes->getOrInsertColumn("Angular Connectivity");
   int uw_conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");

   int ref_col = m_attributes->getColumnIndex("Axial Line Ref");
   int leng_col = m_attributes->getColumnIndex("Segment Length");

   int i = -1;
   for (auto shape: m_shapes) {
       i++;
       Connector& connector = connectionset[size_t(i)];
      AttributeRow& row = m_attributes->getRow(AttributeKey(shape.first));

      row.setValue(ref_col, float(connector.m_segment_axialref));
      row.setValue(leng_col, float(shape.second.getLine().length()));

      // all indices should match... (including lineset/connectionset versus m_shapes)
      m_connectors.push_back( connector );
      float total_weight = 0.0f;
      for (auto iter = connector.m_forward_segconns.begin(); iter != connector.m_forward_segconns.end(); ++iter) {
         total_weight += iter->second;
      }
      for (auto iter = connector.m_back_segconns.begin(); iter != connector.m_back_segconns.end(); ++iter) {
         total_weight += iter->second;
      }
      row.setValue(w_conn_col, float(total_weight));
      row.setValue(uw_conn_col, float(connector.m_forward_segconns.size() + connector.m_back_segconns.size()));

      // free up connectionset as we go along:
      connectionset[size_t(i)] = Connector();
   }

   m_displayed_attribute = -2; // <- override if it's already showing
   setDisplayedAttribute(uw_conn_col);
}

// this pushes axial map values to a segment map
// the segment map is 'this', the axial map is passed:

void ShapeGraph::pushAxialValues(ShapeGraph& axialmap)
{
   if (!m_attributes->hasColumn("Axial Line Ref")) {
      // this should never happen
      // AT: I am converting this to throw an error
      throw depthmapX::RuntimeException("Axial line ref does not exist");
   }

   std::vector<int> colindices;
   for (size_t i = 0; i < axialmap.m_attributes->getNumColumns(); i++) {
      std::string colname = std::string("Axial ") + axialmap.m_attributes->getColumnName(i);
      colindices.push_back(m_attributes->getOrInsertColumn(colname));
   }
   for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
      int axialref = (int) iter->getRow().getValue("Axial Line Ref");
      // P.K: The original code here got the index of the row, but the column
      // "Axial Line Ref" should actually contain keys, not indices
      AttributeRow& row = axialmap.m_attributes->getRow(AttributeKey(axialref));
      for (size_t k = 0; k < axialmap.m_attributes->getNumColumns(); k++) {
         float val = row.getValue(k);
         // need to look up the column index:
         iter->getRow().setValue(colindices[k],val);
      }
   }
}
