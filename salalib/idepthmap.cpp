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

// this is the Depthmap interface
// definition

// Quick mod - TV
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <time.h>

#include <genlib/paftl.h>
#include <salalib/mgraph.h>
#include <salalib/ngraph.h>

#include <salalib/idepthmap.h>
#include <salalib/idepthmapx.h>

//////////////////////////////////////////////////////////////////////////////////////

// setData takes a "MetaGraph *" as the parameter, but:
// m_data for an IGraphFile is an "IGraphOrganizer *" (local)

IGraphFile::IGraphFile()
{
   m_data = (void *) new IGraphOrganizer;
}

IGraphFile::~IGraphFile()
{
   if (m_data != NULL) {
      delete ((IGraphOrganizer *)m_data);
      m_data = NULL;
   }
}

void IGraphFile::setData(void *data)
{
   if (m_data != NULL) {
      ((IGraphOrganizer *)m_data)->m_graph = (MetaGraph *) data;
   }
}

IGraphFile *newGraphFile()
{
   IGraphFile *graphfile = NULL;
   MetaGraph *graph = new MetaGraph();
   // REMINDER: fill in some more details here:
    std::string version = "Sala.dll version";
	char tmpbuf[9];
    // Quick mod - TV
#if defined(_WIN32)
    std::string date = _strdate(tmpbuf);
#else
    char nowDate[20];
    {
        int datelen = 0;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        datelen = strftime(nowDate, 20, "%m/%d/%y", localtime(&tv.tv_sec));
    }
    std::string date = nowDate;
#endif

   graph->setProperties("Name","Organisation",date,version);
	graphfile = new IGraphFile();
	graphfile->setData(graph);
   // ensure the graph is labelled for deletion after use
   ((IGraphOrganizer *)graphfile->m_data)->setDeleteFlag();
   return graphfile;
}

IGraphFile *openGraphFile(const char *filename)
{
   if (filename == NULL) {
      return NULL;
   }
   IGraphFile *graphfile = NULL;
   MetaGraph *graph = new MetaGraph();
   if (graph->read( filename ) == MetaGraph::OK) {
	   graphfile = new IGraphFile();
		graphfile->setData(graph);
      // ensure the graph is labelled for deletion after use
      ((IGraphOrganizer *)graphfile->m_data)->setDeleteFlag();
	}
	else {
	   delete graph;
	}
   return graphfile;
}

void closeGraphFile(IGraphFile *& file)
{
   if (file != NULL) {
      delete file;
   }
   file = NULL;
}

/////////////////////////////////////////////////////////////////////

bool IGraphFile::save(const char *filename)
{
   if (filename == NULL) {
      return false;
   }

   MetaGraph *graph = ((IGraphOrganizer *)m_data)->m_graph;

   // there's a little issue with the state variable which needs to be written
   // ...this is an explicit set before save according to what's in the file:
   int state = graph->getState();
   if (graph->PointMaps::maps_vector.size() > 0) {
      state |= MetaGraph::POINTMAPS;
   }
   if (graph->m_data_maps.getMapCount() > 0) {
      state |= MetaGraph::DATAMAPS;
   }
   if (graph->m_shape_graphs.getMapCount() > 0) {
      state |= MetaGraph::SHAPEGRAPHS;
   }
   graph->setState(state);

   return (graph->write(filename, METAGRAPH_VERSION) == MetaGraph::OK);
}

int IGraphFile::getVGAMapCount()
{
   return ((IGraphOrganizer *)m_data)->getIVGAMapCount();
}

IVGAMap *IGraphFile::getFirstVGAMap()
{
   return ((IGraphOrganizer *)m_data)->getFirstIVGAMap();
}

IVGAMap *IGraphFile::getNextVGAMap()
{
   return ((IGraphOrganizer *)m_data)->getNextIVGAMap();
}

// Shape layers: used for segment and axial maps

int IGraphFile::getShapeMapCount()
{
   return ((IGraphOrganizer *)m_data)->getIShapeMapCount();
}

IShapeMap *IGraphFile::getFirstShapeMap()
{
   return ((IGraphOrganizer *)m_data)->getFirstIShapeMap();
}

IShapeMap *IGraphFile::getNextShapeMap()
{
   return ((IGraphOrganizer *)m_data)->getNextIShapeMap();
}

IShapeMap *IGraphFile::insertShapeMap(const char *name, int type)
{
   IShapeMap *map = NULL;

   switch (type) {
      // can't handle drawing maps yet
      case IShapeMap::IDRAWINGMAP:
         map = NULL;
         break;
      // should be able to handle these: else
      case IShapeMap::IDATAMAP:
      case IShapeMap::ICONVEXMAP:
      case IShapeMap::IAXIALMAP:
      case IShapeMap::ISEGMENTMAP:
      case IShapeMap::ISHAPEMAP:
         map = ((IGraphOrganizer *)m_data)->addShapeMap(name, type);
         break;
      default:
         map = NULL;
         break;
   }

   return map;
}

// retrieve the attribute table for either vga or shape layer interfaces:
IAttributes *IGraphFile::getAttributes(IVGAMap *ivga)
{
   return ((IGraphOrganizer *)m_data)->getIAttributes(ivga);
}

IAttributes *IGraphFile::getAttributes(IShapeMap *ishape)
{
   return ((IGraphOrganizer *)m_data)->getIAttributes(ishape);
}

int IGraphFile::getViewOrder(IVGAMap *ivga)
{
   return ((IGraphOrganizer *)m_data)->getViewOrder(ivga);
}

int IGraphFile::getViewOrder(IShapeMap *ishape)
{
   return ((IGraphOrganizer *)m_data)->getViewOrder(ishape);
}

/////////////////////////////////////////////////////////////////////////////////////

// This is a bit of a mess, as it's not been programmed to work like this from the start...

