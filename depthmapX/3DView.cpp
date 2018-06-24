// depthmapX - spatial network analysis platform
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


// 3DView.cpp : implementation file
//

#include <QtGui>
#include <QtOpenGL>
#include <QTimer>
#include <QtWidgets/QFileDialog>
#include <genlib/xmlparse.h>
#include "mainwindow.h"
#include "3DView.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifndef _WIN32 // Not working?
#define LOBYTE(w)           ((unsigned char)((w) & 0xff))
#define GetRValue(rgb)      (LOBYTE(rgb))
#define GetGValue(rgb)      (LOBYTE((rgb) >> 8))
#define GetBValue(rgb)      (LOBYTE((rgb)>>16))
#else
#define finite _finite
#endif

#define ID_ADD_AGENT                    32947
#define ID_3D_PAN                       32948
#define ID_3D_ZOOM                      32949
#define ID_3D_ROT                       32950
#define ID_3D_PAUSE                     32951
#define ID_3D_PLAY_LOOP                 32981
#define ID_3D_FILLED                    32983

/////////////////////////////////////////////////////////////////////////////
// Q3DView

Q3DView::Q3DView(QWidget *parent, QGraphDoc* doc)
    : QOpenGLWidget(parent)
{
   m_points = NULL;
   m_pointcount = 0;
   m_mouse_mode = ID_3D_ROT;//resouce
   m_mouse_mode_on = 0;
   m_key_mode_on = 0;
   m_quick_draw = false;
   m_animating = false;
   m_drawtrails = false;
   m_fill = true;
   m_track = 0.0;
   m_right_mouse = false;

   // Quick mod - TV
   m_panx = m_pany = m_rotx = m_roty = 0.0f;
   m_zoom = 1.0f;
   m_roty = 60.0f;
   QTimer* m_Timer = new QTimer(this);
   connect(m_Timer, SIGNAL(timeout()), this, SLOT(timerSlot()));
   setWindowIcon(QIcon(tr(":/images/cur/icon-1-3.png")));
   pDoc = doc;
   setWindowTitle(pDoc->m_base_title+":3D View");
   m_Timer->start(20);
   grabKeyboard();
   pDoc->m_redraw_flag[QGraphDoc::VIEW_3D] = QGraphDoc::REDRAW_TOTAL;
   OnRedraw(0, 0);
   setMouseTracking(true);
}

Q3DView::~Q3DView()
{
    // Quick mod - TV
    releaseKeyboard();
}

/////////////////////////////////////////////////////////////////////////////
// Q3DView drawing
// void Q3DView::paintEvent(QPaintEvent *event)
void Q3DView::paintGL()
{
   OnRedraw(0, 0);
   DrawScene();
}

int Q3DView::OnRedraw(int wParam, int lParam)
{
   int flag = pDoc->GetRedrawFlag(QGraphDoc::VIEW_3D);
   if (flag != QGraphDoc::REDRAW_DONE) 
   {
      //
      while (!pDoc->SetRedrawFlag(QGraphDoc::VIEW_3D,QGraphDoc::REDRAW_DONE)) 
	  {
         // prefer waitformultipleobjects here
         //sleep(1);
      }
      //
      if (flag == QGraphDoc::REDRAW_TOTAL)
	  {
         qWarning("Reload\n");
         ReloadLineData();
         ReloadPointData();
         m_quick_draw = false;
      }
   }

   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Q3DView message handlers

void Q3DView::timerEvent(QTimerEvent *event)
{
   if (m_mouse_mode == ID_3D_PLAY_LOOP) {
      PlayLoop();
      SetModelMat();
      m_quick_draw = true;
      if (!m_animating) {  // if animating will redraw below
         DrawScene();
      }
      update();
   }
   else if (m_key_mode_on) {
      QSize diff(0,0);
      switch (m_keydown) {
      /*case 37:*/ case Qt::Key_Left: diff.setWidth(-2); break;
      /*case 38:*/ case Qt::Key_Up: diff.setHeight(-2); break; // up arrow: note, as per MS reversed y
      /*case 39:*/ case Qt::Key_Right: diff.setWidth(2); break;
      /*case 40:*/ case Qt::Key_Down: diff.setHeight(2);
      }
      switch (m_key_mode_on) {
      case ID_3D_ROT:
         Rot(diff);
         break;
      case ID_3D_PAN:
         Pan(diff);
         break;
      case ID_3D_ZOOM:
         Zoom(diff);
         break;
      }
      SetModelMat();
      m_quick_draw = true;
      if (!m_animating) {  // if animating will redraw below
         DrawScene();
      }
   }

   if (m_animating && !pDoc->m_communicator && pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints()) {
      PointMap& pointmap = pDoc->m_meta_graph->getDisplayedPointMap();
      m_animating = false;
      for (size_t i = 0; i < m_mannequins.size(); i++) {
         m_mannequins[i].frame();
         if (m_mannequins[i].m_frame == 0) {
            if (m_mannequins[i].m_playback) {
               int j = m_mannequins[i].m_trace_id;
               Trace& trace = m_traces[j];
               if (m_mannequins[i].m_time >= trace.starttime && m_mannequins[i].m_time <= trace.endtime) {
                  // active zone:
                  m_mannequins[i].m_active = true;
                  size_t k = 0;
                  while (k < m_traces[j].events.size() && m_traces[j].events[k].t < m_mannequins[i].m_time) {
                     k++;
                  }
                  if (k < m_traces[j].events.size()) {
                     Point2f p = (Point2f) m_traces[j].events[k];
                     PixelRef pix = pointmap.pixelate(p); // note, take the pix before you scale!
                     p.normalScale(m_region);
                     m_mannequins[i].advance(p);
                     auto iter = m_pixels.find(pix);
                     if (iter != m_pixels.end()) {
                        if (iter->second.m_value < 10) {
                           iter->second.m_value += 1;
                        }
                     }
                  }
               }
               else {
                  m_mannequins[i].m_active = false;
               }
            }
            else {
               int j = m_mannequins[i].m_agent_id;
               m_agents[j].onMove();
               Point2f p = m_agents[j].getLocation();
               p.normalScale(m_region);
               m_mannequins[i].advance(p);
               //
               // pretty coloured pixels
               PixelRef pix = m_agents[j].getNode();
               auto iter = m_pixels.find(pix);
               if (iter != m_pixels.end()) {
                  if (iter->second.m_value < 10) {
                     iter->second.m_value += 1;
                  }
               }
            }
         }
      }
      m_animating = true;
      update();
   }
}

//void Q3DView::Init()
void Q3DView::initializeGL()
{
   m_oldRect = QRect(0, 0, width(), height());

   glClearDepth(1.0f);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_FLAT);
   glEnableClientState(GL_VERTEX_ARRAY);
   
}

