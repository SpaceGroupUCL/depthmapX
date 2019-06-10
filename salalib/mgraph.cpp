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

#include "salalib/alllinemap.h"
#include "salalib/mapconverter.h"
#include "salalib/isovist.h"
#include "salalib/mgraph.h"

#include "salalib/importutils.h"
#include "salalib/parsers/ntfp.h"
#include "salalib/parsers/tigerp.h"
#include "salalib/parsers/dxfp.h"

#include "salalib/segmmodules/segmangular.h"
#include "salalib/segmmodules/segmtulip.h"
#include "salalib/segmmodules/segmtulipdepth.h"
#include "salalib/segmmodules/segmmetric.h"
#include "salalib/segmmodules/segmmetricpd.h"
#include "salalib/segmmodules/segmtopological.h"
#include "salalib/segmmodules/segmtopologicalpd.h"
#include "salalib/axialmodules/axialintegration.h"
#include "salalib/axialmodules/axialstepdepth.h"
#include "salalib/vgamodules/vgaisovist.h"
#include "salalib/vgamodules/vgavisualglobal.h"
#include "salalib/vgamodules/vgavisualglobaldepth.h"
#include "salalib/vgamodules/vgavisuallocal.h"
#include "salalib/vgamodules/vgametric.h"
#include "salalib/vgamodules/vgametricdepth.h"
#include "salalib/vgamodules/vgaangular.h"
#include "salalib/vgamodules/vgaangulardepth.h"
#include "salalib/vgamodules/vgathroughvision.h"
#include "salalib/agents/agenthelpers.h"

#include "mgraph440/mgraph.h"

#include "genlib/pafmath.h"
#include "genlib/p2dpoly.h"
#include "genlib/comm.h"

#include "math.h"
#include "time.h"

#include <sstream>
#include <tuple>


MetaGraph::MetaGraph(std::string name)
{ 
   m_name = name;
   m_state = 0; 
   m_view_class = VIEWNONE;
   m_file_version = -1; // <- if unsaved, file version is -1

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
   QtRegion bounds = m_region;
   if (bounds.atZero() && ((getState() & MetaGraph::SHAPEGRAPHS) == MetaGraph::SHAPEGRAPHS)) {
      bounds = getDisplayedShapeGraph().getRegion();
   }
   if (bounds.atZero() && ((getState() & MetaGraph::DATAMAPS) == MetaGraph::DATAMAPS)) {
      bounds = getDisplayedDataMap().getRegion();
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
      val = getDisplayedShapeGraph().getLocationValue(point);
   }
   else if (viewingProcessedShapes()) {
      val = getDisplayedDataMap().getLocationValue(point);
   }

   return val;
}

bool MetaGraph::setGrid( double spacing, const Point2f& offset )
{
   m_state &= ~POINTMAPS;

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

bool MetaGraph::clearPoints()
{
   bool b_return = getDisplayedPointMap().clearPoints();
   return b_return;
}

bool MetaGraph::makeGraph( Communicator *communicator, int algorithm, double maxdist )
{
   // this is essentially a version tag, and remains for historical reasons:
   m_state |= ANGULARGRAPH;

   bool graphMade = false;
   
   try {
      // algorithm is now used for boundary graph option (as a simple boolean)
      graphMade = getDisplayedPointMap().sparkGraph2(communicator, (algorithm != 0), maxdist);
   } 
   catch (Communicator::CancelledException) {
      graphMade = false;
   }

   if (graphMade) {
      setViewClass(SHOWVGATOP);
   }

   return graphMade;
}

bool MetaGraph::unmakeGraph(bool removeLinks)
{
   bool graphUnmade = getDisplayedPointMap().unmake(removeLinks);

   if (graphUnmade) {
      setViewClass(SHOWVGATOP);
   }

   return graphUnmade;
}

bool MetaGraph::analyseGraph( Communicator *communicator, Options options , bool simple_version )   // <- options copied to keep thread safe
{
   bool analysisCompleted = false;

   if (options.point_depth_selection) {
      if (m_view_class & VIEWVGA && !getDisplayedPointMap().isSelected()) {
         return false;
      }
      else if (m_view_class & VIEWAXIAL && !getDisplayedShapeGraph().isSelected()) {
         return false;
      }
   }

   try {
      analysisCompleted = true;
      if (options.point_depth_selection == 1) {
         if (m_view_class & VIEWVGA) {
             analysisCompleted = VGAVisualGlobalDepth().run(communicator, getDisplayedPointMap(), false);
         }
         else if (m_view_class & VIEWAXIAL) {
            if (!getDisplayedShapeGraph().isSegmentMap()) {
                analysisCompleted = AxialStepDepth().run(communicator, getDisplayedShapeGraph(), false);
            }
            else {
                analysisCompleted = SegmentTulipDepth().run(communicator, getDisplayedShapeGraph(), false);
            }
         }
         // REPLACES:
         // Graph::calculate_point_depth_matrix( communicator );
      }
      else if (options.point_depth_selection == 2) {
         if (m_view_class & VIEWVGA) {
             analysisCompleted = VGAMetricDepth().run(communicator, getDisplayedPointMap(), false);
         }
         else if (m_view_class & VIEWAXIAL && getDisplayedShapeGraph().isSegmentMap()) {
             analysisCompleted = SegmentMetricPD().run(communicator, getDisplayedShapeGraph(), false);
         }
      }
      else if (options.point_depth_selection == 3) {
          analysisCompleted = VGAAngularDepth().run(communicator, getDisplayedPointMap(), false);
      }
      else if (options.point_depth_selection == 4) {
         if (m_view_class & VIEWVGA) {
            getDisplayedPointMap().binDisplay( communicator );
         }
         else if (m_view_class & VIEWAXIAL && getDisplayedShapeGraph().isSegmentMap()) {
             analysisCompleted = SegmentTopologicalPD().run(communicator, getDisplayedShapeGraph(), false);
         }
      }
      else if (options.output_type == Options::OUTPUT_ISOVIST) {
         analysisCompleted = VGAIsovist().run(communicator, getDisplayedPointMap(), simple_version);
      }
      else if (options.output_type == Options::OUTPUT_VISUAL) {
          bool localResult = true;
          bool globalResult = true;
          if (options.local) {
              localResult = VGAVisualLocal(options.gates_only).run(communicator, getDisplayedPointMap(), simple_version);
          }
          if (options.global) {
              globalResult = VGAVisualGlobal(options.radius, options.gates_only).run(communicator, getDisplayedPointMap(), simple_version);
          }
          analysisCompleted = globalResult & localResult;
      }
      else if (options.output_type == Options::OUTPUT_METRIC) {
          analysisCompleted = VGAMetric(options.radius, options.gates_only).run(communicator, getDisplayedPointMap(), simple_version);
      }
      else if (options.output_type == Options::OUTPUT_ANGULAR) {
          analysisCompleted = VGAAngular(options.radius, options.gates_only).run(communicator, getDisplayedPointMap(), simple_version);
      }
      else if (options.output_type == Options::OUTPUT_THRU_VISION) {
          analysisCompleted = VGAThroughVision().run(communicator, getDisplayedPointMap(), simple_version);
      }
   } 
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   return analysisCompleted;
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

int MetaGraph::polyBegin(const Line& line)
{
   if (!isEditableMap()) {
      return -1;
   }
   ShapeMap& map = getEditableMap();
   return map.polyBegin(line);
}

bool MetaGraph::polyAppend(int shape_ref, const Point2f& point)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyAppend(shape_ref, point);
}

bool MetaGraph::polyClose(int shape_ref)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyClose(shape_ref);
}

bool MetaGraph::polyCancel(int shape_ref)
{
   if (!isEditableMap()) {
      return false;
   }
   ShapeMap& map = getEditableMap();
   return map.polyCancel(shape_ref);
}

