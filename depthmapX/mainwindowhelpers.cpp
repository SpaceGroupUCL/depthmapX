// Copyright (C) 2020 Petros Koutsolampros
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

#include "depthmapX/mainwindowhelpers.h"
#include "depthmapX/mainwindow.h"

QMenu *MainWindowHelpers::getOrAddRootMenu(MainWindow *mainWindow, QString menuTitle) {
    QMenuBar *menuBar = mainWindow->menuBar();
    QMenu *menu = nullptr;
    foreach (QAction *action, menuBar->actions()) {
        if (action->menu()) {
            QMenu *childMenu = action->menu();
            if (childMenu != nullptr && childMenu->title() == menuTitle) {
                menu = childMenu;
            }
        }
    }
    if (menu == nullptr) {
        menu = menuBar->addMenu(menuTitle);
    }
    return menu;
}

QMenu *MainWindowHelpers::getOrAddMenu(QMenu *parent, QString menuTitle, QAction *before) {
    QMenu *menu = nullptr;
    foreach (QAction *action, parent->actions()) {
        if (action->menu()) {
            QMenu *childMenu = action->menu();
            if (childMenu != nullptr && childMenu->title() == menuTitle) {
                menu = childMenu;
            }
        }
    }
    if (menu == nullptr) {
        if (!before) {
            return parent->addMenu(menuTitle);
        }
        menu = new QMenu(menuTitle, parent);
        parent->insertMenu(before, menu);
    }
    return menu;
}
