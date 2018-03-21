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


#include "settingsimpl.h"

SettingsImpl::SettingsImpl(QSettingsFactory* factory) : mSettingsFactory(factory)
{

}


const QVariant SettingsImpl::readSetting(const QString &tag, const QVariant &defaultValue) const
{
    auto settings = mSettingsFactory->getSettings();
    return settings->value(tag, defaultValue);
}

void SettingsImpl::writeSetting(const QString &tag, const QVariant &value)
{
    auto settings = mSettingsFactory->getSettings();
    settings->setValue(tag, value);
}

class SettingsTransactionImpl : public SettingsTransaction
{
public:
    SettingsTransactionImpl(std::unique_ptr<QSettings> &&settings) : mSettings(std::move(settings))
    {}
    virtual const QVariant readSetting(const QString &tag, const QVariant &defaultValue) const
    {
        return mSettings->value(tag, defaultValue);
    }
    virtual void writeSetting(const QString &tag, const QVariant &value)
    {
        mSettings->setValue(tag, value);
    }

private:
    std::unique_ptr<QSettings> mSettings;
};

std::unique_ptr<SettingsTransaction> SettingsImpl::getTransaction()
{
    return std::unique_ptr<SettingsTransaction>(new SettingsTransactionImpl(mSettingsFactory->getSettings()));
}
