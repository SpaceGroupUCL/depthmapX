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
#include "salalib/linkutils.h"


TEST_CASE("Test linking - fully filled grid (no geometry)", "")
{
    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0
    Point2f bottomLeft(0,0);
    Point2f topRight(2,4);
    int fill_type = 0; // = QDepthmapView::FULLFILL

    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));
    spacePixel->m_region = QtRegion(bottomLeft, topRight);
    PointMap pointMap("Test PointMap");
    pointMap.setSpacePixel(spacePixel.get());
    pointMap.setGrid(spacing, offset);
    Point2f gridBottomLeft = pointMap.getRegion().bottom_left;
    Point2f midPoint(gridBottomLeft.x + spacing * (floor(pointMap.getCols() * 0.5) + 0.5),
                         gridBottomLeft.y + spacing * (floor(pointMap.getRows() * 0.5) + 0.5));
    pointMap.makePoints(midPoint, fill_type);

    std::vector<Line> mergeLines;

    SECTION ("Successful: bottom-left to top-right")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0].a.x == 0);
        REQUIRE(links[0].a.y == 0);
        REQUIRE(links[0].b.x == 4);
        REQUIRE(links[0].b.y == 8);

        // make sure pixels are not already merged
        REQUIRE(!pointMap.isPixelMerged(links[0].a));
        REQUIRE(!pointMap.isPixelMerged(links[0].b));

        // merge
        depthmapX::mergePixelPairs(links, pointMap);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(links[0].a));
        REQUIRE(pointMap.isPixelMerged(links[0].b));

        const std::vector<std::pair<PixelRef, PixelRef>> &mergedPixelPairs = pointMap.getMergedPixelPairs();

        REQUIRE(mergedPixelPairs.size() == 1);
        REQUIRE(mergedPixelPairs[0].first.x == 0);
        REQUIRE(mergedPixelPairs[0].first.y == 0);
        REQUIRE(mergedPixelPairs[0].second.x == 4);
        REQUIRE(mergedPixelPairs[0].second.y == 8);

        const std::vector<SimpleLine> &mergeLines = depthmapX::getMergedPixelsAsLines(pointMap);

        Point2f p1position = pointMap.depixelate(links[0].a);
        Point2f p2position = pointMap.depixelate(links[0].b);

        REQUIRE(mergeLines.size() == 1);
        REQUIRE(mergeLines[0].start().x == p1position.x);
        REQUIRE(mergeLines[0].start().y == p1position.y);
        REQUIRE(mergeLines[0].end().x == p2position.x);
        REQUIRE(mergeLines[0].end().y == p2position.y);
    }

    SECTION ("Successfull: bottom-left to top-right and bottom-right to top-left")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        Point2f start(topRight.x, bottomLeft.y);
        Point2f end(bottomLeft.x, topRight.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE(links.size() == 2);
        REQUIRE(links[0].a.x == 0);
        REQUIRE(links[0].a.y == 0);
        REQUIRE(links[0].b.x == 4);
        REQUIRE(links[0].b.y == 8);
        REQUIRE(links[1].a.x == 0);
        REQUIRE(links[1].a.y == 8);
        REQUIRE(links[1].b.x == 4);
        REQUIRE(links[1].b.y == 0);

        // make sure pixels are not already merged
        REQUIRE(!pointMap.isPixelMerged(links[0].a));
        REQUIRE(!pointMap.isPixelMerged(links[0].b));
        REQUIRE(!pointMap.isPixelMerged(links[1].a));
        REQUIRE(!pointMap.isPixelMerged(links[1].b));

        // merge
        depthmapX::mergePixelPairs(links, pointMap);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(links[0].a));
        REQUIRE(pointMap.isPixelMerged(links[0].b));
        REQUIRE(pointMap.isPixelMerged(links[1].a));
        REQUIRE(pointMap.isPixelMerged(links[1].b));
    }

    SECTION ("Failing: merge line start out of grid")
    {
        Point2f start(bottomLeft.x - spacing, bottomLeft.y - spacing);
        mergeLines.push_back(Line(start,topRight));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Line ends not both on painted analysis space"));
    }

    SECTION ("Failing: merge line end out of grid")
    {
        Point2f end(topRight.x + spacing, topRight.y + spacing);
        mergeLines.push_back(Line(bottomLeft,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Line ends not both on painted analysis space"));
    }

    SECTION ("Failing: second link start overlapping")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        Point2f start(bottomLeft.x, bottomLeft.y);
        Point2f end(topRight.x - 1, topRight.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Overlapping link found"));
    }

    SECTION("Failing: second link end overlapping")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        Point2f start(bottomLeft.x + 1, bottomLeft.y);
        Point2f end(topRight.x, topRight.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Overlapping link found"));
    }

    SECTION("Failing: fully overlapping link (bottom-left to top-right)")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        mergeLines.push_back(Line(bottomLeft,topRight));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Overlapping link found"));
    }

    SECTION("Failing: link overlapping to previously merged")
    {
        mergeLines.push_back(Line(bottomLeft,topRight));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0].a.x == 0);
        REQUIRE(links[0].a.y == 0);
        REQUIRE(links[0].b.x == 4);
        REQUIRE(links[0].b.y == 8);

        // make sure pixels are not already merged
        REQUIRE(!pointMap.isPixelMerged(links[0].a));
        REQUIRE(!pointMap.isPixelMerged(links[0].b));

        // merge
        depthmapX::mergePixelPairs(links, pointMap);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(links[0].a));
        REQUIRE(pointMap.isPixelMerged(links[0].b));

        // now try to merge the same link again
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Link pixel found that is already linked on the map"));
    }
}

