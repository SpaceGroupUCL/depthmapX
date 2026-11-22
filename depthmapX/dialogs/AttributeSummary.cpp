// Copyright (C) 2011-2026, Tasos Varoudis

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

#include "AttributeSummary.h"
#include "GraphDoc.h"
#include "ColumnPropertiesDlg.h"

CAttributeSummary::CAttributeSummary(QGraphDoc *pDoc, QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
	m_pDoc = pDoc;
}

void CAttributeSummary::OnOK()
{
	accept();
}

void CAttributeSummary::OnDblclkList(int row, int column)
{
	if (row != -1) {
        CColumnPropertiesDlg dlg(&(m_pDoc->m_meta_graph->getAttributeTable()), &(m_pDoc->m_meta_graph->getLayers()), row);
		dlg.exec();
	}
}

void CAttributeSummary::UpdateData(bool value)
{

}

void CAttributeSummary::showEvent(QShowEvent * event)
{
    const AttributeTable& table = m_pDoc->m_meta_graph->getAttributeTable();
	
	c_list->setSelectionBehavior(QAbstractItemView::SelectRows);

    c_list->setColumnCount(4);
	
	QTableWidgetItem *Item;
	Item = new QTableWidgetItem("Attribute");
	c_list->setColumnWidth(0, 220);
	Item->setTextAlignment(Qt::AlignLeft);
	c_list->setHorizontalHeaderItem(0, Item);

	Item = new QTableWidgetItem("Minimum");
	c_list->setColumnWidth(1, 100);
	Item->setTextAlignment(Qt::AlignRight);
	c_list->setHorizontalHeaderItem(1, Item);

	Item = new QTableWidgetItem("Average");
	c_list->setColumnWidth(2, 100);
	Item->setTextAlignment(Qt::AlignRight);
	c_list->setHorizontalHeaderItem(2, Item);

	Item = new QTableWidgetItem("Maximum");
	c_list->setColumnWidth(3, 100);
	Item->setTextAlignment(Qt::AlignRight);
	c_list->setHorizontalHeaderItem(3, Item);
	
	c_list->clearContents();

    c_list->setRowCount(table.getNumColumns());
    for (int i = 0; i < table.getNumColumns(); i++) {
        const AttributeColumn& column = table.getColumn(i);
        const auto &stats = column.getStats();
        Item = new QTableWidgetItem(QString(column.getName().c_str()));
		Item->setFlags(Qt::NoItemFlags);
		c_list->setRowHeight(i, 20);
		c_list->setItem(i, 0, Item);
		//
		// Min
        Item = new QTableWidgetItem(stats.count ? QString::number(stats.min) : QString());
		Item->setFlags(Qt::NoItemFlags);
		c_list->setItem(i, 1, Item);
		// Avg
        Item = new QTableWidgetItem(stats.count ? QString::number(stats.total / stats.count) : QString());
		Item->setFlags(Qt::NoItemFlags);
		c_list->setItem(i, 2, Item);
		// Max
        Item = new QTableWidgetItem(stats.count ? QString::number(stats.max) : QString());
		Item->setFlags(Qt::NoItemFlags);
		c_list->setItem(i, 3, Item);
	}
}
