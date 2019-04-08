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
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidgetItem>
#include <QtGui/QFocusEvent>

#include "mainwindow.h"

QT_BEGIN_NAMESPACE

AttribWindow::AttribWindow(QWidget *parent, bool custom)
    : QListWidget(parent)
{
    custom = false;

	main_frm = parent;
    installEventFilter(this);
    setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), 
        this, SLOT(showContextMenu(const QPoint&)));

	connect(this, SIGNAL(itemSelectionChanged()), parent, SLOT(OnSelchangingList()));
}

AttribWindow::~AttribWindow()
{
    // nothing todo
}

void AttribWindow::showContextMenu(const QPoint &point)
{
	QListWidgetItem *item = itemAt(point);
    if (!item) return;

    QPoint ptt(mapToGlobal(point));

    ((MainWindow *)main_frm)->showContextMenu(ptt);
}

QT_END_NAMESPACE
