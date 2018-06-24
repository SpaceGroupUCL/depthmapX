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


#include <QtGui>
#include <QEvent>
#include "mainwindow.h"


CMSCommunicator::CMSCommunicator()
{ 
	m_function = -1;

    simple_version = true; // we want simple dX // TV
//   GetApp()->m_process_count += 1;
}

CMSCommunicator::~CMSCommunicator()
{
}

inline void CMSCommunicator::CommPostMessage(int m, int x) const
{
   QGraphDoc *pDoc = (QGraphDoc *)parent_doc;
   pDoc->ProcPostMessage(m, x);
}

//! [0]
RenderThread::RenderThread(QObject *parent)
    : QThread(parent)
{
    abort = false;
}
//! [0]

//! [1]
RenderThread::~RenderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}
//! [1]

//! [2]
void RenderThread::render(void* Praram)
{
	m_parent = Praram;

    if (!isRunning()) {
        start(NormalPriority);
    } else {
        restart = true;
        condition.wakeOne();
    }
}
//! [2]

//! [3]
void RenderThread::run()
{
   QGraphDoc *pDoc = (QGraphDoc *) m_parent;
   CMSCommunicator *comm = pDoc->m_communicator;
   comm->parent_doc = m_parent;
   MainWindow* pMain = (MainWindow*)pDoc->m_mainFrame;

   if (comm) {
       // move simple setting to comm
       comm->simple_version = pMain->m_simpleVersion;

      int ok;
      switch (comm->GetFunction()) 
	  {
      case CMSCommunicator::IMPORT:
         ok = pDoc->m_meta_graph->loadLineData( comm, comm->GetOption() );
         if (ok == 1) {
			 pDoc->modifiedFlag = true;
         }
         else if (ok == -1) {
             emit showWarningMessage(tr("Warning"), tr("An error was found in the import file"));
         }
         // This might change the line layers available, alert the layer chooser:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADDRAWING));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::IMPORTMIF:
         ok = pDoc->m_meta_graph->loadMifMap( comm, *comm, comm->GetInfile2() );
         switch (ok) {
         case MINFO_MULTIPLE:
             //BUG
            //QMessageBox::warning(0, tr("Warning"), tr("The imported MIF file contains multiple shapes per object.\n Please note that depthmapX has broken these up, so each shape has one row of attribute data.\n Please consult your MapInfo provider for details."), QMessageBox::Ok, QMessageBox::Ok);
        case MINFO_OK:
            pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            break;
         case MINFO_HEADER:
            emit showWarningMessage(tr("Warning"), tr("depthmapX had a problem reading the header information in your MIF file."));
            break;
         case MINFO_TABLE:
            emit showWarningMessage(tr("Warning"), tr("depthmapX had a problem reading the table data in your MID file."));
            break;
         case MINFO_MIFPARSE:
            emit showWarningMessage(tr("Warning"), tr("depthmapX had a problem reading the shape data in your MIF file.\n\
                                        Please ensure that your shape data contains only points, lines, polylines or regions."));
            break;
         case MINFO_OBJROWS:
            emit showWarningMessage(tr("Warning"), tr("depthmapX had a problem reading the shape data in your MIF file.\n\
									It seems as though there are a different number of shapes to rows in the associated table.\n\
                                    This may be due to the existance of unsupported shape types in the file."));
            break;
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEPOINTS:
         ok = pDoc->m_meta_graph->makePoints( comm->GetSeedPoint(), comm->GetOption(), comm );
         if (ok) {
			 pDoc->modifiedFlag = true;
         }
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEGRAPH:
         ok = pDoc->m_meta_graph->makeGraph( comm, pDoc->m_make_algorithm, pDoc->m_make_maxdist );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::ANALYSEGRAPH:
         ok = pDoc->m_meta_graph->analyseGraph( comm, pMain->m_options, comm->simple_version);
         pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEISOVIST:
         if (comm->GetSeedAngle() == -1.0) {
            ok = pDoc->m_meta_graph->makeIsovist( comm, comm->GetSeedPoint() );
         }
         else {
            double ang1 = comm->GetSeedAngle() - comm->GetSeedFoV() * 0.5;
            double ang2 = comm->GetSeedAngle() + comm->GetSeedFoV() * 0.5;
            if (ang1 < 0.0) 
               ang1 += 2.0 * M_PI;
            if (ang2 > 2.0 * M_PI)
               ang2 -= 2.0 * M_PI;
            ok = pDoc->m_meta_graph->makeIsovist( comm, comm->GetSeedPoint(), ang1, ang2 );
         }
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEISOVISTPATH:
         // the graph is going to build this path from a selection in a data map:
         // a data map must be topmost with lines or polylines selected
         // linedata must be displayed as per usual
         ok = pDoc->m_meta_graph->makeIsovistPath( comm, comm->GetSeedAngle(), comm->simple_version );

         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEALLLINEMAP:
         ok = pDoc->m_meta_graph->makeAllLineMap( comm, comm->GetSeedPoint() );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEFEWESTLINEMAP:
         ok = pDoc->m_meta_graph->makeFewestLineMap( comm, comm->GetOption() );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEDRAWING:
         // option 1 is: 0 a data map, 1 an axial map
         ok = pDoc->m_meta_graph->convertToDrawing( comm, comm->GetString().toStdString(), comm->GetOption(1) );
         if (ok) {
			 pDoc->modifiedFlag = true;
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADDRAWING));
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERMAP:
         ok = pDoc->m_meta_graph->convertDrawingToAxial( comm, comm->GetString().toStdString());
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERMAPSHAPE:
         ok = pDoc->m_meta_graph->convertDataToAxial( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               pDoc->SetUpdateFlag(QGraphDoc::DELETED_TABLE);
            }
            else {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No lines available in current layer to convert to axial lines"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERSEGMAP:
         ok = pDoc->m_meta_graph->convertDrawingToSegment( comm, comm->GetString().toStdString() );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERSEGMAPSHAPE:
         ok = pDoc->m_meta_graph->convertDataToSegment( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               pDoc->SetUpdateFlag(QGraphDoc::DELETED_TABLE);
            }
            else {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No lines available in current layer to convert to segments"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;
      
      case CMSCommunicator::MAKEGATESMAP:
         // note: relies on the fact that make data map from drawing sets option 1 to -1, whereas make data layer from graph it to either 0 or 1
         ok = pDoc->m_meta_graph->convertToData( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), comm->GetOption(1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               pDoc->SetUpdateFlag(QGraphDoc::DELETED_TABLE);
            }
            else {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKECONVEXMAP:
         // note: relies on the fact that make convex map from drawing sets option 1 to -1, whereas make convex map from data sets it to either 0 or 1
         ok = pDoc->m_meta_graph->convertToConvex( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), comm->GetOption(1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               pDoc->SetUpdateFlag(QGraphDoc::DELETED_TABLE);
            }
            else {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No polygons currently visible in drawing layers"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEBOUNDARYMAP:
         break;

      case CMSCommunicator::MAKESEGMENTMAP:
         // convert percentage to fraction, and pass to metagraph
         ok = pDoc->m_meta_graph->convertAxialToSegment( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1), double(comm->GetOption(2)) / 100.0);
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               pDoc->SetUpdateFlag(QGraphDoc::DELETED_TABLE);
            }
            else {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            emit showWarningMessage(tr("Warning"), tr("No lines exist in map to convert to segments"));
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::AXIALANALYSIS:
         ok = pDoc->m_meta_graph->analyseAxial( comm, pMain->m_options, comm->simple_version );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::SEGMENTANALYSISTULIP:
         ok = pDoc->m_meta_graph->analyseSegmentsTulip( comm, pMain->m_options );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::SEGMENTANALYSISANGULAR:
         ok = pDoc->m_meta_graph->analyseSegmentsAngular( comm, pMain->m_options );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::TOPOMETANALYSIS:
         ok = pDoc->m_meta_graph->analyseTopoMet( comm, pMain->m_options );
         if (ok) {
            pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
         }
         pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::POINTDEPTH:
         {
            // Set up for options step depth selection
            Options options;
            options.global = 0;
            options.point_depth_selection = 1;
            
            ok = pDoc->m_meta_graph->analyseGraph( comm, options, comm->simple_version);
            if (ok) {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
            }
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
         }
         break;     

      case CMSCommunicator::METRICPOINTDEPTH:
         {
            // Set up for options metric point depth selection
            Options options;
            options.global = 0;
            options.point_depth_selection = 2;

            ok = pDoc->m_meta_graph->analyseGraph( comm, options, comm->simple_version );
            if (ok) {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
            }
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
         }
         break;

      case CMSCommunicator::ANGULARPOINTDEPTH:
         {
            // Set up for options angular point depth selection
            Options options;
            options.global = 0;
            options.point_depth_selection = 3;

            ok = pDoc->m_meta_graph->analyseGraph( comm, options, comm->simple_version );
            if (ok) {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
            }
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
         }
         break;

      case CMSCommunicator::TOPOLOGICALPOINTDEPTH:
         {
            // Set up for options topological point depth selection (segment maps only)
            Options options;
            options.global = 0;
            options.point_depth_selection = 4;

            ok = pDoc->m_meta_graph->analyseGraph( comm, options, comm->simple_version);
            if (ok) {
               pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
            }
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
         }
         break;

      case CMSCommunicator::AGENTANALYSIS:
         {
            try {
              pDoc->m_meta_graph->runAgentEngine( comm );
              pDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
              pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
            } catch (depthmapX::PointMapException const & e) {
              emit runtimeExceptionThrown(e.getErrorType(), e.what());
          }
         }
         break;
      }

      emit closeWaitDialog();
	  msleep(100);
      delete pDoc->m_communicator;     // Ensure we delete *the* communicator
      pDoc->m_communicator = NULL;
      // REDRAW_TOTAL to REDRAW_GRAPH // recenterView after fill // TV
      pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
   }
   return;
}