void Q3DView::DrawScene()
{
    std::unique_lock<std::mutex> lock(m_draw_mutex, std::try_to_lock);
    if (!lock.owns_lock()){
        return;
    }
   
   makeCurrent();

   SetModelMat();

   QRgb bg = qRgb(0,0,0);
   QRgb fg = qRgb(128,128,128);

   glClearColor((GLfloat)GetRValue(bg)/255.0f,(GLfloat)GetGValue(bg)/255.0f,(GLfloat)GetBValue(bg)/255.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glColor3f(1.0f,0.0f,0.0f);
   if (m_male_template.m_init) {
      for (size_t i = 0; i < m_mannequins.size(); i++) {
         if (m_mannequins[i].m_active) {
            if (i % 2 == 0) {
               m_mannequins[i].draw(m_male_template, m_drawtrails, m_fill);
            }
            else {
               m_mannequins[i].draw(m_female_template, m_drawtrails, m_fill);
            }
         }
      }
   }

   if (m_fill) {
      if (!m_animating) {
         if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints()) {
            // okay, you can go for it and draw all the squares in cutesy 3d:
            PointMap& pointmap = pDoc->m_meta_graph->getDisplayedPointMap();
            AttributeTable& table = pointmap.getAttributeTable();
            for (int i = 0; i < table.getRowCount(); i++) {
               PixelRef pix = table.getRowKey(i);
               PafColor color;
               int col = pointmap.getDisplayedAttribute();
               float value = table.getNormValue(i,col);
               if (value != -1.0f) {
                  color.makeAxmanesque(value);
                  glColor3f(color.redf(),color.greenf(),color.bluef());
                  Point2f p = pointmap.depixelate(pix);
                  p.normalScale(m_region);
                  glPushMatrix();
                  glTranslatef(p.x,p.y,0.0f);
                  glVertexPointer(3, GL_FLOAT, 0, m_rect);
                  glDrawArrays(GL_QUADS, 0, 4);
                  glPopMatrix();
               }
            }
         }
      }
      else {
         for (auto pixel: m_pixels) {
            int& value = pixel.second.m_value;
            if (value != -1) {
               if (pafrand() % 10000 == 0) {
                  value--;
               }
               PafColor color;
               color.makeAxmanesque(float(value)/10.0f);
               glColor3f(color.redf(),color.greenf(),color.bluef());
               Point2f& p = pixel.second.m_point;
               glPushMatrix();
               glTranslatef(p.x,p.y,0.0f);
               glVertexPointer(3, GL_FLOAT, 0, m_rect);
               glDrawArrays(GL_QUADS, 0, 4);
               glPopMatrix();
            }
         }
      }
   }

   glColor3f((GLfloat)GetRValue(fg)/255.0f,(GLfloat)GetGValue(fg)/255.0f,(GLfloat)GetBValue(fg)/255.0f);
   if (m_pointcount) {
      glVertexPointer(3, GL_FLOAT, 0, m_points);
      glDrawArrays(GL_LINES, 0, m_pointcount);
   }

   glFlush();

#if defined(_WIN32)
   SwapBuffers(wglGetCurrentDC());
#else
    ;
#endif
}

