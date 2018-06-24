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


#include <QtCore/QEvent>
#include <QtCore/QDebug>

#include <QtWidgets/QMenu>
#include <QtWidgets/QLayout>
#include <Qt3DInput/QKeyEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidgetItem>
#include <QtGui/QFocusEvent>

#include "mainwindow.h"
#include "tableView.h"

#define ROW_HEIGHT 20
#define PG_COUNT 40

static bool in_update = 0;

tableView::tableView(QWidget *parent, QGraphDoc* p)
    : QTableWidget(parent)
{
	pDoc = p;
	m_from = m_curr_row = 0;

    connect(this, SIGNAL(itemChanged(QTableWidgetItem*)), this,
        SLOT(itemChanged(QTableWidgetItem*)));

    connect(this, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this,
        SLOT(itemEditChanged(QTableWidgetItem*)));

	RedoTable();
    setWindowIcon(QIcon(tr(":/images/cur/icon-1-5.png")));
    setWindowTitle(pDoc->m_base_title+":Table View");

    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

	m_protect_edit = false;
}

tableView::~tableView()
{
    // nothing todo
}

void tableView::RedoTable()
{
   clear();

   if (pDoc->m_meta_graph->viewingProcessed()) {
      const AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
      int col = pDoc->m_meta_graph->getDisplayedAttribute();
	  int i;

      m_column_count = table.getColumnCount();
	  setColumnCount(m_column_count+1);
      connect(horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(colum_Sort(int)));
	  verticalHeader()->hide();

	  QTableWidgetItem *Item = new QTableWidgetItem("Ref Number");
	  Item->setTextAlignment(Qt::AlignLeft);
	  setHorizontalHeaderItem(0, Item);

	  for (i = 0; i < m_column_count; i++) {
         QTableWidgetItem *Item = new QTableWidgetItem(QString("%1").arg(table.getColumnName(i).c_str()));
		 Item->setTextAlignment(Qt::AlignLeft);
		 setHorizontalHeaderItem(i+1, Item);
      }

	  m_row_count = table.getRowCount();
	  setRowCount(m_row_count);
      PrepareCache(m_curr_row);
   }
}

void tableView::scrollContentsBy(int dx, int dy)
{
	if(!dy){
		QTableWidget::scrollContentsBy(dx, 0);
		return;
	}
    PrepareCache(m_curr_row - dy);
	m_curr_row -= dy;
	QTableWidget::scrollContentsBy(dx, dy);
}

QSize tableView::sizeHint() const
{
	return QSize(2000, 2000);
}

void tableView::PrepareCache(int to)
{
	in_update = 1;
	QTableWidgetItem *Item;
	const AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
	AttributeIndex& index = table.m_display_index;

    int diff = PG_COUNT;
    if(to+PG_COUNT >= m_row_count)
    {
        diff = m_row_count - to;
    }
    for (int i = 0; i < diff; i++)
	{
		for (int j = 0; j < m_column_count+1; j++)
		{
			if(!j)
			{
                Item = item(to+i, j);
                if(Item)
				{
					if(table.isSelected(to+i)) Item->setCheckState(Qt::Checked);
					else Item->setCheckState(Qt::Unchecked);
				}
				else
				{
					Item = new QTableWidgetItem(QString("%1").arg(table.getRowKey(index[to+i].index)));
					if(table.isSelected(to+i)) Item->setCheckState(Qt::Checked);
					else Item->setCheckState(Qt::Unchecked);
					setItem(to+i, 0, Item);
					continue;
				}
			}
			if(!item(to+i, j))
			{
				Item = new QTableWidgetItem(QString("%1").arg(table.getValue(index[to+i].index, j-1)));
				setRowHeight(to+i, ROW_HEIGHT);
				setItem(to+i, j, Item);
			}
		}
	}
	in_update = 0;
}

