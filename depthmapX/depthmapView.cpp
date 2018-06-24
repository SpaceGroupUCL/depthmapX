// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017 Christian Sailer

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
#include <QToolBar>
#include <QEvent>
#include <QtWidgets/QMessageBox>

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qmenu.h>
#include <qactiongroup.h>
#include <qdebug.h>
#include <qsettings.h>

#include "mainwindow.h"
#include "depthmapView.h"
#include "viewhelpers.h"

class QToolBar;

#include "compatibilitydefines.h"

#define DMP_TIMER_SPLASH      1
#define DMP_TIMER_REDRAW      2
#define DMP_TIMER_HOVER       3
#define DMP_TIMER_3DVIEW      4


// version 5.0 dll exports have split out the attribute table
// functionality, so a single class operates with the VGA interface
// and the Axial interface, this means previous DLLs will not work
// with version 5.0 and above
// version 5.09 dll has reconfigured the data layers into shape layers:
// this means previous DLLs will not work with version 5.09 and above
// version 7.08 dll has a very slight change so that the analysis type
// is sent to the attribute table -- if it is vga it will work off pixelrefs,
// otherwise it will work off rowids
// version 7.09 dll has a new version number function so that "idepthmap.h"
// can be included multiple times in user projects
// version 8.00 dll has significant upgrades to the naming conventions,
// the ability to perform whole graph analyses and adding shapes
// version 10.04 dll has further upgrades to naming conventions,
// also, extra functions for the sala.dll build: includes graph opening, closing and so on

//QT_BEGIN_NAMESPACE
// Palette entries...
// A set of colours we would really like to have

static int pressed_nFlags;

inline Point2f QDepthmapView::LogicalUnits( const QPoint& p )
{
   return m_centre + Point2f(m_unit * double(p.x() - m_physical_centre.width()), 
                             m_unit * double(m_physical_centre.height() - p.y()));
}

inline QPoint QDepthmapView::PhysicalUnits( const Point2f& p )
{
   return QPoint( m_physical_centre.width() + int((p.x - m_centre.x) / m_unit + 0.4999), 
                  m_physical_centre.height() - int((p.y - m_centre.y) / m_unit + 0.4999) );
}

inline int PixelDist(QPoint a, QPoint b)
{
   return (int)sqrt(double((b.x()-a.x())*(b.x()-a.x())+(b.y()-a.y())*(b.y()-a.y())));
}

static QRgb colorMerge(QRgb color, QRgb mergecolor)
{
   return (color & 0x006f6f6f) | (mergecolor & 0x00a0a0a0);
}

QDepthmapView::QDepthmapView(QGraphDoc &pDoc, Settings &settings, QWidget *parent)
    : MapView(pDoc, settings, parent)
{
   m_drag_rect_a.setRect(0, 0, 0, 0);
   m_drag_rect_b.setRect(0, 0, 0, 0);

   // Several screen drawing booleans:
   m_continue_drawing = false;
   m_drawing = false;
   m_queued_redraw = false;

   m_viewport_set = false;
   m_clear = false;
   m_redraw = false;

   m_redraw_all = false;
   m_redraw_no_clear = false;

   m_resize_viewport = false;
   m_invalidate = false; // our own invalidation

   m_right_mouse_drag = false;
   m_alt_mode = false;

   m_current_mode = NONE;

   m_snap = false;
   m_repaint_tag = 0;
   m_showlinks = false;
   m_mouse_mode = SELECT;
   m_fillmode = FULLFILL;

   m_active_point_handle = -1;
   m_poly_points = 0;
   PafColor selcol(SALA_SELECTED_COLOR);

   m_selected_color = qRgb(selcol.redb(),selcol.greenb(),selcol.blueb());

   m_initialSize = m_settings.readSetting(SettingTag::depthmapViewSize, QSize(2000, 2000)).toSize();

   installEventFilter(this);

   setAttribute(Qt::WA_NoBackground, 1);
   setMouseTracking( 1 );
   setWindowIcon(QIcon(tr(":/images/cur/icon-1-4.png")));
}

QDepthmapView::~QDepthmapView()
{
    m_settings.writeSetting(SettingTag::depthmapViewSize, size());
}

int QDepthmapView::OnRedraw(int wParam, int lParam)
{
   if (m_pDoc.GetRemenuFlag(QGraphDoc::VIEW_MAP)) {
      m_pDoc.SetRemenuFlag(QGraphDoc::VIEW_MAP, false);
      // redo the menus for this *view* directly:
      //((CChildFrame*) GetParentFrame())->m_view_selector.RedoMenu( *m_pDoc.m_meta_graph );
   }
   if (m_pDoc.GetRedrawFlag(QGraphDoc::VIEW_MAP) != QGraphDoc::REDRAW_DONE) {
      if (!m_pDoc.m_communicator) {
         m_queued_redraw = false;
         switch (m_pDoc.GetRedrawFlag(QGraphDoc::VIEW_MAP)) {
            case QGraphDoc::REDRAW_POINTS:
               if (m_pDoc.m_meta_graph->viewingProcessedLines()) {
                  // Axial lines are thicker on selection, so background needs clearing
                  m_redraw_all = true;
               }
               else {
                  m_redraw_no_clear = true;
               }
               break;
            case QGraphDoc::REDRAW_GRAPH:
               m_redraw_all = true;
               break;
            case QGraphDoc::REDRAW_TOTAL:
               m_viewport_set = false;
               m_redraw_all = true;
               break;
         }
         m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP, QGraphDoc::REDRAW_DONE);
         repaint();
      }
      else {
         killTimer(Tid_redraw);
         startTimer(100);
         m_queued_redraw = true;
      }
   }
   return 0;
}

static QPoint hit_point;
bool QDepthmapView::eventFilter(QObject *object, QEvent *e)
{
    // Get the keyboard modifiers and set them in pressed_nFlags.
    switch ( e->type() ) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
            Qt::KeyboardModifiers keyMods = QApplication::keyboardModifiers();
            pressed_nFlags = ( keyMods & Qt::ShiftModifier ) ? pressed_nFlags |= MK_SHIFT : pressed_nFlags &= ~MK_SHIFT;
            pressed_nFlags = ( keyMods & Qt::ControlModifier ) ? pressed_nFlags |= MK_CONTROL : pressed_nFlags &= ~MK_CONTROL;
    }

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
                    float val = m_pDoc.m_meta_graph->getLocationValue(LogicalUnits(hit_point));
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

void QDepthmapView::timerEvent(QTimerEvent *event)
{
   if (event->timerId() == Tid_redraw) {
      if (m_queued_redraw) {
         // Internal own redraw
         OnRedraw(0,0);
      }
      else if (m_continue_drawing) {
         if (!m_drawing) {
            m_internal_redraw = true;
            repaint();
         }
      }
   }
}

void QDepthmapView::InitViewport(const QRect& phys_bounds, QGraphDoc *pDoc)
{
   QtRegion bounds = pDoc->m_meta_graph->getBoundingBox();
   m_unit = __max( bounds.width() / double(phys_bounds.width()), 
                   bounds.height() / double(phys_bounds.height()) );
   m_centre = bounds.getCentre();
   m_physical_centre = QSize(phys_bounds.width() / 2, phys_bounds.height() / 2);

   m_viewport_set = true;
}

QtRegion QDepthmapView::LogicalViewport(const QRect& phys_bounds, QGraphDoc *pDoc)
{
   if (m_resize_viewport) {
      m_physical_centre = QSize(phys_bounds.width() / 2, phys_bounds.height() / 2);
      m_resize_viewport = false;
   }

   return QtRegion( LogicalUnits(QPoint(phys_bounds.left(), phys_bounds.bottom())),
                  LogicalUnits(QPoint(phys_bounds.right(), phys_bounds.top())) );
}

void QDepthmapView::SetRedrawflag()
{
   if (m_pDoc.GetRemenuFlag(QGraphDoc::VIEW_MAP))
      m_pDoc.SetRemenuFlag(QGraphDoc::VIEW_MAP, false);

   if (m_pDoc.GetRedrawFlag(QGraphDoc::VIEW_MAP) != QGraphDoc::REDRAW_DONE)
   {
      if (m_pDoc.m_communicator) {
         m_queued_redraw = false;
         switch (m_pDoc.GetRedrawFlag(QGraphDoc::VIEW_MAP)) {
            case QGraphDoc::REDRAW_POINTS:
               if (m_pDoc.m_meta_graph->viewingProcessedLines()) {
                  // Axial lines are thicker on selection, so background needs clearing
                  m_redraw_all = true;
               }
               else {
                  m_redraw_no_clear = true;
               }
               break;

            case QGraphDoc::REDRAW_GRAPH:
               m_redraw_all = true;
               break;

            case QGraphDoc::REDRAW_TOTAL:
               m_viewport_set = false;
               m_redraw_all = true;
               break;
         }
         m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_DONE);
      }
      else {
         killTimer(Tid_redraw);
         Tid_redraw = startTimer(100);
         m_queued_redraw = true;
      }
   }
}

void QDepthmapView::paintEvent(QPaintEvent *)
{
    QPainter pDC(m_pixmap);

	SetRedrawflag();

    foreach (QWidget *widget, QApplication::topLevelWidgets()) 
	{
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
		{
			m_foreground = mainWin->m_foreground;
			m_background = mainWin->m_background;
			mainWin->updateToolbar();
			break;
		}
    }

/* if (pDC->IsPrinting()) 
   {
      if (!m_pDoc.m_meta_graph->setLock(this))
	  {
         return;
      }

      PrintBaby(pDC, m_pDoc);

      m_pDoc.m_meta_graph->releaseLock(this);

      return;
   }*/

   auto lock = m_pDoc.m_meta_graph->getLockDeferred();
   if (!lock.try_lock()){
       return;
   }

   m_drawing = true;


   QRect rect;
   int state = m_pDoc.m_meta_graph->getState();

   if (m_invalidate) 
   {
      //   selected colour is used for picking up highlights in OnMouseMove
      pDC.setPen(QPen(QBrush(QColor(m_selected_color)), 1, Qt::DotLine, Qt::RoundCap));

      if (m_invalidate & DRAWLINE) {
         if ((m_repaint_tag & DRAWLINE) && (m_invalidate == DRAWLINE || m_invalidate == LINEOFF)) {
            QRect tmpRect(PhysicalUnits(m_old_line.start()),PhysicalUnits(m_old_line.end()));
            DrawLine(&pDC, tmpRect, 0);
            m_repaint_tag &= ~DRAWLINE;
         }
         else if (m_invalidate == DRAWLINE || m_invalidate == LINEON) {
            QRect tmpRect(PhysicalUnits(m_line.start()),PhysicalUnits(m_line.end()));
            DrawLine(&pDC, tmpRect, 1);
            m_old_line = m_line;
            m_repaint_tag |= DRAWLINE;
         }
         m_invalidate = 0;
      }
      else if (m_invalidate & SNAP) {
         if ((m_repaint_tag & SNAP) && (m_invalidate == SNAP || m_invalidate == SNAPOFF)) {
            QPoint tmppoint(PhysicalUnits(m_old_snap_point));
            DrawCross(&pDC, tmppoint, 0);
            m_repaint_tag &= ~SNAP;
         }
         if (m_invalidate == SNAP || m_invalidate == SNAPON) {
            QPoint tmppoint(PhysicalUnits(m_snap_point));
            DrawCross(&pDC, tmppoint, 1);
            m_repaint_tag |= SNAP;
         }
         m_old_snap_point = m_snap_point;
         m_invalidate = 0;
      }
      else {
		 pDC.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
		 if(!m_drag_rect_b.isEmpty()) pDC.drawRect( m_drag_rect_b);
		 if(!m_drag_rect_a.isEmpty()) pDC.drawRect( m_drag_rect_a);
		 pDC.setCompositionMode(QPainter::CompositionMode_SourceOver);

		 m_drag_rect_b = m_drag_rect_a;
         m_invalidate = 0;
      }
   }
   else if (m_redraw_all || m_redraw_no_clear) 
   {
      m_repaint_tag = 0;   // <- for things that remember pixels in last paint colour (e.g., DrawCross and DrawLine
	  rect = QRect(0, 0, width(), height());
      m_redraw = true;

      if (!m_viewport_set && state & (MetaGraph::LINEDATA | MetaGraph::SHAPEGRAPHS | MetaGraph::DATAMAPS)) {
         InitViewport(rect, &m_pDoc);
      }
      if (m_redraw_all) {
         m_clear = true;
      }

      m_redraw_all = false;
      m_redraw_no_clear = false;
   }
   else if (!m_internal_redraw) {
      // Give up on this: it just doesn't work on NT
      // && DCB_RESET != pDC->GetBoundsRect(rect, DCB_RESET)) {
	  rect = QRect(0, 0, width(), height());
      
      m_clear = true;
      m_redraw = true;
   }
   m_internal_redraw = false;

   // if redraw signalled:
   if (m_redraw && (state & (MetaGraph::LINEDATA | MetaGraph::SHAPEGRAPHS | MetaGraph::DATAMAPS)) && m_viewport_set) {

      // note that the redraw rect is dependent on the cleared portion above
      // note you *must* check *state* before drawing, you cannot rely on view_class as it can be set up before the layer is ready to draw:
      if (state & MetaGraph::POINTMAPS && (!m_pDoc.m_meta_graph->getDisplayedPointMap().isProcessed()
          || m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA))
          && !m_pDoc.m_communicator) // <- m_communicator because I'm having thread locking problems
	  {
         m_pDoc.m_meta_graph->getDisplayedPointMap().setScreenPixel( m_unit ); // only used by points (at the moment!)
         m_pDoc.m_meta_graph->getDisplayedPointMap().makeViewportPoints( LogicalViewport(rect, &m_pDoc) );
      }
      if (state & MetaGraph::SHAPEGRAPHS && (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWAXIAL | MetaGraph::VIEWBACKAXIAL))) {
         m_pDoc.m_meta_graph->getDisplayedShapeGraph().makeViewportShapes( LogicalViewport(rect, &m_pDoc) );
      }
      if (state & MetaGraph::DATAMAPS && (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWBACKDATA | MetaGraph::VIEWDATA))) {
         m_pDoc.m_meta_graph->getDisplayedDataMap().makeViewportShapes( LogicalViewport(rect, &m_pDoc) );
      }
      if (state & MetaGraph::LINEDATA) {
         m_pDoc.m_meta_graph->SuperSpacePixel::makeViewportShapes( LogicalViewport(rect, &m_pDoc) );
      }

      m_continue_drawing = true;
      m_redraw = false;
   }

   if (m_clear) 
   {
		pDC.fillRect(rect, QBrush(QColor(m_background)));
        m_clear = false;
   }

   // If the meta graph (at least) contains a DXF, draw it:
   if (m_continue_drawing && (state & (MetaGraph::LINEDATA | MetaGraph::SHAPEGRAPHS | MetaGraph::DATAMAPS)) && m_viewport_set) 
   {
      if (Output(&pDC, &m_pDoc, true))
	  {
         Tid_redraw = startTimer(100);
         m_continue_drawing = true;
      }
      else {
         m_continue_drawing = false;
         // Finished: kill the timer:
         if (!m_queued_redraw) {
            killTimer(Tid_redraw);
         }
      }
   }
   else {
      m_continue_drawing = false;   // can't draw for some reason
   }

   m_drawing = false;
   QPainter screenPainter(this);
   screenPainter.drawPixmap(0,0,width(),height(),*m_pixmap);
}