bool MetaGraph::moveSelShape(const Line& line)
{
   bool shapeMoved = false;
   if (m_view_class & VIEWAXIAL) {
      ShapeGraph& map = getDisplayedShapeGraph();
      if (!map.isEditable()) {
         return false;
      }
      if (map.getSelCount() > 1) {
         return false;
      }
      int rowid = *map.getSelSet().begin();
      shapeMoved = map.moveShape(rowid,line);
      if (shapeMoved) {
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
      int rowid = *map.getSelSet().begin();
      shapeMoved = map.moveShape(rowid, line);
      if (shapeMoved) {
         map.clearSel();
      }
   }
   return shapeMoved;
}

//////////////////////////////////////////////////////////////////

// returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
int MetaGraph::makeIsovist(Communicator *communicator, const Point2f& p, double startangle, double endangle, bool simple_version)
{
   int isovistMade = 0;
   // first make isovist
   Isovist iso;

   if (makeBSPtree(communicator)) {
      isovistMade = 1;
      iso.makeit(m_bsp_root, p, m_region, startangle, endangle);
      int shapelayer = getMapRef(m_dataMaps, "Isovists");
      if (shapelayer == -1) {
         m_dataMaps.emplace_back("Isovists",ShapeMap::DATAMAP);
         setDisplayedDataMapRef(m_dataMaps.size() - 1);
         shapelayer = m_dataMaps.size() - 1;
         m_state |= DATAMAPS;
         isovistMade = 2;
      }
      ShapeMap& map = m_dataMaps[shapelayer];
      // false: closed polygon, true: isovist
      int polyref = map.makePolyShape(iso.getPolygon(),false);  
      map.getAllShapes()[polyref].setCentroid(p);
      map.overrideDisplayedAttribute(-2);
      map.setDisplayedAttribute(-1);
      setViewClass(SHOWSHAPETOP);
      AttributeTable& table = map.getAttributeTable();
      AttributeRow& row = table.getRow(AttributeKey(polyref));
      iso.setData(table,row, simple_version);
   }
   return isovistMade;
}

static std::pair<double,double> startendangle( Point2f vec, double fov)
{
   std::pair<double,double> angles;
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
   int pathMade = 0;

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
      std::set<int> selset = map->getSelSet();
      std::map<int,SalaShape>& shapes = map->getAllShapes();
      for (auto& shapeRef: selset) {
         const SalaShape& path = shapes.at(shapeRef);
         if (path.isLine() || path.isPolyLine()) {
            if (first) {
               pathMade = 1;
               isovistmapref = getMapRef(m_dataMaps, "Isovists");
               if (isovistmapref == -1) {
                  m_dataMaps.emplace_back("Isovists",ShapeMap::DATAMAP);
                  isovistmapref = m_dataMaps.size() - 1;
                  setDisplayedDataMapRef(isovistmapref);
                  pathMade = 2;
               }
               isovists = &(m_dataMaps[isovistmapref]);
               first = false;
            }
            // now make an isovist:
            Isovist iso;
            // 
            std::pair<double,double> angles;
            angles.first = 0.0;
            angles.second = 0.0;
            //
            if (path.isLine()) {
               Point2f start = path.getLine().t_start();
               Point2f vec = path.getLine().vector();
               if (fov < 2.0 * M_PI) {
                  angles = startendangle(vec, fov);
               }
               iso.makeit(m_bsp_root, start, m_region, angles.first, angles.second);
               int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
               isovists->getAllShapes()[polyref].setCentroid(start);
               AttributeTable& table = isovists->getAttributeTable();
               AttributeRow& row = table.getRow(AttributeKey(polyref));
               iso.setData(table,row, simple_version);
            }
            else {
               for (size_t i = 0; i < path.m_points.size() - 1; i++) {
                  Line li = Line(path.m_points[i],path.m_points[i+1]);
                  Point2f start = li.t_start();
                  Point2f vec = li.vector();
                  if (fov < 2.0 * M_PI) {
                     angles = startendangle(vec, fov);
                  }
                  iso.makeit(m_bsp_root, start, m_region, angles.first, angles.second);
                  int polyref = isovists->makePolyShape(iso.getPolygon(),false);  
                  isovists->getAllShapes().find(polyref)->second.setCentroid(start);
                  AttributeTable& table = isovists->getAttributeTable();
                  AttributeRow& row = table.getRow(AttributeKey(polyref));
                  iso.setData(table,row, simple_version);
               }
            }
         }
      }
      if (isovists) {
         isovists->overrideDisplayedAttribute(-2);
         isovists->setDisplayedAttribute(-1);
         setDisplayedDataMapRef(isovistmapref);
      }
   }
   return pathMade;
}

// this version uses your own isovist (and assumes no communicator required for BSP tree
bool MetaGraph::makeIsovist(const Point2f& p, Isovist& iso)
{
   if (makeBSPtree()) {
      iso.makeit(m_bsp_root, p, m_region);
      return true;
   }
   return false;
}

