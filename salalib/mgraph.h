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


#ifndef __MGRAPH_H__
#define __MGRAPH_H__

// Interface: the meta graph loads and holds all sorts of arbitrary data...

#include "mgraph_consts.h"

#include <genlib/paftl.h>
#include <genlib/p2dpoly.h>

#include <salalib/vertex.h>      // mainly deprecated, but includes display params
#include <salalib/fileproperties.h>
#include <salalib/spacepix.h>
#include <salalib/attributes.h>

#include <salalib/pointdata.h>
#include <salalib/connector.h>
#include <salalib/shapemap.h>
#include <salalib/axialmap.h>
#include <salalib/datalayer.h>   // datalayers deprecated

#include <mutex>

// for agent engine interface
#include <salalib/nagent.h>
#include <salalib/importtypedefs.h>

#include <vector>
#include <memory>

///////////////////////////////////////////////////////////////////////////////////

class Communicator;

// A meta graph is precisely what it says it is

class MetaGraph : public SuperSpacePixel, public PointMaps, public FileProperties
{
public:
   enum { ADD = 0x0001, REPLACE = 0x0002, CAT = 0x0010, DXF = 0x0020, NTF = 0x0040, RT1 = 0x0080, GML = 0x0100 };
   enum { NONE = 0x0000, /* GRAPH = 0x0001, Deprecated */ POINTMAPS = 0x0002, LINEDATA = 0x0004, 
          FIXED = 0x0008 /* Deprecated */, ANGULARGRAPH = 0x0010, DATAMAPS = 0x0020, AXIALLINES = 0x0040, /* BOUNDARYGRAPH = 0x0080, Deprecated */ SHAPEGRAPHS = 0x0100,
          PREWRITTEN = 0x1000, BUGGY = 0x8000 };
   enum { NOT_EDITABLE = 0, EDITABLE_OFF = 1, EDITABLE_ON = 2 };
protected:
   int m_file_version;
   int m_state;
public:
   // some display options now set at file level
   bool m_showgrid;
   bool m_showtext;
   //
public:
   ShapeMaps<ShapeMap> m_data_maps;
   ShapeGraphs m_shape_graphs;
public:
   MetaGraph();
   ~MetaGraph();
   //
   int getVersion()
   {
      // note, if unsaved, m_file_version is -1
      return m_file_version;
   }



private:
   std::recursive_mutex mLock;

public:
    std::unique_lock<std::recursive_mutex> getLock(){
       return std::unique_lock<recursive_mutex>(mLock);
   }

    std::unique_lock<std::recursive_mutex> getLockDeferred(){
        return std::unique_lock<std::recursive_mutex>(mLock, std::defer_lock_t());
    }