void QDepthmapView::resizeGL(int w, int h)
{
//   m_viewport_set = false;
   m_redraw_all = true;
   m_resize_viewport = true;
   m_pDoc.m_view[QGraphDoc::VIEW_MAP] = this;
   m_pixmap = new QPixmap(w, h);
   update();
}

void QDepthmapView::BeginDrag(QPoint point)
{
   m_current_mode = NONE;
   int handle = -1;
   for (size_t i = 0; i < m_point_handles.size(); i++) {
      QPoint pt = PhysicalUnits(m_point_handles[i]);
      if (abs(pt.x()-point.x()) < 7 && abs(pt.y()-point.y()) < 7) {
         handle = (int)i;
         m_mouse_mode |= OVERHANDLE;
         SetCursor(m_mouse_mode);
         break;
      }
   }
   m_active_point_handle = handle;
   // for now, only two handles exist:
   if (m_active_point_handle != -1) {
      if (m_active_point_handle == 0) {
         m_line = Line(m_point_handles[1],m_point_handles[0]);
      }
      else {
         m_line = Line(m_point_handles[0],m_point_handles[1]);
      }
      m_mouse_mode |= DRAWLINE;
      m_mouse_mode &= ~OVERHANDLE;
      SetCursor(m_mouse_mode);
      m_invalidate = LINEON;
      update();
   }
}

void QDepthmapView::mouseMoveEvent(QMouseEvent *e)
{
   QPoint point = e->pos();

   if (pressed_nFlags & MK_CONTROL) {
      if (pressed_nFlags & MK_SHIFT) {
         // CTRL and SHIFT key down together, snap to displayed drawing layers
         // (note, don't use SHIFT on it's own, because that's already used for multiple select)
         MetaGraph *graph = m_pDoc.m_meta_graph;
         Point2f p = LogicalUnits(point);
         double d = -1.0;
         for (int i = 0; i < graph->getLineFileCount(); i++) {
            for (int j = 0; j < graph->getLineLayerCount(i); j++) {
               ShapeMap& map = graph->getLineLayer(i,j);
               if (map.isShown()) {
                  Point2f px = map.getClosestVertex(p);
                  if (!px.atZero() && (d == -1 || dist(p,px) < d)) {
                     d = dist(p,px);
                     m_snap_point = px;
                  }
               }
            }
         }
         if (d != -1) {
            point = PhysicalUnits(m_snap_point);
            if (!m_snap) {
               m_invalidate = SNAPON;
               m_snap = true;
               update();
            }
            else if (m_old_snap_point != m_snap_point) {
               m_invalidate = SNAP;
               m_snap = true;
               update();
            }
         }
      }
      else {
         // If only CTRL key down, snap to grid
         if (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA)) {
            PointMap& map = m_pDoc.m_meta_graph->getDisplayedPointMap();
            if (m_pDoc.m_meta_graph->getDisplayedPointMap().getSpacing() / m_unit > 20) {
               // hi-res snap when zoomed in
               m_snap_point = map.depixelate(map.pixelate(LogicalUnits(point),false,2),0.5);
            }
            else {
               m_snap_point = map.depixelate(map.pixelate(LogicalUnits(point)));
            }
            point = PhysicalUnits(m_snap_point);
            if (!m_snap) {
               m_invalidate = SNAPON;
               m_snap = true;
               update();
            }
            else if (m_old_snap_point != m_snap_point) {
               m_invalidate = SNAP;
               m_snap = true;
               update();
            }
         }
      }
   }
   else if (m_snap) {
      m_invalidate = SNAPOFF;
      update();
      m_snap = false;
   }

   // Left button down...
   if (pressed_nFlags & MK_LBUTTON) {
      if (m_current_mode == DRAG) {

         point -= m_mouse_point;
         m_drag_rect_a.translate(point);
         m_centre.x -= double(point.x()) * m_unit;
         m_centre.y += double(point.y()) * m_unit;
         m_mouse_point += point;

         m_invalidate = DRAG;
         update();
      }
      else if (m_current_mode == SELECT || m_current_mode == ZOOM_IN || m_current_mode == JOIN || m_current_mode == UNJOIN) {
         int x1, y1, x2, y2;
		 if (m_mouse_point.x()>point.x())
		 {
			 x1 = point.x();
			 x2 = m_mouse_point.x() - 1;
		 }
		 else
		 {
			 x1 = m_mouse_point.x();
			 x2 = point.x() - 1;
		 }

		 if (m_mouse_point.y()>point.y())
		 {
			 y1 = point.y();
			 y2 = m_mouse_point.y() - 1;
		 }
		 else
		 {
			 y1 = m_mouse_point.y();
			 y2 = point.y() - 1;
		 }
		 m_drag_rect_a = QRect(x1, y1, x2-x1, y2-y1);
         m_invalidate = SELECT;
         update();

      }
      // NB. deliberatle mouse mode not current mode
      else if (m_mouse_mode == (SELECT | OVERHANDLE)) {
         // ideally, I would enforce click once, move to location, and click again, but this
         // at least allows novices to use it easily:
         if (abs(point.x()-m_mouse_point.x())+abs(point.y()-m_mouse_point.y())>3) {
            BeginDrag(m_mouse_point);
            m_current_mode = SELECT | DRAWLINE;
         }
      }
      // NB. deliberatle mouse mode not current mode
      else if (m_mouse_mode == LINETOOL) {
         // ideally, I would enforce click once, move to location, and click again, but this
         // at least allows novices to use it easily:
         if (PixelDist(point, m_mouse_point) > 6) {
            m_mouse_mode |= DRAWLINE;
            m_current_mode = m_mouse_mode;
            m_line = Line(m_mouse_location, LogicalUnits(point));
            m_invalidate = LINEON;
         update();
         }
      }
      else if (m_current_mode == PENCIL) {
         if (m_mouse_point != point &&
             m_pDoc.m_meta_graph->getDisplayedPointMap().pixelate(LogicalUnits(point)) !=
             m_pDoc.m_meta_graph->getDisplayedPointMap().pixelate(LogicalUnits(m_mouse_point))) {
             m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(LogicalUnits(point),true);
             m_redraw_no_clear = true;
             m_mouse_point = point;
            // Redraw scene
         update();
         }
      }
      else if (m_current_mode == ERASE) {
         if (m_mouse_point != point &&
               m_pDoc.m_meta_graph->getDisplayedPointMap().pixelate(LogicalUnits(point)) !=
               m_pDoc.m_meta_graph->getDisplayedPointMap().pixelate(LogicalUnits(m_mouse_point))) {
            m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(LogicalUnits(point),false);
            m_redraw_no_clear = true;
            m_mouse_point = point;
            // Redraw scene
         update();
         }
      }
   }
   else if (pressed_nFlags & MK_RBUTTON) {
      if (!m_right_mouse_drag && abs(point.x()-m_mouse_point.x())+abs(point.y()-m_mouse_point.y())>3) {
         // begin right mouse drag:
         SetCursor(DRAG);
         m_mouse_point = point;

         m_drag_rect_a = QRect(0,0,width(),height());
         m_drag_rect_b = QRect(0,0,0,0);
         m_invalidate = DRAG;
         update();

         //SetCapture();
         
         m_right_mouse_drag = true;
      }
      else if (m_right_mouse_drag) {
         // continue right mouse drag
         point -= m_mouse_point;
         m_drag_rect_a.translate(point);
         m_centre.x -= double(point.x()) * m_unit;
         m_centre.y += double(point.y()) * m_unit;
         m_mouse_point += point;
         m_invalidate = DRAG;
         update();
      }
   }
   else if ((m_mouse_mode & SELECT) && !(m_mouse_mode & DRAWLINE) && !m_right_mouse_drag) {
      // in select mode, might be over a point handle -- if so, change the cursor:
      if (m_point_handles.size()) {
         bool found = false;
         for (size_t i = 0; i < m_point_handles.size(); i++) {
            QPoint pt = PhysicalUnits(m_point_handles[i]);
            if (abs(pt.x()-point.x()) < 7 && abs(pt.y()-point.y()) < 7) {
               found = true;
               m_mouse_mode |= OVERHANDLE;
               SetCursor(m_mouse_mode);
               break;
            }
         }
         if (!found && (m_mouse_mode & OVERHANDLE)) {
            m_mouse_mode &= ~OVERHANDLE;
            SetCursor(m_mouse_mode);
         }
      }
      else if (m_mouse_mode & OVERHANDLE) {
         m_mouse_mode &= ~OVERHANDLE;
         SetCursor(m_mouse_mode);
      }
   }

   if (!m_right_mouse_drag) {
      if (m_mouse_mode & DRAWLINE) {
         m_line = Line(m_line.t_start(), m_snap ? m_snap_point : LogicalUnits(point));
         if (m_line.t_end() != m_old_line.t_end()) {
            m_invalidate = DRAWLINE;
         update();
         }
      }
      else if (m_mouse_mode & JOINB) {
         point -= m_mouse_point;
         m_drag_rect_a.translate(point);
         m_mouse_point += point;
         // Redraw scene
         m_invalidate = JOINB;
         update();
      }
   }

   m_pDoc.m_position = m_snap ? m_snap_point : LogicalUnits( point );
   m_pDoc.UpdateMainframestatus();
   hit_point = point;
}