bool MetaGraph::makeBSPtree(Communicator *communicator)
{
   if (m_bsp_tree) {
      return true;
   }

   std::vector<TaggedLine> partitionlines;
   for (const auto& pixelGroup: m_drawingFiles) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         // chooses the first editable layer it can find:
         if (pixel.isShown()) {
             auto refShapes = pixel.getAllShapes();
             int k = -1;
             for (const auto& refShape: refShapes) {
                 k++;
                 std::vector<Line> newLines = refShape.second.getAsLines();
                 // I'm not sure what the tagging was meant for any more,
                 // tagging at the moment tags the *polygon* it was original attached to
                 // must check it is not a zero length line:
                 for(const Line& line: newLines) {
                     if(line.length() > 0.0) {
                         partitionlines.push_back(TaggedLine(line,k));
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

      time_t atime = 0;
      communicator->CommPostMessage( Communicator::NUM_RECORDS, partitionlines.size() );
      qtimer( atime, 0 );

      try {
         BSPTree::make(communicator,atime,partitionlines,m_bsp_root);
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

int MetaGraph::addShapeGraph(std::unique_ptr<ShapeGraph>& shapeGraph) {
    m_shapeGraphs.push_back(std::move(shapeGraph));
    int mapref= int(m_shapeGraphs.size() - 1);
    setDisplayedShapeGraphRef(mapref);
    m_state |= SHAPEGRAPHS;
    setViewClass(SHOWAXIALTOP);
    return mapref;
}

int MetaGraph::addShapeGraph(const std::string& name, int type)
{
    std::unique_ptr<ShapeGraph> shapeGraph(new ShapeGraph(name, type));
    int mapref = addShapeGraph(shapeGraph);
    // add a couple of default columns:
    AttributeTable& table = m_shapeGraphs[size_t(mapref)]->getAttributeTable();
    int connIdx = table.insertOrResetLockedColumn("Connectivity");
    if ((type & ShapeMap::LINEMAP) != 0) {
        table.insertOrResetLockedColumn("Line Length");
    }
    m_shapeGraphs[mapref]->setDisplayedAttribute(connIdx);
    return mapref;
}
int MetaGraph::addShapeMap(const std::string& name)
{
   m_dataMaps.emplace_back(name,ShapeMap::DATAMAP);
   m_state |= DATAMAPS;
   setViewClass(SHOWSHAPETOP);
   return m_dataMaps.size() - 1;
}
void MetaGraph::removeDisplayedMap()
{
   int ref = getDisplayedMapRef();
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      removePointMap(ref);
      if (m_pointMaps.empty()) {
         setViewClass(SHOWHIDEVGA);
         m_state &= ~POINTMAPS;
      }
      break;
   case VIEWAXIAL:
      removeShapeGraph(ref);
      if (m_shapeGraphs.empty()) {
         setViewClass(SHOWHIDEAXIAL);
         m_state &= ~SHAPEGRAPHS;
      }
      break;
   case VIEWDATA:
      removeDataMap(ref);
      if (m_dataMaps.empty()) {
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

   bool converted = true;
   
   try {
      auto shapeGraph = MapConverter::convertDrawingToAxial( comm, layer_name, m_drawingFiles );
      int mapref = addShapeGraph(shapeGraph);
      setDisplayedShapeGraphRef(mapref);
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

bool MetaGraph::convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool converted = true;
   
   try {
       auto shapeGraph = MapConverter::convertDataToAxial( comm, layer_name, getDisplayedDataMap(), pushvalues );
       addShapeGraph(shapeGraph);

       m_shapeGraphs.back()->overrideDisplayedAttribute(-2); // <- override if it's already showing
       m_shapeGraphs.back()->setDisplayedAttribute(
                   m_shapeGraphs.back()->getAttributeTable().getColumnIndex("Connectivity") );

       setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 1));
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      if (!keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

// typeflag: -1 convert drawing to convex, 0 or 1, convert data to convex (1 is pushvalues)
bool MetaGraph::convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal, int shapeMapType, bool copydata)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS; // and convex maps...

   bool converted = false;
   
   try {
      int mapref = -1;
      if (shapeMapType == ShapeMap::DRAWINGMAP) {
          auto shapeGraph = MapConverter::convertDrawingToConvex( comm, layer_name, m_drawingFiles );
          mapref = addShapeGraph(shapeGraph);
      }
      else if (shapeMapType == ShapeMap::DATAMAP) {
          auto shapeGraph = MapConverter::convertDataToConvex( comm, layer_name, getDisplayedDataMap(), copydata );
          mapref = addShapeGraph(shapeGraph);
      }

      m_shapeGraphs.back()->overrideDisplayedAttribute( -2 ); // <- override if it's already showing
      m_shapeGraphs.back()->setDisplayedAttribute( -1 );
      setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 1));

      if (mapref != -1) {
         converted = true;
      }
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      if (shapeMapType != ShapeMap::DRAWINGMAP && !keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

bool MetaGraph::convertDrawingToSegment(Communicator *comm, std::string layer_name)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool converted = true;
   
   try {
       auto shapeGraph = MapConverter::convertDrawingToSegment( comm, layer_name, m_drawingFiles );
       addShapeGraph(shapeGraph);

       setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 1));
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

bool MetaGraph::convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool converted = true;
   
   try {
       auto shapeGraph = MapConverter::convertDataToSegment( comm, layer_name, getDisplayedDataMap(), pushvalues );
       addShapeGraph(shapeGraph);

       m_shapeGraphs.back()->overrideDisplayedAttribute( -2 ); // <- override if it's already showing
       m_shapeGraphs.back()->setDisplayedAttribute( -1 );
       setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 1));
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      if (!keeporiginal) {
         removeDataMap( getDisplayedDataMapRef() );
         if (m_dataMaps.empty()) {
            setViewClass(SHOWHIDESHAPE);
            m_state &= ~DATAMAPS;
         }
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

// note: type flag says whether this is graph to data map or drawing to data map

bool MetaGraph::convertToData(Communicator *, std::string layer_name, bool keeporiginal, int shapeMapType, bool copydata)
{
   int oldstate = m_state;

   m_state &= ~DATAMAPS;

   bool converted = false;
   
   try {
      // This should be much easier than before,
      // simply move the shapes from the drawing layer
      // note however that more than one layer might be combined:
      // create map layer...
      m_dataMaps.emplace_back(layer_name,ShapeMap::DATAMAP);
      int destmapref = m_dataMaps.size() - 1;
      ShapeMap& destmap = m_dataMaps.back();
      AttributeTable& table = destmap.getAttributeTable();
      int count = 0;
      //
      // drawing to data
      if (shapeMapType == ShapeMap::DRAWINGMAP) {
         int layercol = destmap.addAttribute("Drawing Layer");
         // add all visible layers to the set of map:
         for (const auto& pixelGroup: m_drawingFiles) {
            int j = 0;
            for (const auto& pixel: pixelGroup.m_spacePixels) {
               if (pixel.isShown()) {
                  auto refShapes = pixel.getAllShapes();
                  for (const auto& refShape: refShapes) {
                     int key = destmap.makeShape(refShape.second);
                     table.getRow(AttributeKey(key)).setValue(layercol,float(j+1));
                     count++;
                  }
                  pixel.setShow(false);
               }
               j++;
            }
         }
      }
      // convex, axial or segment graph to data (similar)
      else {
         ShapeGraph& sourcemap = getDisplayedShapeGraph();
         count = sourcemap.getShapeCount();
         // take viewed graph and push all geometry to it (since it is *all* geometry, pushing is easy)
         int copyflag = copydata ? (ShapeMap::COPY_GEOMETRY | ShapeMap::COPY_ATTRIBUTES) : (ShapeMap::COPY_GEOMETRY);
         destmap.copy(sourcemap, copyflag);
      }
      //
      if (count == 0) {
         // if no objects converted then a crash is caused, so remove it:
         removeDataMap(destmapref);
         converted = false;
      }
      else {
         // we can stop here! -- remember to set up display:
         setDisplayedDataMapRef(destmapref);
         destmap.invalidateDisplayedAttribute();
         destmap.setDisplayedAttribute(-1);
         converted = true;
      }
   } 
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      if (shapeMapType != ShapeMap::DRAWINGMAP && !keeporiginal) {
         removeShapeGraph( getDisplayedShapeGraphRef() );
         if (m_shapeGraphs.empty()) {
            setViewClass(SHOWHIDEAXIAL);
            m_state &= ~SHAPEGRAPHS;
         }
      }
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return converted;
}

bool MetaGraph::convertToDrawing(Communicator *, std::string layer_name, bool fromDisplayedDataMap)
{
   bool converted = false;

   int oldstate = m_state;

   m_state &= ~LINEDATA;

   try {
      const ShapeMap *sourcemap;
      if (fromDisplayedDataMap) {
         sourcemap = &(getDisplayedDataMap());
      }
      else {
         sourcemap = &(getDisplayedShapeGraph());
      }
      //
      if (sourcemap->getShapeCount() != 0) {
         // this is very simple: create a new drawing layer, and add the data...
         auto group = m_drawingFiles.begin();
         for (; group != m_drawingFiles.end(); ++group) {
            if (group->getName() == "Converted Maps") {
               break;
            }
         }
         if (group == m_drawingFiles.end()) {
            m_drawingFiles.emplace_back(std::string("Converted Maps"));
            group = std::prev(m_drawingFiles.end());
         }
         group->m_spacePixels.emplace_back(layer_name);
         group->m_spacePixels.back().copy(*sourcemap, ShapeMap::COPY_GEOMETRY);
         //
         // dummy set still required:
         group->m_spacePixels.back().invalidateDisplayedAttribute();
         group->m_spacePixels.back().setDisplayedAttribute(-1);
         //      
         // three levels of merge region:
         if (group->m_spacePixels.size() == 1) {
            group->m_region = group->m_spacePixels.back().getRegion();
         }
         else {
            group->m_region = runion(group->m_region, group->m_spacePixels.back().getRegion());
         }
         if (m_drawingFiles.size() == 1) {
            m_region = group->m_region;
         }
         else {
            m_region = runion(m_region, group->m_region);
         }
         //
         converted = true;
      }
      converted = true;
   }
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      m_state |= LINEDATA;
   }

   return converted;
}

bool MetaGraph::convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues, double stubremoval)
{
   int oldstate = m_state;

   m_state &= ~SHAPEGRAPHS;

   bool converted = true;

   int orig_ref = getDisplayedShapeGraphRef();

   try {
       if (orig_ref == -1) {
          return false;
       }

       auto shapeGraph = MapConverter::convertAxialToSegment(comm, getDisplayedShapeGraph(),
                                                             layer_name, keeporiginal,
                                                             pushvalues, stubremoval);
       addShapeGraph(shapeGraph);

       m_shapeGraphs.back()->overrideDisplayedAttribute(-2); // <- override if it's already showing
       m_shapeGraphs.back()->setDisplayedAttribute(
                   m_shapeGraphs.back()->getAttributeTable().getColumnIndex("Connectivity") );

       setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 1));
   }
   catch (Communicator::CancelledException) {
      converted = false;
   }

   m_state |= oldstate;

   if (converted) {
      if (!keeporiginal) {
         removeShapeGraph(orig_ref);
      }
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return converted;
}

int MetaGraph::loadMifMap(Communicator *comm, std::istream& miffile, std::istream& midfile)
{
   int oldstate = m_state;
   m_state &= ~DATAMAPS;

   int mapLoaded = -1;
   
   try {
      // create map layer...
      m_dataMaps.emplace_back(comm->GetMBInfileName(),ShapeMap::DATAMAP);
      int mifmapref = m_dataMaps.size() - 1;
      ShapeMap& mifmap = m_dataMaps.back();
      mapLoaded = mifmap.loadMifMap(miffile, midfile);
      if (mapLoaded == MINFO_OK || mapLoaded == MINFO_MULTIPLE) { // multiple is just a warning
          // display an attribute:
         mifmap.overrideDisplayedAttribute(-2);
         mifmap.setDisplayedAttribute(-1);
         setDisplayedDataMapRef(mifmapref);
      }
      else { // error: undo!
         removeDataMap(mifmapref);
      }
   } 
   catch (Communicator::CancelledException) {
      mapLoaded = -1;
   }

   m_state = oldstate;

   if (mapLoaded == MINFO_OK || mapLoaded == MINFO_MULTIPLE) { // MINFO_MULTIPLE is simply a warning
      m_state |= DATAMAPS;
      setViewClass(SHOWSHAPETOP);
   }

   return mapLoaded;
}  

bool MetaGraph::makeAllLineMap( Communicator *communicator, const Point2f& seed )
{
   int oldstate = m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload)
   m_view_class &= ~VIEWAXIAL;   // Also clear the view_class flag

   bool mapMade = true;

   try {
       // this is an index to look up the all line map, used by UI to determine if can make fewest line map
       // note: it is not saved for historical reasons
       if (m_all_line_map != -1) {
          removeShapeGraph(m_all_line_map);
          m_all_line_map = -1;
       }

      m_shapeGraphs.push_back(std::unique_ptr<AllLineMap>(new AllLineMap(communicator, m_drawingFiles, seed)));

      m_all_line_map = int(m_shapeGraphs.size() - 1);
      setDisplayedShapeGraphRef(m_all_line_map);
   } 
   catch (Communicator::CancelledException) {
      mapMade = false;
   }

   m_state = oldstate;

   if (mapMade) {
      m_state |= SHAPEGRAPHS;
      setViewClass(SHOWAXIALTOP);
   }

   return mapMade;
}


bool MetaGraph::makeFewestLineMap( Communicator *communicator, int replace )
{
   int oldstate= m_state;
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool mapMade = true;

   try {
       // no all line map
       if (m_all_line_map == -1) {
          return false;
       }

       AllLineMap* alllinemap = dynamic_cast<AllLineMap*>(m_shapeGraphs[size_t(m_all_line_map)].get());

       if(alllinemap == nullptr) {
           throw depthmapX::RuntimeException("Failed to cast from ShapeGraph to AllLineMap");
       }

       // waiting for C++17...
       std::unique_ptr<ShapeGraph> fewestlinemap_subsets, fewestlinemap_minimal;
       std::tie(fewestlinemap_subsets, fewestlinemap_minimal) = alllinemap->extractFewestLineMaps(communicator);

       if (replace != 0) {
           int index = -1;

           for(size_t i = 0; i < m_shapeGraphs.size(); i++) {
               if(m_shapeGraphs[i]->getName() == "Fewest-Line Map (Subsets)" ||
                       m_shapeGraphs[i]->getName() == "Fewest Line Map (Subsets)") {
                   index = int(i);
               }
           }

           if(index != -1) {
               removeShapeGraph(index);
           }

           for(size_t i = 0; i < m_shapeGraphs.size(); i++) {
               if(m_shapeGraphs[i]->getName() == "Fewest-Line Map (Subsets)" ||
                         m_shapeGraphs[i]->getName() == "Fewest Line Map (Subsets)") {
                   index = int(i);
               }
           }

           if(index != -1) {
               removeShapeGraph(index);
           }
       }
       addShapeGraph(fewestlinemap_subsets);
       addShapeGraph(fewestlinemap_minimal);

       setDisplayedShapeGraphRef(int(m_shapeGraphs.size() - 2));

   } 
   catch (Communicator::CancelledException) {
      mapMade = false;
   }

   m_state = oldstate;

   if (mapMade) {
      m_state |= SHAPEGRAPHS;   // note: should originally have at least one axial map
      setViewClass(SHOWAXIALTOP);
   }

   return mapMade;
}

bool MetaGraph::analyseAxial( Communicator *communicator, Options options, bool ) // options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool analysisCompleted = false;

   try {
       analysisCompleted = AxialIntegration(options.radius_set, options.weighted_measure_col, options.choice, options.fulloutput,
                        options.local)
           .run(communicator, getDisplayedShapeGraph(), false);
   } 
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   m_state |= SHAPEGRAPHS;

   return analysisCompleted;
}

bool MetaGraph::analyseSegmentsTulip( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload)

   bool analysisCompleted = false;

   try {
       analysisCompleted = SegmentTulip(options.radius_set, options.sel_only, options.tulip_bins, options.weighted_measure_col,
                    options.radius_type, options.choice)
           .run(communicator, getDisplayedShapeGraph(), false);
   }
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   m_state |= SHAPEGRAPHS;

   return analysisCompleted;
}

bool MetaGraph::analyseSegmentsAngular( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload)

   bool analysisCompleted = false;

   try {
       analysisCompleted = SegmentAngular(options.radius_set).run(communicator, getDisplayedShapeGraph(), false);
   }
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   m_state |= SHAPEGRAPHS;

   return analysisCompleted;
}

