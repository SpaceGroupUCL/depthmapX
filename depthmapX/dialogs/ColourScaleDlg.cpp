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

#include "mainwindow.h"

CColourScaleDlg::CColourScaleDlg(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint);

    m_show_lines = false;
    m_show_fill = false;
    m_show_centroids = false;
    m_blue = tr("");
    m_red = tr("");
    m_color = -1;

    m_docked = false;

    m_viewDoc = NULL;

    m_red_brush.setColor(QColor(255, 128, 128)); // CreateSolidBrush(RGB(255,128,128));
    m_blue_brush.setColor(QColor(128, 128, 255));

    c_blue_slider_ctrl->setMinimum(0);
    c_blue_slider_ctrl->setMaximum(100);
    c_red_slider_ctrl->setMinimum(0);
    c_red_slider_ctrl->setMaximum(100);
    c_blue_slider_ctrl->setTickInterval(10);
    c_red_slider_ctrl->setTickInterval(10);

    // these are out of order...
    c_color_type->addItem(QString(tr("Equal Ranges (3-Colour)")));            // AXMANESQUE
    c_color_type->addItem(QString(tr("Equal Ranges (Blue-Red)")));            // BLUERED
    c_color_type->addItem(QString(tr("Equal Ranges (Purple-Orange)")));       // PURPLEORANGE
    c_color_type->addItem(QString(tr("depthmapX Classic")));                  // DEPTHMAPCLASSIC
    c_color_type->addItem(QString(tr("Equal Ranges (Greyscale)")));           // GREYSCALE
    c_color_type->addItem(QString(tr("Equal Ranges (Monochrome)")));          // MONOCHROME
    c_color_type->addItem(QString(tr("Equal Ranges (3-Colour Hue Only)")));   // HUEONLYAXMANESQUE

    m_color_type_map.push_back(DisplayParams::AXMANESQUE);
    m_color_type_map.push_back(DisplayParams::BLUERED);
    m_color_type_map.push_back(DisplayParams::PURPLEORANGE);
    m_color_type_map.push_back(DisplayParams::DEPTHMAPCLASSIC);
    m_color_type_map.push_back(DisplayParams::GREYSCALE);
    m_color_type_map.push_back(DisplayParams::MONOCHROME);
    m_color_type_map.push_back(DisplayParams::HUEONLYAXMANESQUE);

    Clear();

    UpdateData(false);
}

void CColourScaleDlg::OnChangeBlueValue() {
    QString str;
    str = c_blue_value_window->text();
    m_displayparams.blue = GetNormValue(str.toDouble());
    c_blue_slider_ctrl->setValue(m_displayparams.blue * 100.0);
    c_ok->setEnabled(true);
    c_applytoall->setEnabled(true);
}

void CColourScaleDlg::OnChangeRedValue() {
    QString str;
    str = c_red_value_window->text();
    m_displayparams.red = GetNormValue(str.toDouble());
    c_red_slider_ctrl->setValue(m_displayparams.red * 100);
    c_ok->setEnabled(true);
    c_applytoall->setEnabled(true);
}

void CColourScaleDlg::OnReleasedRedSlider(int val) {
    double value = double(c_red_slider_ctrl->value()) / 100.0;
    QString text;
    text.sprintf("%.2f", GetActualValue(value));
    c_red_value_window->setText(text);
}

void CColourScaleDlg::OnReleasedBlueSlider(int val) {
    double value = double(c_blue_slider_ctrl->value()) / 100.0;
    QString text;
    text.sprintf("%.2f", GetActualValue(value));
    c_blue_value_window->setText(text);
}

void CColourScaleDlg::OnSelchangeColor(int value) {
    UpdateData(true);

    if (m_color_type_map.size() <= (size_t)value)
        return;

    m_color = m_color_type_map[value];

    if (m_color == 0 || m_color == 3 || m_color == 5 || m_color == 6) {
        m_red = "Red";
        m_blue = "Blue";
    } else if (m_color == 4) {
        m_red = "Orange";
        m_blue = "Purple";
    } else if (m_color == 1) {
        m_red = "White";
        m_blue = "Black";
    } else {
        m_red = "Thick";
        m_blue = "Thin";
    }

    UpdateData(false);

    c_ok->setEnabled(true);
    c_applytoall->setEnabled(true);
}

void CColourScaleDlg::OnBnClickedShowLines(bool value) { c_ok->setEnabled(true); }

