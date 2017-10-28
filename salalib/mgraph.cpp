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



// The meta graph 

#include <math.h>
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/pafmath.h>
#include <genlib/p2dpoly.h>
#include <genlib/dxfp.h>
#include <genlib/comm.h>

#include "isovist.h"
#include "ntfp.h"
#include "tigerp.h"
#include "gmlmap.h"
#include <salalib/mgraph.h>

// shouldn't really include this -- required for node in PushValuesToLayer
#include <salalib/ngraph.h>

// Quick mod - TV
#pragma warning (disable: 4800)

///////////////////////////////////////////////////////////////////////////////////

MetaGraph::MetaGraph()
{ 
   m_state = 0; 
   m_lock = NULL;
   m_view_class = VIEWNONE;
   m_file_version = -1; // <- if unsaved, file version is -1

   // Standard layers no longer added: the gates layer will be initialised with the first push to layer,
   // or when made from axial lines.

   m_attr_conv_table = NULL;

   // whether or not showing text / grid saved with file:
   m_showtext = false;
   m_showgrid = false;

   // bsp tree for making isovists:
   m_bsp_tree = false;
   m_bsp_root = NULL;
}

MetaGraph::~MetaGraph()
{
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;
}

QtRegion MetaGraph::getBoundingBox() const
{
   QtRegion bounds = SuperSpacePixel::getRegion();
   if (bounds.isNull() && ((getState() & MetaGraph::SHAPEGRAPHS) == MetaGraph::SHAPEGRAPHS)) {
      bounds = m_shape_graphs.getBoundingBox();
   }
   if (bounds.isNull() && ((getState() & MetaGraph::DATAMAPS) == MetaGraph::DATAMAPS)) {
      bounds = m_data_maps.getBoundingBox();
   }
   return bounds;
}

bool MetaGraph::setViewClass(int command)
{
   if (command < 0x10) {
      throw ("Use with a show command, not a view class type");
   }
   if ((command & (SHOWHIDEVGA | SHOWVGATOP)) && (~m_state & POINTMAPS)) 
      return false; 
   if ((command & (SHOWHIDEAXIAL | SHOWAXIALTOP)) && (~m_state & SHAPEGRAPHS)) 
      return false; 
   if ((command & (SHOWHIDESHAPE | SHOWSHAPETOP)) && (~m_state & DATAMAPS)) 
      return false; 
   switch (command) {
   case SHOWHIDEVGA:
      if (m_view_class & (VIEWVGA | VIEWBACKVGA)) {
         m_view_class &= ~(VIEWVGA | VIEWBACKVGA);
         if (m_view_class & VIEWBACKAXIAL) {
            m_view_class ^= (VIEWAXIAL | VIEWBACKAXIAL);
         }
         else if (m_view_class & VIEWBACKDATA) {
            m_view_class ^= (VIEWDATA | VIEWBACKDATA);
         }
      }
      else if (m_view_class & (VIEWAXIAL | VIEWDATA)) {
         m_view_class &= ~(VIEWBACKAXIAL | VIEWBACKDATA);
         m_view_class |= VIEWBACKVGA;
      }
      else {
         m_view_class |= VIEWVGA;
      }
      break;
   case SHOWHIDEAXIAL:
      if (m_view_class & (VIEWAXIAL | VIEWBACKAXIAL)) {
         m_view_class &= ~(VIEWAXIAL | VIEWBACKAXIAL);
         if (m_view_class & VIEWBACKVGA) {
            m_view_class ^= (VIEWVGA | VIEWBACKVGA);
         }
         else if (m_view_class & VIEWBACKDATA) {
            m_view_class ^= (VIEWDATA | VIEWBACKDATA);
         }
      }
      else if (m_view_class & (VIEWVGA | VIEWDATA)) {
         m_view_class &= ~(VIEWBACKVGA | VIEWBACKDATA);
         m_view_class |= VIEWBACKAXIAL;
      }
      else {
         m_view_class |= VIEWAXIAL;
      }
      break;
   case SHOWHIDESHAPE:
      if (m_view_class & (VIEWDATA | VIEWBACKDATA)) {
         m_view_class &= ~(VIEWDATA | VIEWBACKDATA);
         if (m_view_class & VIEWBACKVGA) {
            m_view_class ^= (VIEWVGA | VIEWBACKVGA);
         }
         else if (m_view_class & VIEWBACKAXIAL) {
            m_view_class ^= (VIEWAXIAL | VIEWBACKAXIAL);
         }
      }
      else if (m_view_class & (VIEWVGA | VIEWAXIAL)) {
         m_view_class &= ~(VIEWBACKVGA | VIEWBACKAXIAL);
         m_view_class |= VIEWBACKDATA;
      }
      else {
         m_view_class |= VIEWDATA;
      }
      break;
   case SHOWVGATOP:
      if (m_view_class & VIEWAXIAL) {
         m_view_class = VIEWBACKAXIAL | VIEWVGA;
      }
      else if (m_view_class & VIEWDATA) {
         m_view_class = VIEWBACKDATA | VIEWVGA;
      }
      else {
         m_view_class = VIEWVGA | (m_view_class & (VIEWBACKAXIAL | VIEWBACKDATA));
      }
      break;
   case SHOWAXIALTOP:
      if (m_view_class & VIEWVGA) {
         m_view_class = VIEWBACKVGA | VIEWAXIAL;
      }
      else if (m_view_class & VIEWDATA) {
         m_view_class = VIEWBACKDATA | VIEWAXIAL;
      }
      else {
         m_view_class = VIEWAXIAL | (m_view_class & (VIEWBACKVGA | VIEWBACKDATA));
      }
      break;
   case SHOWSHAPETOP:
      if (m_view_class & VIEWVGA) {
         m_view_class = VIEWBACKVGA | VIEWDATA;
      }
      else if (m_view_class & VIEWAXIAL) {
         m_view_class = VIEWBACKAXIAL | VIEWDATA;
      }
      else {
         m_view_class = VIEWDATA | (m_view_class & (VIEWBACKVGA | VIEWBACKAXIAL));
      }
      break;
   }
   return true;
}

double MetaGraph::getLocationValue(const Point2f& point)
{
   // this varies according to whether axial or vga information is displayed on top
   double val = -2;

   if (viewingProcessedPoints()) {
      val = getDisplayedPointMap().getLocationValue(point);
   }
   else if (viewingProcessedLines()) {
      val = m_shape_graphs.getDisplayedMap().getLocationValue(point);
   }
   else if (viewingProcessedShapes()) {
      val = m_data_maps.getDisplayedMap().getLocationValue(point);
   }

   return val;
}

void MetaGraph::copyLineData(const SuperSpacePixel& meta)
{
   m_state &= ~LINEDATA;

   *(SuperSpacePixel *)this = meta;

   PointMaps::setSpacePixel( (SuperSpacePixel *) this );   // <- also helpfully gives PointMap the space pixel

   m_state |= LINEDATA;
}

void MetaGraph::copyPointMap(const PointMap& meta)
{
   m_state &= ~POINTMAPS;

   *(PointMap *)this = meta;

   m_state |= POINTMAPS;
}

bool MetaGraph::setGrid( double spacing, const Point2f& offset )
{
   m_state &= ~POINTMAPS;

   getDisplayedPointMap().setSpacePixel( (SuperSpacePixel *) this );
   getDisplayedPointMap().setGrid( spacing, offset );

   m_state |= POINTMAPS;

   // just reassert that we should be viewing this (since set grid is essentially a "new point map")
   setViewClass(SHOWVGATOP);

   return true;
}

// AV TV // semifilled
bool MetaGraph::makePoints( const Point2f& p, int fill_type , Communicator *communicator )
{
//   m_state &= ~POINTS;

   try {
      getDisplayedPointMap().makePoints( p, fill_type, communicator );
   }
   catch (Communicator::CancelledException) {

      // By this stage points almost certainly exist,
      // To avoid problems, just say points exist:
      m_state |= POINTMAPS;
      
      return false;
   }

//   m_state |= POINTS;

   return true;
}

bool MetaGraph::undoPoints()
{
   bool b_return = getDisplayedPointMap().undoPoints();
/*   
   if (PointMap::m_point_count == 0) {
      m_state &= ~POINTS;
   }
   else {
      m_state |= POINTS;
   }
*/
   return b_return;
}

bool MetaGraph::clearPoints()
{
   bool b_return = getDisplayedPointMap().clearPoints();
/*   
   if (PointMap::m_point_count == 0) {
      m_state &= ~POINTS;
   }
*/
   return b_return;
}

