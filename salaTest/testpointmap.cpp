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


TEST_CASE("Test SuperSpacePixel construction", "")
{
    const float EPSILON = 0.001;
    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0

    // create a new SuperSpacePixel
    // The PointMap needs the m_region variable from this
    // object as a definition of the area the grid needs to cover
    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));

    SECTION( "Construct a plain SuperSpacePixel without underlying geometry" )
    {
        Point2f bottomLeft(0,0);
        Point2f topRight(2,4);

        // set m_region to the bounds
        spacePixel->m_region = QtRegion(bottomLeft, topRight);

        // check if the bounds are set correctly
        REQUIRE(spacePixel->m_region.bottom_left.x == Approx(bottomLeft.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.bottom_left.y == Approx(bottomLeft.y).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.x == Approx(topRight.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.y == Approx(topRight.y).epsilon(EPSILON));
    }

    SECTION( "Construct a SuperSpacePixel using underlying geometry" )
    {
        Point2f lineStart(0,0);
        Point2f lineEnd(2,4);

        Point2f bottomLeft(std::min(lineStart.x,lineEnd.x),std::min(lineStart.y,lineEnd.y));
        Point2f topRight(std::max(lineStart.x,lineEnd.x),std::max(lineStart.y,lineEnd.y));

        // push a SpacePixelFile in the SuperSpacePixel
        spacePixel->m_spacePixels.emplace_back("Test SpacePixelGroup");

        // push a ShapeMap in the SpacePixelFile
        spacePixel->m_spacePixels.back().m_spacePixels.emplace_back("Test ShapeMap");

        // add a line to the ShapeMap
        spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(lineStart, lineEnd));

        // check if the ShapeMap bounds are set correctly
        REQUIRE(spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion().bottom_left.x == Approx(bottomLeft.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion().bottom_left.y == Approx(bottomLeft.y).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion().top_right.x == Approx(topRight.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion().top_right.y == Approx(topRight.y).epsilon(EPSILON));

        // SpacePixelGroup (and thus SuperSpacePixel and SpacePixelFile)
        // does not automatically grow its region when a new shapemap/file
        // is added to it therefore we have to do this externally
        spacePixel->m_spacePixels.back().m_region = spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion();

        // check if the SpacePixelFile bounds are set correctly
        REQUIRE(spacePixel->m_spacePixels.back().m_region.bottom_left.x == Approx(bottomLeft.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_region.bottom_left.y == Approx(bottomLeft.y).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_region.top_right.x == Approx(topRight.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_spacePixels.back().m_region.top_right.y == Approx(topRight.y).epsilon(EPSILON));

        spacePixel->m_region = spacePixel->m_spacePixels.back().m_region;

        // check if the SuperSpacePixel bounds are set correctly
        REQUIRE(spacePixel->m_region.bottom_left.x == Approx(bottomLeft.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.bottom_left.y == Approx(bottomLeft.y).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.x == Approx(topRight.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.y == Approx(topRight.y).epsilon(EPSILON));
    }

    // construct a sample pointMap
    PointMap pointMap("Test PointMap");

    // assign the spacePixel
    bool spacePixelSet = pointMap.setSpacePixel(spacePixel.get());

    // check if the spacePixel was set
    REQUIRE(spacePixelSet);
}

TEST_CASE("Test grid filling", "")
{
    const float EPSILON = 0.001;
    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0

    // create a new SuperSpacePixel
    // The PointMap needs the m_region variable from this
    // object as a definition of the area the grid needs to cover
    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));

    // Construct a plain SuperSpacePixel without underlying geometry
    {
        Point2f bottomLeft(0,0);
        Point2f topRight(2,4);

        // set m_region to the bounds
        spacePixel->m_region = QtRegion(bottomLeft, topRight);

        // check if the bounds are set correctly
        REQUIRE(spacePixel->m_region.bottom_left.x == Approx(bottomLeft.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.bottom_left.y == Approx(bottomLeft.y).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.x == Approx(topRight.x).epsilon(EPSILON));
        REQUIRE(spacePixel->m_region.top_right.y == Approx(topRight.y).epsilon(EPSILON));
    }

    // construct a sample pointMap
    PointMap pointMap("Test PointMap");

    // assign the spacePixel
    bool spacePixelSet = pointMap.setSpacePixel(spacePixel.get());

    // check if the spacePixel was set
    REQUIRE(spacePixelSet);

    // set the grid

    // create the grid with bounds as set above
    bool gridIsSet = pointMap.setGrid(spacing, offset);

    // check if the grid was set
    REQUIRE(gridIsSet);

    // check if the spacing is correct
    REQUIRE(spacing == pointMap.getSpacing());

    // fill the grid

    // seems like fill_type is actually connected to the
    // QDepthmapView class which is a GUI class (depthmapView.h)
    // TODO Disentangle GUI enum from pointMap.makePoints
    int fill_type = 0; // = QDepthmapView::FULLFILL

    Point2f gridBottomLeft = pointMap.getRegion().bottom_left;

    SECTION( "Check if the points are made when fill selection in a cell" )
    {
        // Check if the points are made (grid filled) when
        // the selected position is certainly in a cell
        // This calculation should make the point directly
        // at the centre of a central cell
        Point2f midPoint(gridBottomLeft.x + spacing * (floor(pointMap.getCols() * 0.5) + 0.5),
                         gridBottomLeft.y + spacing * (floor(pointMap.getRows() * 0.5) + 0.5));
        bool pointsMade = pointMap.makePoints(midPoint, fill_type);
        REQUIRE(pointsMade);

    }

    SECTION("Check if the points are made when fill selection between cells")
    {
        // Check if the points are made (grid filled) when
        // the selected position is certainly between cells
        // This calculation should make the point directly
        // at the edge of a central cell
        Point2f midPoint(gridBottomLeft.x + spacing * (floor(pointMap.getCols() * 0.5)),
                         gridBottomLeft.y + spacing * (floor(pointMap.getRows() * 0.5)));
        bool pointsMade = pointMap.makePoints(midPoint, fill_type);
        REQUIRE(pointsMade);
    }
}

// PointMap::setGrid is quite convoluted with various parameters
// affecting the result, such as the limits of the region to be
// covered (bottomLeft, topRight), the spacing and the location
// of the plan in space. For example every grid created will be
// in relation to the origin (0,0), no matter where the region
// is and the current pixel can always be calculated as if the
// origin always falls in the centre of a cell.

TEST_CASE("Quirks in grid creation - Origin always at 0", "")
{

    double spacing = 0.5;
    const float EPSILON = 0.001;
    Point2f offset(0,0); // seems that this is always set to 0,0

    Point2f bottomLeft(0,0);
    Point2f topRight(0,0);

    SECTION ("Region from origin to positive x, positive y quadrant")
    {
        spacing = 0.5;
        bottomLeft.x = 0;
        bottomLeft.y = 0;
        topRight.x = 1;
        topRight.y = 1;
    }

    SECTION ("Region away from origin to positive x, positive y quadrant")
    {
        spacing = 0.5;
        bottomLeft.x = 1;
        bottomLeft.y = 1;
        topRight.x = 2;
        topRight.y = 2;
    }

    SECTION ("Region from origin to negative x, negative y quadrant")
    {
        spacing = 0.5;
        bottomLeft.x = -1;
        bottomLeft.y = -1;
        topRight.x = 0;
        topRight.y = 0;
    }

    SECTION ("Region in all quadrants")
    {
        spacing = 0.5;
        bottomLeft.x = -1;
        bottomLeft.y = -1;
        topRight.x = 1;
        topRight.y = 1;
    }

    SECTION ("Region in positive x, positive y quadrant, non-rectangular")
    {
        spacing = 0.5;
        bottomLeft.x = 1;
        bottomLeft.y = 2;
        topRight.x = 3;
        topRight.y = 4;
    }

    SECTION ("Region in positive x, positive y quadrant, floating-point limits")
    {
        spacing = 0.5;
        bottomLeft.x = 1.1;
        bottomLeft.y = 2.2;
        topRight.x = 3.3;
        topRight.y = 4.4;
    }

    SECTION ("Region in positive x, positive y quadrant, floating-point limits")
    {
        spacing = 0.5;
        bottomLeft.x = 0.1;
        bottomLeft.y = 0.2;
        topRight.x = 0.3;
        topRight.y = 0.4;
    }

    SECTION ("Region in negative x, negative y quadrant, floating-point limits")
    {
        spacing = 0.5;
        bottomLeft.x = -0.4;
        bottomLeft.y = -0.3;
        topRight.x = -0.2;
        topRight.y = -0.1;
    }

    SECTION ("Region in all quadrants, floating-point limits")
    {
        spacing = 0.5;
        bottomLeft.x = -1.1;
        bottomLeft.y = -2.2;
        topRight.x = 3.3;
        topRight.y = 4.4;
    }

    SECTION ("Region in all quadrants, floating-point limits, smaller spacing")
    {
        spacing = 0.25;
        bottomLeft.x = 1.1;
        bottomLeft.y = 2.2;
        topRight.x = 3.3;
        topRight.y = 4.4;
    }

    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));
    spacePixel->m_region = QtRegion(bottomLeft, topRight);
    PointMap pointMap("Test PointMap");
    bool spacePixelSet = pointMap.setSpacePixel(spacePixel.get());
    bool gridIsSet = pointMap.setGrid(spacing, offset);

    int bottomLeftPixelIndexX = int(floor(bottomLeft.x / spacing - 0.5)) + 1;
    int bottomLeftPixelIndexY = int(floor(bottomLeft.y / spacing - 0.5)) + 1;

    int topRightPixelIndexX = int(floor(topRight.x / spacing - 0.5)) + 1;
    int topRightPixelIndexY = int(floor(topRight.y / spacing - 0.5)) + 1;

    int numCellsX = topRightPixelIndexX - bottomLeftPixelIndexX + 1;
    int numCellsY = topRightPixelIndexY - bottomLeftPixelIndexY + 1;

    // check if the size of the grid is as expected
    REQUIRE(pointMap.getCols() == numCellsX);
    REQUIRE(pointMap.getRows() == numCellsY);

    Point2f gridBottomLeft(bottomLeftPixelIndexX * spacing - 0.5 * spacing,
                           bottomLeftPixelIndexY * spacing - 0.5 * spacing);

    // check if the bottom-left corner of the bottom-left pixel is as expected
    REQUIRE(pointMap.getRegion().bottom_left.x == Approx(gridBottomLeft.x).epsilon(EPSILON));
    REQUIRE(pointMap.getRegion().bottom_left.y == Approx(gridBottomLeft.y).epsilon(EPSILON));

    Point2f midPoint(gridBottomLeft.x + spacing * (floor(numCellsX * 0.5) + 0.5),
                      gridBottomLeft.y + spacing * (floor(numCellsY * 0.5) + 0.5));

    int fill_type = 0; // = QDepthmapView::FULLFILL

    bool pointsMade = pointMap.makePoints(midPoint, fill_type);

    // check if the grid is filled
    REQUIRE(pointsMade);
}

TEST_CASE("PointMap copy, assign and add to vector")
{
    const double EPSILON = 0.001;
    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0

    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));

    double rectSize = 1.5;

    Point2f line0Start(0,0);
    Point2f line0End(0,rectSize);
    Point2f line1Start(0,rectSize);
    Point2f line1End(rectSize,rectSize);
    Point2f line2Start(rectSize,rectSize);
    Point2f line2End(rectSize,0);
    Point2f line3Start(rectSize,0);
    Point2f line3End(0,0);

    spacePixel->m_spacePixels.emplace_back("Test SpacePixelGroup");
    spacePixel->m_spacePixels.back().m_spacePixels.emplace_back("Test ShapeMap");
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line0Start, line0End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line1Start, line1End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line2Start, line2End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line3Start, line3End));
    spacePixel->m_spacePixels.back().m_region = spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion();
    spacePixel->m_region = spacePixel->m_spacePixels.back().m_region;
    PointMap pointMap("Test PointMap");
    bool spacePixelSet = pointMap.setSpacePixel(spacePixel.get());



    Point2f gridBottomLeft = pointMap.getRegion().bottom_left;

    Point2f midPoint(gridBottomLeft.x + spacing * (floor(pointMap.getCols() * 0.5) + 0.5),
                     gridBottomLeft.y + spacing * (floor(pointMap.getRows() * 0.5) + 0.5));

    int fill_type = 0; // = QDepthmapView::FULLFILL
    bool gridIsSet = pointMap.setGrid(spacing, offset);

    bool pointsMade = pointMap.makePoints(midPoint, fill_type);

    bool boundaryGraph = false;
    double maxDist = -1;
    // a communicator is required in order to create the connections between the pixels
    std::unique_ptr<Communicator> comm(new ICommunicator());

    bool graphMade = pointMap.sparkGraph2(comm.get(), boundaryGraph, maxDist);

    REQUIRE(graphMade);

    std::vector<PointMap> pointmaps;
    pointmaps.push_back(pointMap);
    REQUIRE(pointmaps.size() == 1);

    PointMap assignedPointMap("Assigned PointMap");
    REQUIRE(assignedPointMap.getName() == "Assigned PointMap");

    assignedPointMap = pointMap;
    REQUIRE(assignedPointMap.getName() == pointMap.getName());
    pointmaps.push_back(assignedPointMap);
    REQUIRE(pointmaps.size() == 2);

    PointMap copiedPointMap(pointMap);
    REQUIRE(copiedPointMap.getName() == pointMap.getName());
    pointmaps.push_back(copiedPointMap);
    REQUIRE(pointmaps.size() == 3);
}