void Q3DView::Reshape(int x, int y)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0f, (GLfloat) x / (GLfloat) y, 0.1f, 3.0f);
   // leave matrix mode in model view:
   glMatrixMode(GL_MODELVIEW);
}

void Q3DView::OnRecentreView() 
{
    SetModelMat();

}

void Q3DView::SetModelMat()
{
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(-m_panx, m_pany, -m_zoom);
   glRotatef(-m_roty, 1.0f, 0.0f, 0.0f);
   glRotatef(-m_rotx, 0.0f, 0.0f, 1.0f);
   glTranslatef(-0.5f, -0.5f, 0.0f);
}

void Q3DView::ReloadLineData()
{
   if (m_points) {
      delete [] m_points;
      m_points = NULL;
   }
   m_region = QtRegion();

   if (pDoc->m_meta_graph && pDoc->m_meta_graph->getState() & MetaGraph::LINEDATA) {
      // should really check communicator is not open...
      auto mgraphLock = pDoc->m_meta_graph->getLock();
      std::unique_lock<std::mutex> drawLock(m_draw_mutex);

      SuperSpacePixel& superspacepix = *(pDoc->m_meta_graph);

      prefvec<Line> lines;
      for (const auto& pixelGroup: superspacepix.m_spacePixels) {
         for (const auto& pixel: pixelGroup.m_spacePixels) {
            if (pixel.isShown()) {
               if (m_region.atZero()) {
                  m_region = pixel.getRegion();
               }
               else {
                  m_region = runion(m_region, pixel.getRegion());
               }

               auto refShapes = pixel.getAllShapes();
               for (const auto& refShape: refShapes) {
                  const SalaShape& shape = refShape.second;
                  if (shape.isLine()) {
                     lines.push_back(shape.getLine());
                  }
                  else if (shape.isPolyLine() || shape.isPolygon()) {
                     for (int n = 0; n < shape.m_points.size() - 1; n++) {
                        lines.push_back(Line(shape.m_points[n],shape.m_points[n+1]));
                     }
                     if (shape.isPolygon()) {
                        lines.push_back(Line(shape.m_points.back(),shape.m_points.front()));
                     }
                  }
               }
            }
         }
      }

      m_pointcount = lines.size() * 2;
      if (m_pointcount) {
         // now scale up to a nice square region around midpoint:
         if (m_region.width() > m_region.height()) {
            double oldheight = m_region.height();
            m_region.bottom_left.y -= (m_region.width()-oldheight)/2;
            m_region.top_right.y += (m_region.width()-oldheight)/2;
         }
         else {
            double oldwidth = m_region.width();
            m_region.bottom_left.x -= (m_region.height()-oldwidth)/2;
            m_region.top_right.x += (m_region.height()-oldwidth)/2;
         }

         m_points = new GLfloat [m_pointcount*3];
         for (int i = 0; i < m_pointcount; i++) {
            Point2f p;
            if (i % 2 == 0) {
               p = lines[i/2].start();
            }
            else {
               p = lines[i/2].end();
            }
            p.normalScale(m_region);
            m_points[i*3+0] = p.x;
            m_points[i*3+1] = p.y;
            m_points[i*3+2] = 0.0;
         }
      }
   }

   // note: as affects region, will also affect point data:
   ReloadPointData();
}

const GLfloat g_rect[][3] = 
{
   {-0.5f,-0.5f,-0.05f},
   {0.5f,-0.5f,-0.05f},
   {0.5f,0.5f,-0.05f},
   {-0.5f,0.5f,-0.05f}
};

