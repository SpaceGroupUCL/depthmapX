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


#include <QtGui>
#include <QDesktopServices>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>

#include "mainwindow.h"
#include "depthmapView.h"
#include "3DView.h"
#include "PlotView.h"
#include "tableView.h"
#include "AboutDlg.h"
#include "settingsdialog/settingsdialog.h"


static int current_view_type = 0;

const QString editstatetext[] = {"Not Editable", "Editable Off", "Editable On"};

QmyEvent::QmyEvent(Type type, void* wp, int lp)
    : QEvent(type)
{
    registerEventType(type);
    wparam = wp;
    lparam = lp;
}

void MainWindow::actionEvent ( QActionEvent * event )
{
    int id;
    if(id = event->action()->data().toInt())
    {
        int k = id;

    }
}

bool MainWindow::eventFilter(QObject *object, QEvent *e)
{
    if (object == this && e->type() == (QEvent::Type)FOCUSGRAPH)
    {
        OnFocusGraph((QGraphDoc*)((QmyEvent*)e)->wparam, ((QmyEvent*)e)->lparam);
        return true;
    }
    return QObject::eventFilter(object, e);
}

MainWindow::MainWindow(const QString &fileToLoad, Settings &settings) : mSettings(settings)
{
    m_treeDoc = NULL;
    mdiArea = new QMdiArea;
    setCentralWidget(mdiArea);
    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(updateActiveWindows()));

    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(setActiveSubWindow(QWidget *)));


    m_indexWidget = new indexWidget(this, false);
    QDockWidget *indexDock = new QDockWidget(tr("Index"), this);
    indexDock->setObjectName(QLatin1String("IndexWindow"));
    indexDock->setWidget(m_indexWidget);
    addDockWidget(Qt::LeftDockWidgetArea, indexDock);

    QDockWidget *AttributesListDock = new QDockWidget(tr("AttributesList"), this);
    AttributesListDock->setObjectName(QLatin1String("AttributesListWindow"));
    AttributesListDock->setWidget(setupAttributesListWidget());
    addDockWidget(Qt::LeftDockWidgetArea, AttributesListDock);

    readSettings(); // read setting or generate default
    setWindowTitle(TITLE_BASE);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateToolbar();
    updateActiveWindows();
    updateGLWindows(true, true);

    installEventFilter(this);
//	setWindowIcon(QIcon(tr(":/images/cur/icon-1-1.png")));

    if (fileToLoad.length()>0)
    {
        MapView *child = createMapView();

        QByteArray ba = fileToLoad.toUtf8(); // quick fix for weird chars (russian filename bug report)
        char *file = ba.data(); // quick fix for weird chars (russian filename bug report)
        if(child->getGraphDoc()->OnOpenDocument(file)) // quick fix for weird chars (russian filename bug report)
        {
             child->setCurrentFile(fileToLoad);
             child->postLoadFile();
             statusBar()->showMessage(tr("File loaded"), 2000);
             child->show();
             OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
             setCurrentFile(fileToLoad);
        } else {
             child->close();
             QMessageBox::warning(this, "Failed to load", QString("Failed to load file ")+fileToLoad, QMessageBox::Ok, QMessageBox::Ok );
        }

    }
}

QWidget * MainWindow::setupAttributesListWidget()
{
    QWidget *widget = new QWidget(this);

    QLayout *vlayout = new QVBoxLayout(widget);
    vlayout->setMargin(1);

    QLayout *hlayout = new QHBoxLayout();
    vlayout->addWidget(m_attrWindow = new AttribWindow(this, false));
    vlayout->addItem(hlayout);

    hlayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    attr_add_button = new QToolButton(widget);
    attr_add_button->setText(tr("Add"));
    attr_add_button->setIcon(QIcon(tr(":/images/win/b-5-19.png")));
    attr_add_button->setAutoRaise(true);
    attr_add_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(attr_add_button);
    connect(attr_add_button, SIGNAL(clicked()), this, SLOT(OnAddColumn()));

    attr_del_button = new QToolButton(widget);
    attr_del_button->setText(tr("Remove"));
    attr_del_button->setIcon(QIcon(tr(":/images/win/b-5-21.png")));
    attr_del_button->setAutoRaise(true);
    attr_del_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hlayout->addWidget(attr_del_button);
    connect(attr_del_button, SIGNAL(clicked()), this, SLOT(OnRemoveColumn()));

    return widget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (activeMapView()) {
         event->ignore();
    } else {
        QApplication::postEvent((QObject*)&m_wndColourScale, new QEvent(QEvent::Close));
         writeSettings();
         event->accept();
    }
}

void MainWindow::OnFileNew()
{
    MapView *child = createMapView();
    child->getGraphDoc()->OnNewDocument();
    child->setCurrentFile("");
    child->postLoadFile();
    child->show();
    OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
}

void MainWindow::OnFileOpen()
{
   QString template_string;
   template_string += "Graph Files (*.graph)\nAll files (*.*)";

   QFileDialog::Options options = 0;
   QString selectedFilter;
     QString fileName = QFileDialog::getOpenFileName(
       0, tr("Open"),
       "",
       template_string,
       &selectedFilter,
       options);
     if (!fileName.isEmpty()) {
          QMdiSubWindow *existing = findMapView(fileName);
          if (existing) {
                mdiArea->setActiveSubWindow(existing);
                return;
          }
          MapView *child = createMapView();
          QByteArray ba = fileName.toUtf8(); // quick fix for weird chars (russian filename bug report)
          char *file = ba.data(); // quick fix for weird chars (russian filename bug report)
          if(child->getGraphDoc()->OnOpenDocument(file)) // quick fix for weird chars (russian filename bug report)
          {
               child->setCurrentFile(fileName);
               child->postLoadFile();
               statusBar()->showMessage(tr("File loaded"), 2000);
               child->show();
               OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
               setCurrentFile(fileName);
          } else {
               child->close();
          }
     }
}

void MainWindow::showContextMenu(QPoint &point)
{
    QMenu menu;
    menu.addAction(renameColumnAct);
    menu.addAction(updateColumAct);
    menu.addAction(removeColumAct);
    menu.addSeparator();
    menu.addAction(columnPropertiesAct);

    menu.exec(point);
}

void MainWindow::OnFilePrint()
{

}

void MainWindow::OnFilePrintPreview()
{

}

void MainWindow::OnFilePrintSetup()
{

}

void MainWindow::OnFileExit()
{

}

void MainWindow::OnEditUndo()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnEditUndo();
    }
}

void MainWindow::OnEditCopyData()
{
}

void MainWindow::OnEditCopy()
{
    MapView* m_p = activeMapView();
    if(m_p) m_p->OnEditCopy();
}

void MainWindow::OnEditSave()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        m_p->OnEditSave();
    }
}

void MainWindow::OnEditClear()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnEditClear();
    }
}

void MainWindow::OnEditQuery()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnEditQuery();
    }
}

void MainWindow::OnViewZoomsel()
{
    MapView* m_p = activeMapView();
    if(m_p) m_p->OnViewZoomsel();
}

void MainWindow::OnEditSelectToLayer()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnEditSelectToLayer();
    }
}

void MainWindow::OnFileImport()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnFileImport();
    }
}

void MainWindow::OnLayerNew()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnLayerNew();
    }
}

void MainWindow::OnLayerDelete()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnLayerDelete();
    }
}

void MainWindow::OnLayerConvert()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnLayerConvert();
    }
}

void MainWindow::OnLayerConvertDrawing()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnLayerConvertDrawing();
    }
}

void MainWindow::OnConvertMapShapes()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnConvertMapShapes();
    }
}

void MainWindow::OnFileExport()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnFileExport();
    }
}
void MainWindow::OnFileExportLinks()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnFileExportLinks();
    }
}

void MainWindow::OnAxialConnectionsExportAsDot()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnAxialConnectionsExportAsDot();
    }
}

void MainWindow::OnAxialConnectionsExportAsPairCSV()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnAxialConnectionsExportAsPairCSV();
    }
}

void MainWindow::OnSegmentConnectionsExportAsPairCSV()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnSegmentConnectionsExportAsPairCSV();
    }
}

void MainWindow::OnPointmapExportConnectionsAsCSV()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnPointmapExportConnectionsAsCSV();
    }
}

void MainWindow::OnAddColumn()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnAddColumn();
    }
}

void MainWindow::OnRenameColumn()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnRenameColumn();
    }
}

void MainWindow::OnUpdateColumn()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnUpdateColumn();
    }
}

void MainWindow::OnRemoveColumn()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnRemoveColumn();
    }
}

void MainWindow::OnColumnProperties()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnColumnProperties();
    }
}

void MainWindow::OnPushToLayer()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnPushToLayer();
    }
}

void MainWindow::OnEditGrid()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnEditGrid();
    }
}

void MainWindow::OnToolsMakeGraph()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsMakeGraph();
    }
}

void MainWindow::OnToolsImportVGALinks()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnVGALinksFileImport();
    }
}

void MainWindow::OnToolsRun()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsRun();
    }
}

void MainWindow::OnToolsAgentRun()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsAgentRun();
    }
}

void MainWindow::OnToolsIsovistpath()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsIsovistpath();
    }
}

void MainWindow::OnToolsAgentLoadProgram()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        if(m_p->m_view[QGraphDoc::VIEW_3D])
            ((Q3DView*)m_p->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentLoadProgram();
    }
}

void MainWindow::OnToolsRunAxa()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsRunAxa();
    }
}

void MainWindow::OnToolsPD()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsPD();
    }
}

void MainWindow::OnToolsMakeFewestLineMap()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsMakeFewestLineMap();
    }
}

void MainWindow::OnToolsAxialConvShapeMap()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsAxialConvShapeMap();
    }
}

void MainWindow::OnToolsLineLoadUnlinks()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsLineLoadUnlinks();
    }
}

void MainWindow::OnToolsRunSeg()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsRunSeg();
    }
}

void MainWindow::OnToolsTopomet()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsTopomet();
    }
}

void MainWindow::OnToolsTPD()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsTPD();
    }
}

void MainWindow::OnToolsMPD()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsMPD();
    }
}

void MainWindow::OnToolsPointConvShapeMap()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsPointConvShapeMap();
    }
}

void MainWindow::OnToolsAPD()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnToolsAPD();
    }
}

void MainWindow::OnToolsOptions()
{
    SettingsDialog dialog(mSettings);
    if(QDialog::Accepted == dialog.exec()) {
        readSettings();
    }
}

void MainWindow::OnShowResearchtoolbar()
{
    //if (GetApp()->m_show_researchtoolbar) {
    //	m_wndResearchToolBar.show();
    //}
    //else {
    //	m_wndResearchToolBar.hide();
    //}
}

void MainWindow::OnViewCentreView()
{
    activeMapDoc()->SetRedrawFlag(QGraphDoc::VIEW_MAP, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
}

void MainWindow::OnViewShowGrid()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnViewShowGrid();
    }
}

void MainWindow::OnViewSummary()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        m_p->OnViewSummary();
    }
}

void MainWindow::OnViewColourRange()
{
    if (m_wndColourScale.isVisible()) {
        m_wndColourScale.hide();
    }
    else {
        QRect recta,rectb;
        recta = geometry();
        rectb = m_wndColourScale.geometry();
        m_wndColourScale.setGeometry(recta.right() - 7 - rectb.width(), recta.top() + 68, rectb.width(), rectb.height());
        m_wndColourScale.m_docked = true;
        m_wndColourScale.show();
    }
}

void MainWindow::OnHelpBugs()
{
    bool foo = QDesktopServices::openUrl( QUrl("https://github.com/SpaceGroupUCL/depthmapX/issues") );
}

void MainWindow::OnHelpManual()
{
    bool foo = QDesktopServices::openUrl( QUrl("http://www.vr.ucl.ac.uk/depthmap/depthmap4r1.pdf") );
}

void MainWindow::OnHelpTutorials()
{
    bool foo = QDesktopServices::openUrl( QUrl("http://www.vr.ucl.ac.uk/depthmap/tutorials/") );
}

void MainWindow::OnHelpSalaManual()
{
    bool foo = QDesktopServices::openUrl( QUrl("http://www.vr.ucl.ac.uk/depthmap/scripting/") );
}

void MainWindow::OnFileClose()
{
    MapView* m_p = activeMapView();
    if(m_p) QApplication::postEvent((QObject*)m_p, new QEvent(QEvent::Close));
}

void MainWindow::OnFileSave()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        bool saved = m_p->OnFileSave();
        if(saved) {
            statusBar()->showMessage(tr("File saved"), 2000);
            setCurrentFile(m_p->m_opened_name);
            updateSubWindowTitles(m_p->m_base_title);
        } else {
            statusBar()->showMessage(tr("File not saved"), 2000);
        }
    }
}

void MainWindow::OnFileSaveAs()
{
    QGraphDoc* m_p = activeMapDoc();
    if(m_p)
    {
        bool saved = m_p->OnFileSaveAs();
        if(saved) {
            statusBar()->showMessage(tr("File saved"), 2000);
            setCurrentFile(m_p->m_opened_name);
            updateSubWindowTitles(m_p->m_base_title);
        } else {
            statusBar()->showMessage(tr("File not saved"), 2000);
        }
    }
}
void MainWindow::updateSubWindowTitles(QString newTitle) {
    QList<QMdiSubWindow *> windowList = mdiArea->subWindowList();
    QList<QMdiSubWindow *>::iterator iter = windowList.begin(), end =
    windowList.end();
    for ( ; iter != end; ++iter )
    {
        QWidget *p = 0;
        if (QMdiSubWindow *subWindow = *iter)
        {
            p = qobject_cast<MapView *>(subWindow->widget());
            if(p) subWindow->setWindowTitle(newTitle +":Map View");
            p = qobject_cast<QPlotView *>(subWindow->widget());
            if(p) subWindow->setWindowTitle(newTitle +":Scatter Plot");
            p = qobject_cast<tableView *>(subWindow->widget());
            if(p) subWindow->setWindowTitle(newTitle +":Table View");
            p = qobject_cast<Q3DView *>(subWindow->widget());
            if(p) subWindow->setWindowTitle(newTitle +":3D View");
        }
    }

}

void MainWindow::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.exec();
}

MapView *MainWindow::createMapView()
{
    QGraphDoc* doc = new QGraphDoc("", "");
    doc->m_mainFrame = this;

    if(m_defaultMapWindowIsLegacy)
    {
        QDepthmapView *child = new QDepthmapView(*doc, mSettings);
        mdiArea->addSubWindow(child);
        return child;
    }
    else
    {
        GLView *child = new GLView(*doc, mSettings);
        mdiArea->addSubWindow(child);
        return child;
    }

}

MapView *MainWindow::activeMapView()
{
    QWidget *p = 0;
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
    {
        p = qobject_cast<MapView *>(activeSubWindow->widget());
        if(p) return (MapView *)p;
        if(!p)
        {
            p = qobject_cast<QPlotView *>(activeSubWindow->widget());
            if(p) return (MapView *)(((QPlotView*)p)->pDoc->m_view[1]);
        }
        if(!p)
        {
            p = qobject_cast<tableView *>(activeSubWindow->widget());
            if(p) return (MapView *)(((tableView*)p)->pDoc->m_view[1]);
        }
        if(!p)
        {
            p = qobject_cast<Q3DView *>(activeSubWindow->widget());
            if(p) return (MapView *)(((Q3DView*)p)->pDoc->m_view[1]);
        }
    }
    current_view_type = 0;
    return 0;
}

