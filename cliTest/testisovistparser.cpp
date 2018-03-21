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
#include "depthmapXcli/isovistparser.h"
#include "argumentholder.h"
#include "selfcleaningfile.h"

TEST_CASE("Isovistparser string constants")
{
    IsovistParser parser;
    REQUIRE(parser.getModeName() == "ISOVIST");
    REQUIRE(parser.getHelp() == "Arguments for isovist mode:\n" \
                                "  -ii <x,y[,angle,viewangle]> Define an isoivist at position x,y with\n"\
                                "    optional direction angle and view angle for partial isovists\n"\
                                "  -if <isovist file> load isovist definitions from a file (csv)\n"\
                                "    the relevant headers must be called x, y, angle and viewangle\n"\
                                "    the latter two are optional.\n"\
                                "  Those two arguments cannot be mixed\n"\
                                "  Angles for partial isovists are in degrees, counted anti-clockwise with 0°\n"\
                                "  pointing to the right.\n\n" );
}

TEST_CASE("Parse isovist on the command line")
{
    ArgumentHolder ah{"prog", "-ii", "1,2", "-ii", "1,5,27,180"};
    IsovistParser parser;
    parser.parse(ah.argc(), ah.argv());

    REQUIRE(parser.getIsovists().size() == 2);
    REQUIRE(parser.getIsovists()[0].getLocation().y == Approx(2.0));
    REQUIRE(parser.getIsovists()[0].getViewAngle() == 0.0);

    REQUIRE(parser.getIsovists()[1].getLocation().y == Approx(5.0));
    REQUIRE(parser.getIsovists()[1].getViewAngle() == Approx(3.141592));

}


TEST_CASE("Parse isovists from file")
{
    IsovistParser parser;

    SECTION("Full isovists")
    {
        SelfCleaningFile scf("fullisovists.csv");
        {
            std::ofstream file(scf.Filename());
            file << "id,x,y,angle\n1,1,1,27\n2,2.2,1.1,190\n" << std::flush;
        }

        ArgumentHolder ah{"prog", "-if", scf.Filename()};
        parser.parse(ah.argc(), ah.argv());

        REQUIRE(parser.getIsovists().size() == 2);
        REQUIRE(parser.getIsovists()[0].getLocation().y == Approx(1.0));
        REQUIRE(parser.getIsovists()[0].getAngle() == 0.0);
        REQUIRE(parser.getIsovists()[0].getViewAngle() == 0.0);

        REQUIRE(parser.getIsovists()[1].getLocation().y == Approx(1.1));
        REQUIRE(parser.getIsovists()[1].getAngle() == 0.0);
        REQUIRE(parser.getIsovists()[1].getViewAngle() == 0.0);
    }

    SECTION("Partial isovists")
    {
        SelfCleaningFile scf("fullisovists.csv");
        {
            std::ofstream file(scf.Filename());
            file << "id,x,y,angle,foo,ViewAngle\n1,1,1,27,bar,180\n2,2.2,1.1,180,baz,90\n" << std::flush;
        }

        ArgumentHolder ah{"prog", "-if", scf.Filename()};
        parser.parse(ah.argc(), ah.argv());

        REQUIRE(parser.getIsovists().size() == 2);
        REQUIRE(parser.getIsovists()[0].getLocation().y == Approx(1.0));
        REQUIRE(parser.getIsovists()[0].getAngle() == Approx(0.4712388) );
        REQUIRE(parser.getIsovists()[0].getViewAngle() == Approx(3.141592));

        REQUIRE(parser.getIsovists()[1].getLocation().y == Approx(1.1));
        REQUIRE(parser.getIsovists()[1].getAngle() == Approx(3.141592));
        REQUIRE(parser.getIsovists()[1].getViewAngle() == Approx(1.57079));
    }

}

TEST_CASE("Command line failures")
{
    IsovistParser parser;

    SECTION( "Missing arguments for -ii")
    {
        ArgumentHolder ah{"prog", "-ii"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-ii requires an argument"));
    }

    SECTION( "Missing arguments for -if")
    {
        ArgumentHolder ah{"prog", "-if"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-if requires an argument"));
    }

    SECTION( "Using -if twice")
    {
        ArgumentHolder ah{"prog", "-if", "foo.csv", "-if", "bar.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-if can only be used once"));
    }

    SECTION( "Using -ii and -if")
    {
        ArgumentHolder ah{"prog", "-ii", "1,1", "-if", "bar.csv"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-if cannot be used together with -ii"));

    }

    SECTION( "Using -if and -ii")
    {
        ArgumentHolder ah{"prog", "-if", "foo.csv", "-ii", "1,1"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-ii cannot be used together with -if"));
    }

    SECTION("Nothing to do")
    {
        ArgumentHolder ah{"prog"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("No isovists defined. Use -ii or -if"));
    }


}