bool IGraphFile::pushValuesToMap(IShapeMap *source, IShapeMap *dest, const char *source_attribute, const char *dest_attribute, int push_type)
{
   int source_type = 0, dest_type = 0;
   if (source->isDataMap()) {
      source_type = MetaGraph::VIEWDATA;
   }
   else {
      source_type = MetaGraph::VIEWAXIAL;
   }
   if (dest->isDataMap()) {
      dest_type = MetaGraph::VIEWDATA;
   }
   else {
      dest_type = MetaGraph::VIEWAXIAL;
   }

   IGraphOrganizer *data = ((IGraphOrganizer *)m_data);

   int source_layer = data->getMapRef(source);
   int dest_layer = data->getMapRef(dest);

   if (source_layer == -1 || dest_layer == -1) {
      return false;
   }

   AttributeTable *source_table = data->getAttributeTable(source);
   AttributeTable *dest_table = data->getAttributeTable(dest);
   if (source_table == NULL || dest_table == NULL) {
      return false;
   }
   int source_col;
   if (strcmp(source_attribute,"Ref Number") == 0) {
      source_col = -1;
   }
   else {
      source_col = source_table->getColumnIndex(source_attribute);
      if (source_col == -1) {
         return false;
      }
   }
   int dest_col = -2;
   if (dest_attribute != NULL) {
      if (strcmp(dest_attribute,"Ref Number") == 0) {
         // cannot overwrite
         return false;
      }
      else {
         dest_col = dest_table->getColumnIndex(dest_attribute);
         if (dest_col == -1) {
            dest_col = dest_table->insertColumn(dest_attribute);
         }
         else if (dest_table->isColumnLocked(dest_col)) {
            // cannot overwrite
            return false;
         }
      }
   }

   // note that for push_type, IPUSH_MAX should match PUSH_FUNC_MAX, etc
   // -2 singles automatic naming of push column
   data->m_graph->pushValuesToLayer(source_type,source_layer,dest_type,dest_layer,source_col,dest_col,push_type,false);

   return true;
}

bool IGraphFile::pushValuesToMap(IVGAMap *source, IShapeMap *dest, const char *source_attribute, const char *dest_attribute, int push_type)
{
   int source_type = MetaGraph::VIEWVGA;
   int dest_type;
   if (dest->isDataMap()) {
      dest_type = MetaGraph::VIEWDATA;
   }
   else {
      dest_type = MetaGraph::VIEWAXIAL;
   }

   IGraphOrganizer *data = ((IGraphOrganizer *)m_data);

   int source_layer = data->getMapRef(source);
   int dest_layer = data->getMapRef(dest);

   if (source_layer == -1 || dest_layer == -1) {
      return false;
   }

   AttributeTable *source_table = data->getAttributeTable(source);
   AttributeTable *dest_table = data->getAttributeTable(dest);
   if (source_table == NULL || dest_table == NULL) {
      return false;
   }
   int source_col;
   if (strcmp(source_attribute,"Ref Number") == 0) {
      source_col = -1;
   }
   else {
      source_col = source_table->getColumnIndex(source_attribute);
      if (source_col == -1) {
         return false;
      }
   }
   int dest_col = -2;
   if (dest_attribute != NULL) {
      if (strcmp(dest_attribute,"Ref Number") == 0) {
         // cannot overwrite
         return false;
      }
      else {
         dest_col = dest_table->getColumnIndex(dest_attribute);
         if (dest_col == -1) {
            dest_col = dest_table->insertColumn(dest_attribute);
         }
         else if (dest_table->isColumnLocked(dest_col)) {
            // cannot overwrite
            return false;
         }
      }
   }

   // note that for push_type, IPUSH_MAX should match PUSH_FUNC_MAX, etc
   // -2 singles automatic naming of push column
   data->m_graph->pushValuesToLayer(source_type,source_layer,dest_type,dest_layer,source_col,dest_col,push_type,false);

   return true;
}

bool IGraphFile::pushValuesToMap(IShapeMap *source, IVGAMap *dest, const char *source_attribute,  const char *dest_attribute, int push_type)
{
   int source_type;
   if (source->isDataMap()) {
      source_type = MetaGraph::VIEWDATA;
   }
   else {
      source_type = MetaGraph::VIEWAXIAL;
   }
   int dest_type  = MetaGraph::VIEWVGA;

   IGraphOrganizer *data = ((IGraphOrganizer *)m_data);

   int source_layer = data->getMapRef(source);
   int dest_layer = data->getMapRef(dest);

   if (source_layer == -1 || dest_layer == -1) {
      return false;
   }

   AttributeTable *source_table = data->getAttributeTable(source);
   AttributeTable *dest_table = data->getAttributeTable(dest);
   if (source_table == NULL || dest_table == NULL) {
      return false;
   }
   int source_col;
   if (strcmp(source_attribute,"Ref Number") == 0) {
      source_col = -1;
   }
   else {
      source_col = source_table->getColumnIndex(source_attribute);
      if (source_col == -1) {
         return false;
      }
   }
   int dest_col = -2;
   if (dest_attribute != NULL) {
      if (strcmp(dest_attribute,"Ref Number") == 0) {
         // cannot overwrite
         return false;
      }
      else {
         dest_col = dest_table->getColumnIndex(dest_attribute);
         if (dest_col == -1) {
            dest_col = dest_table->insertColumn(dest_attribute);
         }
         else if (dest_table->isColumnLocked(dest_col)) {
            // cannot overwrite
            return false;
         }
      }
   }

   // note that for push_type, IPUSH_MAX should match PUSH_FUNC_MAX, etc
   // -2 singles automatic naming of push column
   data->m_graph->pushValuesToLayer(source_type,source_layer,dest_type,dest_layer,source_col,dest_col,push_type,false);

   return true;
}

/////////////////////////////////////////////////////////////////////////////////////