void Q3DView::ReloadPointData()
{
   m_mannequins.clear();
   m_agents.clear();
   m_animating = false;

   if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints()) {
      //
      if (!m_region.atZero()) {
         GLfloat unit = pDoc->m_meta_graph->getDisplayedPointMap().getSpacing() / m_region.width();
         m_male_template.Init(unit, true);
         m_female_template.Init(unit, false);
         for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
               m_rect[i][j] = unit * g_rect[i][j];
            }
         }
      }
      else {
         m_male_template.Destroy();
         m_female_template.Destroy();
      }
      //
      m_pixels.clear();
      PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();
      AttributeTable& table = map.getAttributeTable();
      for (int i = 0; i < table.getRowCount(); i++) {
         PixelRef pix = table.getRowKey(i);
         Point2f p = map.depixelate(pix);
         p.normalScale(m_region);
         m_pixels[pix] = C3DPixelData(p);
      }
   }
   else {
      m_male_template.Destroy();
      m_female_template.Destroy();
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Q3DView::On3dPan() 
{
   m_mouse_mode = ID_3D_PAN;
}

void Q3DView::On3dRot()
{
   m_mouse_mode = ID_3D_ROT;
}

void Q3DView::On3dZoom() 
{
   m_mouse_mode = ID_3D_ZOOM;
}

void Q3DView::OnPlayLoop() 
{
   m_mouse_mode = ID_3D_PLAY_LOOP;
}

void Q3DView::OnAddAgent() 
{
   m_mouse_mode = ID_ADD_AGENT;
}

void Q3DView::OnToolsAgentLoadProgram() 
{
   QString template_string;
   template_string += "Text files (*.txt)\nAll files (*.*)";

   QFileDialog::Options options = 0;
   QString selectedFilter;
   QStringList infiles = QFileDialog::getOpenFileNames(
	   0, tr("Import"),
	   "",
	   template_string,
	   &selectedFilter,
	   options);

   if (!infiles.size()) {
	   return;
   }

   std::string filename = infiles[0].toStdString();
   
   if (!filename.empty()) {
      m_animating = false;
      std::unique_lock<std::mutex> lock(m_draw_mutex);

      m_agents.clear();
      m_mannequins.clear();
      if (!m_agent_program.open(filename)) {
		  QMessageBox::warning(this, tr("depthmapX"), tr("Unable to understand agent program"), QMessageBox::Ok, QMessageBox::Ok);
      }
   }
   else {
	   QMessageBox::warning(this, tr("depthmapX"), tr("No file selected"), QMessageBox::Ok, QMessageBox::Ok);
   }
}

void Q3DView::On3dFilled() 
{
   if (m_fill) {
      m_fill = false;
   }
   else {
      m_fill = true;
   }
   update();
}

void Q3DView::OnAgentTrails() 
{
   if (m_drawtrails) {
      m_drawtrails = false;
   }
   else {
      m_drawtrails = true;
   }
}
/////////////////////////////////////////////////////////////////////////////////////////

void Q3DView::OnLButtonDown(unsigned int nFlags, QPoint point) 
{
    m_right_mouse = false;

   switch (m_mouse_mode) {
   case ID_3D_PAN: case ID_3D_ROT: case ID_3D_ZOOM:
      m_mouse_mode_on = m_mouse_mode;
      m_mouse_origin = point;
      break;

   case ID_ADD_AGENT:
      {
         if (m_male_template.m_init) {
            CreateAgent(point);
         }
      }
      break;
   }
}

void Q3DView::OnLButtonUp(unsigned int nFlags, QPoint point) 
{
	m_mouse_mode_on = 0;
    m_quick_draw = false;

    update();
}

QSize Q3DView::sizeHint() const
{
   return QSize(2000, 2000);
}

void Q3DView::OnRButtonDown(unsigned int nFlags, QPoint point) 
{
   m_right_mouse = true;

   std::unique_lock<std::mutex> lock(m_draw_mutex);

   m_mouse_origin = point;
}

void Q3DView::OnRButtonUp(unsigned int nFlags, QPoint point) 
{
    m_right_mouse = false;
    m_quick_draw = false;

    update();
}


void Q3DView::OnMouseMove(unsigned int nFlags, QPoint point) 
{
   if (m_mouse_mode_on && point != m_mouse_origin) {
      QSize diff(m_mouse_origin.x() - point.x(), m_mouse_origin.y() - point.y());
      switch (m_mouse_mode) {
      case ID_3D_PAN: 
         Pan(diff);
         m_mouse_origin = point;
         break;
      case ID_3D_ROT:
         Rot(diff);
         m_mouse_origin = point;
         break;
      case ID_3D_ZOOM:
         Zoom(diff);
         m_mouse_origin = point;
         break;
      }
      SetModelMat();
      m_quick_draw = true;

      update();
   }
   else if (m_right_mouse && point != m_mouse_origin) {
      // always pan with right mouse
      QSize diff(m_mouse_origin.x() - point.x(), m_mouse_origin.y() - point.y());
      Pan(diff);
      m_mouse_origin = point;
      SetModelMat();
      m_quick_draw = true;

      update();
   }
}

void Q3DView::Pan(QSize diff) 
{
   m_panx += 0.005f * (diff.width());
   if (m_panx < -1.0f) {
      m_panx = -1.0f;
   }
   else if (m_panx > 1.0f) {
      m_panx = 1.0f;
   }
   m_pany += 0.005f * (diff.height());
   if (m_pany < -1.0f) {
      m_pany = -1.0f;
   }
   else if (m_pany > 1.0f) {
      m_pany = 1.0f;
   }
}

void Q3DView::Rot(QSize diff) 
{
   m_rotx += 0.5f * (diff.width());
   if (m_rotx < -180.0f) {
      m_rotx = 180.0f;
   }
   else if (m_rotx > 180.0f) {
      m_rotx = -180.0f;
   }
   m_roty += 0.5f * (diff.height());
   if (m_roty < 0.0f) {
      m_roty = 0.0f;
   }
   else if (m_roty > 90.0f) {
      m_roty = 90.0f;
   }
}

void Q3DView::Zoom(QSize diff) 
{
   m_zoom += 0.005f * (diff.height());
   if (m_zoom < 0.02f) {
      m_zoom = 0.02f;
   }
   else if (m_zoom > 2.5f) {
      m_zoom = 2.5f;
   }
}

void Q3DView::PlayLoop() 
{
   m_rotx += 0.05f;
   if (m_rotx < -180.0f) {
      m_rotx = 180.0f;
   }
   else if (m_rotx > 180.0f) {
      m_rotx = -180.0f;
   }
   m_track += 0.001 * M_PI;
   if (m_track > 2.0 * M_PI) {
      m_track -= 2.0 * M_PI;
   }
   m_roty += 0.05f * cosf(m_track);
   if (m_roty < 0.0f) {
      m_roty = 0.0f;
   }
   else if (m_roty > 75.0f) {
      m_roty = 75.0f;
   }
   m_zoom -= 0.001f * sinf(m_track);
   if (m_zoom < 0.02f) {
      m_zoom = 0.02f;
   }
   else if (m_zoom > 2.5f) {
      m_zoom = 2.5f;
   }
}

void Q3DView::CreateAgent(QPoint point)
{
   if (!m_male_template.m_init) {
      return;
   }

   std::unique_lock<std::mutex> lock(m_draw_mutex);
   bool animating = m_animating;
   m_animating = false;

   // click test
   GLint viewport[4];
   GLdouble mvmatrix[16], projmatrix[16];

   glViewport(0, 0, width(), height());
   glGetIntegerv(GL_VIEWPORT, viewport);
   Reshape(viewport[2], viewport[3]);
   SetModelMat();

   glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
   glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

   GLint realy = viewport[3] - point.y();

   GLdouble wx1, wy1, wz1, wx2, wy2, wz2;
   gluUnProject((GLdouble) point.x(), (GLdouble) realy, 0.0,
                mvmatrix, projmatrix, viewport, &wx1, &wy1, &wz1);
   gluUnProject((GLdouble) point.x(), (GLdouble) realy, 1.0,
                mvmatrix, projmatrix, viewport, &wx2, &wy2, &wz2);

   // 0 plane has to lie between wz1 and wz2:
   if (finite(wz1) && finite(wz2) && wz1 > 0 && wz2 < 0) {
      double scaling = wz1/(wz2-wz1);
      Point2f p(wx1-scaling*(wx2-wx1),wy1-scaling*(wy2-wy1));

      if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints()) {
         // okay, you can go for it and add an agent:
         PointMap& pointmap = pDoc->m_meta_graph->getDisplayedPointMap();
         p.denormalScale(m_region);
         PixelRef pix = pointmap.pixelate(p);
         if (pointmap.getPoint(pix).filled()) {
            m_agents.push_back( Agent(&m_agent_program, &pointmap) );
            m_agents.tail().onInit(pix);
            Point2f p2 = m_agents.tail().getLocation();
            p2.normalScale(m_region);
            m_mannequins.push_back( QMannequin(p2, m_agents.size()-1) );
            m_agents.tail().onMove();
            p2 = m_agents.tail().getLocation();
            p2.normalScale(m_region);
            m_mannequins.tail().advance(p2);

            m_animating = true;
         }
      }
   }

   m_animating |= animating;
}

