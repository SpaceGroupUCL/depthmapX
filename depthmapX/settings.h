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
#include <QString>
#include <QSettings>
#include <memory>


/**
 * Tags for settings to be used when reading writing settings. Don't just use
 * string literals
 */
namespace SettingTag
{
    const QString position = "pos";
    const QString size = "size";
    const QString foregroundColour = "forColor";
    const QString backgroundColour = "backColor";
    const QString antialiasingSamples = "antialisingSamples";
    const QString simpleVersion = "simple";
    const QString recentFileList = "recentFileList";
    const QString mwMaximised = "mainWindowMaximised";
    const QString licenseAccepted = "licenseAccepted";
    const QString depthmapViewSize = "depthmapViewSize";
    const QString legacyMapWindow = "legacyMapWindow";
}

/**
 * @brief Class encapsulating one settings transaction
 * This is supposed to use one underlying QSettings object for several reads/writes
 */
class SettingsTransaction
{
public:
    virtual const QVariant readSetting( const QString& tag, const QVariant& defaultValue = QVariant()) const = 0;
    virtual void writeSetting( const QString& tag, const QVariant& value) = 0;
    virtual ~SettingsTransaction(){}
};

/**
 * @brief The Settings class
 * Encapsulates reading/writing settings so we don't have to faff around with constructing QSettings in
 * 7 different location. The read/write methods on this class create a temporary QSettings object, so
 * if you want to read/write more than one setting in one go, use a transaction.
 */
class Settings : public SettingsTransaction
{
public:
    virtual std::unique_ptr<SettingsTransaction> getTransaction() = 0;
};