bool MetaGraph::analyseTopoMetMultipleRadii( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload)

   bool analysisCompleted = true;

   try {
      // note: "output_type" reused for analysis type (either 0 = topological or 1 = metric)
      for(size_t r = 0; r < options.radius_set.size(); r++) {
          if(options.output_type == 0) {
              if(!SegmentTopological(options.radius, options.sel_only).run(communicator, getDisplayedShapeGraph(), false))
                  analysisCompleted = false;
          } else {
              if(!SegmentMetric(options.radius, options.sel_only).run(communicator, getDisplayedShapeGraph(), false))
                  analysisCompleted = false;
          }
      }
   }
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   m_state |= SHAPEGRAPHS;

   return analysisCompleted;
}

bool MetaGraph::analyseTopoMet( Communicator *communicator, Options options ) // <- options copied to keep thread safe
{
   m_state &= ~SHAPEGRAPHS;      // Clear axial map data flag (stops accidental redraw during reload) 

   bool analysisCompleted = false;

   try {
      // note: "output_type" reused for analysis type (either 0 = topological or 1 = metric)
       if(options.output_type == 0) {
           analysisCompleted = SegmentTopological(options.radius, options.sel_only).run(communicator, getDisplayedShapeGraph(), false);
       } else {
           analysisCompleted = SegmentMetric(options.radius, options.sel_only).run(communicator, getDisplayedShapeGraph(), false);
       }
   } 
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   m_state |= SHAPEGRAPHS;

   return analysisCompleted;
}

int MetaGraph::loadLineData( Communicator *communicator, int load_type )
{
    if (load_type & DXF) {
       // separate the stream and the communicator, allowing non-file streams read
       return depthmapX::importFile(*this, *communicator, communicator, communicator->GetMBInfileName(), depthmapX::ImportType::DRAWINGMAP, depthmapX::ImportFileType::DXF);
    }

   m_state &= ~LINEDATA;      // Clear line data flag (stops accidental redraw during reload) 

   // if bsp tree exists 
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

   if (load_type & REPLACE) {
      m_drawingFiles.clear();
   }

   m_drawingFiles.emplace_back(communicator->GetMBInfileName());

   if (load_type & CAT) {
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
         m_drawingFiles.pop_back();
         return 0;
      }
      catch (std::invalid_argument&) {
         m_drawingFiles.pop_back();
         return -1;
      }
      catch (std::out_of_range&) {
         m_drawingFiles.pop_back();
         return -1;
      }

      if (communicator->IsCancelled()) {
         m_drawingFiles.pop_back();
         return 0;
      }

      m_drawingFiles.back().m_region = map.getRegion();;

      for (auto layer: map.layers) {

         m_drawingFiles.back().m_spacePixels.emplace_back(layer.getName());
         m_drawingFiles.back().m_spacePixels.back().init(layer.getLineCount(), map.getRegion());

         for (auto geometry: layer.geometries) {
            for (auto& line: geometry.lines) {
               m_drawingFiles.back().m_spacePixels.back().makeLineShape( line );
            }
         }

         // TODO: Investigate why setDisplayedAttribute needs to be set to -2 first
         m_drawingFiles.back().m_spacePixels.back().setDisplayedAttribute(-2);
         m_drawingFiles.back().m_spacePixels.back().setDisplayedAttribute(-1);
      }
   }

   if (m_drawingFiles.size() == 1) {
      m_region = m_drawingFiles.back().m_region;
   }
   else {
      m_region = runion(m_region, m_drawingFiles.back().m_region);
   }

   m_state |= LINEDATA;

   return 1;
}

// From: Alasdair Turner (2004) - Depthmap 4: a researcher's handbook (p. 6):
// [..] CAT, which stands for Chiron and Alasdair Transfer Format [..]

void MetaGraph::writeMapShapesAsCat(ShapeMap& map, std::ostream &stream) {
    stream << "CAT" << std::endl;
    for (auto refShape: map.getAllShapes()) {
        SalaShape& shape = refShape.second;
        if(shape.isPolyLine() || shape.isPolygon()) {
            stream << "Begin " << (shape.isPolyLine() ? "Polyline" : "Polygon") << std::endl;
            for (Point2f p: shape.m_points) {
                stream << p.x << " " << p.y << std::endl;
            }
            stream << "End " << (shape.isPolyLine() ? "Polyline" : "Polygon") << std::endl;
        } else if(shape.isLine()) {
            stream << "Begin Polyline" << std::endl;
            stream << shape.getLine().ax() << " " << shape.getLine().ay() << std::endl;
            stream << shape.getLine().bx() << " " << shape.getLine().by() << std::endl;
            stream << "End Polyline" << std::endl;
        }
    }
}

int MetaGraph::loadCat( std::istream& stream, Communicator *communicator )
{
   if (communicator) {
      long size = communicator->GetInfileSize();
      communicator->CommPostMessage( Communicator::NUM_RECORDS, size );
   }

   time_t atime = 0;
   
   qtimer( atime, 0 );

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
   m_drawingFiles.back().m_region = QtRegion(min_point, max_point);
   m_drawingFiles.back().m_spacePixels.emplace_back();
   m_drawingFiles.back().m_spacePixels.back().init( numlines, QtRegion(min_point, max_point) );

   // in MSVC 6, ios::eof remains set and it needs to be cleared.
   // in MSVC 8 it's even worse: it won't even seekg until eof flag has been cleared
   stream.clear();
   stream.seekg(0, std::ios::beg);

   parsing = 0;
   first = true;
   std::vector<Point2f> points;

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
                  m_drawingFiles.back().m_spacePixels.back().makePolyShape(points, false);
               }
               else { // polyline
                  m_drawingFiles.back().m_spacePixels.back().makePolyShape(points, true);
               }
            }
            else if (points.size() == 2) {
               m_drawingFiles.back().m_spacePixels.back().makeLineShape(Line(points[0],points[1]));
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
         if (qtimer( atime, 500 )) {
            if (communicator->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            communicator->CommPostMessage( Communicator::CURRENT_RECORD, size );
         }
      }
   }

   m_drawingFiles.back().m_spacePixels.back().setDisplayedAttribute(-2);
   m_drawingFiles.back().m_spacePixels.back().setDisplayedAttribute(-1);

   return 1;
}

int MetaGraph::loadRT1(const std::vector<std::string>& fileset, Communicator *communicator)
{
   TigerMap map;

   try {
      map.parse( fileset, communicator );
   }
   catch (Communicator::CancelledException) {
      m_drawingFiles.pop_back();
      return 0;
   }
   catch (std::invalid_argument&) {
      m_drawingFiles.pop_back();
      return -1;
   }
   catch (std::out_of_range&) {
      m_drawingFiles.pop_back();
      return -1;
   }

   if (communicator->IsCancelled()) {
      m_drawingFiles.pop_back();
      return 0;
   }

   m_drawingFiles.back().m_region = QtRegion(map.getBottomLeft(), map.getTopRight());

   // for each category
   for (auto val: map.m_categories) {
      ShapeMap shapeMap = ShapeMap(val.first);
      shapeMap.init(val.second.chains.size(), map.getRegion() );

      // for each chains in category:
      for (size_t j = 0; j < val.second.chains.size(); j++) {
         // for each node pair in each category
         for (size_t k = 0; k < val.second.chains[j].lines.size(); k++) {
            shapeMap.makeLineShape( val.second.chains[j].lines[k] );
         }
      }

      shapeMap.setDisplayedAttribute(-2);
      shapeMap.setDisplayedAttribute(-1);
      m_drawingFiles.back().m_spacePixels.emplace_back(std::move(shapeMap));
   
   }

   return 1;
}

