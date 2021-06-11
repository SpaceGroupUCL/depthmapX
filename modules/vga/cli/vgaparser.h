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

class VgaParser : public IModeParser {
  public:
    virtual std::string getModeName() const { return "VGA"; }

    virtual std::string getHelp() const {
        return "Mode options for VGA:\n"
               "-vm <vga mode> one of isovist, visiblity, metric, angular, thruvision\n"
               "-vg turn on global measures for visibility, requires radius between 1 and 99 or n\n"
               "-vl turn on local measures for visibility\n"
               "-vr set visibility radius\n";
    }

  public:
    VgaParser();
    virtual void parse(int argc, char *argv[]);
    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    enum VgaMode { NONE, ISOVIST, VISBILITY, METRIC, ANGULAR, THRU_VISION };

    // vga options
    VgaMode getVgaMode() const { return m_vgaMode; }
    bool localMeasures() const { return m_localMeasures; }
    bool globalMeasures() const { return m_globalMeasures; }
    const std::string &getRadius() const { return m_radius; }

  private:
    // vga options
    VgaMode m_vgaMode;
    bool m_localMeasures;
    bool m_globalMeasures;
    std::string m_radius;
};
