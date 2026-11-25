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

#include "depthmapXcli/imodeparser.h"
#include "genlib/p2dpoly.h"
#include <vector>

class IsovistZoneParser : public IModeParser {
  public:
    IsovistZoneParser() {}

    virtual std::string getModeName() const { return "ISOVISTZONE"; }

    virtual std::string getHelp() const {
        return "Mode options for pointmap ISOVISTZONE are:\n"
               "  -izo <point> Origin point (can be given multiple times)\n"
               "  -izn <name> Set of an origin. Either not provided or provided for every origin\n"
               "  -izf <file path> load origins from a file (csv)\n"
               "    the relevant headers must be called x, y and set\n"
               "    the last one is optional.\n"
               "  -izr <distance> Restrict distance of isovist zone from origins\n";
    }

    virtual void parse(int argc, char **argv);

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    std::vector<Point2f> getOrigins() const { return m_origins; }

  private:
    std::vector<Point2f> m_origins;
    std::vector<std::string> m_originSets;
    float m_restrictDistance = -1;
};