IShapeMap *IGraphFile::importMap(const char *filename, const char *type, const char *newmapname) 
{
   // Note: 08-APR-2010 -- still have to add DXF and CAT (all to get automatic analysis working)

   IShapeMap *shapemap = NULL;

   MetaGraph *graph = ((IGraphOrganizer *)m_data)->m_graph;

   string type_str = string(type);

   if (!(type_str.compare("MIF") == 0 || type_str.compare("TXT") == 0 || type_str.compare("CSV") == 0)) {
      return NULL;
   }

   if (type_str.compare("TXT") == 0 || type_str.compare("CSV") == 0) 
   {
      // these can be x,y points or x1,y1,x2,y2 lines
      // first line should be labels, creates a datamap
   	ifstream file(filename);

      if (!file) {
		   return NULL;
      }
      else {
         int mapref = graph->importTxt( file, newmapname, (type_str.compare("CSV")==0) );
         
         if (mapref != -1) {
            // note, at this stage in development, you CANNOT go from the mapref directly here as the getIShapeMap has both shape graphs and data maps mixed together
            ShapeMap& basemap = graph->m_data_maps.getMap(mapref);
			   shapemap = ((IGraphOrganizer *)m_data)->getIShapeMap(&basemap);
         }
      }
   }
   else if (type_str.compare("MIF") == 0) {

      string miffilename = string(filename) + ".mif";
       string midfilename = string(filename) + ".mid";

	   ifstream miffile( miffilename.c_str() );
	   ifstream midfile( midfilename.c_str() );

	   if (miffile && midfile) {
           int mapref = graph->m_data_maps.addMap(newmapname,ShapeMap::DATAMAP);

		   ShapeMap& mifmap = graph->m_data_maps.getMap(mapref);
	
		   int ok = mifmap.loadMifMap(miffile,midfile);
		   if (ok == MINFO_OK || ok == MINFO_MULTIPLE) { // multiple is just a warning
            // note, at this stage in development, you CANNOT go from the mapref directly here as the getIShapeMap has both shape graphs and data maps mixed together
			   shapemap = ((IGraphOrganizer *)m_data)->getIShapeMap(&mifmap);
	      }
	      else { // error: undo!
		       graph->m_data_maps.removeMap(mapref);
	   	}
	   }
   }

	return shapemap;
}

IShapeMap *IGraphFile::makeAxialMapFromBaseMap(IComm *comm, IShapeMap *basemap, const char *newmapname)
{
   IShapeMap *retvar = NULL;

   bool created_comm = false;
   if (comm == NULL) {
	  comm = getIComm();
	  if (comm == NULL || comm->isValid()) {
		   return NULL;
	  }
	  created_comm = true;
   }

   retvar = ((IGraphOrganizer *)m_data)->makeAxialMapFromBaseMap(comm, basemap, newmapname);

   if (created_comm) {
	  delete comm;
   }

   return retvar;
}


IShapeMap *IGraphFile::makeSegmentMapFromAxialMap(IComm *comm, IShapeMap *axialmap, const char *newmapname, double stubremoval) 
{
   IShapeMap *retvar = NULL;

   bool created_comm = false;
   if (comm == NULL) {
	  comm = getIComm();
	  if (comm == NULL || comm->isValid()) {
		   return NULL;
	  }
	  created_comm = true;
   }

   retvar = ((IGraphOrganizer *)m_data)->makeSegmentMapFromAxialMap(comm, axialmap, newmapname, stubremoval);

   if (created_comm) {
	  delete comm;
   }

   return retvar;
}


/////////////////////////////////////////////////////////////////////////////////////

// m_data for a IComm should be a "Communicator *"

IComm::IComm()
{
   m_data = NULL;
}

// n.b., may be problematic if not an ICommunicator
IComm::~IComm()
{
   // check that this is a DLL interface created Communicator before deleting it
	if (m_data != NULL && ((Communicator *)m_data)->GetDeleteFlag()) {
		delete ((ICommunicator *) m_data);
		m_data = NULL;
	}
   // note: front-end supplied Communicators will be deleted by the front-end
}

void IComm::setData(void *data)
{
   m_data = data;
}

void *IComm::getData()
{
   return m_data;
}

bool IComm::isValid()
{
   return (m_data != NULL);
}

IComm *getIComm()
{
   ICommunicator *comm = new ICommunicator();
   IComm *icomm = new IComm();
   icomm->setData(comm);
   return icomm;
}

void IComm::close()
{
   // check that this is a DLL interface created Communicator before deleting it
	if (m_data != NULL && ((Communicator *)m_data)->GetDeleteFlag()) {
   	delete ((ICommunicator *) m_data);
	   m_data = NULL;
   }
   // note: front-end supplied Communicators will IGNORE this instruction
}

void IComm::cancelProcess()
{
	((ICommunicator *)m_data)->Cancel();
}

int IComm::getNumSteps()
{
	return ((ICommunicator *)m_data)->num_steps;
}

int IComm::getCurrentStep()
{
	return ((ICommunicator *)m_data)->step;
}

int IComm::getNumRecords()
{
	return ((ICommunicator *)m_data)->num_records;
}

int IComm::getCurrentRecord()
{
	return ((ICommunicator *)m_data)->record;
}

bool IComm::isProcessCancelled()
{
	return (m_data == NULL) ? false : (((Communicator *)m_data)->IsCancelled());
}

void IComm::setNumSteps(int steps)
{
   if (m_data != NULL) {
      ((Communicator *)m_data)->CommPostMessage( Communicator::NUM_STEPS, steps );
   }
}

void IComm::setCurrentStep(int step)
{
   if (m_data != NULL) {
      ((Communicator *)m_data)->CommPostMessage( Communicator::CURRENT_STEP, step );
   }
}

void IComm::setNumRecords(int records)
{
   if (m_data != NULL) {
	  ((Communicator *)m_data)->CommPostMessage( Communicator::NUM_RECORDS, records );
   }
}

void IComm::setCurrentRecord(int record)
{
   if (m_data != NULL) {
      ((Communicator *)m_data)->CommPostMessage( Communicator::CURRENT_RECORD, record );
   }
}

//////////////////////////////////////////////////////////////////////////////////

// m_data for a IVGAMap should be a "PointMap *"

IVGAMap::IVGAMap()
{
   m_data = NULL;

   m_cursor_point = -1;
   m_cursor_selected_point = -1;
}

IVGAMap::~IVGAMap()
{
}

void IVGAMap::setData(void *data)
{
   m_data = data;
}

int IVGAMap::getPointCount()
{
   return ((PointMap *)m_data)->getPointCount();
}

int IVGAMap::getFirstPoint()
{
   PointMap& pd = *(PointMap *)m_data;
   PixelRef cur(0,0);
   while (cur.y < pd.getRows() && !pd.getPoint(cur).filled()) {
      cur.x += 1;
      if (cur.x == pd.getCols()) {
         cur.x = 0;
         cur.y += 1;
      }
   }
   if (cur.y == pd.getRows()) {
      m_cursor_point = -1;
      return -1;
   }
   m_cursor_point = (int)cur;
   return m_cursor_point;
}

int IVGAMap::getNextPoint()
{
   PointMap& pd = *(PointMap *)m_data;
   PixelRef cur(m_cursor_point);
   do {
      cur.x += 1;
      if (cur.x == pd.getCols()) {
         cur.x = 0;
         cur.y += 1;
      }
   } while (cur.y < pd.getRows() && !pd.getPoint(cur).filled());
   if (cur.y == pd.getRows()) {
      m_cursor_point = -1;
      return -1;
   }
   m_cursor_point = (int)cur;
   return m_cursor_point;
}