void QDepthmapView::mouseDoubleClickEvent(QMouseEvent *e)
{

}

void QDepthmapView::mousePressEvent(QMouseEvent *e)
{
    switch(e->button())
    {
    case Qt::LeftButton:
        pressed_nFlags = MK_LBUTTON;
        break;
    case Qt::RightButton:
        pressed_nFlags = MK_RBUTTON;
        break;
    }
	QPoint point = e->pos();
	if (pressed_nFlags & MK_RBUTTON)
	{
	   if (m_snap) {
		  point = PhysicalUnits(m_snap_point);
	   }
	   m_mouse_point = point;
	}
	else
	{
	   if (m_snap) {
		  point = PhysicalUnits(m_snap_point);
	   }

	   m_mouse_point = point;
	   m_mouse_location = m_snap ? m_snap_point : LogicalUnits(point);
	   // Just reset?

	   m_current_mode = NONE;
	   if (m_current_mode == NONE) {
		  m_current_mode = m_mouse_mode;
		  switch (m_mouse_mode) {
		  case DRAG:
			 {
				m_mouse_point = point;
				m_drag_rect_a = QRect(0,0,width(),height());
				m_drag_rect_b= QRect(0,0,0,0);
				m_invalidate = DRAG;
				update();
				//SetCapture();
			 }
			 break;
		  case SELECT:
		  case ZOOM_IN:
		  case JOIN:
		  case UNJOIN:
			 {
				m_mouse_point = point;
				m_drag_rect_a = QRect(0,0,0,0);
				m_drag_rect_b = QRect(0,0,0,0);

				m_invalidate = SELECT;
				update();
				//SetCapture();
			 }
			 break;
		  case PENCIL:
			 {
				m_current_mode = PENCIL;
				// Fill the point
                m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(LogicalUnits(point),true);
				m_mouse_point = point;
				// Redraw scene
				m_redraw_no_clear = true;
			 update();
			 }
			 break;
		  case ERASE:
			 // no longer used
			 break;
		  default:
			 break;
		  }
	   }
	}

}

void QDepthmapView::BeginJoin()
{
   if (m_pDoc.m_meta_graph->getSelCount() > 1 && m_pDoc.m_meta_graph->viewingProcessedPoints()) {
      QtRegion r = m_pDoc.m_meta_graph->getDisplayedPointMap().getSelBounds();
      QRect rect(PhysicalUnits(Point2f(r.bottom_left.x,r.top_right.y)),PhysicalUnits(Point2f(r.top_right.x,r.bottom_left.y)));
      int spacer = int(ceil(5.0 * m_pDoc.m_meta_graph->getDisplayedPointMap().getSpacing() / (m_unit * 10.0) ));
      m_mouse_point = this->rect().center();
      m_drag_rect_a = QRect(-rect.width()-spacer/2,-rect.height()-spacer/2,spacer/2,spacer/2);
      m_drag_rect_a.translate(m_mouse_point);
   }
   else {
      m_drag_rect_a = QRect(0,0,0,0);
   }
   m_drag_rect_b = QRect(0,0,0,0);

   m_mouse_mode |= JOINB;
   SetCursor(m_mouse_mode);
}

void QDepthmapView::mouseReleaseEvent(QMouseEvent *e)
{
	QPoint point = e->pos();
	if (pressed_nFlags & MK_LBUTTON)
	{
	   if (m_snap) {
		  point = PhysicalUnits(m_snap_point);
	   }
	   Point2f location = m_snap ? m_snap_point : LogicalUnits( point );

	   switch (m_current_mode) {
	   case ZOOM_IN:
		  if (m_drag_rect_a.width() > 2 && m_drag_rect_a.height() > 2) {
			 QRect rect = QRect(0,0,width(),height());
			 double ratio = __min( double(rect.height() / double(m_drag_rect_a.height()) ),
								   double(rect.width()) / double(m_drag_rect_a.width()) );
             ZoomTowards(1.0/ratio, LogicalUnits(m_drag_rect_a.center()) );
		  }
		  else {
             ZoomTowards(0.75, m_centre);
		  }
		  break;
	   case ZOOM_OUT:
		  {
             ZoomTowards(1.5, m_centre);
		  }
		  break;
	   case DRAG:
		  {   
			 // Stop drag rect...
			 m_drag_rect_a = QRect(0,0,0,0);
			 m_invalidate = DRAG;
			 update();

			 // Redraw scene
			 m_invalidate = 0;
			 m_redraw_all = true;
			 update();
		  }
		  break;
	   case UNJOIN:
	   case SELECT: 
	   case JOIN:
		  {
             QtRegion r;
			 if (m_drag_rect_a.isEmpty()) {
                r = QtRegion( LogicalUnits(m_mouse_point), LogicalUnits(m_mouse_point) );
			 }
			 else {
                r = QtRegion( LogicalUnits( QPoint(m_drag_rect_a.left(), m_drag_rect_a.bottom()) ),
							LogicalUnits( QPoint(m_drag_rect_a.right(), m_drag_rect_a.top()) ) );
			 }

			 // Stop drag rect...
			 m_drag_rect_a= QRect(0,0,0,0);
			 m_invalidate = SELECT;
			 update();

             if (!m_pDoc.m_communicator) {
				// After checking that processing isn't occurring...
				// Do the selection (might take a while if someone selects the lot...)
				if (pressed_nFlags & MK_SHIFT) {
                   m_pDoc.m_meta_graph->setCurSel( r, true ); // <- add to current sel
				}
				else {
                   m_pDoc.m_meta_graph->setCurSel( r, false ); // <- reset current sel
				}
				// Redraw scene
                m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
			 }
		  }
		  break;
	   case JOIN | JOINB:
		  {
			 m_current_mode = NONE;
			 // now get on with join:
			 bool ok = false; 
			 bool clearcursor = false;
             if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                ok = m_pDoc.m_meta_graph->getDisplayedPointMap().mergePoints( LogicalUnits(point) );
			 }
             else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                if (m_pDoc.m_meta_graph->getSelCount() == 1) {
                   ok = m_pDoc.m_meta_graph->getDisplayedShapeGraph().linkShapes( LogicalUnits(point) );
				}
				else {
				   // oops: you are only allowed to join lines one to one:
                   m_pDoc.m_meta_graph->clearSel();
				   clearcursor = true;
				}
			 }
			 if (clearcursor || ok) {
				m_mouse_mode = JOIN;
				SetCursor(JOIN);
				m_drag_rect_a = QRect(0,0,0,0);
				if (ok) {
                    m_pDoc.modifiedFlag = true;
                   m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
				}
				else {
				   m_redraw_all = true;
			 update();
				}
			 }
		  }
		  break;
	   case UNJOIN | JOINB:
		  {
			 m_current_mode = NONE;
			 // now get on with unjoin:
			 bool ok = false;
			 bool clearcursor = false;
             if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                if (m_pDoc.m_meta_graph->getSelCount() == 1) {
                   ok = m_pDoc.m_meta_graph->getDisplayedShapeGraph().unlinkShapes( LogicalUnits(point) );
				}
				else {
				   // oops: you are only allowed to join lines one to one:
                   m_pDoc.m_meta_graph->clearSel();
				   clearcursor = true;
				}
			 }
			 if (clearcursor || ok) {
				m_mouse_mode = UNJOIN;
				SetCursor(UNJOIN);
				m_drag_rect_a = QRect(0,0,0,0);
				if (ok) {
                    m_pDoc.modifiedFlag = true;
                   m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
				}
				else {
				   m_redraw_all = true;
			 update();
				}
			 }
		  }
		  break;
	   case SELECT | DRAWLINE:
		  {
			 m_current_mode = NONE;
			 m_mouse_mode &= ~DRAWLINE;
			 m_invalidate = LINEOFF;
			 SetCursor(m_mouse_mode);
			 update();
             if (m_pDoc.m_meta_graph->moveSelShape(Line(m_line.t_start(),location))) {
                m_pDoc.modifiedFlag = true;
                m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
			 }
		  }
		  break;
	   case SELECT | OVERHANDLE:

		  BeginDrag(point);
		  break;

	   case LINETOOL: case POLYGONTOOL:
		  {
			 m_current_mode = NONE;
			 m_mouse_mode |= DRAWLINE;
			 m_line = Line(location, location);
			 m_invalidate = LINEON;
			 update();
		  }
		  break;
	   case LINETOOL | DRAWLINE:
		  {
			 m_current_mode = NONE;
			 m_mouse_mode &= ~DRAWLINE;
			 m_invalidate = LINEOFF;
			 update();
             if (m_pDoc.m_meta_graph->makeShape(Line(m_line.t_start(),location))) {
                m_pDoc.modifiedFlag = true;
                m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
			 }
		  } 
		  break;
	   case POLYGONTOOL | DRAWLINE:
		  {
			 m_current_mode = NONE;
			 m_mouse_mode &= ~DRAWLINE;
			 m_invalidate = LINEOFF;
			 update();
			 // if it's the first part, just make it a line:
			 if (m_poly_points == 0) {
                m_currentlyEditingShapeRef = m_pDoc.m_meta_graph->polyBegin(Line(m_line.t_start(),location));
				m_poly_start = m_line.t_start();
				m_poly_points += 2;
				m_mouse_mode |= DRAWLINE;
				m_line = Line(location, location);
				m_invalidate = LINEON;
			 update();
			 }
			 else if (m_poly_points > 2 && PixelDist(point,PhysicalUnits(m_poly_start)) < 6) {
				// check to see if it's back to the original start point, if so, close off
                m_pDoc.m_meta_graph->polyClose(m_currentlyEditingShapeRef);
				m_poly_points = 0;
                m_currentlyEditingShapeRef = -1;
			 }
			 else {
                m_pDoc.m_meta_graph->polyAppend(m_currentlyEditingShapeRef, location);
				m_poly_points += 1;
				m_mouse_mode |= DRAWLINE;
				m_line = Line(location, location);
				m_invalidate = LINEON;
				update();
			 }
             m_pDoc.modifiedFlag = true;
             m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
		  }
		  break;
	   case FILL:
          m_pDoc.OnFillPoints( location, m_fillmode );
		  break;
	   case SEEDISOVIST:
		  m_current_mode = NONE;
          m_pDoc.OnMakeIsovist( location );
		  break;
	   case SEEDHALFOVIST:
		  m_current_mode = NONE;
		  m_mouse_mode |= DRAWLINE;
		  m_line = Line(location,location);
		  m_invalidate = LINEON;
		  update();
		  break;
	   case SEEDHALFOVIST | DRAWLINE:
		  {
			 m_current_mode = NONE;
			 m_mouse_mode &= ~DRAWLINE;
			 m_invalidate = LINEOFF;
			 update();
			 Point2f vec = m_line.vector();
			 vec.normalise();
             m_pDoc.OnMakeIsovist( m_line.t_start(), vec.angle() );
		  }
		  break;
	   case SEEDAXIAL:
          m_pDoc.OnToolsAxialMap( location );
		  // switch to select mode (stops you accidently pressing twice)
	      OnEditSelect();
		  break;
	   }

	   if (m_mouse_mode == JOIN) {
          if (m_pDoc.m_meta_graph->isSelected()) {
			 BeginJoin();
		  }
	   }
	   else if (m_mouse_mode == UNJOIN) {
          if (m_pDoc.m_meta_graph->isSelected()) {
             if (m_pDoc.m_meta_graph->viewingProcessedPoints()) {
                if (m_pDoc.m_meta_graph->getDisplayedPointMap().unmergePoints()) {
                    m_pDoc.modifiedFlag = true;
                   m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
				}
			 }
			 else {
				m_mouse_mode |= JOINB;
				SetCursor(m_mouse_mode);
			 }
		  }
	   }
	}
	else
	{
		if (m_snap) {
			point = PhysicalUnits(m_snap_point);
		}

		// Right click now only acts as a cancel,
		// (and because I personally am used to right-click to zoom out, zoom out!

		if (!m_right_mouse_drag) {
			// cancel any tool which uses drawline:
			if (m_mouse_mode & DRAWLINE) {
				m_mouse_mode &= ~DRAWLINE;
				if (m_mouse_mode & POLYGONTOOL && m_poly_points > 0) {
					m_poly_points = 0;
                    m_currentlyEditingShapeRef = -1;
                    m_pDoc.m_meta_graph->polyCancel(m_currentlyEditingShapeRef);
                    m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
				}
				else {
					m_invalidate = LINEOFF;
					update();
				}
			}
			// cancel other tools:
		switch (m_mouse_mode) {
		 case ZOOM_IN:
             ZoomTowards(0.75, m_centre);
			 break;
		 case JOIN | JOINB:
		 case UNJOIN | JOINB:
			 m_mouse_mode &= ~JOINB;
			 SetCursor(m_mouse_mode);
			 // drop through intentional
		 case SELECT:
             if (m_pDoc.m_meta_graph->isSelected()) {
                 m_pDoc.m_meta_graph->clearSel();
				 // Redraw scene
				 m_redraw_no_clear = true;
				 update();
			 }
			 break;
		 case PENCIL:
			 {
                 m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(LogicalUnits(point),false);
				 m_redraw_all = true;
				 update();
			 }
			 break;
			}
		}
		else {
			m_right_mouse_drag = false;
			SetCursor(m_mouse_mode);

			// Stop drag rect...
			m_drag_rect_a = QRect(0,0,0,0);
			m_invalidate = DRAG;
			update();

			// Redraw scene
			m_invalidate = 0;
			m_redraw_all = true;
			update();
		}
	}
	pressed_nFlags &= ~MK_LBUTTON;
	pressed_nFlags &= ~MK_RBUTTON;
}

