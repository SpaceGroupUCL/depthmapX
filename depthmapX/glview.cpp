// depthmapX - spatial network analysis platform
// Copyright (C) 2017, Petros Koutsolampros

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


#include "depthmapX/glview.h"
#include "depthmapX/depthmapView.h"
#include "salalib/linkutils.h"
#include "salalib/geometrygenerators.h"
#include "mainwindow.h"
#include <QMouseEvent>
#include <QCoreApplication>

static QRgb colorMerge(QRgb color, QRgb mergecolor)
{
   return (color & 0x006f6f6f) | (mergecolor & 0x00a0a0a0);
}

GLView::GLView(QGraphDoc &pDoc,
               Settings &settings,
               QWidget *parent)
    : MapView(pDoc, settings, parent),
      m_eyePosX(0),
      m_eyePosY(0)
{
    m_core = QCoreApplication::arguments().contains(QStringLiteral("--coreprofile"));

    m_foreground = settings.readSetting(SettingTag::foregroundColour, qRgb(128,255,128)).toInt();
    m_background = settings.readSetting(SettingTag::backgroundColour, qRgb(0,0,0)).toInt();
    m_initialSize = m_settings.readSetting(SettingTag::depthmapViewSize, QSize(2000, 2000)).toSize();
    m_antialiasingSamples = settings.readSetting(SettingTag::antialiasingSamples, 0).toInt();

    if(m_antialiasingSamples) {
        QSurfaceFormat format;
        format.setSamples(m_antialiasingSamples);    // Set the number of samples used for multisampling
        setFormat(format);
    }

    loadDrawingGLObjects();

    loadAxes();

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedShapeGraph());
    }
    m_visiblePointMap.setGridColour(colorMerge(m_foreground, m_background));
    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedPointMap());
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
        m_visibleDataMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedDataMap());
    }

    m_dragLine.setStrokeColour(m_foreground);
    m_selectionRect.setStrokeColour(m_background);

    matchViewToCurrentMetaGraph();

    installEventFilter(this);
    setMouseTracking(true);
    m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = this;
}

GLView::~GLView()
{
    makeCurrent();
    m_selectionRect.cleanup();
    m_dragLine.cleanup();
    m_axes.cleanup();
    m_visibleDrawingLines.cleanup();
    m_visiblePointMap.cleanup();
    m_visibleShapeGraph.cleanup();
    m_visibleDataMap.cleanup();
    doneCurrent();
    m_settings.writeSetting(SettingTag::depthmapViewSize, size());
}

QSize GLView::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLView::sizeHint() const
{
    return m_initialSize;
}

void GLView::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(qRed(m_background)/255.0f, qGreen(m_background)/255.0f, qBlue(m_background)/255.0f, 1);

    m_selectionRect.initializeGL(m_core);
    m_dragLine.initializeGL(m_core);
    m_axes.initializeGL(m_core);
    m_visibleDrawingLines.initializeGL(m_core);
    m_visiblePointMap.initializeGL(m_core);
    m_visibleShapeGraph.initializeGL(m_core);
    m_visibleDataMap.initializeGL(m_core);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
         m_visiblePointMap.loadGLObjectsRequiringGLContext(m_pDoc.m_meta_graph->getDisplayedPointMap());
    }

    m_mModel.setToIdentity();

    m_mView.setToIdentity();
    m_mView.translate(0, 0, -1);
}

