// Copyright (C) 2011-2012, Tasos Varoudis
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

#include <QPixmap>
#include <QDir>
#include <QDateTime>

#include "coreapplication.h"

#ifdef _WIN32
#include <windows.h>
#endif


int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(resource);
    Q_INIT_RESOURCE(settingsdialog);

    CoreApplication app(argc, argv);

    return app.exec();
}
