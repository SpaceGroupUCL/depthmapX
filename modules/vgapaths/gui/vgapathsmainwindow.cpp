// Copyright (C) 2020 Petros Koutsolampros

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

#include "vgapathsmainwindow.h"

#include "modules/vgapaths/core/extractlinkdata.h"
#include "modules/vgapaths/core/vgaangularshortestpath.h"
#include "modules/vgapaths/core/vgaisovistzone.h"
#include "modules/vgapaths/core/vgametricdepthlinkcost.h"
#include "modules/vgapaths/core/vgametricshortestpath.h"
#include "modules/vgapaths/core/vgametricshortestpathtomany.h"
#include "modules/vgapaths/core/vgavisualshortestpath.h"

#include "depthmapX/mainwindowhelpers.h"

#include <QMenuBar>
#include <QMessageBox>

bool VGAPathsMainWindow::createMenus(MainWindow *mainWindow) {
    QMenu *toolsMenu = MainWindowHelpers::getOrAddRootMenu(mainWindow, tr("&Tools"));
    QMenu *visibilitySubMenu = MainWindowHelpers::getOrAddMenu(toolsMenu, tr("&Visibility"));
    QMenu *shortestPathSubMenu = MainWindowHelpers::getOrAddMenu(visibilitySubMenu, tr("Shortest Paths"));

    QAction *visibilityShortestPathAct = new QAction(tr("Visibility Shortest Path"), this);
    visibilityShortestPathAct->setStatusTip(tr("Shortest visual path between two selected points"));
    connect(visibilityShortestPathAct, &QAction::triggered, this,
            [this, mainWindow] { OnShortestPath(mainWindow, PathType::VISUAL); });
    shortestPathSubMenu->addAction(visibilityShortestPathAct);

    QAction *metricShortestPathAct = new QAction(tr("&Metric Shortest Path"), this);
    metricShortestPathAct->setStatusTip(tr("Shortest metric path between two selected points"));
    connect(metricShortestPathAct, &QAction::triggered, this,
            [this, mainWindow] { OnShortestPath(mainWindow, PathType::METRIC); });
    shortestPathSubMenu->addAction(metricShortestPathAct);

    QAction *angularShortestPathAct = new QAction(tr("&Angular Shortest Path"), this);
    angularShortestPathAct->setStatusTip(tr("Shortest angular path between two selected points"));
    connect(angularShortestPathAct, &QAction::triggered, this,
            [this, mainWindow] { OnShortestPath(mainWindow, PathType::ANGULAR); });
    shortestPathSubMenu->addAction(angularShortestPathAct);

    QAction *extractLinkDataAct = new QAction(tr("&Extract Link Data"), this);
    extractLinkDataAct->setStatusTip(tr("Extracts data from the links and adds them to the attribute table"));
    connect(extractLinkDataAct, &QAction::triggered, this, [this, mainWindow] { OnExtractLinkData(mainWindow); });
    visibilitySubMenu->addAction(extractLinkDataAct);

    return true;
}

void VGAPathsMainWindow::OnShortestPath(MainWindow *mainWindow, PathType pathType) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a VGA map"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    PointMap &pointMap = graphDoc->m_meta_graph->getDisplayedPointMap();
    if (pointMap.getSelSet().size() != 2) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please select two cells to create a path between"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    const PixelRef &pixelFrom = *pointMap.getSelSet().begin();
    const PixelRef &pixelTo = *std::next(pointMap.getSelSet().begin());

    graphDoc->m_communicator = new CMSCommunicator();
    switch (pathType) {
    case PathType::VISUAL:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAVisualShortestPath(pointMap, pixelFrom, pixelTo)));
        break;
    case PathType::METRIC: {
        std::set<PixelRef> pixelsFrom;
        pixelsFrom.insert(pixelFrom);
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAMetricShortestPath(pointMap, pixelsFrom, pixelTo)));
        break;
    }
    case PathType::ANGULAR:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAAngularShortestPath(pointMap, pixelFrom, pixelTo)));
        break;
    }
    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Calculating shortest path..."));
    graphDoc->m_thread.render(graphDoc);
}

void VGAPathsMainWindow::OnExtractLinkData(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a VGA map"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    graphDoc->m_communicator->setAnalysis(
        std::unique_ptr<IAnalysis>(new ExtractLinkData(graphDoc->m_meta_graph->getDisplayedPointMap())));

    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Extracting link data.."));
    graphDoc->m_thread.render(graphDoc);
}

void VGAPathsMainWindow::OnMakeIsovistZones(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a VGA map"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    std::map<std::string, std::set<PixelRef>> originPointSets;
    float restrictDistance = -1;

    graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
        new VGAIsovistZone(graphDoc->m_meta_graph->getDisplayedPointMap(), originPointSets, restrictDistance)));

    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Making isovist zones.."));
    graphDoc->m_thread.render(graphDoc);
}

void VGAPathsMainWindow::OnMetricShortestPathsToMany(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a VGA map"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    std::set<PixelRef> pixelsFrom;
    std::set<PixelRef> pixelsTo;

    graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
        new VGAMetricShortestPathToMany(graphDoc->m_meta_graph->getDisplayedPointMap(), pixelsFrom, pixelsTo)));

    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Making isovist zones.."));
    graphDoc->m_thread.render(graphDoc);
}