void GLView::paintGL()
{
    glEnable(GL_MULTISAMPLE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    if(m_datasetChanged) {

        loadDrawingGLObjects();
        m_visibleDrawingLines.updateGL(m_core);

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
            m_visibleShapeGraph.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedShapeGraph());
            m_visibleShapeGraph.updateGL(m_core);
        }

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
            m_visibleDataMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedDataMap());
            m_visibleDataMap.updateGL(m_core);
        }

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
            m_visiblePointMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedPointMap());
            m_visiblePointMap.updateGL(m_core);
            m_visiblePointMap.loadGLObjectsRequiringGLContext(m_pDoc.m_meta_graph->getDisplayedPointMap());
        }

        m_datasetChanged = false;
    }

    m_axes.paintGL(m_mProj, m_mView, m_mModel);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.showGrid(m_pDoc.m_meta_graph->m_showgrid);
        m_visiblePointMap.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
        m_visibleDataMap.paintGL(m_mProj, m_mView, m_mModel);
    }

    m_visibleDrawingLines.paintGL(m_mProj, m_mView, m_mModel);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.paintGLOverlay(m_mProj, m_mView, m_mModel);
    }
    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.paintGLOverlay(m_mProj, m_mView, m_mModel);
    }


    float pos [] = {
        float(min(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x())),
        float(min(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y())),
        float(max(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x())),
        float(max(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y()))
    };
    m_selectionRect.paintGL(m_mProj, m_mView, m_mModel, QMatrix2x2(pos));

    if((m_mouseMode & MOUSE_MODE_SECOND_POINT) == MOUSE_MODE_SECOND_POINT) {
        float pos [] = {
            float(m_tempFirstPoint.x),
            float(m_tempFirstPoint.y),
            float(m_tempSecondPoint.x),
            float(m_tempSecondPoint.y)
        };
        m_dragLine.paintGL(m_mProj, m_mView, m_mModel, QMatrix2x2(pos));
    }
}

void GLView::loadAxes() {
    std::vector<std::pair<SimpleLine, PafColor>> axesData;
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,1,0), PafColor(1,0,0)));
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,0,1), PafColor(0,1,0)));
    m_axes.loadLineData(axesData);
}

void GLView::loadDrawingGLObjects() {
    auto lock = m_pDoc.m_meta_graph->getLock();
    m_visibleDrawingLines.loadLineData(m_pDoc.m_meta_graph->getVisibleDrawingLines(), m_foreground);
}

void GLView::resizeGL(int w, int h)
{
    m_screenWidth = w;
    m_screenHeight = h;
    m_screenRatio = GLfloat(w) / h;
    recalcView();
}

