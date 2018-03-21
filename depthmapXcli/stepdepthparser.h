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

class StepDepthParser : public IModeParser
{
public:
    StepDepthParser()
    {}

    virtual std::string getModeName() const
    {
        return "STEPDEPTH";
    }

    virtual std::string getHelp() const
    {
        return "Mode options for pointmap STEPDEPTH are:\n" \
               "  -sdp <step depth point> point where to calculate step depth from. Can be repeated\n" \
               "  -sdf <step depth point file> a file with a point per line to calculate step depth from\n";
    }

    virtual void parse(int argc, char** argv);

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    std::vector<Point2f> getStepDepthPoints() const { return m_stepDepthPoints; }

private:
    std::vector<Point2f> m_stepDepthPoints;
};


