// Copyright (C) 2017 Petros Koutsolampros, Christian Sailer

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
#include "salalib/entityparsing.h"
#include "genlib/p2dpoly.h"
#include <sstream>
#include <vector>

TEST_CASE("Failing line parser", "")
{
    const float EPSILON = 0.001;
    {
        // header only has 3 elements
        std::stringstream stream;
        stream << "x1,y1,x2" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parseLines(stream,','), Catch::Contains("Badly formatted header (should contain x1, y1, x2 and y2)"));
    }

    {
        // header has y1 twice instead of y2
        std::stringstream stream;
        stream << "x1,y1,x2,y1" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parseLines(stream,','), Catch::Contains("Badly formatted header (should contain x1, y1, x2 and y2)"));
    }

    {
        // error parsing line
        std::stringstream stream;
        stream << "x1,y1,x2,y2" << std::endl;
        stream << "1.2,3.4,5.6" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parseLines(stream,','), Catch::Contains("Error parsing line"));
    }
}
TEST_CASE("Successful line parser", "")
{
    const float EPSILON = 0.001;
    {
        std::stringstream stream;
        stream << "x1,y1,x2,y2" << std::endl;
        stream << "1.2,3.4,5.6,7.8" << std::endl;
        std::vector<Line> lines = EntityParsing::parseLines(stream,',');
        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0].start().x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(lines[0].start().y == Approx(3.4).epsilon(EPSILON));
        REQUIRE(lines[0].end().x == Approx(5.6).epsilon(EPSILON));
        REQUIRE(lines[0].end().y == Approx(7.8).epsilon(EPSILON));
    }

    {
        std::stringstream stream;
        stream << "x1\ty1\tx2\ty2" << std::endl;
        stream << "1.2\t3.4\t5.6\t7.8" << std::endl;
        std::vector<Line> lines = EntityParsing::parseLines(stream,'\t');
        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0].start().x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(lines[0].start().y == Approx(3.4).epsilon(EPSILON));
        REQUIRE(lines[0].end().x == Approx(5.6).epsilon(EPSILON));
        REQUIRE(lines[0].end().y == Approx(7.8).epsilon(EPSILON));
    }

    {
        std::stringstream stream;
        stream << "x1\ty1\tx2\ty2" << std::endl;
        stream << "1.2\t3.4\t5.6\t7.8" << std::endl;
        stream << "0.1\t0.2\t0.3\t0.4" << std::endl;
        std::vector<Line> lines = EntityParsing::parseLines(stream,'\t');
        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0].start().x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(lines[0].start().y == Approx(3.4).epsilon(EPSILON));
        REQUIRE(lines[0].end().x == Approx(5.6).epsilon(EPSILON));
        REQUIRE(lines[0].end().y == Approx(7.8).epsilon(EPSILON));
        REQUIRE(lines[1].start().x == Approx(0.1).epsilon(EPSILON));
        REQUIRE(lines[1].start().y == Approx(0.2).epsilon(EPSILON));
        REQUIRE(lines[1].end().x == Approx(0.3).epsilon(EPSILON));
        REQUIRE(lines[1].end().y == Approx(0.4).epsilon(EPSILON));
    }
}

