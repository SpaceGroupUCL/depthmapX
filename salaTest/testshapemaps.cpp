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

#include "../genlib/p2dpoly.h"
#include "../salalib/mgraph.h"
#include "../salalib/shapemap.h"
#include "../salalib/axialmap.h"
#include "catch.hpp"
#include <sstream>
#include <iostream>
#include <cmath>

TEST_CASE("Testing ShapeMap::getAllShapes variants")
{
    const float EPSILON = 0.001;
    Point2f line0Start(0,1);
    Point2f line0End  (3,2);
    Point2f line1Start(1,1);
    Point2f line1End  (1,-1);

    std::unique_ptr<ShapeMap> shapeMap(new ShapeMap("Test ShapeMap"));

    shapeMap->makeLineShape(Line(line0Start, line0End));
    shapeMap->makeLineShape(Line(line1Start, line1End));

    std::vector<Point2f> polyVertices;
    polyVertices.push_back(Point2f(-1,-1));
    polyVertices.push_back(Point2f( 2,-1));
    polyVertices.push_back(Point2f( 0, 0));

    shapeMap->makePolyShape(polyVertices, false, false);

    SECTION("ShapeMap::getAllShapesAsLines")
    {
        std::vector<SimpleLine> lines = shapeMap->getAllShapesAsLines();

        REQUIRE(lines.size() == 5);

        REQUIRE(lines[0].start().x == Approx(line0Start.x).epsilon(EPSILON));
        REQUIRE(lines[0].start().y == Approx(line0Start.y).epsilon(EPSILON));
        REQUIRE(lines[0].end().x == Approx(line0End.x).epsilon(EPSILON));
        REQUIRE(lines[0].end().y == Approx(line0End.y).epsilon(EPSILON));

        REQUIRE(lines[1].start().x == Approx(line1Start.x).epsilon(EPSILON));
        REQUIRE(lines[1].start().y == Approx(line1Start.y).epsilon(EPSILON));
        REQUIRE(lines[1].end().x == Approx(line1End.x).epsilon(EPSILON));
        REQUIRE(lines[1].end().y == Approx(line1End.y).epsilon(EPSILON));

        REQUIRE(lines[2].start().x == Approx(polyVertices[0].x).epsilon(EPSILON));
        REQUIRE(lines[2].start().y == Approx(polyVertices[0].y).epsilon(EPSILON));
        REQUIRE(lines[2].end().x == Approx(polyVertices[1].x).epsilon(EPSILON));
        REQUIRE(lines[2].end().y == Approx(polyVertices[1].y).epsilon(EPSILON));

        REQUIRE(lines[3].start().x == Approx(polyVertices[1].x).epsilon(EPSILON));
        REQUIRE(lines[3].start().y == Approx(polyVertices[1].y).epsilon(EPSILON));
        REQUIRE(lines[3].end().x == Approx(polyVertices[2].x).epsilon(EPSILON));
        REQUIRE(lines[3].end().y == Approx(polyVertices[2].y).epsilon(EPSILON));

        REQUIRE(lines[4].start().x == Approx(polyVertices[2].x).epsilon(EPSILON));
        REQUIRE(lines[4].start().y == Approx(polyVertices[2].y).epsilon(EPSILON));
        REQUIRE(lines[4].end().x == Approx(polyVertices[0].x).epsilon(EPSILON));
        REQUIRE(lines[4].end().y == Approx(polyVertices[0].y).epsilon(EPSILON));
    }
    SECTION("ShapeMap::getAllLinesWithColour")
    {
        shapeMap->overrideDisplayedAttribute(-2);
        shapeMap->setDisplayedAttribute(-1); // displayed attribute is shape_ref

        std::vector<std::pair<SimpleLine, PafColor>> colouredLines = shapeMap->getAllLinesWithColour();

        REQUIRE(colouredLines.size() == 2);

        REQUIRE(colouredLines[0].first.start().x == Approx(std::min(line0Start.x, line0End.x)).epsilon(EPSILON));
        REQUIRE(colouredLines[0].first.start().y == Approx(std::min(line0Start.y, line0End.y)).epsilon(EPSILON));
        REQUIRE(colouredLines[0].first.end().x == Approx(std::max(line0Start.x, line0End.x)).epsilon(EPSILON));
        REQUIRE(colouredLines[0].first.end().y == Approx(std::max(line0Start.y, line0End.y)).epsilon(EPSILON));
        REQUIRE(colouredLines[0].second.redf() == Approx(0.2f).epsilon(EPSILON));
        REQUIRE(colouredLines[0].second.greenf() == Approx(0.2f).epsilon(EPSILON));
        REQUIRE(colouredLines[0].second.bluef() == Approx(0.86667f).epsilon(EPSILON));

        REQUIRE(colouredLines[1].first.start().x == Approx(line1Start.x).epsilon(EPSILON));
        REQUIRE(colouredLines[1].first.start().y == Approx(line1Start.y).epsilon(EPSILON));
        REQUIRE(colouredLines[1].first.end().x == Approx(line1End.x).epsilon(EPSILON));
        REQUIRE(colouredLines[1].first.end().y == Approx(line1End.y).epsilon(EPSILON));
        REQUIRE(colouredLines[1].second.redf() == Approx(0.13333f).epsilon(EPSILON));
        REQUIRE(colouredLines[1].second.greenf() == Approx(0.86667f).epsilon(EPSILON));
        REQUIRE(colouredLines[1].second.bluef() == Approx(0.53333f).epsilon(EPSILON));
    }
    SECTION("ShapeMap::getAllPolygonsWithColour")
    {
        shapeMap->overrideDisplayedAttribute(-2);
        shapeMap->setDisplayedAttribute(-1); // displayed attribute is shape_ref

        std::vector<std::pair<std::vector<Point2f>, PafColor>> colouredPolygons = shapeMap->getAllPolygonsWithColour();

        REQUIRE(colouredPolygons.size() == 1);

        auto iter = colouredPolygons.begin();
        const std::vector<Point2f> vertices = iter->first;
        const PafColor colour = iter->second;

        REQUIRE(vertices[0].x == Approx(polyVertices[0].x).epsilon(EPSILON));
        REQUIRE(vertices[0].y == Approx(polyVertices[0].y).epsilon(EPSILON));
        REQUIRE(vertices[1].x == Approx(polyVertices[1].x).epsilon(EPSILON));
        REQUIRE(vertices[1].y == Approx(polyVertices[1].y).epsilon(EPSILON));
        REQUIRE(vertices[2].x == Approx(polyVertices[2].x).epsilon(EPSILON));
        REQUIRE(vertices[2].y == Approx(polyVertices[2].y).epsilon(EPSILON));
        REQUIRE(colour.redf() == Approx(0.86667f).epsilon(EPSILON));
        REQUIRE(colour.greenf() == Approx(0.2f).epsilon(EPSILON));
        REQUIRE(colour.bluef() == Approx(0.2f).epsilon(EPSILON));
    }
}

