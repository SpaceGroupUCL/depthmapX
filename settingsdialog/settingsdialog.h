// Copyright (C) 2017 Petros Koutsolampros

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

#pragma once
#include <QDialog>
#include "depthmapX/settings.h"
#include <vector>
#include <memory>
#include "settingsdialog/settingspage.h"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(Settings &settings);

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void createIcons();
    std::vector<std::unique_ptr<SettingsPage>> settingsPages;
    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
    Settings &m_settings;
    void saveChanges();
    void saveChangesAndClose();
};