void CColourScaleDlg::OnBnClickedShowFill(bool value) { c_ok->setEnabled(true); }

void CColourScaleDlg::OnBnClickedShowCentroids(bool value) { c_ok->setEnabled(true); }

void CColourScaleDlg::OnBnClickedApplytoall() {
    MyUpdateData(true, true);

    // don't destroy
    c_ok->setEnabled(false);
    c_applytoall->setEnabled(false);
}

void CColourScaleDlg::OnOK() {
    MyUpdateData(true, false);

    // don't destroy
    c_ok->setEnabled(false);
}

void CColourScaleDlg::OnCancel() {
    // don't destroy, simply hide:
    hide();
}

void CColourScaleDlg::UpdateData(bool value) {
    if (value) {
        m_blue = c_blue->text();
        m_red = c_red->text();
        if (c_show_lines->checkState())
            m_show_lines = true;
        else
            m_show_lines = false;

        if (c_show_fill->checkState())
            m_show_fill = true;
        else
            m_show_fill = false;

        if (c_show_centroids->checkState())
            m_show_centroids = true;
        else
            m_show_centroids = false;
    } else // push data to controls:
    {
        c_blue->setText(m_blue);
        c_red->setText(m_red);
        if (m_show_lines)
            c_show_lines->setCheckState(Qt::Checked);
        else
            c_show_lines->setCheckState(Qt::Unchecked);

        if (m_show_fill)
            c_show_fill->setCheckState(Qt::Checked);
        else
            c_show_fill->setCheckState(Qt::Unchecked);

        if (m_show_centroids)
            c_show_centroids->setCheckState(Qt::Checked);
        else
            c_show_centroids->setCheckState(Qt::Unchecked);
    }
}

void CColourScaleDlg::Clear() {
    m_color = -1;
    c_color_type->setEnabled(false);
    m_blue = "Min";
    m_red = "Max";
    c_blue_value_window->setEnabled(false);
    c_blue_value_window->setText(tr(""));
    c_red_value_window->setEnabled(false);
    c_red_value_window->setText(tr(""));
    c_blue_slider_ctrl->setEnabled(false);
    c_blue_slider_ctrl->setValue(0);
    c_red_slider_ctrl->setEnabled(false);
    c_red_slider_ctrl->setValue(100);
    c_ok->setEnabled(false);
    c_applytoall->setEnabled(false);

    UpdateData(false);
}

double CColourScaleDlg::GetActualValue(double sliderpos) {
    return sliderpos * (m_display_max - m_display_min) + m_display_min;
}

float CColourScaleDlg::GetNormValue(double actualval) {
    return ((actualval - m_display_min) / (m_display_max - m_display_min));
}

void CColourScaleDlg::Fill() {
    if (m_color == 0 || m_color == 3 || m_color == 5) {
        m_red = "Red";
        m_blue = "Blue";
    } else if (m_color == 4) {
        m_red = "Orange";
        m_blue = "Purple";
    } else if (m_color == 1) {
        m_red = "White";
        m_blue = "Black";
    } else {
        m_red = "Thick";
        m_blue = "Thin";
    }

    QString text;
    text.sprintf("%.2f", GetActualValue(m_displayparams.blue));
    c_blue_value_window->setText(text);
    text.sprintf("%.2f", GetActualValue(m_displayparams.red));
    c_red_value_window->setText(text);

    c_blue_slider_ctrl->setValue(int(m_displayparams.blue * 100));
    c_red_slider_ctrl->setValue(int(m_displayparams.red * 100));

    c_color_type->setEnabled(true);
    c_blue_value_window->setEnabled(true);
    c_red_value_window->setEnabled(true);
    c_blue_slider_ctrl->setEnabled(true);
    c_red_slider_ctrl->setEnabled(true);
    c_ok->setEnabled(false);
    c_applytoall->setEnabled(false);

    UpdateData(false);
}

void CColourScaleDlg::OnFocusGraph(QGraphDoc *pDoc, int lParam) {
    if (lParam == QGraphDoc::CONTROLS_DESTROYALL && pDoc == m_viewDoc) { // Lost graph
        m_viewDoc = NULL;
        MyUpdateData(false, false);
    } else if (lParam == QGraphDoc::CONTROLS_LOADALL &&
               pDoc != m_viewDoc) { // [Possible] change of window (sent on focus)
        m_viewDoc = pDoc;
        MyUpdateData(false, false);
    } else if (lParam != QGraphDoc::CONTROLS_LOADALL && pDoc == m_viewDoc) { // Force update if match current window
        MyUpdateData(false, false);
    }
}