   //
   void copyLineData(const SuperSpacePixel& meta);
   void copyPointMap(const PointMap& meta);
   //
   int getState() const
      { return m_state; }
   // use with caution: only very rarely needed outside MetaGraph itself
   void setState(int state)
      { m_state = state; }
   //
   // quick loaders from input streams rather than files:
   bool importCat( istream& stream );
   // make a graph using the supplied seed and graph spacing:
   void fastGraph( const Point2f& seed, double spacing );
   //
   int loadLineData( Communicator *communicator, int load_type );
   int loadCat( istream& stream, Communicator *communicator );
   int loadRT1(const std::vector<string>& fileset, Communicator *communicator);
   ShapeMap &createNewShapeMap(depthmapX::ImportType mapType, std::string name);
   void deleteShapeMap(depthmapX::ImportType mapType, ShapeMap &shapeMap);
   void updateParentRegions(ShapeMap &shapeMap);
   int importLinesAsShapeMap(const std::vector<Line> &lines, QtRegion region, std::string name, depthmapX::Table &data );
   int importPointsAsShapeMap(const std::vector<Point2f> &points, QtRegion region, std::string name, depthmapX::Table &data);
   bool undoPoints();
   bool clearPoints();
   bool setGrid( double spacing, const Point2f& offset = Point2f() );                 // override of PointMap
   bool makePoints( const Point2f& p, int semifilled, Communicator *communicator = NULL);  // override of PointMap
   bool makeGraph( Communicator *communicator, int algorithm, double maxdist );
   bool analyseGraph(Communicator *communicator, Options options , bool simple_version); // <- options copied to keep thread safe
   bool analyseAngular( Communicator *communicator, bool analyse_in_memory );
   bool makeAxialLines( Communicator *communicator, bool analyse_in_memory );
   //
   // helpers for editing maps
   bool isEditableMap();
   ShapeMap& getEditableMap();
   // currently only making / moving lines, but should be able to extend this to polys fairly easily:
   bool makeShape(const Line& line);
   bool moveSelShape(const Line& line);
   // onto polys as well:
   bool polyBegin(const Line& line);
   bool polyAppend(const Point2f& point);
   bool polyClose();
   bool polyCancel();
   //
   int addShapeGraph(const std::string& name, int type);
   int addShapeMap(const std::string& name);
   void removeDisplayedMap();
   //
   // various map conversions
   bool convertDrawingToAxial(Communicator *comm, std::string layer_name);  // n.b., name copied for thread safety
   bool convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues);
   bool convertDrawingToSegment(Communicator *comm, std::string layer_name);
   bool convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues);
   bool convertToData(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag);  // -1 signifies convert from drawing layer, else convert from data map
   bool convertToDrawing(Communicator *comm, std::string layer_name, int typeflag); // 0 signifies convert from data map, else convert from graph
   bool convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal, int typeflag); // -1 signifies convert from drawing layer, else convert from data map
   bool convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal, bool pushvalues, double stubremoval);
   // note: not same categories
   bool convertPointsToShape();
   //bool convertBoundaryGraph( Communicator *communicator );
   //
   // some compatibility with older version horrors:
   int convertDataLayersToShapeMap(DataLayers& datalayers, PointMap& pointmap);
   void convertShapeGraphToShapeMap(const ShapeGraph& axialmap);
   //
   int loadMifMap(Communicator *comm, istream& miffile, istream& midfile);
   bool makeAllLineMap( Communicator *communicator, const Point2f& seed );
   bool makeFewestLineMap( Communicator *communicator, int replace );
   bool analyseAxial( Communicator *communicator, Options options, bool simple_version ); // <- options copied to keep thread safe
   bool analyseSegments( Communicator *communicator, Options options ); // <- options copied to keep thread safe
   bool analyseTopoMet( Communicator *communicator, Options options ); // <- options copied to keep thread safe
   //
   bool hasAllLineMap()
   { return m_shape_graphs.hasAllLineMap(); }
   //
   enum { PUSH_FUNC_MAX = 0, PUSH_FUNC_MIN = 1, PUSH_FUNC_AVG = 2, PUSH_FUNC_TOT = 3};
   bool pushValuesToLayer(int desttype, int destlayer, int push_func, bool count_col = false);
   bool pushValuesToLayer(int sourcetype, int sourcelayer, int desttype, int destlayer, int col_in, int col_out, int push_func, bool count_col = false);
   //
   int getDisplayedMapRef() const;
   //
   // NB -- returns 0 (not editable), 1 (editable off) or 2 (editable on)
   int isEditable() const;
   bool canUndo() const;
   void undo();
   //
   ShapeGraph& getDisplayedShapeGraph()
   { return m_shape_graphs.getDisplayedMap(); }
   ShapeMap& getDisplayedDataMap()
   { return m_data_maps.getDisplayedMap(); }
   ShapeGraphs& getShapeGraphs()
   { return m_shape_graphs; }
   ShapeMaps<ShapeMap>& getDataMaps()
   { return m_data_maps; }
   //
   int getDisplayedMapType();
   //
   QtRegion getBoundingBox() const;
   //
   int getDisplayedAttribute() const;
   void setDisplayedAttribute(int col);
   int addAttribute(const std::string& name);
   void removeAttribute(int col);
   bool isAttributeLocked(int col);
   AttributeTable& getAttributeTable(int type = -1, int layer = -1);
   const AttributeTable& getAttributeTable(int type = -1, int layer = -1) const;
   //
   void loadGraphAgent();
   void unloadGraphAgent();
   //
   int getLineFileCount() const
      { return (int) SuperSpacePixel::size(); }

   // Quick mod - TV
   const std::string& getLineFileName(int file) const
      { return SuperSpacePixel::at(file).getName(); }
   int getLineLayerCount(int file) const
      { return (int) SuperSpacePixel::at(file).size(); }

   //
