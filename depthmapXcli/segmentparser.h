// Copyright (C) 2018 Petros Koutsolampros

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

class SegmentParser : public IModeParser
{
public:
    SegmentParser();

    // IModeParser interface
public:
    std::string getModeName() const;
    std::string getHelp() const;
    void parse(int argc, char **argv);
    void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    enum class AnalysisType {
        NONE,
        ANGULAR_TULIP,
        ANGULAR_FULL,
        TOPOLOGICAL,
        METRIC
    };

    enum class RadiusType {
        NONE,
        SEGMENT_STEPS,
        ANGULAR,
        METRIC
    };

    AnalysisType getAnalysisType() const { return _analysisType; }

    RadiusType getRadiusType() const { return _radiusType; }

    bool includeChoice() const { return _includeChoice; }

    int getTulipBins() const { return _tulipBins; }

    const std::vector<double> getRadii() const { return _radii;}

private:
    AnalysisType _analysisType;
    RadiusType _radiusType;
    bool _includeChoice;
    int _tulipBins;
    std::vector<double> _radii;
};
