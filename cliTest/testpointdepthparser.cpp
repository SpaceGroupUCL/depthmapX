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

#include <catch.hpp>
#include "depthmapXcli/pointdepthparser.h"
#include "argumentholder.h"
#include "selfcleaningfile.h"

TEST_CASE("PointDepthParserFail", "Error cases")
{
    SECTION("Missing argument to -pdp")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdp"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-pdp requires an argument"));
    }

    SECTION("Missing argument to -pdf")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdf"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-pdf requires an argument"));
    }

    SECTION("Missing argument to -pdt")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdt"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-pdt requires an argument"));
    }

    SECTION("rubbish input to -pdp")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdp", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid origin point provided (foo). Should only contain digits dots and commas"));
    }

    SECTION("rubbish input to -pdt")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdt", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid point depth type: foo"));
    }

    SECTION("Non-existing file provided")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog", "-pdf", "foo.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Failed to load file foo.csv, error"));
    }

    SECTION("Neiter points nor point file provided")
    {
        PointDepthParser parser;
        ArgumentHolder ah{"prog"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Either -pdp or -pdf must be given"));
    }

    SECTION("Points and pointfile provided")
    {
        PointDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-pdp", "0.1,5.2", "-pdf", "testpoints.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-pdf cannot be used together with -pdp"));
    }

    SECTION("Pointfile and points provided")
    {
        PointDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-pdf", "testpoints.csv", "-pdp", "0.1,5.2"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-pdp cannot be used together with -pdf"));
    }

    SECTION("Malformed pointfile")
    {
        PointDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-pdf", "testpoints.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 1"));
    }

    SECTION("Malformed point arg")
    {
        PointDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-pdp", "0.1"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 0.1"));
    }

}

TEST_CASE("PointDepthParserSuccess", "Read successfully")
{
    PointDepthParser parser;
    double x1 = 1.0;
    double y1 = 2.0;
    double x2 = 1.1;
    double y2 = 1.2;

    SECTION("Read from commandline")
    {
        std::stringstream p1;
        p1 << x1 << "," << y1 << std::flush;
        std::stringstream p2;
        p2 << x2 << "," << y2 << std::flush;

        ArgumentHolder ah{"prog", "-pdp", p1.str(), "-pdp", p2.str(), "-pdt", "visual"};
        parser.parse(ah.argc(), ah.argv());
    }

    SECTION("Read from file")
    {
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f(scf.Filename().c_str());
            f << "x\ty\n" << x1 << "\t" << y1 << "\n"
                          << x2 << "\t" << y2 << "\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-pdf", scf.Filename(), "-pdt", "visual"};
        parser.parse(ah.argc(), ah.argv() );
    }

    auto points = parser.getOriginPoints();
    REQUIRE(points.size() == 2);
    REQUIRE(points[0].x == Approx(x1));
    REQUIRE(points[0].y == Approx(y1));
    REQUIRE(points[1].x == Approx(x2));
    REQUIRE(points[1].y == Approx(y2));
    REQUIRE(parser.getPointDepthType() == PointDepthParser::PointDepthType::VISUAL);
}
