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

#include "catch.hpp"
#include "../depthmapX/gllinesuniform.h"

TEST_CASE("Test GLLinesUniform::loadLineData()", "")
{
    Point2f line1Start(0,0);
    Point2f line1End(2,4);

    Point2f line2Start(1,1);
    Point2f line2End(3,5);

    std::vector<SimpleLine> lines;

    SimpleLine line1 = SimpleLine(line1Start, line1End);
    lines.push_back(line1);

    SimpleLine line2 = SimpleLine(line2Start, line2End);
    lines.push_back(line2);

    QRgb lineColour = qRgb(255,0,0);

    GLLinesUniform gllinesuniform;
    gllinesuniform.loadLineData(lines, lineColour);

    REQUIRE(gllinesuniform.vertexCount() == 4);
}