void GLView::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_wasPanning) {
        m_wasPanning = false;
        return;
    }
    QPoint mousePoint = event->pos();
    Point2f worldPoint = getWorldPoint(mousePoint);
    if (!m_pDoc.m_communicator) {
        QtRegion r;
        if(m_mouseDragRect.isNull())
        {
            r.bottom_left = worldPoint;
            r.top_right = worldPoint;
        }
        else
        {
            r.bottom_left.x = min(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x());
            r.bottom_left.y = min(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y());
            r.top_right.x = max(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x());
            r.top_right.y = max(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y());
        }
        bool selected = false;
        switch(m_mouseMode)
        {
        case MOUSE_MODE_NONE:
        {
            // nothing, deselect
            m_pDoc.m_meta_graph->clearSel();
            break;
        }
        case MOUSE_MODE_SELECT:
        {
            // typical selection
            Qt::KeyboardModifiers keyMods = QApplication::keyboardModifiers();
            m_pDoc.m_meta_graph->setCurSel( r, keyMods & Qt::ShiftModifier );
            ((MainWindow *) m_pDoc.m_mainFrame)->updateToolbar();
            break;
        }
        case MOUSE_MODE_ZOOM_IN:
        {
            if(r.width() > 0)
            {
                OnViewZoomToRegion(r);
                recalcView();
            }
            else
            {
                zoomBy(0.8, mousePoint.x(), mousePoint.y());
            }
            break;
        }
        case MOUSE_MODE_ZOOM_OUT:
        {
            zoomBy(1.2, mousePoint.x(), mousePoint.y());
            break;
        }
        case MOUSE_MODE_FILL_FULL:
        {
            m_pDoc.OnFillPoints( worldPoint, 0 );
            break;
        }
        case MOUSE_MODE_PENCIL:
        {
            m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(worldPoint,true);
            break;
        }
        case MOUSE_MODE_SEED_ISOVIST:
        {
            m_pDoc.OnMakeIsovist( worldPoint );
            break;
        }
        case MOUSE_MODE_SEED_TARGETED_ISOVIST:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_SEED_TARGETED_ISOVIST | MOUSE_MODE_SECOND_POINT:
        {
            Line directionLine(m_tempFirstPoint,worldPoint);
            Point2f vec = directionLine.vector();
            vec.normalise();
            m_pDoc.OnMakeIsovist( m_tempFirstPoint, vec.angle() );
            m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST;
            break;
        }
        case MOUSE_MODE_SEED_AXIAL:
        {
            m_pDoc.OnToolsAxialMap( worldPoint );
            break;
        }
        case MOUSE_MODE_LINE_TOOL:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_LINE_TOOL | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_LINE_TOOL | MOUSE_MODE_SECOND_POINT:
        {
            if (m_pDoc.m_meta_graph->makeShape(Line(m_tempFirstPoint,worldPoint))) {
               m_pDoc.modifiedFlag = true;
               m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
            }
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_LINE_TOOL;

            break;
        }
        case MOUSE_MODE_POLYGON_TOOL:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_polyPoints = 0;
            m_mouseMode = MOUSE_MODE_POLYGON_TOOL | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_POLYGON_TOOL | MOUSE_MODE_SECOND_POINT:
        {
            if (m_polyPoints == 0) {
               m_pDoc.m_meta_graph->polyBegin(Line(m_tempFirstPoint,worldPoint));
               m_polyStart = m_tempFirstPoint;
               m_tempFirstPoint = m_tempSecondPoint;
               m_polyPoints += 2;
            }
            else if (m_polyPoints > 2 && PixelDist(mousePoint, getScreenPoint(m_polyStart)) < 6) {
               // check to see if it's back to the original start point, if so, close off
               m_pDoc.m_meta_graph->polyClose();
               m_polyPoints = 0;
               m_mouseMode = MOUSE_MODE_POLYGON_TOOL;
            }
            else {
               m_pDoc.m_meta_graph->polyAppend(worldPoint);
               m_tempFirstPoint = m_tempSecondPoint;
               m_polyPoints += 1;
            }
            break;
        }
        case MOUSE_MODE_POINT_STEP_DEPTH:
        {
            m_pDoc.m_meta_graph->setCurSel( r, false );
            m_pDoc.OnToolsPD();
            break;
        }
        case MOUSE_MODE_JOIN:
        {
            selected = m_pDoc.m_meta_graph->setCurSel( r, false );
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if(selectedCount > 0) {
                Point2f selectionCentre;
                if(selectedCount > 1) {
                    QtRegion selBounds = m_pDoc.m_meta_graph->getSelBounds();
                    selectionCentre.x = (selBounds.bottom_left.x + selBounds.top_right.x)*0.5;
                    selectionCentre.y = (selBounds.bottom_left.y + selBounds.top_right.y)*0.5;
                } else {
                    const std::set<int> &selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                        selectionCentre = m_pDoc.m_meta_graph->getDisplayedPointMap().depixelate(*selectedSet.begin());
                    } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                        selectionCentre = m_pDoc.m_meta_graph->getDisplayedShapeGraph().getAllShapes().find(*selectedSet.begin())->second.getCentroid();
                    }
                }
                m_tempFirstPoint = selectionCentre;
                m_tempSecondPoint = selectionCentre;
                m_mouseMode = MOUSE_MODE_JOIN | MOUSE_MODE_SECOND_POINT;
            }
            break;
        }
        case MOUSE_MODE_JOIN | MOUSE_MODE_SECOND_POINT:
        {
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if (selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                    m_pDoc.m_meta_graph->getDisplayedPointMap().mergePoints( worldPoint );
                } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL && selectedCount == 1) {
                    m_pDoc.m_meta_graph->setCurSel( r, true ); // add the new one to the selection set
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (selectedSet.size() == 2) {
                        std::set<int>::iterator it = selectedSet.begin();
                        int axRef1 = *it;
                        it++;
                        int axRef2 = *it;
                        // axial is only joined one-by-one
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.m_meta_graph->getDisplayedShapeGraph().linkShapes(axRef1, axRef2, true);
                        m_pDoc.m_meta_graph->clearSel();
                    }
                }
                m_pDoc.m_meta_graph->clearSel();
                m_mouseMode = MOUSE_MODE_JOIN;
            }
            break;
        }
        case MOUSE_MODE_UNJOIN:
        {
            m_pDoc.m_meta_graph->setCurSel( r, false );
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if(selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                    if (m_pDoc.m_meta_graph->getDisplayedPointMap().unmergePoints()) {
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
                    }
                } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    Point2f selectionCentre = m_pDoc.m_meta_graph->getDisplayedShapeGraph().getAllShapes().find(*selectedSet.begin())->second.getCentroid();
                    m_tempFirstPoint = selectionCentre;
                    m_tempSecondPoint = selectionCentre;
                    m_mouseMode = MOUSE_MODE_UNJOIN | MOUSE_MODE_SECOND_POINT;
                }
            }
            break;
        }
        case MOUSE_MODE_UNJOIN | MOUSE_MODE_SECOND_POINT:
        {
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if (selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL && selectedCount == 1) {
                    m_pDoc.m_meta_graph->setCurSel( r, true ); // add the new one to the selection set
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (selectedSet.size() == 2) {
                        std::set<int>::iterator it = selectedSet.begin();
                        int axRef1 = *it;
                        it++;
                        int axRef2 = *it;
                        // axial is only joined one-by-one
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.m_meta_graph->getDisplayedShapeGraph().unlinkShapes(axRef1, axRef2, true);
                        m_pDoc.m_meta_graph->clearSel();
                    }
                }
                m_pDoc.m_meta_graph->clearSel();
                m_mouseMode = MOUSE_MODE_UNJOIN;
            }
            break;
        }
        }

        m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
    }
    m_mouseDragRect.setWidth(0);
    m_mouseDragRect.setHeight(0);
}

