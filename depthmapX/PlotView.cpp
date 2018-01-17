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

// depthmapX - spatial network analysis platform

// PlotView.cpp : implementation file
//
#include <qpainter.h>
#include <qevent.h>

#include "mainwindow.h"
#include "PlotView.h"

#ifdef _WIN32
#define finite _finite
#endif

/////////////////////////////////////////////////////////////////////////////
// PlotView
static int pressed_nFlags;
QPlotView::QPlotView()
{
   m_x_axis = -1;
   m_y_axis = -1;
   curr_x = -1;
   curr_y = -1;


   m_queued_redraw = false;
   m_view_trend_line = false;
   m_view_rsquared = false;
   m_view_monochrome = false;
   m_view_origin = false;
   m_view_equation = false;

   // for drawing drag rect
   m_drawdragrect = false;
   m_selecting = false;

   setAttribute(Qt::WA_NoBackground, 1);
   setMouseTracking( true );
   setWindowIcon(QIcon(tr(":/images/cur/icon-1-2.png")));

   installEventFilter(this);
}

/////////////////////////////////////////////////////////////////////////////
// QPlotView drawing

int QPlotView::screenX(double x)
{
   return m_screen_bounds.left() + m_screen_bounds.width() * (m_data_bounds.width() ? (x - m_data_bounds.bottom_left.x) / m_data_bounds.width() : 0.5);
}

int QPlotView::screenY(double y)
{
   return m_screen_bounds.top() - (abs(m_screen_bounds.height()) * (m_data_bounds.height() ? (y - m_data_bounds.bottom_left.y) / m_data_bounds.height(): 0.5));
}

double QPlotView::dataX(int x)
{
   return m_data_bounds.bottom_left.x + m_data_bounds.width() * double(x - m_screen_bounds.left()) / double(m_screen_bounds.width());
}

double QPlotView::dataY(int y)
{
   return m_data_bounds.bottom_left.y + m_data_bounds.height() * double(m_screen_bounds.top() - y) / double(abs(m_screen_bounds.height()));
}

QSize QPlotView::sizeHint() const
{
	return QSize(2000, 2000);
}

static QPoint hit_point;
bool QPlotView::eventFilter(QObject *object, QEvent *e)
{
	if(e->type() == QEvent::ToolTip)
	{
		if (!pDoc->m_communicator) {
         // first, check you have a meta graph
			if (pDoc->m_meta_graph) {
				if (pDoc->m_meta_graph->viewingProcessed()) {
					AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
					size_t xfloor = idx_x.searchfloorindex(ValuePair(-1,dataX(hit_point.x()-2)));
					size_t xceil = idx_x.searchceilindex(ValuePair(-1,dataX(hit_point.x()+2)));
					size_t yfloor = idx_y.searchfloorindex(ValuePair(-1,dataY(hit_point.y()+2)));
					size_t yceil = idx_y.searchceilindex(ValuePair(-1,dataY(hit_point.y()-2)));
					// work out anything near this point...
					pvecint xkeys;
					for (size_t i = xfloor + 1; i < xceil; i++) {
						int index = idx_x[i].index;
						xkeys.add(index);
					}
					pvecint finalkeys;
					for (size_t j = yfloor + 1; j < yceil; j++) {
						size_t index = idx_y[j].index;
						if (xkeys.searchindex(index) != paftl::npos) {
							finalkeys.push_back( table.getRowKey(index) );
						}
					}
					if (finalkeys.size()) {
						AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
						// and that it has an appropriate state to display a hover wnd
						float val = table.getValue( finalkeys[0], pDoc->m_meta_graph->getDisplayedAttribute() );
						if (val == -1.0f) 
							setToolTip("No value");
						else if (val != -2.0f)
							setToolTip(QString("%1").arg(val));
					}
				}
			}
		}
	}
    return QObject::eventFilter(object, e);
}


