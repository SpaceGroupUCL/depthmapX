// Copyright (C) 2018 Petros Koutsolampros

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

#include "../depthmapXcli/segmentparser.h"
#include "argumentholder.h"

TEST_CASE("Test segment mode and help")
{
    SegmentParser parser;
    REQUIRE(parser.getModeName() == "SEGMENT");
    REQUIRE(parser.getHelp() == "Mode options for Segment Analysis:\n"\
                                "  -st  <type of analysis> one of:\n"
                                "       tulip (Angular Tulip - Faster)\n"\
                                "       angular (Angular Full - Slower)\n"\
                                "       topological\n"\
                                "       metric\n"\
                                "  -sr  <radius/list of radii>\n"\
                                "  -srt <radius type> (only for Tulip) one of:\n"\
                                "       steps\n"\
                                "       metric\n"\
                                "       angular\n"\
                                "  -sic to include choice (only for Tulip)\n"\
                                "  -stb <tulip bins> (4 to 1024, 1024 approximates full angular)\n");

}

TEST_CASE("Test Segment Parsing Exceptions","")
{
    SegmentParser parser;
    SECTION("No segment mode")
    {
        ArgumentHolder ah{"prog"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "No analysis type given" );
    }

    SECTION("Argument missing -st")
    {
        ArgumentHolder ah{"prog", "-st"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-st requires an argument" );
    }

    SECTION("Invalid SEGMENT mode")
    {
        ArgumentHolder ah{"prog", "-st", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "Invalid SEGMENT mode: foo" );
    }

    SECTION("Argument missing -sr")
    {
        ArgumentHolder ah{"prog", "-sr"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-sr requires an argument" );
    }

    SECTION("Argument missing -srt")
    {
        ArgumentHolder ah{"prog", "-srt"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-srt requires an argument" );
    }

    SECTION("Invalid SEGMENT radius type")
    {
        ArgumentHolder ah{"prog", "-srt", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "Invalid SEGMENT radius type: foo" );
    }

    SECTION("Argument missing -stb")
    {
        ArgumentHolder ah{"prog", "-stb"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-stb requires an argument" );
    }

    SECTION("Missung radius metric")
    {
        ArgumentHolder ah{"prog", "-st", "metric"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "At least one radius must be provided" );
    }

    SECTION("Missung radius topological")
    {
        ArgumentHolder ah{"prog", "-st", "topological"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "At least one radius must be provided" );
    }

    SECTION("Missung radius angular")
    {
        ArgumentHolder ah{"prog", "-st", "angular"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "At least one radius must be provided" );
    }

    SECTION("Missung radius tulip")
    {
        ArgumentHolder ah{"prog", "-st", "tulip"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "At least one radius must be provided" );
    }

    SECTION("Missing tulip radius type")
    {
        ArgumentHolder ah{"prog", "-st", "tulip", "-sr", "n"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "Radius type is required for tulip analysis" );
    }

    SECTION("Missing tulip bins")
    {
        ArgumentHolder ah{"prog", "-st", "tulip", "-sr", "n", "-srt", "steps"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "Tulip bins are required for tulip analysis" );
    }

    SECTION("Tulip bins out of range")
    {
        ArgumentHolder ah{"prog", "-st", "tulip", "-sr", "n", "-srt", "steps", "-stb", "2"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-stb must be a number between 4 and 1024, got 2" );
    }

    SECTION("Tulip bins out of range")
    {
        ArgumentHolder ah{"prog", "-st", "tulip", "-sr", "n", "-srt", "steps", "-stb", "1025"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), "-stb must be a number between 4 and 1024, got 1025" );
    }
}

TEST_CASE("Test segment mode parsing", "")
{
    SegmentParser parser;
    SECTION("Analysis Metric")
    {
        ArgumentHolder ah{"prog", "-st", "metric", "-sr", "n"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(parser.includeChoice());
        REQUIRE(parser.getTulipBins() == 0);
        REQUIRE(parser.getAnalysisType() == SegmentParser::AnalysisType::METRIC);
        REQUIRE(parser.getRadiusType() == SegmentParser::RadiusType::NONE);
        REQUIRE(parser.getRadii().size() == 1);
        REQUIRE(int(parser.getRadii()[0]) == -1);
    }
    SECTION("Analysis Angular Full")
    {
        ArgumentHolder ah{"prog", "-st", "angular", "-sr", "n"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(parser.includeChoice());
        REQUIRE(parser.getTulipBins() == 0);
        REQUIRE(parser.getAnalysisType() == SegmentParser::AnalysisType::ANGULAR_FULL);
        REQUIRE(parser.getRadiusType() == SegmentParser::RadiusType::NONE);
        REQUIRE(parser.getRadii().size() == 1);
        REQUIRE(int(parser.getRadii()[0]) == -1);
    }
    SECTION("Analysis Topological")
    {
        ArgumentHolder ah{"prog", "-st", "topological", "-sr", "n"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(parser.includeChoice());
        REQUIRE(parser.getTulipBins() == 0);
        REQUIRE(parser.getAnalysisType() == SegmentParser::AnalysisType::TOPOLOGICAL);
        REQUIRE(parser.getRadiusType() == SegmentParser::RadiusType::NONE);
        REQUIRE(parser.getRadii().size() == 1);
        REQUIRE(int(parser.getRadii()[0]) == -1);
    }
    SECTION("Analysis Tulip")
    {
        ArgumentHolder ah{"prog", "-st", "tulip", "-sr", "n", "-srt", "steps", "-stb", "1024", "-sic"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE(parser.includeChoice());
        REQUIRE(parser.getTulipBins() == 1024);
        REQUIRE(parser.getAnalysisType() == SegmentParser::AnalysisType::ANGULAR_TULIP);
        REQUIRE(parser.getRadiusType() == SegmentParser::RadiusType::SEGMENT_STEPS);
        REQUIRE(parser.getRadii().size() == 1);
        REQUIRE(int(parser.getRadii()[0]) == -1);
    }

}
