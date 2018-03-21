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

#include <QWidget>

/////////////////////////////////////////////////////////////////////////////
// PlotView view

//class CHoverWnd;

#include <QRect>
#include <QPoint>
#include <QSize>
#include "qpixmap.h"

#include "GraphDoc.h"
#include "genlib/linreg.h"

#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010

class QPlotView : public QWidget
{
	Q_OBJECT

// Attributes
public:
	QGraphDoc* pDoc;
	QPlotView();
    QSize sizeHint() const;

	int m_x_axis;
	int m_y_axis;
	int curr_x;
	int curr_y;

	AttributeIndex idx_x;
	AttributeIndex idx_y;
	void RedoIndices();

	bool m_queued_redraw;
	bool m_view_origin;
	bool m_view_trend_line;
	bool m_view_equation;
	bool m_view_rsquared;
	bool m_view_monochrome;

	LinReg<float> m_regression;

	//
	double dataX(int x);
	int screenX(double x);
	double dataY(int x);
	int screenY(double x);
	//
    QtRegion m_data_bounds;
	QRect m_screen_bounds;
	//
	QPoint m_mouse_point;
	QRect m_drag_rect_a;
	QRect m_drag_rect_b;
	bool m_selecting;
	bool m_drawdragrect;
	QRgb m_background;
	QRgb m_foreground;
	
	void* m_parent;//MainWindow*


// Operations
public:
	void SetAxis(int axis, int col, bool reset);
	void ResetRegression();

	bool Output(QPainter *pDC, QGraphDoc *pDoc, bool screendraw);
	void PrintOutput(QPainter *pDC, QGraphDoc *pDoc);

	// this is a tells us how many 100ms ticks have passed since the mouse moved
	int Tid_redraw;

	void OnViewTrendLine();
	void OnViewRsquared();
	void OnViewColor();
	void OnViewOrigin();
	void OnViewEquation();
    int OnRedraw(int wParam, int lParam);
	void OnEditCopy();
	void OnLButtonDown(bool nFlags, QPoint point);
	void OnMouseMove(bool nFlags, QPoint point);
	void OnLButtonUp(bool nFlags, QPoint point);

// Implementation
protected:
	virtual bool eventFilter(QObject *object, QEvent *e);
	virtual void keyPressEvent(QKeyEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void timerEvent(QTimerEvent *event);

private:
    QPixmap *m_pixmap;
};
