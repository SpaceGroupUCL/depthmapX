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

#include "linkparser.h"
#include "salalib/mgraph.h"
#include "salalib/entityparsing.h"
#include "exceptions.h"
#include "runmethods.h"
#include "parsingutils.h"
#include <cstring>
#include <memory>
#include <sstream>

using namespace depthmapX;

void LinkParser::parse(int argc, char *argv[])
{
    std::string linksFile;
    std::vector<std::string> manualLinks;

    for ( int i = 1; i < argc;  )
    {
        if ( std::strcmp ("-lf", argv[i]) == 0)
        {
            if (!linksFile.empty())
            {
                throw CommandLineException("-lf can only be used once at the moment");
            }
            else if (manualLinks.size() != 0)
            {
                throw CommandLineException("-lf can not be used in conjunction with -lnk");
            }
            ENFORCE_ARGUMENT("-lf", i)
            linksFile = argv[i];
        }
        else if ( std::strcmp ("-lnk", argv[i]) == 0)
        {
            if (!linksFile.empty())
            {
                throw CommandLineException("-lf can not be used in conjunction with -lnk");
            }
            ENFORCE_ARGUMENT("-lnk", i)
            if (!has_only_digits_dots_commas(argv[i]))
            {
                std::stringstream message;
                message << "Invalid link provided ("
                        << argv[i]
                        << "). Should only contain digits dots and commas"
                        << flush;
                throw CommandLineException(message.str().c_str());
            }
            manualLinks.push_back(argv[i]);
        }
        ++i;
    }
    if ( manualLinks.size() == 0 && linksFile.empty())
    {
        throw CommandLineException("one of -lf or -lnk must be provided");
    }

    if(!linksFile.empty())
    {
        std::ifstream linksStream(linksFile);
        if (!linksStream)
        {
            std::stringstream message;
            message << "Failed to load file " << linksFile << ", error " << flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }
        vector<Line> lines = EntityParsing::parseLines(linksStream, '\t');
        _mergeLines.insert(std::end(_mergeLines), std::begin(lines), std::end(lines));
    }
    else if(!manualLinks.empty())
    {
        std::stringstream linksStream;
        linksStream << "x1,y1,x2,y2";
        std::vector<std::string>::iterator iter = manualLinks.begin(), end =
        manualLinks.end();
        for ( ; iter != end; ++iter )
        {
            linksStream << "\n" << *iter;
        }
        vector<Line> lines = EntityParsing::parseLines(linksStream, ',');
        _mergeLines.insert(std::end(_mergeLines), std::begin(lines), std::end(lines));
    }
}

void LinkParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::linkGraph(clp, _mergeLines, perfWriter);
}
