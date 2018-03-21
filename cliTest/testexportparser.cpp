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

#include "catch.hpp"
#include "../depthmapXcli/exportparser.h"
#include "argumentholder.h"

TEST_CASE("ExportParser Fail", "Parsing errors")
{
    // missing arguments

    SECTION("Missing argument to -em")
    {
        ExportParser parser;
        ArgumentHolder ah{"prog", "-em"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-em requires an argument"));
    }
}

TEST_CASE("ExportParser Success", "Read successfully")
{
    ExportParser parser;

    SECTION("Correctly parse mode pointmap-connections-csv")
    {
        ArgumentHolder ah{"prog", "-em", "pointmap-connections-csv"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE(parser.getExportMode() == ExportParser::POINTMAP_CONNECTIONS_CSV);
    }

    SECTION("Correctly parse mode pointmap-data-csv")
    {
        ArgumentHolder ah{"prog", "-em", "pointmap-data-csv"};
        parser.parse(ah.argc(), ah.argv());
        REQUIRE(parser.getExportMode() == ExportParser::POINTMAP_DATA_CSV);
    }
}