bool MetaGraph::makeGraph( Communicator *communicator, int algorithm, double maxdist )
{
   // this is essentially a version tag, and remains for historical reasons:
   m_state |= ANGULARGRAPH;

   bool retvar = false;
   
   try {
      // algorithm is now used for boundary graph option (as a simple boolean)
      retvar = getDisplayedPointMap().sparkGraph2(communicator, (algorithm != 0), maxdist);
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   if (retvar) {
      setViewClass(SHOWVGATOP);
   }

   return retvar;
}

bool MetaGraph::analyseGraph( Communicator *communicator, Options options , bool simple_version )   // <- options copied to keep thread safe
{
   bool retvar = false;

   if (options.point_depth_selection) {
      if (m_view_class & VIEWVGA && !PointMaps::getDisplayedPointMap().isSelected()) {
         return false;
      }
      else if (m_view_class & VIEWAXIAL && !m_shape_graphs.getDisplayedMap().isSelected()) {
         return false;
      }
   }

   try {
      retvar = true;
      if (options.point_depth_selection == 1) {
         if (m_view_class & VIEWVGA) {
            PointMaps::getDisplayedPointMap().analyseVisualPointDepth( communicator );
         }
         else if (m_view_class & VIEWAXIAL) {
            if (!m_shape_graphs.getDisplayedMap().isSegmentMap()) {
               m_shape_graphs.getDisplayedMap().stepdepth( communicator );
            }
            else {
               m_shape_graphs.getDisplayedMap().angularstepdepth( communicator );
            }
         }
         // REPLACES:
         // Graph::calculate_point_depth_matrix( communicator );
      }
      else if (options.point_depth_selection == 2) {
         if (m_view_class & VIEWVGA) {
            PointMaps::getDisplayedPointMap().analyseMetricPointDepth( communicator );
         }
         else if (m_view_class & VIEWAXIAL && m_shape_graphs.getDisplayedMap().isSegmentMap()) {
            m_shape_graphs.getDisplayedMap().analyseTopoMetPD( communicator, 1 ); // 1 is metric step depth
         }
      }
      else if (options.point_depth_selection == 3) {
         PointMaps::getDisplayedPointMap().analyseAngularPointDepth( communicator );
      }
      else if (options.point_depth_selection == 4) {
         if (m_view_class & VIEWVGA) {
            PointMaps::getDisplayedPointMap().binDisplay( communicator );
         }
         else if (m_view_class & VIEWAXIAL && m_shape_graphs.getDisplayedMap().isSegmentMap()) {
            m_shape_graphs.getDisplayedMap().analyseTopoMetPD( communicator, 0 ); // 0 is topological step depth
         }
      }
      else if (options.output_type == Options::OUTPUT_ISOVIST) {
         PointMaps::getDisplayedPointMap().analyseIsovist( communicator, *this, simple_version );
      }
      else if (options.output_type == Options::OUTPUT_VISUAL) {
         PointMaps::getDisplayedPointMap().analyseVisual( communicator, options, simple_version );
         // REPLACES:
         // Graph::calculate_depth_matrix( communicator, options, output_graph );
      }
      else if (options.output_type == Options::OUTPUT_METRIC) {
         PointMaps::getDisplayedPointMap().analyseMetric( communicator, options );
      }
      else if (options.output_type == Options::OUTPUT_ANGULAR) {
         PointMaps::getDisplayedPointMap().analyseAngular( communicator, options );
      }
      else if (options.output_type == Options::OUTPUT_THRU_VISION) {
         retvar = analyseThruVision( communicator, options.gatelayer );
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   return retvar;
}

//////////////////////////////////////////////////////////////////

bool MetaGraph::isEditableMap()
{
   if (m_view_class & VIEWAXIAL) {
      return getDisplayedShapeGraph().isEditable();
   }
   else if (m_view_class & VIEWDATA) {
      return getDisplayedDataMap().isEditable();
   }
   // still to do: allow editing of drawing layers
   return false;
}

ShapeMap& MetaGraph::getEditableMap()
{
   ShapeMap *map = NULL;
   if (m_view_class & VIEWAXIAL) {
      map = &(getDisplayedShapeGraph());
   }
   else if (m_view_class & VIEWDATA) {
      map = &(getDisplayedDataMap());
   }
   else {
      // still to do: allow editing of drawing layers
   }
   if (map == NULL || !map->isEditable()) {
      throw 0;
   }
   return *map;
}

bool MetaGraph::makeShape(const Line& line)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return (map.makeLineShape(line,true) != -1);
}

bool MetaGraph::polyBegin(const Line& line)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return (map.polyBegin(line) != -1);
}

bool MetaGraph::polyAppend(const Point2f& point)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyAppend(point);
}

bool MetaGraph::polyClose() 
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyClose();
}

bool MetaGraph::polyCancel()
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyCancel();
}

bool MetaGraph::moveSelShape(const Line& line)
{
   bool retvar = false;
   if (m_view_class & VIEWAXIAL) {
      ShapeGraph& map = getDisplayedShapeGraph();
      if (!map.isEditable()) {
         return false;
      }
      if (map.getSelCount() > 1) {
         return false;
      }
      // note, selection sets currently store rowids not uids, but moveShape sensibly works off uid:
      int rowid = map.getSelSet()[0];
      retvar = map.moveShape(map.getIndex(rowid),line);
      if (retvar) {
         map.clearSel();
      }
   }
   else if (m_view_class & VIEWDATA) {
      ShapeMap& map = getDisplayedDataMap();
      if (!map.isEditable()) {
         return false;
      }
      if (map.getSelCount() > 1) {
         return false;
      }
      // note, selection sets currently store rowids not uids, but moveShape sensibly works off uid:
      int rowid = map.getSelSet()[0];
      retvar = map.moveShape(map.getIndex(rowid),line);
      if (retvar) {
         map.clearSel();
      }
   }
   return retvar;
}

//////////////////////////////////////////////////////////////////

// returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
int MetaGraph::makeIsovist(Communicator *communicator, const Point2f& p, double startangle, double endangle, bool simple_version)
{
   int retvar = 0;
   // first make isovist
   Isovist iso;

   if (makeBSPtree(communicator)) {
      retvar = 1;
      iso.makeit(m_bsp_root,p,SuperSpacePixel::m_region, startangle, endangle);
      int shapelayer = m_data_maps.getMapRef("Isovists");
      if (shapelayer == -1) {
         shapelayer = m_data_maps.addMap("Isovists",ShapeMap::DATAMAP);
         m_state |= DATAMAPS;
         retvar = 2;
      }
      ShapeMap& map = m_data_maps.getMap(shapelayer);
      // false: closed polygon, true: isovist
      int polyref = map.makePolyShape(iso.getPolygon(),false);  
      map.getAllShapes().search(polyref).setCentroid(p);
      map.overrideDisplayedAttribute(-2);
      map.setDisplayedAttribute(-1);
      setViewClass(SHOWSHAPETOP);
      AttributeTable& table = map.getAttributeTable();
      int row = table.getRowid(polyref);
      iso.setData(table,row, simple_version);
   }
   return retvar;
}

static pair<double,double> startendangle( Point2f vec, double fov)
{
   pair<double,double> angles;
   // n.b. you must normalise this before getting the angle!
   vec.normalise();
   angles.first = vec.angle() - fov / 2.0;
   angles.second = vec.angle() + fov / 2.0;
   if (angles.first < 0.0) 
      angles.first += 2.0 * M_PI;
   if (angles.second > 2.0 * M_PI)
      angles.second -= 2.0 * M_PI;
   return angles;
}

// returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
int MetaGraph::makeIsovistPath(Communicator *communicator, double fov, bool simple_version)
{
   int retvar = 0;

   // must be showing a suitable map -- that is, one which may have polylines or lines
   ShapeMap *map = NULL, *isovists = NULL;
   int isovistmapref = -1;
   int viewclass = getViewClass() & VIEWFRONT;
   if (viewclass == VIEWAXIAL) {
      map = &getDisplayedShapeGraph();
   }
   else if (viewclass == VIEWDATA) {
      map = &getDisplayedDataMap();
   }
   else {
      return 0;
   }

   // must have a selection: the selected shapes will form the set from which to create the isovist paths
   if (!map->isSelected()) {
      return 0;
   }

   bool first = true;
   if (makeBSPtree(communicator)) {
      pvecint& selset = map->getSelSet();
      pqmap<int,SalaShape>& shapes = map->getAllShapes();
      for (size_t i = 0; i < selset.size(); i++) {
         SalaShape& path = shapes.value(selset[i]);
         if (path.isLine() || path.isPolyLine()) {
            if (first) {
               retvar = 1;
               isovistmapref = m_data_maps.getMapRef("Isovists");
               if (isovistmapref == -1) {
                  isovistmapref = m_data_maps.addMap("Isovists",ShapeMap::DATAMAP);
                  retvar = 2;
               }
               isovists = &(m_data_maps.getMap(isovistmapref));
               first = false;
            }
            // now make an isovist:
            Isovist iso;
            // 
            pair<double,double> angles;
            angles.first = 0.0;
            angles.second = 0.0;
            //
            if (path.isLine()) {
               Point2f start = path.getLine().t_start();
               Point2f vec = path.getLine().vector();
               if (fov < 2.0 * M_PI) {
                  angles = startendangle(vec, fov);
               }
               iso.makeit(m_bsp_root,start,SuperSpacePixel::m_region, angles.first, angles.second);
               int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
               isovists->getAllShapes().search(polyref).setCentroid(start);
               AttributeTable& table = isovists->getAttributeTable();
               int row = table.getRowid(polyref);
               iso.setData(table,row, simple_version);
            }
            else {
               for (size_t i = 0; i < path.size() - 1; i++) {
                  Line li = Line(path[i],path[i+1]);
                  Point2f start = li.t_start();
                  Point2f vec = li.vector();
                  if (fov < 2.0 * M_PI) {
                     angles = startendangle(vec, fov);
                  }
                  iso.makeit(m_bsp_root,start,SuperSpacePixel::m_region, angles.first, angles.second);
                  int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
                  isovists->getAllShapes().search(polyref).setCentroid(start);
                  AttributeTable& table = isovists->getAttributeTable();
                  int row = table.getRowid(polyref);
                  iso.setData(table,row, simple_version);
               }
            }
         }
      }
      if (isovists) {
         isovists->overrideDisplayedAttribute(-2);
         isovists->setDisplayedAttribute(-1);
         m_data_maps.setDisplayedMapRef(isovistmapref);
      }
   }
   return retvar;
}

