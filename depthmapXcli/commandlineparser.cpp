// Copyright (C) 2017 Christian Sailer
// Copyright (C) 2011-2026, Tasos Varoudis

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

#include "commandlineparser.h"
#include "exceptions.h"
#include "imodeparserfactory.h"
#include "parsingutils.h"
#include "performancewriter.h"
#include "genlib/comm.h"
#include "version.h"
#include <exception>
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace depthmapX;

void CommandLineParser::printHelp(){
    std::cout << "Usage: depthmapXcli -m <mode> -f <filename> -o <output file> [-s] [-t <times.csv>] [-p] [mode options]\n"
              << "       depthmapXcli -v prints the current version\n"
              << "       depthmapXcli -h prints this help text\n"
              << "-s enables simple mode\n"
              << "-t <times.csv> enables output of runtimes as csv file\n"
              << "-p enables text progress printing\n"

              << "Possible modes are:\n";
              std::for_each(_parserFactory.getModeParsers().begin(), _parserFactory.getModeParsers().end(), [](const ModeParserVec::value_type &p)->void{ std::cout << "  " << p->getModeName() << "\n"; });
              std::cout << "\n";
              std::for_each(_parserFactory.getModeParsers().begin(), _parserFactory.getModeParsers().end(), [](const ModeParserVec::value_type &p)->void{ std::cout << p->getHelp() << "\n"; });
              std::cout << std::flush;
}

void CommandLineParser::printVersion(){
    std::cout << TITLE_BASE << "\n" << std::flush;
}


CommandLineParser::CommandLineParser(const IModeParserFactory &parserFactory)
    :  m_simpleMode(false), _parserFactory(parserFactory), _modeParser(0)
{}

int CommandLineParser::execute(int argc, char *argv[], std::ostream &errors) {
    try {
        parse(argc, argv);
        if (!isValid()) {
            if (printVersionMode()) {
                printVersion();
            } else {
                printHelp();
            }
            return 0;
        }

        PerformanceWriter perfWriter(getTimingFile());
        run(perfWriter);
        perfWriter.write();
        return 0;
    } catch (const Communicator::CancelledException &) {
        errors << "Analysis cancelled.\n";
    } catch (const std::exception &error) {
        const char *message = error.what();
        errors << (*message ? message : "An error occurred during analysis.")
               << "\nType 'depthmapXcli -h' for help\n";
    } catch (...) {
        errors << "An unknown error occurred during analysis.\n";
    }
    return -1;
}

void CommandLineParser::parse(size_t argc, char *argv[])
{
    m_valid = false;
    m_printVersionMode = false;
    if (argc <= 1)
    {
        throw CommandLineException("No commandline parameters provided - don't know what to do");
    }
    for ( size_t i = 1; i < argc;  )
    {
        if ( std::strcmp("-h", argv[i])== 0)
        {
            return;
        }
        else if ( std::strcmp("-v", argv[i])== 0)
        {
            m_printVersionMode = true;
            return;
        }
        else if ( std::strcmp ("-m", argv[i]) == 0)
        {
            if ( _modeParser)
            {
                throw CommandLineException("-m can only be used once");
            }
            ENFORCE_ARGUMENT("-m", i)

            for (auto iter = _parserFactory.getModeParsers().begin(), end = _parserFactory.getModeParsers().end();
                 iter != end; ++iter )
            {
                if ((*iter)->getModeName() == argv[i])
                {
                    _modeParser = iter->get();
                    break;
                }
            }

            if (!_modeParser)
            {
                throw CommandLineException(std::string("Invalid mode: ") + argv[i]);
            }
        }
        else if ( std::strcmp ("-f", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-f", i)
            m_fileName = argv[i];
        }
        else if ( std::strcmp ("-o", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-o", i)
            m_outputFile = argv[i];
        }
        else if ( std::strcmp ("-t", argv[i]) == 0)
        {
            ENFORCE_ARGUMENT("-t", i)
            m_timingFile = argv[i];
        }
        else if ( std::strcmp("-s", argv[i]) == 0)
        {
            m_simpleMode = true;
        }
        else if ( std::strcmp("-p", argv[i]) == 0)
        {
            m_printProgress = true;
        }
        ++i;
    }

    if (!_modeParser)
    {
        throw CommandLineException("-m for mode is required");
    }
    if (m_fileName.empty())
    {
        throw CommandLineException("-f for input file is required");
    }
    if (m_outputFile.empty())
    {
        throw CommandLineException("-o for output file is required");
    }
    _modeParser->parse(argc, argv);
    m_valid = true;
}

void CommandLineParser::run(IPerformanceSink &perfWriter) const
{
    if (!m_valid || !_modeParser)
    {
        throw CommandLineException("Trying to run with invalid command line parameters");
    }
    _modeParser->run(*this, perfWriter);
}

