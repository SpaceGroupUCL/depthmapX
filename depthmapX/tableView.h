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


#include "GraphDoc.h"
#include <QTableWidget>

class QEvent;
class QTableWidgetItem;

class tableView : public QTableWidget
{
    Q_OBJECT

public:
    tableView(QWidget *parent = 0, QGraphDoc * p = 0);
    ~tableView();
    QSize sizeHint() const;

	int m_column_count;
	int m_row_count;
	int m_from;
	int m_curr_row;
	QGraphDoc* pDoc;
	bool m_protect_edit;

private slots:
	void itemChanged ( QTableWidgetItem * item );
	void itemEditChanged(QTableWidgetItem*);
	void colum_Sort(int sort);
protected:
	virtual void closeEvent(QCloseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
	virtual void scrollContentsBy ( int dx, int dy );

private:
	int OnRedraw(int wParam, int lParam);
	void RedoTable();
    void PrepareCache(int to);

    bool m_custom;
};