void QPlotView::paintEvent(QPaintEvent *event)
{
    QPainter pDC(m_pixmap);

	QRect rect = QRect(0, 0, width(), height());
	PafColor selcol(SALA_SELECTED_COLOR);
	pDC.setPen(QPen(QBrush(QColor(selcol.redb(), selcol.greenb(), selcol.blueb())), 1, Qt::DotLine, Qt::RoundCap));

/*  if (pDC->IsPrinting()) 
	{
		PrintOutput(pDC, pDoc);
	}
	else */
	{   // if (m_clear)
		if (m_drawdragrect)
		{
		    pDC.setCompositionMode(QPainter::RasterOp_SourceAndNotDestination);
		    if(!m_drag_rect_b.isEmpty()) pDC.drawRect( m_drag_rect_b);
		    if(!m_drag_rect_a.isEmpty()) pDC.drawRect( m_drag_rect_a);
		    pDC.setCompositionMode(QPainter::CompositionMode_SourceOver);
			m_drag_rect_b = m_drag_rect_a;
			m_drawdragrect = false;
		}
		else {
			m_background = ((MainWindow*)m_parent)->m_background;
			m_foreground = ((MainWindow*)m_parent)->m_foreground;
			pDC.fillRect(rect, QBrush(QColor(m_background)));
			// m_clear = false;
			Output(&pDC, pDoc, true);
            OnRedraw(0, 0);
		}
    }

    QPainter screenPainter(this);
    screenPainter.drawPixmap(0,0,width(),height(),*m_pixmap);
}

void QPlotView::resizeEvent(QResizeEvent *event)
{
   pDoc->m_view[QGraphDoc::VIEW_SCATTER] = this;
   setWindowTitle(pDoc->m_base_title+":Scatter Plot");
   m_pixmap = new QPixmap(width(),height());
}

void QPlotView::closeEvent(QCloseEvent *event)
{
   pDoc->m_view[QGraphDoc::VIEW_SCATTER] = NULL;
   if (!pDoc->OnCloseDocument(QGraphDoc::VIEW_SCATTER))
   {
	   pDoc->m_view[QGraphDoc::VIEW_SCATTER] = this;
	   event->ignore();
   }
}

void QPlotView::OnEditCopy() 
{
/*   // Copy to Clipboard
   if (OpenClipboard() && EmptyClipboard()) {

      CMetaFileDC *pDC = new CMetaFileDC;

      QRect rect;
      GetClientRect(rect);
      QPainter *dummyDC = new QPainter;
      dummyDC->CreateCompatibleDC(GetDC());
      int oldmm = dummyDC->SetMapMode(MM_HIMETRIC);
      QSize size(rect.width(),rect.height());
      dummyDC->DPtoLP(&size);
      rect.SetRect(0,0,size.width(),size.height());
      dummyDC->SetMapMode(oldmm);
      pDC->CreateEnhanced(dummyDC,NULL,rect,_T("Depthmap\0Graph\0\0"));
      pDC->SetAttribDC(dummyDC->m_hAttribDC);
      PrintOutput(pDC, pDoc);
      dummyDC->DeleteDC();
      delete dummyDC;

      HENHMETAFILE hMF = pDC->CloseEnhanced();
      
      HANDLE h = SetClipboardData(CF_ENHMETAFILE, hMF);

      CloseClipboard();

      pDC->DeleteDC();
      delete pDC;
   }*/
}

