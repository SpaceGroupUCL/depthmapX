// Copyright (C) 2017 Christian Sailer
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

#ifndef RUNMETHODS_H
#define RUNMETHODS_H
#include <string>
#include "commandlineparser.h"
#include "radiusconverter.h"
#include "performancesink.h"
#include "vgaparser.h"
#include "axialparser.h"
#include "agentparser.h"
#include "exportparser.h"
#include "salalib/isovistdef.h"
#include <vector>

class Line;
class Point2f;

namespace dm_runmethods{
    void importFiles(const CommandLineParser &cmdP, const std::vector<std::string> &filesToImport, IPerformanceSink &perfWriter);
    void linkGraph(const CommandLineParser &cmdP, const std::vector<Line> &mergeLines, IPerformanceSink &perfWriter );
    void runVga(const CommandLineParser &cmdP, const VgaParser &vgaP, const IRadiusConverter &converter, IPerformanceSink &perfWriter );
    void runVisualPrep(const CommandLineParser &clp, double gridSize, const std::vector<Point2f> &fillPoints, double maxVisibility, bool boundaryGraph, IPerformanceSink &perfWriter);
    void runAxialAnalysis(const CommandLineParser& clp, const AxialParser &ap, IPerformanceSink &perfWriter);
    void runAgentAnalysis(const CommandLineParser &cmdP, const AgentParser &agentP, IPerformanceSink &perfWriter );
    void runIsovists(const CommandLineParser &cmdP, const std::vector<IsovistDefinition> &isovists, IPerformanceSink &perfWriter );
    void exportData(const CommandLineParser &cmdP, const ExportParser &exportP, IPerformanceSink &perfWriter );
    void runStepDepth(const CommandLineParser &clp, const std::vector<Point2f> &stepDepthPoints, IPerformanceSink &perfWriter);
}
#endif // RUNMETHODS_H