ShapeMap &MetaGraph::createNewShapeMap(depthmapX::ImportType mapType, std::string name) {

    if (mapType == depthmapX::ImportType::DATAMAP) {
        m_dataMaps.emplace_back(name, ShapeMap::DATAMAP);
        m_dataMaps.back().setDisplayedAttribute(0);
        return m_dataMaps.back();
    }
    // depthmapX::ImportType::DRAWINGMAP
    m_drawingFiles.back().m_spacePixels.emplace_back(name);
    return m_drawingFiles.back().m_spacePixels.back();
}

void MetaGraph::deleteShapeMap(depthmapX::ImportType mapType, ShapeMap &shapeMap) {

    switch(mapType) {
        case depthmapX::ImportType::DRAWINGMAP: {
            // go through the files to find if the layer is in one of them
            // if it is, remove it and if the remaining file is empty then
            // remove that too
            auto pixelGroup = m_drawingFiles.begin();
            for (; pixelGroup != m_drawingFiles.begin(); ++pixelGroup) {
                auto mapToRemove = pixelGroup->m_spacePixels.end();
                auto pixel = pixelGroup->m_spacePixels.begin();
                for (; pixel != pixelGroup->m_spacePixels.end(); ++pixel) {
                    if(&(*pixel) == &shapeMap) {
                        mapToRemove = pixel;
                        break;
                    }
                }
                if(mapToRemove != pixelGroup->m_spacePixels.end()) {
                    pixelGroup->m_spacePixels.erase(mapToRemove);
                    if(pixelGroup->m_spacePixels.size() == 0) {
                        m_drawingFiles.erase(pixelGroup);
                    }
                    break;
                }
            }
        }
        case depthmapX::ImportType::DATAMAP: {
            for(size_t i = 0; i < m_dataMaps.size(); i++) {
                if(&m_dataMaps[i] == &shapeMap) {
                    m_dataMaps.erase(m_dataMaps.begin() + i);
                    break;
                }
            }
        }
    }
}

void MetaGraph::updateParentRegions(ShapeMap &shapeMap) {
    if(m_drawingFiles.back().m_region.atZero()) {
        m_drawingFiles.back().m_region = shapeMap.getRegion();
    } else {
        m_drawingFiles.back().m_region = runion(m_drawingFiles.back().m_region, shapeMap.getRegion());
    }
    if(m_region.atZero()) {
        m_region = m_drawingFiles.back().m_region;
    } else {
        m_region = runion(m_region, m_drawingFiles.back().m_region);
    }
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

   bool valuesPushed = pushValuesToLayer(sourcetype, sourcelayer, desttype, destlayer, col_in,
                                         col_out, push_func, count_col);

   m_state = oldstate;

   return valuesPushed;
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
      if ((table_out.hasColumn(name) && table_out.getColumn(table_out.getColumnIndex(name)).isLocked()) || name == "Object Count") {
         name = std::string("Copied ") + name;
      }
      col_out = table_out.insertOrResetColumn(name);
   }

   int col_count = -1;
   if (count_col) {
      col_count = table_out.insertOrResetColumn("Object Count");
      if (col_count <= col_out) {
         col_out++;
      }
   }

   if (sourcetype & VIEWDATA) {

      for (auto iter_out = table_out.begin(); iter_out != table_out.end(); iter_out++) {
         int key_out = iter_out->getKey().value;
         std::vector<int> gatelist;
         if (desttype == VIEWVGA) {
             if (!isObjectVisible(m_pointMaps[destlayer].m_layers, iter_out->getRow())) {
                continue;
             }
            gatelist = m_dataMaps[sourcelayer].pointInPolyList(m_pointMaps[destlayer].getPoint(key_out).m_location);
         }
         else if (desttype == VIEWAXIAL) {
             if (!isObjectVisible(m_shapeGraphs[destlayer]->getLayers(), iter_out->getRow())) {
                continue;
             }
            auto shapeMap = m_shapeGraphs[destlayer]->getAllShapes();
            gatelist = m_dataMaps[sourcelayer].shapeInPolyList(shapeMap[key_out]);
         }
         else if (desttype == VIEWDATA) {
            if (sourcelayer == destlayer) {
               // error: pushing to same map
               return false;
            }
            if (!isObjectVisible(m_dataMaps[destlayer].getLayers(), iter_out->getRow())) {
               continue;
            }
            auto dataMap = m_dataMaps[destlayer].getAllShapes();
            gatelist = m_dataMaps[sourcelayer].shapeInPolyList(dataMap[key_out]);
         }
         double val = -1.0;
         int count = 0;
         for (int gate: gatelist) {
             AttributeRow &row_in =
                 m_dataMaps[sourcelayer].getAttributeRowFromShapeIndex(gate);

            if (isObjectVisible(m_dataMaps[sourcelayer].getLayers(), row_in)) {
               double thisval = row_in.getValue(col_in);
               pushValue(val,count,thisval,push_func);
            }
         }
         if (push_func == PUSH_FUNC_AVG && val != -1.0) {
            val /= double(count);
         }
         iter_out->getRow().setValue(col_out,float(val));
         if (count_col) {
            iter_out->getRow().setValue(col_count,float(count));
         }
      }
   }
   else {
      // prepare a temporary value table to store counts and values
      std::vector<double> vals(table_out.getNumRows());
      std::vector<int> counts(table_out.getNumRows());

      for (size_t i = 0; i < table_out.getNumRows(); i++) {
         counts[i] = 0; // count set to zero for all
         vals[i] = -1;
      }

      if (sourcetype & VIEWVGA) {
         for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
            int pix_in = iter_in->getKey().value;
            if (!isObjectVisible(m_pointMaps[sourcelayer].getLayers(), iter_in->getRow())) {
               continue;
            }
            std::vector<int> gatelist;
            if (desttype == VIEWDATA) {
                gatelist = m_dataMaps[size_t(destlayer)].pointInPolyList(m_pointMaps[size_t(sourcelayer)].getPoint(pix_in).m_location);
                double thisval = iter_in->getRow().getValue(col_in);
                for (int gate: gatelist) {
                    AttributeRow &row_out =
                        m_dataMaps[destlayer].getAttributeRowFromShapeIndex(gate);
                   if (isObjectVisible(m_dataMaps[destlayer].getLayers(), row_out)) {
                      double& val = vals[gate];
                      int& count = counts[gate];
                      pushValue(val,count,thisval,push_func);
                   }
                }
            } else if (desttype == VIEWAXIAL) {
               // note, "axial" could be convex map, and hence this would be a valid operation
               gatelist = m_shapeGraphs[size_t(destlayer)]->pointInPolyList(m_pointMaps[size_t(sourcelayer)].getPoint(pix_in).m_location);
               double thisval = iter_in->getRow().getValue(col_in);
               for (int gate: gatelist) {
                   int key_out = m_shapeGraphs[destlayer]->getShapeRefFromIndex(gate)->first;
                   AttributeRow &row_out =
                       table_out.getRow(AttributeKey(key_out));
                  if (isObjectVisible(m_shapeGraphs[destlayer]->getLayers(), row_out)) {
                     double& val = vals[gate];
                     int& count = counts[gate];
                     pushValue(val,count,thisval,push_func);
                  }
               }
            }
         }
      }
      else if (sourcetype & VIEWAXIAL) {
         // note, in the spirit of mapping fewer objects in the gate list, it is *usually* best to 
         // perform axial -> gate map in this direction
         // however, "Axial" to VGA, likely to have more points than "axial" shapes, should probably be performed using the first 
         // algorithm
         for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
            int key_in = iter_in->getKey().value;
            if (!isObjectVisible(m_shapeGraphs[size_t(sourcelayer)]->getLayers(),iter_in->getRow())) {
               continue;
            }
            std::vector<int> gatelist;
            if (desttype == VIEWDATA) {
               auto dataMap = m_shapeGraphs[size_t(sourcelayer)]->getAllShapes();
               gatelist = m_dataMaps[size_t(destlayer)].shapeInPolyList(dataMap[key_in]);
               double thisval = iter_in->getRow().getValue(col_in);
               for (int gate: gatelist) {
                   int key_out = m_dataMaps[destlayer].getShapeRefFromIndex(gate)->first;
                   AttributeRow &row_out =
                       table_out.getRow(AttributeKey(key_out));
                   if (isObjectVisible(m_dataMaps[size_t(destlayer)].getLayers(), row_out)) {
                     double& val = vals[gate];
                     int& count = counts[gate];
                     pushValue(val,count,thisval,push_func);
                  }
               }
            }
            else if (desttype == VIEWAXIAL) {
               auto shapeMap = m_shapeGraphs[size_t(sourcelayer)]->getAllShapes();
               gatelist = m_shapeGraphs[size_t(destlayer)]->shapeInPolyList(shapeMap[key_in]);
               double thisval = iter_in->getRow().getValue(col_in);
               for (int gate: gatelist) {
                   int key_out = m_shapeGraphs[destlayer]->getShapeRefFromIndex(gate)->first;
                   AttributeRow &row_out =  table_out.getRow(AttributeKey(key_out));
                   if (isObjectVisible(m_shapeGraphs[destlayer]->getLayers(), row_out)) {
                     double& val = vals[gate];
                     int& count = counts[gate];
                     pushValue(val,count,thisval,push_func);
                  }
               }
            }
         }
      }
      int i = -1;
      for (auto iter = table_out.begin(); iter != table_out.end(); iter++) {
          i++;

         if (!isObjectVisible(m_shapeGraphs[destlayer]->getLayers(), iter->getRow())) {
            continue;
         }
         if (push_func == PUSH_FUNC_AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
         }
         iter->getRow().setValue(col_out,float(vals[i]));
         if (count_col) {
            iter->getRow().setValue(col_count,float(counts[i]));
         }
      }
   }

   // display new data in the relevant layer
   if (desttype == VIEWVGA) {
      m_pointMaps[destlayer].overrideDisplayedAttribute(-2);
      m_pointMaps[destlayer].setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWAXIAL) {
      m_shapeGraphs[destlayer]->overrideDisplayedAttribute(-2);
      m_shapeGraphs[destlayer]->setDisplayedAttribute(col_out);
   }
   else if (desttype == VIEWDATA) {
      m_dataMaps[destlayer].overrideDisplayedAttribute(-2);
      m_dataMaps[destlayer].setDisplayedAttribute(col_out);
   }


   return true;
}