/*   SpacePixel& getLineLayer(int file, int layer)
      { return SuperSpacePixel::at(file).at(layer); }
   const SpacePixel& getLineLayer(int file, int layer) const
      { return SuperSpacePixel::at(file).at(layer); }*/
   ShapeMap& getLineLayer(int file, int layer)
      { return SuperSpacePixel::at(file).at(layer); }
   const ShapeMap& getLineLayer(int file, int layer) const
      { return SuperSpacePixel::at(file).at(layer); }
   //
   // Some error handling -- the idea is that you catch the error in MetaGraph,
   // return a generic error code and then get your front end to interrogate the 
   // last error (pretty much as per standard Unix etc).
   // May have problems with multithreading.
public:
   class Error 
   {
   protected:
      std::string error;
   public:
      Error(const std::string& err = std::string()) { error = err; }
   };
protected:
   Error m_last_error;
public:
   Error& getLastError()
   { return m_last_error; }
   // for drawing either axial analysis or VGA
protected:
   int m_view_class;
public:
   enum { SHOWHIDEVGA = 0x0100, SHOWVGATOP = 0x0200, SHOWHIDEAXIAL = 0x0400, SHOWAXIALTOP = 0x0800, SHOWHIDESHAPE = 0x1000, SHOWSHAPETOP = 0x2000 };
   enum { VIEWNONE = 0x00, VIEWVGA = 0x01, VIEWBACKVGA = 0x02, VIEWAXIAL = 0x04, VIEWBACKAXIAL = 0x08,
          VIEWLINKS = 0x10, VIEWDATA = 0x20, VIEWBACKDATA = 0x40, VIEWFRONT = 0x25, VIEWBACK = 0x4a };
   //
   int getViewClass()
   { return m_view_class; }
   // These functions make specifying conditions to do things much easier:
   bool viewingNone()
   { return (m_view_class == VIEWNONE); }
   bool viewingProcessed()
   { return ((m_view_class & (VIEWAXIAL | VIEWDATA)) || (m_view_class & VIEWVGA && getDisplayedPointMap().isProcessed())); }
   bool viewingShapes()
   { return (m_view_class & (VIEWAXIAL | VIEWDATA)) != 0; }
   bool viewingProcessedLines()
   { return ((m_view_class & VIEWAXIAL) == VIEWAXIAL); } 
   bool viewingProcessedShapes()
   { return ((m_view_class & VIEWDATA) == VIEWDATA); } 
   bool viewingProcessedPoints()
   { return ((m_view_class & VIEWVGA) && getDisplayedPointMap().isProcessed()); }
   bool viewingUnprocessedPoints()
   { return ((m_view_class & VIEWVGA) && !getDisplayedPointMap().isProcessed()); }
   //
   bool setViewClass(int command);
   //
   double getLocationValue(const Point2f& point);
   //
