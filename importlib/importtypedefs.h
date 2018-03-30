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

#include <vector>
#include <map>

namespace depthmapX {
    typedef std::vector<std::string> ColumnData;
    typedef std::map<std::string, ColumnData> Table;

    class Polyline : public QtRegion
    {
    public:
        std::vector<Point2f> m_vertices;
        bool m_closed = false;
        Polyline(std::vector<Point2f> vertices, bool closed) : m_vertices(vertices), m_closed(closed) {
        }
    };

    enum ImportType {
        DRAWINGMAP, DATAMAP
    };

    enum ImportFileType {
        CSV, TSV, DXF
    };
}
