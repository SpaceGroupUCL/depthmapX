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

#include "pointdepthparser.h"
#include "exceptions.h"
#include "parsingutils.h"
#include "salalib/entityparsing.h"
#include "runmethods.h"
#include <sstream>
#include <cstring>

using namespace depthmapX;

void PointDepthParser::parse(int argc, char ** argv)
{

    std::vector<std::string> points;
    std::string pointFile;
    for ( int i = 1; i < argc; ++i )
    {
        if ( std::strcmp ("-pdp", argv[i]) == 0)
        {
            if (!pointFile.empty())
            {
                throw CommandLineException("-pdp cannot be used together with -pdf");
            }
            ENFORCE_ARGUMENT("-pdp", i)
            if (!has_only_digits_dots_commas(argv[i]))
            {
                std::stringstream message;
                message << "Invalid origin point provided ("
                        << argv[i]
                        << "). Should only contain digits dots and commas"
                        << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            points.push_back(argv[i]);
        }
        else if ( std::strcmp("-pdf", argv[i]) == 0 )
        {
            if (!points.empty())
            {
                throw CommandLineException("-pdf cannot be used together with -pdp");
            }
            ENFORCE_ARGUMENT("-pdf", i)
            pointFile = argv[i];
        }
        else if ( std::strcmp ("-pdt", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-pdt", i)
            if ( std::strcmp(argv[i], "angular") == 0 )
            {
                m_pointDepthType = PointDepthType::ANGULAR;
            }
            else if ( std::strcmp(argv[i], "metric") == 0 )
            {
                m_pointDepthType = PointDepthType::METRIC;
            }
            else if ( std::strcmp(argv[i], "visual") == 0 )
            {
                m_pointDepthType = PointDepthType::VISUAL;
            }
            else
            {
                throw CommandLineException(std::string("Invalid point depth type: ") + argv[i]);
            }
        }
    }

    if (pointFile.empty() && points.empty())
    {
        throw CommandLineException("Either -pdp or -pdf must be given");
    }

    if(!pointFile.empty())
    {
        std::ifstream pointsStream(pointFile);
        if (!pointsStream)
        {
            std::stringstream message;
            message << "Failed to load file " << pointFile << ", error " << std::strerror(errno) << std::flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }
        std::vector<Point2f> parsed = EntityParsing::parsePoints(pointsStream, '\t');
        m_originPoints.insert(std::end(m_originPoints), std::begin(parsed), std::end(parsed));
    }
    else if(!points.empty())
    {
        std::stringstream pointsStream;
        pointsStream << "x,y";
        std::vector<std::string>::iterator iter = points.begin(), end =
        points.end();
        for ( ; iter != end; ++iter )
        {
            pointsStream << "\n" << *iter;
        }
        std::vector<Point2f> parsed = EntityParsing::parsePoints(pointsStream, ',');
        m_originPoints.insert(std::end(m_originPoints), std::begin(parsed), std::end(parsed));

    }

    if (m_pointDepthType == PointDepthType::NONE)
    {
        throw CommandLineException("Point depth type (-pdt) must be provided");
    }
}

void PointDepthParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::runPointDepth(clp, m_pointDepthType, m_originPoints, perfWriter);
}
