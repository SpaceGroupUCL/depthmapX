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

#include <catch.hpp>
#include <../cliTest/selfcleaningfile.h>
#include <../depthmapX/settingsimpl.h>


class TestSettingsFactory : public QSettingsFactory
{
public:
    TestSettingsFactory(const QString &filename) : mFilename(filename)
    {}
    virtual std::unique_ptr<QSettings> getSettings() const
    {
        return std::unique_ptr<QSettings>(new QSettings(mFilename, QSettings::IniFormat));
    }
private:
    QString mFilename;
};

TEST_CASE("Test simple settings")
{
    SelfCleaningFile scf("./test.ini");
    SettingsImpl settings(new TestSettingsFactory(scf.Filename().c_str()));

    REQUIRE(settings.readSetting("test1", "bar").toString().toStdString() == "bar");
    settings.writeSetting("test1", "foo");
    REQUIRE(settings.readSetting("test1", "bar").toString().toStdString() == "foo");
}


TEST_CASE("Test settings transaction")
{
    SelfCleaningFile scf("./test.ini");
    SettingsImpl settings(new TestSettingsFactory(scf.Filename().c_str()));

    REQUIRE(settings.readSetting("test1", "bar").toString() == "bar");
    {
        auto transaction = settings.getTransaction();
        transaction->writeSetting("test1", "foo");
    }
    REQUIRE(settings.readSetting("test1", "bar").toString() == "foo");

}
