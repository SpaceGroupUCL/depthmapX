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
#include "salalib/mgraph.h"


TEST_CASE("Test getVisibleLines", "")
{
    const float EPSILON = 0.001;

    // create a new MetaGraph(SuperSpacePixel)
    std::unique_ptr<MetaGraph> mgraph(new MetaGraph());

    Point2f visibleLineStart(0,0);
    Point2f visibleLineEnd(2,4);
    Point2f hiddenLineStart(1,1);
    Point2f hiddenLineEnd(3,5);

    // push a SpacePixelFile in the MetaGraph
    mgraph->SuperSpacePixel::push_back(SpacePixelFile("Test SpacePixelFile"));

    // push a ShapeMap in the SpacePixelFile
    mgraph->SuperSpacePixel::tail().push_back(ShapeMap("Visible ShapeMap"));

    // add a line to the first ShapeMap
    mgraph->SuperSpacePixel::tail().tail().makeLineShape(Line(visibleLineStart, visibleLineEnd));

    // push a ShapeMap in the SpacePixelFile
    mgraph->SuperSpacePixel::tail().push_back(ShapeMap("Hidden ShapeMap"));

    // add a line to the second ShapeMap
    mgraph->SuperSpacePixel::tail().tail().makeLineShape(Line(hiddenLineStart, hiddenLineEnd));

    SECTION( "Get visible lines when none is hidden" )
    {
        // first check without hiding anything

        const std::vector<SimpleLine>& visibleLines = mgraph->getVisibleDrawingLines();

        REQUIRE(visibleLines.size() == 2);
        REQUIRE(visibleLines[0].start().x == Approx(visibleLineStart.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].start().y == Approx(visibleLineStart.y).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().x == Approx(visibleLineEnd.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().y == Approx(visibleLineEnd.y).epsilon(EPSILON));
        REQUIRE(visibleLines[1].start().x == Approx(hiddenLineStart.x).epsilon(EPSILON));
        REQUIRE(visibleLines[1].start().y == Approx(hiddenLineStart.y).epsilon(EPSILON));
        REQUIRE(visibleLines[1].end().x == Approx(hiddenLineEnd.x).epsilon(EPSILON));
        REQUIRE(visibleLines[1].end().y == Approx(hiddenLineEnd.y).epsilon(EPSILON));
    }

    SECTION( "Get visible lines when some are hidden" )
    {
        // now hide the second SpacePixelFile
        mgraph->SuperSpacePixel::tail().tail().setShow(false);

        const std::vector<SimpleLine>& visibleLines = mgraph->getVisibleDrawingLines();

        REQUIRE(visibleLines.size() == 1);
        REQUIRE(visibleLines[0].start().x == Approx(visibleLineStart.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].start().y == Approx(visibleLineStart.y).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().x == Approx(visibleLineEnd.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().y == Approx(visibleLineEnd.y).epsilon(EPSILON));
    }
}
