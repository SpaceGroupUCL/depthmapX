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

#ifndef ENTITYPARSING_H
#define ENTITYPARSING_H

#include "genlib/p2dpoly.h"
#include "genlib/exceptions.h"
#include "isovistdef.h"
#include <vector>
#include <iostream>
#include <string>

namespace EntityParsing {

    class EntityParseException : public depthmapX::BaseException
    {
    public:
        EntityParseException(std::string message) : depthmapX::BaseException(message)
        {}
    };

    std::vector<std::string> split(const std::string &s, char delim);
    std::vector<Line> parseLines(std::istream& stream, char delimiter);
    std::vector<Point2f> parsePoints(std::istream& stream, char delimiter);
    std::pair<std::vector<std::string>, std::vector<Point2f>> parsePointSets(std::istream& stream, char delimiter = '\t');
    Point2f parsePoint(const std::string &point, char delimiter = ',');
    std::vector<IsovistDefinition> parseIsovists(std::istream &stream, char delimiter);
    IsovistDefinition parseIsovist(const std::string &isovist);
    std::vector<std::pair<int, int> > parseRefPairs(std::istream& stream, char delimiter);
}

#endif // ENTITYPARSING_H