TEST_CASE("Failing point parser", "")
{
    const float EPSILON = 0.001;
    {
        // header only has 3 elements
        std::stringstream stream;
        stream << "x" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parsePoints(stream,','), Catch::Contains("Badly formatted header (should contain x and y)"));
    }

    {
        // header has y1 twice instead of y2
        std::stringstream stream;
        stream << "x,x" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parsePoints(stream,','), Catch::Contains("Badly formatted header (should contain x and y)"));
    }

    {
        // error parsing line
        std::stringstream stream;
        stream << "x,y" << std::endl;
        stream << "1.2" << std::endl;
        REQUIRE_THROWS_WITH(EntityParsing::parsePoints(stream,','), Catch::Contains("Error parsing line"));
    }
}
TEST_CASE("Successful point parser", "")
{
    const float EPSILON = 0.001;
    {
        std::stringstream stream;
        stream << "x,y" << std::endl;
        stream << "1.2,3.4" << std::endl;
        std::vector<Point2f> points = EntityParsing::parsePoints(stream,',');
        REQUIRE(points.size() == 1);
        REQUIRE(points[0].x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(points[0].y == Approx(3.4).epsilon(EPSILON));
    }

    {
        std::stringstream stream;
        stream << "x\ty" << std::endl;
        stream << "1.2\t3.4" << std::endl;
        std::vector<Point2f> points = EntityParsing::parsePoints(stream,'\t');
        REQUIRE(points.size() == 1);
        REQUIRE(points[0].x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(points[0].y == Approx(3.4).epsilon(EPSILON));
    }

    {
        std::stringstream stream;
        stream << "x\ty" << std::endl;
        stream << "1.2\t3.4" << std::endl;
        stream << "0.1\t0.2" << std::endl;
        std::vector<Point2f> points = EntityParsing::parsePoints(stream,'\t');
        REQUIRE(points.size() == 2);
        REQUIRE(points[0].x == Approx(1.2).epsilon(EPSILON));
        REQUIRE(points[0].y == Approx(3.4).epsilon(EPSILON));
        REQUIRE(points[1].x == Approx(0.1).epsilon(EPSILON));
        REQUIRE(points[1].y == Approx(0.2).epsilon(EPSILON));
    }
}


TEST_CASE("Test point parsing")
{
    REQUIRE_THROWS_WITH(EntityParsing::parsePoint("foo", '|'), Catch::Contains("Badly formatted point data, should be <number>|<number>, was foo" ));
    auto point = EntityParsing::parsePoint("1.235|27.25", '|');
    REQUIRE(point.x == Approx(1.235));
    REQUIRE(point.y == Approx(27.25));

    point = EntityParsing::parsePoint("1.235|bar", '|');
    REQUIRE(point.x == Approx(1.235));
    REQUIRE(point.y == 0.0);
}

TEST_CASE("Successful Isovist parser")
{
    const float EPSILON = 0.0001;
    {
        std::stringstream stream;
        stream << "x,y\n1.0,2.34\n0.5,9.2\n" << std::flush;
        auto result = EntityParsing::parseIsovists(stream, ',');
        REQUIRE(result.size() == 2);
        REQUIRE(result[0].getLocation().x == Approx(1.0).epsilon(EPSILON));
        REQUIRE(result[0].getLocation().y == Approx(2.34).epsilon(EPSILON));
        REQUIRE(result[0].getAngle() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(result[0].getViewAngle() == 0.0);
        REQUIRE(result[1].getLocation().x == Approx(0.5).epsilon(EPSILON));
        REQUIRE(result[1].getLocation().y == Approx(9.2).epsilon(EPSILON));
        REQUIRE(result[1].getAngle() == Approx(0.0).epsilon(EPSILON));
        REQUIRE(result[1].getViewAngle() == 0.0);
    }
    {
        std::stringstream stream;
        stream << "x,y,angle,viewAngle\n1.0,2.34,90,90\n0.5,9.2,180,270\n" << std::flush;
        auto result = EntityParsing::parseIsovists(stream, ',');
        REQUIRE(result.size() == 2);
        REQUIRE(result[0].getLocation().x == Approx(1.0).epsilon(EPSILON));
        REQUIRE(result[0].getLocation().y == Approx(2.34).epsilon(EPSILON));
        REQUIRE(result[0].getAngle() == Approx(M_PI/2.0).epsilon(EPSILON));
        REQUIRE(result[0].getViewAngle() == Approx(M_PI/2.0).epsilon(EPSILON));
        REQUIRE(result[1].getLocation().x == Approx(0.5).epsilon(EPSILON));
        REQUIRE(result[1].getLocation().y == Approx(9.2).epsilon(EPSILON));
        REQUIRE(result[1].getAngle() == Approx(M_PI).epsilon(EPSILON));
        REQUIRE(result[1].getViewAngle() == Approx(M_PI*1.5).epsilon(EPSILON));
    }
}

TEST_CASE("Failing Isovist parser")
{
    {
        std::stringstream stream;
        stream << "x,angle,viewAngle\n" << std::flush;
        REQUIRE_THROWS_WITH(EntityParsing::parseIsovists(stream, ','), Catch::Contains("Badly formatted header (should contain x and y, might also have angle and viewangle for partial isovists)"));
    }

    {
        std::stringstream stream;
        stream << "x,y,angle,viewAngle\n1.0,1.0,270" << std::flush;
        REQUIRE_THROWS_WITH(EntityParsing::parseIsovists(stream, ','), Catch::Contains("Error parsing line: 1.0,1.0,270"));
    }
}

TEST_CASE("Parsing single isovist")
{
    SECTION("Success full")
    {
        auto result =  EntityParsing::parseIsovist("1,1");
        REQUIRE(result.getLocation().x == Approx(1.0));
        REQUIRE(result.getLocation().y == Approx(1.0));
        REQUIRE(result.getAngle() == 0.0);
        REQUIRE(result.getViewAngle() == 0.0);
    }

    SECTION("Success partial isovist")
    {
        auto result =  EntityParsing::parseIsovist("1,1,27,90");
        REQUIRE(result.getLocation().x == Approx(1.0));
        REQUIRE(result.getLocation().y == Approx(1.0));
        REQUIRE(result.getAngle() == Approx(0.4712388));
        REQUIRE(result.getViewAngle() == Approx(M_PI/2.0));
    }

    SECTION("Failed bad string")
    {
        REQUIRE_THROWS_WITH(EntityParsing::parseIsovist("1,1,27"), Catch::Contains("Failed to parse '1,1,27' to an isovist definition"));
    }
}