void tableView::itemChanged(QTableWidgetItem * item)
{
	if(in_update) return;
	int row = item->row();
	int col = item->column();
	if(col == 0)
	{
        std::vector<int> x;
		AttributeTable& table = pDoc->m_meta_graph->getAttributeTable();
		AttributeIndex& index = table.m_display_index;
		x.push_back(table.getRowKey(index[row].index));
		pDoc->m_meta_graph->setSelSet(x);
		pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION, this);
        PrepareCache(m_curr_row);
	}
	else
	{
         wchar_t *endptr;
         // first reset contents:
         double value = wcstod((wchar_t*)item->text().utf16(), &endptr); // AT: Unicode conversion -- this doesn't look safe -- edittext is a CString!
         if (endptr == (wchar_t*)item->text().utf16()) {
			QMessageBox::warning(this, tr("Warning"), tr("Cannot convert text to number"), QMessageBox::Ok, QMessageBox::Ok);
            return;
         }

         MetaGraph *graph = pDoc->m_meta_graph;
         if (graph && graph->viewingProcessed()) {
            // go for the change:
            AttributeTable &table = graph->getAttributeTable();
            double value2 = table.getValue(table.m_display_index[row].index, col-1);
            if (value2 == 0 || fabs((value / value2) - 1.0) > 1e-5) {
               table.changeValue(table.m_display_index[row].index, col-1, value);
               pDoc->modifiedFlag = true;
            }

			RedoTable();
            // note: this as caller will prevent us from redrawing ourself:
            // could be either new data or new selection, just go for a big redraw:
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA,this);
		}
	}
}

void tableView::colum_Sort(int col_id)
{
	if (col_id - 1 != pDoc->m_meta_graph->getDisplayedAttribute())
	{
		pDoc->m_meta_graph->setDisplayedAttribute(col_id - 1);
		clearContents();
        PrepareCache(m_curr_row);
		return;

		RedoTable();
	}
}

int tableView::OnRedraw(int wParam, int lParam)
{
   bool done = false;
   if (pDoc->GetRemenuFlag(QGraphDoc::VIEW_TABLE)) {
      pDoc->SetRemenuFlag(QGraphDoc::VIEW_TABLE,false);
      // this is big: start from scratch...
      RedoTable();
      done = true;
   }
   int flag = pDoc->GetRedrawFlag(QGraphDoc::VIEW_TABLE);
   if (flag != QGraphDoc::REDRAW_DONE) {
      //
      while (!pDoc->SetRedrawFlag(QGraphDoc::VIEW_TABLE,QGraphDoc::REDRAW_DONE)) {
         // prefer waitformultipleobjects here
      }
      //
      if (!done && lParam != (long) this) {
         if (wParam == QGraphDoc::NEW_TABLE) {
            // this is big start from scratch...
            RedoTable();
            done = true;
         }
         else {
            // redo the cache and redisplay
            PrepareCache(m_curr_row);
         }
      }
   }

   return 0;
}

void tableView::itemEditChanged(QTableWidgetItem* item)
{
	int row = item->row();
	int col = item->column();

	if (col > 0 && col < pDoc->m_meta_graph->getAttributeTable().getColumnCount() + 1) {
	// don't let them edit a locked attribute
		if (pDoc->m_meta_graph->getAttributeTable().isColumnLocked(col-1)) {
		   QMessageBox::warning(this, tr("Warning"), tr("This column is locked and cannot be edited"), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

void tableView::closeEvent(QCloseEvent *event)
{
   pDoc->m_view[QGraphDoc::VIEW_TABLE] = NULL;
   if (!pDoc->OnCloseDocument(QGraphDoc::VIEW_TABLE))
   {
	   pDoc->m_view[QGraphDoc::VIEW_TABLE] = this;
	   event->ignore();
   }
}

void tableView::resizeEvent(QResizeEvent *event)
{
   pDoc->m_view[QGraphDoc::VIEW_TABLE] = this;
}
