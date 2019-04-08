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


#include "depthmapX/views/mapview.h"
#include <QRect>
#include <QPoint>
#include <QSize>

#include <qpixmap.h>

#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010

class QDepthmapView : public MapView
{
    Q_OBJECT

public:
    QDepthmapView(QGraphDoc &pDoc,
                  Settings &settings,
                  QWidget *parent = Q_NULLPTR);
    ~QDepthmapView();
    QSize sizeHint() const override;
    void SetRedrawflag();
    void saveToFile();

	bool m_showgrid;
	bool m_showtext;
	bool m_showlinks;
	int m_current_mode;

   // Selectable mouse modes
   enum {NONE = 0x0000, SELECT = 0x10000, FILL = 0x0002, SEEDAXIAL = 0x0004, 
         LINETOOL = 0x0008, POLYGONTOOL = 0x0010,
         ZOOMORDRAG  = 0x0300, 
         DRAG = 0x0101, 
         ZOOM = 0x0200, ZOOM_IN = 0x0202, ZOOM_OUT = 0x0204, 
         GENERICPENCIL = 0x0800, PENCIL = 0x0801, ERASE = 0x0802,
         SNAP = 0x1000, SNAPON = 0x1001, SNAPOFF = 0x1002, // snap modes are not used outside invalidate -- '1000' used to test snap
         DRAWLINE = 0x2000, LINEON = 0x2001, LINEOFF = 0x2002,
         GENERICISOVIST = 0x4000, SEEDISOVIST = 0x4001, SEEDHALFOVIST = 0x4002,
         OVERHANDLE = 0x8000,
         GENERICJOIN = 0x20000, JOINB = 0x00400, JOIN = 0x20001, UNJOIN = 0x20002
   };
   enum {FULLFILL = 0, SEMIFILL = 1, AUGMENT = 2}; // AV TV
   virtual void OnModeJoin() override;
   virtual void OnModeUnjoin() override;
   virtual void OnModeSeedAxial() override;
   virtual void OnModeIsovist() override;
   virtual void OnModeTargetedIsovist() override;
   virtual void OnEditLineTool() override;
   virtual void OnEditPolygonTool() override;
   virtual void OnEditFill() override;
   virtual void OnEditSemiFill() override;
   virtual void OnEditAugmentFill() override; // AV TV
   virtual void OnViewZoomIn() override;
   virtual void OnViewZoomOut() override;
   virtual void OnViewPan() override;
   virtual void OnViewZoomsel() override;
   virtual void OnEditSelect() override;
   virtual void OnEditPencil() override;
   virtual void postLoadFile() override;
   virtual void OnEditCopy() override;
   virtual void OnEditSave() override;
   virtual void OnViewZoomToRegion(QtRegion regionToZoomAt) override;

protected:
    virtual void timerEvent(QTimerEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeGL(int w, int h) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void wheelEvent(QWheelEvent *e) override;
    virtual bool eventFilter(QObject *object, QEvent *e) override;

public slots:
    int OnRedraw(int wParam, int lParam);
    void OnEditEraser();

private:
   int m_mouse_mode;
	int Tid_redraw;
    QSize m_initialSize;

////////////////////////////////////////////////////////////////////
	QCursor m_cursor;

   QPoint m_mouse_point;
   Point2f m_mouse_location;
   int m_fillmode;
   QRect m_drag_rect_a;
   QRect m_drag_rect_b;

   QRgb m_selected_color;

   bool ModeOk();

   // lots of screen drawing booleans... to be tidied up...
   bool m_drawing;
   bool m_continue_drawing;
   //
   int m_invalidate; // <- includes the mode
   bool m_queued_redraw;
   bool m_internal_redraw;
   bool m_clear;
   bool m_redraw;

   bool m_resize_viewport;
   bool m_viewport_set;
   bool m_redraw_all;
   bool m_redraw_no_clear;

   bool m_right_mouse_drag;
   bool m_alt_mode;

   // logical units:
   Point2f m_centre;
   double m_unit;
   // keep tabs on the screen size:
   QSize m_physical_centre;

   // record any previous find location loc:
   Point2f m_lastfindloc;

   // Snap to control (must be in map units for accuracy)
   Point2f m_snap_point;
   Point2f m_old_snap_point;
   QRgb m_cross_pixels[18];
   bool m_snap;
   int m_repaint_tag;

   // Line start and end (must be in map units in case you zoom out / pan while you're drawing!
   Line m_line;
   Line m_old_line;
   std::vector<QRgb> m_line_pixels;
   std::vector<Point2f> m_point_handles;
   int m_active_point_handle;

   int m_currentlyEditingShapeRef = -1;

   // polygon drawing utilities
   Point2f m_poly_start;
   int m_poly_points;

   QRgb m_background;
   QRgb m_foreground;

///////////////////////////////////////////////////////////////////////

   Point2f LogicalUnits( const QPoint& p );
   QPoint PhysicalUnits( const Point2f& p );

   // hover information
   void ResetHoverWnd(const QPoint& p = QPoint(-1,-1));
   void CreateHoverWnd();

   bool IsAtZoomLimits(double ratio, double maxZoomOutRatio);
   void ZoomTowards(double ratio, const Point2f& point);

   void InitViewport(const QRect& phys_bounds, QGraphDoc *pDoc);
   QtRegion LogicalViewport(const QRect& phys_bounds, QGraphDoc *pDoc);

   int GetSpacer(QGraphDoc *pDoc);
   void PrintBaby(QPainter *pDC, QGraphDoc *pDoc);
   bool Output(QPainter *pDC, QGraphDoc *pDoc, bool screendraw);
   bool DrawPoints(QPainter *pDC, QGraphDoc *pDoc, int spacer, unsigned long ticks, bool screendraw);
   bool DrawAxial(QPainter *pDC, QGraphDoc *pDoc, int spacer, unsigned long ticks, bool screendraw);
   bool DrawShapes(QPainter *pDC, ShapeMap& map, bool muted, int spacer, unsigned long ticks, bool screendraw);

   void DrawLink(QPainter *pDC, int spacer, const Line& logical);
   void DrawPointHandle(QPainter *pDC, QPoint pt);

   //
   void OutputEPS(std::ofstream& stream, QGraphDoc *pDoc , bool includeScale = true);
   void OutputEPSMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, QRect& rect, float spacer);
   void OutputEPSLine(std::ofstream& stream, Line& line, int spacer, QtRegion& logicalviewport, QRect& rect);
   void OutputEPSPoly(std::ofstream& stream, const SalaShape& shape, int spacer, QtRegion& logicalviewport, QRect& rect);

   void OutputSVG( std::ofstream& stream, QGraphDoc *pDoc );
   void OutputSVGMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, int h);
   void OutputSVGLine(std::ofstream& stream, Line& line, QtRegion& logicalviewport, int h);
   void OutputSVGPoly(std::ofstream& stream, const SalaShape& shape, QtRegion& logicalviewport, int h);

   void FillLocation(QPainter *pDC, QPoint& p, int spacer, unsigned int blocked, QRgb color);
   void DrawLine(QPainter *pDC, QRect& line, bool drawit);
   void DrawCross(QPainter *pDC, QPoint& centre, bool drawit);

   void SetCursor(int mode);
   void AltMode();
   void BeginJoin();
   void BeginDrag(QPoint point);

   QPixmap *m_pixmap;

};
   
   
