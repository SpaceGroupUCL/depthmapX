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

#include "catch.hpp"
#include "../depthmapXcli/parsingutils.h"

TEST_CASE("AxialRadiusParsing success")
{
    std::string testString = "5,1,n";
    auto result = depthmapX::parseRadiusList(testString);
    std::vector<double> expectedResult = {1.0 ,5.0,-1.0};
    REQUIRE( result == expectedResult );
}

TEST_CASE("AxialRadiusParsing failure")
{
    REQUIRE_THROWS_WITH(depthmapX::parseRadiusList("5,1.1"), Catch::Contains("Found non integer radius 1.1"));
    REQUIRE_THROWS_WITH(depthmapX::parseRadiusList("5,foo"), Catch::Contains("Found either 0 or unparsable radius foo"));
    REQUIRE_THROWS_WITH(depthmapX::parseRadiusList("5,0"), Catch::Contains("Found either 0 or unparsable radius 0"));
    REQUIRE_THROWS_WITH(depthmapX::parseRadiusList("5,-1"), Catch::Contains("Radius must be either n or a positive integer"));
}