TEST_CASE("SalaShape handles empty and degenerate polylines") {
    SECTION("Empty shape") {
        SalaShape shape(SalaShape::SHAPE_POLY);
        shape.setCentroidAreaPerim();
        REQUIRE(shape.getArea() == Approx(0.0));
        REQUIRE(shape.getPerimeter() == Approx(0.0));
        REQUIRE(std::isfinite(shape.getCentroid().x));
        REQUIRE(std::isfinite(shape.getCentroid().y));
        REQUIRE(shape.getAngDev() == Approx(0.0));
    }

    SECTION("Collinear points") {
        SalaShape shape(SalaShape::SHAPE_POLY);
        shape.m_points = {Point2f(0.0, 0.0), Point2f(2.0, 0.0), Point2f(4.0, 0.0)};
        shape.setCentroidAreaPerim();
        REQUIRE(shape.getArea() == Approx(0.0));
        REQUIRE(shape.getCentroid().x == Approx(2.0));
        REQUIRE(shape.getCentroid().y == Approx(0.0));
    }
}

TEST_CASE("ShapeMap rejects stale polygon edit references") {
    ShapeMap shapeMap("Test ShapeMap");
    const int shapeRef = shapeMap.polyBegin(Line(Point2f(0.0, 0.0), Point2f(1.0, 0.0)));

    REQUIRE_FALSE(shapeMap.polyAppend(shapeRef + 1000, Point2f(2.0, 0.0)));
    REQUIRE_FALSE(shapeMap.polyClose(shapeRef + 1000));
    REQUIRE_FALSE(shapeMap.polyCancel(shapeRef + 1000));
    REQUIRE(shapeMap.getAllShapes().size() == 1);

    shapeMap.removeShape(shapeRef + 1000, false);
    REQUIRE(shapeMap.getAllShapes().size() == 1);
}
