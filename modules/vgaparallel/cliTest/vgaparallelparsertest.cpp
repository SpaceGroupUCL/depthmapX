// Copyright (C) 2020 Petros Koutsolampros

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

#include "modules/vgaparallel/cli/vgaparallelparser.h"
#include "cliTest/argumentholder.h"
#include "cliTest/selfcleaningfile.h"
#include <catch.hpp>

TEST_CASE("VgaParallelParser", "Error cases") {
    SECTION("Missing argument to -vr") {
        VgaParallelParser parser;
        ArgumentHolder ah{
            "prog",
            "-vr",
        };
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-vr requires an argument"));
    }

    SECTION("Missing argument to -vm") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-vm requires an argument"));
    }

    SECTION("rubbish input to -vm") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid VGAPARALLEL mode: foo"));
    }

    SECTION("no input to -vr in metric analysis") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm", "metric"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()),
                            Catch::Contains("Metric vga requires a radius, use -vr <radius>"));
    }

    SECTION("rubbish input to -vr in metric analysis") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm", "metric", "-vr", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()),
                            Catch::Contains("Radius must be a positive integer number or n, got foo"));
    }

    SECTION("no input to -vr in visual global analysis") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm", "visiblity-global"};
        REQUIRE_THROWS_WITH(
            parser.parse(ah.argc(), ah.argv()),
            Catch::Contains("Global measures in VGA/visibility analysis require a radius, use -vr <radius>"));
    }

    SECTION("rubbish input to -vr in visual global analysis") {
        VgaParallelParser parser;
        ArgumentHolder ah{"prog", "-vm", "visiblity-global", "-vr", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()),
                            Catch::Contains("Radius must be a positive integer number or n, got foo"));
    }
}

TEST_CASE("Successful VgaParallelParser", "Read successfully") {
    VgaParallelParser parser;

    SECTION("Read from commandline") {
        ArgumentHolder ah{"prog", "-vm", "metric", "-vr", "5"};
        parser.parse(ah.argc(), ah.argv());
    }

    REQUIRE(parser.getVgaMode() == VgaParallelParser::VgaMode::METRIC);
    REQUIRE(parser.getRadius() == "5");
}
