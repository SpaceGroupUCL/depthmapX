// Copyright (C) 2011-2026, Tasos Varoudis

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
#include <exception>
#include <stdexcept>
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
   emit pDoc->m_thread.progressChanged(m, x);
}

void CMSCommunicator::runAnalysis(QGraphDoc &graphDoc) {
    if (!m_analysis) {
        throw std::logic_error("No module analysis was configured.");
    }
    if (m_analysis->run(this)) {
        if (m_successUpdateFlagType != -1) {
            emit graphDoc.m_thread.updateRequested(m_successUpdateFlagType, m_successUpdateFlagModified);
        }
        if (m_successRedrawFlag != QGraphDoc::REDRAW_DONE) {
            emit graphDoc.m_thread.redrawRequested(m_successRedrawFlagViewType, m_successRedrawFlag,
                                                  m_successRedrawReason);
        }
    }
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
   if (!comm) {
       return;
   }
   comm->parent_doc = m_parent;
   MainWindow *pMain = static_cast<MainWindow *>(pDoc->m_mainFrame);

   QString title;
   QString message;
   try {
      comm->simple_version = pMain->m_simpleVersion;

      int ok = 1;
      switch (comm->GetFunction()) 
	  {
      case CMSCommunicator::IMPORT:
         ok = pDoc->m_meta_graph->loadLineData( comm, comm->GetOption() );
         if (ok == 1) {
			 emit updateRequested(QGraphDoc::UNDECLARED);
         }
         else if (ok == -1) {
             emit showWarningMessage(tr("Warning"), tr("An error was found in the import file"));
         }
         // This might change the line layers available, alert the layer chooser:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADDRAWING));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::IMPORTMIF:
         ok = pDoc->m_meta_graph->loadMifMap( comm, comm->getInFileStream(), comm->GetInfile2() );
         switch (ok) {
         case MINFO_MULTIPLE:
             //BUG
            //QMessageBox::warning(0, tr("Warning"), tr("The imported MIF file contains multiple shapes per object.\n Please note that depthmapX has broken these up, so each shape has one row of attribute data.\n Please consult your MapInfo provider for details."), QMessageBox::Ok, QMessageBox::Ok);
        case MINFO_OK:
            emit updateRequested(QGraphDoc::NEW_TABLE);
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
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEPOINTS:
         ok = pDoc->m_meta_graph->makePoints( comm->GetSeedPoint(), comm->GetOption(), comm );
         if (ok) {
			 emit updateRequested(QGraphDoc::UNDECLARED);
         }
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEGRAPH:
         ok = pDoc->m_meta_graph->makeGraph( comm, pDoc->m_make_algorithm, pDoc->m_make_maxdist );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::ANALYSEGRAPH:
         ok = pDoc->m_meta_graph->analyseGraph( comm, pMain->m_options, comm->simple_version);
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEISOVIST:
         if (comm->GetSeedAngle() == -1.0) {
            ok = pDoc->m_meta_graph->makeIsovist( comm, comm->GetSeedPoint(), 0, 0, comm->simple_version );
         }
         else {
            double ang1 = comm->GetSeedAngle() - comm->GetSeedFoV() * 0.5;
            double ang2 = comm->GetSeedAngle() + comm->GetSeedFoV() * 0.5;
            if (ang1 < 0.0) 
               ang1 += 2.0 * M_PI;
            if (ang2 > 2.0 * M_PI)
               ang2 -= 2.0 * M_PI;
            ok = pDoc->m_meta_graph->makeIsovist( comm, comm->GetSeedPoint(), ang1, ang2, comm->simple_version );
         }
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEISOVISTSFROMFILE: {

          try {
              auto isovists = EntityParsing::parseIsovists(comm->GetInfile2(), ',');
              comm->CommPostMessage(Communicator::NUM_STEPS, isovists.size());
              int isovistIdx = 1;
              for (IsovistDefinition &isovist : isovists) {
                  comm->CommPostMessage(Communicator::CURRENT_STEP, isovistIdx);
                  pDoc->m_meta_graph->makeIsovist(comm, isovist.getLocation(), isovist.getLeftAngle(),
                                                  isovist.getRightAngle(), comm->simple_version);
                  isovistIdx++;
              }

              emit updateRequested(QGraphDoc::NEW_DATA);
              // Tell the sidebar about the new map:
              QApplication::postEvent(
                  pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void *)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
              emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);

          } catch (EntityParsing::EntityParseException &e) {
              emit showWarningMessage(tr("Warning"),
                                      tr("depthmapX was unable to parse the isovists test file. Error: %1")
                                          .arg(QString::fromStdString(e.what())));
          }
          break;
      }

      case CMSCommunicator::MAKEISOVISTPATH:
         // the graph is going to build this path from a selection in a data map:
         // a data map must be topmost with lines or polylines selected
         // linedata must be displayed as per usual
         ok = pDoc->m_meta_graph->makeIsovistPath( comm, comm->GetSeedAngle(), comm->simple_version );

         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEALLLINEMAP:
         ok = pDoc->m_meta_graph->makeAllLineMap( comm, comm->GetSeedPoint() );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADGRAPH));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEFEWESTLINEMAP:
         ok = pDoc->m_meta_graph->makeFewestLineMap( comm, comm->GetOption() );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_TABLE);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEDRAWING: {
         // option 1 is: 0 a data map, 1 an axial map
         ok = pDoc->m_meta_graph->convertToDrawing( comm, comm->GetString().toStdString(), (comm->GetOption(1) == 0));
         if (ok) {
			 emit updateRequested(QGraphDoc::UNDECLARED);
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         // Tell the sidebar about the new map:
         QApplication::postEvent(pMain, new QmyEvent((enum QEvent::Type)FOCUSGRAPH, (void*)pDoc, QGraphDoc::CONTROLS_LOADDRAWING));
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA );
         break;
      }
      case CMSCommunicator::MAKEUSERMAP:
         ok = pDoc->m_meta_graph->convertDrawingToAxial( comm, comm->GetString().toStdString());
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_TABLE);
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERMAPSHAPE:
         ok = pDoc->m_meta_graph->convertDataToAxial( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               emit updateRequested(QGraphDoc::DELETED_TABLE);
            }
            else {
               emit updateRequested(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No lines available in current layer to convert to axial lines"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERSEGMAP:
         ok = pDoc->m_meta_graph->convertDrawingToSegment( comm, comm->GetString().toStdString() );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_TABLE);
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::MAKEUSERSEGMAPSHAPE:
         ok = pDoc->m_meta_graph->convertDataToSegment( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1) );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               emit updateRequested(QGraphDoc::DELETED_TABLE);
            }
            else {
               emit updateRequested(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No lines available in current layer to convert to segments"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;
      
      case CMSCommunicator::MAKEGATESMAP: {
         // note: relies on the fact that make data map from drawing sets option 1 to -1, whereas make data layer from graph it to either 0 or 1
         int shapeMapType = ShapeMap::DRAWINGMAP;
         bool copydata = false;
         if(comm->GetOption(1) != -1) {
             shapeMapType = ShapeMap::DATAMAP;
             copydata = (comm->GetOption(1) != 0);
         }
         ok = pDoc->m_meta_graph->convertToData( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), shapeMapType, copydata );
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               emit updateRequested(QGraphDoc::DELETED_TABLE);
            }
            else {
               emit updateRequested(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No objects currently visible in drawing layers"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;
      }
      case CMSCommunicator::MAKECONVEXMAP: {
         // note: relies on the fact that make convex map from drawing sets option 1 to -1, whereas make convex map from data sets it to either 0 or 1
         int shapeMapType = ShapeMap::DRAWINGMAP;
         bool copydata = false;
         if(comm->GetOption(1) != -1) {
             shapeMapType = ShapeMap::DATAMAP;
             copydata = (comm->GetOption(1) != 0);
         }
         ok = pDoc->m_meta_graph->convertToConvex( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1),  shapeMapType, copydata);
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               emit updateRequested(QGraphDoc::DELETED_TABLE);
            }
            else {
               emit updateRequested(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No polygons currently visible in drawing layers"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;
      }
      case CMSCommunicator::MAKEBOUNDARYMAP:
         break;

      case CMSCommunicator::MAKESEGMENTMAP:
         // convert percentage to fraction, and pass to metagraph
         ok = pDoc->m_meta_graph->convertAxialToSegment( comm, comm->GetString().toStdString(), (comm->GetOption(0) == 1), (comm->GetOption(1) == 1), double(comm->GetOption(2)) / 100.0);
         if (ok) {
            if (comm->GetOption(0) == 0) {
               // note: there is both a new table and a deleted table, but deleted table leads to a greater redraw:
               emit updateRequested(QGraphDoc::DELETED_TABLE);
            }
            else {
               emit updateRequested(QGraphDoc::NEW_TABLE);
            }
         }
         else {
            Communicator::checkCancelled(comm);
            emit showWarningMessage(tr("Warning"), tr("No lines exist in map to convert to segments"));
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::AXIALANALYSIS:
         ok = pDoc->m_meta_graph->analyseAxial( comm, pMain->m_options, comm->simple_version );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::SEGMENTANALYSISTULIP:
         ok = pDoc->m_meta_graph->analyseSegmentsTulip( comm, pMain->m_options );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::SEGMENTANALYSISANGULAR:
         ok = pDoc->m_meta_graph->analyseSegmentsAngular( comm, pMain->m_options );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::TOPOMETANALYSIS:
         ok = pDoc->m_meta_graph->analyseTopoMet( comm, pMain->m_options );
         if (ok) {
            emit updateRequested(QGraphDoc::NEW_DATA);
         }
         emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
         break;

      case CMSCommunicator::POINTDEPTH:
         {
            // Set up for options step depth selection
            Options options;
            options.global = 0;
            options.point_depth_selection = 1;
            
            ok = pDoc->m_meta_graph->analyseGraph( comm, options, comm->simple_version);
            if (ok) {
               emit updateRequested(QGraphDoc::NEW_DATA);
            }
            emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
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
               emit updateRequested(QGraphDoc::NEW_DATA);
            }
            emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
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
               emit updateRequested(QGraphDoc::NEW_DATA);
            }
            emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
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
               emit updateRequested(QGraphDoc::NEW_DATA);
            }
            emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
         }
         break;

      case CMSCommunicator::AGENTANALYSIS:
         {
            try {
              pDoc->m_meta_graph->runAgentEngine( comm );
              emit updateRequested(QGraphDoc::NEW_TABLE);
              emit redrawRequested(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DATA );
            } catch (depthmapX::PointMapException const & e) {
              emit runtimeExceptionThrown(e.getErrorType(), e.what());
          }
         }
         break;
      case CMSCommunicator::FROMCONNECTOR:
        {
          comm->runAnalysis(*pDoc);
          break;
        }
      }
      if (ok <= 0 && comm->GetFunction() != CMSCommunicator::IMPORTMIF) {
         Communicator::checkCancelled(comm);
      }

   } catch (const Communicator::CancelledException &) {
      title = tr("Analysis cancelled");
      message = tr("Analysis cancelled.");
   } catch (const std::exception &error) {
      title = tr("Analysis failed");
      message = QString::fromUtf8(error.what());
      if (message.isEmpty()) {
         message = tr("An error occurred during analysis.");
      }
   } catch (...) {
      title = tr("Analysis failed");
      message = tr("An unknown error occurred during analysis.");
   }
   emit analysisCompleted(title, message);
}