void GLView::mousePressEvent(QMouseEvent *event)
{
    m_mouseLastPos = event->pos();
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_mouseLastPos.x();
    int dy = event->y() - m_mouseLastPos.y();

    Point2f worldPoint = getWorldPoint(event->pos());

    if (event->buttons() & Qt::RightButton
            || (event->buttons() & Qt::LeftButton && m_mouseMode == MOUSE_MODE_PAN)) {
        panBy(dx, dy);
        m_wasPanning = true;
    } else if (event->buttons() & Qt::LeftButton) {
        Point2f lastWorldPoint = getWorldPoint(m_mouseLastPos);

        if(m_mouseDragRect.isNull()) {
            m_mouseDragRect.setX(lastWorldPoint.x);
            m_mouseDragRect.setY(lastWorldPoint.y);
        }
        m_mouseDragRect.setWidth(worldPoint.x - m_mouseDragRect.x());
        m_mouseDragRect.setHeight(worldPoint.y - m_mouseDragRect.y());
        update();
    }
    if((m_mouseMode & MOUSE_MODE_SECOND_POINT) == MOUSE_MODE_SECOND_POINT)
    {
        m_tempSecondPoint = worldPoint;
        update();
    }
    m_mouseLastPos = event->pos();
    m_pDoc.m_position = worldPoint;
    m_pDoc.UpdateMainframestatus();
}

void GLView::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;

    int x = event->x();
    int y = event->y();

    zoomBy(1 - 0.25f * numDegrees.y() / 15.0f, x, y);

    event->accept();
}

bool GLView::eventFilter(QObject *object, QEvent *e)
{
    if(e->type() == QEvent::ToolTip)
    {
        if (!m_pDoc.m_communicator)
        {
            if(m_pDoc.m_meta_graph)
            {
                if (m_pDoc.m_meta_graph->viewingProcessed() && m_pDoc.m_meta_graph->getSelCount() > 1) {
                    float val = m_pDoc.m_meta_graph->getSelAvg();
                    int count = m_pDoc.m_meta_graph->getSelCount();
                    if (val == -1.0f)
                        setToolTip("Null selection");
                    else if (val != -2.0f)
                        setToolTip(QString("Selection\nAverage: %1\nCount: %2").arg(val).arg(count));
                    else setToolTip("");
                }
                else if (m_pDoc.m_meta_graph->viewingProcessed()) {
                    // and that it has an appropriate state to display a hover wnd
                    QHelpEvent *helpEvent = static_cast<QHelpEvent*>(e); // Tool tip events come as the type QHelpEvent
                    float val = m_pDoc.m_meta_graph->getLocationValue(getWorldPoint(helpEvent->pos()));
                    if (val == -1.0f)
                        setToolTip("No value");
                    else if (val != -2.0f)
                        setToolTip(QString("%1").arg(val));
                    else setToolTip("");
                }
            }
        }
    }

    return QObject::eventFilter(object, e);
}