// Agent functionality: some of it still kept here with the metagraph
// (to allow push value to layer and back again)

void MetaGraph::runAgentEngine(Communicator *comm)
{
   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   if (m_agent_engine.m_gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      int colgates = table.insertOrResetColumn(g_col_gate);
      pushValuesToLayer(VIEWDATA,m_agent_engine.m_gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
      table.insertOrResetColumn(g_col_gate_counts);
   }


   m_agent_engine.run(comm, &(getDisplayedPointMap()) );

   if(m_agent_engine.m_record_trails) {
       std::string mapName = "Agent Trails";
       int count = 1;
       while(std::find_if(std::begin(m_dataMaps), std::end(m_dataMaps),
                          [&] (ShapeMap const& m) {return m.getName() == mapName; }) != m_dataMaps.end()) {
           mapName = "Agent Trails " + std::to_string(count);
           count++;
       }
       m_dataMaps.emplace_back(mapName);
       m_agent_engine.insertTrailsInMap(m_dataMaps.back());

       m_state |= DATAMAPS;
   }

   if (m_agent_engine.m_gatelayer != -1) {
      // switch column counts from vga layer to gates layer...
      int colcounts = table.getColumnIndex(g_col_gate_counts);
      AttributeTable& tableout = m_dataMaps[m_agent_engine.m_gatelayer].getAttributeTable();
      int targetcol = tableout.insertOrResetColumn("Agent Counts");
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
// TODO: Undocumented functionality
bool MetaGraph::analyseThruVision(Communicator *comm, int gatelayer)
{
   bool analysisCompleted = false;

   AttributeTable& table = getDisplayedPointMap().getAttributeTable();

   // always have temporary gate counting layers -- makes it easier to code
   int colgates = table.insertOrResetColumn(g_col_gate);
   int colcounts = table.insertOrResetColumn(g_col_gate_counts);

   if (gatelayer != -1) {
      // switch the reference numbers from the gates layer to the vga layer
      pushValuesToLayer(VIEWDATA,gatelayer,
                        VIEWVGA,getDisplayedPointMapRef(),
                        -1,colgates,PUSH_FUNC_TOT);
   }

   try {
       analysisCompleted = VGAThroughVision().run(comm, getDisplayedPointMap(), false);
   }
   catch (Communicator::CancelledException) {
      analysisCompleted = false;
   }

   // note after the analysis, the column order might have changed... retrieve:
   colgates = table.getColumnIndex(g_col_gate);
   colcounts = table.getColumnIndex(g_col_gate_counts);

   if (analysisCompleted && gatelayer != -1) {
      AttributeTable& tableout = m_dataMaps[gatelayer].getAttributeTable();
      int targetcol = tableout.insertOrResetColumn("Thru Vision Counts");
      pushValuesToLayer(VIEWVGA,getDisplayedPointMapRef(),
                        VIEWDATA,gatelayer,
                        colcounts,targetcol,PUSH_FUNC_TOT);
   }

   // and always delete the temporary columns:
   table.removeColumn(colcounts);
   table.removeColumn(colgates);

   return analysisCompleted;
}

///////////////////////////////////////////////////////////////////////////////////

int MetaGraph::getDisplayedMapRef() const
{
   int ref = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      ref = getDisplayedPointMapRef();
      break;
   case VIEWAXIAL:
      ref = getDisplayedShapeGraphRef();
      break;
   case VIEWDATA:
      ref = getDisplayedDataMapRef();
      break;
   }
   return ref;
}

// I'd like to use this more often so that several classes other than data maps and shape graphs 
// can be used in the future

int MetaGraph::getDisplayedMapType()
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      return ShapeMap::POINTMAP;
   case VIEWAXIAL:
      return getDisplayedShapeGraph().getMapType();
   case VIEWDATA:
      return getDisplayedDataMap().getMapType();
   }
   return ShapeMap::EMPTYMAP;
}

AttributeTable& MetaGraph::getDisplayedMapAttributes()
{
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      return getDisplayedPointMap().getAttributeTable();
   case VIEWAXIAL:
      return getDisplayedShapeGraph().getAttributeTable();
   case VIEWDATA:
      return getDisplayedDataMap().getAttributeTable();
   }
   throw depthmapX::RuntimeException("No map displayed to get attribute table from");
}

bool MetaGraph::hasVisibleDrawingLayers() {
    if(!m_drawingFiles.empty()) {
        for (const auto& pixelGroup: m_drawingFiles) {
           for (const auto& pixel: pixelGroup.m_spacePixels) {
              if (pixel.isShown()) return true;
           }
        }
    }
    return false;
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
         int type = getDisplayedShapeGraph().getMapType();
         if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
            editable = getDisplayedShapeGraph().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
         }
         else {
            editable = NOT_EDITABLE;
         }
      }
      break;
   case VIEWDATA:
      editable = getDisplayedDataMap().isEditable() ? EDITABLE_ON : EDITABLE_OFF;
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
      canundo = getDisplayedShapeGraph().canUndo();
      break;
   case VIEWDATA:
      canundo = getDisplayedDataMap().canUndo();
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
      getDisplayedShapeGraph().undo();
      break;
   case VIEWDATA:
      getDisplayedDataMap().undo();
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
      col = getDisplayedShapeGraph().addAttribute(name);
      break;
   case VIEWDATA:
      col = getDisplayedDataMap().addAttribute(name);
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
      getDisplayedShapeGraph().removeAttribute(col);
      break;
   case VIEWDATA:
      getDisplayedDataMap().removeAttribute(col);
      break;
   }
}

bool MetaGraph::isAttributeLocked(int col)
{
   return getAttributeTable(m_view_class).getColumn(col).isLocked();
}