TEST_CASE("Test PointMap connections output", "")
{
    const float EPSILON = 0.001;
    double spacing = 0.5;
    Point2f offset(0,0); // seems that this is always set to 0,0

    std::unique_ptr<SuperSpacePixel> spacePixel(new SuperSpacePixel("Test SuperSpacePixel"));

    double rectSize = 1.5;

    Point2f line0Start(0,0);
    Point2f line0End(0,rectSize);
    Point2f line1Start(0,rectSize);
    Point2f line1End(rectSize,rectSize);
    Point2f line2Start(rectSize,rectSize);
    Point2f line2End(rectSize,0);
    Point2f line3Start(rectSize,0);
    Point2f line3End(0,0);

    spacePixel->m_spacePixels.emplace_back("Test SpacePixelGroup");
    spacePixel->m_spacePixels.back().m_spacePixels.emplace_back("Test ShapeMap");
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line0Start, line0End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line1Start, line1End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line2Start, line2End));
    spacePixel->m_spacePixels.back().m_spacePixels.back().makeLineShape(Line(line3Start, line3End));
    spacePixel->m_spacePixels.back().m_region = spacePixel->m_spacePixels.back().m_spacePixels.back().getRegion();
    spacePixel->m_region = spacePixel->m_spacePixels.back().m_region;
    PointMap pointMap("Test PointMap");
    bool spacePixelSet = pointMap.setSpacePixel(spacePixel.get());



    Point2f gridBottomLeft = pointMap.getRegion().bottom_left;

    Point2f midPoint(gridBottomLeft.x + spacing * (floor(pointMap.getCols() * 0.5) + 0.5),
                     gridBottomLeft.y + spacing * (floor(pointMap.getRows() * 0.5) + 0.5));

    int fill_type = 0; // = QDepthmapView::FULLFILL
    bool gridIsSet = pointMap.setGrid(spacing, offset);

    bool pointsMade = pointMap.makePoints(midPoint, fill_type);

    bool boundaryGraph = false;
    double maxDist = -1;
    // a communicator is required in order to create the connections between the pixels
    std::unique_ptr<Communicator> comm(new ICommunicator());

    bool graphMade = pointMap.sparkGraph2(comm.get(), boundaryGraph, maxDist);

    REQUIRE(graphMade);

    SECTION("PointMap::outputLinksAsCSV") {
        std::stringstream stream;
        pointMap.mergePixels(65537, 131074);
        pointMap.mergePixels(131073, 65538);
        pointMap.outputLinksAsCSV(stream);

        REQUIRE(stream.good());
        char line[1000];
        std::vector<std::string> lines;
        while( !stream.eof())
        {
            stream.getline(line, 1000);
            lines.push_back(line);
        }
        std::vector<std::string> expected{ "RefFrom,RefTo",
                                           "65537,131074",
                                           "65538,131073"};
        REQUIRE(lines == expected);
    }

    SECTION("PointMap::outputConnectionsAsCSV") {
        std::stringstream stream;
        pointMap.outputConnectionsAsCSV(stream);

        REQUIRE(stream.good());
        char line[1000];
        std::vector<std::string> lines;
        while( !stream.eof())
        {
            stream.getline(line, 1000);
            lines.push_back(line);
        }
        std::vector<std::string> expected{ "RefFrom,RefTo",
                                           "65537,131073", "65537,131074", "65537,65538",
                                           "65538,131074", "65538,131073", "131073,131074"};
        REQUIRE(lines == expected);
    }

    SECTION("PointMap::outputConnections") {
        std::stringstream stream;
        pointMap.outputConnections(stream);

        REQUIRE(stream.good());
        char line[1000];
        std::vector<std::string> lines;
        while( !stream.eof())
        {
            stream.getline(line, 1000);
            lines.push_back(line);
        }
        std::vector<std::string> expected{ "#graph v1.0",
                                           "node {",
                                           "  ref    65537",
                                           "  origin 0.5 0.5 0",
                                           "  connections [",
                                           "    131073,",
                                           "    131074,",
                                           "    65538,",
                                           "  ]",
                                           "}",
                                           "node {",
                                           "  ref    65538",
                                           "  origin 0.5 1 0",
                                           "  connections [",
                                           "    131074,",
                                           "    65537,",
                                           "    131073,",
                                           "  ]",
                                           "}",
                                           "node {",
                                           "  ref    131073",
                                           "  origin 1 0.5 0",
                                           "  connections [",
                                           "    131074,",
                                           "    65538,",
                                           "    65537,",
                                           "  ]",
                                           "}",
                                           "node {",
                                           "  ref    131074",
                                           "  origin 1 1 0",
                                           "  connections [",
                                           "    65538,",
                                           "    65537,",
                                           "    131073,",
                                           "  ]",
                                           "}",
                                           "" };
        REQUIRE(lines == expected);
    }

}
TEST_CASE("Direct pointmap linking - fully filled grid (no geometry)", "")
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

    PixelRef bottomLeftPixel = pointMap.pixelate(bottomLeft);
    PixelRef topRightPixel = pointMap.pixelate(topRight);

    // make sure pixels are not already merged
    REQUIRE(!pointMap.isPixelMerged(bottomLeftPixel));
    REQUIRE(!pointMap.isPixelMerged(topRightPixel));

    // merge
    pointMap.mergePixels(bottomLeftPixel, topRightPixel);

    // make sure pixels are merged
    REQUIRE(pointMap.isPixelMerged(bottomLeftPixel));
    REQUIRE(pointMap.isPixelMerged(topRightPixel));

    SECTION ("Make sure we get the correct number of merged pixel pairs")
    {
        const std::vector<std::pair<PixelRef, PixelRef>> &pixelPairs = pointMap.getMergedPixelPairs();
        REQUIRE(pixelPairs.size() == 1);
        REQUIRE(pixelPairs[0].first == bottomLeftPixel);
        REQUIRE(pixelPairs[0].second == topRightPixel);
    }

    SECTION ("Overwrite the pixelpair by re-merging the first pixel of the pair")
    {
        PixelRef aboveBottomLeftPixel = pointMap.pixelate(Point2f(bottomLeft.x, bottomLeft.y + 1));

        // merge
        pointMap.mergePixels(aboveBottomLeftPixel, topRightPixel);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(aboveBottomLeftPixel));
        REQUIRE(pointMap.isPixelMerged(topRightPixel));

        // and previous pixel is not merged any more
        REQUIRE(!pointMap.isPixelMerged(bottomLeftPixel));

        // make sure we get the correct number of merged pixel pairs
        const std::vector<std::pair<PixelRef, PixelRef>> &pixelPairs = pointMap.getMergedPixelPairs();
        REQUIRE(pixelPairs.size() == 1);
        REQUIRE(pixelPairs[0].first == aboveBottomLeftPixel);
        REQUIRE(pixelPairs[0].second == topRightPixel);
    }

    SECTION ("Overwrite the pixelpair by re-merging the second pixel of the pair")
    {
        PixelRef belowTopRightPixel = pointMap.pixelate(Point2f(topRight.x, topRight.y - 1));

        // merge
        pointMap.mergePixels(bottomLeftPixel, belowTopRightPixel);

        // make sure pixels are merged
        REQUIRE(pointMap.isPixelMerged(bottomLeftPixel));
        REQUIRE(pointMap.isPixelMerged(belowTopRightPixel));

        // and previous pixel is not merged any more
        REQUIRE(!pointMap.isPixelMerged(topRightPixel));

        // make sure we get the correct number of merged pixel pairs
        const std::vector<std::pair<PixelRef, PixelRef>> &pixelPairs2 = pointMap.getMergedPixelPairs();
        REQUIRE(pixelPairs2.size() == 1);
        REQUIRE(pixelPairs2[0].first == bottomLeftPixel);
        REQUIRE(pixelPairs2[0].second == belowTopRightPixel);
    }

    SECTION ("Merge the same pixel twice to erase the pair")
    {
        pointMap.mergePixels(bottomLeftPixel, bottomLeftPixel);

        // make sure no pixel is merged
        REQUIRE(!pointMap.isPixelMerged(bottomLeftPixel));
        REQUIRE(!pointMap.isPixelMerged(topRightPixel));

        // make sure we get the correct number of merged pixel pairs
        const std::vector<std::pair<PixelRef, PixelRef>> &pixelPairs3 = pointMap.getMergedPixelPairs();
        REQUIRE(pixelPairs3.size() == 0);
    }
}