//////////////////////////////////////////////////////////////////////////////////////

const GLfloat g_male_mannequin_points[][3] = 
{
   {0.0f,0.0f,0.3f},    //  0 top head
   {0.0f,0.0f,0.0f},    //  1 top spine
   {0.0f,0.0f,-0.7f},   //  2 base spine
   {-0.25f,0.0f,0.0f},  //  3 left shoulder
   {-0.15f,0.0f,-0.7f},  //  4 left hip
   {0.25f,0.0f,0.0f},   //  5 right shoulder
   {0.15f,0.0f,-0.7f},   //  6 right hip
   {-0.25f,0.0f,-0.8f}, //  7 bottom left arm (hung from top spine)
   {-0.15f,0.0f,0.0f},   //  8 top left leg (hung from base spine)
   {-0.15f,0.0f,-1.0f}   //  9 bottom left leg (hung from base spine)
};

const GLfloat g_female_mannequin_points[][3] = 
{
   {0.0f,0.0f,0.3f},    //  0 top head 
   {0.0f,0.0f,0.0f},    //  1 top spine
   {0.0f,0.0f,-0.7f},   //  2 base spine
   {-0.2f,0.0f,0.0f},   //  3 left shoulder
   {-0.2f,0.0f,-0.7f}, //  4 left hip
   {0.2f,0.0f,0.0f},    //  5 right shoulder
   {0.2f,0.0f,-0.7f},  //  6 right hip
   {-0.25f,0.0f,-0.8f}, //  7 bottom left arm (hung from top spine)
   {-0.2f,0.0f,0.0f},  //  8 top left leg (hung from base spine)
   {-0.1f,0.0f,-1.0f}   //  9 bottom left leg (hung from base spine)
};