// this version uses your own isovist (and assumes no communicator required for BSP tree
bool MetaGraph::makeIsovist(const Point2f& p, Isovist& iso)
{
   if (makeBSPtree()) {
      iso.makeit(m_bsp_root,p,SuperSpacePixel::m_region);
      return true;
   }
   return false;
}

bool MetaGraph::makeBSPtree(Communicator *communicator)
{
   if (m_bsp_tree) {
      return true;
   }

   prefvec<TaggedLine> partitionlines;
   for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
      for (size_t j = 0; j < SuperSpacePixel::at(i).size(); j++) {
         // chooses the first editable layer it can find:
         if (SuperSpacePixel::at(i).at(j).isShown()) {
            for (size_t k = 0; k < SuperSpacePixel::at(i).at(j).getAllShapes().size(); k++) {
               SalaShape& shape = SuperSpacePixel::at(i).at(j).getAllShapes().at(k);
               // I'm not sure what the tagging was meant for any more, 
               // tagging at the moment tags the *polygon* it was original attached to
               // must check it is not a zero length line:
               if (shape.isLine() && shape.getLine().length() > 0.0) {
                  partitionlines.push_back(TaggedLine(shape.getLine(),k));
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     if (shape[n] != shape[n+1]) {
                        partitionlines.push_back(TaggedLine(Line(shape[n],shape[n+1]),k));
                     }
                  }
                  if (shape.isPolygon() && shape.head() != shape.tail()) {
                     partitionlines.push_back(TaggedLine(Line(shape.tail(),shape.head()),k));
                  }
               }
            }
         }
      }
   }

   if (partitionlines.size()) {
      //
      // Now we'll try the BSP tree:
      //
      if (m_bsp_root) {
         delete m_bsp_root;
         m_bsp_root = NULL;
      }
      m_bsp_root = new BSPNode();

      // Quick mod - TV
#if defined(_WIN32)      
      __time64_t atime = 0;
#else
      time_t atime = 0;
#endif      
      communicator->CommPostMessage( Communicator::NUM_RECORDS, partitionlines.size() );
      qtimer( atime, 0 );

      try {
         m_bsp_root->make(communicator,atime,partitionlines,NULL);
         m_bsp_tree = true;
      } 
      catch (Communicator::CancelledException) {
         m_bsp_tree = false;
         // probably best to delete the half made bastard of a tree:
         delete m_bsp_root;
         m_bsp_root = NULL;
      }
   }

   partitionlines.clear();

   return m_bsp_tree;
}

//////////////////////////////////////////////////////////////////

int MetaGraph::addShapeGraph(const std::string& name, int type)
{
   int mapref= m_shape_graphs.addMap(name,type);
   m_state |= SHAPEGRAPHS;
   setViewClass(SHOWAXIALTOP);
   // add a couple of default columns:
   AttributeTable& table = m_shape_graphs.getMap(mapref).getAttributeTable();
   table.insertLockedColumn("Connectivity");
   if ((type & ShapeMap::LINEMAP) != 0) {
      table.insertLockedColumn("Line Length");
   }
   return mapref;
}
int MetaGraph::addShapeMap(const std::string& name)
{
   int ref = m_data_maps.addMap(name,ShapeMap::DATAMAP);
   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);
   return ref;
}
void MetaGraph::removeDisplayedMap()
{
   int ref = getDisplayedMapRef();
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      PointMaps::removeMap(ref);
      if (PointMaps::maps_vector.size() == 0) {
         setViewClass(SHOWHIDEVGA);
         m_state &= ~POINTMAPS;
      }
      break;
   case VIEWAXIAL:
      m_shape_graphs.removeMap(ref);
      if (m_shape_graphs.getMapCount() == 0) {
         setViewClass(SHOWHIDEAXIAL);
         m_state &= ~SHAPEGRAPHS;
      }
      break;
   case VIEWDATA:
      m_data_maps.removeMap(ref);
      if (m_data_maps.getMapCount() == 0) {
         setViewClass(SHOWHIDESHAPE);
         m_state &= ~DATAMAPS;
      }
      break;
   }
}


//////////////////////////////////////////////////////////////////


bool MetaGraph::convertDrawingToAxial(Communicator *comm, std::string layer_name)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDrawingToAxial( comm, layer_name, (SuperSpacePixel&) *this );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDataToAxial( comm, layer_name, m_data_maps.getDisplayedMap(), pushvalues );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         m_data_maps.removeMap( m_data_maps.getDisplayedMapRef() );
         if (m_data_maps.getMapCount() == 0) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

