// Copyright (C) 2017 Christian Sailer

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
#include "settings.h"
#include <qsettings.h>
#include <qstandardpaths.h>
#include <memory>

class QSettingsFactory
{
public:
    virtual std::unique_ptr<QSettings> getSettings() const = 0;
    virtual ~QSettingsFactory() {}
};

class DefaultSettingsFactory : public QSettingsFactory
{
public:
    DefaultSettingsFactory()
    {
        m_settingsFile = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).first() +  "/depthmapXsettings.ini";
    }

    // QSettingsFactory interface
public:
    virtual std::unique_ptr<QSettings> getSettings() const
    {
        return std::unique_ptr<QSettings>(new QSettings(m_settingsFile, QSettings::IniFormat));
    }

private:
    QString m_settingsFile;
};

class SettingsImpl : public Settings
{
public:
    SettingsImpl(QSettingsFactory *settingsFactory);

    // SettingsTransaction interface
public:
    virtual const QVariant readSetting(const QString &tag, const QVariant &defaultValue) const;
    virtual void writeSetting(const QString &tag, const QVariant &value);

    // Settings interface
    virtual std::unique_ptr<SettingsTransaction> getTransaction();

private:
    std::unique_ptr<QSettingsFactory> mSettingsFactory;

};
