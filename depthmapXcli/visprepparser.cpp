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

#include "visprepparser.h"
#include "exceptions.h"
#include "parsingutils.h"
#include "salalib/entityparsing.h"
#include "runmethods.h"
#include <sstream>
#include <cstring>

using namespace depthmapX;

void VisPrepParser::parse(int argc, char ** argv)
{

    std::vector<std::string> points;
    std::string pointFile;
    for ( int i = 1; i < argc; ++i )
    {
        if ( std::strcmp ("-pg", argv[i]) == 0)
        {
            if (m_grid >= 0)
            {
                throw CommandLineException("-pg can only be used once");
            }
            ENFORCE_ARGUMENT("-pg", i)
            m_grid = std::atof(argv[i]);
            if (m_grid <= 0)
            {
                throw CommandLineException(std::string("-pg must be a number >0, got ") + argv[i]);
            }
        }
        else if ( std::strcmp ("-pp", argv[i]) == 0)
        {
            if (!pointFile.empty())
            {
                throw CommandLineException("-pp cannot be used together with -pf");
            }
            ENFORCE_ARGUMENT("-pp", i)
            if (!has_only_digits_dots_commas(argv[i]))
            {
                std::stringstream message;
                message << "Invalid fill point provided ("
                        << argv[i]
                        << "). Should only contain digits dots and commas"
                        << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            points.push_back(argv[i]);
        }
        else if ( std::strcmp("-pf", argv[i]) == 0 )
        {
            if (!points.empty())
            {
                throw CommandLineException("-pf cannot be used together with -pp");
            }
            ENFORCE_ARGUMENT("-pf", i)
            pointFile = argv[i];
        }
        else if ( std::strcmp("-pr", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-pr", i);
            m_maxVisibility = std::atof(argv[i]);
            if ( m_maxVisibility == 0.0)
            {
                std::stringstream message;
                message << "Restricted visibility of '" << argv[i] << "' makes no sense, use a positive number or -1 for unrestricted";
                throw CommandLineException(message.str());
            }
        }
        else if ( std::strcmp("-pb", argv[i]) == 0 )
        {
            m_boundaryGraph = true;
        }
    }
    if (m_grid < 0)
    {
        throw CommandLineException("Use -pg to define grid");
    }

    if (pointFile.empty() && points.empty())
    {
        throw CommandLineException("Either -pp or -pf must be given");
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
        m_fillPoints.insert(std::end(m_fillPoints), std::begin(parsed), std::end(parsed));
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
        m_fillPoints.insert(std::end(m_fillPoints), std::begin(parsed), std::end(parsed));

    }
}

void VisPrepParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    dm_runmethods::runVisualPrep(clp, m_grid, m_fillPoints, m_maxVisibility, m_boundaryGraph, perfWriter);
}