void CColourScaleDlg::MyUpdateData(bool dir, bool apply_to_all) {
    if (dir == false) {
        // push data to controls:
        if (m_viewDoc == NULL) {
            Clear();
        } else {
            MetaGraph *graph = m_viewDoc->m_meta_graph;
            if (graph->viewingProcessed()) {
                if (graph->getViewClass() & MetaGraph::VIEWVGA) {
                    PointMap &map = graph->getDisplayedPointMap();
                    m_display_min = map.getDisplayMinValue();
                    m_display_max = map.getDisplayMaxValue();
                    m_displayparams = map.getDisplayParams();
                    m_color = m_displayparams.colorscale;
                } else if (graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                    ShapeGraph &map = graph->getDisplayedShapeGraph();
                    m_display_min = map.getDisplayMinValue();
                    m_display_max = map.getDisplayMaxValue();
                    m_displayparams = map.getDisplayParams();
                    m_color = m_displayparams.colorscale;
                    bool show_lines = m_show_lines, show_fill = m_show_fill, show_centroids = m_show_centroids;
                    map.getPolygonDisplay(show_lines, show_fill, show_centroids);
                    m_show_lines = show_lines;
                    m_show_fill = show_fill;
                    m_show_centroids = show_centroids;
                } else if (graph->getViewClass() & MetaGraph::VIEWDATA) {
                    ShapeMap &map = graph->getDisplayedDataMap();
                    m_display_min = map.getDisplayMinValue();
                    m_display_max = map.getDisplayMaxValue();
                    m_displayparams = map.getDisplayParams();
                    m_color = m_displayparams.colorscale;
                    bool show_lines = m_show_lines, show_fill = m_show_fill, show_centroids = m_show_centroids;
                    map.getPolygonDisplay(show_lines, show_fill, show_centroids);
                    m_show_lines = show_lines;
                    m_show_fill = show_fill;
                    m_show_centroids = show_centroids;
                } else if (graph->getViewClass() & MetaGraph::VIEWTRACES) {
                    TraceMap &map = graph->getDisplayedTraceMap();
                    m_display_min = map.getDisplayMinValue();
                    m_display_max = map.getDisplayMaxValue();
                    m_displayparams = map.getDisplayParams();
                    m_color = m_displayparams.colorscale;
                    bool show_lines = m_show_lines, show_fill = m_show_fill, show_centroids = m_show_centroids;
                    map.getPolygonDisplay(show_lines, show_fill, show_centroids);
                    m_show_lines = show_lines;
                    m_show_fill = show_fill;
                    m_show_centroids = show_centroids;
                }
                for (size_t i = 0; i < m_color_type_map.size(); i++) {
                    if (m_color == m_color_type_map[i]) {
                        c_color_type->setCurrentIndex(i);
                    }
                }
                Fill();
            } else {
                Clear();
            }
        }
        UpdateData(false);
    } else {
        // get data from controls:
        UpdateData(true);

        if (m_viewDoc != NULL) {
            MetaGraph *graph = m_viewDoc->m_meta_graph;
            m_color = m_color_type_map[c_color_type->currentIndex()];
            m_displayparams.colorscale = m_color;
            if (graph->getViewClass() & MetaGraph::VIEWVGA) {
                graph->getDisplayedPointMap().setDisplayParams(m_displayparams, apply_to_all);
            } else if (graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                graph->getDisplayedShapeGraph().setDisplayParams(m_displayparams, apply_to_all);
                graph->getDisplayedShapeGraph().setPolygonDisplay(m_show_lines, m_show_fill, m_show_centroids);
            } else if (graph->getViewClass() & MetaGraph::VIEWDATA) {
                graph->getDisplayedDataMap().setDisplayParams(m_displayparams, apply_to_all);
                graph->getDisplayedDataMap().setPolygonDisplay(m_show_lines, m_show_fill, m_show_centroids);
            } else if (graph->getViewClass() & MetaGraph::VIEWTRACES) {
                graph->getDisplayedTraceMap().setDisplayParams(m_displayparams, apply_to_all);
                graph->getDisplayedTraceMap().setPolygonDisplay(m_show_lines, m_show_fill, m_show_centroids);
            }
        }
        m_viewDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP);
    }
}

void CColourScaleDlg::showEvent(QShowEvent *event) {
    // UpdateData(false);
}