void QDepthmapView::wheelEvent(QWheelEvent *e)
{
   short zDelta = e->delta();
   QPoint position = e->pos();
   QPoint centre(m_physical_centre.width(),m_physical_centre.height());
   auto zoomFactor = 1.0 + std::abs(double(zDelta)) / 120.0;
   if (zDelta > 0) {
      zoomFactor = 1.0/zoomFactor;
   }
   Point2f newCentre = ViewHelpers::calculateCenter(position, centre, zoomFactor);

   // Same as LogicalUnits() with non-discreet input
   newCentre.x = m_centre.x + m_unit * double(newCentre.x - m_physical_centre.width());
   newCentre.y = m_centre.y + m_unit * double(m_physical_centre.height() - newCentre.y);

    if(!IsAtZoomLimits(zoomFactor, 10)) {
        ZoomTowards(zoomFactor, newCentre);
    }
}

// provides a way to limit how much can we zoom out in relation to the window
// and graph bounding box size to avoid problems when zooming out too far
bool QDepthmapView::IsAtZoomLimits(double ratio, double maxZoomOutRatio) {
    if ( ratio < 1 )
    {
        return false;
    }
    // for zoom out
    QtRegion bounds = m_pDoc.m_meta_graph->getBoundingBox();
    double maxUnit = __max(bounds.width() / width(), bounds.height() / height());
    return m_unit * ratio > maxZoomOutRatio * maxUnit;
}

void QDepthmapView::ZoomTowards(double ratio, const Point2f& point)
{
   m_centre = point;
   m_unit *= ratio;

   m_invalidate = 0;

   // Redraw
   m_redraw_all = true;
   update();
}

QSize QDepthmapView::sizeHint() const
{
    return m_initialSize;
}

void QDepthmapView::saveToFile()
{
}

void QDepthmapView::postLoadFile()
{
    m_redraw_all = 1;
    setWindowTitle(m_pDoc.m_base_title+":Map View");
}

bool QDepthmapView::Output(QPainter *pDC, QGraphDoc *pDoc, bool screendraw)
{
   unsigned long ticks = 0;//GetTickCount();

   int state = pDoc->m_meta_graph->getState();

   bool b_continue = false;

   int spacer = GetSpacer(pDoc);

   if (!pDoc->m_communicator)
   {
      int viewclass = pDoc->m_meta_graph->getViewClass();
      if (viewclass & MetaGraph::VIEWVGA) {
         if (!b_continue && viewclass & MetaGraph::VIEWBACKAXIAL) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedShapeGraph(), true, spacer, ticks, screendraw);
         }
         if (!b_continue && viewclass & MetaGraph::VIEWBACKDATA) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedDataMap(), true, spacer, ticks, screendraw);
         }
         if (!b_continue) { 
            b_continue = DrawPoints(pDC, pDoc, spacer, ticks, screendraw);
         }
      }
      else if (!b_continue && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
         if (viewclass & MetaGraph::VIEWBACKVGA) {
            b_continue = DrawPoints(pDC, pDoc, spacer, ticks, screendraw);
         }
         if (!b_continue && viewclass & MetaGraph::VIEWBACKDATA) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedDataMap(), true, spacer, ticks, screendraw);
         }
         if (!b_continue) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedShapeGraph(), false, spacer, ticks, screendraw);
         }
      }
      else if (!b_continue && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
         if (viewclass & MetaGraph::VIEWBACKAXIAL) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedShapeGraph(), true, spacer, ticks, screendraw);
         }
         if (!b_continue && viewclass & MetaGraph::VIEWBACKVGA) {
            b_continue = DrawPoints(pDC, pDoc, spacer, ticks, screendraw);
         }
         if (!b_continue) {
            b_continue = DrawShapes(pDC, pDoc->m_meta_graph->getDisplayedDataMap(), false, spacer, ticks, screendraw);
         }
      }
   }

   if (!b_continue && state & MetaGraph::LINEDATA) 
   {
      bool nextlayer = false, first = true;
      pDC->setPen(QPen(QBrush(QColor(m_foreground)), spacer/20+1, Qt::SolidLine, Qt::RoundCap));
      while ( (b_continue = pDoc->m_meta_graph->SuperSpacePixel::findNextShape(nextlayer)) )
	  {
/*       Line l = pDoc->m_meta_graph->SuperSpacePixel::getNextLine();
         if (nextlayer || first) {
            PafColor color;
            color = pDoc->m_meta_graph->SuperSpacePixel::getLineColor();
            pDC->SelectObject(oldpen);
            pen.DeleteObject();
            if (color.alphab() != 0) {
               pen.CreatePen( PS_SOLID, spacer/20+1, color );
            }
            else {
               pen.CreatePen( PS_SOLID, spacer/20+1, GetApp()->m_foreground );
            }
            pDC->SelectObject(&pen);
            first = false;
            nextlayer = false;
         }*/

         const SalaShape& shape = pDoc->m_meta_graph->SuperSpacePixel::getNextShape();
		 spacer = GetSpacer(pDoc);
		 
         if (shape.isPoint()) {
         }
         else if (shape.isLine()) {
            Line line = shape.getLine();
			QPoint p1 = PhysicalUnits(line.start());
            QPoint p2 = PhysicalUnits(line.end());
            if (p1 != p2) {
               pDC->drawLine( p1, p2 );
            }
         }
         else {
		    size_t i;
            for (i = 1; i < shape.m_points.size(); i++)
			{
               pDC->drawLine(PhysicalUnits(shape.m_points[i-1]), PhysicalUnits(shape.m_points[i]));
            }
            if (shape.isClosed()) {
               pDC->drawLine(PhysicalUnits(shape.m_points[i-1]), PhysicalUnits(shape.m_points[0]));
            }
         }
      }
   }

   if (!b_continue && spacer > 1 && pDoc->m_meta_graph->viewingProcessedShapes() && pDoc->m_meta_graph->m_showtext) {
/*      setFont(QFont("Arial", 10, 10));
      pDC->setPen(QPen(QBrush(QColor(m_foreground)), 1, Qt::SolidLine, Qt::FlatCap));

      // display the layer attribute data:
      // THIS NEEDS SORTING OUT
      ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();
      for (int i = 0; i < map.getObjectCount(); i++) {
         if (map.getDisplayedAttributeValue(i) > 0) {
            std::string text = map.getDisplayedAttributeText(i);
            QPoint p = PhysicalUnits(map.getCentroid(i));
            QSize sz = pDC->GetTextExtent(text.c_str());
            pDC->drawText(p.x(), p.y()-(sz.height()/2), text.c_str());
         }
      }*/
   }

   if (!b_continue && m_showlinks) 
   {
	  pDC->setBrush(QBrush( QColor(m_foreground), Qt::SolidPattern));
      if (pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA && pDoc->m_meta_graph->getDisplayedPointMap().isProcessed())
	  {
         PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();
         // merge lines
         pDC->setPen(QPen(QBrush(QColor(00, 255, 0)), spacer/10+1, Qt::SolidLine));
         while ( (b_continue = map.findNextMergeLine()) ) {
            Line line = map.getNextMergeLine();
            DrawLink(pDC, spacer, line);
         }
      }
      else if ((state & MetaGraph::SHAPEGRAPHS) && (pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL)) {
         // link lines
         pDC->setPen(QPen(QBrush(QColor(00, 255, 0)), spacer/10+1, Qt::SolidLine));
         while ( (b_continue = pDoc->m_meta_graph->getDisplayedShapeGraph().findNextLinkLine()) ) {
            Line line = pDoc->m_meta_graph->getDisplayedShapeGraph().getNextLinkLine();
            DrawLink(pDC, spacer, line);
         }
         pDC->setPen(QPen(QBrush(QColor(255, 00, 00)), spacer/10+1, Qt::SolidLine));
         pDC->setBrush(QBrush( QColor(m_background), Qt::SolidPattern));

         while ( (b_continue = pDoc->m_meta_graph->getDisplayedShapeGraph().findNextUnlinkPoint()) ) {
            QPoint p = PhysicalUnits( pDoc->m_meta_graph->getDisplayedShapeGraph().getNextUnlinkPoint() );
            p -= QPoint(2+spacer/2,2+spacer/2);
            QRect rect(p, QSize(spacer+4,spacer+4));
            pDC->drawEllipse( rect );
         }
      }
   }

   return b_continue;
}

bool QDepthmapView::DrawPoints(QPainter *pDC, QGraphDoc *pDoc, int spacer, unsigned long ticks, bool screendraw) 
{
   unsigned long c_tick = 0;
   bool b_continue = false;

   PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

   bool muted = (map.isProcessed() && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWBACKVGA);
   if (m_showlinks) {
      if (!muted) {
         muted = true;
      }
      else {
         return b_continue; // too confusing!
      }
   }

   if (pDoc->m_meta_graph->getViewClass() & (MetaGraph::VIEWBACKAXIAL | MetaGraph::VIEWBACKDATA)) {
      spacer /= 2;   // allow see through to axial lines
   }

   bool monochrome = (map.isProcessed() && map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);

   //if (monochrome) {
//      standardbrush = new QBrush( GetApp()->m_foreground );
   //}
   pDC->setBrush(QBrush( QColor(m_foreground), Qt::SolidPattern));

   while ( (b_continue = map.findNextPoint()) ) 
   {
      Point2f logical = map.getNextPointLocation();

      PafColor color;
      color = map.getCurrentPointColor();

      if (color.alphab() != 0) 
	  { // alpha == 0 is transparent
         if (monochrome && !map.getPointSelected()) 
		 {
            QPoint p = PhysicalUnits(logical);
            int subspacer = (3 * color.blueb() * spacer) / 255;
            if (subspacer >= 1) {
               pDC->drawEllipse( p.x() - subspacer, p.y() + subspacer, subspacer*2, subspacer*2);
            }
            else {
                pDC->drawPoint( p );
            }
         }
         else {
            QRgb rgb = qRgb(color.redb(),color.greenb(),color.blueb());
            if (muted && !map.getPointSelected()) { // keeps selected points bright yellow
               rgb = colorMerge(rgb, m_background);
            }
            QPoint p = PhysicalUnits(logical);
            if (spacer > 1)
			{
               // Standard code
			   pDC->setBrush(QBrush( QColor(rgb), Qt::SolidPattern));
			   pDC->fillRect(QRect(p.x() - spacer, p.y() - spacer, spacer*2, spacer*2), QBrush(QColor(rgb)));
            }
            else {
               pDC->drawPoint( p );
            }
         }
      }
      if (screendraw && c_tick++ > 10000) break;
   }

   if (!b_continue && pDoc->m_meta_graph->m_showgrid) 
   {
      pDC->setPen(QPen(QBrush(QColor(colorMerge(m_foreground, m_background))), 1, Qt::SolidLine));
      if (pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
         // show grid as though points are filling the spaces
         while ( (b_continue = map.findNextRow()) ) {
            Line logical = map.getNextRow();
            pDC->drawLine(PhysicalUnits( logical.start()), PhysicalUnits( logical.end() ) );
         }
         while ( (b_continue = map.findNextCol()) ) {
            Line logical = map.getNextCol();
            pDC->drawLine(PhysicalUnits( logical.start()), PhysicalUnits( logical.end() ) );
         }
      }
      else {
         // show actual grid
         while ( (b_continue = map.findNextPointRow()) ) {
            Line logical = map.getNextPointRow();
            pDC->drawLine(PhysicalUnits( logical.start()), PhysicalUnits( logical.end() ) );
         }
         while ( (b_continue = map.findNextPointCol()) ) {
            Line logical = map.getNextPointCol();
            pDC->drawLine(PhysicalUnits( logical.start()), PhysicalUnits( logical.end() ) );
         }
      }
   }

   return b_continue;
}