// typeflag: -1 convert drawing to convex, 0 or 1, convert data to convex (1 is pushvalues)
bool MetaGraph::convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS; // and convex maps...

   bool retvar = false;
   
   try {
      int mapref;
      if (typeflag == -1) {
         mapref = m_shape_graphs.convertDrawingToConvex( comm, layer_name, (SuperSpacePixel&) *this );
      }
      else {
         mapref = m_shape_graphs.convertDataToConvex( comm, layer_name, m_data_maps.getDisplayedMap(), (typeflag != 0) );
      }
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (typeflag != -1 && !keeporiginal) {
         m_data_maps.removeMap( m_data_maps.getDisplayedMapRef() );
         if (m_data_maps.getMapCount() == 0) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDrawingToSegment(Communicator *comm, std::string layer_name)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDrawingToSegment( comm, layer_name, (SuperSpacePixel&) *this );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;
   
   try {
      int mapref = m_shape_graphs.convertDataToSegment( comm, layer_name, m_data_maps.getDisplayedMap(), pushvalues );
      if (mapref != -1) {
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         m_data_maps.removeMap( m_data_maps.getDisplayedMapRef() );
         if (m_data_maps.getMapCount() == 0) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

// note: type flag says whether this is graph to data map or drawing to data map

bool MetaGraph::convertToData(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag)
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   bool retvar = false;
   
   try {
      // This should be much easier than before,
      // simply move the shapes from the drawing layer
      // note however that more than one layer might be combined:
      // create map layer...
      int destmapref = m_data_maps.addMap(layer_name,ShapeMap::DATAMAP);
      ShapeMap& destmap = m_data_maps.getMap(destmapref);
      AttributeTable& table = destmap.getAttributeTable();
      int count = 0;
      //
      // drawing to data
      if (typeflag == -1) {
         int layercol = destmap.addAttribute("Drawing Layer");
         // add all visible layers to the set of map:
         for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
            for (size_t j = 0; j < SuperSpacePixel::at(i).size(); j++) {
               if (SuperSpacePixel::at(i).at(j).isShown()) {
                  for (size_t k = 0; k < SuperSpacePixel::at(i).at(j).getAllShapes().size(); k++) {
                     int key = destmap.makeShape(SuperSpacePixel::at(i).at(j).getAllShapes().at(k));
                     table.setValue(table.getRowid(key),layercol,float(j+1));
                     count++;
                  }
                  SuperSpacePixel::at(i).at(j).setShow(false);
               }
            }
         }
      }
      // convex, axial or segment graph to data (similar)
      else {
         ShapeGraph& sourcemap = getDisplayedShapeGraph();
         count = sourcemap.getShapeCount();
         // take viewed graph and push all geometry to it (since it is *all* geometry, pushing is easy)
         int copyflag = (typeflag == 0) ? (ShapeMap::COPY_GEOMETRY) : (ShapeMap::COPY_GEOMETRY | ShapeMap::COPY_ATTRIBUTES);
         destmap.copy(sourcemap, copyflag);
      }
      //
      if (count == 0) {
         // if no objects converted then a crash is caused, so remove it:
         m_data_maps.removeMap(destmapref);
         retvar = false;
      }
      else {
         // we can stop here! -- remember to set up display:
         m_data_maps.setDisplayedMapRef(destmapref);
         destmap.invalidateDisplayedAttribute();
         destmap.setDisplayedAttribute(-1);
         retvar = true;
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (typeflag != -1 && !keeporiginal) {
         m_shape_graphs.removeMap( m_shape_graphs.getDisplayedMapRef() );
         if (m_shape_graphs.getMapCount() == 0) {
            setViewClass(SHOWHIDEAXIAL);
            m_state &= ~SHAPEGRAPHS;
         }
      }
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return retvar;
}

bool MetaGraph::convertToDrawing(Communicator *comm, std::string layer_name, int typeflag)
{
   bool retvar = false;

   int oldstate = m_state;

   m_state &= ~LINEDATA;

   try {
      const ShapeMap *sourcemap;
      if (typeflag == 0) {
         sourcemap = &(getDisplayedDataMap());
      }
      else {
         sourcemap = &(getDisplayedShapeGraph());
      }
      //
      if (sourcemap->getShapeCount() != 0) {
         // this is very simple: create a new drawing layer, and add the data...
         int group = -1;
         for (size_t i = 0; i < SuperSpacePixel::size(); i++) {
            if (SuperSpacePixel::at(i).getName() == "Converted Maps") {
               group = i;
            }
         }
         if (group == -1) {
            SuperSpacePixel::push_back(std::string("Converted Maps"));
            group = SuperSpacePixel::size() - 1;
         }
         SuperSpacePixel::at(group).push_back(ShapeMap(layer_name));
         SuperSpacePixel::at(group).tail().copy(*sourcemap, ShapeMap::COPY_GEOMETRY);
         //
         // dummy set still required:
         SuperSpacePixel::at(group).tail().invalidateDisplayedAttribute();
         SuperSpacePixel::at(group).tail().setDisplayedAttribute(-1);
         //      
         // two levels of merge region:
         if (SuperSpacePixel::at(group).size() == 1) {
            SuperSpacePixel::at(group).m_region = sourcemap->getRegion();
         }
         else {
            SuperSpacePixel::at(group).m_region = runion(SuperSpacePixel::at(group).m_region, sourcemap->getRegion());
         }
         if (SuperSpacePixel::size() == 1) {
            SuperSpacePixel::m_region = SuperSpacePixel::at(group).m_region;
         }
         else {
            SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::at(group).m_region);
         }
         //
         retvar = true;
      }
      retvar = true;
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      m_state |= LINEDATA;
   }

   return retvar;
}

bool MetaGraph::convertPointsToShape()
{
   bool retvar = false;
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   if (m_data_maps.getMapCount() == 0) {
      m_data_maps.addMap("Gates",ShapeMap::DATAMAP);
   }

   if (m_data_maps.getDisplayedMap().makeShapeFromPointSet(getDisplayedPointMap()) != -1) {
      getDisplayedPointMap().clearSel();
      // override the displayed attribute and redisplay:
      m_data_maps.getDisplayedMap().overrideDisplayedAttribute(-2);
      m_data_maps.getDisplayedMap().setDisplayedAttribute(-1);
      // set up a specifc view class to show both layers:
      m_view_class = VIEWVGA | VIEWBACKDATA;
      m_state |= DATAMAPS;
      retvar = true;
   }
   else if (!oldstate) {
      m_data_maps.removeMap(0);
   }

   m_state |= oldstate;

   return retvar;
}

/*
bool MetaGraph::convertBoundaryGraph( Communicator *communicator )
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.convertBoundaryGraph( communicator, (PointMap&) *this );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}
*/

bool MetaGraph::convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues, double stubremoval)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool retvar = false;

   int orig_ref = m_shape_graphs.getDisplayedMapRef();

   try {
      int mapref = m_shape_graphs.convertAxialToSegment( comm, layer_name, keeporiginal, pushvalues, stubremoval);
      if (mapref != -1) {
         retvar = true;
      }
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= oldstate;

   if (retvar) {
      if (!keeporiginal) {
         m_shape_graphs.removeMap(orig_ref);
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

int MetaGraph::loadMifMap(Communicator *comm, istream& miffile, istream& midfile)
{
   int oldstate = m_state;
   m_state &= ~DATAMAPS;

   int retvar = -1;
   
   try {
      // create map layer...
      int mifmapref = m_data_maps.addMap(comm->GetMBInfileName(),ShapeMap::DATAMAP);
      ShapeMap& mifmap = m_data_maps.getMap(mifmapref);
      retvar = mifmap.loadMifMap(miffile, midfile);
      if (retvar == MINFO_OK || retvar == MINFO_MULTIPLE) { // multiple is just a warning
          // display an attribute:
         mifmap.overrideDisplayedAttribute(-2);
         mifmap.setDisplayedAttribute(-1);
         m_data_maps.setDisplayedMapRef(mifmapref);
      }
      else { // error: undo!
         m_data_maps.removeMap(mifmapref);
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = -1;
   }

   m_state = oldstate;

   if (retvar == MINFO_OK || retvar == MINFO_MULTIPLE) { // MINFO_MULTIPLE is simply a warning
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return retvar;
}  

bool MetaGraph::makeAllLineMap( Communicator *communicator, const Point2f& seed )
{
   int oldstate = m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.makeAllLineMap( communicator, (SuperSpacePixel&) *this, seed );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state = oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}


bool MetaGraph::makeFewestLineMap( Communicator *communicator, int replace )
{
   int oldstate= m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      retvar = m_shape_graphs.makeFewestLineMap(communicator, (replace != 0));
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state = oldstate;

   if (retvar) {
      m_state |= SHAPEGRAPHS;   // note: should originally have at least one axial map
      setViewClass(SHOWAXIALTOP);
   }

   return retvar;
}

bool MetaGraph::analyseAxial( Communicator *communicator, Options options, bool simple_version ) // options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      pvecint radius;
      for (size_t i = 0; i < options.radius_list.size(); i++) {
         radius.push_back( (int) options.radius_list[i] );
      }
      retvar = m_shape_graphs.getDisplayedMap().integrate( communicator, radius, options.choice, options.local, options.fulloutput, options.weighted_measure_col, simple_version );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}

bool MetaGraph::analyseSegments( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      if (options.tulip_bins == 0) {
         retvar = m_shape_graphs.getDisplayedMap().analyseAngular(communicator, options.radius_list);
      }
      else {
         retvar = m_shape_graphs.getDisplayedMap().analyseTulip(communicator, options.tulip_bins, options.choice, 
                                                                 options.radius_type, options.radius_list, options.weighted_measure_col);
      }
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}

bool MetaGraph::analyseTopoMet( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool retvar = false;

   try {
      // note: "output_type" reused for analysis type (either 0 = topological or 1 = metric)
      retvar = m_shape_graphs.getDisplayedMap().analyseTopoMet(communicator, options.output_type, options.radius, options.sel_only);
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   m_state |= SHAPEGRAPHS;

   return retvar;
}


bool MetaGraph::analyseAngular( Communicator *communicator, bool analyse_in_memory )
{
   bool retvar = false;
   /*
   Graph::m_nodes.openread();

   if (analyse_in_memory) {
      Graph::m_nodes.loadmem();
   }
   try {
      retvar = Graph::angular_analysis( communicator );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }
   if (analyse_in_memory) {
      Graph::m_nodes.unloadmem();
   }

   Graph::m_nodes.close();

   if (retvar) {
      setDisplayAttribute( AttrHeader::MEDIAN_ANGLE );
   }

   m_state |= AXIALLINES;
   */
   return retvar;
}

bool MetaGraph::makeAxialLines( Communicator *communicator, bool analyse_in_memory )
{
   bool retvar = false;

   /*
   m_state &= ~AXIALLINES;      // Clear axial line data flag (stops accidental redraw during reload) 

   Graph::m_nodes.openread();

   if (analyse_in_memory) {
      Graph::m_nodes.loadmem();
   }
   try {
      retvar = AxialLines::makeAxialLines( (Graph&) *this, (PointMap&) *this );
   } 
   catch (Communicator::CancelledException) {
      retvar = false;
   }
   if (analyse_in_memory) {
      Graph::m_nodes.unloadmem();
   }

   Graph::m_nodes.close();

   if (retvar) 
      m_state |= AXIALLINES;
   */
   return retvar;
}

const int DXFCIRCLERES = 36;

int MetaGraph::loadLineData( Communicator *communicator, int load_type )
{
   m_state &= ~LINEDATA;      // Clear line data flag (stops accidental redraw during reload) 
/*
   if (m_state & POINTS) {
      PointMap::s_bl = NoPixel; // <- force coming clear to clear *all* points
      PointMap::clearPoints();  // If points exist, clear them
      m_state &= ~POINTS;        // ...and clear the flag
   }
*/
   // if bsp tree exists 
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

   if (load_type & REPLACE) {
      SuperSpacePixel::clear();
   }

   SuperSpacePixel::push_back(communicator->GetMBInfileName());

   if (load_type & DXF) {

      // separate the stream and the communicator, allowing non-file streams read
      int error = loadDxf(*communicator, communicator);

      if (error != 1) {
         return error;
      }
   }
   else if (load_type & CAT) {
      // separate the stream and the communicator, allowing non-file streams read
      int error = loadCat(*communicator, communicator);
      if (error != 1) {
         return error;
      }
   }
   else if (load_type & RT1) {
      // separate the stream and the communicator, allowing non-file streams read
      int error = loadRT1(communicator->GetFileSet(), communicator);
      if (error != 1) {
         return error;
      }
   }
   else if (load_type & NTF) {
   
      NtfMap map;

      try {
         map.open(communicator->GetFileSet(), communicator);
      }
      catch (Communicator::CancelledException) {
         SuperSpacePixel::pop_back();
         return 0;
      }
      catch (pexception) {
         SuperSpacePixel::pop_back();
         return -1;
      }

      if (communicator->IsCancelled()) {
         SuperSpacePixel::pop_back();
         return 0;
      }

      SuperSpacePixel::tail().m_region = map.getRegion();;

      for (size_t i = 0; i < map.size(); i++) {

         SuperSpacePixel::tail().push_back(ShapeMap(map[i].getName()));
         SuperSpacePixel::tail().at(i).init(map[i].getLineCount(), map.getRegion());

         for (size_t j = 0; j < map[i].size(); j++) {

            for (size_t k = 0; k < map[i][j].size(); k++) {

               SuperSpacePixel::tail().at(i).makeLineShape( map[i][j][k] );
            }
         }

         SuperSpacePixel::tail().at(i).setDisplayedAttribute(-2);
         SuperSpacePixel::tail().at(i).setDisplayedAttribute(-1);
      }
   }

   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   return 1;
}

int MetaGraph::loadDxf( istream& stream, Communicator *communicator)
{
   DxfParser dp;

   if (communicator) {
      dp = DxfParser( communicator );

      try {
         *communicator >> dp;
      }
      catch (Communicator::CancelledException) {
         SuperSpacePixel::pop_back();
         return 0;
      }
      catch (pexception) {
         SuperSpacePixel::pop_back();
         return -1;
      }

      if (communicator->IsCancelled()) {
         SuperSpacePixel::pop_back();
         return 0;
      }
   }
   else {
      dp.open(stream);
   }

   SuperSpacePixel::tail().m_region = QtRegion(dp.getExtMin(), dp.getExtMax());

   int i = 0;

   for (auto& layer: dp.getLayers())
   {
      const DxfLayer& dxf_layer = layer.second;

      if (dxf_layer.empty()) {
         continue;
      }
      
      SuperSpacePixel::tail().push_back(ShapeMap(dxf_layer.getName().c_str()));
      // note the circle lines are not counted in the total number of lines, as we have to specify number of segments
      SuperSpacePixel::tail().at(i).init(dxf_layer.numTotalPoints() + dxf_layer.numTotalLines() + dxf_layer.numArcs() * DXFCIRCLERES + dxf_layer.numCircles() * DXFCIRCLERES, QtRegion(Point2f(dxf_layer.getExtMin()),Point2f(dxf_layer.getExtMax())) );

      for (int jp = 0; jp < dxf_layer.numPoints(); jp++) {

         const DxfVertex& dxf_point = dxf_layer.getPoint( jp );
         Point2f point = Point2f(dxf_point);
         SuperSpacePixel::tail().at(i).makePointShape( point );

      }

      for (int j = 0; j < dxf_layer.numLines(); j++) {

         const DxfLine& dxf_line = dxf_layer.getLine( j );
         Line line = Line( Point2f(dxf_line.getStart()), Point2f(dxf_line.getEnd()) );
         SuperSpacePixel::tail().at(i).makeLineShape( line );   

      }
      for (int k = 0; k < dxf_layer.numPolyLines(); k++) {

         const DxfPolyLine& poly = dxf_layer.getPolyLine( k );
         pvecpoint points;
         for (int m = 0; m < poly.numVertices(); m++) {
            points.push_back(poly.getVertex(m));
         }
         SuperSpacePixel::tail().at(i).makePolyShape( points, (poly.getAttributes() & DxfPolyLine::CLOSED) != DxfPolyLine::CLOSED );
      }
      for (int l = 0; l < dxf_layer.numSplines(); l++) {

         const DxfSpline& poly = dxf_layer.getSpline( l );

         pvecpoint points;
         for (int m = 0; m < poly.numVertices(); m++) {
            points.push_back(poly.getVertex(m));
         }
         SuperSpacePixel::tail().at(i).makePolyShape( points, (poly.getAttributes() & DxfPolyLine::CLOSED) != DxfPolyLine::CLOSED );

      }
      // needs fixing to use appropriate poly type:
      for (int n = 0; n < dxf_layer.numArcs(); n++) {
         
         const DxfArc& circ = dxf_layer.getArc( n );
         int segments = circ.numSegments(DXFCIRCLERES);
         if (segments > 1) {
            for (int m = 0; m < segments; m++) {
               // note, loops on DXFCIRCLERES (e.g. 36) to 0
               Line line = Line( Point2f(circ.getVertex(m,DXFCIRCLERES)), Point2f(circ.getVertex(m+1,DXFCIRCLERES)) );
               SuperSpacePixel::tail().at(i).makeLineShape( line );
            }
         }
      }
      for (int nc = 0; nc < dxf_layer.numCircles(); nc++) {
         
         const DxfCircle& circ = dxf_layer.getCircle( nc );
         pvecpoint points;
         for (int m = 0; m < DXFCIRCLERES; m++) {
            points.push_back(circ.getVertex(m,DXFCIRCLERES));
         }
         SuperSpacePixel::tail().at(i).makePolyShape( points, false );
      }

      SuperSpacePixel::tail().at(i).setDisplayedAttribute(-2);
      SuperSpacePixel::tail().at(i).setDisplayedAttribute(-1);

      i++;
   }

   return 1;
}

int MetaGraph::loadCat( istream& stream, Communicator *communicator )
{
   if (communicator) {
      long size = communicator->GetInfileSize();
      communicator->CommPostMessage( Communicator::NUM_RECORDS, size );
   }

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t a_time = 0;
#else
   time_t a_time = 0;
#endif
   
   qtimer( a_time, 0 );

   long size = 0; 
   int numlines = 0;
   int parsing = 0;
   bool first = true;

   Point2f current_point, min_point, max_point;

   while (!stream.eof()) {

      std::string inputline;
      stream >> inputline;
      if (inputline.length() > 1 && inputline[0] != '#') {
         if (!parsing) {
            if (dXstring::toLower(inputline) == "begin polygon") {
               parsing = 1;
            }
            else if (dXstring::toLower(inputline) == "begin polyline") {
               parsing = 2;
            }
         }
         else if (dXstring::toLower(inputline).substr(0,3) == "end") {
            parsing = 0;
         }
         else {
            auto tokens = dXstring::split(inputline, ' ', true);
            current_point.x = stod(tokens[0]);
            current_point.y = stod(tokens[1]);
            numlines++;
            if (first) {
               min_point = current_point;
               max_point = current_point;
               first = false;
            }
            else {
               if (current_point.x < min_point.x) {
                  min_point.x = current_point.x;
               }
               if (current_point.y < min_point.y) {
                  min_point.y = current_point.y;
               }
               if (current_point.x > max_point.x) {
                  max_point.x = current_point.x;
               }
               if (current_point.y > max_point.y) {
                  max_point.y = current_point.y;
               }
            }
         }
      }
   }
   SuperSpacePixel::tail().m_region = QtRegion(min_point, max_point);
   SuperSpacePixel::tail().push_back(ShapeMap());
   SuperSpacePixel::tail().tail().init( numlines, QtRegion(min_point, max_point) );

   // in MSVC 6, ios::eof remains set and it needs to be cleared.
   // in MSVC 8 it's even worse: it won't even seekg until eof flag has been cleared
   stream.clear();
   stream.seekg(0, ios::beg);

   parsing = 0;
   first = true;
   pvecpoint points;

   while (!stream.eof()) {

      std::string inputline;
      stream >> inputline;

      if (inputline.length() > 1 && inputline[0] != '#') {
         if (!parsing) {
            if (dXstring::toLower(inputline) == "begin polygon") {
               parsing = 1;
               first = true;
            }
            else if (dXstring::toLower(inputline) == "begin polyline") {
               parsing = 2;
               first = true;
            }
         }
         else if (dXstring::toLower(inputline).substr(0,3) == "end") {
            if (points.size() > 2) {
               if (parsing == 1) { // polygon
                  SuperSpacePixel::tail().tail().makePolyShape(points, false);
               }
               else { // polyline
                  SuperSpacePixel::tail().tail().makePolyShape(points, true);
               }
            }
            else if (points.size() == 2) {
               SuperSpacePixel::tail().tail().makeLineShape(Line(points[0],points[1]));
            }
            points.clear();
            parsing = 0;
         }
         else {
             auto tokens = dXstring::split(inputline, ' ', true);
            current_point.x = stod(tokens[0]);
            current_point.y = stod(tokens[1]);
            points.push_back(current_point);
         }
      }

      size += inputline.length() + 1;

      if (communicator) {
         if (qtimer( a_time, 500 )) {
            if (communicator->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            communicator->CommPostMessage( Communicator::CURRENT_RECORD, size );
         }
      }
   }

   SuperSpacePixel::tail().tail().setDisplayedAttribute(-2);
   SuperSpacePixel::tail().tail().setDisplayedAttribute(-1);

   return 1;
}

int MetaGraph::loadRT1(const std::vector<string>& fileset, Communicator *communicator)
{
   TigerMap map;

   try {
      map.parse( fileset, communicator );
   }
   catch (Communicator::CancelledException) {
      SuperSpacePixel::pop_back();
      return 0;
   }
   catch (pexception) {
      SuperSpacePixel::pop_back();
      return -1;
   }

   if (communicator->IsCancelled()) {
      SuperSpacePixel::pop_back();
      return 0;
   }

   SuperSpacePixel::tail().m_region = QtRegion(map.getBottomLeft(), map.getTopRight());

   // for each category
   for (size_t i = 0; i < map.size(); i++) {

      SuperSpacePixel::tail().push_back(ShapeMap(map.key(i)));
      SuperSpacePixel::tail().at(i).init(map.value(i).size(), map.getRegion() );

      // for each chains in category:
      for (size_t j = 0; j < map.value(i).size(); j++) {
         // for each node pair in each category
         for (size_t k = 0; k < map.value(i).at(j).size(); k++) {
            SuperSpacePixel::tail().at(i).makeLineShape( map.value(i).at(j).at(k) );   
         }
      }

      SuperSpacePixel::tail().at(i).setDisplayedAttribute(-2);
      SuperSpacePixel::tail().at(i).setDisplayedAttribute(-1);
   
   }

   return 1;
}



/*
// DEPRECATED

void MetaGraph::fastGraph( istream& stream, double spacing )
{
   // does the lot -- assumes a bounding polygon,
   // and tries to find a location outside all other polygons to populate grid
   prefvec<Poly> polygons;

   // any name for the file will do...
   SuperSpacePixel::push_back(SpacePixelFile("salad"));
   
   // load the data from the file
   loadCat( stream, NULL, &polygons );

   // organise the data from the file
   for (int i = 0; i < SuperSpacePixel::tail().size(); i++) {
      SuperSpacePixel::tail().at(i).sortPixelLines();
   }
   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   setGrid( spacing );

   // this is a silly way to do this, but there you go, randomly choose points until you get one
   // that hits empty space...
   srand(time(NULL));
   int testhits, count = 0;
   PixelRef testpixel;
   Point2f testpoint;
   do {
      count++;
      testhits = 0;
      testpixel = PixelRef( rand() % PointMap::getCols(), rand() % PointMap::getRows() );
      testpoint = PointMap::depixelate(testpixel);
      for (int i = 0; i < polygons.size(); i++) {
         try {
            if (polygons[i].contains(testpoint)) {
               testhits++;
            }
         }
         catch (int) {
            // polygons throw if on edge:
            // break from this loop and continue do-while loop:
            testhits = 0;
            break;
         }
      }
   } while (testhits != 1 && count < (PointMap::getCols() * PointMap::getRows()) );

   if (testhits != 1) {
      return; // give up, you must have tried just about every location by now...
   }

   PointMap::makePoints(testpoint, Point::FILLED);

   m_state |= POINTS;

   PointMap::sparkGraph2(NULL, 0);

   m_state |= GRAPH | ANGULARGRAPH;

   setViewClass(SHOWVGATOP);

   // Testing: 
   // write("dummy.graph");
}
*/

bool MetaGraph::importCat(istream& filecontents)
{
   // any name for the file will do...
   SuperSpacePixel::push_back(SpacePixelFile("salad"));

   // load the data from the file
   if (!loadCat( filecontents, NULL )) {
      return false;
   }

   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   return true;
}

bool MetaGraph::importDxf(istream& filecontents)
{
   // any name for the file will do...
   SuperSpacePixel::push_back(SpacePixelFile("salad"));

   // load the data from the file
   if (!loadDxf( filecontents, NULL )) {
      return false;
   }

   if (SuperSpacePixel::size() == 1) {
      SuperSpacePixel::m_region = SuperSpacePixel::tail().m_region;
   }
   else {
      SuperSpacePixel::m_region = runion(SuperSpacePixel::m_region, SuperSpacePixel::tail().m_region);
   }

   m_state |= LINEDATA;

   return true;
}

// a second does the lot, especially for evolutionary graphs
// essentially, hand the ecoevograph a new meta graph each time...
// it'll do everything for you

void MetaGraph::fastGraph( const Point2f& seed, double spacing )
{
   setGrid( spacing );

   PointMaps::getDisplayedPointMap().makePoints(seed, 0); // 0 = not semifilled

   m_state |= POINTMAPS;

   PointMaps::getDisplayedPointMap().sparkGraph2(NULL, 0, -1.0);

   // historical tag - essentially stamps version, so kept:
   m_state |= ANGULARGRAPH;

   setViewClass(SHOWVGATOP);

   // Testing: 
   // write("dummy.graph");
}

int MetaGraph::importTxt( istream& stream, const std::string& name, bool csv )
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   int x = m_data_maps.addMap(name,ShapeMap::DATAMAP);

   if (!m_data_maps.getDisplayedMap().importTxt( stream, csv )) {
      m_data_maps.removeMap(x);
      m_state = oldstate;
      return -1;
   }

   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);

   return x;
}

///////////////////////////////////////////////////////////////////////////////

// New layer interaction code

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

// similar (but much, much easier than above -- relies on order of index order of
// axial lines being the same as as the created poly index order and both attribute 
// tables -- should be since the map is in indexed order, as is its attribute table)

// DEPRECATED: only need to switch map type flag

void MetaGraph::convertShapeGraphToShapeMap(const ShapeGraph& axialmap)
{
   int x = m_data_maps.addMap("Axial Gates",ShapeMap::DATAMAP);
   ShapeMap& shapemap = m_data_maps.getMap(x);

   // use the dangermouse all polygon grabber:
   const pqmap<int,SalaShape>& polys = axialmap.getAllShapes();
   shapemap.init(axialmap.getShapeCount(),axialmap.getRegion());
   size_t i;
   for (i = 0; i < axialmap.getShapeCount(); i++) {
      if (polys[i].isLine()) {   // it ought to be since we're starting with an axial map!
         shapemap.makeLineShape(polys[i].getLine());
      }
   }

   // now convert attributes and we're done!
   const AttributeTable& table_in = axialmap.getAttributeTable();
   AttributeTable& table_out = shapemap.getAttributeTable();

   for (i = 0; i < size_t(table_in.getColumnCount()); i++) {
      table_out.insertColumn(table_in.getColumnName(i));
   }
   
   int counter = 0;
   for (i = 0; i < size_t(table_in.getRowCount()); i++) {
      if (polys[i].isLine()) {   // check needs to be maintained so indices match
         for (int j = 0; j < table_in.getColumnCount(); j++) {
            table_out.setValue(counter,j,table_in.getValue(i,j));
         }
         counter++;  // in case 'i' skips over a few non-line objects
      }
   }

   shapemap.overrideDisplayedAttribute(-2);
   shapemap.setDisplayedAttribute(-1);
}


// the tidy(ish) version: still needs to be at top level and switch between layers

bool MetaGraph::pushValuesToLayer(int desttype, int destlayer, int push_func, bool count_col)
{
   int sourcetype = m_view_class;
   int sourcelayer = getDisplayedMapRef();
   int col_in = getDisplayedAttribute(); 
   // no col_out specified
   int col_out = -2;

   // temporarily turn off everything to prevent redraw during sensitive time:
   int oldstate = m_state;
   m_state &= ~(DATAMAPS | AXIALLINES | POINTMAPS);

   bool retvar = pushValuesToLayer(sourcetype,sourcelayer,desttype,destlayer,col_in,col_out,push_func,count_col);

   m_state = oldstate;

   return retvar;
}

// helper

void pushValue(double& val, int& count, double thisval, int push_func)
{
   if (thisval != -1) {
      switch (push_func) {
      case MetaGraph::PUSH_FUNC_MAX:
         if (val == -1 || thisval > val)
            val = thisval;
         break;
      case MetaGraph::PUSH_FUNC_MIN:
         if (val == -1 || thisval < val)
            val = thisval;
         break;
      case MetaGraph::PUSH_FUNC_AVG:
      case MetaGraph::PUSH_FUNC_TOT:
         if (val == -1.0) 
            val = thisval;
         else 
            val += thisval;
         break;
      }
      count++;
   }
}

// the full ubercontrol version:

bool MetaGraph::pushValuesToLayer(int sourcetype, int sourcelayer, int desttype, int destlayer, int col_in, int col_out, int push_func, bool count_col)
{
   AttributeTable& table_in = getAttributeTable(sourcetype, sourcelayer);
   AttributeTable& table_out = getAttributeTable(desttype, destlayer);

   if (col_out == -2) {
      std::string name = table_in.getColumnName(col_in);
      if ((table_out.isValidColumn(name) && table_out.isColumnLocked(table_out.getColumnIndex(name))) || name == "Object Count") {
         name = std::string("Copied ") + name;
      }
      col_out = table_out.insertColumn(name);
   }

   int col_count = -1;
   if (count_col) {
      col_count = table_out.insertColumn("Object Count");
      if (col_count <= col_out) {
         col_out++;
      }
   }

   if (sourcetype & VIEWDATA) {
      pvecint gatelist;
      for (int i = 0; i < table_out.getRowCount(); i++) {
         if (!table_out.isVisible(i)) {
            continue;
         }
         gatelist.clear();
         if (desttype == VIEWVGA) {
            m_data_maps.getMap(sourcelayer).pointInPolyList(PointMaps::maps_vector.at(destlayer).getPoint(table_out.getRowKey(i)).m_location,gatelist);
         }
         else if (desttype == VIEWAXIAL) {
            m_data_maps.getMap(sourcelayer).shapeInPolyList(m_shape_graphs.getMap(destlayer).getAllShapes().search(table_out.getRowKey(i)),gatelist);
         }
         else if (desttype == VIEWDATA) {
            if (sourcelayer == destlayer) {
               // error: pushing to same map
               return false;
            }
            m_data_maps.getMap(sourcelayer).shapeInPolyList(m_data_maps.getMap(destlayer).getAllShapes().search(table_out.getRowKey(i)),gatelist);
         }
         double val = -1.0;
         int count = 0;
         for (size_t j = 0; j < gatelist.size(); j++) {
            if (table_in.isVisible(gatelist[j])) {
               double thisval = table_in.getValue(gatelist[j],col_in);
               pushValue(val,count,thisval,push_func);
            }
         }
         if (push_func == PUSH_FUNC_AVG && val != -1.0) {
            val /= double(count);
         }
         table_out.setValue(i,col_out,float(val));
         if (count_col) {
            table_out.setValue(i,col_count,float(count));
         }
      }
   }
   else {
      // prepare a temporary value table to store counts and values
      double *vals = new double [table_out.getRowCount()];
      int *counts = new int [table_out.getRowCount()];

      int i;
      for (i = 0; i < table_out.getRowCount(); i++) {
         counts[i] = 0; // count set to zero for all
         vals[i] = -1;
      }

      pvecint gatelist;
      if (sourcetype & VIEWVGA) {
         for (int i = 0; i < table_in.getRowCount(); i++) {
            if (!table_in.isVisible(i)) {
               continue;
            }
            gatelist.clear();
            if (desttype == VIEWDATA) {
               m_data_maps.getMap(destlayer).pointInPolyList(PointMaps::maps_vector.at(sourcelayer).getPoint(table_in.getRowKey(i)).m_location,gatelist);
            }
            else if (desttype == VIEWAXIAL) {
               // note, "axial" could be convex map, and hence this would be a valid operation
               m_shape_graphs.getMap(destlayer).pointInPolyList(PointMaps::maps_vector.at(sourcelayer).getPoint(table_in.getRowKey(i)).m_location,gatelist);
            }
            double thisval = table_in.getValue(i,col_in);
            for (size_t j = 0; j < gatelist.size(); j++) {
               if (table_out.isVisible(gatelist[j])) {
                  double& val = vals[gatelist[j]];
                  int& count = counts[gatelist[j]];
                  pushValue(val,count,thisval,push_func);
               }
            }
         }
      }
      else if (sourcetype & VIEWAXIAL) {
         // note, in the spirit of mapping fewer objects in the gate list, it is *usually* best to 
         // perform axial -> gate map in this direction
         // however, "Axial" to VGA, likely to have more points than "axial" shapes, should probably be performed using the first 
         // algorithm
         for (int i = 0; i < table_in.getRowCount(); i++) {
            if (!table_in.isVisible(i)) {
               continue;
            }
            gatelist.clear();
            if (desttype == VIEWDATA) {
               m_data_maps.getMap(destlayer).shapeInPolyList(m_shape_graphs.getMap(sourcelayer).getAllShapes().search(table_in.getRowKey(i)),gatelist);
            }
            else if (desttype == VIEWAXIAL) {
               m_shape_graphs.getMap(destlayer).shapeInPolyList(m_shape_graphs.getMap(sourcelayer).getAllShapes().search(table_in.getRowKey(i)),gatelist);
            }
            double thisval = table_in.getValue(i,col_in);
            for (size_t j = 0; j < gatelist.size(); j++) {
               if (table_out.isVisible(gatelist[j])) {
                  double& val = vals[gatelist[j]];
                  int& count = counts[gatelist[j]];
                  pushValue(val,count,thisval,push_func);
               }
            }
         }
      }

      for (i = 0; i < table_out.getRowCount(); i++) {
         if (!table_out.isVisible(i)) {
            continue;
         }
         if (push_func == PUSH_FUNC_AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
         }
         table_out.setValue(i,col_out,float(vals[i]));
         if (count_col) {
            table_out.setValue(i,col_count,float(counts[i]));
         }
      }

      delete [] vals;
      delete [] counts;
   }

   // display new data in the relevant layer
   if (desttype == VIEWVGA) {
      PointMaps::maps_vector.at(destlayer).overrideDisplayedAttribute(-2);
      PointMaps::maps_vector.at(destlayer).setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWAXIAL) {
      m_shape_graphs.getMap(destlayer).overrideDisplayedAttribute(-2);
      m_shape_graphs.getMap(destlayer).setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWDATA) {
      m_data_maps.getMap(destlayer).overrideDisplayedAttribute(-2);
      m_data_maps.getMap(destlayer).setDisplayedAttribute(col_out);
   }


   return true;
}

// DEPRECATED CODE: REPLACED WITH THE FUNCTION ABOVE
// Replaced 21-Aug-05

/*
if (m_view_class & VIEWAXIAL) {
   return m_shape_graphs.pushValuesToLayer();
}

// note: only pushes to gates...
DataLayer& layer = (DataLayer&) getLayer(DataLayers::GATES);

int attr = PointMaps::getDisplayedPointMap().getDisplayedAttribute();
const AttributeTable& table = PointMaps::getDisplayedPointMap().getAttributeTable();

// give the layer col a nice name!
int col = layer.addColumn( table.getColumnName(attr) );

// I think this is the only way to store how many points are in the object:
int *objpointcounts = new int [layer.getObjectCount()];
for (int h = 0; h < layer.getObjectCount(); h++) {
   objpointcounts[h] = 0;
}

// just doing here for now:
PointMap& map = PointMaps::getDisplayedPointMap();
for (int i = 0; i < map.m_cols; i++) {
   for (int j = 0; j < map.m_rows; j++) {
      if ( map.m_points[i][j].getState() & Point::FILLED ) {
         int obj = map.m_points[i][j].getDataObject( getCurrentLayerRef() );
         if (obj != -1) {
            if (average_over_isovist) {
               // doesn't include self for now...
               double val = 0.0;
               Node& n = map.m_points[i][j].getNode();
               n.first();
               while (!n.is_tail())
               {
                  val += table.getValue(table.getRowIndex(n.cursor()),attr);
               }
               layer[obj][col] += val / map.m_points[i][j].getNode().count();
            }
            else {
               layer[obj][col] += table.getValue(table.getRowIndex(PixelRef(i,j)),attr);
            }
            objpointcounts[obj] += 1;
         }
      }
   }
}

for (int k = 0; k < layer.getObjectCount(); k++)
{
   layer[k][col] /= double(objpointcounts[k]);
}

delete [] objpointcounts;

// finally, tell layers that we'd like to view this next time:
layer.setDisplayColumn(col+1); // (+1 for ref number)
*/

///////////////////////////////////////////////////////////////////////////////////

// Agent functionality: some of it still kept here with the metagraph
// (to allow push value to layer and back again)

void MetaGraph::runAgentEngine(Communicator *comm)
{
   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   if (m_agent_engine.m_gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      int colgates = table.insertColumn(g_col_gate);
      pushValuesToLayer(VIEWDATA,m_agent_engine.m_gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
      table.insertColumn(g_col_gate_counts);
   }

   m_agent_engine.run(comm, &(getDisplayedPointMap()) );

   if (m_agent_engine.m_gatelayer != -1) {
      // switch column counts from vga layer to gates layer...
      int colcounts = table.getColumnIndex(g_col_gate_counts);
      AttributeTable& tableout = m_data_maps.getMap(m_agent_engine.m_gatelayer).getAttributeTable();
      int targetcol = tableout.insertColumn("Agent Counts");
      pushValuesToLayer(VIEWVGA,getDisplayedPointMapRef(),
                        VIEWDATA,m_agent_engine.m_gatelayer,
                        colcounts,targetcol,PUSH_FUNC_TOT);
      // and delete the temporary columns:
      table.removeColumn(colcounts);
      int colgates = table.getColumnIndex(g_col_gate);
      table.removeColumn(colgates);
   }
}

// Thru vision

bool MetaGraph::analyseThruVision(Communicator *comm, int gatelayer)
{
   bool retvar = false;

   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   // always have temporary gate counting layers -- makes it easier to code
   int colgates = table.insertColumn(g_col_gate);
   int colcounts = table.insertColumn(g_col_gate_counts);

   if (gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      pushValuesToLayer(VIEWDATA,gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
   }

   try {
      retvar = getDisplayedPointMap().analyseThruVision(comm);
   }
   catch (Communicator::CancelledException) {
      retvar = false;
   }

   // note after the analysis, the column order might have changed... retrieve:
   colgates = table.getColumnIndex(g_col_gate);
   colcounts = table.getColumnIndex(g_col_gate_counts);

   if (retvar && gatelayer != -1) {
      AttributeTable& tableout = m_data_maps.getMap(gatelayer).getAttributeTable();
      int targetcol = tableout.insertColumn("Thru Vision Counts");
      pushValuesToLayer(VIEWVGA,getDisplayedPointMapRef(),
                        VIEWDATA,gatelayer,
                        colcounts,targetcol,PUSH_FUNC_TOT);
   }

   // and always delete the temporary columns:
   table.removeColumn(colcounts);
   table.removeColumn(colgates);

   return retvar;
}

///////////////////////////////////////////////////////////////////////////////////

int MetaGraph::getDisplayedMapRef() const
{
   int ref = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      ref = PointMaps::getDisplayedPointMapRef();
      break;
   case VIEWAXIAL:
      ref = m_shape_graphs.getDisplayedMapRef();
      break;
   case VIEWDATA:
      ref = m_data_maps.getDisplayedMapRef();
      break;
   }
   return ref;
}

// I'd like to use this more often so that several classes other than data maps and shape graphs 
// can be used in the future

int MetaGraph::getDisplayedMapType()
{
   int type = ShapeMap::EMPTYMAP;
   switch (m_view_class & VIEWFRONT) {
   case VIEWAXIAL:
      type = m_shape_graphs.getDisplayedMap().getMapType();
      break;
   case VIEWDATA:
      type = m_data_maps.getDisplayedMap().getMapType();
      break;
   }
   return type;
}

// note: 0 is not at all editable, 1 is editable off and 2 is editable on
int MetaGraph::isEditable() const
{
   int editable = 0;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      if (getDisplayedPointMap().isProcessed()) {
         editable = NOT_EDITABLE;
      }
      else {
         editable = EDITABLE_ON;
      }
      break;
   case VIEWAXIAL:
      {
         int type = m_shape_graphs.getDisplayedMap().getMapType();
         if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
            editable = m_shape_graphs.getDisplayedMap().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
         }
         else {
            editable = NOT_EDITABLE;
         }
      }
      break;
   case VIEWDATA:
      editable = m_data_maps.getDisplayedMap().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
      break;
   }
   return editable;
}

bool MetaGraph::canUndo() const
{
   bool canundo = false;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      canundo = getDisplayedPointMap().canUndo();
      break;
   case VIEWAXIAL:
      canundo = m_shape_graphs.getDisplayedMap().canUndo();
      break;
   case VIEWDATA:
      canundo = m_data_maps.getDisplayedMap().canUndo();
      break;
   }
   return canundo;
}

void MetaGraph::undo()
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().undoPoints();
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().undo();
      break;
   case VIEWDATA:
      m_data_maps.getDisplayedMap().undo();
      break;
   }
}

// Moving to global ways of doing things:

int MetaGraph::addAttribute(const std::string& name)
{
   int col;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      col = getDisplayedPointMap().addAttribute(name);
      break;
   case VIEWAXIAL:
      col = m_shape_graphs.getDisplayedMap().addAttribute(name);
      break;
   case VIEWDATA:
      col = m_data_maps.getDisplayedMap().addAttribute(name);
      break;
   }
   return col;
}

void MetaGraph::removeAttribute(int col)
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().removeAttribute(col);
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().removeAttribute(col);
      break;
   case VIEWDATA:
      m_data_maps.getDisplayedMap().removeAttribute(col);
      break;
   }
}