void GLView::zoomBy(float dzf, int mouseX, int mouseY)
{
    float pzf = m_zoomFactor;
    m_zoomFactor = m_zoomFactor * dzf;
    if(m_zoomFactor < m_minZoomFactor) m_zoomFactor = m_minZoomFactor;
    else if(m_zoomFactor > m_maxZoomFactor) m_zoomFactor = m_maxZoomFactor;
    m_eyePosX += (m_zoomFactor - pzf) * m_screenRatio * GLfloat(mouseX - m_screenWidth*0.5f) / GLfloat(m_screenWidth);
    m_eyePosY -= (m_zoomFactor - pzf) * GLfloat(mouseY - m_screenHeight*0.5f) / GLfloat(m_screenHeight);
    recalcView();
}
void GLView::panBy(int dx, int dy)
{
    m_eyePosX += m_zoomFactor * GLfloat(dx) / m_screenHeight;
    m_eyePosY -= m_zoomFactor * GLfloat(dy) / m_screenHeight;

    recalcView();
}
void GLView::recalcView()
{
    m_mProj.setToIdentity();

    if(m_perspectiveView)
    {
        m_mProj.perspective(45.0f, m_screenRatio, 0.01f, 100.0f);
        m_mProj.scale(1.0f, 1.0f, m_zoomFactor);
    }
    else
    {
        m_mProj.ortho(-m_zoomFactor * 0.5f * m_screenRatio, m_zoomFactor * 0.5f * m_screenRatio, -m_zoomFactor * 0.5f, m_zoomFactor * 0.5f, 0, 10);
    }
    m_mProj.translate(m_eyePosX, m_eyePosY, 0.0f);
    update();
}

Point2f GLView::getWorldPoint(const QPoint &screenPoint) {
    return Point2f(+ m_zoomFactor * float(screenPoint.x() - m_screenWidth*0.5)  / m_screenHeight - m_eyePosX,
                   - m_zoomFactor * float(screenPoint.y() - m_screenHeight*0.5) / m_screenHeight - m_eyePosY);
}

QPoint GLView::getScreenPoint(const Point2f &worldPoint) {
    return QPoint((worldPoint.x + m_eyePosX) * m_screenHeight / m_zoomFactor + m_screenWidth*0.5 ,
                   - (worldPoint.y + m_eyePosY) * m_screenHeight / m_zoomFactor + m_screenHeight*0.5);

}

void GLView::matchViewToCurrentMetaGraph() {
    const QtRegion &region = m_pDoc.m_meta_graph->getBoundingBox();
    OnViewZoomToRegion(region);
    recalcView();
}

void GLView::OnViewZoomToRegion(QtRegion region) {
    if((region.top_right.x == 0 && region.bottom_left.x == 0)
            || (region.top_right.y == 0 && region.bottom_left.y == 0))
        // region is unset, don't try to change the view to it
        return;
    m_eyePosX = - (region.top_right.x + region.bottom_left.x)*0.5f;
    m_eyePosY = - (region.top_right.y + region.bottom_left.y)*0.5f;
    if(region.width() > region.height())
    {
        m_zoomFactor = region.top_right.x - region.bottom_left.x;
    }
    else
    {
        m_zoomFactor = region.top_right.y - region.bottom_left.y;
    }
    m_minZoomFactor = m_zoomFactor * 0.001;
    m_maxZoomFactor = m_zoomFactor * 10;
}

