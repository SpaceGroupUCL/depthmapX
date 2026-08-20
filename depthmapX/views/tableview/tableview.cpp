// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2019, Petros Koutsolampros

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

#include "tableview.h"

#include "mainwindow.h"

#include <QKeyEvent>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtGui/QFocusEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTableWidgetItem>

#define ROW_HEIGHT 20
#define PG_COUNT 40

TableView::TableView(Settings &settings, QWidget *parent, QGraphDoc *p) : QTableWidget(parent) {
    m_mainWindow = parent;
    pDoc = p;
    m_from = m_curr_row = 0;

    connect(this, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(itemChanged(QTableWidgetItem *)));

    connect(this, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(itemEditChanged(QTableWidgetItem *)));

    RedoTable();
    setWindowIcon(QIcon(tr(":/images/cur/icon-1-5.png")));
    setWindowTitle(pDoc->m_base_title + ":Table View");

    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    m_protect_edit = false;

    m_initialSize = settings.readSetting(SettingTag::depthmapViewSize, QSize(2000, 2000)).toSize();
}

void TableView::focusInEvent(QFocusEvent *e) {
    RedoTable();
    pDoc->SetRedrawFlag(QGraphDoc::VIEW_TABLE, QGraphDoc::REDRAW_DONE);
    QTableWidget::focusInEvent(e);
}

void TableView::RedoTable() {
    clear();

    if (pDoc->m_meta_graph->viewingProcessed()) {
        const AttributeTable &table = pDoc->m_meta_graph->getAttributeTable();

        m_column_count = table.getNumColumns();
        setColumnCount(m_column_count + 1);
        connect(horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(colum_Sort(int)));
        verticalHeader()->hide();

        QTableWidgetItem *Item = new QTableWidgetItem("Ref Number");
        Item->setTextAlignment(Qt::AlignLeft);
        setHorizontalHeaderItem(0, Item);

        for (int i = 0; i < m_column_count; i++) {
            QTableWidgetItem *Item = new QTableWidgetItem(QString("%1").arg(table.getColumnName(i).c_str()));
            Item->setTextAlignment(Qt::AlignLeft);
            setHorizontalHeaderItem(i + 1, Item);
        }

        m_row_count = table.getNumRows();
        setRowCount(m_row_count);
        const AttributeTableHandle &tableHandle = pDoc->m_meta_graph->getAttributeTableHandle();
        auto &index = tableHandle.getTableIndex();
        int idx = 0;
        int found = -1;
        for(auto &indexItem: index) {
            if(indexItem.row->isSelected()) {
                if(idx >= m_curr_row && idx < m_curr_row + PG_COUNT) {
                    found = -1;
                    break;
                }
                found = idx;
            }
            idx++;
        }
        if(found != -1) {
            if(found > m_row_count - PG_COUNT) {
                PrepareCache(m_row_count - PG_COUNT);
                QTableWidget::scrollToBottom();
            } else {
                PrepareCache(found);
                QTableWidget::scrollToItem(item(found, 0));
            }
        } else {
            PrepareCache(m_curr_row);
        }
    }
}

void TableView::scrollContentsBy(int dx, int dy) {
    if (dy != 0) {
        PrepareCache(m_curr_row - dy);
        m_curr_row -= dy;
    }
    QTableWidget::scrollContentsBy(dx, dy);
}

QSize TableView::sizeHint() const { return m_initialSize; }

void TableView::PrepareCache(int to) {
    m_updating = true;
    QTableWidgetItem *Item;
    const AttributeTableHandle &tableHandle = pDoc->m_meta_graph->getAttributeTableHandle();
    auto &index = tableHandle.getTableIndex();

    int diff = PG_COUNT;
    if (to + PG_COUNT >= m_row_count) {
        diff = m_row_count - to;
    }
    for (int i = 0; i < diff; i++) {
        auto &indexItem = index[to + i];
        for (int j = 0; j < m_column_count + 1; j++) {
            if (!j) {
                Item = item(to + i, j);
                if (Item) {
                    if (indexItem.row->isSelected())
                        Item->setCheckState(Qt::Checked);
                    else
                        Item->setCheckState(Qt::Unchecked);
                } else {
                    Item = new QTableWidgetItem(QString("%1").arg(indexItem.key.value));
                    if (indexItem.row->isSelected())
                        Item->setCheckState(Qt::Checked);
                    else
                        Item->setCheckState(Qt::Unchecked);
                    setItem(to + i, 0, Item);
                    continue;
                }
            }
            if (!item(to + i, j)) {
                Item = new QTableWidgetItem(QString("%1").arg(indexItem.row->getValue(j - 1)));
                setRowHeight(to + i, ROW_HEIGHT);
                setItem(to + i, j, Item);
            }
        }
    }
    m_updating = false;
}

void TableView::itemChanged(QTableWidgetItem *item) {
    if (m_updating)
        return;
    int row = item->row();
    int col = item->column();
    MetaGraph *graph = pDoc->m_meta_graph;
    AttributeTable &table = graph->getAttributeTable();
    AttributeTableHandle &tableHandle = graph->getAttributeTableHandle();
    auto &index = tableHandle.getTableIndex();
    if (col == 0) {
        std::vector<int> x;
        x.push_back(index[row].key.value);
        pDoc->m_meta_graph->setSelSet(x);
        pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION, this);
        PrepareCache(m_curr_row);
    } else {
        double value = -1;
        try {
            value = std::stod(item->text().toStdString());
        } catch (std::invalid_argument) {
            QMessageBox::warning(this, tr("Warning"), tr("Cannot convert text to number"), QMessageBox::Ok,
                                 QMessageBox::Ok);
            return;
        }

        if (graph && graph->viewingProcessed()) {
            // go for the change:
            double value2 = index[row].row->getValue(col - 1);
            if (value2 == 0 || fabs((value / value2) - 1.0) > 1e-5) {
                index[row].mutable_row->setValue(col - 1, value);
                pDoc->modifiedFlag = true;
            }

            RedoTable();
            // note: this as caller will prevent us from redrawing ourself:
            // could be either new data or new selection, just go for a big redraw:
            pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA, this);
        }
    }
}

void TableView::colum_Sort(int col_id) {
    if (col_id - 1 != pDoc->m_meta_graph->getDisplayedAttribute()) {
        pDoc->m_meta_graph->setDisplayedAttribute(col_id - 1);
        ((MainWindow *)m_mainWindow)->chooseAttributeOnIndex(col_id);
        clearContents();
        PrepareCache(m_curr_row);
        return;

        RedoTable();
    }
}

void TableView::itemEditChanged(QTableWidgetItem *item) {
    int row = item->row();
    int col = item->column();

    if (col > 0 && col < pDoc->m_meta_graph->getAttributeTable().getNumColumns() + 1) {
        // don't let them edit a locked attribute
        if (pDoc->m_meta_graph->getAttributeTable().getColumn(col - 1).isLocked()) {
            QMessageBox::warning(this, tr("Warning"), tr("This column is locked and cannot be edited"),
                                 QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

void TableView::closeEvent(QCloseEvent *event) {
    pDoc->m_view[QGraphDoc::VIEW_TABLE] = NULL;
    if (!pDoc->OnCloseDocument(QGraphDoc::VIEW_TABLE)) {
        pDoc->m_view[QGraphDoc::VIEW_TABLE] = this;
        event->ignore();
    }
}

void TableView::resizeEvent(QResizeEvent *event) {
    QTableView::resizeEvent(event);
    pDoc->m_view[QGraphDoc::VIEW_TABLE] = this;
}