void QPlotView::PrintOutput(QPainter *pDC, QGraphDoc *pDoc)
{
/*   QRect rectin, rectout;
   GetClientRect(rectin);

   int pixels;
   if (pDC->IsPrinting()) {
      pixels = __min( pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES) );

      if (rectin.width() > rectin.height()) {
         rectout.SetRect( QPoint(0,0), QPoint(pixels * rectin.height() / rectin.width(), pixels) );
      }
      else {
         rectout.SetRect( QPoint(0,0), QPoint(pixels, pixels * rectin.width() / rectin.height()) );
      }
      // Not sure what isotropic does for us
      pDC->SetMapMode( MM_ISOTROPIC );
   }
   else {
      // When 'copy'ing, the rectin and rectout are set one to one:
      rectout = rectin;
   }

   pDC->SetWindowExt( rectin.Size() );
   pDC->SetViewportExt( rectout.Size() );

   CGdiObject *oldpen   = pDC->SelectStockObject( NULL_PEN );
   QBrush brush( GetApp()->m_background );
   CGdiObject *oldbrush = pDC->SelectObject( &brush );

   pDC->Rectangle( rectin );

   pDC->SelectObject( oldbrush );
   pDC->SelectObject( oldpen );

   if (!Output( pDC, pDoc, false )) {
      AfxMessageBox(_T("Unable to print / copy screen at this time"));
      pDC->AbortDoc();
      return;
   }*/
}