CMannequinTemplate::CMannequinTemplate()
{
   m_init = false;
   m_unit = 0.0f;
}

CMannequinTemplate::~CMannequinTemplate()
{
}

void CMannequinTemplate::Init(GLfloat unit, bool male)
{
   m_unit = unit;
   m_male = male;

   for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 3; j++) {
         m_points[i][j] = m_unit * (male ? g_male_mannequin_points[i][j] : g_female_mannequin_points[i][j]);
      }
   }

   m_init = true;
}

void CMannequinTemplate::Destroy()
{
   m_init = false;
   m_unit = 0.0f;
}

QMannequin::QMannequin(const Point2f& startloc, int id, bool playback)
{
   m_left = true;
   m_paused = false;
   m_frame = 0;
   m_zrot = 0.0f;
   m_startloc = startloc;
   m_nextloc = startloc;
   m_pointcount = 0;
   m_pointstart = 0;
   m_time = 0.0;
   m_active = true;
   m_playback = playback;
   if (m_playback) {
      m_trace_id = id;
   }
   else {
      m_agent_id = id;
   }
}

QMannequin::QMannequin(const QMannequin& man)
{
   m_left = man.m_left;
   m_paused = man.m_paused;
   m_frame = man.m_frame;
   m_zrot = man.m_zrot;
   m_startloc = man.m_startloc;
   m_lastloc = man.m_lastloc;
   m_nextloc = man.m_nextloc;
   m_pointcount = man.m_pointcount;
   m_pointstart = man.m_pointstart;
   memcpy(m_points+m_pointstart,man.m_points+man.m_pointstart*3,m_pointcount*sizeof(GLfloat)*3);
   m_time = man.m_time;
   m_active = man.m_active;
   m_playback = man.m_playback;
   m_trace_id = man.m_trace_id;
   m_agent_id = man.m_agent_id;
}

QMannequin& QMannequin::operator = (const QMannequin& man)
{
   if (&man != this) {
      m_left = man.m_left;
      m_paused = man.m_paused;
      m_frame = man.m_frame;
      m_zrot = man.m_zrot;
      m_startloc = man.m_startloc;
      m_lastloc = man.m_lastloc;
      m_nextloc = man.m_nextloc;
      m_pointcount = man.m_pointcount;
      m_pointstart = man.m_pointstart;
      memcpy(m_points+m_pointstart,man.m_points+man.m_pointstart*3,m_pointcount*sizeof(GLfloat)*3);
      m_time = man.m_time;
      m_active = man.m_active;
      m_playback = man.m_playback;
      m_trace_id = man.m_trace_id;
      m_agent_id = man.m_agent_id;
   }
   return *this;
}

void QMannequin::frame()
{
   m_frame++;
   if (m_frame >= 24) {
      m_frame = 0;
      m_time += 1.0;
      m_left = m_left ? false : true;
   }
}

void QMannequin::advance(const Point2f& nextloc)
{
   m_lastloc = m_nextloc;
   m_nextloc = nextloc;
   if (m_nextloc == m_lastloc) {
      m_paused = true;
   }
   else {
      Point2f vec = m_nextloc - m_lastloc;
      vec.normalise();
      m_zrot = 90.0 + 180.0 * vec.angle() / M_PI;
      m_paused = false;
      //
      m_pointcount++;
      if (m_pointcount > 25) {
         m_pointstart++;
         m_pointcount = 25;
         if (m_pointstart > 25) {
            memcpy(m_points,m_points + (m_pointstart-1)*3, (m_pointcount-1)*sizeof(GLfloat)*3);
            m_pointstart = 0;
            m_pointcount = 25;
         }
      }
      int base = (m_pointstart+m_pointcount-1) * 3;
      m_points[base] = m_lastloc.x;
      m_points[base+1] = m_lastloc.y;
      m_points[base+2] = 0.0f;
   }
}

