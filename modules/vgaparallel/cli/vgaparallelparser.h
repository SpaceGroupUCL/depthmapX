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

#pragma once

#include "depthmapXcli/commandlineparser.h"
#include "depthmapXcli/imodeparser.h"
#include "depthmapXcli/radiusconverter.h"
#include <string>

class VgaParallelParser : public IModeParser {
  public:
    virtual std::string getModeName() const { return "VGA"; }

    virtual std::string getHelp() const {
        return "Mode options for VGAPARALLEL:\n"
               "-vm <vga mode> one of visiblity-global (default algorithm)"
               "                      visibility-local (default algorithm)"
               "                      visibility-local-adjmatrix (alternative algorithm)"
               "                      metric (default algorithm)"
               "                      angular (default algorithm)\n"
               "-vr <radius> radius between 1 and 99 or n, to limit visibility\n";
    }

  public:
    VgaParallelParser();
    virtual void parse(int argc, char *argv[]);
    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    enum VgaMode { NONE, VISBILITY_GLOBAL, VISBILITY_LOCAL, VISBILITY_LOCAL_ADJMATRIX, METRIC, ANGULAR };

    // vga options
    VgaMode getVgaMode() const { return m_vgaMode; }
    const std::string &getRadius() const { return m_radius; }

  private:
    // vga options
    VgaMode m_vgaMode;
    std::string m_radius;
};
