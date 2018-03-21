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
#include "../depthmapX/gllines.h"

TEST_CASE("Test GLLines::loadLineData()", "")
{
    Point2f line1Start(0,0);
    Point2f line1End(2,4);
    PafColor line1colour(255,0,0);
    Point2f line2Start(1,1);
    Point2f line2End(3,5);
    PafColor line2colour(0,255,0);

    std::vector<std::pair<SimpleLine, PafColor>> colouredLines;

    std::pair<SimpleLine, PafColor> colouredLine1 = std::pair<SimpleLine, PafColor> (SimpleLine(line1Start, line1End), line1colour);
    colouredLines.push_back(colouredLine1);

    std::pair<SimpleLine, PafColor> colouredLine2 = std::pair<SimpleLine, PafColor> (SimpleLine(line2Start, line2End), line2colour);
    colouredLines.push_back(colouredLine2);

    GLLines gllines;
    gllines.loadLineData(colouredLines);

    REQUIRE(gllines.vertexCount() == 4);
}
