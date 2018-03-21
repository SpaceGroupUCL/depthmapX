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
#include "salalib/axialmap.h"


/**
 * This seems a bit silly, but this is a list of structs that are serialised by just dumping the memory content
 * into a stream, so the size/layout of these must be the same across all platforms to ensure
 * reading writing of graph files.
 */
TEST_CASE("Enforce struct sizes")
{
    REQUIRE(sizeof(RadialKey) == 16);
    REQUIRE(sizeof(RadialLine) == 64);
    REQUIRE(sizeof(PolyConnector) == 56);
    REQUIRE(sizeof(QtRegion) == 32);
    REQUIRE(sizeof(Line) == 40);
}
