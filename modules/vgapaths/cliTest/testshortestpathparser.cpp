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

#include "cliTest/argumentholder.h"
#include "cliTest/selfcleaningfile.h"
#include "modules/vgapaths/cli/shortestpathparser.h"
#include <catch.hpp>

TEST_CASE("ShortestPathParser Fail", "Error cases") {
    SECTION("Missing argument to -spo") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-spo requires an argument"));
    }
    SECTION("Missing argument to -spd") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spd"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-spd requires an argument"));
    }
    SECTION("Missing argument to -spt") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spt"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-spt requires an argument"));
    }

    SECTION("rubbish input to -spo") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spo", "foo"};
        REQUIRE_THROWS_WITH(
            parser.parse(ah.argc(), ah.argv()),
            Catch::Contains("Invalid origin point provided (foo). Should only contain digits dots and commas"));
    }

    SECTION("rubbish input to -spd") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spd", "foo"};
        REQUIRE_THROWS_WITH(
            parser.parse(ah.argc(), ah.argv()),
            Catch::Contains("Invalid destination point provided (foo). Should only contain digits dots and commas"));
    }

    SECTION("Invalid Shortest Path type") {
        ShortestPathParser parser;
        ArgumentHolder ah{"prog", "-spt", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "Invalid Shortest Path type: foo");
    }
}

TEST_CASE("ShortestPathParser Success", "Read successfully") {
    ShortestPathParser parser;
    double x1 = 1.0;
    double y1 = 2.0;
    double x2 = 1.1;
    double y2 = 1.2;
    std::string type = "metric";

    std::stringstream p1;
    p1 << x1 << "," << y1 << std::flush;
    std::stringstream p2;
    p2 << x2 << "," << y2 << std::flush;

    ArgumentHolder ah{"prog", "-spo", p1.str(), "-spd", p2.str(), "-spt", type};
    parser.parse(ah.argc(), ah.argv());

    Point2f origin = *parser.getOrigins().begin();
    Point2f destination = *parser.getDestinations().begin();
    ShortestPathParser::ShortestPathType shortestPathType = parser.getShortestPathType();
    REQUIRE(origin.x == Approx(x1));
    REQUIRE(origin.y == Approx(y1));
    REQUIRE(destination.x == Approx(x2));
    REQUIRE(destination.y == Approx(y2));
    REQUIRE(shortestPathType == ShortestPathParser::ShortestPathType::METRIC);
}
