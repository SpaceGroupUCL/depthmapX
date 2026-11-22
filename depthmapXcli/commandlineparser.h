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

#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include <memory>
class IModeParserFactory;
class IModeParser;
class IPerformanceSink;



class CommandLineParser
{
public:
    CommandLineParser(const IModeParserFactory &parserFactory);
    int execute(int argc, char *argv[], std::ostream &errors);
    void parse(size_t argc, char *argv[]);

    const std::string &getFileName() const { return m_fileName; }
    const std::string &getOuputFile() const {return m_outputFile;}
    const std::string &getTimingFile() const {return m_timingFile;}
    bool isValid() const { return m_valid; }
    bool printVersionMode() const { return m_printVersionMode; }
    bool simpleMode() const { return m_simpleMode; }
    bool printProgress() const { return m_printProgress; }
    const IModeParser& modeOptions() const{ return *_modeParser;};


    void printHelp();
    void printVersion();
    void run(IPerformanceSink &perfWriter) const;


private:
    std::string m_fileName;
    std::string m_outputFile;
    std::string m_timingFile;
    bool m_valid;
    bool m_printVersionMode;
    bool m_simpleMode;
    bool m_printProgress = false;

    const IModeParserFactory &_parserFactory;
    IModeParser * _modeParser;

};