QGraphDoc *MainWindow::activeMapDoc()
{
    QWidget *p = 0;
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
    {
        p = qobject_cast<MapView *>(activeSubWindow->widget());
        if(p) return ((MapView *)p)->getGraphDoc();
        p = qobject_cast<QPlotView *>(activeSubWindow->widget());
        if(p) return ((QPlotView *)p)->pDoc;
        p = qobject_cast<tableView *>(activeSubWindow->widget());
        if(p) return ((tableView *)p)->pDoc;
        p = qobject_cast<Q3DView *>(activeSubWindow->widget());
        if(p) return ((Q3DView *)p)->pDoc;
    }
    return 0;
}

QMdiSubWindow *MainWindow::findMapView(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        MapView *mdiChild = qobject_cast<MapView *>(window->widget());
          if (mdiChild && mdiChild->getCurrentFile() == canonicalFilePath) return window;
    }
    return 0;
}

void MainWindow::OnWindowMap()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        if(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP])
            return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP]);
        QDepthmapView *child = new QDepthmapView(*m_p->getGraphDoc(), mSettings);
        mdiArea->addSubWindow(child);
        child->show();
    }
}

void MainWindow::OnViewTable()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        if(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_TABLE])
            return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_TABLE]);
        tableView *child = new tableView(this, m_p->getGraphDoc());
        child->pDoc = m_p->getGraphDoc();
        mdiArea->addSubWindow(child);
        child->show();
    }
}

void MainWindow::OnWindow3dView()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        if(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_3D])
            return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_3D]);
        Q3DView *child = new Q3DView(this, m_p->getGraphDoc());
        child->pDoc = m_p->getGraphDoc();
        mdiArea->addSubWindow(child);
        child->show();
    }
}

void MainWindow::OnWindowGLView()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        if(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP_GL])
            return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP_GL]);
        GLView *child = new GLView(*m_p->getGraphDoc(), mSettings);
        mdiArea->addSubWindow(child);
        child->show();
    }
}

void MainWindow::OnViewScatterplot()
{
    MapView* m_p = activeMapView();
    if(m_p)
    {
        if(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_SCATTER])
            return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_SCATTER]);
        QPlotView *child = new QPlotView;
        child->pDoc = m_p->getGraphDoc();
        child->m_parent = this;
        mdiArea->addSubWindow(child);
        child->show();
    }
}

void MainWindow::update3DToolbar()
{
    updateActiveWindows();
}

void MainWindow::updateActiveWindows()
{
    current_view_type = 0;
    QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow();
    if(!activeSubWindow)
    {
        editToolBar->hide();
        thirdViewToolBar->hide();
        plotToolBar->hide();
        return;
    }

    QWidget* p = qobject_cast<QPlotView *>(activeSubWindow->widget());
    if(p)
    {
        editToolBar->hide();
        thirdViewToolBar->hide();
        plotToolBar->show();
        current_view_type = QGraphDoc::VIEW_SCATTER;
        OnFocusGraph(((QPlotView*)p)->pDoc, QGraphDoc::CONTROLS_LOADALL);
        RedoPlotViewMenu(((QPlotView*)p)->pDoc);

        if(((QPlotView*)p)->m_view_monochrome) toggleColor->setChecked(true);
        else toggleColor->setChecked(false);
        if(((QPlotView*)p)->m_view_origin) toggleOrg->setChecked(true);
        else toggleOrg->setChecked(false);
        if(((QPlotView*)p)->m_view_trend_line) viewTrend->setChecked(true);
        else viewTrend->setChecked(false);
        if(((QPlotView*)p)->m_view_equation) yx->setChecked(true);
        else yx->setChecked(false);
        if(((QPlotView*)p)->m_view_rsquared) Rtwo->setChecked(true);
        else Rtwo->setChecked(false);
    }
    else if(qobject_cast<tableView *>(activeSubWindow->widget()))
    {
        editToolBar->hide();
        thirdViewToolBar->hide();
        plotToolBar->hide();
        current_view_type = QGraphDoc::VIEW_TABLE;
        return;
    }
    else if(p = qobject_cast<Q3DView *>(activeSubWindow->widget()))
    {
        editToolBar->hide();
        plotToolBar->hide();
        thirdViewToolBar->show();
        QGraphDoc* pDoc = activeMapDoc();
        Q3DView *ptr = (Q3DView *)p;

        if(ptr->m_animating) toolsAgentsPlayAct->setChecked(true);
        else toolsAgentsPlayAct->setChecked(0);
        if(!ptr->m_animating) toolsAgentsPauseAct->setChecked(true);
        else toolsAgentsPauseAct->setChecked(0);

        if(ptr->m_mouse_mode == ID_3D_PAN) thirdPanAct->setChecked(true);
        else thirdPanAct->setChecked(0);
        if(ptr->m_mouse_mode == ID_3D_ROT) thirdRotAct->setChecked(true);
        else thirdRotAct->setChecked(0);
        if(ptr->m_mouse_mode == ID_3D_ZOOM) thirdZoomAct->setChecked(true);
        else thirdZoomAct->setChecked(0);
        if(ptr->m_mouse_mode == ID_3D_PLAY_LOOP) playLoopAct->setChecked(true);
        else playLoopAct->setChecked(0);
        if(ptr->m_fill) thirdFilledAct->setChecked(true);
        else thirdFilledAct->setChecked(0);

        if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints())
            toolsImportTracesAct->setEnabled(true);
        else
            toolsImportTracesAct->setEnabled(false);

        if (!pDoc->m_meta_graph || !pDoc->m_meta_graph->viewingProcessedPoints())
        {
            if (ptr->m_mouse_mode == ID_ADD_AGENT) ptr->m_mouse_mode = ID_3D_ROT;
            addAgentAct->setEnabled(false);
        }
        else
        {
            addAgentAct->setEnabled(true);
            if (ptr->m_mouse_mode == ID_ADD_AGENT) addAgentAct->setChecked(1);
            else addAgentAct->setChecked(0);
        }

        if (ptr->m_mannequins.size())
        {
            toolsAgentsPlayAct->setEnabled(true);
            toolsAgentsPauseAct->setEnabled(true);
            toolsAgentsStopAct->setEnabled(true);

            if (((Q3DView *)p)->m_animating) toolsAgentsPlayAct->setChecked(true);
            else toolsAgentsPlayAct->setChecked(false);
        }
        else
        {
            toolsAgentsPlayAct->setChecked(false);
            toolsAgentsPlayAct->setEnabled(false);
            toolsAgentsPauseAct->setEnabled(false);
            toolsAgentsStopAct->setEnabled(false);
        }

        if (ptr->m_mannequins.size()) {
            toolsAgentsPauseAct->setEnabled(true);
            if (!ptr->m_animating) {
                toolsAgentsPauseAct->setChecked(true);
            }
            else {
                toolsAgentsPauseAct->setChecked(false);
            }
        }
        else {
            toolsAgentsPauseAct->setChecked(false);
            toolsAgentsPauseAct->setEnabled(false);
        }

        if (ptr->m_mannequins.size()) {
            toolsAgentsStopAct->setEnabled(true);
        }
        else {
            toolsAgentsStopAct->setEnabled(false);
        }

        if (ptr->m_drawtrails) {
            agentTrailsAct->setChecked(1);
        }
        else {
            agentTrailsAct->setChecked(0);
        }

        if (ptr->m_fill) {
            thirdFilledAct->setChecked(1);
        }
        else {
            thirdFilledAct->setChecked(0);
        }
        current_view_type = QGraphDoc::VIEW_3D;
        return;
    }
    else if((p = qobject_cast<MapView *>(activeSubWindow->widget())))
    {
        editToolBar->show();
        thirdViewToolBar->hide();
        plotToolBar->hide();
        current_view_type = QGraphDoc::VIEW_MAP;
        QWidget* v = qobject_cast<MapView *>(activeSubWindow->widget());
        if(v) current_view_type = QGraphDoc::VIEW_MAP_GL;
        switch(m_selected_mapbar_item)
        {
        case ID_MAPBAR_ITEM_SELECT:
            SelectButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_MOVE:
            DragButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_ZOOM_IN:
            zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
            zoomToolButton->setChecked(true);
            zoomInAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_ZOOM_OUT:
            zoomToolButton->setIcon(QIcon(":/images/win/b-5-4.png"));
            zoomToolButton->setChecked(true);
            zoomOutAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_FILL:
            fillColorToolButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_SEMIFILL:
            fillColorToolButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_AUGMENT_FILL: // AV TV
            fillColorToolButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_PENCIL:
            SelectPenButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_LINETOOL:
            lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
            lineToolButton->setChecked(true);
            SelectLineAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_POLYGON:
            lineToolButton->setIcon(QIcon(":/images/win/b-5-11.png"));
            lineToolButton->setChecked(true);
            SelectPolyLineAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_ISOVIST:
            newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
            newisoToolButton->setChecked(true);
            MakeIosAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_HALFISOVIST:
            newisoToolButton->setIcon(QIcon(":/images/win/b-5-13.png"));
            newisoToolButton->setChecked(true);
            PartialMakeIosAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_AL2:
            AxialMapButton->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_JOIN:
            JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
            JoinToolButton->setChecked(true);
            JoinAct->setChecked(true);
            break;
        case ID_MAPBAR_ITEM_UNJOIN:
            JoinToolButton->setIcon(QIcon(":/images/win/b-5-17.png"));
            JoinToolButton->setChecked(true);
            JoinUnlinkAct->setChecked(true);
            break;
        default:
            SelectButton->setChecked(true);
            SelectButton->setChecked(false);
            break;
        }
        QGraphDoc* m_p = activeMapDoc();
        OnFocusGraph(m_p, QGraphDoc::CONTROLS_LOADALL);
        m_p->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_FOCUS );
    }
}

void MainWindow::updateGLWindows(bool datasetChanged, bool recentreView) {
    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
        if(!child) continue;
        if(datasetChanged) child->notifyDatasetChanged();
        if(recentreView) child->matchViewToCurrentMetaGraph();
    }
}

void MainWindow::switchLayoutDirection()
{
    if (layoutDirection() == Qt::LeftToRight)
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);
}

void MainWindow::setActiveSubWindow(QWidget *win)
{
    if (!win) return;
    foreach (QMdiSubWindow *window, mdiArea->subWindowList())
    {
        if(window->widget() == win)
        {
            mdiArea->setActiveSubWindow(window);
            return;
        }
    }
    QString t = QString(TITLE_BASE);
    setWindowTitle(t+" "+windowTitle());
}

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static bool in_FocusGraph;
int MainWindow::OnFocusGraph(QGraphDoc* pDoc, int lParam)
{
    in_FocusGraph = true;
    updateToolbar();
    x_coord->clear();
    y_coord->clear();

    // Replacement for m_linelayer_chooser is my tree ctrl:
    if (lParam == QGraphDoc::CONTROLS_DESTROYALL && pDoc == m_treeDoc) {        // Lost graph
        delete pDoc;
        m_treeDoc = NULL;
        m_topgraph = NULL;
        m_backgraph = NULL;
        m_attrWindow->clear();
        m_indexWidget->clear();
    }
    else if (lParam == QGraphDoc::CONTROLS_LOADALL && pDoc != m_treeDoc) {     // [Possible] change of window (sent on focus)
        m_treeDoc = pDoc;
        m_topgraph = NULL;
        m_backgraph = NULL;
        MakeTree();
    }
    else if (lParam == QGraphDoc::CONTROLS_LOADGRAPH && pDoc == m_treeDoc) {     // Force update if match current window
        m_topgraph = NULL;
        m_backgraph = NULL;
        m_attrWindow->clear();
        m_indexWidget->clear();
        ClearGraphTree();
        MakeGraphTree();
        // also make drawing tree as this overrides layer visible status sometimes:
        MakeDrawingTree();
    }
    else if (lParam == QGraphDoc::CONTROLS_RELOADGRAPH && pDoc == m_treeDoc) {     // Force reload of graph tree if match current window
        m_topgraph = NULL;
        m_backgraph = NULL;
        m_attrWindow->clear();
        m_indexWidget->clear();
        ClearGraphTree();
        MakeTree();
        MakeGraphTree();
    }
    else if (lParam == QGraphDoc::CONTROLS_LOADDRAWING && pDoc == m_treeDoc) {     // Force update if match current window
        MakeDrawingTree();
    }
    else if (lParam == QGraphDoc::CONTROLS_LOADATTRIBUTES && pDoc == m_treeDoc) {     // Force update if match current window
        MakeAttributeList();
    }
    else if (lParam == QGraphDoc::CONTROLS_CHANGEATTRIBUTE && pDoc == m_treeDoc) {     // Force update if match current window
        SetAttributeChecks();
    }
    else if (lParam == QGraphDoc::CONTROLS_LOADCONVERT && pDoc == m_treeDoc) {
        m_topgraph = NULL;
        m_backgraph = NULL;
        m_attrWindow->clear();
        m_indexWidget->clear();
        ClearGraphTree();
        MakeGraphTree();
        // conversions typically turn off drawing layers:
        SetDrawingTreeChecks();
    }
    if (m_treeDoc == NULL) {
//		tree.EnableWindow(FALSE);
        // Stop some strange auto scroll property:
//		SetTreeStyle(TVS_NOSCROLL, TRUE);
    }
    else {
//		tree.EnableWindow(TRUE);
        // Stop some strange auto scroll property:
//		SetTreeStyle(TVS_NOSCROLL, FALSE);
    }

    m_wndColourScale.OnFocusGraph(pDoc, lParam);

    in_FocusGraph = false;
    return 0;
}

void MainWindow::MakeTree()
{
    m_indexWidget->clear();
    m_treegraphmap.clear();
    m_treedrawingmap.clear();

    for (int i = 0; i < 5; i++) m_treeroots[i] = NULL;

    MetaGraph *graph = m_treeDoc->m_meta_graph;
    if (!graph) return;

    int state = graph->getState();
    int viewclass = graph->getViewClass();

    MakeGraphTree();
    MakeDrawingTree();
}

void MainWindow::OnSelchangingList()
{
    if(in_FocusGraph) return;

    int row = -1;
    row = m_attrWindow->currentRow();
    if(row > -1 && m_treeDoc){
      MetaGraph *graph = m_treeDoc->m_meta_graph;
      if (graph->viewingProcessed()) {
         graph->setDisplayedAttribute(row - 1);
      }
      m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_FOCUS );
      SetAttributeChecks();
	  OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE); // Bug Test TV
   }

   // this *does* work here (but only if they click on a valid attribute):
}

