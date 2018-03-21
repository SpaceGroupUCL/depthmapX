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

#include "genlib/p2dpoly.h"
#include <vector>

class GeometryGenerators
{
public:
    static std::vector<Point2f> generateDiskTriangles(int sides, float radius, Point2f position = Point2f(0,0));
    static std::vector<Point2f> generateMultipleDiskTriangles(int sides, float radius, std::vector<Point2f> positions);

    static std::vector<SimpleLine> generateCircleLines(int sides, float radius, Point2f position = Point2f(0,0));
    static std::vector<SimpleLine> generateMultipleCircleLines(int sides, float radius, std::vector<Point2f> positions);
};