int IVGAMap::getSelectedPointCount()
{
   return ((PointMap *)m_data)->getSelCount();
}

int IVGAMap::getFirstSelectedPoint()
{
   if (((PointMap *)m_data)->getSelSet().size()) {
      m_cursor_selected_point = 0;
      return ((PointMap *)m_data)->getSelSet()[m_cursor_selected_point];
   }
   m_cursor_selected_point = -1;
   return m_cursor_selected_point;
}

int IVGAMap::getNextSelectedPoint()
{
   m_cursor_selected_point++;
   if (m_cursor_selected_point < (int)((PointMap *)m_data)->getSelSet().size()) {
      return ((PointMap *)m_data)->getSelSet()[m_cursor_selected_point];
   }
   m_cursor_selected_point = -1;
   return m_cursor_selected_point;
}

DPoint IVGAMap::getLocation(int id)
{
   if (id == -1) {
      return DPoint();
   }
   Point2f p = ((PointMap *)m_data)->depixelate(id);
   return DPoint(p.x,p.y);
}

void IVGAMap::clearSelection()
{
   ((PointMap *)m_data)->clearSel();
}

void IVGAMap::selectPoint(int id, bool replace) // replace defaults to false
{
   pvecint selset;
   selset.push_back(id);
   ((PointMap *)m_data)->setCurSel(selset,replace); // note: defaults to add to current selection
}

int IVGAMap::getNode(DPoint& point)
{
   // n.b., "false" returns unconstrained point: must check if inside bounds or not using includes
   PixelRef pix = ((PointMap *)m_data)->pixelate(Point2f(point.x,point.y), false);
   if (!((PointMap *)m_data)->includes(pix)) {
      return -1;
   }
   return int(pix);
}

bool IVGAMap::isEmptyNode(int id)
{
   if (id == -1) {
      return false;
   }
   return ((PointMap *)m_data)->getPoint(id).empty();
}

bool IVGAMap::isGraphNode(int id)
{
   if (id == -1) {
      return false;
   }
   return ((PointMap *)m_data)->getPoint(id).filled();
}

bool IVGAMap::isBoundaryNode(int id)
{
   if (id == -1) {
      return false;
   }
   return ((PointMap *)m_data)->getPoint(id).edge();
}

void IVGAMap::setMergePoint(int id, int merge_id)
{
   if (id == -1) {
      return;
   }
   PixelRef pix;
   if (merge_id == -1) {
      pix = id;
   }
   else {
      pix = PixelRef(merge_id);
   }
   // you cannot just set the merge point, as this is associated with a merge line
   // so this piece of code sets up the merge for you properly:
   ((PointMap *)m_data)->mergePixels(id,pix);
}

// if the point is merged with another point, let the user know what it is
int IVGAMap::getMergePoint(int id)
{
   if (id == -1) {
      return -1;
   }
   PixelRef pix = ((PointMap *)m_data)->getPoint(id).getMergePixel();
   if (pix == NoPixel) {
      return -1;
   }
   return int(pix);
}

// arbitrary data that the user might want to associate with a point
void IVGAMap::setUserData(int id, void *data)
{
   if (id == -1) {
      return;
   }
   ((PointMap *)m_data)->getPoint(id).setUserData(data);
}

// arbitrary data that the user might want to associate with a point
void *IVGAMap::getUserData(int id)
{
   if (id == -1) {
      return NULL;
   }
   return ((PointMap *)m_data)->getPoint(id).getUserData();
}

// the longest line of sight length for bin (0-31) (use -1 for longest line of sight in any direction)
// type 0: standard longest line of site, type 1: longest occluded edge
double IVGAMap::getLoSLength(int id, int bin, int type)
{
   PointMap& pd = *(PointMap *)m_data;
   if (id == -1 || !pd.getPoint(id).filled()) {
      return -1.0;
   }
   else if (bin == -1) {
      // note, just goes through all of them to find the longest!
      double maxdist = -1.0;
      for (int i = 0; i < 32; i++) {
         double thisdist = (type == 0) ? pd.getPoint(id).getNode().bindistance(bin) :
                                         pd.getPoint(id).getNode().occdistance(bin);
         if (thisdist > maxdist) {
            maxdist = thisdist;
         }
      }
      return maxdist;
   }
   else if (type == 0) {
      return pd.getPoint(id).getNode().bindistance(bin);
   }
   else {
      return pd.getPoint(id).getNode().occdistance(bin);
   }
}

int IVGAMap::getConnectedPointCount(int id, int b)
{
   PointMap& pd = *(PointMap *)m_data;
   if (id == -1 || !pd.getPoint(id).filled()) {
      return 0;
   }
   if (b == -1) {
      return pd.getPoint(id).getNode().count();
   }
   else {
      return pd.getPoint(id).getNode().bincount(b);
   }
}

int IVGAMap::getFirstConnectedPoint(int id, int b)
{
   PointMap& pd = *(PointMap *)m_data;
   if (id == -1 || !pd.getPoint(id).filled()) {
      return -1;
   }
   Node& node = pd.getPoint(id).getNode();
   if (b == -1) {
      if (node.count() == 0) {
         return -1;
      }
      else {
         node.first();
         return node.cursor();
      }
   }
   else {
      if (node.bincount(b) == 0) {
         return -1;
      }
      else {
         node.bin(b).first();
         return node.bin(b).cursor();
      }
   }
}

int IVGAMap::getNextConnectedPoint(int id, int b)
{
   PointMap& pd = *(PointMap *)m_data;
   if (id == -1 || !pd.getPoint(id).filled()) {
      return -1;
   }
   Node& node = pd.getPoint(id).getNode();
   if (b == -1) {
      node.next();
      if (node.is_tail()) {
         return -1;
      }
      else {
         return node.cursor();
      }
   }
   else {
      node.bin(b).next();
      if (node.bin(b).is_tail()) {
         return -1;
      }
      else {
         return node.bin(b).cursor();
      }
   }
}

char IVGAMap::getGridConnections(int id)
{
   // note, doesn't matter if this point is not filled -- it just has
   // zero grid connections
   if (id == -1) {
      return -1;
   }
   return ((PointMap *)m_data)->getPoint(id).getGridConnections();
}