bool QPlotView::Output(QPainter *pDC, QGraphDoc *pDoc, bool screendraw) 
{
// this is going to need a timer at somepoint, but for now, it's all very easy to start off:
   auto lock = pDoc->m_meta_graph->getLockDeferred();
    if (!lock.try_lock()) {
      return false;
   }

   if (pDoc->m_communicator || !pDoc->m_meta_graph->viewingProcessed()) {
      return false;
   }

   AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
   int displaycol = pDoc->m_meta_graph->getDisplayedAttribute();

   QRect rect = QRect(0, 0, width(), height());
   int mindim = __min(rect.width(),rect.height());
   int viscount = table.getVisibleRowCount();
   if(!viscount) m_x_axis = m_y_axis = 0;

   int spacer = floor(2.0 * sqrt(double(mindim)/double(viscount != 0 ? viscount : 1)));
   if (!screendraw && spacer < 4) {
      spacer = 4;
   }

   // text formatting
   int pointsize;
   if (mindim > 200) {
      pointsize = 100;
   }
   else {
      pointsize = 50+mindim/4;
   }

   QFont font = QFont("Arial", 10, 10);
   setFont(font);

   QPen pen = QPen(QBrush(QColor(m_foreground)), 1, Qt::SolidLine, Qt::RoundCap);
   QPen oldpen = pDC->pen();
   pDC->setPen(pen);

   float minx, miny, maxx, maxy;
   QString str_minx, str_miny, str_maxx, str_maxy;

   minx = m_data_bounds.bottom_left.x = m_view_origin ? 0.0 : table.getVisibleMinValue(m_x_axis);
   miny = m_data_bounds.bottom_left.y = m_view_origin ? 0.0 : table.getVisibleMinValue(m_y_axis);
   maxx = m_data_bounds.top_right.x = table.getVisibleMaxValue(m_x_axis);
   maxy = m_data_bounds.top_right.y = table.getVisibleMaxValue(m_y_axis);

   str_minx = QString(tr("%1")).arg(minx);
   str_miny = QString(tr("%1")).arg(miny);
   str_maxx = QString(tr("%1")).arg(maxx);
   str_maxy = QString(tr("%1")).arg(maxy);

   // now work out the drawing window for 
   QSize sizex = QSize(str_maxx.length()*7, 16);
   QSize sizey = QSize(str_miny.length()*7, 16);
   QSize sizey2 = QSize(str_maxy.length()*7, 16);
   if (sizey2.width() > sizey.width()) {
	   sizey.rwidth() = sizey2.width();
   }
   // doesn't matter which, just want a height:
   int texth = sizex.height();

   int xaxis_pos = 99*rect.height()/100 - 2 * texth;
   int yaxis_pos = rect.width()/100 + sizey.width() + texth;
   int miny_pos = xaxis_pos - texth;
   m_screen_bounds.setTop(miny_pos);
   int maxy_pos = rect.height()/100 + texth / 2;
   m_screen_bounds.setBottom(maxy_pos);
   int minx_pos = yaxis_pos + texth;
   m_screen_bounds.setLeft(minx_pos);
   int maxx_pos = 99*rect.width()/100 - sizex.width() / 2;
   m_screen_bounds.setRight(maxx_pos);

   int width = maxx_pos -minx_pos;
   if (minx == maxx) {
      minx_pos = maxx_pos = minx_pos + width / 2;
	  m_screen_bounds.setRight(minx_pos);
	  m_screen_bounds.setLeft(minx_pos);
      width = 0;
   }
   int height = maxy_pos - miny_pos;
   if (miny == maxy) {
      miny_pos = maxy_pos = miny_pos + height / 2;
	  m_screen_bounds.setBottom(miny_pos);
	  m_screen_bounds.setTop(miny_pos);
      height = 0;
   }

   pDC->setPen(pen);
   pDC->drawText(minx_pos-50, 99*rect.height()/100 - texth, 100, 16, Qt::AlignCenter, str_minx);
   if (minx != maxx) {
      pDC->drawText(maxx_pos-50, 99*rect.height()/100 - texth, 100, 16, Qt::AlignCenter, str_maxx);
   }

   pDC->drawText(yaxis_pos-texth-100, miny_pos - texth/2, 100, 16, Qt::AlignRight , str_miny);
   if (miny != maxy) {
      pDC->drawText(yaxis_pos-texth-100, maxy_pos - texth/2, 100, 16, Qt::AlignRight , str_maxy);
   }

   pDC->drawLine(QPoint(yaxis_pos-texth/2,miny_pos), QPoint(yaxis_pos,miny_pos));
   if (miny != maxy) {
      pDC->drawLine(QPoint(yaxis_pos,miny_pos), QPoint(yaxis_pos,maxy_pos));
      pDC->drawLine(QPoint(yaxis_pos,maxy_pos), QPoint(yaxis_pos-texth/2,maxy_pos));
   }

   pDC->drawLine(QPoint(minx_pos,xaxis_pos+texth/2), QPoint(minx_pos,xaxis_pos));
   if (minx != maxx) {
      pDC->drawLine(QPoint(minx_pos,xaxis_pos), QPoint(maxx_pos,xaxis_pos));
      pDC->drawLine(QPoint(maxx_pos,xaxis_pos), QPoint(maxx_pos,xaxis_pos+texth/2));
   }

   int sel_parity = 0;
   QPen pen2;
   for (int i = 0; i < table.getRowCount(); i++) {
      if (!table.isVisible(i)) {
         continue;
      }
      float x = table.getValue(i,m_x_axis);
      float y = table.getValue(i,m_y_axis);
      if (!finite(x) || !finite(y) || x == -1.0f || y == -1.0f) {
         continue;
      }
      QRgb rgb;
      if (m_view_monochrome) {
         rgb = m_foreground;
      }
      else {
         PafColor color = table.getDisplayColor(i);
         rgb = qRgb(color.redb(),color.greenb(),color.blueb());
      }
      int tempspacer = spacer;
      if (table.isSelected(i)) {
         tempspacer = (spacer + 1) * 2 - 1;
         if (m_view_monochrome) {
            rgb = qRgb(0xff,0,0);
         }
      }
      if (tempspacer == 0) {
         tempspacer = 1;
      }
      if (!m_view_monochrome) {
		  pen2 = QPen(QBrush(QColor(rgb)), spacer, Qt::SolidLine, Qt::FlatCap);
         //pDC->setPen(QPen(QBrush(QColor(rgb)), spacer, Qt::SolidLine, Qt::FlatCap));
      }
      else if (sel_parity != (table.isSelected(i) ? 1 : -1)) {
		  pen2 = QPen(QBrush(QColor(rgb)), spacer, Qt::SolidLine, Qt::FlatCap);
         //pDC->setPen(QPen(QBrush(QColor(rgb)), spacer, Qt::SolidLine, Qt::FlatCap));
         sel_parity = (table.isSelected(i) ? 1 : -1);
      }
	  pDC->setPen(pen2);
      //
      QPoint point(screenX(x),screenY(y));
      pDC->drawLine(QPoint(point.x()-tempspacer, point.y()-tempspacer), QPoint(point.x()+tempspacer+1, point.y()+tempspacer+1));
      pDC->drawLine(QPoint(point.x()-tempspacer,point.y()+tempspacer), QPoint(point.x()+tempspacer+1,point.y()-tempspacer-1));
      pDC->setPen(pen);
   }

   // trend line if reqd
   if (m_view_trend_line) {
      QPoint bl, tr;
      QString string;
      if (m_regression.model(m_data_bounds.bottom_left.x) < m_data_bounds.bottom_left.y) {
         // check line is on page
         if (m_regression.model(m_data_bounds.top_right.x) > m_data_bounds.bottom_left.y) {
            bl = QPoint(screenX(m_regression.invmodel(m_data_bounds.bottom_left.y)),m_screen_bounds.top());
            if (m_regression.model(m_data_bounds.top_right.x) < m_data_bounds.top_right.y) {
               tr = QPoint(m_screen_bounds.right(), screenY(m_regression.model(m_data_bounds.top_right.x)));
            }
            else {
               tr = QPoint(screenX(m_regression.invmodel(m_data_bounds.top_right.y)),m_screen_bounds.bottom());
            }
            pDC->drawLine(bl, tr);
         }
      }
      else if (m_regression.model(m_data_bounds.bottom_left.x) > m_data_bounds.top_right.y) {
         // check line is on page
         if (m_regression.model(m_data_bounds.top_right.x) < m_data_bounds.top_right.y) {
            bl = QPoint(screenX(m_regression.invmodel(m_data_bounds.bottom_left.x)),m_screen_bounds.bottom());
            if (m_regression.model(m_data_bounds.top_right.x) > m_data_bounds.bottom_left.x) {
               tr = QPoint(m_screen_bounds.right(), screenY(m_regression.model(m_data_bounds.top_right.x)));
            }
            else {
               tr = QPoint(screenX(m_regression.invmodel(m_data_bounds.top_right.y)),m_screen_bounds.top());
            }
            pDC->drawLine(bl, tr);
         }
      }
      else {
         bl = QPoint(m_screen_bounds.left(),screenY(m_regression.model(m_data_bounds.bottom_left.x)));
         double trv = m_regression.model(m_data_bounds.top_right.x);
         if (trv >= m_data_bounds.bottom_left.y && trv <= m_data_bounds.top_right.y) {
            string += " v1";
            tr = QPoint(m_screen_bounds.right(), screenY(trv));
         }
         else if (m_regression.b() > 0) {   // upward inclined
            string += " v2";
            tr = QPoint(screenX(m_regression.invmodel(m_data_bounds.top_right.y)),m_screen_bounds.bottom());
         }
         else { // downward inclined
            string += " v3";
            tr = QPoint(screenX(m_regression.invmodel(m_data_bounds.bottom_left.y)),m_screen_bounds.top());
         }
		 pDC->drawLine(bl, tr);
      }
   }

   QString string;
   int textpos = texth;
   if (m_view_rsquared) {
      // set text formating
      // hope ascii superscript 2 in place!
      string = QString(tr("R\xb2 = %1").arg(sqr(m_regression.r())));
      pDC->drawText(QPointF(rect.width()-string.length()*7, textpos), string);
      textpos += texth;
   }
   if (m_view_equation) {
      if (m_regression.a() >= 0) {
         string = QString(tr("y = %1 x + %1").arg(m_regression.b()).arg(m_regression.a()));
      }
      else {
         string = QString(tr("y = %1 x - %1").arg(m_regression.b()).arg(fabs(m_regression.a())));
      }
	  pDC->drawText(QPointF(rect.width()-string.length()*7, textpos), string);
   }

   QString xlabel(table.getColumnName(m_x_axis).c_str());
   pDC->drawText(QPointF(width/2+minx_pos, 99*rect.height()/100-texth), xlabel);


   QString ylabel(table.getColumnName(m_y_axis).c_str());
   // this is last to avoid switch between hfont and vfont
   QMatrix matrix;
   matrix.translate(sizey.width(), rect.height()/2+ylabel.length()*7/2);
   matrix.rotate(270);
   pDC->setMatrix(matrix);

   pDC->drawText(QPointF(sizey.width()/2, height+miny_pos), ylabel);

   pDC->setPen(oldpen);

   return true;
}
/*
/////////////////////////////////////////////////////////////////////////////
// QPlotView printing

void QPlotView::OnBeginPrinting(QPainter* pDC, CPrintInfo* pInfo) 
{
	CView::OnBeginPrinting(pDC, pInfo);
}

void QPlotView::OnEndPrinting(QPainter* pDC, CPrintInfo* pInfo) 
{
	CView::OnEndPrinting(pDC, pInfo);
}

BOOL QPlotView::OnPreparePrinting(CPrintInfo* pInfo) 
{
	// default preparation
	return DoPreparePrinting(pInfo);
}
*/
/////////////////////////////////////////////////////////////////////////////
// QPlotView message handlers