void GLView::resetView() {
    m_visiblePointMap.showLinks(false);
    m_visibleShapeGraph.showLinks(false);
    m_pDoc.m_meta_graph->clearSel();
    update();
}

void GLView::OnModeJoin()
{
    if (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL)) {
        resetView();
        m_mouseMode = MOUSE_MODE_JOIN;
        m_visiblePointMap.showLinks(true);
        m_visibleShapeGraph.showLinks(true);
        m_pDoc.m_meta_graph->clearSel();
        notifyDatasetChanged();
    }
}

void GLView::OnModeUnjoin()
{
    if (m_pDoc.m_meta_graph->getState() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL)) {
        resetView();
        m_mouseMode = MOUSE_MODE_UNJOIN;
        m_visiblePointMap.showLinks(true);
        m_visibleShapeGraph.showLinks(true);
        m_pDoc.m_meta_graph->clearSel();
        notifyDatasetChanged();
    }
}
void GLView::OnViewPan()
{
    m_mouseMode = MOUSE_MODE_PAN;
}

void GLView::OnViewZoomIn()
{
    m_mouseMode = MOUSE_MODE_ZOOM_IN;
}

void GLView::OnViewZoomOut()
{
    m_mouseMode = MOUSE_MODE_ZOOM_OUT;
}

void GLView::OnEditFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_FULL;
}

void GLView::OnEditSemiFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_SEMI;
}

void GLView::OnEditAugmentFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_AUGMENT;
}

void GLView::OnEditPencil()
{
    resetView();
    m_mouseMode = MOUSE_MODE_PENCIL;
}

void GLView::OnModeIsovist()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_ISOVIST;
}

void GLView::OnModeTargetedIsovist()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST;
}

void GLView::OnModeSeedAxial()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_AXIAL;
}

void GLView::OnModeStepDepth()
{
    resetView();
    m_mouseMode = MOUSE_MODE_POINT_STEP_DEPTH;
}

void GLView::OnEditLineTool()
{
    resetView();
    m_mouseMode = MOUSE_MODE_LINE_TOOL;
}

void GLView::OnEditPolygonTool()
{
    resetView();
    m_mouseMode = MOUSE_MODE_POLYGON_TOOL;
}

void GLView::OnEditSelect()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SELECT;
}

void GLView::postLoadFile()
{
    matchViewToCurrentMetaGraph();
    setWindowTitle(m_pDoc.m_base_title+":Map View (GL)");
}

void GLView::OnViewZoomsel()
{
   if (m_pDoc.m_meta_graph && m_pDoc.m_meta_graph->isSelected()) {
      OnViewZoomToRegion(m_pDoc.m_meta_graph->getSelBounds());
   }
}

void GLView::closeEvent(QCloseEvent *event)
{
    m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = NULL;
    if (!m_pDoc.OnCloseDocument(QGraphDoc::VIEW_MAP_GL))
    {
        m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = this;
        event->ignore();
    }
}

void GLView::OnEditCopy()
{
    std::unique_ptr<QDepthmapView> tmp(new QDepthmapView(m_pDoc, m_settings));
    Point2f topLeftWorld = getWorldPoint(QPoint(0,0));
    Point2f bottomRightWorld = getWorldPoint(QPoint(width(),height()));

    tmp->setAttribute(Qt::WA_DontShowOnScreen);
    tmp->show();
    tmp->postLoadFile();
    tmp->OnViewZoomToRegion(QtRegion(topLeftWorld, bottomRightWorld));
    tmp->OnEditCopy();
}

void GLView::OnEditSave()
{
    std::unique_ptr<QDepthmapView> tmp(new QDepthmapView(m_pDoc, m_settings));
    Point2f topLeftWorld = getWorldPoint(QPoint(0,0));
    Point2f bottomRightWorld = getWorldPoint(QPoint(width(),height()));

    tmp->setAttribute(Qt::WA_DontShowOnScreen);
    tmp->show();
    tmp->postLoadFile();
    tmp->OnViewZoomToRegion(QtRegion(topLeftWorld, bottomRightWorld));
    tmp->OnEditSave();

}
