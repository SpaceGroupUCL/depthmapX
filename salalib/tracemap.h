// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Petros Koutsolampros

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

#include "salalib/shapemap.h"

#include "genlib/p2dpoly.h"

class TraceMap : public ShapeMap {
  private:
    std::map<int, std::vector<double>> m_traceTimes;

  public:
    TraceMap(std::string name = std::string()) : ShapeMap(name, ShapeMap::TRACEMAP) {}
    int makeTraceWithRef(const std::vector<Event2f> &trace, int trace_ref,
                         const std::map<int, float> &extraAttributes = std::map<int,float>());
    int makeTrace(const std::vector<Event2f> &trace, const std::map<int, float> &extraAttributes = std::map<int,float>());
    bool read(std::istream& stream);
    bool write(std::ofstream &stream);
    void writeTracesToXMLFile(std::ofstream &stream);
};