int QPlotView::OnRedraw(int wParam, int lParam)
{
   if (pDoc->GetRemenuFlag(QGraphDoc::VIEW_SCATTER)) {
      pDoc->SetRemenuFlag(QGraphDoc::VIEW_SCATTER, false);
	  MainWindow* m_Frm = (MainWindow*)pDoc->m_mainFrame;
	  m_Frm->RedoPlotViewMenu(pDoc);
   }
   if (pDoc->GetRedrawFlag(QGraphDoc::VIEW_SCATTER) != QGraphDoc::REDRAW_DONE) {

      if (!pDoc->m_communicator) {

         m_queued_redraw = false;

         while (!pDoc->SetRedrawFlag(QGraphDoc::VIEW_SCATTER,QGraphDoc::REDRAW_DONE)) {
            // prefer waitformultipleobjects here
//            Sleep(1);
         }
	  }
      else {
//         killTimer(Tid_redraw);
//         Tid_redraw = startTimer(100);
         m_queued_redraw = true;
      }
   }

   return 0;
}

void QPlotView::keyPressEvent(QKeyEvent *e)
{
	char key = e->key();
}

void QPlotView::RedoIndices()
{
   if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessed()) {
      AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
      idx_x.makeIndex(table, m_x_axis, false);
      idx_y.makeIndex(table, m_y_axis, false);
   }
}


