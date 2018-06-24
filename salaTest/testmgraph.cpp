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
    mgraph->m_spacePixels.emplace_back("Test SpacePixelFile");

    // push a ShapeMap in the SpacePixelFile
    mgraph->m_spacePixels.back().m_spacePixels.emplace_back("Visible ShapeMap");

    // add a line to the first ShapeMap
    mgraph->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(visibleLineStart, visibleLineEnd));

    // push a ShapeMap in the SpacePixelFile
    mgraph->m_spacePixels.back().m_spacePixels.emplace_back("Hidden ShapeMap");

    // add a line to the second ShapeMap
    mgraph->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(hiddenLineStart, hiddenLineEnd));

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
        mgraph->m_spacePixels.back().m_spacePixels.back().setShow(false);

        const std::vector<SimpleLine>& visibleLines = mgraph->getVisibleDrawingLines();

        REQUIRE(visibleLines.size() == 1);
        REQUIRE(visibleLines[0].start().x == Approx(visibleLineStart.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].start().y == Approx(visibleLineStart.y).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().x == Approx(visibleLineEnd.x).epsilon(EPSILON));
        REQUIRE(visibleLines[0].end().y == Approx(visibleLineEnd.y).epsilon(EPSILON));
    }
}

TEST_CASE("Test pointMaps", "")
{
    std::unique_ptr<MetaGraph> mgraph(new MetaGraph());
    int pointMapIdx = mgraph->addNewPointMap("Kenny");
    REQUIRE(mgraph->getPointMaps().size() == 1);
    REQUIRE(pointMapIdx == 0);
    REQUIRE(mgraph->getPointMaps()[0].getName() == "Kenny");
    REQUIRE(mgraph->getDisplayedPointMapRef() == pointMapIdx);
    REQUIRE(mgraph->getDisplayedPointMap().getName() == "Kenny");

    SECTION( "Add another and remove the first through the MetaGraph" )
    {
        int pointMapIdx = mgraph->addNewPointMap("Stan");
        REQUIRE(mgraph->getPointMaps().size() == 2);
        REQUIRE(pointMapIdx == 1);
        REQUIRE(mgraph->getPointMaps()[1].getName() == "Stan");
        REQUIRE(mgraph->getDisplayedPointMapRef() == 1);
        REQUIRE(mgraph->getDisplayedPointMap().getName() == "Stan");

        mgraph->setState(MetaGraph::POINTMAPS);
        mgraph->setViewClass(MetaGraph::SHOWVGATOP);
        mgraph->setDisplayedPointMapRef(0);
        REQUIRE(mgraph->getDisplayedPointMapRef() == 0);
        REQUIRE(mgraph->getDisplayedPointMap().getName() == "Kenny");

        mgraph->removeDisplayedMap();
        REQUIRE(mgraph->getPointMaps().size() == 1);
        REQUIRE(mgraph->getPointMaps()[0].getName() == "Stan");
        REQUIRE(mgraph->getDisplayedPointMapRef() == 0);
    }
}