public:
   // these are dependent on what the view class is:
   bool isSelected()                              // does a selection exist
   {  if (m_view_class & VIEWVGA) 
         return getDisplayedPointMap().isSelected();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().isSelected();
      else if (m_view_class & VIEWDATA) 
         return m_data_maps.getDisplayedMap().isSelected();
      else 
         return false;
   }
   bool setCurSel( QtRegion& r, bool add = false )  // set current selection
   {  if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().setCurSel(r, add);
      else if (m_view_class & VIEWDATA)
         return m_data_maps.getDisplayedMap().setCurSel( r, add );
      else if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().setCurSel( r, add );
      else if (m_state & POINTMAPS && !getDisplayedPointMap().isProcessed()) // this is a default select application
         return getDisplayedPointMap().setCurSel( r, add );
      else if (m_state & DATAMAPS) // I'm not sure why this is a possibility, but it appears you might have state & DATAMAPS without VIEWDATA...
         return m_data_maps.getDisplayedMap().setCurSel( r, add );
      else 
         return false;
   }
   bool clearSel()
   {
      // really needs a separate clearSel for the datalayers... at the moment this is handled in PointMap
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().clearSel();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().clearSel();
      else if (m_view_class & VIEWDATA) 
         return m_data_maps.getDisplayedMap().clearSel();
      else
         return false;
   }
   int getSelCount()
   {
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().getSelCount();
      else if (m_view_class & VIEWAXIAL) 
         return (int) m_shape_graphs.getDisplayedMap().getSelCount();
      else if (m_view_class & VIEWDATA) 
         return (int) m_data_maps.getDisplayedMap().getSelCount();
      else
         return 0;
   }
   float getSelAvg()
   {
      if (m_view_class & VIEWVGA)
         return (float)getDisplayedPointMap().getAttributeTable().getSelAvg();
      else if (m_view_class & VIEWAXIAL) 
         return (float)m_shape_graphs.getDisplayedMap().getAttributeTable().getSelAvg();
      else if (m_view_class & VIEWDATA) 
         return (float)m_data_maps.getDisplayedMap().getAttributeTable().getSelAvg();
      else
         return -1.0f;
   }
   QtRegion getSelBounds()
   {
      if (m_view_class & VIEWVGA)
         return getDisplayedPointMap().getSelBounds();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelBounds();
      else if (m_view_class & VIEWDATA) 
         return m_data_maps.getDisplayedMap().getSelBounds();
      else
         return QtRegion();
   }
   // setSelSet expects a set of ref ids:
   void setSelSet(const std::vector<int>& selset, bool add = false)
   { if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         getDisplayedPointMap().setCurSel(selset,add);
      else if (m_view_class & VIEWAXIAL) 
         m_shape_graphs.getDisplayedMap().setCurSel(selset,add);
      else // if (m_view_class & VIEWDATA) 
         m_data_maps.getDisplayedMap().setCurSel(selset,add); }
   std::set<int>& getSelSet()
   {  if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         return getDisplayedPointMap().getSelSet();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelSet();
      else // if (m_view_class & VIEWDATA) 
         return m_data_maps.getDisplayedMap().getSelSet(); }
   const std::set<int>& getSelSet() const
   {  if (m_view_class & VIEWVGA && m_state & POINTMAPS)
         return getDisplayedPointMap().getSelSet();
      else if (m_view_class & VIEWAXIAL) 
         return m_shape_graphs.getDisplayedMap().getSelSet();
      else // if (m_view_class & VIEWDATA) 
         return m_data_maps.getDisplayedMap().getSelSet(); }
//
public:
   // no longer supported
   //int addLineDynamic(const Line& l);
   //bool removeLineDynamic(LineKey linekey);
   //
   // Agent engine interface:
protected:
   AgentEngine m_agent_engine;
public:
   AgentEngine& getAgentEngine()
   { return m_agent_engine; }
   void runAgentEngine(Communicator *comm);
   //
public:
   // thru vision
   bool analyseThruVision(Communicator *comm = NULL, int gatelayer = -1);
   // BSP tree for making isovists
protected:
   BSPNode *m_bsp_root;
   bool m_bsp_tree;
public:
   bool makeBSPtree(Communicator *communicator = NULL);
   void resetBSPtree() { m_bsp_tree = false; }
   // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
   int makeIsovist(Communicator *communicator, const Point2f& p, double startangle = 0, double endangle = 0, bool simple_version = true);
   // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
   int makeIsovistPath(Communicator *communicator, double fov_angle = 2.0 * M_PI, bool simple_version = true);
   bool makeIsovist(const Point2f& p, Isovist& iso);
protected:
   // properties
public:
   // a few read-write returns:
   enum { OK, WARN_BUGGY_VERSION, WARN_CONVERTED, NOT_A_GRAPH, DAMAGED_FILE, DISK_ERROR, NEWER_VERSION, DEPRECATED_VERSION };
   // likely to use communicator if too slow...
   int readFromFile( const std::string& filename );
   int readFromStream( istream &stream, const std::string& filename );
   int write( const std::string& filename, int version, bool currentlayer = false);
   //
   std::vector<SimpleLine> getVisibleDrawingLines();
protected:
   pqvector<AttrBody> *m_attr_conv_table;
   int convertAttributes( ifstream& stream, int version );
   int convertVirtualMem( ifstream& stream, int version );
   //
   streampos skipVirtualMem(istream &stream, int version);
   streampos copyVirtualMem(istream& reader, ofstream& writer, int version);
};

///////////////////////////////////////////////////////////////////////////////

#endif