bool MetaGraph::isAttributeLocked(int col)
{
   return getAttributeTable(m_view_class).isColumnLocked(col);
}

int MetaGraph::getDisplayedAttribute() const
{
   int col = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      col = getDisplayedPointMap().getDisplayedAttribute();
      break;
   case VIEWAXIAL:
      col = m_shape_graphs.getDisplayedMap().getDisplayedAttribute();
      break;
   case VIEWDATA:
      col = m_data_maps.getDisplayedMap().getDisplayedAttribute();
      break;
   }
   return col;
}

// this is coming from the front end, so force override:
void MetaGraph::setDisplayedAttribute(int col)
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      getDisplayedPointMap().overrideDisplayedAttribute(-2);
      getDisplayedPointMap().setDisplayedAttribute(col);
      break;
   case VIEWAXIAL:
      m_shape_graphs.getDisplayedMap().overrideDisplayedAttribute(-2);
      m_shape_graphs.getDisplayedMap().setDisplayedAttribute(col);
      break;
   case VIEWDATA:
      m_data_maps.getDisplayedMap().overrideDisplayedAttribute(-2);
      m_data_maps.getDisplayedMap().setDisplayedAttribute(col);
      break;
   }
}

// const and non-const versions:

AttributeTable& MetaGraph::getAttributeTable(int type, int layer)
{
   AttributeTable *tab = NULL;
   if (type == -1) {
      type = m_view_class;
   }
   switch (type & VIEWFRONT) {
   case VIEWVGA:
      tab = (layer == -1) ? &(getDisplayedPointMap().getAttributeTable()) : &(PointMaps::maps_vector.at(layer).getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = (layer == -1) ? &(m_shape_graphs.getDisplayedMap().getAttributeTable()) : &(m_shape_graphs.getMap(layer).getAttributeTable());
      break;
   case VIEWDATA:
      tab = (layer == -1) ? &(m_data_maps.getDisplayedMap().getAttributeTable()) : &(m_data_maps.getMap(layer).getAttributeTable());
      break;
   }
   return *tab;
}

const AttributeTable& MetaGraph::getAttributeTable(int type, int layer) const
{
   const AttributeTable *tab = NULL;
   if (type == -1) {
      type = m_view_class & VIEWFRONT;
   }
   switch (type) {
   case VIEWVGA:
      tab = layer == -1 ? &(getDisplayedPointMap().getAttributeTable()) : &(PointMaps::maps_vector.at(layer).getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = layer == -1 ? &(m_shape_graphs.getDisplayedMap().getAttributeTable()) : &(m_shape_graphs.getMap(layer).getAttributeTable());
      break;
   case VIEWDATA:
      tab = layer == -1 ? &(m_data_maps.getDisplayedMap().getAttributeTable()) : &(m_data_maps.getMap(layer).getAttributeTable());
      break;
   }
   return *tab;
}


///////////////////////////////////////////////////////////////////////////////////

/*
// These two functions are no longer supported

// for editing spacespixel lines post build

// *before* using these functions you need to make at least one layer
// *editable* (e.g., getLineLayer(0,0).setEditable(true)) 
// *after* using these functions you need to rebuild the graph
// (use dynamicSparkGraph2)

int MetaGraph::addLineDynamic(const Line& l)
{
   LineKey linekey = -1;

   // this is only used once the graph is built
   if (!getDisplayedPointMap().isProcessed()) {
      return linekey;
   }

   PointMaps::getDisplayedPointMap().blockLines();

   for (int i = 0; i < getLineFileCount(); i++) {
      for (int j = 0; j < getLineLayerCount(i); j++) {
         // chooses the first editable layer it can find:
         if (SuperSpacePixel::at(i).at(j).isEditable()) {
            SpacePixel& spacepix = SuperSpacePixel::at(i).at(j);
            linekey.file = i;
            linekey.layer = j;
            linekey.lineref = spacepix.addLineDynamic(l);
         }
      }
   }

   if (linekey != -1) {
      // update the pointdata... nb.  The graph isn't affected until you rebuild graph
      // (as you might be playing with more than one line at a time it seems sensible to 
      // wait until you're ready to go with all of them)
      PointMaps::getDisplayedPointMap().addLineDynamic(linekey,l);
   }

   return linekey;
}

bool MetaGraph::removeLineDynamic(LineKey linekey)
{
   bool retvar = false;

   // this is only used once the graph is built
   if (!getDisplayedPointMap().isProcessed()) {
      return retvar;
   }

   // first *before adding or removing the line* ensure existing lines are blocked
   PointMaps::getDisplayedPointMap().blockLines();

   if (linekey != -1) {  // <- this will be typical value when unset
      SpacePixel& spacepix = SuperSpacePixel::at(linekey.file).at(linekey.layer);
      Line line;
      retvar = spacepix.removeLineDynamic(linekey.lineref,line);

      if (retvar) {
         // update the pointdata... nb.  The graph isn't affected until you rebuild graph
         // (as you might be playing with more than one line at a time it seems sensible to 
         // wait until you're ready to go with all of them)
         // Note: the line itself is used to find the affected pixels
         PointMaps::getDisplayedPointMap().removeLineDynamic(linekey,line);
      }
   }

   return retvar;
}
*/
///////////////////////////////////////////////////////////////////////////////

void MetaGraph::loadGraphAgent()
{
//   Graph::m_nodes.openread();
//   Graph::m_nodes.loadmem();
}

void MetaGraph::unloadGraphAgent()
{
//   Graph::m_nodes.unloadmem();
//   Graph::m_nodes.close();
}

///////////////////////////////////////////////////////////////////////////////

int MetaGraph::read( const std::string& filename )
{
   m_state = 0;   // <- clear the state out

   // clear BSP tree if it exists:
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

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
         PointMaps::maps_vector.push_back(PointMap());
         if (temp_state & 0x0001) { // 0x0001 was "GRAPH"
            PointMaps::maps_vector.back().m_processed = true;
            temp_state &= ~0x0001;
         }
         if (temp_state & 0x0080) { // 0x0080 was "BOUNDARYGRAPH"
            PointMaps::maps_vector.back().m_boundarygraph = true;
            temp_state &= ~0x0080;
         }
         PointMaps::maps_vector.back().read( stream, version );
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
      PointMaps::maps_vector.back().m_processed = true;

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
      if (PointMaps::maps_vector.size()) {
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

int MetaGraph::write( const std::string& filename, int version, bool currentlayer )
{
   ofstream stream;

   int oldstate = m_state;
   m_state = 0;   // <- temporarily clear out state, avoids any potential read / write errors

   char type;

   // As of MetaGraph version 70 the disk caching has been removed
   stream.open( filename.c_str(), ios::binary | ios::out | ios::trunc );
   if (stream.fail()) {
      if (stream.rdbuf()->is_open()) {
         stream.close();
      }
      m_state = oldstate;
      return DISK_ERROR;  
   }
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

   stream.close();

   m_state = oldstate;
   return OK;
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

   for (i = 0; i < PointMaps::maps_vector.size(); i++) {
      PointMap& map = PointMaps::maps_vector.at(i);
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

#include <salalib/ngraph.h>  // for conversion

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

   PixelRefList bins_b[32];
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

   PointMap& map = PointMaps::maps_vector.back();
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

