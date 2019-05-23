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

#include "salalib/importtypedefs.h"
#include "salalib/mgraph.h"
#include "salalib/parsers/dxfp.h"
#include <map>
#include <vector>

namespace depthmapX {
    bool importFile(MetaGraph &mgraph, std::istream &stream, Communicator *communicator, std::string name,
                    ImportType mapType, ImportFileType fileType);
    bool importTxt(ShapeMap &shapeMap, std::istream &stream, char delimiter);
    depthmapX::Table csvToTable(std::istream &stream, char delimiter);
    std::vector<Line> extractLines(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col, ColumnData &y2col);
    std::map<int, Line> extractLinesWithRef(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col, ColumnData &y2col,
                                            ColumnData &refcol);
    std::vector<Point2f> extractPoints(ColumnData &x, ColumnData &y);
    std::map<int, Point2f> extractPointsWithRefs(ColumnData &x, ColumnData &y, ColumnData &ref);
    bool importDxfLayer(const DxfLayer &dxfLayer, ShapeMap &shapeMap);
    bool importAttributes(AttributeTable &attributes, std::istream &stream, char delimiter);
} // namespace depthmapX
