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

#include <QWidget>
#include <QtWidgets>
#include "settingspage.h"
#include <map>
#include <memory>

class InterfacePage : public SettingsPage
{
    Q_OBJECT

private:
    QColor m_background;
    QColor m_foreground;
    std::map<QListWidgetItem *, QColor *> colourMap;
    int m_antialiasingSamples = 0;
    bool m_defaultMapWindowIsLegacy = false;
    void readSettings(Settings &settings) {
        m_foreground = QColor(settings.readSetting(SettingTag::foregroundColour, qRgb(128,255,128)).toInt());
        m_background = QColor(settings.readSetting(SettingTag::backgroundColour, qRgb(0,0,0)).toInt());
        m_antialiasingSamples = settings.readSetting(SettingTag::antialiasingSamples, 0).toInt();
        m_defaultMapWindowIsLegacy = settings.readSetting(SettingTag::legacyMapWindow, false).toBool();
    }
private slots:
    void onInterfaceColourlItemClicked(QListWidgetItem *item);
public:
    InterfacePage(Settings &settings, QWidget *parent = 0);
    virtual void writeSettings(Settings &settings) override {
        settings.writeSetting(SettingTag::backgroundColour, m_background.rgb());
        settings.writeSetting(SettingTag::foregroundColour, m_foreground.rgb());
        settings.writeSetting(SettingTag::antialiasingSamples, m_antialiasingSamples);
        settings.writeSetting(SettingTag::legacyMapWindow, m_defaultMapWindowIsLegacy);
    }
};
