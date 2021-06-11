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

#include "vgamainwindow.h"

#include "modules/vga/core/vgaangular.h"
#include "modules/vga/core/vgaangulardepth.h"
#include "modules/vga/core/vgaisovist.h"
#include "modules/vga/core/vgametric.h"
#include "modules/vga/core/vgametricdepth.h"
#include "modules/vga/core/vgathroughvision.h"
#include "modules/vga/core/vgavisualglobal.h"
#include "modules/vga/core/vgavisualglobaldepth.h"
#include "modules/vga/core/vgavisuallocal.h"

#include "vgaoptionsdlg.h"

#include "depthmapX/mainwindowhelpers.h"

#include <QMenuBar>
#include <QMessageBox>

bool VGAMainWindow::createMenus(MainWindow *mainWindow) {
    QMenu *toolsMenu = MainWindowHelpers::getOrAddRootMenu(mainWindow, tr("&Tools"));
    QMenu *visibilitySubMenu = MainWindowHelpers::getOrAddMenu(toolsMenu, tr("&Visibility"));
    connect(visibilitySubMenu, &QMenu::aboutToShow, this,
            [this, mainWindow] { OnAboutToShowVisibilityMenu(mainWindow); });

    m_vgaAct = new QAction(tr("&Run Visibility Graph Analysis..."), mainWindow);
    m_vgaAct->setStatusTip(tr("Angular distance from current selection\nAngular Depth"));
    connect(m_vgaAct, &QAction::triggered, this, [this, mainWindow] { OnVGA(mainWindow); });
    visibilitySubMenu->addAction(m_vgaAct);

    QMenu *stepMenu = MainWindowHelpers::getOrAddMenu(visibilitySubMenu, tr("Step &Depth"));
    connect(stepMenu, &QMenu::aboutToShow, this, [this, mainWindow] { OnAboutToShowStepDepthMenu(mainWindow); });

    m_angularStepAct = new QAction(tr("&Angular Step"), mainWindow);
    m_angularStepAct->setStatusTip(tr("Angular distance from current selection\nAngular Depth"));
    connect(m_angularStepAct, &QAction::triggered, this,
            [this, mainWindow] { OnVGAStep(mainWindow, StepType::ANGULAR); });
    stepMenu->addAction(m_angularStepAct);

    m_metricStepAct = new QAction(tr("&Metric Step"), mainWindow);
    m_metricStepAct->setStatusTip(tr("Distance from current selection\nMetric Depth"));
    connect(m_metricStepAct, &QAction::triggered, this,
            [this, mainWindow] { OnVGAStep(mainWindow, StepType::METRIC); });
    stepMenu->addAction(m_metricStepAct);

    m_visualStepAct = new QAction(tr("&Visibility Step"), mainWindow);
    m_visualStepAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
    connect(m_visualStepAct, &QAction::triggered, this,
            [this, mainWindow] { OnVGAStep(mainWindow, StepType::VISUAL); });
    stepMenu->addAction(m_visualStepAct);

    return true;
}

void VGAMainWindow::OnVGA(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a pointmap"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    VGAOptionsDlg dlg;

    dlg.m_layer_names.push_back("<None>");
    for (auto &dataMap : graphDoc->m_meta_graph->getDataMaps()) {
        dlg.m_layer_names.push_back(dataMap.getName());
    }

    if (QDialog::Accepted != dlg.exec()) {
        return;
    }

    graphDoc->m_communicator = new CMSCommunicator();

    if (dlg.m_output_type == Options::OUTPUT_ISOVIST) {

        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAIsovist(graphDoc->m_meta_graph->getDisplayedPointMap(), graphDoc->m_communicator->simple_version)));
    } else if (dlg.m_output_type == Options::OUTPUT_VISUAL) {
        if (dlg.m_local) {

            graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
                new VGAVisualLocal(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_gates_only,
                                   graphDoc->m_communicator->simple_version)));
        }
        if (dlg.m_global) {
            graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
                new VGAVisualGlobal(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_numericRadius,
                                    dlg.m_gates_only, graphDoc->m_communicator->simple_version)));
        }
    } else if (dlg.m_output_type == Options::OUTPUT_METRIC) {
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAMetric(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_numericRadius, dlg.m_gates_only)));
    } else if (dlg.m_output_type == Options::OUTPUT_ANGULAR) {
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAAngular(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_numericRadius, dlg.m_gates_only)));

    } else if (dlg.m_output_type == Options::OUTPUT_THRU_VISION) {
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAThroughVision(graphDoc->m_meta_graph->getDisplayedPointMap())));
    }

    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Analysing graph..."));
    graphDoc->m_thread.render(graphDoc);
}

void VGAMainWindow::OnVGAStep(MainWindow *mainWindow, StepType stepType) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a pointmap"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    graphDoc->m_communicator = new CMSCommunicator();
    switch (stepType) {
    case StepType::ANGULAR:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAAngularDepth(graphDoc->m_meta_graph->getDisplayedPointMap())));
        break;
    case StepType::METRIC:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAMetricDepth(graphDoc->m_meta_graph->getDisplayedPointMap())));
        break;
    case StepType::VISUAL:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAVisualGlobalDepth(graphDoc->m_meta_graph->getDisplayedPointMap())));
        break;
    }
    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS,
                                                    QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Calculating step..."));
    graphDoc->m_thread.render(graphDoc);
}

void VGAMainWindow::OnAboutToShowVisibilityMenu(MainWindow *mainWindow) {

    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (!graphDoc) {
        m_vgaAct->setEnabled(false);
        return;
    }

    if (graphDoc->m_meta_graph->viewingProcessedPoints()) {
        m_vgaAct->setEnabled(true);
    } else {
        m_vgaAct->setEnabled(false);
    }
}

void VGAMainWindow::OnAboutToShowStepDepthMenu(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (!graphDoc) {
        m_visualStepAct->setEnabled(false);
        m_metricStepAct->setEnabled(false);
        m_angularStepAct->setEnabled(false);
        return;
    }

    if (graphDoc->m_meta_graph->viewingProcessedPoints() && graphDoc->m_meta_graph->isSelected()) {
        m_visualStepAct->setEnabled(true);
        m_metricStepAct->setEnabled(true);
        m_angularStepAct->setEnabled(true);
    } else {
        m_visualStepAct->setEnabled(false);
        m_metricStepAct->setEnabled(false);
        m_angularStepAct->setEnabled(false);
    }
}
