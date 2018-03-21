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
#include "isovistparser.h"
#include "parsingutils.h"
#include "exceptions.h"
#include "salalib/entityparsing.h"
#include <sstream>
#include "runmethods.h"
#include <cstring>

using namespace depthmapX;

IsovistParser::IsovistParser()
{

}


std::string IsovistParser::getModeName() const
{
    return "ISOVIST";
}

std::string IsovistParser::getHelp() const
{
    return "Arguments for isovist mode:\n" \
           "  -ii <x,y[,angle,viewangle]> Define an isoivist at position x,y with\n"\
           "    optional direction angle and view angle for partial isovists\n"\
           "  -if <isovist file> load isovist definitions from a file (csv)\n"\
           "    the relevant headers must be called x, y, angle and viewangle\n"\
           "    the latter two are optional.\n"\
           "  Those two arguments cannot be mixed\n"\
           "  Angles for partial isovists are in degrees, counted anti-clockwise with 0°\n"\
           "  pointing to the right.\n\n";
}

void IsovistParser::parse(int argc, char **argv)
{
    std::string isovistFile;

    for( int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-ii") == 0 )
        {
            if ( !isovistFile.empty())
            {
                throw CommandLineException("-ii cannot be used together with -if");
            }
            ENFORCE_ARGUMENT("-ii", i);
            mIsovists.push_back(EntityParsing::parseIsovist(argv[i]));
        }
        else if( std::strcmp(argv[i], "-if") == 0)
        {
            if ( !isovistFile.empty())
            {
                throw CommandLineException("-if can only be used once");
            }
            if (!mIsovists.empty())
            {
                throw depthmapX::CommandLineException("-if cannot be used together with -ii");
            }
            ENFORCE_ARGUMENT("-if",i);
            isovistFile = argv[i];
        }
    }

    if (!isovistFile.empty())
    {
        std::ifstream file(isovistFile);
        if ( !file.good())
        {
            std::stringstream message;
            message << "Failed to find file " << isovistFile;
            throw depthmapX::CommandLineException(message.str());
        }
        mIsovists = EntityParsing::parseIsovists(file, ',');
    }
    if (mIsovists.empty())
    {
        throw CommandLineException("No isovists defined. Use -ii or -if");
    }

}

void IsovistParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::runIsovists(clp, mIsovists, perfWriter);
}
