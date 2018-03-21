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
#include "depthmapXcli/stepdepthparser.h"
#include "argumentholder.h"
#include "selfcleaningfile.h"

TEST_CASE("StepDepthParserFail", "Error cases")
{
    SECTION("Missing argument to -sdp")
    {
        StepDepthParser parser;
        ArgumentHolder ah{"prog", "-sdp"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-sdp requires an argument"));
    }
    SECTION("Missing argument to -sdf")
    {
        StepDepthParser parser;
        ArgumentHolder ah{"prog", "-sdf"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-sdf requires an argument"));
    }

    SECTION("rubbish input to -sdp")
    {
        StepDepthParser parser;
        ArgumentHolder ah{"prog", "-sdp", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid step depth point provided (foo). Should only contain digits dots and commas"));
    }

    SECTION("Non-existing file provided")
    {
        StepDepthParser parser;
        ArgumentHolder ah{"prog", "-sdf", "foo.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Failed to load file foo.csv, error"));
    }

    SECTION("Neiter points nor point file provided")
    {
        StepDepthParser parser;
        ArgumentHolder ah{"prog"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Either -sdp or -sdf must be given"));
    }

    SECTION("Points and pointfile provided")
    {
        StepDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-sdp", "0.1,5.2", "-sdf", "testpoints.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-sdf cannot be used together with -sdp"));
    }

    SECTION("Pointfile and points provided")
    {
        StepDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-sdf", "testpoints.csv", "-sdp", "0.1,5.2"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-sdp cannot be used together with -sdf"));
    }

    SECTION("Malformed pointfile")
    {
        StepDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-sdf", "testpoints.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 1"));
    }

    SECTION("Malformed point arg")
    {
        StepDepthParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-sdp", "0.1"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 0.1"));
    }

}

TEST_CASE("StepDepthParserSuccess", "Read successfully")
{
    StepDepthParser parser;
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

        ArgumentHolder ah{"prog", "-sdp", p1.str(), "-sdp", p2.str()};
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
        ArgumentHolder ah{"prog", "-sdf", scf.Filename()};
        parser.parse(ah.argc(), ah.argv() );
    }

    auto points = parser.getStepDepthPoints();
    REQUIRE(points.size() == 2);
    REQUIRE(points[0].x == Approx(x1));
    REQUIRE(points[0].y == Approx(y1));
    REQUIRE(points[1].x == Approx(x2));
    REQUIRE(points[1].y == Approx(y2));
}
