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
#include "Catch/fakeit.hpp"
#include "../depthmapXcli/commandlineparser.h"
#include "../depthmapXcli/imodeparser.h"
#include "../depthmapXcli/imodeparserfactory.h"
#include <cstring>
#include <sstream>
#include "argumentholder.h"


using namespace fakeit;

class TestParser: public IModeParser
{
public:
    TestParser(const std::string &modeName) : _parseCalled(false), _runCalled(false), _modeName(modeName)
    {
    }

    virtual std::string getModeName()const
    {
        return _modeName;
    }

    virtual std::string getHelp() const
    {
        return formatTestHelpString(_runCalled, _parseCalled);
    }

    virtual void parse(int , char ** )
    {
        _parseCalled = true;
    }

    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
    {
        _runCalled = true;
    }

    static std::string formatTestHelpString(bool runCalled, bool parseCalled)
    {
        std::stringstream buf;
        buf << "runCalled " << (runCalled ? "yes" : "no")
            << " : parseCalled "  << (parseCalled? "yes": "no")
            << std::flush;
        return buf.str();
    }

private:
    bool _parseCalled;
    mutable bool _runCalled;
    std::string _modeName;
};


TEST_CASE("Invalid Parser","Constructor"){
    std::vector<std::unique_ptr<IModeParser> > parsers;
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST1"))));
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST2"))));

    Mock<IModeParserFactory> factoryMock;
    When(Method(factoryMock,getModeParsers)).AlwaysReturn(parsers);

    {
        CommandLineParser cmdP(factoryMock.get());
        REQUIRE_THROWS_WITH(cmdP.parse(0, 0), Catch::Contains("No commandline parameters provided - don't know what to do"));
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "-m requires an argument");
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-f"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "-f requires an argument");
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-o"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "-o requires an argument");
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-t", "-o"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "-t requires an argument");
    }


    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "-f"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "-m requires an argument");
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "LaLaLa"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), "Invalid mode: LaLaLa");
    }
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-f", "inputfile.graph", "-o", "outputfile.graph"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), Catch::Contains("-m for mode is required"));
    }
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-m", "TEST2","-f", "inputfile.graph"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), Catch::Contains("-m can only be used once"));
    }

    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-o", "outputfile.graph"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), Catch::Contains("-f for input file is required"));
    }
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-f", "inputfile.graph"};
        REQUIRE_THROWS_WITH(cmdP.parse(ah.argc(), ah.argv()), Catch::Contains("-o for output file is required"));
    }

}

TEST_CASE("Valid Parser","CheckValues"){
    std::vector<std::unique_ptr<IModeParser> > parsers;
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST1"))));
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST2"))));

    Mock<IModeParserFactory> factoryMock;
    When(Method(factoryMock,getModeParsers)).AlwaysReturn(parsers);

    SECTION("Parser test2 used")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST2", "-f", "inputfile.graph", "-o", "outputfile.graph", "-lnk", "1.2,3.4,5.6,7.8"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.isValid());
        REQUIRE(cmdP.getTimingFile().empty());
        REQUIRE(parsers[0]->getHelp() == TestParser::formatTestHelpString(false, false));
        REQUIRE(parsers[1]->getHelp() == TestParser::formatTestHelpString(false, true));
    }
    SECTION("Parser test1 used")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.isValid());
        REQUIRE_FALSE(cmdP.simpleMode());
        REQUIRE(cmdP.getTimingFile().empty());
        REQUIRE(cmdP.modeOptions().getModeName() == "TEST1");
        REQUIRE(parsers[0]->getHelp() == TestParser::formatTestHelpString(false, true));
        REQUIRE(parsers[1]->getHelp() == TestParser::formatTestHelpString(false, false));
    }
    SECTION("Parser test1 used, timings file, simple mode")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph", "-s", "-t", "timings.csv"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.isValid());
        REQUIRE(cmdP.simpleMode());
        REQUIRE(cmdP.getTimingFile() == "timings.csv");
        REQUIRE(parsers[0]->getHelp() == TestParser::formatTestHelpString(false, true));
        REQUIRE(parsers[1]->getHelp() == TestParser::formatTestHelpString(false, false));
    }

}

TEST_CASE("Run Tests","Check we only run if it's appropriate"){
    std::vector<std::unique_ptr<IModeParser> > parsers;
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST1"))));
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST2"))));

    Mock<IModeParserFactory> factoryMock;
    When(Method(factoryMock,getModeParsers)).AlwaysReturn(parsers);

    Mock<IPerformanceSink> perfSink;

    SECTION("Fail - run without parsing")
    {
        CommandLineParser cmdP(factoryMock.get());
        REQUIRE_THROWS_WITH(cmdP.run(perfSink.get()), Catch::Contains("Trying to run with invalid command line parameters"));
    }

    SECTION("Fail run with help on the command line")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-h", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(cmdP.isValid());
        REQUIRE_THROWS_WITH(cmdP.run(perfSink.get()), Catch::Contains("Trying to run with invalid command line parameters"));
    }

    SECTION("Fail run with version on the command line")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-v", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(cmdP.isValid());
        REQUIRE_THROWS_WITH(cmdP.run(perfSink.get()), Catch::Contains("Trying to run with invalid command line parameters"));
    }

    SECTION("Fail run with version on the command line as last parameter")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph", "-v"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(cmdP.isValid());
        REQUIRE_THROWS_WITH(cmdP.run(perfSink.get()), Catch::Contains("Trying to run with invalid command line parameters"));
    }

    SECTION("Fail run without sub parser")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog" "-f", "inputfile.graph", "-o", "outputfile.graph"};
        REQUIRE_THROWS(cmdP.parse(ah.argc(), ah.argv()));
        REQUIRE_FALSE(cmdP.isValid());
        REQUIRE_THROWS_WITH(cmdP.run(perfSink.get()), Catch::Contains("Trying to run with invalid command line parameters"));
    }


    SECTION("Parser test1 used, timings file, simple mode")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{"prog", "-m", "TEST1", "-f", "inputfile.graph", "-o", "outputfile.graph", "-s", "-t", "timings.csv"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.isValid());
        REQUIRE(cmdP.simpleMode());
        REQUIRE(cmdP.getTimingFile() == "timings.csv");
        REQUIRE(parsers[0]->getHelp() == TestParser::formatTestHelpString(false, true));
        REQUIRE(parsers[1]->getHelp() == TestParser::formatTestHelpString(false, false));
        cmdP.run(perfSink.get());
        REQUIRE(parsers[0]->getHelp() == TestParser::formatTestHelpString(true, true));
        REQUIRE(parsers[1]->getHelp() == TestParser::formatTestHelpString(false, false));

    }

}


TEST_CASE("Invalid Parser Need Help", "CheckForHelp")
{
    std::vector<std::unique_ptr<IModeParser> > parsers;
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST1"))));
    parsers.push_back(std::move(std::unique_ptr<IModeParser>(new TestParser("TEST2"))));

    Mock<IModeParserFactory> factoryMock;
    When(Method(factoryMock,getModeParsers)).AlwaysReturn(parsers);

    SECTION("Help requested")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{ "prog", "-h"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(cmdP.isValid());
    }
    SECTION("Version requested")
    {
        CommandLineParser cmdP(factoryMock.get());
        ArgumentHolder ah{ "prog", "-v"};
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE_FALSE(cmdP.isValid());
    }
}