void QMannequin::draw(CMannequinTemplate& templ, bool drawtrails, bool highlight)
{
   if (!highlight) {
      if (templ.m_male) {
         glColor3f(0.4f,0.4f,0.8f);
      }
      else {
         glColor3f(0.7f,0.4f,0.6f);
      }
   }
   else {
      if (templ.m_male) {
         glColor3f(0.8f,0.8f,1.0f);
      }
      else {
         glColor3f(1.0f,0.8f,0.9f);
      }
   }

   glPushMatrix();

   // based on 24 frames per second:
   GLfloat framef = GLfloat(m_frame % 24) / 24.0f;
   if (m_paused) {
      framef = 0.0f;
   }
   glTranslatef( framef * m_nextloc.x + (1.0f - framef) * m_lastloc.x, 
                framef * m_nextloc.y + (1.0f - framef) * m_lastloc.y, 0.0f );
   glRotatef(m_zrot,0.0f,0.0f,1.0f);

   // now use framef as a swing:
   framef *= 2.0f;
   if (framef > 1.0f) {
      framef = 2.0f - framef;
   }
   GLfloat swing = framef * 30.0f * (m_left ? -1.0f : 1.0f);

   GLfloat h = cos(M_PI*0.16667f*framef) + 0.7f;  // 2 * M_PI * 30.0f * framef / 360.0f = M_PI * 0.16667 * framef

   glTranslatef( 0.0f, 0.0f, h * templ.m_unit);

   glBegin(GL_LINES);
      // head
      glVertex3fv(templ.m_points[0]);
      glVertex3fv(templ.m_points[1]);
      // spine
      glVertex3fv(templ.m_points[1]);
      glVertex3fv(templ.m_points[2]);
      // shoulders
      glVertex3fv(templ.m_points[3]);
      glVertex3fv(templ.m_points[5]);
      // hips
      glVertex3fv(templ.m_points[4]);
      glVertex3fv(templ.m_points[6]);
   glEnd();

   glPushMatrix();
   glRotatef(swing,1.0f,0.0f,0.0f);

   glBegin(GL_LINES);
      // left arm 
      glVertex3fv(templ.m_points[3]);
      glVertex3fv(templ.m_points[7]);
   glEnd();

   glRotatef(180.0f,0.0f,0.0f,1.0f);
   glRotatef(2.0*swing,1.0f,0.0f,0.0f);

   glBegin(GL_LINES);
      // right arm (n.b., reuse same point pair)
      glVertex3fv(templ.m_points[3]);
      glVertex3fv(templ.m_points[7]);
   glEnd();

   glPopMatrix();

   glTranslatef(0.0f,0.0f,-0.7f*templ.m_unit);
   glRotatef(-swing,1.0f,0.0f,0.0f);

   glBegin(GL_LINES);
      // left leg 
      glVertex3fv(templ.m_points[8]);
      glVertex3fv(templ.m_points[9]);
   glEnd();

   glRotatef(180.0f,0.0f,0.0f,1.0f);
   glRotatef(2.0*-swing,1.0f,0.0f,0.0f);

   glBegin(GL_LINES);
      // right leg (n.b., reuse same point pair)
      glVertex3fv(templ.m_points[8]);
      glVertex3fv(templ.m_points[9]);
   glEnd();

   glPopMatrix();

   // trails...
   if (drawtrails && m_pointcount > 1) {
      glVertexPointer(3, GL_FLOAT, 0, m_points);
      glDrawArrays(GL_LINE_STRIP, m_pointstart, m_pointcount);
   }
}

void Q3DView::OnKeyDown(unsigned int nChar, unsigned int nRepCnt, unsigned int nFlags) 
{
    // Quick mod - TV

    switch (nChar) {
    //case 37: case 38: case 39: case 40:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Right:
    case Qt::Key_Down:
      m_key_mode_on = m_mouse_mode;
      m_keydown = nChar;
      break;

    //case 33: // Page Up
    case Qt::Key_PageUp:
      switch (m_mouse_mode)
      {
      case ID_ADD_AGENT: m_mouse_mode = ID_3D_ZOOM; break;
      case ID_3D_ROT: m_mouse_mode = ID_ADD_AGENT; break;
      case ID_3D_PAN: m_mouse_mode = ID_3D_ROT; break;
      case ID_3D_ZOOM: m_mouse_mode = ID_3D_PAN; break;
      }
      break;

    //case 34: // Page Down
    case Qt::Key_PageDown:
      switch (m_mouse_mode)
      {
      case ID_ADD_AGENT: m_mouse_mode = ID_3D_ROT; break;
      case ID_3D_ROT: m_mouse_mode = ID_3D_PAN; break;
      case ID_3D_PAN: m_mouse_mode = ID_3D_ZOOM; break;
      case ID_3D_ZOOM: m_mouse_mode = ID_ADD_AGENT; break;
      }
      break;

    //case 'T':
    case Qt::Key_T:
      OnAgentTrails();
      break;

    //case 'A':
    case Qt::Key_A:
      {
         QPoint point;
//         ::GetCursorPos(&point);
         CreateAgent(point);
      }
      break;
   }
}

void Q3DView::OnKeyUp(unsigned int nChar, unsigned int nRepCnt, unsigned int nFlags) 
{
    m_key_mode_on = 0;
}

bool Q3DView::OnMouseWheel(unsigned int nFlags, short zDelta, QPoint pt) 
{
   QSize diff(0,-zDelta/5);
   Zoom(diff);
   SetModelMat();
   m_quick_draw = true;
   update();

   return 0;
}