void MainWindow::OnSelchangingTree(QTreeWidgetItem* hItem, int col)
{
    if(in_FocusGraph) return;

    MetaGraph *graph = m_treeDoc->m_meta_graph;
    bool update = false;

    // look it up in the table to see what to do:
    size_t n = m_treegraphmap.searchindex(hItem);
    if (n != paftl::npos) {
        ItemTreeEntry entry = m_treegraphmap.value(n);
        bool remenu = false;
        if (entry.m_cat != -1) {
            if (entry.m_subcat == -1) {
                switch (entry.m_type) {
                case 0:
                    if (graph->getViewClass() & MetaGraph::VIEWVGA) {
                        if (graph->getDisplayedPointMapRef() == entry.m_cat) {
                            graph->setViewClass(MetaGraph::SHOWHIDEVGA);
                        }
                        else {
                            graph->setDisplayedPointMapRef(entry.m_cat);
                        }
                    }
                    else {
                        graph->setDisplayedPointMapRef(entry.m_cat);
                        graph->setViewClass(MetaGraph::SHOWVGATOP);
                    }
                    remenu = true;
                    break;
                case 1:
                   if (graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                      if (graph->getShapeGraphs().getDisplayedMapRef() == entry.m_cat) {
                         graph->setViewClass(MetaGraph::SHOWHIDEAXIAL);
                      }
                      else {
                         graph->getShapeGraphs().setDisplayedMapRef(entry.m_cat);
                      }
                   }
                   else {
                      graph->getShapeGraphs().setDisplayedMapRef(entry.m_cat);
                      graph->setViewClass(MetaGraph::SHOWAXIALTOP);
                   }
                    remenu = true;
                    break;
                case 2:
                   if (graph->getViewClass() & MetaGraph::VIEWDATA) {
                      if (graph->getDataMaps().getDisplayedMapRef() == entry.m_cat) {
                         graph->setViewClass(MetaGraph::SHOWHIDESHAPE);
                      }
                      else {
                         graph->getDataMaps().setDisplayedMapRef(entry.m_cat);
                      }
                   }
                   else {
                      graph->getDataMaps().setDisplayedMapRef(entry.m_cat);
                      graph->setViewClass(MetaGraph::SHOWSHAPETOP);
                   }
                    remenu = true;
                    break;
                case 4:
                    // slightly different for this one
                    break;
                }
                if (remenu) {
                    SetGraphTreeChecks();
                    m_treeDoc->SetRemenuFlag(QGraphDoc::VIEW_ALL, true);
                    OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
                m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE );
            }
            else if (entry.m_subcat == -2) {
                // hit editable box
                if (entry.m_type == 1) {
                    int type = graph->getShapeGraphs().getMap(entry.m_cat).getMapType();
                    if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
                        graph->getShapeGraphs().getMap(entry.m_cat).setEditable(hItem->checkState(0));
                        update = true;
                    }
                }
                if (entry.m_type == 2) {
                    graph->getDataMaps().getMap(entry.m_cat).setEditable(hItem->checkState(0));
                    update = true;
                }
                if (update) {
                    // Depending on if the map is displayed you may have to redraw -- I'm just going to redraw *anyway*
                    // (it may be worth switching it to topmost when they do click here)
                    OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
            }
            else {
                // They've clicked on the displayed layers
                if (entry.m_type == 1) {
                   update = true;
                   graph->getShapeGraphs().getMap(entry.m_cat).setLayerVisible(entry.m_subcat, hItem->checkState(0));
                }
                else if (entry.m_type == 2) {
                   update = true;
                   graph->getDataMaps().getMap(entry.m_cat).setLayerVisible(entry.m_subcat, hItem->checkState(0));
                }
                if (update) {
                    m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE );
                    OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
            }
        }
    }
    else {
        size_t n = m_treedrawingmap.searchindex(hItem);
        if (n != paftl::npos) {
            ItemTreeEntry entry = m_treedrawingmap.value(n);
            if (entry.m_subcat != -1) {
                if (graph->getLineLayer(entry.m_cat,entry.m_subcat).isShown()) {
                    graph->getLineLayer(entry.m_cat,entry.m_subcat).setShow(false);
                    graph->PointMaps::redoBlockLines();
                    graph->resetBSPtree();
                }
                else {
                    graph->getLineLayer(entry.m_cat,entry.m_subcat).setShow(true);
                    graph->PointMaps::redoBlockLines();
                    graph->resetBSPtree();
                }
            }
            m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_LINESET );
        }
    }
}

void MainWindow::SetGraphTreeChecks()
{
    in_FocusGraph = true;
    MetaGraph *graph = m_treeDoc->m_meta_graph;
    int viewclass = graph->getViewClass();
    for (size_t i = 0; i < m_treegraphmap.size(); i++) {
        QTreeWidgetItem* key = m_treegraphmap.key(i);
        ItemTreeEntry entry = m_treegraphmap[i];
        int checkstyle = 7;
        if (entry.m_cat != -1) {
            if (entry.m_subcat == -1) {
                // this is the main type box hit
                switch (entry.m_type) {
                    case 0:
                        if (viewclass & MetaGraph::VIEWVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                    case 1:
                        if (viewclass & MetaGraph::VIEWAXIAL && graph->getShapeGraphs().getDisplayedMapRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKAXIAL && graph->getShapeGraphs().getDisplayedMapRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                    case 2:
                        if (viewclass & MetaGraph::VIEWDATA && graph->getDataMaps().getDisplayedMapRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKDATA && graph->getDataMaps().getDisplayedMapRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                }
                if(checkstyle == 5) key->setCheckState(0, Qt::Checked);
                else if(checkstyle == 6) key->setCheckState(0, Qt::PartiallyChecked);
                else if(checkstyle == 7) key->setCheckState(0, Qt::Unchecked);
            }
            else if (entry.m_subcat == -2) {
                // the editable box
                int editable = MetaGraph::NOT_EDITABLE;
                switch (entry.m_type) {
                    case 0:
                        if (graph->PointMaps::at(entry.m_cat).isProcessed()) {
                            editable = MetaGraph::NOT_EDITABLE;
                        }
                        else {
                            editable = MetaGraph::EDITABLE_ON;
                        }
                        break;
                    case 1:
                        {
                            int type = graph->getShapeGraphs().getMap(entry.m_cat).getMapType();
                            if (type == ShapeMap::SEGMENTMAP || type == ShapeMap::ALLLINEMAP) {
                                editable = MetaGraph::NOT_EDITABLE;
                            }
                            else {
                                editable = graph->getShapeGraphs().getMap(entry.m_cat).isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
                            }
                        }
                        break;
                    case 2:
                        editable = graph->getDataMaps().getMap(entry.m_cat).isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
                        break;
                }
                switch (editable) {
                    case MetaGraph::NOT_EDITABLE:
                        key->setFlags(0);
                        break;
                    case MetaGraph::EDITABLE_OFF:
                        key->setCheckState(0, Qt::Unchecked);
                        key->setText(0, editstatetext[2]);
                        break;
                    case MetaGraph::EDITABLE_ON:
                        key->setCheckState(0, Qt::Checked);
                        key->setText(0, editstatetext[2]);
                    break;
                }
            }
            else {
                // the displayed layers (note that VGA graphs (type 0)
                // do not currently have layers supported
                bool show = false;
                if (entry.m_type == 1) {
                    show = graph->getShapeGraphs().getMap(entry.m_cat).isLayerVisible(entry.m_subcat);
                }
                else if (entry.m_type == 2) {
                    show = graph->getDataMaps().getMap(entry.m_cat).isLayerVisible(entry.m_subcat);
                }
                if (show) {
                      key->setCheckState(0, Qt::Checked);
                }
                else {
                      key->setCheckState(0, Qt::Unchecked);
                }
            }
        }
    }
    MakeAttributeList();
    in_FocusGraph = false;
}

void MainWindow::SetDrawingTreeChecks()
{
    MetaGraph *graph = m_treeDoc->m_meta_graph;
    int viewclass = graph->getViewClass();
    for (size_t i = 0; i < m_treedrawingmap.size(); i++) {
        ItemTreeEntry entry = m_treedrawingmap[i];
        if (entry.m_subcat != -1) {
            if (graph->getLineLayer(entry.m_cat,entry.m_subcat).isShown()) {
                  m_treedrawingmap.key(i)->setIcon(0, m_tree_icon[12]);
            }
            else {
                  m_treedrawingmap.key(i)->setIcon(0, m_tree_icon[13]);
            }
        }
    }
}

// clear the graph tree (not the drawing tree) but also clear the attribute list

void MainWindow::ClearGraphTree()
{
    m_attribute_locked.clear();

    for (int i = 2; i >= 0; i--) {
        if (m_treeroots[i]) {
            m_treeroots[i] = NULL;
        }
    }
    m_treegraphmap.clear();
}

void MainWindow::MakeGraphTree()
{
    MetaGraph *graph = m_treeDoc->m_meta_graph;

    int state = graph->getState();
    int viewclass = graph->getViewClass();

    if (state & MetaGraph::POINTMAPS) {
        if (!m_treeroots[0]) {
            QTreeWidgetItem* hItem = m_indexWidget->addNewRootFolder(tr("Visibility Graphs"));
            hItem->setIcon(0, m_tree_icon[0]);
            ItemTreeEntry entry(0,-1,-1);
            m_treegraphmap.add(hItem, entry);
            m_treeroots[0] = hItem;
        }
        QTreeWidgetItem* hItem = m_treeroots[0]->child(0);
        for (size_t i = 0; i < m_treeDoc->m_meta_graph->PointMaps::size(); i++) {
            QString name = QString(m_treeDoc->m_meta_graph->PointMaps::at(i).getName().c_str());
            if (hItem == NULL) {
                hItem = m_indexWidget->addNewFolder(name, m_treeroots[0]);
                hItem->setCheckState(0, Qt::Unchecked);
                ItemTreeEntry entry(0,(short)i,-1);
                m_treegraphmap.add(hItem,entry);
                {
                    QTreeWidgetItem* hNewItem = m_indexWidget->addNewItem("Editable", 0);
                    hNewItem->setCheckState(0, Qt::Unchecked);
                    ItemTreeEntry newentry = ItemTreeEntry(0,(short)i,-2);
                    m_treegraphmap.add(hNewItem, newentry);
                }
            }
            else if (hItem->text(0) != name) hItem->setText(0, name);
            hItem = hItem->child(1);
        }
        while (hItem != NULL)
        {
            QTreeWidgetItem* hItemDestroy = hItem;
            hItem = hItemDestroy->child(0);
            hItemDestroy->removeChild(hItemDestroy);
        }
    }
    else if (m_treeroots[0]) {
        m_treeroots[0]->removeChild(m_treeroots[0]);
        m_treegraphmap.remove(m_treeroots[0]);
        m_treeroots[0] = NULL;
    }

    if (state & MetaGraph::SHAPEGRAPHS) {
        if (!m_treeroots[1]) {
            QTreeWidgetItem* hItem = m_indexWidget->addNewRootFolder(tr("Shape Graphs"));
            hItem->setIcon(0, m_tree_icon[1]);
            ItemTreeEntry entry(1,-1,-1);
            m_treegraphmap.add(hItem, entry);
            m_treeroots[1] = hItem;
        }
        QTreeWidgetItem* hItem = m_treeroots[1]->child(0);
        for (size_t i = 0; i < m_treeDoc->m_meta_graph->getShapeGraphs().getMapCount(); i++) {
            QString name = QString(m_treeDoc->m_meta_graph->getShapeGraphs().getMap(i).getName().c_str());
            if (hItem == NULL) {
                hItem = m_indexWidget->addNewFolder(name, m_treeroots[1]);
                hItem->setCheckState(0, Qt::Unchecked);
                ItemTreeEntry entry(1,(short)i,-1);
                m_treegraphmap.add(hItem,entry);
                {
                    QTreeWidgetItem* hNewItem = m_indexWidget->addNewItem("Editable", 0);
                    hNewItem->setCheckState(0, Qt::Unchecked);
                    ItemTreeEntry newentry = ItemTreeEntry(1,(short)i,-2);
                    m_treegraphmap.add(hNewItem, newentry);
                }
            }
            else if (hItem->text(0) != name) hItem->setText(0, name);
            QTreeWidgetItem* hNewItem = hItem->child(0);
            AttributeTable& table = m_treeDoc->m_meta_graph->getShapeGraphs().getMap(i).getAttributeTable();
            for (int j = 0; j < table.getLayerCount(); j++) {
                QString name = QString(table.getLayerName(j).c_str());
                if (hNewItem == NULL) {
                    hNewItem = m_indexWidget->addNewItem(name, 0);
                    ItemTreeEntry entry(1,(short)i,j);
                    m_treegraphmap.add(hNewItem,entry);
                }
                else if (hNewItem->text(0) != name) hNewItem->setText(0, name);
                hNewItem = hNewItem->child(1);
            }
            hItem = hNewItem;
        }
        while (hItem != NULL)
        {
            QTreeWidgetItem* hItemDestroy = hItem;
            hItem = hItemDestroy->child(0);
            hItemDestroy->removeChild(hItemDestroy);
        }
    }
    else if (m_treeroots[1]) {
        m_treeroots[1]->removeChild(m_treeroots[1]);
        m_treegraphmap.remove(m_treeroots[1]);
        m_treeroots[1] = NULL;
    }

    if (state & MetaGraph::DATAMAPS) {
        if (!m_treeroots[2]) {
            QTreeWidgetItem* hItem = m_indexWidget->addNewRootFolder(tr("Data Maps"));
            hItem->setIcon(0, m_tree_icon[2]);
            ItemTreeEntry entry(2,-1,-1);
            m_treegraphmap.add(hItem, entry);
            m_treeroots[2] = hItem;
        }
        QTreeWidgetItem* hItem = m_treeroots[2]->child(0);
        for (size_t i = 0; i < m_treeDoc->m_meta_graph->getDataMaps().getMapCount(); i++) {
            QString name = QString(m_treeDoc->m_meta_graph->getDataMaps().getMap(i).getName().c_str());
            if (hItem == NULL)
            {
                hItem = m_indexWidget->addNewFolder(name, m_treeroots[2]);
                hItem->setCheckState(0, Qt::Unchecked);
                ItemTreeEntry entry(2,(short)i,-1);
                m_treegraphmap.add(hItem, entry);
                {
                    hItem = m_indexWidget->addNewItem(tr("Editable"), 0);
                    hItem->setCheckState(0, Qt::Unchecked);
                    ItemTreeEntry newentry = ItemTreeEntry(2,(short)i,-2);
                    m_treegraphmap.add(hItem, newentry);
                }
            }
            else if (hItem->text(0) != name) hItem->setText(0, name);

            QTreeWidgetItem* hNewItem = hItem->child(0);
            AttributeTable& table = m_treeDoc->m_meta_graph->getDataMaps().getMap(i).getAttributeTable();
            for (int j = 0; j < table.getLayerCount(); j++) {
                QString name = QString(table.getLayerName(j).c_str());
                if (hNewItem == NULL) {
                    hNewItem = m_indexWidget->addNewItem(name, 0);
                    hNewItem->setCheckState(0, Qt::Unchecked);
                    ItemTreeEntry entry(2,(short)i,j);
                    m_treegraphmap.add(hNewItem,entry);
                }
                else if (hNewItem->text(0) != name) hNewItem->setText(0, name);
                hNewItem = hNewItem->child(0);
            }
            hItem = hNewItem;
        }
        while (hItem != NULL)
        {
            QTreeWidgetItem* hItemDestroy = hItem;
            hItem = hItemDestroy->child(0);
            hItemDestroy->removeChild(hItemDestroy);
        }
    }
    else if (m_treeroots[2]) {
        m_treeroots[2]->removeChild(m_treeroots[2]);
        m_treegraphmap.remove(m_treeroots[2]);
        m_treeroots[2] = NULL;
    }

    SetGraphTreeChecks();
}

void MainWindow::MakeDrawingTree()
{
    MetaGraph *graph = m_treeDoc->m_meta_graph;

    int state = graph->getState();

    if (state & MetaGraph::LINEDATA) {
        if (m_treeroots[4]) {
            m_treeroots[4] = NULL;
            m_treedrawingmap.clear();
        }
        // we'll do all of these if it works...
        QTreeWidgetItem* root = m_indexWidget->addNewRootFolder(tr("Drawing Layers"));
        root->setIcon(0, m_tree_icon[4]);
        ItemTreeEntry entry(4,0,-1);
        m_treedrawingmap.add(root,entry);
        m_treeroots[4] = root;
        for (int i = 0; i < m_treeDoc->m_meta_graph->getLineFileCount(); i++) {

            QTreeWidgetItem* subroot = m_indexWidget->addNewFolder(QString(m_treeDoc->m_meta_graph->getLineFileName(i).c_str()));
            subroot->setIcon(0, m_tree_icon[8]);
            ItemTreeEntry entry(4,i,-1);
            m_treedrawingmap.add(subroot,entry);

            for (int j = 0; j < m_treeDoc->m_meta_graph->getLineLayerCount(i); j++) {
                QString name(m_treeDoc->m_meta_graph->getLineLayer(i,j).getName().c_str());
                QTreeWidgetItem* hItem = m_indexWidget->addNewItem(name, 0);
                if (m_treeDoc->m_meta_graph->getLineLayer(i,j).isShown()) {
                    hItem->setCheckState(0, Qt::Checked);
                }
                else {
                    hItem->setCheckState(0, Qt::Unchecked);
                }
                ItemTreeEntry entry(4,i,j);
                m_treedrawingmap.add(hItem,entry);
            }
        }
    }
}

void MainWindow::MakeAttributeList()
{
    MetaGraph *graph = m_treeDoc->m_meta_graph;
    if (graph == NULL) {
        return;
    }
    auto lock = graph->getLockDeferred();
    if (lock.try_lock()) {

        // just doing this the simple way to start off with
        // (when you add new attributes, list is cleared and re
        m_attribute_locked.clear();
        m_attrWindow->clear();

        int cx = 0;
        QString name;
        if (graph->viewingProcessed()) {
            const AttributeTable& table = graph->getAttributeTable();
            m_attrWindow->addItem(tr("Ref Number"));
            m_attribute_locked.push_back(true);

            for (int i = 0; i < table.getColumnCount(); i++) {
                name = QString(table.getColumnName(i).c_str());
                    m_attrWindow->addItem(name);
                    m_attribute_locked.push_back(table.isColumnLocked(i));
                //}
            }
        }
    }

    SetAttributeChecks();
}

void MainWindow::SetAttributeChecks()
{
    MetaGraph *graph = m_treeDoc->m_meta_graph;
    if (graph == NULL) return;

    QListWidgetItem * it;
    if (graph->viewingProcessed()) {
        int image, displayed_attribute = graph->getDisplayedAttribute();
        for (int i = 0; ; i++) {
            it = m_attrWindow->item(i);
            if(!it) break;
            if ((i-1) == displayed_attribute) {
                if (!m_attribute_locked[i]) {
                    image = 9;
                }
                else {
                    image = 17;
                }
            }
            else {
                if (!m_attribute_locked[i]) {
                    image = 10;
                }
                else {
                    image = 18;
                }
            }
            it->setIcon(m_tree_icon[image]);
        }
    }
}

void MainWindow::OninvertColor()
{
    activeMapDoc()->OnSwapColours();
}

void MainWindow::OnzoomTo()
{
    activeMapView()->OnViewZoomsel();
}

void MainWindow::SelectButtonTriggered()
{
    m_selected_mapbar_item = ID_MAPBAR_ITEM_SELECT;
    activeMapView()->OnEditSelect();
}

void MainWindow::DragButtonTriggered()
{
    m_selected_mapbar_item = ID_MAPBAR_ITEM_MOVE;
    activeMapView()->OnViewPan();
}

void MainWindow::SelectPenTriggered()
{
    m_selected_mapbar_item = ID_MAPBAR_ITEM_PENCIL;
    activeMapView()->OnEditPencil();
}

void MainWindow::AxialMapTriggered()
{
    m_selected_mapbar_item = ID_MAPBAR_ITEM_AL2;
    activeMapView()->OnModeSeedAxial();
}

void MainWindow::StepDepthTriggered()
{
    activeMapDoc()->OnToolsPD();
}

void MainWindow::zoomButtonTriggered()
{
    int id = zoomInAct->data().value<int>();
    if(id == ID_MAPBAR_ITEM_ZOOM_IN)
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_IN;
        activeMapView()->OnViewZoomIn();
    }
    else
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_OUT;
        activeMapView()->OnViewZoomOut();
    }
}

void MainWindow::FillButtonTriggered()
{
    int id;// = qVariantValue<int>(STDFillColorAct->data());
    if( qobject_cast<QAction *>(sender()) ) { // Not sure // Hack TV
        QAction* temp = qobject_cast<QAction *>(sender());
        id = temp->data().value<int>();
        delete temp;
    } else {
        id = STDFillColorAct->data().value<int>();
    }

    if(id == ID_MAPBAR_ITEM_FILL)
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_FILL;
        activeMapView()->OnEditFill();
    }
    else if (id == ID_MAPBAR_ITEM_SEMIFILL)         // AV TV
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_SEMIFILL;
        activeMapView()->OnEditSemiFill();
    }
    else
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_AUGMENT_FILL;
        activeMapView()->OnEditAugmentFill(); // AV TV
    }
}

