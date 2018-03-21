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
#include "../depthmapXcli/agentparser.h"
#include "argumentholder.h"
#include "selfcleaningfile.h"

TEST_CASE("AgentParserFail", "Parsing errors")
{
    // missing arguments

    SECTION("Missing argument to -am")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-am"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-am requires an argument"));
    }

    SECTION("Missing argument to -ats")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-ats requires an argument"));
    }

    SECTION("Missing argument to -arr")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-arr"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-arr requires an argument"));
    }

    SECTION("Missing argument to -afov")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-afov"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-afov requires an argument"));
    }

    SECTION("Missing argument to -asteps")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-asteps"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-asteps requires an argument"));
    }

    SECTION("Missing argument to -alife")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alife"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alife requires an argument"));
    }

    SECTION("Missing argument to -alife")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alife"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alife requires an argument"));
    }

    SECTION("Missing argument to -alocseed")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alocseed"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocseed requires an argument"));
    }

    SECTION("Missing argument to -alocfile")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alocfile"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocfile requires an argument"));
    }

    SECTION("Missing argument to -aloc")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-aloc"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-aloc requires an argument"));
    }

    // rubbish input

    SECTION("Non-numeric input to -ats")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-ats must be a number >0, got foo"));
    }

    SECTION("Non-numeric input to -arr")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-arr", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-arr must be a number >0, got foo"));
    }

    SECTION("Non-numeric input to -atrails")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-atrails", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-atrails must be a number >=1 or 0 for all (max possible = 50), got foo"));
    }

    SECTION("Non-numeric input to -afov")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-afov", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-afov must be a number between 1 and 32, got foo"));
    }

    SECTION("Out of range input to -afov (0)")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-afov", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-afov must be a number between 1 and 32, got 0"));
    }

    SECTION("Out of range input to -afov (33)")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-afov", "33"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-afov must be a number between 1 and 32, got 33"));
    }

    SECTION("Non-numeric input to -asteps")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-asteps", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-asteps must be a number >0, got foo"));
    }

    SECTION("Non-numeric input to -alife")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alife", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alife must be a number >0, got foo"));
    }

    SECTION("Rubbish input to -alocseed")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alocseed", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid starting location seed provided (foo). Should only contain digits"));
    }

    SECTION("Rubbish input to -aloc")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-aloc", "foo"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Invalid starting point provided (foo). Should only contain digits dots and commas"));
    }

    SECTION("Define graph output twice")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ot", "graph", "-ot", "graph"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Same output type argument (graph) provided twice"));
    }

    SECTION("Define gatecounts output twice")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ot", "gatecounts", "-ot", "gatecounts"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Same output type argument (gatecounts) provided twice"));
    }

    SECTION("Define trails output twice")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ot", "trails", "-ot", "trails"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Same output type argument (trails) provided twice"));
    }
}
TEST_CASE("AgentParserInputFail", "Bad or missing input")
{
    SECTION("-ats not provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000", "-alocseed", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Total number of timesteps (-ats <timesteps>) is required"));
    }

    SECTION("-arr not provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "5000", "-afov","15", "-asteps", "3", "-alife", "1000", "-alocseed", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Release rate (-arr <rate>) is required"));
    }

    SECTION("-afov not provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "5000", "-arr", "0.1", "-asteps", "3", "-alife", "1000", "-alocseed", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Agent field-of-view (-afov <bins>) is required"));
    }

    SECTION("-asteps not provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "5000", "-arr", "0.1", "-afov","15", "-alife", "1000", "-alocseed", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Agent number of steps before turn decision (-asteps <steps>) is required"));
    }

    SECTION("-alife not provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alocseed", "0"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Agent life in timesteps (-alife <timesteps>) is required"));
    }

    SECTION("No random starting poins, manual points or point file provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Either -aloc, -alocfile or -alocseed must be given"));
    }

    SECTION("Manual points and pointfile provided")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-aloc", "0.1,5.2", "-alocfile", "testpoints.csv", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocfile cannot be used together with -aloc"));
    }

    SECTION("Pointfile and manual points provided")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-alocfile", "testpoints.csv", "-aloc", "0.1,5.2", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-aloc cannot be used together with -alocfile"));
    }

    SECTION("Manual points and random points provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-aloc", "0.1,5.2", "-alocseed", "0", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocseed cannot be used together with -aloc"));
    }

    SECTION("Pointfile and random points provided")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-alocfile", "testpoints.csv", "-alocseed", "0", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocseed cannot be used together with -alocfile"));
    }

    SECTION("Random points and manual points provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alocseed", "0", "-aloc", "0.1,5.2", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-aloc cannot be used together with -alocseed"));
    }

    SECTION("Random points and Pointfile provided")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\t2\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-alocseed", "0", "-alocfile", "testpoints.csv", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("-alocfile cannot be used together with -alocseed"));
    }

    SECTION("Non-existing file provided")
    {
        AgentParser parser;
        ArgumentHolder ah{"prog", "-alocfile", "foo.csv", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Failed to load file foo.csv, error"));
    }

    SECTION("Malformed pointfile")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-alocfile", "testpoints.csv", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 1"));
    }

    SECTION("Malformed point arg")
    {
        AgentParser parser;
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f("testpoints.csv");
            f << "x\ty\n1\n" << std::flush;
        }
        ArgumentHolder ah{"prog", "-aloc", "0.1", "-ats", "5000", "-arr", "0.1", "-afov","15", "-asteps", "3", "-alife", "1000"};
        REQUIRE_THROWS_WITH(parser.parse(ah.argc(), ah.argv()), Catch::Contains("Error parsing line: 0.1"));
    }
}