double IVGAMap::getGridSpacing()
{
   return ((PointMap *)m_data)->getSpacing();

}

const char *IVGAMap::getDisplayedAttributeColumn()
{
   PointMap& pd = *(PointMap *)m_data;
   return pd.getAttributeTable().getColumnName(pd.getDisplayedAttribute()).c_str();
}

void IVGAMap::setDisplayedAttributeColumn(const char *attribute)
{
   if (strcmp(attribute,"Ref Number") == 0) {
      ((PointMap *)m_data)->setDisplayedAttribute(-1);
   }
   else {
      int col = ((PointMap *)m_data)->getAttributeTable().getColumnIndex(attribute);
      if (col != -1) {
         ((PointMap *)m_data)->setDisplayedAttribute(col);
      }
   }
}

void IVGAMap::analysePointDepth(IComm *comm, int method)
{
   PointMap& pd = *(PointMap *)m_data;

   bool created_comm = false;
   if (comm == NULL) {
	  comm = getIComm();
	  if (comm == NULL || comm->isValid()) {
		   return;
	  }
	  created_comm = true;
   }

   switch (method) {
      case IVGA_STEP:
         pd.analyseVisualPointDepth((Communicator *)(comm->getData()));
         break;
      case IVGA_METRIC:
         pd.analyseMetricPointDepth((Communicator *)(comm->getData()));
         break;
      case IVGA_ANGULAR:
         pd.analyseAngularPointDepth((Communicator *)(comm->getData()));
         break;
   }

   if (created_comm) {
	  delete comm;
   }
}

//////////////////////////////////////////////////////////////////////////////////

// m_data for a IShapeMap should be a "ShapeMap *"

// remind programmers that cursors are invalidated by add / remove shape

IShapeMap::IShapeMap()
{
   m_data = NULL;

   m_cursor_shape = -1;
   m_cursor_selected_shape = -1;
}

IShapeMap::~IShapeMap()
{
}

void IShapeMap::setData(void *data)
{
   m_data = data;
}

// Basic info about the shape map

const char *IShapeMap::getMapName()
{
   return ((ShapeMap *)m_data)->getName().c_str();
}

// basic info about the map type 
bool IShapeMap::isAxialMap()
{
   return ((ShapeMap *)m_data)->isAxialMap();
}
bool IShapeMap::isSegmentMap()
{
   return ((ShapeMap *)m_data)->isSegmentMap();
}
bool IShapeMap::isDataMap()
{
   return (((ShapeMap *)m_data)->getMapType() == ShapeMap::DATAMAP);
}
bool IShapeMap::isDrawingMap()
{
   return (((ShapeMap *)m_data)->getMapType() == ShapeMap::DRAWINGMAP);
}
int IShapeMap::getMapType()
{
   int actual_type = ((ShapeMap *)m_data)->getMapType();
   int return_type = 0;
   // at the moment there are fewer map types in idepthmap.h than in shapemap.h
   switch (actual_type) {
      case ShapeMap::DRAWINGMAP: 
         return_type = IShapeMap::IDRAWINGMAP;
         break;
      case ShapeMap::DATAMAP: 
         return_type = IShapeMap::IDATAMAP;
         break;
      case ShapeMap::CONVEXMAP: 
         return_type = IShapeMap::ICONVEXMAP;
         break;
      case ShapeMap::AXIALMAP: case ShapeMap::ALLLINEMAP:
         return_type = IShapeMap::IAXIALMAP;
         break;
      case ShapeMap::SEGMENTMAP: 
         return_type = IShapeMap::ISEGMENTMAP;
         break;
      case ShapeMap::PESHMAP: case ShapeMap::POINTMAP: 
         return_type = IShapeMap::ISHAPEMAP;
         break;
      default:
         return_type = 0;
         break;
   };
   return return_type;
}

// iterator for all shapes (uses m_cursor_shape)
int IShapeMap::getShapeCount()
{
   return ((ShapeMap *)m_data)->getShapeCount();
}

int IShapeMap::getFirstShape()
{
   ShapeMap& axmap = *((ShapeMap *)m_data);
   
   m_cursor_shape = 0;

   if (m_cursor_shape >= (int)axmap.getShapeCount()) {
      m_cursor_shape = -1;
   }

   return m_cursor_shape;
}

int IShapeMap::getNextShape()
{
   ShapeMap& axmap = *((ShapeMap *)m_data);
   
   m_cursor_shape++;

   if (m_cursor_shape >= (int)axmap.getShapeCount()) {
      m_cursor_shape = -1;
   }

   return m_cursor_shape;
}

/// iterator for selected shapes in the Depthmap UI (uses m_cursor_selected_shape)
int IShapeMap::getSelectedShapeCount()
{
   return ((ShapeMap *)m_data)->getSelCount();
}

int IShapeMap::getFirstSelectedShape()
{
   ShapeMap& axmap = *((ShapeMap *)m_data);
   if (axmap.getSelCount() > 0) {
      m_cursor_selected_shape = 0;
      return axmap.getSelSet().at(0);
   }
   else {
      m_cursor_selected_shape = -1;
      return -1;
   }
}

int IShapeMap::getNextSelectedShape()
{
   ShapeMap& axmap = *((ShapeMap *)m_data);
   if (axmap.isSelected() && m_cursor_selected_shape < (int)axmap.getSelCount() - 1) {
      m_cursor_selected_shape++;
      return axmap.getSelSet().at(m_cursor_selected_shape);
   }
   else {
      m_cursor_selected_shape = -1;
      return -1;
   }
}

/////////////////////////////////////////////////////////////////////////////

// 
void IShapeMap::clearSelection()
{
   ((ShapeMap *)m_data)->clearSel();
}

void IShapeMap::selectShape(int id, bool replace) // replace defaults to false
{
   pvecint temp;
   temp.push_back(id);
   ((ShapeMap *)m_data)->setCurSelDirect(temp, !replace ); // <- default: add to existing selection
}

int IShapeMap::selectShapesWithinRadius(DPoint& point, double dist)
{
   ShapeMap& map = *((ShapeMap *)m_data);
   pvecint shapes;
   int ret = map.withinRadius( Point2f(point.x,point.y), dist, shapes );
   map.setCurSelDirect(shapes,true);   // <- note, add to existing selection
   return ret;
}

/////////////////////////////////////////////////////////////////////////////