bool QDepthmapView::DrawShapes(QPainter *pDC, ShapeMap& map, bool muted, int spacer, unsigned long ticks, bool screendraw) 
{
   unsigned long c_tick = 0;
   bool b_continue = false;

   if (m_showlinks) {
      if (!muted) {
         muted = true;
      }
      else {
         return b_continue; // too confusing!
      }
   }

   bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);

   if (screendraw && !muted) {
      // i.e., at the front... implies can select... allow this draw to overwrite any existing point handle locations:
      m_point_handles.clear();
   }

   QPen pen(QBrush(QColor(m_foreground)), spacer/20+1, Qt::SolidLine);
   bool dummy;
   int count = 0;
   while ( (b_continue = map.findNextShape(dummy)) ) 
   {
      count++;
      PafColor color;
      // n.b., MONOCHROME settings only for line thickness...
      color = map.getShapeColor();
      bool selected = map.getShapeSelected();

      // n.b., getNextShape clears polygon ready for next, so get polygon color and selected attribute before this:
      const SalaShape& poly = map.getNextShape();
      QPoint *points = NULL;
      int drawable = 0;
      if (!poly.isPoint() && !poly.isLine() && !poly.m_points.empty()) {
         points = new QPoint [poly.m_points.size()];
         if (poly.m_points.size() > 0) drawable++;
         for (auto& point: poly.m_points) {
            points[drawable] = PhysicalUnits(point);
            if (points[drawable] != points[drawable-1]) {
               drawable++;
            }
         }
      }
      //
      QBrush brush;
      QPen pen2;
      QRgb rgb;
      int tempspacer = selected ? spacer * 3: spacer;
      if (!monochrome || selected) 
	  {
         rgb = qRgb(color.redb(),color.greenb(),color.blueb());
         if (muted && !selected) {
            rgb = colorMerge(rgb, m_background);
         }
         if (poly.isClosed() || poly.isPoint()) {
			brush = QBrush( QColor(rgb), Qt::SolidPattern);
         }
         else {
            pen2 = QPen(QBrush(QColor(rgb)), tempspacer/10+1, Qt::SolidLine);
           }
      }
      else {
         int thickness = (spacer * color.blueb()) / 255;
         if (thickness < 1) {
            // note, monochrome excludes lines below 'thin' threshold
            continue;
         }
         if (poly.isClosed()) {
			brush = QBrush( QColor(m_background), Qt::SolidPattern);
         }
         pen2 = QPen(QBrush(QColor(m_foreground)), thickness, Qt::SolidLine);
      }
      if (poly.isClosed()) {
         if (drawable > 1) {
            if (!map.m_show_lines) {
                pDC->setPen(Qt::NoPen);
            }
            else if (monochrome && !selected) {
               pDC->setPen(pen2);
            }
            else {
               pDC->setPen(pen2);
            }
            if (!map.m_show_fill) {
				pDC->setBrush(Qt::NoBrush);
            }
            else {
				pDC->setBrush(brush);
            }
            pDC->drawPolygon( points, drawable );
            //
            if (map.m_show_centroids) {
               Point2f p = poly.getCentroid();
			   pDC->setPen(QColor(255,255,0));
               pDC->drawPoint( PhysicalUnits(p));
            }
         }
         else {
            pDC->setPen(QColor(rgb));
            pDC->drawPoint(points[0]);
         }
      }
      else {
         if (poly.isPoint()) {
            QPoint point = PhysicalUnits(poly.getPoint());
            if (tempspacer < 2) {
               pDC->setPen(QColor(rgb));
               pDC->drawPoint(point);
            }
            else {
               QRect rect(point.x()-tempspacer/2, point.y()-tempspacer/2, tempspacer, tempspacer);
               if (tempspacer < 4) {
				  pDC->setBrush(QBrush( QColor(rgb), Qt::SolidPattern));
                  pDC->drawRect(rect);
               }
               else {
                  pDC->setPen(pen);
                  pDC->setBrush(brush);
                  pDC->drawEllipse( rect );
               }
            }
         }
         else if (poly.isLine()) {
            pDC->setPen(pen2);
            Line l = poly.getLine();
            QPoint start = PhysicalUnits(l.start());
            QPoint end = PhysicalUnits(l.end());
            if (start != end) {
               pDC->drawLine ( start, end );
            }
            if (selected && screendraw && !muted && map.getSelCount() == 1 && map.isEditable()) {
               m_point_handles.push_back(l.start());
               m_point_handles.push_back(l.end());
            }
         }
         else {
            if (drawable > 1) {
               pDC->setPen(pen2);
               pDC->drawPolyline( points, drawable );
            }
            else {
			   pDC->setPen(QColor(rgb));
               pDC->drawPoint(points[0]);
            }
         }
      }
      if (points) {
         delete [] points;
      }
      if (screendraw && c_tick++ > 10000) break;
   }

   if (screendraw && !muted) {
      // i.e., at the front... implies can select... allow this draw to overwrite to draw point handle locations:
      for (size_t i = 0; i < m_point_handles.size(); i++) {
         DrawPointHandle(pDC,PhysicalUnits(m_point_handles[i]));
      }
   }

   return b_continue;
}

void QDepthmapView::DrawLink(QPainter *pDC, int spacer, const Line& logical) 
{
   spacer += 4;
   QPoint p1 = PhysicalUnits( logical.start() );
   QPoint p2 = PhysicalUnits( logical.end() );
   pDC->drawLine( p1, p2 );
   p1 -= QPoint(spacer/2,spacer/2);
   p2 -= QPoint(spacer/2,spacer/2);
   QRect rect1(p1, QSize(spacer,spacer));
   QRect rect2(p2, QSize(spacer,spacer));
   pDC->drawEllipse( rect1 );
   pDC->drawEllipse( rect2 );
}

void QDepthmapView::DrawPointHandle(QPainter *pDC, QPoint pt) 
{
   QRect rect(pt.x()-7, pt.y()-7, 14, 14);
   pDC->setBrush(QBrush( QColor(m_foreground), Qt::SolidPattern));
   pDC->drawRect(rect);
   rect.adjust(1,1,1,1);// DeflateRect(1,1,1,1);
   pDC->setBrush(QBrush( QColor(m_background), Qt::SolidPattern));
   pDC->drawRect(rect);
   rect.adjust(1,1,1,1);//DeflateRect(1,1,1,1);
   pDC->setBrush(QBrush( QColor(m_selected_color), Qt::SolidPattern));
   pDC->drawRect(rect);
}