TEST_CASE("AgentParserSuccess", "Read successfully")
{
    AgentParser parser;
    double x1 = 1.0;
    double y1 = 2.0;
    double x2 = 1.1;
    double y2 = 1.2;

    int totalTimeSteps = 5000;
    std::stringstream ats;
    ats << totalTimeSteps << std::flush;

    double releaseRate = 0.1;
    std::stringstream arr;
    arr << releaseRate << std::flush;

    int agentFOV = 15;
    std::stringstream afov;
    afov << agentFOV << std::flush;

    int agentStepsBeforeTurnDecision = 3;
    std::stringstream asteps;
    asteps << agentStepsBeforeTurnDecision << std::flush;

    int agentLifeTimesteps = 1000;
    std::stringstream alife;
    alife << agentLifeTimesteps << std::flush;

    SECTION("Random starting locations (points vector should be empty, seed 0)")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0"};
        parser.parse(ah.argc(), ah.argv());

        auto points = parser.getReleasePoints();
        REQUIRE(points.size() == 0);
        REQUIRE(parser.randomReleaseLocationSeed() == 0);
    }

    SECTION("Random starting locations (points vector should be empty, seed 1)")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "1"};
        parser.parse(ah.argc(), ah.argv());

        auto points = parser.getReleasePoints();
        REQUIRE(points.size() == 0);
        REQUIRE(parser.randomReleaseLocationSeed() == 1);
    }

    SECTION("Read from commandline")
    {
        std::stringstream p1;
        p1 << x1 << "," << y1 << std::flush;
        std::stringstream p2;
        p2 << x2 << "," << y2 << std::flush;

        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-aloc", p1.str(), "-aloc", p2.str()};
        parser.parse(ah.argc(), ah.argv());

        auto points = parser.getReleasePoints();
        REQUIRE(points.size() == 2);
        REQUIRE(points[0].x == Approx(x1));
        REQUIRE(points[0].y == Approx(y1));
        REQUIRE(points[1].x == Approx(x2));
        REQUIRE(points[1].y == Approx(y2));
    }

    SECTION("Read from file")
    {
        SelfCleaningFile scf("testpoints.csv");
        {
            std::ofstream f(scf.Filename().c_str());
            f << "x\ty\n" << x1 << "\t" << y1 << "\n"
                          << x2 << "\t" << y2 << "\n" << std::flush;
        }

        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocfile", scf.Filename()};
        parser.parse(ah.argc(), ah.argv() );

        auto points = parser.getReleasePoints();
        REQUIRE(points.size() == 2);
        REQUIRE(points[0].x == Approx(x1));
        REQUIRE(points[0].y == Approx(y1));
        REQUIRE(points[1].x == Approx(x2));
        REQUIRE(points[1].y == Approx(y2));
    }

    SECTION("Output type not set")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0"};
        parser.parse(ah.argc(), ah.argv());

        auto outputTypes = parser.outputTypes();
        REQUIRE(outputTypes.size() == 0);
    }

    SECTION("Set output type to graph")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0", "-ot", "graph"};
        parser.parse(ah.argc(), ah.argv());

        auto outputTypes = parser.outputTypes();
        REQUIRE(outputTypes.size() == 1);
        REQUIRE(outputTypes[0] == AgentParser::OutputType::GRAPH);
    }

    SECTION("Set output type to gatecounts")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0", "-ot", "gatecounts"};
        parser.parse(ah.argc(), ah.argv());

        auto outputTypes = parser.outputTypes();
        REQUIRE(outputTypes.size() == 1);
        REQUIRE(outputTypes[0] == AgentParser::OutputType::GATECOUNTS);
    }

    SECTION("Set output type to trails")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0", "-atrails", "1", "-ot", "trails"};
        parser.parse(ah.argc(), ah.argv());

        auto noOfTrails = parser.recordTrailsForAgents();
        REQUIRE(noOfTrails == 1);

        auto outputTypes = parser.outputTypes();
        REQUIRE(outputTypes.size() == 1);
        REQUIRE(outputTypes[0] == AgentParser::OutputType::TRAILS);
    }

    SECTION("Set two output types")
    {
        ArgumentHolder ah{"prog", "-ats", ats.str(), "-arr", arr.str(), "-afov", afov.str(), "-asteps", asteps.str(), "-alife", alife.str(), "-alocseed", "0", "-ot", "graph", "-ot", "gatecounts"};
        parser.parse(ah.argc(), ah.argv());

        auto outputTypes = parser.outputTypes();
        REQUIRE(outputTypes.size() == 2);
        REQUIRE(outputTypes[0] == AgentParser::OutputType::GRAPH);
        REQUIRE(outputTypes[1] == AgentParser::OutputType::GATECOUNTS);
    }

    REQUIRE(parser.totalSystemTimestemps() == totalTimeSteps);
    REQUIRE(parser.releaseRate() == Approx(releaseRate));
    REQUIRE(parser.agentFOV() == agentFOV);
    REQUIRE(parser.agentStepsBeforeTurnDecision() == agentStepsBeforeTurnDecision);
    REQUIRE(parser.agentLifeTimesteps() == agentLifeTimesteps);

}