// iterator for connections
int IShapeMap::getConnectedShapeCount(int id, int dir)
{
   const Connector& axline = ((ShapeMap *)m_data)->getConnections().at(id);
   int linecount = 0;
   switch (dir) {
   case CONN_ALL:
      if (((ShapeMap *)m_data)->isSegmentMap()) {
         linecount = axline.count(Connector::SEG_CONN_ALL);
      }
      else {
         linecount = axline.count(Connector::CONN_ALL);
      }
      break;
   case CONN_FW:
      linecount = axline.count(Connector::SEG_CONN_FW);
      break;
   case CONN_BK:
      linecount = axline.count(Connector::SEG_CONN_BK);
      break;
   }
   return linecount;
}

int IShapeMap::getFirstConnectedShape(int id, int dir)
{
   const Connector& axline = ((ShapeMap *)m_data)->getConnections().at(id);
   int retvar = -1;
   axline.first();
   switch (dir) {
   case CONN_ALL:
      if (((ShapeMap *)m_data)->isSegmentMap()) {
         retvar = axline.cursor(Connector::SEG_CONN_ALL);
      }
      else {
         retvar = axline.cursor(Connector::CONN_ALL);
      }
      break;
   case CONN_FW:
      retvar = axline.cursor(Connector::SEG_CONN_FW);
      break;
   case CONN_BK:
      retvar = axline.cursor(Connector::SEG_CONN_BK);
      break;
   }
   return retvar;
}

int IShapeMap::getNextConnectedShape(int id, int dir)
{
   const Connector& axline = ((ShapeMap *)m_data)->getConnections().at(id);
   int retvar = -1;
   axline.next();
   switch (dir) {
   case CONN_ALL:
      if ( ((ShapeMap *)m_data)->isSegmentMap()) {
         retvar = axline.cursor(Connector::SEG_CONN_ALL);
      }
      else {
         retvar = axline.cursor(Connector::CONN_ALL);
      }
      break;
   case CONN_FW:
      retvar = axline.cursor(Connector::SEG_CONN_FW);
      break;
   case CONN_BK:
      retvar = axline.cursor(Connector::SEG_CONN_BK);
      break;
   }
   return retvar;
}

int IShapeMap::getConnectionDirection(int id, int dir)
{
   const Connector& axline =  ((ShapeMap *)m_data)->getConnections().at(id);
   int retvar = -1;
   if ( ((ShapeMap *)m_data)->isSegmentMap()) {
      switch (dir) {
      case CONN_ALL:
         retvar = (axline.direction(Connector::SEG_CONN_ALL) == 1) ? CONN_FW : CONN_BK;
         break;
      case CONN_FW:
         retvar = (axline.direction(Connector::SEG_CONN_FW) == 1) ? CONN_FW : CONN_BK;
         break;
      case CONN_BK:
         retvar = (axline.direction(Connector::SEG_CONN_BK) == 1) ? CONN_FW : CONN_BK;
         break;
      }
   }
   return retvar;
}

double IShapeMap::getConnectionWeight(int id, int dir)
{
   const Connector& axline =  ((ShapeMap *)m_data)->getConnections().at(id);
   double retvar = 0.0;
   if ( ((ShapeMap *)m_data)->isSegmentMap()) {
      switch (dir) {
      case CONN_ALL:
         retvar = axline.weight(Connector::SEG_CONN_ALL);
         break;
      case CONN_FW:
         retvar = axline.weight(Connector::SEG_CONN_FW);
         break;
      case CONN_BK:
         retvar = axline.weight(Connector::SEG_CONN_BK);
         break;
      }
   }
   return retvar;
}

// test shape is a point and test shape is a line (also need others)
bool IShapeMap::isPointShape(int id)
{
   return ((ShapeMap *)m_data)->getAllShapes().at(id).isPoint();
}
bool IShapeMap::isLineShape(int id)
{
   return ((ShapeMap *)m_data)->getAllShapes().at(id).isLine();
}
bool IShapeMap::isPolyLineShape(int id)
{
   return ((ShapeMap *)m_data)->getAllShapes().at(id).isPolyLine();
}
bool IShapeMap::isPolygonShape(int id)
{
   return ((ShapeMap *)m_data)->getAllShapes().at(id).isPolygon();
}

// the physical point coordinates (test it's a line first)
DPoint IShapeMap::getPointCoords(int id)
{
   // note, using rowid
   ShapeMap& axmap = *((ShapeMap *)m_data);
   const Point2f& pt = axmap.getAllShapes().at(id).getPoint();
   return DPoint(pt.x,pt.y);
}

// the physical line coordinates (test it's a line first)
DLine IShapeMap::getLineCoords(int id)
{
   // note, using rowid
   ShapeMap& axmap = *((ShapeMap *)m_data);
   const Line& linein = axmap.getAllShapes().at(id).getLine();
   return DLine(DPoint(linein.ax(),linein.ay()),DPoint(linein.bx(),linein.by()));
}

// for polylines and polygons, use a pair of functions:
int IShapeMap::getVertexCount(int id)
{
   return ((ShapeMap *)m_data)->getAllShapes().at(id).size();
}

DPoint IShapeMap::getVertex(int id, int v)
{
   Point2f pt = ((ShapeMap *)m_data)->getAllShapes().at(id).at(v);
   return DPoint(pt.x,pt.y);
}

////////////////////////////////////////////////////////////////////////

int IShapeMap::getShape(DPoint& point)
{
   return ((ShapeMap *)m_data)->getClosestOpenGeom(Point2f(point.x,point.y));
}

// Get the displayed attribute column
const char *IShapeMap::getDisplayedAttributeColumn()
{
   ShapeMap& map = *((ShapeMap *)m_data);
   return map.getAttributeTable().getColumnName(map.getDisplayedAttribute()).c_str();
}

