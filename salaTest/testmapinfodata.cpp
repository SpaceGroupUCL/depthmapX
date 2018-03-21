// Copyright (C) 2017-2018 Petros Koutsolampros

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
#include "salalib/mgraph.h"
#include "salalib/MapInfoData.h"

TEST_CASE("MapInfo failing header", "")
{
    std::string mifdata = "Version 300\n";

    SECTION("Missing quotes around delimiter") {
        mifdata += "Charset \"WindowsLatin1\"\n" \
                   "Delimiter ,\n" \
                   "Index 1,2\n" \
                   "CoordSys Earth Projection 8, 79, \"m\", -2, 49, 0.9996012717, 400000, -100000";
    }

    SECTION("Missing CoordSys") {
        mifdata += "Charset \"WindowsLatin1\"\n" \
                   "Delimiter \",\"\n" \
                   "Index 1,2\n" \
                   "Bounds (-7845061.1011, -15524202.1641) (8645061.1011, 4470074.53373)\n";
    }

    std::stringstream mifstream(mifdata);

    MapInfoData mapinfodata;
    REQUIRE_FALSE(mapinfodata.readheader(mifstream));
}

TEST_CASE("MapInfo failing column attribute columns", "")
{

    std::string mifdata = "";


    SECTION("Missing Columns at beginning") {
        mifdata += "Tolumns 2\n" \
                   "  ID Integer\n" \
                   "  Length_m Float\n" \
                   "Data\n";
    }

    SECTION("Missing Column number") {
        mifdata += "Columns\n" \
                   "  ID Integer\n" \
                   "  Length_m Float\n" \
                   "Data\n";
    }

    SECTION("Missing Data at end") {
        mifdata += "Columns 2\n" \
                   "  ID Integer\n" \
                   "  Length_m Float\n" \
                   "Bata\n";
    }

    std::string middata =
            "1,1017.81\n" \
            "2,568.795\n" \
            "3,216.026";

    std::stringstream mifstream(mifdata);
    std::stringstream midstream(middata);

    std::vector<std::string> columnheads;

    MapInfoData mapinfodata;
    REQUIRE_FALSE(mapinfodata.readcolumnheaders(mifstream, midstream, columnheads));
}

TEST_CASE("Complete proper MapInfo file", "")
{
    const float EPSILON = 0.001;

    // A typical MIF

    std::string mifdata =
            "Version 300\n" \
            "Charset \"WindowsLatin1\"\n" \
            "Delimiter \",\"\n" \
            "Index 1,2\n" \
            "CoordSys Earth Projection 8, 79, \"m\", -2, 49, 0.9996012717, 400000, -100000";

    SECTION("With Bounds") {
        mifdata += "Bounds (-7845061.1011, -15524202.1641) (8645061.1011, 4470074.53373)\n";
    }

    SECTION("Without Bounds") {
        mifdata += "\n";
    }

    mifdata +=
            "Columns 2\n" \
            "  ID Integer\n" \
            "  Length_m Float\n" \
            "Data\n" \
            "\n" \
            "Line 534014.29 182533.33 535008.52 182764.11\n" \
            "    Pen (1,2,0)\n" \
            "Line 533798.68 183094.69 534365.48 183159.01\n" \
            "    Pen (1,2,0)\n" \
            "Point 534014.29 182533.33\n" \
            "    Symbol (34,0,12)";


    // A Typical MID

    std::string middata =
            "1,1017.81\n" \
            "2,568.795\n" \
            "3,216.026";

    ShapeMap shapeMap("MapInfoTest");
    MapInfoData mapinfodata;

    std::stringstream mifstream(mifdata);
    std::stringstream midstream(middata);
    REQUIRE(mapinfodata.import(mifstream, midstream, shapeMap) == MINFO_OK);

    std::map<int, SalaShape> shapes = shapeMap.getAllShapes();
    REQUIRE(shapes.size() == 3);
    REQUIRE(depthmapX::getMapAtIndex(shapes, 0)->second.isLine());
    REQUIRE(depthmapX::getMapAtIndex(shapes, 0)->second.getLine().ax() == Approx(534014.29).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 0)->second.getLine().ay() == Approx(182533.33).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 0)->second.getLine().bx() == Approx(535008.52).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 0)->second.getLine().by() == Approx(182764.11).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 1)->second.isLine());
    REQUIRE(depthmapX::getMapAtIndex(shapes, 1)->second.getLine().ax() == Approx(533798.68).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 1)->second.getLine().ay() == Approx(183094.69).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 1)->second.getLine().bx() == Approx(534365.48).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 1)->second.getLine().by() == Approx(183159.01).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 2)->second.isPoint());
    REQUIRE(depthmapX::getMapAtIndex(shapes, 2)->second.getPoint().x == Approx(534014.29).epsilon(EPSILON));
    REQUIRE(depthmapX::getMapAtIndex(shapes, 2)->second.getPoint().y == Approx(182533.33).epsilon(EPSILON));

    AttributeTable att = shapeMap.getAttributeTable();
    REQUIRE(att.getColumnCount() == 2);
    REQUIRE(att.getColumnName(0) == "Id");
    REQUIRE(att.getColumnName(1) == "Length_M");

    REQUIRE(att.getRowCount() == 3);
    REQUIRE(att.getValue(0,"Id") == 1);
    REQUIRE(att.getValue(1,"Id") == 2);
    REQUIRE(att.getValue(2,"Id") == 3);
    REQUIRE(att.getValue(0,"Length_M") == Approx(1017.81).epsilon(EPSILON));
    REQUIRE(att.getValue(1,"Length_M") == Approx(568.795).epsilon(EPSILON));
    REQUIRE(att.getValue(2,"Length_M") == Approx(216.026).epsilon(EPSILON));
}
