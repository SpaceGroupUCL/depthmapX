// Copyright (C) 2017 Christian Sailer

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

#include "viewhelpers.h"
#include "catch.hpp"
#include <time.h>
#include <sstream>
#include <iomanip>

TEST_CASE("Calculating the new center", "[calculateCenter]"){
    auto point = QPoint(100, 100);
    auto oldCenter = QPoint(200,200);
    auto newCenter = Point2f(150,150);

    REQUIRE(ViewHelpers::calculateCenter(point, oldCenter, 0.5) == newCenter);

    newCenter.x = 300;
    newCenter.y = 300;

    REQUIRE(ViewHelpers::calculateCenter(point, oldCenter, 2.0) == newCenter);

}

TEST_CASE("Date string format", "[getCurrentDate]"){
    auto now = time(NULL);
    const tm* ltime = localtime(&now);
    std::stringstream sstream;
    sstream << ltime->tm_year + 1900 << "/" <<
            std::setfill('0') << std::setw(2) << ltime->tm_mon + 1 << "/" <<
            std::setfill('0') << std::setw(2) << ltime->tm_mday << std::flush;

    REQUIRE(ViewHelpers::getCurrentDate() ==  sstream.str().c_str());
}