void QDepthmapView::OutputEPS( std::ofstream& stream, QGraphDoc *pDoc, bool includeScale )
{
   // This output EPS is a copy of the standard output... obviously, if you change
   // standard output, remember to change this one too!
   
   // now the two are a little out of synch

   if (!m_viewport_set) {
       QMessageBox::warning(this, tr("Warning"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
       return;
   }

   QRect clrect, rect;
   clrect = QRect(0, 0, width(), height());//GetClientRect( clrect );

   stream << "%!PS-Adobe-3.0 EPSF-3.0\n"
          << "%%BoundingBox: 0 0 " << clrect.width() << " " << clrect.height() << "\n"
          << "%%Creator: " << TITLE_BASE << std::endl;

   // temporarily inflate resolution for EPS draw
   rect = QRect(clrect.left() * 10, clrect.top() * 10, clrect.width() * 10, clrect.height() * 10);
   QPoint oldcentre = QPoint(m_physical_centre.width(), m_physical_centre.height());
   double oldunit = m_unit;
   m_physical_centre = QSize(rect.width() / 2, rect.height() / 2);
   m_unit /= 10;

   int bg = (int)m_background;
   float bgr = float(GetRValue(bg))/255.0f;
   float bgg = float(GetGValue(bg))/255.0f;
   float bgb = float(GetBValue(bg))/255.0f;

   int fg = (int)m_foreground;
   float fgr = float(GetRValue(fg))/255.0f;
   float fgg = float(GetGValue(fg))/255.0f;
   float fgb = float(GetBValue(fg))/255.0f;
   
   stream << "/M {moveto} def" << std::endl;
   stream << "/L {lineto} def" << std::endl;
   stream << "/R {rlineto} def" << std::endl;
   stream << "/C {setrgbcolor} def" << std::endl;
   stream << "/W {setlinewidth} def" << std::endl;

   stream << "newpath\n"
          << 0 << " " << 0 << " M\n"
          << clrect.width() << " " << 0 << " R\n"
          << 0 << " " << clrect.height() << " R\n"
          << -clrect.width() << " " << 0 << " R\n"
          << "closepath\n"
          << bgr << " " << bgg << " " << bgb << " C\n"
          << "fill" << std::endl;

   int state = pDoc->m_meta_graph->getState();

   QtRegion logicalviewport = LogicalViewport(rect, pDoc);

   if (state & MetaGraph::POINTMAPS) {
      pDoc->m_meta_graph->getDisplayedPointMap().setScreenPixel( m_unit ); // only used by points (at the moment!)
      pDoc->m_meta_graph->getDisplayedPointMap().makeViewportPoints( logicalviewport );
   }
   if (state & MetaGraph::SHAPEGRAPHS) {
      pDoc->m_meta_graph->getDisplayedShapeGraph().makeViewportShapes( logicalviewport );
   }
   if (state & MetaGraph::DATAMAPS) {
      pDoc->m_meta_graph->getDisplayedDataMap().makeViewportShapes( logicalviewport );
   }
   if (state & MetaGraph::LINEDATA) {
      pDoc->m_meta_graph->SuperSpacePixel::makeViewportShapes( logicalviewport );
   }

   double spacer = GetSpacer(pDoc) / 10.0;
   if (spacer < 0.1) {
      spacer = 0.1;
   }

   if (state & MetaGraph::POINTMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {

      // Define EPS box using spacer dimensions:
      stream << "/bx\n"
             << " { newpath M\n" 
             << "   " <<  2 * spacer << " " << 0 << " R\n"
             << "   " <<  0 << " " << 2 * spacer << " R\n"
             << "   " <<  2 * -spacer << " " << 0 << " R\n" 
             << "   closepath } def" << std::endl;
      stream << "/fbx\n"
             << " { C fill } def" << std::endl;

      PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

      while ( map.findNextPoint() ) {

         Point2f logical = map.getNextPointLocation();

         PafColor color = map.getCurrentPointColor();

         if (color.alphab() != 0) { // alpha == 0 is transparent

            QPoint p = PhysicalUnits(logical);

            // Now do EPS box... remember the coordinate system is the right way up!
            stream << p.x() / 10.0 - spacer << " " << (rect.height() - p.y()) / 10.0 - spacer << " bx" << std::endl;
            stream << color.redf() << " " << color.greenf() << " " << color.bluef() << " fbx" << std::endl;
         }
      }
   }

   if (state & MetaGraph::SHAPEGRAPHS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {

      ShapeMap& map = pDoc->m_meta_graph->getDisplayedShapeGraph();

      OutputEPSMap(stream, map, logicalviewport, rect, spacer);
   }

   if (state & MetaGraph::DATAMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {

      ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();

      OutputEPSMap(stream, map, logicalviewport, rect, spacer);
   }

   if (state & MetaGraph::LINEDATA) {

      stream << "newpath" << std::endl;
      bool nextlayer = false;
      bool first = true;
      int style = 0;

      while ( pDoc->m_meta_graph->SuperSpacePixel::findNextShape(nextlayer) ) {

         const SalaShape& shape = pDoc->m_meta_graph->SuperSpacePixel::getNextShape();

         Line l;
         if (shape.isPoint()) {
         }
         else if (shape.isLine()) {
            Line line = shape.getLine();
            OutputEPSLine(stream, line, spacer, logicalviewport, rect);
         }
         else {
            OutputEPSPoly(stream, shape, spacer, logicalviewport, rect);
         }
      }

      int fg = (int)m_foreground;
      float fgr = float(GetRValue(fg))/255.0f;
      float fgg = float(GetGValue(fg))/255.0f;
      float fgb = float(GetBValue(fg))/255.0f;
      if (style == 1) {
         stream << "[" << spacer/10+1 << "]";
      }
      else {
         stream << "[]";
      }
      stream << " 0 setdash" << std::endl;
      stream << fgr << " " << fgg << " " << fgb << " C" << std::endl;
      stream << spacer/10+1 << " W" << std::endl;
      stream << "stroke" << std::endl;
   }

   // loaded paths
   if (pDoc->m_evolved_paths.size()) {

      stream << "newpath" << std::endl;

      for (size_t i = 0; i < pDoc->m_evolved_paths.size(); i++) {

         const prefvec<Point2f>& path = pDoc->m_evolved_paths[i];

         if (path.size() > 1) {

            QPoint last = PhysicalUnits(path[0]);
            stream << (last.x() - spacer/40) / 10.0 << " " << (rect.height() - last.y() + spacer/40) / 10.0 << " M\n" << std::endl;

            for (size_t j = 1; j < path.size(); j++) {
               QPoint next = PhysicalUnits(path[j]);
               stream << (next.x() - last.x() - spacer/40) / 10.0 << " " << (last.y() - next.y() - spacer/40) / 10.0 << " L" << std::endl;
               last = next;
            }

            stream << fgr << " " << fgg << " " << fgb << " C" << std::endl;
            stream << spacer/20+1 << " W" << std::endl;
            stream << "stroke" << std::endl;
         }
      }
   }
/*
   if (pDoc->m_agent_manager && pDoc->m_agent_manager->isPaused())
   {
      if (pDoc->m_agent_manager->isEcomorphic()) {
         // draw the ecomorphic artworks
         Ecomorph& eco = pDoc->m_agent_manager->getEcomorph();
         for (int i = 0; i < eco.arts().size(); i++)
         {
            PixelRef pos = eco.arts()[i].getPos();
            Point2f logical = pDoc->m_meta_graph->getDisplayedPointMap().depixelate(pos);
            // this is in units of logical is unit based so this actually works okay:
            QPoint p = PhysicalUnits(Point2f(logical.x, logical.y));
            QPoint bottomleft = PhysicalUnits(Point2f(logical.x - 0.5, logical.y - 0.5));
            // Now do EPS box... remember the coordinate system is the right way up!
            stream << (p.x - spacer) / 10.0 << " " << (rect.Height() - p.y - spacer) / 10.0 << " bx" << std::endl;
            stream << bgr << " " << bgg << " " << bgb << " fbx" << std::endl;
            // And cover with line:
            stream << (bottomleft.x - spacer/20) / 10.0 << " " << (rect.Height() - bottomleft.y + spacer/20) / 10.0 << " M\n"
                   << (spacer * 2 - spacer/20) / 10.0 << " " << 0 << " L\n"
                   << 0 << " " << (spacer * 2 - spacer/20) / 10.0 << " L\n"
                   << (-spacer * 2 + spacer/20) / 10.0 << " " << 0 << " L\n"
                   << 0 << " " << (-spacer * 2 + spacer/20) / 10.0 << " L" << std::endl;
            stream << fgr << " " << fgg << " " << fgb << " setrgbcolor" << std::endl;
            stream << spacer/10+1 << " setlinewidth" << std::endl;
            stream << "stroke" << std::endl;
         }
      }
   }
*/

   if(includeScale) {
       // add the scale to the bottom lefthand corner
       double logicalwidth = m_unit * rect.width();
       if (logicalwidth > 10) {
          int workingwidth = floor(log10(logicalwidth/2)*2.0);
          int barwidth = (int) pow(10.0,(double)(workingwidth/2)) * ((workingwidth%2 == 0) ? 1 : 5);
          double physicalbar = double(barwidth) / m_unit;
          stream << "newpath" << std::endl;
          stream << "0 0 M 0 18 R" << std::endl;
          stream << physicalbar / 10.0 << " 0 R 0 -18 R closepath" << std::endl;
          stream << bgr << " " << bgg << " " << bgb << " C" << std::endl;
          stream << "fill newpath" << std::endl;
          stream << fgr << " " << fgg << " " << fgb << " C" << std::endl;
          stream << "0 12 M" << std::endl;
          stream << physicalbar / 10.0 << " 0 R" << std::endl;
          stream << "3 W stroke" << std::endl;
          stream << "0 6 M 0 7.5 R" << std::endl;
          stream << physicalbar / 10.0 << " 6 M 0 7.5 R" << std::endl;
          stream << "1.5 W stroke" << std::endl;
          stream << "/Arial findfont 12 scalefont setfont" << std::endl;
          // assume metres!
          if (barwidth > 1000) {
             stream << "(" << (barwidth / 1000) << "km) stringwidth pop 2 div" << std::endl;
          }
          else {
             stream << "(" << barwidth << "m) stringwidth pop 2 div" << std::endl;
          }
          stream << physicalbar / 20.0 << " exch sub" << std::endl;
          stream << "0 M" << std::endl;
          stream << "(" << barwidth << "m) show" << std::endl;
       }
   }

   stream << "showpage" << std::endl;

   // undo temporary unit setting
   m_unit = oldunit;
   m_physical_centre = QSize(oldcentre.x(), oldcentre.y()) ;
}

void QDepthmapView::OutputEPSMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, QRect& rect, float spacer)
{
   bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);
   double thickness = 1.0, oldthickness = 1.0;
   bool closed, oldclosed = false;
   PafColor color, oldcolor;

   int fg = (int)m_foreground;
   float fgr = float(GetRValue(fg))/255.0f;
   float fgg = float(GetGValue(fg))/255.0f;
   float fgb = float(GetBValue(fg))/255.0f;

   stream << "newpath" << std::endl;
   stream << fgr << " " << fgg << " " << fgb << " C" << std::endl;
   bool dummy;
   while ( map.findNextShape(dummy) ) {

      // note: getNextLine clears current line, so getLineColor before line
      color = map.getShapeColor();
      const SalaShape& shape = map.getNextShape();
      closed = shape.isClosed();

      if (monochrome) {
         thickness = 3.0 * (color.blueb() * spacer) / 255.0;
         // note: anything below 'thin' threshold in monochrome note drawn
         if (thickness < 0.25) {
            continue;
         }
         if (thickness != oldthickness || closed != oldclosed) {
            stream << oldthickness << " W" << std::endl;
            stream << (oldclosed ? "fill" : "stroke") << std::endl;
            oldthickness = thickness;
            oldclosed = closed;
         }
      }
      else if (color != oldcolor || closed != oldclosed) {
         stream << oldcolor.redf() << " " << oldcolor.greenf() << " " << oldcolor.bluef() << " C" << std::endl;
         stream << (oldclosed ? "fill" : "stroke") << std::endl;
         oldcolor = color;
         oldclosed = closed;
      }

      Line l;
      if (shape.isPoint()) {
      }
      else if (shape.isLine()) {
         Line line = shape.getLine();
         OutputEPSLine(stream, line, spacer, logicalviewport, rect);
      }
      else {
         OutputEPSPoly(stream, shape, spacer, logicalviewport, rect);
      }
   }
   stream << thickness << " W" << std::endl;
   if (!monochrome) {
      stream << color.redf() << " " << color.greenf() << " " << color.bluef() << " C" << std::endl;
   }
   stream << (closed ? "fill" : "stroke") << std::endl;
}

void QDepthmapView::OutputEPSLine(std::ofstream& stream, Line& line, int spacer, QtRegion& logicalviewport, QRect& rect)
{
   bool drewit = false;
   if (line.crop(logicalviewport)) {
      QPoint start = PhysicalUnits(line.start());
      QPoint end = PhysicalUnits(line.end());
      // 10 units corresponds to 1 pixel on the screen
      if (sqrt(sqr(start.x() - end.x()) + sqr(start.y() - end.y())) > 5.0)
	  {
         stream << (start.x() / 10.0) << " " << (rect.height() - start.y()) / 10.0 << " M ";
         stream << (end.x() / 10.0) << " " << (rect.height() - end.y()) / 10.0 << " L" << std::endl;
      }
   }
}

void QDepthmapView::OutputEPSPoly(std::ofstream& stream, const SalaShape& shape, int spacer, QtRegion& logicalviewport, QRect& rect)
{
   bool starter = true;
   Point2f lastpoint = shape.m_points[0];
   int count = shape.isClosed() ? shape.m_points.size() + 1 : shape.m_points.size();
   int size = shape.m_points.size();
   for (int i = 1; i < count; i++) {
      Line line(lastpoint,shape.m_points[i%size]);
      if (line.crop(logicalviewport)) {
         // note: use t_start and t_end so that this line moves in the correct direction
         QPoint start = PhysicalUnits(line.t_start());
         QPoint end = PhysicalUnits(line.t_end());
         // 5.0 is about 1/2 pixel width
         if (sqrt(sqr(start.x() - end.x()) + sqr(start.y() - end.y())) > 5.0) 
		 {
            if (starter) {
               stream << start.x() / 10.0 << " " << (rect.height() - start.y()) / 10.0 << " M ";
            }
            stream << end.x() / 10.0 << " " << (rect.height() - end.y()) / 10.0 << " L" << std::endl;
            // note: you must use t_end (true end) so that it takes the end point from the shape[i] end:
            lastpoint = line.t_end();
            starter = false;
         }
      }
      else {
         lastpoint = shape.m_points[i];
         starter = true;
      }
   }
}

void QDepthmapView::DrawCross(QPainter *pDC, QPoint& centre, bool drawit)
{
   if (drawit) {
      for (int i = 0; i < 9; i++) {
         QPoint pos(centre.x()-4+i,centre.y());
         pDC->setPen(QColor(m_foreground));
         pDC->drawPoint(pos);
      }
      for (int j = 0; j < 9; j++) {
         if (j != 4) {
            QPoint pos(centre.x(),centre.y()-4+j);
            pDC->setPen(QColor(m_foreground));
            pDC->drawPoint(pos);
         }
      }
   }
   else { // erase it
      for (int i = 0; i < 9; i++) {
         QPoint pos(centre.x()-4+i,centre.y());
         pDC->drawPoint(pos);
      }
      for (int j = 0; j < 9; j++) {
         if (j != 4) {
            QPoint pos(centre.x(),centre.y()-4+j);
            pDC->drawPoint(pos);
         }
      }
   }
}

int QDepthmapView::GetSpacer(QGraphDoc *pDoc)
{
   int spacer = 1;
   int viewclass = pDoc->m_meta_graph->getViewClass();
   if (viewclass & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA)) {
      spacer = int(ceil(5.0 * pDoc->m_meta_graph->getDisplayedPointMap().getSpacing() / (m_unit * 10.0) ));
   }
   else if (viewclass & MetaGraph::VIEWAXIAL) {
      spacer = int(ceil(pDoc->m_meta_graph->getDisplayedShapeGraph().getSpacing() / (m_unit * 10.0) ));
   }
   else if (viewclass & MetaGraph::VIEWDATA) {
      spacer = int(ceil(pDoc->m_meta_graph->getDisplayedDataMap().getSpacing() / (m_unit * 10.0) ));
   }
   return spacer;
}

void QDepthmapView::DrawLine(QPainter *pDC, QRect& line, bool drawit)
{
   if (line.width() == 0 && line.height() == 0) {
      return;
   }
   if (drawit) {
      m_line_pixels.clear();
   }
   else if (!m_line_pixels.size()) {
      return; // shouldn't happen, but does appear to
   }
   bool wide = (abs(line.width()) > abs(line.height()));
   pDC->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
   pDC->setPen(QColor(m_foreground));

   if (wide) {
      int step = (line.width() > 0) ? 1 : -1;
      for (int i = 0; (i * step) < abs(line.width()); i += step) {
         QPoint thispixel(i, int((float(line.height())/float(line.width())) * float(i)));
         if (drawit) {
            m_line_pixels.push_back(m_foreground);
            pDC->drawPoint(line.topLeft() + thispixel);
         }
         else {
            pDC->drawPoint(line.topLeft() + thispixel);
         }
      }
   }
   else {
      int step = (line.height() > 0) ? 1 : -1;
      for (int i = 0; (i * step) < abs(line.height()); i += step) {
         QPoint thispixel(int((float(line.width())/float(line.height())) * float(i)), i);
         if (drawit) {
            m_line_pixels.push_back(m_foreground);
            pDC->drawPoint(line.topLeft() + thispixel);
         }
         else {
            pDC->drawPoint(line.topLeft() + thispixel);
         }
      }
   }
   if (!drawit) {
      m_line_pixels.clear();
   }
   pDC->setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void QDepthmapView::SetCursor(int mode)
{
   switch (mode)
   {
   case SELECT:
      m_cursor = QCursor(Qt::ArrowCursor);
      break;
   case SELECT | OVERHANDLE:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00005.png"));
	  break;
   case DRAG:
      m_cursor = QCursor(Qt::OpenHandCursor);
      break;
   case ZOOM_IN:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00008.png"));
      break;
   case ZOOM_OUT:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00009.png"));
      break;
   case SEEDISOVIST:
   case SEEDHALFOVIST:
   case SEEDAXIAL:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00003.png"));
      break;
   case FILL:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00001.png"));
      break;
   case PENCIL:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00002.png"));
      break;
   case LINETOOL:
   case LINETOOL | DRAWLINE:
   case POLYGONTOOL:
   case POLYGONTOOL | DRAWLINE:
   case SELECT | DRAWLINE:
      m_cursor = QCursor(Qt::CrossCursor);
      break;
   case ERASE:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00006.png"));
      break;
   case JOIN:
   case UNJOIN:
      m_cursor = QCursor(Qt::PointingHandCursor);
      break;
   case JOIN | JOINB:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00004.png"));
      break;
   case UNJOIN | JOINB:
      m_cursor = QCursor(QPixmap(":/images/cur/cur00007.png"));
      break;
   default:
      m_cursor = QCursor(Qt::ArrowCursor);
      break;
   }
   setCursor(m_cursor);
}

// Zoom to Selection
void QDepthmapView::OnViewZoomsel()
{
   if (m_pDoc.m_meta_graph && m_pDoc.m_meta_graph->isSelected()) {
      QtRegion sel_bounds = m_pDoc.m_meta_graph->getSelBounds();
      // select a suitable zoom factor based on bounding box dimensions:
      m_centre = sel_bounds.getCentre();
	  QRect phys_bounds = this->rect();
      if (sel_bounds.area() > 1e-9) {
         // base area on selection area
         m_unit =  1.1 * __max( sel_bounds.width() / double(phys_bounds.width()), 
                               sel_bounds.height() / double(phys_bounds.height()) );
      }
      else {
         // base area on some arbitrary zoom into the map
         QtRegion map_bounds = m_pDoc.m_meta_graph->getBoundingBox();
         m_unit = 0.01 * __max( map_bounds.width() / double(phys_bounds.width()), 
                                map_bounds.height() / double(phys_bounds.height()) );
      }
      // Redraw scene
      m_redraw_all = true;
      update();
   }
}

void QDepthmapView::OnViewPan()
{
   m_mouse_mode = DRAG;
   SetCursor(DRAG);
}

void QDepthmapView::OnViewZoomIn() 
{
	m_mouse_mode = ZOOM_IN;
	SetCursor(ZOOM_IN);
}

void QDepthmapView::OnViewZoomOut() 
{
	m_mouse_mode = ZOOM_OUT;
	SetCursor(ZOOM_OUT);
}

void QDepthmapView::OnEditSelect() 
{
   m_mouse_mode = SELECT;
   SetCursor(SELECT);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnModeIsovist() 
{
   m_mouse_mode = SEEDISOVIST;
   SetCursor(SEEDISOVIST);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnModeTargetedIsovist()
{
   m_mouse_mode = SEEDHALFOVIST;
   SetCursor(SEEDHALFOVIST);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnModeSeedAxial()
{
   m_mouse_mode = SEEDAXIAL;
   SetCursor(SEEDAXIAL);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnEditFill() 
{
   m_mouse_mode = FILL;
   m_fillmode = FULLFILL;
   SetCursor(FILL);
}

void QDepthmapView::OnEditSemiFill() 
{
   m_mouse_mode = FILL;
   m_fillmode = SEMIFILL;
   SetCursor(FILL);
}

// AV TV
void QDepthmapView::OnEditAugmentFill()
{
   m_mouse_mode = FILL;
   m_fillmode = AUGMENT;
   SetCursor(FILL);
}

void QDepthmapView::OnEditPencil() 
{
   m_mouse_mode = PENCIL;
   SetCursor(PENCIL);
}

void QDepthmapView::OnEditEraser() 
{
   m_mouse_mode = ERASE;
   SetCursor(ERASE);
}

void QDepthmapView::OnEditLineTool() 
{
   m_mouse_mode = LINETOOL;
   SetCursor(LINETOOL);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnEditPolygonTool()
{
   m_mouse_mode = POLYGONTOOL;
   SetCursor(POLYGONTOOL);
   if (m_showlinks) {
      m_showlinks = false;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnModeJoin() 
{
   if (m_pDoc.m_meta_graph->getState() & (MetaGraph::POINTMAPS | MetaGraph::SHAPEGRAPHS)) {
      m_mouse_mode = JOIN;
      if (!m_pDoc.m_meta_graph->isSelected()) {
         SetCursor(m_mouse_mode);
      }
      else {
         BeginJoin();
      }
      // Redraw scene
      m_showlinks = true;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnModeUnjoin() 
{
   if (m_pDoc.m_meta_graph->getState() & (MetaGraph::POINTMAPS | MetaGraph::SHAPEGRAPHS)) {
      m_mouse_mode = UNJOIN;
      if (!m_pDoc.m_meta_graph->isSelected()) {
         SetCursor(m_mouse_mode);
      }
      else {
         if (m_pDoc.m_meta_graph->viewingProcessedPoints()) {
            m_pDoc.m_meta_graph->clearSel();
         }
         else {
            m_mouse_mode |= JOINB;
            SetCursor(m_mouse_mode);
         }
      }
      // Redraw scene
      m_showlinks = true;
      m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_MAP,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
   }
}

void QDepthmapView::OnEditCopy()
{
    QRect rectin = QRect(0,0,width(),height());
    int state = m_pDoc.m_meta_graph->getState();

    if (state & MetaGraph::POINTMAPS && (!m_pDoc.m_meta_graph->getDisplayedPointMap().isProcessed() || m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA))) {
        m_pDoc.m_meta_graph->getDisplayedPointMap().setScreenPixel( m_unit ); // only used by points (at the moment!)
        m_pDoc.m_meta_graph->getDisplayedPointMap().makeViewportPoints( LogicalViewport(rectin, &m_pDoc) );
    }
    if (state & MetaGraph::SHAPEGRAPHS && (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWBACKAXIAL | MetaGraph::VIEWAXIAL))) {
        m_pDoc.m_meta_graph->getDisplayedShapeGraph().makeViewportShapes( LogicalViewport(rectin, &m_pDoc) );
    }
    if (state & MetaGraph::DATAMAPS && (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWBACKDATA | MetaGraph::VIEWDATA))) {
        m_pDoc.m_meta_graph->getDisplayedDataMap().makeViewportShapes( LogicalViewport(rectin, &m_pDoc) );
    }
    if (state & MetaGraph::LINEDATA) {
        m_pDoc.m_meta_graph->SuperSpacePixel::makeViewportShapes( LogicalViewport(rectin, &m_pDoc) );
    }

   // Copy to Clipboard
    QPixmap image(width(), height());
    QPainter painter;
    painter.begin(&image);           // paint in picture

    Output(&painter, &m_pDoc, false);
    painter.end();                     // painting done

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setPixmap(image);

}

void QDepthmapView::OnEditSave()
{
   // Very similar to copy to clipboard, only writes an EPS instead of a WMF
   if (m_pDoc.m_communicator) {
       QMessageBox::warning(this, tr("Warning"), tr("Another Depthmap process is running, please wait until it completes"), QMessageBox::Ok, QMessageBox::Ok);
      return;
   }

   QString saveas;
   QFilePath path(windowFilePath());
//   saveas = path.m_path + (path.m_name.isEmpty() ? windowTitle() : path.m_name);
   saveas = path.m_path + tr("*.eps");

   QString template_string = tr("Encapsulated Postscript (*.eps)\nScalable Vector Graphics (*.svg)\nAll files (*.*)");

   QFileDialog::Options options = 0;
   QString selectedFilter;
   QString outfile = QFileDialog::getSaveFileName(
       0, tr("Save Screen As"),
       saveas,
       template_string,
       &selectedFilter,
       options);

   if(outfile.isEmpty()) return;

  FILE* fp = fopen(outfile.toLatin1(), "wb");
  fclose(fp);

  std::ofstream stream( outfile.toLatin1() );
  if (stream.fail()) {
     QMessageBox::warning(this, tr("Warning"), tr("Sorry, unable to open ") + outfile + tr(" for writing"), QMessageBox::Ok, QMessageBox::Ok );
     return;
  }

    QFilePath newpath(outfile);

	QString ext = newpath.m_ext.toLower();
	if (ext == "svg") {
       OutputSVG( stream, &m_pDoc );
	}
	else {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "", "Would you like to include a scale?",
                                      QMessageBox::Yes|QMessageBox::No);
        bool includeScale = (reply == QMessageBox::Yes);
	 // Set up complete... run the .eps outputter (copied from standard output!)
       OutputEPS( stream, &m_pDoc, includeScale );
	}
    stream.close();
}

void QDepthmapView::closeEvent(QCloseEvent *event)
{
    m_pDoc.m_view[QGraphDoc::VIEW_MAP] = NULL;
    if (!m_pDoc.OnCloseDocument(QGraphDoc::VIEW_MAP))
	{
        m_pDoc.m_view[QGraphDoc::VIEW_MAP] = this;
		event->ignore();
	}
}


static std::string SVGColor(PafColor color)
{
   std::stringstream text;
   int r = color.redb();
   int g = color.greenb();
   int b = color.blueb();
   text << std::setfill('0') << "#" << std::setw(2) << std::hex << r << std::setw(2) << std::hex << g<< std::setw(2) << std::hex << b;
   return text.str();
}

static QPoint SVGPhysicalUnits(const Point2f& p, const QtRegion& r, int h)
{
   // converts to a 4800 unit wide QtRegion
   return QPoint(
      int(4800.0 * ((p.x-r.bottom_left.x)/r.width())),h - int(4800.0 * ((p.y-r.bottom_left.y)/r.width()))
      );
}

void QDepthmapView::OutputSVG( std::ofstream& stream, QGraphDoc *pDoc )
{
   // This output SVG is a copy of the standard output... obviously, if you change
   // standard output, remember to change this one too!
   
   // also out of synch with EPS!

   // now the two are a little out of synch

   if (!m_viewport_set) {
	   QMessageBox::warning(this, tr("Depthmap"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
      return;
   }

   QRect rect = QRect(0,0,width(), height());

   // we'll make this 24cm wide whatever, and base the height on it:
   int h = (4800 * rect.height()) / rect.width();

   stream << "<?xml version=\"1.0\" standalone=\"no\"?>" << std::endl;
   stream << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << std::endl;
   stream << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << std::endl;
   stream << "<svg width=\"24cm\" height=\"" << (h/200) << "cm\" viewBox=\"0 0 4800 " << h << "\"" << std::endl;
   stream << "xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\">" << std::endl;
   stream << "<desc>" << TITLE_BASE << "</desc>" << std::endl;

   // note, SVG draw completely overrides standard draw physical units to achieve hi-res output
   // (EPS should probably follow this model too)
   QtRegion logicalviewport = LogicalViewport(rect, pDoc);

   stream << "<rect x=\"0\" y=\"0\" width=\"4800\" height=\"" << h << "\" "
          << "fill=\"" << SVGColor(m_background) << "\" stroke=\"none\" stroke-width=\"0\" />" << std::endl;

   int state = pDoc->m_meta_graph->getState();

   if (state & MetaGraph::POINTMAPS) {
      pDoc->m_meta_graph->getDisplayedPointMap().setScreenPixel( m_unit ); // only used by points (at the moment!)
      pDoc->m_meta_graph->getDisplayedPointMap().makeViewportPoints( logicalviewport );
   }
   if (state & MetaGraph::SHAPEGRAPHS) {
      pDoc->m_meta_graph->getDisplayedShapeGraph().makeViewportShapes( logicalviewport );
   }
   if (state & MetaGraph::DATAMAPS) {
      pDoc->m_meta_graph->getDisplayedDataMap().makeViewportShapes( logicalviewport );
   }
   if (state & MetaGraph::LINEDATA) {
      pDoc->m_meta_graph->SuperSpacePixel::makeViewportShapes( logicalviewport );
   }

   if (state & MetaGraph::POINTMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {

      PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

      double spacing = map.getSpacing();
      double spacer = 4800.0 * (spacing/logicalviewport.width()) / 2.0;

      stream << "<g stroke=\"none\">" << std::endl;

      stream << "<defs><rect id=\"a\" width=\"" << 2 * spacer << "\" height=\"" << 2 * spacer << "\" /></defs>" << std::endl;

      while ( map.findNextPoint() ) {

         Point2f logical = map.getNextPointLocation();
         PafColor color = map.getCurrentPointColor();
         
         if (color.alphab() != 0) { // alpha == 0 is transparent

            QPoint p = SVGPhysicalUnits(logical,logicalviewport,h);

            stream << "<use fill=\"" << SVGColor(color) << "\" x=\"" << p.x() - spacer << "\" y=\"" << p.y() - spacer << "\" xlink:href=\"#a\" />" << std::endl;
         }
      }
      stream << "</g>" << std::endl;
   }

   if (state & MetaGraph::SHAPEGRAPHS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {

      ShapeMap& map = pDoc->m_meta_graph->getDisplayedShapeGraph();

      OutputSVGMap(stream, map, logicalviewport, h);
   }

   if (state & MetaGraph::DATAMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {

      ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();

      OutputSVGMap(stream, map, logicalviewport, h);
   }

   if (state & MetaGraph::LINEDATA) {
      // arbitrary stroke width for now
      stream << "<g stroke-width=\"4\" fill=\"none\" stroke=\"" << SVGColor(m_foreground) << "\">" << std::endl;
      bool nextlayer = false;
      while ( pDoc->m_meta_graph->SuperSpacePixel::findNextShape(nextlayer) ) {
         const SalaShape& shape = pDoc->m_meta_graph->SuperSpacePixel::getNextShape();
         Line l;
         if (shape.isPoint()) {
         }
         else if (shape.isLine()) {
            Line line = shape.getLine();
            OutputSVGLine(stream, line, logicalviewport, h);
         }
         else {
            OutputSVGPoly(stream, shape, logicalviewport, h);
         }
      }
      stream << "</g>" << std::endl;
   }

   stream << "</svg>" << std::endl;
}


void QDepthmapView::OutputSVGMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, int h)
{
   bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);
   // monochrome not implemented yet!
   if (monochrome) {
      QMessageBox::warning(this, tr("Depthmap"), tr("This map is displaying in monochrome, which is not yet supported.  Please note your SVG file will be empty.  Sorry for the inconvenience."), QMessageBox::Ok, QMessageBox::Ok);
      return;
   }

   // I haven't implemented all of these, but I hope I will get there:
   bool showlines, showfill, showcentroids;
   map.getPolygonDisplay(showlines,showfill,showcentroids);

   // arbitrary stroke width for now
   stream << "<g stroke-width=\"4\">" << std::endl;

   PafColor color, oldcolor;
   bool closed, oldclosed;

   bool first = true;
   bool dummy;
   while ( map.findNextShape(dummy) ) {

      // note: getNextLine clears current line, so getLineColor before line
      color = map.getShapeColor();
      const SalaShape& shape = map.getNextShape();
      closed = shape.isClosed();

      if (first || color != oldcolor || closed != oldclosed) {
         if (!first) {
            stream << "</g>" << std::endl;
         }
         else {
            first = false;
         }
         if (closed) {
            if (showlines) {
               stream << "<g fill=\"" << SVGColor(color) << "\" stroke=\"" << SVGColor(m_foreground) << "\">" << std::endl;
            }
            else {
               stream << "<g fill=\"" << SVGColor(color) << "\" stroke=\"none\">" << std::endl;
            }
         }
         else {
            stream << "<g fill=\"none\" stroke=\"" << SVGColor(color) << "\">" << std::endl;
         }
         oldcolor = color;
         oldclosed = closed;
      }

      Line l;
      if (shape.isPoint()) {
      }
      else if (shape.isLine()) {
         Line line = shape.getLine();
         OutputSVGLine(stream, line, logicalviewport, h);
      }
      else {
         OutputSVGPoly(stream, shape, logicalviewport, h);
      }
   }
   if (!first) {
      stream << "</g>" << std::endl;
   }

   stream << "</g>" << std::endl;
}

void QDepthmapView::OutputSVGLine(std::ofstream& stream, Line& line, QtRegion& logicalviewport, int h)
{
   bool drewit = false;
   if (line.crop(logicalviewport)) {
      QPoint start = SVGPhysicalUnits(line.start(),logicalviewport,h);
      QPoint end = SVGPhysicalUnits(line.end(),logicalviewport,h);
      // 2.0 is about 0.1mm in a standard SVG output size
      if (dist(Point2f(start.x(), start.y()), Point2f(end.x(), end.y())) >= 2.4f) {
         stream << "<line x1=\"" << start.x() << "\" y1=\"" << start.y() << "\""
                << " x2=\"" << end.x() << "\" y2=\"" << end.y() << "\" />" << std::endl;
      }
   }
}

void QDepthmapView::OutputSVGPoly(std::ofstream& stream, const SalaShape& shape, QtRegion& logicalviewport, int h)
{
   QPoint bl = SVGPhysicalUnits(shape.getBoundingBox().bottom_left,logicalviewport,h);
   QPoint tr = SVGPhysicalUnits(shape.getBoundingBox().top_right,logicalviewport,h);
   if (dist(Point2f(bl.x(), bl.y()), Point2f(tr.x(), tr.y())) < 2.0f) {
      // 2.0 is about 0.1mm in standard SVG output size -- if this is too small, we won't bother trying to draw it at all
      return;
   }
   if (shape.isOpen()) {
      // open lines are fairly easy: simply chop lines as they enter and exit
      stream << "<polyline points=\"";
      bool starter = true, drawn = false;
      Point2f lastpoint = shape.m_points.front();
      auto iter = shape.m_points.begin();
      iter++;
      for (; iter != shape.m_points.end(); iter++) {
         Line line(lastpoint, *iter);
         if (line.crop(logicalviewport)) {
            // note: use t_start and t_end so that this line moves in the correct direction
            QPoint start = SVGPhysicalUnits(line.t_start(),logicalviewport,h);
            QPoint end = SVGPhysicalUnits(line.t_end(),logicalviewport,h); 
            // always draw either from the first point or whenever you enter the viewport:
            if (starter) {
               stream << start.x() << "," << start.y() << " ";
               starter = false;
            }
            // 2.0 is about 0.1mm in a standard SVG output size
            if (dist(Point2f(start.x(), start.y()), Point2f(end.x(), end.y())) >= 2.0f ||  iter == shape.m_points.end() - 1) {
				// also, always draw the very last point regardless of distance
               stream << end.x() << "," << end.y() << " ";
               drawn = true;
            }
         }
         else {
            starter = true;
            drawn = true;
         }
         if (drawn) {
            lastpoint = *iter;
            drawn = false;
         }
      }
      stream << "\" />" << std::endl;
   }
   else {
      // polygons are hard... have to work out entry and exit points to the clipping frame
      // and wind according to their direction
      stream << "<polygon points=\"";
      pqvector<SalaEdgeU> eus = shape.getClippingSet(logicalviewport);
      if (eus.size() == 0) {
         // this should be a shape that is entirely within the viewport:
         QPoint last = SVGPhysicalUnits(shape.m_points[0],logicalviewport,h);
         stream << last.x() << "," << last.y() << " ";
         auto iter = shape.m_points.begin();
         iter++;
         for (; iter != shape.m_points.end(); iter++) {
            QPoint next = SVGPhysicalUnits(*iter,logicalviewport,h);
            if (dist(Point2f(last.x(), last.y()), Point2f(next.x(), next.y())) >= 2.0f) {
               stream << next.x() << "," << next.y() << " ";
               last = next;
            }
         }
      }
      else if (eus.size() == 1) {
         // dummy: getClippingSet deliberately adds a single empty EdgeU if the polygon is completely outside the frame
         // (this can happen when a polygon wraps around the frame)
      }
      else if (eus.size() >= 2) {
         int entry = eus[0].entry ? 0 : 1;
         // this can get very messy (sometimes have to split into separate polys)... here's hoping for the best
         while (entry < (int)eus.size()) {
            int exit = int((entry + 1) % eus.size());
            Point2f pt = logicalviewport.getEdgeUPoint(eus[entry]);
            QPoint last = SVGPhysicalUnits(pt,logicalviewport,h);
            QPoint next;
            stream << last.x() << "," << last.y() << " ";
            for (size_t i = eus[entry].index + 1; i != eus[exit].index; i++) {
               if (i >= shape.m_points.size()) {
                  i = 0;
               }
               next = SVGPhysicalUnits(shape.m_points[i],logicalviewport,h);
               if (dist(Point2f(last.x(), last.y()), Point2f(next.x(), next.y())) >= 2.0f) {
                  stream << next.x() << "," << next.y() << " ";
                  last = next;
               }
            }
            pt = logicalviewport.getEdgeUPoint(eus[exit]);
            last = SVGPhysicalUnits(pt,logicalviewport,h);
            stream << last.x() << "," << last.y() << " ";
            bool breakup = false;
            if (entry + 2 < (int)eus.size() && ccwEdgeU(eus[entry],eus[entry+1],eus[entry+2]) != shape.isCCW()) {
               breakup = true;
            }
            EdgeU& nextentry = breakup ? eus[entry] : eus[(exit+1)%eus.size()];
            if (shape.isCCW()) {
               if (nextentry.edge != eus[exit].edge || nextentry.u < eus[exit].u) {
                  int edge = eus[exit].edge;
                  do {
                     edge++;
                     if (edge > 3) {
                        edge = 0;
                     }
                     next = SVGPhysicalUnits( logicalviewport.getEdgeUPoint(EdgeU(edge,0)),logicalviewport,h);
                     stream << next.x() << "," << next.y() << " ";
                  }
                  while (edge != nextentry.edge);
               }
            }
            else {
               if (nextentry.edge != eus[exit].edge || nextentry.u > eus[exit].u) {
                  int edge = eus[exit].edge;
                  do {
                     edge--;
                     if (edge < 0) {
                        edge = 3;
                     }
                     next = SVGPhysicalUnits( logicalviewport.getEdgeUPoint(EdgeU(edge,1)),logicalviewport,h);
                     stream << next.x() << "," << next.y() << " ";
                  }
                  while (edge != nextentry.edge);
               }
            }
            if (breakup) {
            //if (entry + 2 < eus.size() && ccwEdgeU(eus[entry],eus[entry+1],eus[entry+2]) != shape.isCCW()) {
               stream << "\" />" << std::endl;
               stream << "<polygon points=\"";
            }
            entry += 2;
         }
      }
      stream << "\" />" << std::endl;
   }
}

void QDepthmapView::OnViewZoomToRegion(QtRegion regionToZoomAt) {

    m_centre = regionToZoomAt.getCentre();
    QRect phys_bounds = this->rect();
       m_unit =  1.0 * __max( regionToZoomAt.width() / double(phys_bounds.width()),
                             regionToZoomAt.height() / double(phys_bounds.height()) );
}