void QPlotView::timerEvent(QTimerEvent *event)
{
   if (event->timerId() == Tid_redraw) {

      if (m_queued_redraw) {

         // Internal own redraw
         OnRedraw(0, 0);
      }
   }
}

void QPlotView::OnViewTrendLine() 
{
   if (m_view_trend_line) {
      m_view_trend_line = false;
   }	
   else {
      m_view_trend_line = true;
   }
   update();
}

void QPlotView::OnViewRsquared() 
{
   if (m_view_rsquared) {
      m_view_rsquared = false;
   }
   else {
      m_view_rsquared = true;
   }
   update();
}

void QPlotView::OnViewColor() 
{
   if (m_view_monochrome) {
      m_view_monochrome = false;
   }
   else {
      m_view_monochrome = true;
   }
   update();
}

void QPlotView::SetAxis(int axis, int col, bool reset)
{
   if (axis == 0) {
      if (m_x_axis != col) {
         m_x_axis = col;
      }
   }
   else {
      if (m_y_axis != col) {
         m_y_axis = col;
      }
   }
   if (reset) {
      RedoIndices();
      ResetRegression();
   }
}

void QPlotView::ResetRegression()
{
   m_regression.clear();
   if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessed()) {
      AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
      for (int i = 0; i < table.getRowCount(); i++) {
         if (table.isVisible(i)) {
            float x = table.getValue(i,m_x_axis);
            float y = table.getValue(i,m_y_axis);
            if (finite(x) && finite(y) && x != -1.0f && y != -1.0f) {
               m_regression.add(x,y);
            }
         }
      }
   }
}