void Q3DView::OnToolsImportTraces()
{
   QString template_string;
   template_string += "XML files (*.xml)\nText files (*.txt)\nAll files (*.*)";

   QFileDialog::Options options = 0;
   QString selectedFilter;
   QStringList infiles = QFileDialog::getOpenFileNames(
	   0, tr("Import Traces"),
	   "",
	   template_string,
	   &selectedFilter,
	   options);

   if (!infiles.size()) {
	   return;
   }

   std::string filename = infiles[0].toStdString();
   
   if (!filename.empty()) {
      m_animating = false;
      std::unique_lock<std::mutex> lock(m_draw_mutex);
      m_agents.clear();
      m_traces.clear();
      m_mannequins.clear();
            //
      ifstream file(filename.c_str());
      // Eva's XMLs do not have the header yet:
      xmlelement traceset;
      QString elementname;
      while (file && elementname != "traceset") {
         traceset.parse(file,false);
         elementname = QString(traceset.name.c_str()).toLower();
      }
      while (file) {
         xmlelement trace;
         trace.parse(file,true);
         elementname = QString(trace.name.c_str()).toLower();
         if (elementname == "trace") {
            m_traces.push_back(Trace());
            bool firstevent = true;
            for (int j = 0; j < trace.subelements.size(); j++) {
               // these should be events:
               xmlelement& traceevent = trace.subelements[j];
               if (traceevent.name == "event") {
                  double x = QString(traceevent.attributes["x"].c_str()).toDouble();
                  double y = QString(traceevent.attributes["y"].c_str()).toDouble();
                  double t = QString(traceevent.attributes["t"].c_str()).toDouble();
                  m_traces.tail().events.push_back(Event2f(x,y,t));
                  if (firstevent) {
                     m_traces.tail().starttime = t;
                     firstevent = false;
                  }
               }
            }
            if (m_traces.tail().events.size() >= 1) {
               m_traces.tail().endtime = m_traces.tail().events.tail().t;
               Point2f p = m_traces.tail().events[0];
               p.normalScale(m_region);
               m_mannequins.push_back( QMannequin(p,m_traces.size()-1,true) );
               m_mannequins.tail().m_active = false;
            }
         }
      }
   }
   else {
	   QMessageBox::warning(this, tr("depthmapX"), tr("No file selected"), QMessageBox::Ok, QMessageBox::Ok);
   }
}

void Q3DView::OnToolsAgentsPause()
{
   m_animating = false;
}

void Q3DView::OnToolsAgentsStop()
{
   m_animating = false;
   for (int i = 0; i < m_mannequins.size(); i++) {
      if (m_mannequins[i].m_playback) {
         m_mannequins[i].m_active = false;
         m_mannequins[i].m_time = 0.0;
         m_mannequins[i].m_nextloc = m_mannequins[i].m_startloc;
      }
   }
   for (auto pixel: m_pixels) {
      pixel.second.m_value = -1;
   }
}

void Q3DView::OnToolsAgentsPlay()
{
   m_animating = true;
}

void Q3DView::closeEvent(QCloseEvent *event)
{
   pDoc->m_view[QGraphDoc::VIEW_3D] = NULL;
   if (!pDoc->OnCloseDocument(QGraphDoc::VIEW_3D))
   {
	   pDoc->m_view[QGraphDoc::VIEW_3D] = this;
	   event->ignore();
   }
}

//void Q3DView::resizeEvent(QResizeEvent *event)
void Q3DView::resizeGL(int w, int h)
{
    Reshape(m_oldRect.right(), m_oldRect.bottom());
    OnRecentreView();

    pDoc->m_view[QGraphDoc::VIEW_3D] = this;

    if (w > 0)
    {
      glViewport(0, 0, w, h);

      m_oldRect.setRight(w);
      m_oldRect.setBottom(h);

      Reshape(w,h);
    }
}

void Q3DView::mouseMoveEvent(QMouseEvent *event)
{
    OnMouseMove(0, event->pos());
}

void Q3DView::mousePressEvent(QMouseEvent *event)
{
    switch(event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(0, event->pos());
        ((MainWindow*)pDoc->m_mainFrame)->update3DToolbar();
        break;

    case Qt::RightButton:
        OnRButtonDown(0, event->pos());
        break;
    }
}

void Q3DView::mouseReleaseEvent(QMouseEvent *event)
{
    switch(event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(0, event->pos());
        break;

    case Qt::RightButton:
        OnRButtonUp(0, event->pos());
        break;
    }
}

void Q3DView::keyPressEvent(QKeyEvent *event)
{
    OnKeyDown(event->key(), event->count(), 0);
}

void Q3DView::keyReleaseEvent(QKeyEvent *event)
{
    OnKeyUp(event->key(), event->count(), 0);
}

void Q3DView::wheelEvent(QWheelEvent *event)
{
    OnMouseWheel(0, event->delta(), event->pos());
}

void Q3DView::timerSlot()
{
    timerEvent(NULL);
}
