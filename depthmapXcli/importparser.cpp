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

#include "importparser.h"
#include "exceptions.h"
#include "runmethods.h"
#include "parsingutils.h"
#include <cstring>
#include <memory>
#include <sstream>

using namespace depthmapX;

void ImportParser::parse(int argc, char *argv[])
{
    for ( int i = 1; i < argc; ++i)
    {
        if ( strcmp ("-if", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-if", i)
            m_filesToImport.push_back(argv[i]);
        } else if ( strcmp ("-iaa", argv[i]) == 0)
        {
            m_importAsAttributes = true;
        }
    }
}

void ImportParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::importFiles(clp, *this, perfWriter);
}