// Display the attribute column 
void IShapeMap::setDisplayedAttributeColumn(const char *attribute)
{
   if (strcmp(attribute,"Ref Number") == 0) {
      // invalidate it, as it's almost certain the user wants to redraw the output:
      ((ShapeMap *)m_data)->invalidateDisplayedAttribute();
      ((ShapeMap *)m_data)->setDisplayedAttribute(-1);
   }
   else {
      int col = ((ShapeMap *)m_data)->getAttributeTable().getColumnIndex(attribute);
      if (col != -1) {
         // invalidate it, as it's almost certain the user wants to redraw the output:
         ((ShapeMap *)m_data)->invalidateDisplayedAttribute();
         ((ShapeMap *)m_data)->setDisplayedAttribute(col);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////

// Editing axial maps

void IShapeMap::clearMap()
{
   // zap everything apart from mapinfodata (i.e., the projection)... use with caution!
   ((ShapeMap *)m_data)->clearAll();
}

void IShapeMap::beginShape()
{
   // first, this will interrupt cursors for selections and line retrieval
   // note, do *not* clear the selection here as repeated calls to beginShape will slow to a crawl
   m_cursor_shape = -1;
   m_cursor_selected_shape = -1;

   ((ShapeMap *)m_data)->shapeBegin();
}

void IShapeMap::addVertex(DPoint& point)
{
   ((ShapeMap *)m_data)->shapeVertex(Point2f(point.x,point.y));
}

int IShapeMap::endShape(bool open)
{
   return ((ShapeMap *)m_data)->shapeEnd(open);
}

void IShapeMap::commitShapes()
{
   // note, intended for more than one shape
   ((ShapeMap *)m_data)->shapesCommit();
}

bool IShapeMap::removeShape(int id)
{
   ShapeMap& axmap = *((ShapeMap *)m_data);

   // first, this will interrupt cursors for selections and shape retrieval:
   axmap.clearSel();
   m_cursor_shape = -1;
   m_cursor_selected_shape = -1;

   // note: uses rowid so convert to key.
   // Also note that "undoing" set to true means that it won't try to set up and undo buffer
   if (id >= 0 && id < (int)axmap.getShapeCount()) {
      axmap.removeShape(axmap.getAttributeTable().getRowKey(id),true);
      return true;
   }

   return false;
}

bool IShapeMap::connectDirected(int from, int fromdir, int to, int todir, double conn_weight)
{
   // translate to dmap conn values:
   if (fromdir == CONN_FW) {
      fromdir = 1;
   }
   else {
      fromdir = -1;
   }
   if (todir == CONN_FW) {
      todir = 1;
   }
   else {
      todir = -1;
   }
   return ((ShapeMap *)m_data)->linkShapes(from,fromdir,to,todir,(float)conn_weight);
}

bool IShapeMap::connectUndirected(int id1, int id2)
{
   return ((ShapeMap *)m_data)->linkShapes(id1,id2,false);
}

int IShapeMap::connectIntersected(int id)
{
   return ((ShapeMap *)m_data)->connectIntersected(id,false);
}

void IShapeMap::makeAllConnections()
{
   ((ShapeMap *)m_data)->makeShapeConnections();
}

bool IShapeMap::disconnect(int id1, int id2)
{
   return ((ShapeMap *)m_data)->unlinkShapes(id1,id2,false);
}

/////////////////////////////////////////////////////////////////////////////////////////

// Shape map extras

bool IShapeMap::exportMap(const char *filename, const char *type) 
{
	bool retvar = false;

   string type_str = string(type);

   if (!(type_str.compare("MIF") == 0 || type_str.compare("mif") == 0)) {
      return retvar;
   }

   string filename_str = string(filename);

   // if extension exists already, strip it
   if (filename_str.rfind(".mif") == filename_str.length() - 4 || filename_str.rfind(".MIF") == filename_str.length() - 4) {
      filename_str = filename_str.substr(0,filename_str.length()-4);
   }

	string miffilename = filename_str;
   miffilename.append(".mif");
	string midfilename = filename_str;
   midfilename.append(".mid");

	ofstream miffile(miffilename.c_str());
    if (miffile.fail() || miffile.bad()) {
        return retvar;
    }
    ofstream midfile(midfilename.c_str());
    if (midfile.fail() || midfile.bad()) {
        return retvar;
    }
    
	retvar = ((ShapeMap *)m_data)->outputMifMap(miffile, midfile);

	return retvar;
}

int IShapeMap::analyseSegments(IComm *comm, size_t radius_count, float *radius_list, const char *weighting_column, const char *weighting_column2, const char* routeweight_column, bool interactive)
{
   ShapeGraph& map = *((ShapeGraph *)m_data);

   if (!map.isSegmentMap()) {
      return 0;
   }

   Options options;
   options.tulip_bins = 1024;
   options.choice = true;
   options.weighted_measure_col = -1;
   options.radius_type = Options::RADIUS_METRIC;
   for (size_t i = 0; i < radius_count; i++) {
	   options.radius_list.push_back(radius_list[i]);
   }
   if (radius_count == 0) {
	   // default: radius n
	   options.radius_list.push_back(-1.0f);
   }

   bool created_comm = false;
   if (comm == NULL) {
	  comm = getIComm();
	  if (comm == NULL || comm->isValid()) {
		   return false;
	  }
	  created_comm = true;
   }
   
   std::string weighting_column_str;
   if (weighting_column != NULL) {
      weighting_column_str = weighting_column;
   }
   else {
      weighting_column_str = "Segment Length";
   }
   options.weighted_measure_col = map.getAttributeTable().getColumnIndex(weighting_column_str);
	//EFEF
   std::string weighting_column_str2;
   if (weighting_column2 != NULL) {
      weighting_column_str2 = weighting_column2;
   }
   else {
      weighting_column_str2 = "";
   }
   options.weighted_measure_col2 = map.getAttributeTable().getColumnIndex(weighting_column_str2);
	//routeweight
    std::string routeweight_column_str;
	if (routeweight_column != NULL) {
      routeweight_column_str = routeweight_column;
   }
   else {
      routeweight_column_str = "";
   }
	options.routeweight_col = map.getAttributeTable().getColumnIndex(routeweight_column_str);
   //EFEF
   
   int count = 0;

   try {
	   // note: non-interactive sets selection_only to true and interactive to false (for multi-processor environments)
	   count = map.analyseTulip((Communicator *)(comm->getData()), options.tulip_bins, options.choice, options.radius_type, options.radius_list, options.weighted_measure_col, options.weighted_measure_col2, options.routeweight_col, !interactive, interactive);
   }
   catch (Communicator::CancelledException)
   {
      // this is only used in interactive mode (in non-interactive, it will not throw the exception, simply return count of the number of records it processed)
      count = 0;
   }

   if (created_comm) {
	  delete comm;
   }

   return count;
}

bool IShapeMap::loadUnlinks(IShapeMap *unlinks_points_map) 
{
	((ShapeGraph *)m_data)->unlinkFromShapeMap(*((ShapeMap *)(unlinks_points_map->m_data)));
   return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

// m_data for a IAttributes should be an "AttributeTable *"

IAttributes::IAttributes()
{
   m_data = NULL;
   m_analysis_type = -1;
   // note, m_cursor_column = -1 used for ref_number column
   m_cursor_column = -2;
   m_cursor_row = -1;
}

IAttributes::~IAttributes()
{
}

void IAttributes::setData(void *data, int analysis_type)
{
   m_data = data;
   if (analysis_type & DLL_VGA_ANALYSIS) {
      m_analysis_type = DLL_VGA_ANALYSIS;
   }
   else {
      m_analysis_type = DLL_AXIAL_ANALYSIS | DLL_SEGMENT_ANALYSIS | DLL_DATA_ANALYSIS;
   }
}

int IAttributes::getAttributeColumnCount()
{
   return ((AttributeTable *)m_data)->getColumnCount();
}

const char *IAttributes::getFirstAttributeColumn()
{
   AttributeTable& table = *((AttributeTable *)m_data);

   m_cursor_column = -1;

   if (m_cursor_column >= table.getColumnCount()) {
      m_cursor_column = -2;
      return NULL;
   }

   return table.getColumnName(m_cursor_column).c_str();
}

const char *IAttributes::getNextAttributeColumn()
{
   AttributeTable& table = *((AttributeTable *)m_data);

   m_cursor_column++;

   if (m_cursor_column >= table.getColumnCount()) {
      m_cursor_column = -2;
      return NULL;
   }

   return table.getColumnName(m_cursor_column).c_str();
}

// Get the row entries from the attributes
int IAttributes::getAttributeRowCount()
{
   return ((AttributeTable *)m_data)->getRowCount();
}

int IAttributes::getFirstAttributeRow()
{
   AttributeTable& table = *((AttributeTable *)m_data);
   m_cursor_row = 0;
   if (m_cursor_row >= table.getRowCount()) {
      m_cursor_row = -1;
      return -1;
   }
   if (m_analysis_type != DLL_VGA_ANALYSIS) {   
      return m_cursor_row; // anything but vga
   }
   else {
      return table.getRowKey(m_cursor_row); // vga only
   }
}

int IAttributes::getNextAttributeRow()
{
   AttributeTable& table = *((AttributeTable *)m_data);
   m_cursor_row++;
   if (m_cursor_row >= table.getRowCount()) {
      m_cursor_row = -1;
      return -1;
   }
   if (m_analysis_type != DLL_VGA_ANALYSIS) {
      return m_cursor_row; // anything but vga
   }
   else {
      return table.getRowKey(m_cursor_row); // vga only
   }
}

/////////////////////////////////////////////////////////////////////////

bool IAttributes::insertAttributeColumn(const char *attribute)
{
   if (strcmp(attribute,"Ref Number") != 0) {
      AttributeTable *table = (AttributeTable *)m_data;
      int n = table->getColumnIndex(attribute);
      if (n == -1 || !table->isColumnLocked(n)) {
         table->insertColumn(attribute);
         return true;
      }
   }
   return false;
}

bool IAttributes::deleteAttributeColumn(const char *attribute)
{
   AttributeTable *table = (AttributeTable *)m_data;
   int n = table->getColumnIndex(attribute);
   if (n != -1 && !table->isColumnLocked(n)) {
      table->removeColumn(n);
      if (n >= table->getDisplayColumn()) {
         // there is a problem here: the associated geometry layer will be out of step when it comes to display
         // this is corrected in "getDisplayedAttribute" (untidy, but works)
         table->setDisplayColumn(n-1,true);
      }
      return true;
   }
   return false;
}

bool IAttributes::renameAttributeColumn(const char *oldname, const char *newname)
{
   AttributeTable *table = (AttributeTable *)m_data;
   int n = table->getColumnIndex(oldname);
   if (n != -1 && !table->isColumnLocked(n)) {
      table->renameColumn(n,newname);
      return true;
   }
   return false;
}

bool IAttributes::isValidAttributeColumn(const char *attribute)
{
   return ((AttributeTable *)m_data)->isValidColumn(attribute);
}

bool IAttributes::isLockedAttributeColumn(const char *attribute)
{
   AttributeTable *table = (AttributeTable *)m_data;
   int n = table->getColumnIndex(attribute);
   if (n != -1 && !table->isColumnLocked(n)) {
      return table->isColumnLocked(n);
   }
   // n.b., this should really throw an exception:
   return false;
}

// get an attribute from the attribute table for the current point:
float IAttributes::getAttribute(int id, const char *attribute)
{
   AttributeTable& table = *((AttributeTable *)m_data);
   int index = (m_analysis_type != DLL_VGA_ANALYSIS) ? id : table.getRowid(id); // vga needs to look up rowid, all others use rowid directly
   if (index == -1) {
      return -1.0f;
   }
   return table.getValue(index,attribute);
}

// set an attribute in the attribute table for the current point:
void IAttributes::setAttribute(int id, const char *attribute, float value)
{
   AttributeTable& table = *((AttributeTable *)m_data);
   int index = (m_analysis_type != DLL_VGA_ANALYSIS) ? id : table.getRowid(id); // vga needs to look up rowid, all others use rowid directly
   if (index == -1) {
      return;
   }
   // nb., changeValue modifies tot value (but not reduce min/max)
   table.changeValue(index,attribute,value);
}

// set an attribute in the attribute table for the current point:
void IAttributes::incrAttribute(int id, const char *attribute)
{
   AttributeTable& table = *((AttributeTable *)m_data);
   int index = (m_analysis_type != DLL_VGA_ANALYSIS) ? id : table.getRowid(id); // vga needs to look up rowid, all others use rowid directly
   if (index == -1) {
      return;
   }
   // nb., changeValue modifies tot value (but not reduce min/max)
   table.incrValue(index,attribute);
}

/////////////////////////////////////////////////////////////////////////

bool IAttributes::importTable(const char *filename, bool merge)
{
   ifstream stream(filename);
   if (!stream) {
      return false;
   }
   return ((AttributeTable *)m_data)->importTable(stream, merge);
}

bool IAttributes::exportTable(const char *filename, bool updated_only)
{
   ofstream stream(filename);
   if (!stream) {
      return false;
   }
   return ((AttributeTable *)m_data)->exportTable(stream, updated_only);
}