void QPlotView::mousePressEvent(QMouseEvent *e)
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
	m_mouse_point = e->pos();
	m_drag_rect_a = QRect(0,0,0,0);
	m_drag_rect_b = QRect(0,0,0,0);
	m_selecting = true;
	m_drawdragrect = true;
	update();
}

void QPlotView::mouseMoveEvent(QMouseEvent *e)
{
   if(pressed_nFlags == MK_RBUTTON) return;
   QPoint point = e->pos();
   if (m_selecting) {
	   int x1, y1, x2, y2;
	   if (m_mouse_point.x()>point.x())
	   {
		   x1 = point.x();
		   x2 = m_mouse_point.x();
	   }
	   else
	   {
		   x1 = m_mouse_point.x();
		   x2 = point.x();
	   }

	   if (m_mouse_point.y()>point.y())
	   {
		   y1 = point.y();
		   y2 = m_mouse_point.y();
	   }
	   else
	   {
		   y1 = m_mouse_point.y();
		   y2 = point.y();
	   }
	   m_drag_rect_a = QRect(x1, y1, x2-x1, y2-y1);
       m_drawdragrect = true;
       update();
    }
    hit_point = point;
}

void QPlotView::mouseReleaseEvent(QMouseEvent *e)
{
   if(pressed_nFlags == MK_RBUTTON)
   {
      pDoc->m_meta_graph->clearSel();
      pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
	  return;
   }
   m_selecting = false;

   size_t xfloor = idx_x.searchfloorindex(ValuePair(-1,dataX(m_drag_rect_a.left()-2)));
   size_t xceil = idx_x.searchceilindex(ValuePair(-1,dataX(m_drag_rect_a.right()+2)));
   size_t yfloor = idx_y.searchfloorindex(ValuePair(-1,dataY(m_drag_rect_a.bottom()+2)));
   size_t yceil = idx_y.searchceilindex(ValuePair(-1,dataY(m_drag_rect_a.top()-2)));

   // Stop drag rect...
   m_drag_rect_a = QRect(0,0,0,0);
   m_drawdragrect = false;
   update();
   AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();

   // work out selection
   pvecint xkeys;
   for (size_t i = xfloor + 1; i < xceil; i++) {
      int index = idx_x[i].index;
      xkeys.add(index);
   }
   std::vector<int> finalkeys;
   for (size_t j = yfloor + 1; j < yceil; j++) {
      int index = idx_y[j].index;
      if (xkeys.searchindex(index) != paftl::npos) {
         finalkeys.push_back( table.getRowKey(index) );
      }
   }
   
   // redraw selection set
   bool add = false;
   if (pressed_nFlags & MK_SHIFT) {
      add = true;
   }
   pDoc->m_meta_graph->setSelSet(finalkeys, add);
   pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
}

void QPlotView::OnViewOrigin() 
{
   m_view_origin = !m_view_origin;
   update();
}

/*
void QPlotView::OnUpdateViewOrigin(CCmdUI* pCmdUI) 
{
   if (m_view_origin) {
      pCmdUI->SetCheck(1);
   }
   else {
      pCmdUI->SetCheck(0);
   }	
}
void QPlotView::OnUpdateViewEquation(CCmdUI* pCmdUI) 
{
   if (m_view_equation) {
      pCmdUI->SetCheck(1);
   }
   else {
      pCmdUI->SetCheck(0);
   }	
}
*/

void QPlotView::OnViewEquation() 
{
   m_view_equation = !m_view_equation;
   update();
}

