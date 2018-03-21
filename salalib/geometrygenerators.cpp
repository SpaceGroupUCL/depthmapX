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

#include "geometrygenerators.h"

std::vector<Point2f> GeometryGenerators::generateDiskTriangles(int sides, float radius, Point2f position) {
    std::vector<Point2f> diskTriangles;
    for(int i = 0; i < sides; i++) {
        diskTriangles.push_back(Point2f(position.x, position.y));
        diskTriangles.push_back(Point2f(position.x + radius*sin(2*M_PI*(i+1)/sides), position.y + radius*cos(2*M_PI*(i+1)/sides)));
        diskTriangles.push_back(Point2f(position.x + radius*sin(2*M_PI*i/sides), position.y + radius*cos(2*M_PI*i/sides)));
    }
    return diskTriangles;
}

std::vector<Point2f> GeometryGenerators::generateMultipleDiskTriangles(int sides, float radius, std::vector<Point2f> positions) {
    std::vector<Point2f> diskTriangles = generateDiskTriangles(sides , radius);

    std::vector<Point2f> mulitpleDiskTriangles;

    std::vector<Point2f>::const_iterator iter = positions.begin(), end =
    positions.end();
    for ( ; iter != end; ++iter )
    {
        Point2f position = *iter;
        std::vector<Point2f>::const_iterator iterDiskVertices = diskTriangles.begin(), endDiskPoints =
        diskTriangles.end();
        for ( ; iterDiskVertices != endDiskPoints; ++iterDiskVertices )
        {
            Point2f vertex = *iterDiskVertices;
            mulitpleDiskTriangles.push_back(Point2f(position.x + vertex.x,position.y + vertex.y));
        }
    }
    return mulitpleDiskTriangles;
}

std::vector<SimpleLine> GeometryGenerators::generateCircleLines(int sides, float radius, Point2f position) {
    std::vector<SimpleLine> cirleLines;
    for(int i = 0; i < sides; i++) {
        cirleLines.push_back(SimpleLine(
                    Point2f(position.x + radius*sin(2*M_PI*(i+1)/sides), position.y + radius*cos(2*M_PI*(i+1)/sides)),
                    Point2f(position.x + radius*sin(2*M_PI*i/sides), position.y + radius*cos(2*M_PI*i/sides))
                    ));
    }
    return cirleLines;
}

std::vector<SimpleLine> GeometryGenerators::generateMultipleCircleLines(int sides, float radius, std::vector<Point2f> positions) {
    std::vector<SimpleLine> circleLines = generateCircleLines(sides , radius);

    std::vector<SimpleLine> mulitpleCircleLines;

    std::vector<Point2f>::const_iterator iter = positions.begin(), end =
    positions.end();
    for ( ; iter != end; ++iter )
    {
        Point2f position = *iter;
        std::vector<SimpleLine>::const_iterator iterCircleLines = circleLines.begin(), endCircleLines =
        circleLines.end();
        for ( ; iterCircleLines != endCircleLines; ++iterCircleLines )
        {
            SimpleLine line = *iterCircleLines;
            mulitpleCircleLines.push_back(SimpleLine(
                                                Point2f(position.x + line.start().x, position.y + line.start().y),
                                                Point2f(position.x + line.end().x, position.y + line.end().y)
                                                ));
        }
    }
    return mulitpleCircleLines;
}