void MainWindow::LineButtonTriggered()
{
    int id = SelectLineAct->data().value<int>();
    if(id == ID_MAPBAR_ITEM_LINETOOL)
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_LINETOOL;
        activeMapView()->OnEditLineTool();
    }
    else
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_POLYGON;
        activeMapView()->OnEditPolygonTool();
    }
}

void MainWindow::isoButtonTriggered()
{
    int id = MakeIosAct->data().value<int>();
    if(id == ID_MAPBAR_ITEM_ISOVIST)
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_ISOVIST;
        activeMapView()->OnModeIsovist();
    }
    else
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_HALFISOVIST;
        activeMapView()->OnModeTargetedIsovist();
    }
}

void MainWindow::joinButtonTriggered()
{
    int id = JoinAct->data().value<int>();
    if(id == ID_MAPBAR_ITEM_JOIN)
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_JOIN;
        activeMapView()->OnModeJoin();
    }
    else
    {
        m_selected_mapbar_item = ID_MAPBAR_ITEM_UNJOIN;
        activeMapView()->OnModeUnjoin();
    }
}

void MainWindow::zoomModeTriggered()
{
    zoomInAct = qobject_cast<QAction *>(sender());
    if(zoomInAct->data() == ID_MAPBAR_ITEM_ZOOM_IN)
        zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
    else
        zoomToolButton->setIcon(QIcon(":/images/win/b-5-4.png"));

    zoomToolButton->setChecked(1);
    zoomButtonTriggered();
}

void MainWindow::FillModeTriggered()
{
    fillColorToolButton->setChecked(1);
    FillButtonTriggered();
}

void MainWindow::LineModeTriggered()
{
    SelectLineAct = qobject_cast<QAction *>(sender());
    if(SelectLineAct->data() == ID_MAPBAR_ITEM_LINETOOL)
        lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
    else
        lineToolButton->setIcon(QIcon(":/images/win/b-5-11.png"));
    lineToolButton->setChecked(1);
    LineButtonTriggered();
}

void MainWindow::isoModeTriggered()
{
    MakeIosAct = qobject_cast<QAction *>(sender());
    if(MakeIosAct->data() == ID_MAPBAR_ITEM_ISOVIST)
        newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
    else
        newisoToolButton->setIcon(QIcon(":/images/win/b-5-13.png"));

    newisoToolButton->setChecked(1);
    isoButtonTriggered();
}

void MainWindow::joinTriggered()
{
    JoinAct = qobject_cast<QAction *>(sender());
    if(JoinAct->data() == ID_MAPBAR_ITEM_JOIN)
        JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
    else
        JoinToolButton->setIcon(QIcon(":/images/win/b-5-17.png"));

    JoinToolButton->setChecked(1);
    joinButtonTriggered();
}

void MainWindow::OnFileProperties()
{
   QGraphDoc* gd = activeMapView()->getGraphDoc();
   gd->OnFileProperties();
}

// PlotView message
void MainWindow::OntoggleColor()
{
   QGraphDoc* gd = activeMapDoc();
   if(((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
       ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewColor();
}

void MainWindow::OntoggleOrg()
{
   QGraphDoc* gd = activeMapDoc();
   if(((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
     ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewOrigin();
}

void MainWindow::OnviewTrend()
{
   QGraphDoc* gd = activeMapDoc();
   if(((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
      ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewTrendLine();
}

void MainWindow::OnYX()
{
   QGraphDoc* gd = activeMapDoc();
   if(((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
      ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewEquation();
}

void MainWindow::OnRtwo()
{
   QGraphDoc* gd = activeMapDoc();
   if(((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
        ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewRsquared();
}

void MainWindow::OnToolsImportTraces()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsImportTraces();
}

void MainWindow::OnAddAgent()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnAddAgent();
}

void MainWindow::OnToolsAgentsPlay()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsPlay();
}

void MainWindow::OnToolsAgentsPause()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsPause();
}

void MainWindow::OnToolsAgentsStop()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsStop();
    updateActiveWindows();
}

void MainWindow::OnAgentTrails()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnAgentTrails();
}

void MainWindow::On3dRot()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dRot();
}

void MainWindow::On3dPan()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dPan();
}

void MainWindow::On3dZoom()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dZoom();
}

void MainWindow::OnPlayLoop()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnPlayLoop();
}

void MainWindow::On3dFilled()
{
    QGraphDoc* gd = activeMapDoc();
    if(((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
        ((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dFilled();
}

///////////////////////////////////////
void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
    g_info_curr = new QLabel;
    g_info_curr->setText("      ");
    statusBar()->addPermanentWidget(g_info_curr);
    g_size = new QLabel;
    g_size->setText("      ");
    statusBar()->addPermanentWidget(g_size);
    g_pos_curr = new QLabel;
    g_pos_curr->setText("      ");
    statusBar()->addPermanentWidget(g_pos_curr);
}

void MainWindow::readSettings()
{
    auto settings = mSettings.getTransaction();
    QPoint pos = settings->readSetting(SettingTag::position, QPoint(200, 200)).toPoint();
    QSize size = settings->readSetting(SettingTag::size, QSize(400, 400)).toSize();
    m_foreground = settings->readSetting(SettingTag::foregroundColour, qRgb(128,255,128)).toInt();
    m_background = settings->readSetting(SettingTag::backgroundColour, qRgb(0,0,0)).toInt();
    m_simpleVersion = settings->readSetting(SettingTag::simpleVersion, true).toBool();
    m_defaultMapWindowIsLegacy = settings->readSetting(SettingTag::legacyMapWindow, false).toBool();
    if (settings->readSetting(SettingTag::mwMaximised, true).toBool())
    {
         setWindowState(Qt::WindowMaximized);
    }
    else{
        move(pos);
        resize(size);
    }
}

void MainWindow::writeSettings()
{
    auto settings = mSettings.getTransaction();
    settings->writeSetting(SettingTag::position, pos());
    settings->writeSetting(SettingTag::size, size());
    settings->writeSetting(SettingTag::mwMaximised, windowState() == Qt::WindowMaximized);
}

void MainWindow::setCurrentFile(const QString &fileName)
{

    auto settings = mSettings.getTransaction();

    QStringList files = settings->readSetting(SettingTag::recentFileList).toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings->writeSetting(SettingTag::recentFileList, files);

    updateRecentFileActions(files);
}

void MainWindow::updateRecentFileActions(const QStringList &files)
{
    int numRecentFiles = qMin(files.size(), MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    separatorAct->setVisible(numRecentFiles > 0);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        QMdiSubWindow *existing = findMapView(action->data().toString());
        if (existing) {
            mdiArea->setActiveSubWindow(existing);
            return;
        }
        MapView *child = createMapView();
        QByteArray ba = action->data().toString().toUtf8(); // quick fix for weird chars (russian filename bug report)
        char *file = ba.data(); // quick fix for weird chars (russian filename bug report)
        if(child->getGraphDoc()->OnOpenDocument(file)) // quick fix for weird chars (russian filename bug report)
        {
            child->setCurrentFile(action->data().toString());
            child->postLoadFile();
            setCurrentFile(action->data().toString());
            statusBar()->showMessage(tr("File loaded"), 2000);
            child->show();
            OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
        }
        else child->close();
    }
}

void MainWindow::RedoPlotViewMenu(QGraphDoc* pDoc)
{
   if(!pDoc->m_view[QGraphDoc::VIEW_SCATTER]) return;
   in_FocusGraph = true;

   // this will be used to distinguish between viewing VGA and axial maps
   int view_class = pDoc->m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL | MetaGraph::VIEWDATA);
   int curr_j = 0;

   {
       auto lock = pDoc->m_meta_graph->getLockDeferred();
       if (lock.try_lock()) {
          m_view_map_entries.clear();
          if (view_class == MetaGraph::VIEWVGA) {
             PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();
             int displayed_ref = map.getDisplayedAttribute();

             const AttributeTable& table = map.getAttributeTable();
             m_view_map_entries.add(0, "Ref Number");
             for (int i = 0; i < table.getColumnCount(); i++) {
                m_view_map_entries.add(i+1, table.getColumnName(i));
                if (map.getDisplayedAttribute() == i) {
                   curr_j = i + 1;
                }
             }
          }
          else if (view_class == MetaGraph::VIEWAXIAL) {
             // using attribute tables is very, very simple...
             const ShapeGraph& map = pDoc->m_meta_graph->getDisplayedShapeGraph();
             const AttributeTable& table = map.getAttributeTable();
             m_view_map_entries.add(0, "Ref Number");
             curr_j = 0;
             for (int i = 0; i < table.getColumnCount(); i++) {
                m_view_map_entries.add(i+1, table.getColumnName(i));
                if (map.getDisplayedAttribute() == i) {
                   curr_j = i + 1;
                }
             }
          }
          else if (view_class == MetaGraph::VIEWDATA) {
             // using attribute tables is very, very simple...
             const ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();
             const AttributeTable& table = map.getAttributeTable();
             m_view_map_entries.add(0, "Ref Number");
             curr_j = 0;
             for (int i = 0; i < table.getColumnCount(); i++) {
                m_view_map_entries.add(i+1, table.getColumnName(i));
                if (map.getDisplayedAttribute() == i) {
                   curr_j = i + 1;
                }
             }
          }
       }
    }

   int t, cur_sel = 0;
   x_coord->clear();
   y_coord->clear();

   for (size_t i = 0; i < m_view_map_entries.size(); i++) {
      if (curr_j == m_view_map_entries.key(i)) cur_sel = (int) i;
      x_coord->addItem( QString(m_view_map_entries.value(i).c_str()) );
      y_coord->addItem( QString(m_view_map_entries.value(i).c_str()) );
   }

   t = ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->curr_y;
   if(t != -1) cur_sel = t;
   ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(1, cur_sel - 1, true);
   y_coord->setCurrentIndex(cur_sel);

   t = ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->curr_x;
   if(t != -1) cur_sel = t;
   ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(0, cur_sel - 1, true);
   x_coord->setCurrentIndex(cur_sel);

   in_FocusGraph = false;
}

void MainWindow::OnSelchangeViewSelector_X(const QString &string)
{
   if(in_FocusGraph) return;

   int i = x_coord->currentIndex();

   QGraphDoc* gd = activeMapDoc();
   ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(0, /*m_view_selection*/i - 1, true);
   ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->curr_x = i;

   // note: this is only attached to a scatter view, and changing the attribute only
   // affects the scatter view, so only send draw to the map:
   gd->SetRedrawFlag(QGraphDoc::VIEW_SCATTER, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN );
}

void MainWindow::OnSelchangeViewSelector_Y(const QString &string)
{
   if(in_FocusGraph) return;
   int i = y_coord->currentIndex();

   QGraphDoc* gd = activeMapDoc();
   ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(1, i - 1, true);
   ((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->curr_y = i;

   // note: this is only attached to a scatter view, and changing the attribute only
   // affects the scatter view, so only send draw to the map:
   gd->SetRedrawFlag(QGraphDoc::VIEW_SCATTER, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN );
}


void MainWindow::updateViewMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        RecentAct->setEnabled(0);
        showGridAct->setEnabled(0);
        attributeSummaryAct->setEnabled(0);
        return;
    }
    RecentAct->setEnabled(true);
    showGridAct->setEnabled(true);

    if(m_p->m_meta_graph->m_showgrid) showGridAct->setChecked(true);
    else showGridAct->setChecked(false);

    attributeSummaryAct->setEnabled(true);
    if (!m_p->m_communicator && m_p->m_meta_graph && !m_p->m_meta_graph->viewingNone())
        attributeSummaryAct->setEnabled(true);
    else attributeSummaryAct->setEnabled(0);
}

void MainWindow::updateVisibilitySubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        SetGridAct->setEnabled(0);
        makeVisibilityGraphAct->setEnabled(0);
        importVGALinksAct->setEnabled(0);
        makeIsovistPathAct->setEnabled(0);
        runVisibilityGraphAnalysisAct->setEnabled(0);
        convertDataMapLinesAct->setEnabled(0);
        return;
    }
    if (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA || m_p->m_meta_graph->viewingUnprocessedPoints())
        SetGridAct->setEnabled(true);
    else SetGridAct->setEnabled(0);

    if(m_p->m_meta_graph->viewingUnprocessedPoints())
        makeVisibilityGraphAct->setEnabled(true);
    else makeVisibilityGraphAct->setEnabled(0);

    int state = m_p->m_meta_graph->getState();
    if (state & MetaGraph::LINEDATA)
        makeIsovistPathAct->setEnabled(true);
    else makeIsovistPathAct->setEnabled(0);

    if (m_p->m_meta_graph->viewingProcessedPoints()) {
        importVGALinksAct->setEnabled(true);
        runVisibilityGraphAnalysisAct->setEnabled(true);
    }
    else
    {
        importVGALinksAct->setEnabled(0);
        runVisibilityGraphAnalysisAct->setEnabled(0);
    }

    if ( !m_p->m_communicator &&
         m_p->m_meta_graph->viewingProcessedShapes() &&
        (m_p->m_meta_graph->getState() & MetaGraph::POINTMAPS) &&
         m_p->m_meta_graph->getDisplayedPointMap().isProcessed())
        convertDataMapLinesAct->setEnabled(true);
    else convertDataMapLinesAct->setEnabled(0);
}

void MainWindow::updateStepDepthSubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        visibilityStepAct->setEnabled(0);
        metricStepAct->setEnabled(0);
        angularStepAct->setEnabled(0);
        return;
    }
    if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
        visibilityStepAct->setEnabled(true);
    else visibilityStepAct->setEnabled(0);

    if ((m_p->m_meta_graph->viewingProcessedPoints() || (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())) && m_p->m_meta_graph->isSelected())
        metricStepAct->setEnabled(true);
    else metricStepAct->setEnabled(0);

    if (m_p->m_meta_graph->viewingProcessedPoints() && m_p->m_meta_graph->isSelected())
        angularStepAct->setEnabled(true);
    else angularStepAct->setEnabled(0);
}

void MainWindow::updateAgentToolsSubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        runAgentAnalysisAct->setEnabled(0);
        loadAgentProgramAct->setEnabled(0);
        return;
    }
    if (m_p->m_meta_graph && m_p->m_meta_graph->viewingProcessedPoints() && !m_p->m_communicator)
        runAgentAnalysisAct->setEnabled(true);
    else runAgentAnalysisAct->setEnabled(0);
    if(current_view_type == QGraphDoc::VIEW_3D) loadAgentProgramAct->setEnabled(true);
    else loadAgentProgramAct->setEnabled(0);
}

void MainWindow::updateSegmentSubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        runAngularSegmentAnalysisAct->setEnabled(0);
        runTopologicalOrMetricAnalysisAct->setEnabled(0);
        return;
    }
    if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
        runAngularSegmentAnalysisAct->setEnabled(true);
    else runAngularSegmentAnalysisAct->setEnabled(0);

    if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
        runTopologicalOrMetricAnalysisAct->setEnabled(true);
    else runTopologicalOrMetricAnalysisAct->setEnabled(0);
}

void MainWindow::updateSegmentStepDepthSubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        segmentAngularStepAct->setEnabled(0);
        topologicalStepAct->setEnabled(0);
        segmentMetricStepAct->setEnabled(0);
        return;
    }
    if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
        segmentAngularStepAct->setEnabled(true);
    else segmentAngularStepAct->setEnabled(0);

    if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap() && m_p->m_meta_graph->isSelected())
        topologicalStepAct->setEnabled(true);
    else topologicalStepAct->setEnabled(0);

    if ((m_p->m_meta_graph->viewingProcessedPoints() || (m_p->m_meta_graph->viewingProcessedLines() &&
        m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())) && m_p->m_meta_graph->isSelected())
        segmentMetricStepAct->setEnabled(true);
    else segmentMetricStepAct->setEnabled(0);
}

void MainWindow::updateAxialSubMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        runGraphAnaysisAct->setEnabled(0);
        stepDepthAct->setEnabled(0);
        reduceToFewestLineMapAct->setEnabled(0);
        convertDataMapPointsAct->setEnabled(0);
        loadUnlinksFromFileAct->setEnabled(0);
        return;
    }
    int state = m_p->m_meta_graph->getState();
    // non-segment maps only
    if (state & MetaGraph::SHAPEGRAPHS && !m_p->m_communicator &&
       !m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
        runGraphAnaysisAct->setEnabled(true);
    else runGraphAnaysisAct->setEnabled(0);

    if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
        stepDepthAct->setEnabled(true);
    else stepDepthAct->setEnabled(0);

    state = m_p->m_meta_graph->getState();
    if (state & MetaGraph::SHAPEGRAPHS && !m_p->m_communicator &&
        m_p->m_meta_graph->getDisplayedShapeGraph().isAllLineMap())
        reduceToFewestLineMapAct->setEnabled(true);
    else reduceToFewestLineMapAct->setEnabled(0);

    if ( !m_p->m_communicator && m_p->m_meta_graph &&
         m_p->m_meta_graph->viewingProcessedLines() &&
         m_p->m_meta_graph->getDisplayedShapeGraph().getMapType() == ShapeMap::AXIALMAP)
        convertDataMapPointsAct->setEnabled(true);
    else convertDataMapPointsAct->setEnabled(0);

    if ( !m_p->m_communicator && m_p->m_meta_graph &&
         m_p->m_meta_graph->viewingProcessedLines() &&
         !m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
        loadUnlinksFromFileAct->setEnabled(true);
    else loadUnlinksFromFileAct->setEnabled(0);
}

void MainWindow::updateAttributesMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        addColumAct->setEnabled(0);
        updateColumAct->setEnabled(0);
        renameColumnAct->setEnabled(0);
        removeColumAct->setEnabled(0);
        pushValueAct->setEnabled(0);
        columnPropertiesAct->setEnabled(0);
        return;
    }
    if (!m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
    {
        addColumAct->setEnabled(true);
        columnPropertiesAct->setEnabled(true);
        int col = m_p->m_meta_graph->getDisplayedAttribute();
        if (col == -1 || col == -2 || m_p->m_meta_graph->isAttributeLocked(col))
        {
            renameColumnAct->setEnabled(0);
            updateColumAct->setEnabled(0);
            removeColumAct->setEnabled(0);
            pushValueAct->setEnabled(0);
        }
        else {
            renameColumnAct->setEnabled(true);
            updateColumAct->setEnabled(true);
            removeColumAct->setEnabled(true);
            pushValueAct->setEnabled(true);
        }
    }
    else
    {
        addColumAct->setEnabled(0);
        updateColumAct->setEnabled(0);
        renameColumnAct->setEnabled(0);
        removeColumAct->setEnabled(0);
        pushValueAct->setEnabled(0);
        columnPropertiesAct->setEnabled(0);
    }
}

void MainWindow::updateMapMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        mapNewAct->setEnabled(0);
        deleteAct->setEnabled(0);
        convertActiveMapAct->setEnabled(0);
        convertDrawingMapAct->setEnabled(0);
        convertMapShapesAct->setEnabled(0);
        importAct->setEnabled(0);
        exportAct->setEnabled(0);
        exportLinksAct->setEnabled(0);
        exportAxialConnectionsDotAct->setEnabled(0);
        exportAxialConnectionsPairAct->setEnabled(0);
        exportSegmentConnectionsPairAct->setEnabled(0);
        return;
    }
    mapNewAct->setEnabled(true);
    importAct->setEnabled(true);
    if (!m_p->m_meta_graph->viewingNone() && !m_p->m_communicator)
        deleteAct->setEnabled(true);
    else deleteAct->setEnabled(0);

    if (!m_p->m_communicator && (m_p->m_meta_graph->viewingProcessedLines() || m_p->m_meta_graph->viewingProcessedShapes()))
        convertActiveMapAct->setEnabled(true);
    else convertActiveMapAct->setEnabled(0);

    if (!m_p->m_communicator && (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA) == MetaGraph::LINEDATA)
        convertDrawingMapAct->setEnabled(true);
    else convertDrawingMapAct->setEnabled(0);

    if (m_p->m_meta_graph && m_p->m_meta_graph->viewingShapes())
        convertMapShapesAct->setEnabled(true);
    else convertMapShapesAct->setEnabled(0);

    if (!m_p->m_meta_graph->viewingNone() && !m_p->m_communicator)
    {
        exportAct->setEnabled(true);
        exportLinksAct->setEnabled(true);
        exportAxialConnectionsDotAct->setEnabled(true);
        exportAxialConnectionsPairAct->setEnabled(true);
        exportSegmentConnectionsPairAct->setEnabled(true);
    }
    else
    {
        exportAct->setEnabled(0);
        exportLinksAct->setEnabled(0);
        exportAxialConnectionsDotAct->setEnabled(0);
        exportAxialConnectionsPairAct->setEnabled(0);
        exportSegmentConnectionsPairAct->setEnabled(0);
    }
}


void MainWindow::updateEditMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    if(!m_p)
    {
        copyDataAct->setEnabled(0);
        undoAct->setEnabled(0);
        copyScreenAct->setEnabled(0);
        exportScreenAct->setEnabled(0);
        clearAct->setEnabled(0);
        selectByQueryAct->setEnabled(0);
        //zoomToSelectionAct->setEnabled(0);
        selectionToLayerAct->setEnabled(0);
        return;
    }
    copyScreenAct->setEnabled(true);
    exportScreenAct->setEnabled(true);
    if(m_p->m_meta_graph->isEditable()) clearAct->setEnabled(true);
    else clearAct->setEnabled(0);

    if (m_p->m_meta_graph->canUndo()) undoAct->setEnabled(true);
    else undoAct->setEnabled(0);

    if (m_p->m_meta_graph && !m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
        selectByQueryAct->setEnabled(true);
    else selectByQueryAct->setEnabled(0);

    if(m_p->m_meta_graph->isSelected())
    {
        //zoomToSelectionAct->setEnabled(true);
        selectionToLayerAct->setEnabled(true);
    }
    else
    {
        //zoomToSelectionAct->setEnabled(0);
        selectionToLayerAct->setEnabled(0);
    }
}

void MainWindow::updateFileMenu()
{
    if(mdiArea->activeSubWindow())
    {
        closeAct->setEnabled( true );
        saveAct->setEnabled( true );
        saveAsAct->setEnabled( true );
        propertiesAct->setEnabled( true );
        if(current_view_type == QGraphDoc::VIEW_3D)
        {
            printAct->setEnabled( 0 );
            printPreviewAct->setEnabled( 0 );
        }
        else
        {
            printAct->setEnabled( true );
            printPreviewAct->setEnabled( true );
        }
    }
    else
    {
        closeAct->setEnabled( 0 );
        saveAct->setEnabled( 0 );
        saveAsAct->setEnabled( 0 );
        propertiesAct->setEnabled( 0 );
        printAct->setEnabled( 0 );
        printPreviewAct->setEnabled( 0 );
    }
}

void MainWindow::updateWindowMenu()
{
    QGraphDoc* m_p = activeMapDoc();
    windowMenu->clear();
    windowMenu->addAction(mapAct);

    if(m_p && m_p->m_view[QGraphDoc::VIEW_MAP]) mapAct->setChecked(true);
    else mapAct->setChecked(false);

    windowMenu->addAction(scatterPlotAct);
    if(m_p && m_p->m_view[QGraphDoc::VIEW_SCATTER]) scatterPlotAct->setChecked(true);
    else scatterPlotAct->setChecked(false);

    windowMenu->addAction(tableAct);
    if(m_p && m_p->m_view[QGraphDoc::VIEW_TABLE]) tableAct->setChecked(true);
    else tableAct->setChecked(false);

    windowMenu->addAction(thirdDViewAct);
    if(m_p && m_p->m_view[QGraphDoc::VIEW_3D]) thirdDViewAct->setChecked(true);
    else thirdDViewAct->setChecked(false);

    windowMenu->addAction(glViewAct);
    if(m_p && m_p->m_view[QGraphDoc::VIEW_MAP_GL]) glViewAct->setChecked(true);
    else glViewAct->setChecked(false);

    windowMenu->addSeparator();
    windowMenu->addAction(colourRangeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(cascadeAct);
    windowMenu->addAction(tileAct);
    windowMenu->addAction(arrangeIconsAct);
    windowMenu->addAction(separatorAct);

    if(!m_p)
    {
        mapAct->setEnabled(0);
        scatterPlotAct->setEnabled(0);
        tableAct->setEnabled(0);
        thirdDViewAct->setEnabled(0);
        glViewAct->setEnabled(0);
    }
    else
    {
        thirdDViewAct->setEnabled(true);
        mapAct->setEnabled(true);
        glViewAct->setEnabled(true);
        if (m_p->m_meta_graph && m_p->m_meta_graph->viewingProcessed())
        {
            tableAct->setEnabled(true);
            scatterPlotAct->setEnabled(true);
        }
        else {
            tableAct->setEnabled(0);
            scatterPlotAct->setEnabled(0);
        }
    }

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    int find_count = 1;
    for (int i = 0; i < windows.size(); ++i) {
        MapView *child = qobject_cast<MapView*>(windows.at(i)->widget());
        if(!child) continue;

        QString text;
        text = tr("&%1 %2").arg(find_count++).arg(child->windowTitle());
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMapView());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i)->widget());
    }
}

void MainWindow::UpdateStatus(QString s1, QString s2, QString s3)
{
    g_info_curr->setText(s1);
    g_info_curr->update();
    g_size->setText(s2);
    g_size->update();
    g_pos_curr->setText(s3);
    g_pos_curr->update();
}