TEST_CASE("Test linking - half filled grid", "")
{

    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0
    int fill_type = 0; // = QDepthmapView::FULLFILL

    Point2f lineStart(0,0);
    Point2f lineEnd(2,4);

    Point2f bottomLeft(std::min(lineStart.x,lineEnd.x),std::min(lineStart.y,lineEnd.y));
    Point2f topRight(std::max(lineStart.x,lineEnd.x),std::max(lineStart.y,lineEnd.y));

    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));
    spacePixel->m_spacePixels.emplace_back("Test SpacePixelGroup");
    spacePixel->m_spacePixels.back().m_spacePixels.emplace_back("Test ShapeMap");
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(lineStart, lineEnd));
    spacePixel->m_spacePixels.back().m_region = spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion();
    spacePixel->m_region = spacePixel->m_spacePixels.back().m_region;

    PointMap pointMap("Test PointMap");
    pointMap.setSpacePixel(spacePixel.get());
    pointMap.setGrid(spacing, offset);

    Point2f gridBottomLeft = pointMap.getRegion().bottom_left;
    Point2f gridTopRight = pointMap.getRegion().top_right;
    Point2f topLeftFillPoint(gridBottomLeft.x+spacing, gridTopRight.y-spacing);
    pointMap.makePoints(topLeftFillPoint, fill_type);

    std::vector<Line> mergeLines;

    SECTION("Successful: top-left pixel to one to its right")
    {
        Point2f start(bottomLeft.x, topRight.y);
        Point2f end(bottomLeft.x + spacing, topRight.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0].a.x == 0);
        REQUIRE(links[0].a.y == 8);
        REQUIRE(links[0].b.x == 1);
        REQUIRE(links[0].b.y == 8);

        // make sure pixels are not already merged
        REQUIRE(!pointMap.isPixelMerged(links[0].a));
        REQUIRE(!pointMap.isPixelMerged(links[0].b));

        // merge
        depthmapX::mergePixelPairs(links, pointMap);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(links[0].a));
        REQUIRE(pointMap.isPixelMerged(links[0].b));
    }

    SECTION("Failing: merge line (bottom-right to the one its left) completely out of grid")
    {
        Point2f start(topRight.x, bottomLeft.y);
        Point2f end(topRight.x - 1, bottomLeft.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Line ends not both on painted analysis space"));
    }

    SECTION("Failing: merge line (bottom-right to top-left) start out of grid")
    {
        Point2f start(topRight.x, bottomLeft.y);
        Point2f end(bottomLeft.x, topRight.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Line ends not both on painted analysis space"));
    }

    SECTION("Failing: merge line (top-left to bottom-right) end out of grid")
    {
        Point2f start(bottomLeft.x, topRight.y);
        Point2f end(topRight.x, bottomLeft.y);
        mergeLines.push_back(Line(start,end));
        std::vector<PixelRefPair> links = depthmapX::pixelateMergeLines(mergeLines,pointMap);
        REQUIRE_THROWS_WITH(depthmapX::mergePixelPairs(links, pointMap),
                            Catch::Contains("Line ends not both on painted analysis space"));
    }
}