int MetaGraph::getDisplayedAttribute() const
{
   int col = -1;
   switch (m_view_class & VIEWFRONT) {
   case VIEWVGA:
      col = getDisplayedPointMap().getDisplayedAttribute();
      break;
   case VIEWAXIAL:
      col = getDisplayedShapeGraph().getDisplayedAttribute();
      break;
   case VIEWDATA:
      col = getDisplayedDataMap().getDisplayedAttribute();
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
      getDisplayedShapeGraph().overrideDisplayedAttribute(-2);
      getDisplayedShapeGraph().setDisplayedAttribute(col);
      break;
   case VIEWDATA:
      getDisplayedDataMap().overrideDisplayedAttribute(-2);
      getDisplayedDataMap().setDisplayedAttribute(col);
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
      tab = (layer == -1) ? &(getDisplayedPointMap().getAttributeTable()) : &(m_pointMaps[layer].getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = (layer == -1) ? &(getDisplayedShapeGraph().getAttributeTable()) : &(m_shapeGraphs[layer]->getAttributeTable());
      break;
   case VIEWDATA:
      tab = (layer == -1) ? &(getDisplayedDataMap().getAttributeTable()) : &(m_dataMaps[layer].getAttributeTable());
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
      tab = layer == -1 ? &(getDisplayedPointMap().getAttributeTable()) : &(m_pointMaps[layer].getAttributeTable());
      break;
   case VIEWAXIAL:
      tab = layer == -1 ? &(getDisplayedShapeGraph().getAttributeTable()) : &(m_shapeGraphs[layer]->getAttributeTable());
      break;
   case VIEWDATA:
      tab = layer == -1 ? &(getDisplayedDataMap().getAttributeTable()) : &(m_dataMaps[layer].getAttributeTable());
      break;
   }
   return *tab;
}

LayerManagerImpl &MetaGraph::getLayers(int type, int layer) {
    LayerManagerImpl *tab = NULL;
    if (type == -1) {
        type = m_view_class;
    }
    switch (type & VIEWFRONT) {
    case VIEWVGA:
        tab = (layer == -1) ? &(getDisplayedPointMap().getLayers()) : &(m_pointMaps[layer].getLayers());
        break;
    case VIEWAXIAL:
        tab = (layer == -1) ? &(getDisplayedShapeGraph().getLayers()) : &(m_shapeGraphs[layer]->getLayers());
        break;
    case VIEWDATA:
        tab = (layer == -1) ? &(getDisplayedDataMap().getLayers()) : &(m_dataMaps[layer].getLayers());
        break;
    }
    return *tab;
}

const LayerManagerImpl &MetaGraph::getLayers(int type, int layer) const {
    const LayerManagerImpl *tab = NULL;
    if (type == -1) {
        type = m_view_class & VIEWFRONT;
    }
    switch (type) {
    case VIEWVGA:
        tab = layer == -1 ? &(getDisplayedPointMap().getLayers()) : &(m_pointMaps[layer].getLayers());
        break;
    case VIEWAXIAL:
        tab = layer == -1 ? &(getDisplayedShapeGraph().getLayers()) : &(m_shapeGraphs[layer]->getLayers());
        break;
    case VIEWDATA:
        tab = layer == -1 ? &(getDisplayedDataMap().getLayers()) : &(m_dataMaps[layer].getLayers());
        break;
    }
    return *tab;
}

AttributeTableHandle& MetaGraph::getAttributeTableHandle(int type, int layer)
{
   AttributeTableHandle *tab = NULL;
   if (type == -1) {
      type = m_view_class;
   }
   switch (type & VIEWFRONT) {
   case VIEWVGA:
      tab = (layer == -1) ? &(getDisplayedPointMap().getAttributeTableHandle()) : &(m_pointMaps[layer].getAttributeTableHandle());
      break;
   case VIEWAXIAL:
      tab = (layer == -1) ? &(getDisplayedShapeGraph().getAttributeTableHandle()) : &(m_shapeGraphs[layer]->getAttributeTableHandle());
      break;
   case VIEWDATA:
      tab = (layer == -1) ? &(getDisplayedDataMap().getAttributeTableHandle()) : &(m_dataMaps[layer].getAttributeTableHandle());
      break;
   }
   return *tab;
}

const AttributeTableHandle& MetaGraph::getAttributeTableHandle(int type, int layer) const
{
   const AttributeTableHandle *tab = NULL;
   if (type == -1) {
      type = m_view_class & VIEWFRONT;
   }
   switch (type) {
   case VIEWVGA:
      tab = layer == -1 ? &(getDisplayedPointMap().getAttributeTableHandle()) : &(m_pointMaps[layer].getAttributeTableHandle());
      break;
   case VIEWAXIAL:
      tab = layer == -1 ? &(getDisplayedShapeGraph().getAttributeTableHandle()) : &(m_shapeGraphs[layer]->getAttributeTableHandle());
      break;
   case VIEWDATA:
      tab = layer == -1 ? &(getDisplayedDataMap().getAttributeTableHandle()) : &(m_dataMaps[layer].getAttributeTableHandle());
      break;
   }
   return *tab;
}

int MetaGraph::readFromFile( const std::string& filename )
{

    if (filename.empty()) {
       return NOT_A_GRAPH;
    }

 #ifdef _WIN32
    std::ifstream stream( filename.c_str(), std::ios::binary | std::ios::in );
 #else
    std::ifstream stream( filename.c_str(), std::ios::in );
 #endif
    int result = readFromStream(stream, filename);
    stream.close();
    return result;
}

int MetaGraph::readFromStream( std::istream &stream, const std::string& filename )
{
   m_state = 0;   // <- clear the state out

   // clear BSP tree if it exists:
   if (m_bsp_root) {
      delete m_bsp_root;
      m_bsp_root = NULL;
   }
   m_bsp_tree = false;

   char header[3];
   stream.read( header, 3 );
   if (stream.fail() || header[0] != 'g' || header[1] != 'r' || header[2] != 'f') {
      return NOT_A_GRAPH;
   }
   int version;
   stream.read( (char *) &version, sizeof( version ) );
   m_file_version = version;  // <- recorded for easy debugging
   if (version > METAGRAPH_VERSION) {
      return NEWER_VERSION;
   }
   if (version < METAGRAPH_VERSION) {
       std::unique_ptr<mgraph440::MetaGraph> mgraph(new mgraph440::MetaGraph);
       auto result = mgraph->read(filename);
       if ( result != mgraph440::MetaGraph::OK)
       {
           return DAMAGED_FILE;
       }
       std::stringstream tempstream;
       mgraph->writeToStream(tempstream, METAGRAPH_VERSION, 0);

       return readFromStream(tempstream, filename);
   }

   // have to use temporary state here as redraw attempt may come too early:
   int temp_state = 0;
   int temp_view_class = 0;
   stream.read( (char *) &temp_state, sizeof( temp_state ) );
   stream.read( (char *) &temp_view_class, sizeof(temp_view_class) );
   stream.read( (char *) &m_showgrid, sizeof(m_showgrid) );
   stream.read( (char *) &m_showtext, sizeof(m_showtext) );

   // type codes: x --- properties
   //             v --- virtual graph (from versions below 70)
   //             n --- ngraph format
   //             l --- layer data
   //             p --- point data
   //             d --- data summary layers

   char type;
   stream.read( &type, 1 );
   if (type == 'd') {
       // contains deprecated datalayers. Read through mgraph440 which will
       // convert them into shapemaps
       std::unique_ptr<mgraph440::MetaGraph> mgraph(new mgraph440::MetaGraph);
       auto result = mgraph->read(filename);
       if ( result != mgraph440::MetaGraph::OK)
       {
           return DAMAGED_FILE;
       }
       std::stringstream tempstream;
       mgraph->writeToStream(tempstream, METAGRAPH_VERSION, 0);

       return readFromStream(tempstream, filename);
   }
   if (type == 'x') {
      FileProperties::read(stream);
      if (stream.eof()) {
         // erk... this shouldn't happen
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
       return OK;
   }
   if (type == 'v') {

      skipVirtualMem(stream);

      // and set our filename:
      // Graph::m_nodes.setFilename( filename );

      // and tell everyone it's been archived
      // temp_state |= GRAPHARCHIVED;

      if (stream.eof()) {
         // erk... this shouldn't happen
         return DAMAGED_FILE;
      }
      else if (!stream.eof()) {
         stream.read( &type, 1 );
      }
   }
   if (type == 'l') {
      m_name = dXstring::readString(stream);
      stream.read( (char *) &m_region, sizeof(m_region) );
      int count;
      stream.read( (char *) &count, sizeof(count) );
      for (int i = 0; i < count; i++) {
          m_drawingFiles.emplace_back();
          m_drawingFiles.back().read(stream);
      }

      if (m_name.empty()) {
          m_name = "<unknown>";
      }
      temp_state |= LINEDATA;
      if (!stream.eof()) {
         stream.read( &type, 1 );
      }
      if (!stream.eof() && !stream.good()) {
         // erk... this shouldn't happen
         return DAMAGED_FILE;
      }
   }
   if (type == 'p') {
      readPointMaps( stream );
      temp_state |= POINTMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   if (type == 'g') {
      // record on state of actual point map:
      m_pointMaps.back().m_processed = true;

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
   if (type == 'x') {
      readShapeGraphs(stream);
      temp_state |= SHAPEGRAPHS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   if (type == 's') {
      readDataMaps(stream);
      temp_state |= DATAMAPS;
      if (!stream.eof()) {
         stream.read( &type, 1 );         
      }
   }
   m_state = temp_state;
   m_view_class = temp_view_class;

   return OK;
}

int MetaGraph::write( const std::string& filename, int version, bool currentlayer )
{
   std::ofstream stream;

   int oldstate = m_state;
   m_state = 0;   // <- temporarily clear out state, avoids any potential read / write errors

   char type;

   // As of MetaGraph version 70 the disk caching has been removed
   stream.open( filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc );
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
         writePointMaps( stream, true );
      }
      else if (m_view_class & MetaGraph::VIEWAXIAL) {
         type = 'x';
         stream.write(&type, 1);
         writeShapeGraphs( stream, true );
      }
      else if (m_view_class & MetaGraph::VIEWDATA) {
         type = 's';
         stream.write(&type, 1);
         writeDataMaps( stream, true );
      }
   }
   else {
      if (oldstate & LINEDATA) {
         type = 'l';
         stream.write(&type, 1);
         dXstring::writeString(stream, m_name);
         stream.write( (char *) &m_region, sizeof(m_region) );

         // Quick mod - TV
         int count = m_drawingFiles.size();
         stream.write( (char *) &count, sizeof(count) );
         for (auto& spacePixel: m_drawingFiles) {
            spacePixel.write(stream);
         }
      }
      if (oldstate & POINTMAPS) {
         type = 'p';
         stream.write(&type, 1);
         writePointMaps( stream );
      }
      if (oldstate & SHAPEGRAPHS) {
         type = 'x';
         stream.write(&type, 1);
         writeShapeGraphs( stream );
      }
      if (oldstate & DATAMAPS) {
         type = 's';
         stream.write(&type, 1);
         writeDataMaps( stream );
      }
   }

   stream.close();

   m_state = oldstate;
   return OK;
}


std::streampos MetaGraph::skipVirtualMem(std::istream& stream)
{
   // it's graph virtual memory: skip it
   int nodes = -1;
   stream.read( (char *) &nodes, sizeof(nodes) );

   nodes *= 2;

   for (int i = 0; i < nodes; i++) {
      int connections;
      stream.read( (char *) &connections, sizeof(connections) );
      stream.seekg( stream.tellg() + std::streamoff(connections * sizeof(connections)) );
   }
   return (stream.tellg());
}

std::vector<SimpleLine> MetaGraph::getVisibleDrawingLines() {

    std::vector<SimpleLine> lines;

    for (const auto& pixelGroup: m_drawingFiles) {
       for (const auto& pixel: pixelGroup.m_spacePixels) {
            if (pixel.isShown()) {
                const std::vector<SimpleLine> &newLines = pixel.getAllShapesAsLines();
                lines.insert(std::end(lines), std::begin(newLines), std::end(newLines));
            }
        }
    }
    return lines;
}

int MetaGraph::addNewPointMap(const std::string& name)
{
   std::string myname = name;
   int counter = 1;
   bool duplicate = true;
   while (duplicate) {
      duplicate = false;
      for (auto& pointMap: m_pointMaps) {
         if (pointMap.getName() == myname) {
            duplicate = true;
            myname = dXstring::formatString(counter++,name + " %d");
            break;
         }
      }
   }
   m_pointMaps.push_back(PointMap(m_region, m_drawingFiles, myname));
   m_displayed_pointmap = m_pointMaps.size() - 1;
   return m_pointMaps.size() - 1;
}

bool MetaGraph::readPointMaps(std::istream& stream)
{
   stream.read((char *) &m_displayed_pointmap, sizeof(m_displayed_pointmap));
   int count;
   stream.read((char *) &count, sizeof(count));
   for (int i = 0; i < count; i++) {
      m_pointMaps.push_back(PointMap(m_region, m_drawingFiles));
      m_pointMaps.back().read( stream );
   }
   return true;
}

bool MetaGraph::writePointMaps(std::ofstream& stream, bool displayedmaponly)
{
   if (!displayedmaponly) {
      stream.write((char *) &m_displayed_pointmap, sizeof(m_displayed_pointmap));
      int count = m_pointMaps.size();
      stream.write((char *) &count, sizeof(count));
      for (auto& pointmap: m_pointMaps) {
         pointmap.write( stream );
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
      m_pointMaps[m_displayed_pointmap].write(stream);
   }
   return true;
}

bool MetaGraph::readDataMaps(std::istream& stream )
{
    m_dataMaps.clear(); // empty existing data
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int displayed_map;
    stream.read((char *)&displayed_map,sizeof(displayed_map));
    m_displayed_datamap = size_t(displayed_map);
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int count = 0;
    stream.read((char *) &count, sizeof(count));

    for (size_t j = 0; j < size_t(count); j++) {
        m_dataMaps.emplace_back();
        m_dataMaps.back().read(stream);
    }
    return true;
}

bool MetaGraph::writeDataMaps( std::ofstream& stream, bool displayedmaponly )
{
   if (!displayedmaponly) {
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int displayed_map = (unsigned int)(m_displayed_datamap);
      stream.write((char *)&displayed_map,sizeof(displayed_map));
      // write maps
      // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
      unsigned int count = (unsigned int) m_dataMaps.size();
      stream.write((char *) &count, sizeof(count));
      for (size_t j = 0; j < count; j++) {
         m_dataMaps[j].write(stream);
      }
   }
   else {
      unsigned int dummy;
      // displayed map is 0
      dummy = 0;
      stream.write((char *)&dummy,sizeof(dummy));
      // count is 1
      dummy = 1;
      stream.write((char *)&dummy,sizeof(dummy));
      // write map:
      m_dataMaps[m_displayed_datamap].write(stream);
   }
   return true;
}


bool MetaGraph::readShapeGraphs(std::istream& stream)
{
    m_shapeGraphs.clear(); // empty existing data
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int displayed_map;
    stream.read((char *)&displayed_map,sizeof(displayed_map));
    setDisplayedShapeGraphRef(int(displayed_map));
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
    unsigned int count = 0;
    stream.read((char *) &count, sizeof(count));

    for (size_t j = 0; j < size_t(count); j++) {
        m_shapeGraphs.push_back(std::unique_ptr<ShapeGraph>(new ShapeGraph()));

        // P.K. Hairy solution given that we don't know the type/name of the shapegraph
        // before we actually read it: mark the beginning of the shapegraph in the stream
        // and if it's found to be an AllLineMap then just roll the stream back and read
        // from the mark again

        long mark = stream.tellg();
        m_shapeGraphs.back()->read(stream);
        std::string name = m_shapeGraphs.back()->getName();

        if(name == "All-Line Map" ||
                name == "All Line Map") {
            m_shapeGraphs.pop_back();
            m_shapeGraphs.push_back(std::unique_ptr<AllLineMap>(new AllLineMap()));
            stream.seekg(mark);
            m_shapeGraphs.back()->read(stream);
        }
    }

    // P.K: ideally this should be read together with the
    // all-line map, but the way the graph file is structured
    // this is not possible
    // TODO: Fix on next graph file update

    bool foundAllLineMap = false;
    for(size_t i = 0; i < m_shapeGraphs.size(); i++) {
        ShapeGraph* shapeGraph = m_shapeGraphs[i].get();
        if(shapeGraph->getName() == "All-Line Map" ||
                shapeGraph->getName() == "All Line Map") {
            foundAllLineMap = true;
            AllLineMap* alllinemap = dynamic_cast<AllLineMap *>(shapeGraph);
            if(alllinemap == nullptr) {
                throw depthmapX::RuntimeException("Failed to cast from ShapeGraph to AllLineMap");
            }
            // these are additional essentially for all line axial maps
            // should probably be kept *with* the all line axial map...
            alllinemap->m_poly_connections.clear();
            dXreadwrite::readIntoVector(stream, alllinemap->m_poly_connections);
            alllinemap->m_radial_lines.clear();
            dXreadwrite::readIntoVector(stream, alllinemap->m_radial_lines);

            // this is an index to look up the all line map, used by UI to determine if can make fewest line map
            // note: it is not saved for historical reasons
            // will get confused by more than one all line map
            m_all_line_map = int(i);

            // there is currently only one:
            break;
        }
    }
    if(!foundAllLineMap) {
        // P.K. This is just a dummy read to cover cases where there is no All-Line Map
        // The below is taken from pmemvec<T>::read

        // READ / WRITE USES 32-bit LENGTHS (number of elements)
        // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
        unsigned int length;
        stream.read( (char *) &length, sizeof(unsigned int) );
        stream.read( (char *) &length, sizeof(unsigned int) );
    }
    return true;
}

bool MetaGraph::writeShapeGraphs( std::ofstream& stream, bool displayedmaponly )
{
    if (!displayedmaponly) {
        // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
        unsigned int displayed_map = (unsigned int)(getDisplayedShapeGraphRef());
        stream.write((char *)&displayed_map,sizeof(displayed_map));
        // write maps
        // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion problems
        unsigned int count = (unsigned int) m_shapeGraphs.size();
        stream.write((char *) &count, sizeof(count));
        for (size_t j = 0; j < count; j++) {
            m_shapeGraphs[j]->write(stream);
        }
    }
    else {
        unsigned int dummy;
        // displayed map is 0
        dummy = 0;
        stream.write((char *)&dummy,sizeof(dummy));
        // count is 1
        dummy = 1;
        stream.write((char *)&dummy,sizeof(dummy));
        // write map:
        m_shapeGraphs[getDisplayedShapeGraphRef()]->write(stream);
    }

    if(m_all_line_map == -1) {
        std::vector<PolyConnector> temp_poly_connections;
        std::vector<RadialLine> temp_radial_lines;

        dXreadwrite::writeVector(stream, temp_poly_connections);
        dXreadwrite::writeVector(stream, temp_radial_lines);
    } else {
        AllLineMap* alllinemap = dynamic_cast<AllLineMap *>(m_shapeGraphs[size_t(m_all_line_map)].get());

        if(alllinemap == nullptr) {
            throw depthmapX::RuntimeException("Failed to cast from ShapeGraph to AllLineMap");
        }

        dXreadwrite::writeVector(stream, alllinemap->m_poly_connections);
        dXreadwrite::writeVector(stream, alllinemap->m_radial_lines);
    }
    return true;
}

void MetaGraph::makeViewportShapes( const QtRegion& viewport ) const
{
   m_current_layer = -1;
   size_t i = m_drawingFiles.size() - 1;
   for (auto iter = m_drawingFiles.rbegin(); iter != m_drawingFiles.rend(); iter++) {
      if (iter->isShown()) {
         m_current_layer = (int) i;
         iter->makeViewportShapes( (viewport.atZero() ? m_region : viewport) );
      }
      i--;
   }
}

bool MetaGraph::findNextShape(bool& nextlayer) const
{
   if (m_current_layer == -1)
      return false;
   while (!m_drawingFiles[m_current_layer].findNextShape(nextlayer)) {
      while (++m_current_layer < (int)m_drawingFiles.size() && !m_drawingFiles[m_current_layer].isShown());
      if (m_current_layer == static_cast<int>(m_drawingFiles.size())) {
         m_current_layer = -1;
         return false;
      }
   }
   return true;
}