void MainWindow::updateToolbar()
{
    importAct->setEnabled(0);
    saveAct->setEnabled(0);
    addColumAct->setEnabled(0);
    updateColumAct->setEnabled(0);
    removeColumAct->setEnabled(0);
    pushValueAct->setEnabled(0);
    invertColorAct->setEnabled(0);
    SelectButton->setEnabled(0);
    DragButton->setEnabled(0);
    zoomToolButton->setEnabled(0);
    zoomToAct->setEnabled(0);
    RecentAct->setEnabled(0);
    SetGridAct->setEnabled(0);
    fillColorToolButton->setEnabled(0);
    SelectPenButton->setEnabled(0);
    lineToolButton->setEnabled(0);
    newisoToolButton->setEnabled(0);
    AxialMapButton->setEnabled(0);
    StepDepthButton->setEnabled(0);
    JoinToolButton->setEnabled(0);
    attr_del_button->setEnabled(0);
    attr_add_button->setEnabled(0);

    QGraphDoc* m_p = activeMapDoc();
    MapView* tmpView = activeMapView();
    if(m_p)
    {
        importAct->setEnabled(true);
        saveAct->setEnabled(true);
        addColumAct->setEnabled(true);
        SelectButton->setEnabled(true);
        DragButton->setEnabled(true);
        RecentAct->setEnabled(true);

        if(m_p->m_meta_graph->isSelected())
            zoomToAct->setEnabled(true);

        if (m_p->m_meta_graph->isShown())   // zoom bug VGA // TV
            zoomToolButton->setEnabled(true);
        if (m_p->m_meta_graph->viewingProcessed())
            zoomToolButton->setEnabled(true);

        if (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA || m_p->m_meta_graph->viewingUnprocessedPoints())
            SetGridAct->setEnabled(true);

        if (m_p->m_meta_graph->viewingUnprocessedPoints())
        {
            fillColorToolButton->setEnabled(true);
            SelectPenButton->setEnabled(true);
        }
        else
        {
            if (tmpView)
            {
                if (m_selected_mapbar_item == ID_MAPBAR_ITEM_FILL
                        || m_selected_mapbar_item == ID_MAPBAR_ITEM_SEMIFILL
                        || m_selected_mapbar_item == ID_MAPBAR_ITEM_PENCIL)
                {
                    tmpView->OnEditSelect();
                    SelectButton->setChecked(true);
                }
            }
        }

        int type = m_p->m_meta_graph->getDisplayedMapType();
        if ((type == ShapeMap::DATAMAP && m_p->m_meta_graph->getDisplayedDataMap().isEditable()) ||
            ((type == ShapeMap::AXIALMAP || type == ShapeMap::CONVEXMAP || type == ShapeMap::PESHMAP) &&
            m_p->m_meta_graph->getDisplayedShapeGraph().isEditable()))
            lineToolButton->setEnabled(true);
        else
        {
            if (tmpView)
            {
                if (m_selected_mapbar_item == ID_MAPBAR_ITEM_LINETOOL
                        || m_selected_mapbar_item == ID_MAPBAR_ITEM_POLYGON)
                {
                    tmpView->OnEditSelect();
                    SelectButton->setChecked(true);
                }
            }
        }

        type = m_p->m_meta_graph->getState();
        if (!(~type & MetaGraph::LINEDATA))
            newisoToolButton->setEnabled(true);
        else
        {
            if (tmpView)
            {
                if (m_selected_mapbar_item == ID_MAPBAR_ITEM_ISOVIST || m_selected_mapbar_item == ID_MAPBAR_ITEM_HALFISOVIST)
                {
                    tmpView->OnEditSelect();
                    SelectButton->setChecked(true);
                }
            }
        }

        if (( ( (m_p->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) &&
               (m_p->m_meta_graph->getDisplayedPointMap().isProcessed())) ||
             ( (m_p->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) &&
               (m_p->m_meta_graph->getState() & MetaGraph::SHAPEGRAPHS)) &&
               (!m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap()) ) )
            JoinToolButton->setEnabled(true);
        else
        {
            if (tmpView)
            {
                if (m_selected_mapbar_item == ID_MAPBAR_ITEM_JOIN || m_selected_mapbar_item == ID_MAPBAR_ITEM_UNJOIN)
                {
                    tmpView->OnEditSelect();
                    SelectButton->setChecked(true);
                }
            }
        }

        type = m_p->m_meta_graph->getState();
        if (!(~type & MetaGraph::LINEDATA)) AxialMapButton->setEnabled(true);
        else
        {
            if (tmpView)
            {
                if (m_selected_mapbar_item == ID_MAPBAR_ITEM_AL2)
                {
                    tmpView->OnEditSelect();
                    SelectButton->setChecked(true);
                }
            }
        }

        if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
            StepDepthButton->setEnabled(true);

        if (!m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
        {
            int col = m_p->m_meta_graph->getDisplayedAttribute();
            if (!(col == -1 || col == -2 || m_p->m_meta_graph->isAttributeLocked(col))) {
                renameColumnAct->setEnabled(true);
                updateColumAct->setEnabled(true);
                removeColumAct->setEnabled(true);
                pushValueAct->setEnabled(true);
                invertColorAct->setEnabled(true);
                attr_del_button->setEnabled(true);
                attr_add_button->setEnabled(true);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    newAct->setStatusTip(tr("Create a new graph workspace\nNew Workspace"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(OnFileNew()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Change the printing options\nPage Setup"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(OnFileOpen()));

    closeAct = new QAction(tr("&Close"), this);
    closeAct->setStatusTip(tr("Close the active graph workspace\nClose Workspace"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(OnFileClose()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the active graph workspace\nSave workspace"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(OnFileSave()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the active graph workspace with a new name\nSave Workspace As"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(OnFileSaveAs()));

    propertiesAct = new QAction(tr("Properties..."), this);
    propertiesAct->setStatusTip(tr("Edit graph workspace properties\nWorkspace Properties"));
    connect(propertiesAct, SIGNAL(triggered()), this, SLOT(OnFileProperties()));

    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setStatusTip(tr("Print the active graph workspace\nPrint Workspace"));
    connect(printAct, SIGNAL(triggered()), this, SLOT(OnFilePrint()));

    printPreviewAct = new QAction(tr("Print Pre&view"), this);
    printPreviewAct->setStatusTip(tr("Display full pages\nPrint Preview"));
    connect(printPreviewAct, SIGNAL(triggered()), this, SLOT(OnFilePrintPreview()));

    printSetupAct = new QAction(tr("P&rint Setup..."), this);
    printSetupAct->setStatusTip(tr("Change the printer and printing options\nPrint Setup"));
    connect(printSetupAct, SIGNAL(triggered()), this, SLOT(OnFilePrintSetup()));

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setStatusTip(tr("Quit the application; prompts to save documents\nExit"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(OnFileExit()));

    //Edit Menu Actions
    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcut(tr("Ctrl+Z"));
    undoAct->setStatusTip(tr("Undo the last action\nUndo"));
    connect(undoAct, SIGNAL(triggered()), this, SLOT(OnEditUndo()));

    copyDataAct = new QAction(tr("Copy &Data"), this);
    connect(copyDataAct, SIGNAL(triggered()), this, SLOT(OnEditCopyData()));

    copyScreenAct = new QAction(tr("Copy &Screen"), this);
    copyScreenAct->setShortcut(tr("Ctrl+C"));
    copyScreenAct->setStatusTip(tr("Copy the screen contents to clipboard\nCopy Screen"));
    connect(copyScreenAct, SIGNAL(triggered()), this, SLOT(OnEditCopy()));

    exportScreenAct = new QAction(tr("&Export Screen..."), this);
    exportScreenAct->setStatusTip(tr("Export the screen as an Encapsulated Postscript file\nExport Screen"));
    connect(exportScreenAct, SIGNAL(triggered()), this, SLOT(OnEditSave()));

    clearAct = new QAction(tr("&Clear"), this);
    clearAct->setShortcut(tr("Del"));
    clearAct->setShortcutContext(Qt::ApplicationShortcut);
    clearAct->setStatusTip(tr("Erase the selection\nErase"));
    connect(clearAct, SIGNAL(triggered()), this, SLOT(OnEditClear()));

    selectByQueryAct = new QAction(tr("Select by &Query"), this);
    selectByQueryAct->setShortcut(tr("Ctrl+Q"));
    connect(selectByQueryAct, SIGNAL(triggered()), this, SLOT(OnEditQuery()));

    //zoomToSelectionAct = new QAction(tr("&Zoom to Selection"), this);
    //zoomToSelectionAct->setStatusTip(tr("Zoom in around current selection\nZoom to Selection"));
    //connect(zoomToSelectionAct, SIGNAL(triggered()), this, SLOT(OnViewZoomsel()));

    selectionToLayerAct = new QAction(tr("Selection to &Layer..."), this);
    selectionToLayerAct->setStatusTip(tr("Convert the current selection to a new map layer"));
    connect(selectionToLayerAct, SIGNAL(triggered()), this, SLOT(OnEditSelectToLayer()));

    //Map Menu Actions
    mapNewAct = new QAction(tr("&New..."), this);
    mapNewAct->setStatusTip(tr("Create a new map"));
    connect(mapNewAct, SIGNAL(triggered()), this, SLOT(OnLayerNew()));

    deleteAct = new QAction(tr("&Delete..."), this);
    deleteAct->setStatusTip(tr("Delete the active map"));
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(OnLayerDelete()));

    convertActiveMapAct = new QAction(tr("&Convert Active Map..."), this);
    convertActiveMapAct->setStatusTip(tr("Create a new map from the active map"));
    connect(convertActiveMapAct, SIGNAL(triggered()), this, SLOT(OnLayerConvert()));

    convertDrawingMapAct = new QAction(tr("Convert Drawing &Map..."), this);
    convertDrawingMapAct->setStatusTip(tr("Create a new map from the displayed drawing maps"));
    connect(convertDrawingMapAct, SIGNAL(triggered()), this, SLOT(OnLayerConvertDrawing()));

    convertMapShapesAct = new QAction(tr("Convert Map &Shapes..."), this);
    convertMapShapesAct->setStatusTip(tr("Convert shapes to other shapes within the current map"));
    connect(convertMapShapesAct, SIGNAL(triggered()), this, SLOT(OnConvertMapShapes()));

    importAct = new QAction(QIcon(":/images/down.png"), tr("&Import..."), this);
    importAct->setShortcut(tr("Ctrl+I"));
    importAct->setStatusTip(tr("Import a DXF or points file\nImport Map"));
    connect(importAct, SIGNAL(triggered()), this, SLOT(OnFileImport()));

    exportAct = new QAction(tr("&Export map..."), this);
    exportAct->setShortcut(tr("Ctrl+E"));
    exportAct->setStatusTip(tr("Export the active map"));
    connect(exportAct, SIGNAL(triggered()), this, SLOT(OnFileExport()));

    exportLinksAct = new QAction(tr("&Export links..."), this);
    exportLinksAct->setStatusTip(tr("Export the links of the active map"));
    connect(exportLinksAct, SIGNAL(triggered()), this, SLOT(OnFileExportLinks()));

    exportAxialConnectionsPairAct = new QAction(tr("&Axial Connections as CSV..."), this);
    exportAxialConnectionsPairAct->setStatusTip(tr("Export a list of line-line intersections"));
    connect(exportAxialConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnAxialConnectionsExportAsPairCSV()));

    exportAxialConnectionsDotAct = new QAction(tr("&Axial Connections as Dot..."), this);
    exportAxialConnectionsDotAct->setStatusTip(tr("Export a list of line-line intersections"));
    connect(exportAxialConnectionsDotAct, SIGNAL(triggered()), this, SLOT(OnAxialConnectionsExportAsDot()));

    exportSegmentConnectionsPairAct = new QAction(tr("&Segment Connections as CSV..."), this);
    exportSegmentConnectionsPairAct->setStatusTip(tr("Export a list of line-line intersections and weights"));
    connect(exportSegmentConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnSegmentConnectionsExportAsPairCSV()));

    exportPointmapConnectionsPairAct = new QAction(tr("Visibility Graph Connections as CSV..."), this);
    exportPointmapConnectionsPairAct->setStatusTip(tr("Export connections between cells in a visibility graph as an adjacency list"));
    connect(exportPointmapConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnPointmapExportConnectionsAsCSV()));

    //Attributes Menu Actions
    renameColumnAct = new QAction(tr("&Rename Column..."), this);
    renameColumnAct->setStatusTip(tr("Rename the currently displayed attribute"));
    connect(renameColumnAct, SIGNAL(triggered()), this, SLOT(OnRenameColumn()));

    columnPropertiesAct = new QAction(tr("Column &Properties..."), this);
    columnPropertiesAct->setStatusTip(tr("Summary statistics for the active attribute"));
    connect(columnPropertiesAct, SIGNAL(triggered()), this, SLOT(OnColumnProperties()));

    //Tools Menu Actions
    makeVisibilityGraphAct = new QAction(tr("Make &Visibility Graph..."), this);
    connect(makeVisibilityGraphAct, SIGNAL(triggered()), this, SLOT(OnToolsMakeGraph()));

    importVGALinksAct = new QAction(tr("Import VGA links from file..."), this);
    connect(importVGALinksAct, SIGNAL(triggered()), this, SLOT(OnToolsImportVGALinks()));

    makeIsovistPathAct = new QAction(tr("Make &Isovist Path..."), this);
    connect(makeIsovistPathAct, SIGNAL(triggered()), this, SLOT(OnToolsIsovistpath()));

    runVisibilityGraphAnalysisAct = new QAction(tr("&Run Visibility Graph Analysis..."), this);
    connect(runVisibilityGraphAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRun()));

    visibilityStepAct = new QAction(tr("&Visibility Step"), this);
    visibilityStepAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
    connect(visibilityStepAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

    metricStepAct = new QAction(tr("&Metric Step"), this);
    metricStepAct->setStatusTip(tr("Distance from current selection\nMetric Depth"));
    connect(metricStepAct, SIGNAL(triggered()), this, SLOT(OnToolsMPD()));

    angularStepAct = new QAction(tr("&Angular Step"), this);
    angularStepAct->setStatusTip(tr("Angular distance from current selection\nAngular Depth"));
    connect(angularStepAct, SIGNAL(triggered()), this, SLOT(OnToolsAPD()));

    convertDataMapLinesAct = new QAction(tr("Convert Data Map Lines to Merge Points"), this);
    convertDataMapLinesAct->setStatusTip(tr("Convert displayed data map lines to merge points for current visibility graph"));
    connect(convertDataMapLinesAct, SIGNAL(triggered()), this, SLOT(OnToolsPointConvShapeMap()));

    runAgentAnalysisAct = new QAction(tr("&Run Agent Analysis"), this);
    connect(runAgentAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentRun()));

    loadAgentProgramAct = new QAction(tr("&Load Agent Program"), this);
    connect(loadAgentProgramAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentLoadProgram()));

    runGraphAnaysisAct = new QAction(tr("&Run Graph Analysis..."), this);
    runGraphAnaysisAct->setStatusTip(tr("Analyse currently displayed axial line map\nAxial analysis"));
    connect(runGraphAnaysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRunAxa()));

    stepDepthAct = new QAction(tr("Step &Depth"), this);
    stepDepthAct->setShortcut(tr("Ctrl+D"));
    stepDepthAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
    connect(stepDepthAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

    reduceToFewestLineMapAct = new QAction(tr("Reduce to &Fewest Line Map..."), this);
    connect(reduceToFewestLineMapAct, SIGNAL(triggered()), this, SLOT(OnToolsMakeFewestLineMap()));

    convertDataMapPointsAct = new QAction(tr("Convert Data Map Points to Unlinks"), this);
    convertDataMapPointsAct->setStatusTip(tr("Convert displayed data map points to unlinks for current axial map"));
    connect(convertDataMapPointsAct, SIGNAL(triggered()), this, SLOT(OnToolsAxialConvShapeMap()));

    loadUnlinksFromFileAct = new QAction(tr("Load Unlinks from File..."), this);
    connect(loadUnlinksFromFileAct, SIGNAL(triggered()), this, SLOT(OnToolsLineLoadUnlinks()));

    runAngularSegmentAnalysisAct = new QAction(tr("&Run Angular Segment Analysis..."), this);
    connect(runAngularSegmentAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRunSeg()));

    runTopologicalOrMetricAnalysisAct = new QAction(tr("Run &Topological or Metric Analysis..."), this);
    connect(runTopologicalOrMetricAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsTopomet()));

    segmentAngularStepAct = new QAction(tr("&Angular Step"), this);
    segmentAngularStepAct->setShortcut(tr("Ctrl+D"));
    segmentAngularStepAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
    connect(segmentAngularStepAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

    topologicalStepAct = new QAction(tr("&Topological Step"), this);
    connect(topologicalStepAct, SIGNAL(triggered()), this, SLOT(OnToolsTPD()));

    segmentMetricStepAct = new QAction(tr("&Metric Step"), this);
    connect(segmentMetricStepAct, SIGNAL(triggered()), this, SLOT(OnToolsMPD()));

    optionsAct = new QAction(tr("Options..."), this);
    connect(optionsAct, SIGNAL(triggered()), this, SLOT(OnToolsOptions()));

    //View Menu Actions
    showGridAct = new QAction(tr("Show &Grid"), this);
    showGridAct->setStatusTip(tr("Display grid"));
    showGridAct->setCheckable(true);
    connect(showGridAct, SIGNAL(triggered()), this, SLOT(OnViewShowGrid()));

    attributeSummaryAct = new QAction(tr("&Attribute Summary..."), this);
    attributeSummaryAct->setStatusTip(tr("Show summarised attribute information for selected points"));
    connect(attributeSummaryAct, SIGNAL(triggered()), this, SLOT(OnViewSummary()));

    //Window Menu Actions
    mapAct = new QAction(tr("&Map"), this);
    mapAct->setCheckable(true);
    connect(mapAct, SIGNAL(triggered()), this, SLOT(OnWindowMap()));

    scatterPlotAct = new QAction(tr("&Scatter Plot"), this);
    scatterPlotAct->setCheckable(true);
    connect(scatterPlotAct, SIGNAL(triggered()), this, SLOT(OnViewScatterplot()));

    tableAct = new QAction(tr("&Table"), this);
    tableAct->setCheckable(true);
    connect(tableAct, SIGNAL(triggered()), this, SLOT(OnViewTable()));

    thirdDViewAct = new QAction(tr("&3D View"), this);
    thirdDViewAct->setCheckable(true);
    connect(thirdDViewAct, SIGNAL(triggered()), this, SLOT(OnWindow3dView()));

    glViewAct = new QAction(tr("Map (Open&GL)"), this);
    glViewAct->setCheckable(true);
    connect(glViewAct, SIGNAL(triggered()), this, SLOT(OnWindowGLView()));

    colourRangeAct = new QAction(tr("&Colour Range"), this);
    connect(colourRangeAct, SIGNAL(triggered()), this, SLOT(OnViewColourRange()));

    cascadeAct = new QAction(tr("C&ascade"), this);
    cascadeAct->setStatusTip(tr("Arrange windows so they overlap\nCascade Windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    tileAct = new QAction(tr("T&ile"), this);
    tileAct->setStatusTip(tr("Arrange windows as non-overlapping tiles\nTile Windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    arrangeIconsAct = new QAction(tr("A&rrange Icons"), this);
    arrangeIconsAct->setStatusTip(tr("Arrange icons at the bottom of the window\nArrange Icons"));
//	connect(arrangeIconsAct, SIGNAL(triggered()), mdiArea, SLOT(arrangeSubWindows()));

    //Help Menu Actions
    onlineBugsAct = new QAction(tr("Submit Problems/Ideas"), this);
    onlineBugsAct->setStatusTip(tr("View or Submit Problems and Ideas online"));
    connect(onlineBugsAct, SIGNAL(triggered()), this, SLOT(OnHelpBugs()));

    onlineHandbookAct = new QAction(tr("Get the PDF &Handbook"), this);
    onlineHandbookAct->setStatusTip(tr("View the Original Depthmap Researchers' Handbook"));
    connect(onlineHandbookAct, SIGNAL(triggered()), this, SLOT(OnHelpManual()));

    onlineTutorialsAct = new QAction(tr("Online &Tutorials"), this);
    onlineTutorialsAct->setStatusTip(tr("View the Original Depthmap tutorials online"));
    connect(onlineTutorialsAct, SIGNAL(triggered()), this, SLOT(OnHelpTutorials()));

    onlineScriptingManualAct = new QAction(tr("Online &Scripting Manual"), this);
    onlineScriptingManualAct->setStatusTip(tr("See the Original SalaScript web page"));
    connect(onlineScriptingManualAct, SIGNAL(triggered()), this, SLOT(OnHelpSalaManual()));

    aboutDepthMapAct = new QAction(tr("About &depthmapX..."), this);
    aboutDepthMapAct->setStatusTip(tr("Display program information, version number and copyright\nAbout"));
    connect(aboutDepthMapAct, SIGNAL(triggered()), this, SLOT(OnAppAbout()));
// ToolBar actions

    invertColorAct = new QAction(QIcon(":/images/win/b-5-18.png"), tr("Invert Colour Range"), this);
    invertColorAct->setStatusTip(tr("Invert the colour range\nInvert Colour Range"));
    connect(invertColorAct, SIGNAL(triggered()), this, SLOT(OninvertColor()));

    addColumAct = new QAction(QIcon(":/images/win/b-5-19.png"), tr("&Add Column"), this);
    addColumAct->setStatusTip(tr("Add column to the active map\nAdd Column"));
    connect(addColumAct, SIGNAL(triggered()), this, SLOT(OnAddColumn()));

    updateColumAct = new QAction(QIcon(":/images/win/b-5-20.png"), tr("&Update Column"), this);
    updateColumAct->setStatusTip(tr("Replace column contents using a SalaScript command\nUpdate Column"));
    connect(updateColumAct, SIGNAL(triggered()), this, SLOT(OnUpdateColumn()));

    removeColumAct = new QAction(QIcon(":/images/win/b-5-21.png"), tr("&Remove Column"), this);
    removeColumAct->setStatusTip(tr("Remove column from the active map\nRemove column"));
    connect(removeColumAct, SIGNAL(triggered()), this, SLOT(OnRemoveColumn()));

    pushValueAct = new QAction(QIcon(":/images/win/b-5-22.png"), tr("&Push Values"), this);
    pushValueAct->setStatusTip(tr("Push values from active map to another map\npushValue"));
    connect(pushValueAct, SIGNAL(triggered()), this, SLOT(OnPushToLayer()));

    zoomToAct = new QAction(QIcon(":/images/win/b-5-5.png"), tr("Zoom to Selection"), this);
    zoomToAct->setStatusTip(tr("Zoom in on current selected items\nZoom to Selection"));
    connect(zoomToAct, SIGNAL(triggered()), this, SLOT(OnzoomTo()));

    RecentAct = new QAction(QIcon(":/images/win/b-5-6.png"), tr("&Recentre View"), this);
    RecentAct->setStatusTip(tr("Fit map to window\nRecentre"));
    connect(RecentAct, SIGNAL(triggered()), this, SLOT(OnViewCentreView()));

    SetGridAct = new QAction(QIcon(":/images/win/b-5-7.png"), tr("Set Grid"), this);
    SetGridAct->setStatusTip(tr("Overlay grid on plan\nSet Grid"));
    connect(SetGridAct, SIGNAL(triggered()), this, SLOT(OnEditGrid()));

    toggleColor = new QAction(QIcon(":/images/win/b-7-1.png"), tr("Toggle Colour"), this);
    toggleColor->setCheckable(1);
    toggleColor->setStatusTip(tr("Toggle colour display on and off\nToggle Colour"));
    connect(toggleColor, SIGNAL(triggered()), this, SLOT(OntoggleColor()));

    toggleOrg = new QAction(QIcon(":/images/win/b-7-2.png"), tr("Toggle origin on/off"), this);
    toggleOrg->setCheckable(1);
    toggleOrg->setStatusTip(tr("Toggle graph intersect at origin of X, Y values\nToggle origin on/off"));
    connect(toggleOrg, SIGNAL(triggered()), this, SLOT(OntoggleOrg()));

    viewTrend = new QAction(QIcon(":/images/win/b-7-3.png"), tr("View trend line"), this);
    viewTrend->setCheckable(1);
    viewTrend->setStatusTip(tr("Show regression line\nView trend line"));
    connect(viewTrend, SIGNAL(triggered()), this, SLOT(OnviewTrend()));

    yx = new QAction(QIcon(":/images/win/b-7-4.png"), tr(""), this);
    yx->setCheckable(1);
    yx->setStatusTip(tr(""));
    connect(yx, SIGNAL(triggered()), this, SLOT(OnYX()));

    Rtwo = new QAction(QIcon(":/images/win/b-7-5.png"), tr(""), this);
    Rtwo->setCheckable(1);
    Rtwo->setStatusTip(tr(""));
    connect(Rtwo, SIGNAL(triggered()), this, SLOT(OnRtwo()));

///////////////////////////////////////////////////////////
//Popup toolbar
    {
        zoomToolButton = new QToolButton;
        zoomToolButton->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *zoomMenu = new QMenu;
        zoomInAct = new QAction(tr("Zoom in"), this);
        zoomInAct->setStatusTip(tr("Zoom into or out of (using Alt-key) view of map\nZoom In"));
        zoomInAct->setCheckable(1);
        zoomInAct->setChecked(1);
        zoomInAct->setData(ID_MAPBAR_ITEM_ZOOM_IN);
        connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomModeTriggered()));
        zoomMenu->addAction(zoomInAct);
        zoomOutAct = new QAction(tr("Zoom out"), this);
        zoomOutAct->setStatusTip(tr("Zoom out of or into (using Alt-key) view of map\nZoom Out"));
        zoomOutAct->setCheckable(1);
        zoomOutAct->setData(ID_MAPBAR_ITEM_ZOOM_OUT);
        connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomModeTriggered()));
        zoomMenu->addAction(zoomOutAct);
        zoomMenu->setDefaultAction(zoomInAct);
        zoomToolButton->setMenu(zoomMenu);
        zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
        zoomToolButton->setCheckable(1);
        connect(zoomToolButton, SIGNAL(clicked()), this, SLOT(zoomButtonTriggered()));

        QActionGroup* tGroup = new QActionGroup(this);
        tGroup->addAction(zoomInAct);
        tGroup->addAction(zoomOutAct);
    }
    {
        fillColorToolButton = new QToolButton;
        fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *fillColorMenu = new QMenu;

        STDFillColorAct = new QAction(tr("Standard Fill"), this);
        STDFillColorAct->setStatusTip(tr("Standard Fill grid spaces with points\nFill"));
        STDFillColorAct->setCheckable(1);
        STDFillColorAct->setChecked(1);
        STDFillColorAct->setData(ID_MAPBAR_ITEM_FILL);
        connect(STDFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

        ContextFillColorAct = new QAction(tr("Context Fill"), this);
        ContextFillColorAct->setStatusTip(tr("Context Fill grid spaces with points\nFill"));
        ContextFillColorAct->setCheckable(1);
        ContextFillColorAct->setData(ID_MAPBAR_ITEM_SEMIFILL);
        connect(ContextFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

        // AV test - TV
        AugmentFillColorAct = new QAction(tr("Augmented Fill"), this);
        AugmentFillColorAct->setStatusTip(tr("Augmented Fill grid spaces with points\nFill"));
        AugmentFillColorAct->setCheckable(1);
        AugmentFillColorAct->setData(ID_MAPBAR_ITEM_AUGMENT_FILL);
        //connect(AugmentFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

        fillColorMenu->addAction(STDFillColorAct);
        fillColorMenu->addAction(ContextFillColorAct);
        //fillColorMenu->addAction(AugmentFillColorAct); // AV TV
        fillColorMenu->setDefaultAction(STDFillColorAct);
        fillColorToolButton->setMenu(fillColorMenu);
        fillColorToolButton->setIcon(QIcon(":/images/win/b-5-8.png"));
        fillColorToolButton->setCheckable(1);
        connect(fillColorToolButton, SIGNAL(clicked()), this, SLOT(FillButtonTriggered()));

        QActionGroup* tGroup = new QActionGroup(this);
        tGroup->addAction(STDFillColorAct);
        tGroup->addAction(ContextFillColorAct);
        tGroup->addAction(AugmentFillColorAct); // AV TV
    }
    {
        lineToolButton = new QToolButton;
        lineToolButton->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *lineToolMenu = new QMenu;
        SelectLineAct = new QAction(tr("Line"), this);
        SelectLineAct->setStatusTip(tr("Draw a new line\nLine"));
        SelectLineAct->setCheckable(1);
        SelectLineAct->setChecked(1);
        SelectLineAct->setData(ID_MAPBAR_ITEM_LINETOOL);
        connect(SelectLineAct, SIGNAL(triggered()), this, SLOT(LineModeTriggered()));
        SelectPolyLineAct = new QAction(tr("Polygon"), this);
        SelectPolyLineAct->setStatusTip(tr("Draw a new polygon\nPolygon"));
        SelectPolyLineAct->setCheckable(1);
        SelectPolyLineAct->setData(ID_MAPBAR_ITEM_POLYGON);
        connect(SelectPolyLineAct, SIGNAL(triggered()), this, SLOT(LineModeTriggered()));
        lineToolMenu->addAction(SelectLineAct);
        lineToolMenu->addAction(SelectPolyLineAct);
        lineToolMenu->setDefaultAction(SelectLineAct);
        lineToolButton->setMenu(lineToolMenu);
        lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
        lineToolButton->setCheckable(1);
        connect(lineToolButton, SIGNAL(clicked()), this, SLOT(LineButtonTriggered()));

        QActionGroup* tGroup = new QActionGroup(this);
        tGroup->addAction(SelectLineAct);
        tGroup->addAction(SelectPolyLineAct);
    }
    {
        newisoToolButton = new QToolButton;
        newisoToolButton->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *isoToolMenu = new QMenu;
        MakeIosAct = new QAction(tr("Isovisit"), this);
        MakeIosAct->setStatusTip(tr("Make a new isovist\nIsovist"));
        MakeIosAct->setCheckable(1);
        MakeIosAct->setChecked(1);
        MakeIosAct->setData(ID_MAPBAR_ITEM_ISOVIST);
        connect(MakeIosAct, SIGNAL(triggered()), this, SLOT(isoModeTriggered()));
        PartialMakeIosAct = new QAction(tr("Partial isovisist"), this);
        PartialMakeIosAct->setStatusTip(tr("Make a new partial isovist\nPartial Isovist"));
        PartialMakeIosAct->setCheckable(1);
        PartialMakeIosAct->setData(ID_MAPBAR_ITEM_HALFISOVIST);
        connect(PartialMakeIosAct, SIGNAL(triggered()), this, SLOT(isoModeTriggered()));
        isoToolMenu->addAction(MakeIosAct);
        isoToolMenu->addAction(PartialMakeIosAct);
        isoToolMenu->setDefaultAction(MakeIosAct);
        newisoToolButton->setMenu(isoToolMenu);
        newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
        newisoToolButton->setCheckable(1);
        connect(newisoToolButton, SIGNAL(clicked()), this, SLOT(isoButtonTriggered()));

        QActionGroup* tGroup = new QActionGroup(this);
        tGroup->addAction(MakeIosAct);
        tGroup->addAction(PartialMakeIosAct);
    }
    {
        JoinToolButton = new QToolButton;
        JoinToolButton->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *joinToolMenu = new QMenu;
        JoinAct = new QAction(tr("Link"), this);
        JoinAct->setStatusTip(tr("merge points together\nLink"));
        JoinAct->setCheckable(1);
        JoinAct->setChecked(1);
        JoinAct->setData(ID_MAPBAR_ITEM_JOIN);
        connect(JoinAct, SIGNAL(triggered()), this, SLOT(joinTriggered()));
        JoinUnlinkAct = new QAction(tr("unLink"), this);
        JoinUnlinkAct->setStatusTip(tr("unmerge points\nUnlink"));
        JoinUnlinkAct->setCheckable(1);
        JoinUnlinkAct->setData(ID_MAPBAR_ITEM_UNJOIN);
        connect(JoinUnlinkAct, SIGNAL(triggered()), this, SLOT(joinTriggered()));
        joinToolMenu->addAction(JoinAct);
        joinToolMenu->addAction(JoinUnlinkAct);
        joinToolMenu->setDefaultAction(JoinAct);
        JoinToolButton->setMenu(joinToolMenu);
        JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
        JoinToolButton->setCheckable(1);
        connect(JoinToolButton, SIGNAL(clicked()), this, SLOT(joinButtonTriggered()));

        QActionGroup* tGroup = new QActionGroup(this);
        tGroup->addAction(JoinAct);
        tGroup->addAction(JoinUnlinkAct);
    }

    SelectButton = new QToolButton;
    SelectButton->setStatusTip(tr("Select a grid point\nSelect"));
    SelectButton->setIcon(QIcon(":/images/win/b-5-1.png"));
    SelectButton->setCheckable(1);
    connect(SelectButton, SIGNAL(clicked()), this, SLOT(SelectButtonTriggered()));
    DragButton = new QToolButton;
    DragButton->setStatusTip(tr("Click and drag to move map\nDrag"));
    DragButton->setIcon(QIcon(":/images/win/b-5-2.png"));
    DragButton->setCheckable(1);
    connect(DragButton, SIGNAL(clicked()), this, SLOT(DragButtonTriggered()));
    SelectPenButton = new QToolButton;
    SelectPenButton->setStatusTip(tr("Fill grid spaces individually (or click on a filled space to clear)\nPencil"));
    SelectPenButton->setIcon(QIcon(":/images/win/b-5-9.png"));
    SelectPenButton->setCheckable(1);
    connect(SelectPenButton, SIGNAL(clicked()), this, SLOT(SelectPenTriggered()));
    AxialMapButton = new QToolButton;
    AxialMapButton->setStatusTip(tr("Construct all line axial map from a seed point\nAxial Map"));
    AxialMapButton->setIcon(QIcon(":/images/win/b-5-14.png"));
    AxialMapButton->setCheckable(1);
    connect(AxialMapButton, SIGNAL(clicked()), this, SLOT(AxialMapTriggered()));
    StepDepthButton = new QToolButton;
    StepDepthButton->setStatusTip(tr("Step depth from current selection\nStep Depth"));
    StepDepthButton->setIcon(QIcon(":/images/win/b-5-15.png"));
    StepDepthButton->setCheckable(1);
    connect(StepDepthButton, SIGNAL(clicked()), this, SLOT(StepDepthTriggered()));

    QButtonGroup* pointerTypeGroup = new QButtonGroup;
    pointerTypeGroup->addButton(JoinToolButton, ID_MAPBAR_JOIN_ITEMS);
    pointerTypeGroup->addButton(zoomToolButton, ID_MAPBAR_ZOOM_ITEMS);
    pointerTypeGroup->addButton(fillColorToolButton, ID_MAPBAR_FILL_ITEMS);
    pointerTypeGroup->addButton(lineToolButton, ID_MAPBAR_DRAW_ITEMS);
    pointerTypeGroup->addButton(newisoToolButton, ID_MAPBAR_ISOVIST_ITEMS);
    pointerTypeGroup->addButton(SelectButton, ID_MAPBAR_ITEM_SELECT);
    pointerTypeGroup->addButton(DragButton, ID_MAPBAR_ITEM_MOVE);
    pointerTypeGroup->addButton(SelectPenButton, ID_MAPBAR_ITEM_PENCIL);
    pointerTypeGroup->addButton(AxialMapButton, ID_MAPBAR_ITEM_AL2);
    pointerTypeGroup->addButton(StepDepthButton, ID_MAPBAR_ITEM_PD);

    {
        toolsImportTracesAct = new QAction(QIcon(":/images/win/b-4-1.png"), tr("Import Traces"), this);
        toolsImportTracesAct->setStatusTip(tr("Import agent traces from a file\nImport Traces"));
        connect(toolsImportTracesAct, SIGNAL(triggered()), this, SLOT(OnToolsImportTraces()));

        addAgentAct = new QAction(QIcon(":/images/win/b-4-2.png"), tr("Add Agent"), this);
        addAgentAct->setCheckable(1);
        connect(addAgentAct, SIGNAL(triggered()), this, SLOT(OnAddAgent()));

        QActionGroup *tGroup = new QActionGroup(this);
        tGroup->addAction(toolsImportTracesAct);
        tGroup->addAction(addAgentAct);
    }
    {
        toolsAgentsPlayAct = new QAction(QIcon(":/images/win/b-4-3.png"), tr("Agents Play"), this);
        toolsAgentsPlayAct->setCheckable(1);
        connect(toolsAgentsPlayAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsPlay()));

        toolsAgentsPauseAct = new QAction(QIcon(":/images/win/b-4-4.png"), tr("Agents Pause"), this);
        toolsAgentsPauseAct->setCheckable(1);
        connect(toolsAgentsPauseAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsPause()));

        toolsAgentsStopAct = new QAction(QIcon(":/images/win/b-4-5.png"), tr("Agents Stop"), this);
        connect(toolsAgentsStopAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsStop()));

        QActionGroup *tGroup = new QActionGroup(this);
        tGroup->addAction(toolsAgentsPlayAct);
        tGroup->addAction(toolsAgentsPauseAct);
        tGroup->addAction(toolsAgentsStopAct);
    }
    agentTrailsAct = new QAction(QIcon(":/images/win/b-4-6.png"), tr("Agent Trails"), this);
    agentTrailsAct->setCheckable(1);
    connect(agentTrailsAct, SIGNAL(triggered()), this, SLOT(OnAgentTrails()));
    {
        thirdRotAct = new QAction(QIcon(":/images/win/b-4-7.png"), tr("3D Rot"), this);
        thirdRotAct->setCheckable(1);
        connect(thirdRotAct, SIGNAL(triggered()), this, SLOT(On3dRot()));

        thirdPanAct = new QAction(QIcon(":/images/win/b-4-8.png"), tr("3D Pan"), this);
        thirdPanAct->setCheckable(1);
        connect(thirdPanAct, SIGNAL(triggered()), this, SLOT(On3dPan()));

        thirdZoomAct = new QAction(QIcon(":/images/win/b-4-9.png"), tr("3D Zoom"), this);
        thirdZoomAct->setCheckable(1);
        connect(thirdZoomAct, SIGNAL(triggered()), this, SLOT(On3dZoom()));

        playLoopAct = new QAction(QIcon(":/images/win/b-4-10.png"), tr("Play Loop"), this);
        playLoopAct->setCheckable(1);
        connect(playLoopAct, SIGNAL(triggered()), this, SLOT(OnPlayLoop()));

        QActionGroup *tGroup = new QActionGroup(this);
        tGroup->addAction(thirdRotAct);
        tGroup->addAction(thirdPanAct);
        tGroup->addAction(thirdZoomAct);
        tGroup->addAction(playLoopAct);
        tGroup->addAction(addAgentAct);
    }
    thirdFilledAct = new QAction(QIcon(":/images/win/b-4-11.png"), tr("3D Filled"), this);
    thirdFilledAct->setCheckable(1);
    connect(thirdFilledAct, SIGNAL(triggered()), this, SLOT(On3dFilled()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(closeAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(propertiesAct);
    fileMenu->addSeparator();
    fileMenu->addAction(printAct);
    fileMenu->addAction(printPreviewAct);
    fileMenu->addAction(printSetupAct);
    separatorAct = fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        fileMenu->addAction(recentFileActs[i]);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    updateRecentFileActions(mSettings.readSetting(SettingTag::recentFileList).toStringList());

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addSeparator();
    editMenu->addAction(copyDataAct);
    editMenu->addAction(copyScreenAct);
    editMenu->addAction(exportScreenAct);
    editMenu->addSeparator();
    editMenu->addAction(clearAct);
    editMenu->addSeparator();
    editMenu->addAction(selectByQueryAct);
    //editMenu->addAction(zoomToSelectionAct);
    editMenu->addAction(selectionToLayerAct);

    mapMenu = menuBar()->addMenu(tr("&Map"));
    mapMenu->addAction(mapNewAct);
    mapMenu->addAction(deleteAct);
    mapMenu->addSeparator();
    mapMenu->addAction(convertActiveMapAct);
    mapMenu->addAction(convertDrawingMapAct);
    mapMenu->addAction(convertMapShapesAct);
    mapMenu->addSeparator();
    mapMenu->addAction(importAct);
    exportSubMenu = mapMenu->addMenu(tr("&Export"));
    exportSubMenu->addAction(exportAct);
    exportSubMenu->addAction(exportLinksAct);
    exportSubMenu->addAction(exportAxialConnectionsDotAct);
    exportSubMenu->addAction(exportAxialConnectionsPairAct);
    exportSubMenu->addAction(exportSegmentConnectionsPairAct);
    exportSubMenu->addAction(exportPointmapConnectionsPairAct);

    attributesMenu = menuBar()->addMenu(tr("&Attributes"));
    attributesMenu->addAction(addColumAct);
    attributesMenu->addSeparator();
    attributesMenu->addAction(renameColumnAct);
    attributesMenu->addAction(updateColumAct);
    attributesMenu->addAction(removeColumAct);
    attributesMenu->addAction(columnPropertiesAct);
    attributesMenu->addSeparator();
    attributesMenu->addAction(pushValueAct);

    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    visibilitySubMenu = toolsMenu->addMenu(tr("&Visibility"));
    visibilitySubMenu->addAction(SetGridAct);
    visibilitySubMenu->addAction(makeVisibilityGraphAct);
    visibilitySubMenu->addAction(importVGALinksAct);
    visibilitySubMenu->addAction(makeIsovistPathAct);
    visibilitySubMenu->addSeparator();
    visibilitySubMenu->addAction(runVisibilityGraphAnalysisAct);
    stepDepthSubMenu = visibilitySubMenu->addMenu(tr("Step &Depth"));
    stepDepthSubMenu->addAction(visibilityStepAct);
    stepDepthSubMenu->addAction(metricStepAct);
    stepDepthSubMenu->addAction(angularStepAct);
    visibilitySubMenu->addSeparator();
    visibilitySubMenu->addAction(convertDataMapLinesAct);
    agentToolsSubMenu = toolsMenu->addMenu(tr("&Agent Tools"));
    agentToolsSubMenu->addAction(runAgentAnalysisAct);
    agentToolsSubMenu->addAction(loadAgentProgramAct);

    axialSubMenu = toolsMenu->addMenu(tr("A&xial / Convex / Pesh"));
    axialSubMenu->addAction(runGraphAnaysisAct);
    axialSubMenu->addAction(stepDepthAct);
    axialSubMenu->addSeparator();
    axialSubMenu->addAction(reduceToFewestLineMapAct);
    axialSubMenu->addSeparator();
    axialSubMenu->addAction(convertDataMapPointsAct);
    axialSubMenu->addAction(loadUnlinksFromFileAct);

    segmentSubMenu = toolsMenu->addMenu(tr("&Segment"));
    segmentSubMenu->addAction(runAngularSegmentAnalysisAct);
    segmentSubMenu->addAction(runTopologicalOrMetricAnalysisAct);
    segmentStepDepthSubMenu = segmentSubMenu->addMenu(tr("Step &Depth"));
    segmentStepDepthSubMenu->addAction(segmentAngularStepAct);
    segmentStepDepthSubMenu->addAction(topologicalStepAct);
    segmentStepDepthSubMenu->addAction(segmentMetricStepAct);

    toolsMenu->addSeparator();
    toolsMenu->addAction(optionsAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(RecentAct);
    viewMenu->addAction(showGridAct);
    viewMenu->addAction(attributeSummaryAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(mapAct);
    windowMenu->addAction(scatterPlotAct);
    windowMenu->addAction(tableAct);
    windowMenu->addAction(thirdDViewAct);
    windowMenu->addAction(glViewAct);
    windowMenu->addSeparator();
    windowMenu->addAction(colourRangeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(cascadeAct);
    windowMenu->addAction(tileAct);
    windowMenu->addAction(arrangeIconsAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(onlineBugsAct);
    helpMenu->addAction(onlineHandbookAct);
    helpMenu->addAction(onlineTutorialsAct);
    helpMenu->addAction(onlineScriptingManualAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutDepthMapAct);

    connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));
    connect(visibilitySubMenu, SIGNAL(aboutToShow()), this, SLOT(updateVisibilitySubMenu()));
    connect(stepDepthSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateStepDepthSubMenu()));
    connect(agentToolsSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateAgentToolsSubMenu()));
    connect(segmentSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateSegmentSubMenu()));
    connect(segmentStepDepthSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateSegmentStepDepthSubMenu()));
    connect(axialSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateAxialSubMenu()));
    connect(attributesMenu, SIGNAL(aboutToShow()), this, SLOT(updateAttributesMenu()));
    connect(mapMenu, SIGNAL(aboutToShow()), this, SLOT(updateMapMenu()));
    connect(editMenu, SIGNAL(aboutToShow()), this, SLOT(updateEditMenu()));
    connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(importAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addSeparator();
    fileToolBar->addAction(addColumAct);
    fileToolBar->addAction(updateColumAct);
    fileToolBar->addAction(removeColumAct);
    fileToolBar->addAction(pushValueAct);
    fileToolBar->addSeparator();
    fileToolBar->addAction(invertColorAct);
    fileToolBar->setIconSize(QSize(16,16));
    fileToolBar->setMovable(0);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addWidget(SelectButton);
    editToolBar->addWidget(DragButton);
    editToolBar->addWidget(zoomToolButton);
    editToolBar->addSeparator();
    //editToolBar->addAction(zoomToAct);
    editToolBar->addAction(RecentAct);
    editToolBar->addSeparator();
    editToolBar->addAction(SetGridAct);
    editToolBar->addWidget(fillColorToolButton);
    editToolBar->addWidget(SelectPenButton);
    editToolBar->addWidget(lineToolButton);
    editToolBar->addSeparator();
    editToolBar->addWidget(newisoToolButton);
    editToolBar->addWidget(AxialMapButton);
    editToolBar->addWidget(StepDepthButton);
    editToolBar->addWidget(JoinToolButton);
    editToolBar->setIconSize(QSize(16,16));
    editToolBar->setMovable(0);

    x_coord = new QComboBox(this);
    x_coord->setMinimumContentsLength(20);
    y_coord = new QComboBox(this);
    y_coord->setMinimumContentsLength(20);

    connect(x_coord, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(OnSelchangeViewSelector_X(const QString &)));
    connect(y_coord, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(OnSelchangeViewSelector_Y(const QString &)));


    QAction* xx = new QAction(tr("X = "), this);
    xx->setEnabled(0);
    QAction* yy = new QAction(tr("Y = "), this);
    yy->setEnabled(0);

    plotToolBar = addToolBar(tr("PlotEdit"));
    plotToolBar->addAction(xx);
    plotToolBar->addWidget(x_coord);
    plotToolBar->addSeparator();
    plotToolBar->addAction(yy);
    plotToolBar->addWidget(y_coord);
    plotToolBar->addSeparator();
    plotToolBar->addAction(toggleColor);
    plotToolBar->addAction(toggleOrg);
    plotToolBar->addAction(viewTrend);
    plotToolBar->addAction(yx);
    plotToolBar->addAction(Rtwo);
    plotToolBar->setIconSize(QSize(16,16));
    plotToolBar->setMovable(0);

    thirdViewToolBar = addToolBar(tr("3DView"));
    thirdViewToolBar->addAction(toolsImportTracesAct);
    thirdViewToolBar->addAction(addAgentAct);
    thirdViewToolBar->addSeparator();
    thirdViewToolBar->addAction(toolsAgentsPlayAct);
    thirdViewToolBar->addAction(toolsAgentsPauseAct);
    thirdViewToolBar->addAction(toolsAgentsStopAct);
    thirdViewToolBar->addSeparator();
    thirdViewToolBar->addAction(agentTrailsAct);
    thirdViewToolBar->addSeparator();
    thirdViewToolBar->addAction(thirdRotAct);
    thirdViewToolBar->addAction(thirdPanAct);
    thirdViewToolBar->addAction(thirdZoomAct);
    thirdViewToolBar->addAction(playLoopAct);
    thirdViewToolBar->addSeparator();
    thirdViewToolBar->addAction(thirdFilledAct);
    thirdViewToolBar->setIconSize(QSize(16,16));
    thirdViewToolBar->setMovable(0);

    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-1.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-2.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-3.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-4.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-5.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-6.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-7.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-8.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-9.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-10.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-11.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-12.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-13.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-14.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-15.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-16.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-17.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-18.png")));
    m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-19.png")));
}
