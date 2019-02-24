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

#pragma once

#include "imodeparser.h"
#include "genlib/p2dpoly.h"
#include <vector>

class PointDepthParser : public IModeParser
{
public:
    PointDepthParser() : m_pointDepthType(PointDepthType::NONE)
    {}

    virtual std::string getModeName() const
    {
        return "POINTDEPTH";
    }

    virtual std::string getHelp() const
    {
        return "Mode options for pointmap POINTDEPTH are:\n" \
               "  -pdp <origin point> point where to calculate depth from. Can be repeated\n" \
               "  -pdf <origin point file> a file with a point per line to calculate depth from\n" \
               "  -pdt <type> Point depth type. One of metric, angular or visual";
    }

    enum class PointDepthType {
        NONE,
        ANGULAR,
        METRIC,
        VISUAL
    };

    virtual void parse(int argc, char** argv);

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    std::vector<Point2f> getOriginPoints() const { return m_originPoints; }

    PointDepthType getPointDepthType() const { return m_pointDepthType; }

private:
    std::vector<Point2f> m_originPoints;

    PointDepthType m_pointDepthType;
};


