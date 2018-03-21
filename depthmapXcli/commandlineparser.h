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

#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

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
    void parse(size_t argc, char *argv[]);

    const std::string &getFileName() const { return _fileName; }
    const std::string &getOuputFile() const {return _outputFile;}
    const std::string &getTimingFile() const {return _timingFile;}
    bool isValid() const { return _valid; }
    bool printVersionMode() const { return _printVersionMode; }
    bool simpleMode() const { return _simpleMode; }
    const IModeParser& modeOptions() const{ return *_modeParser;};


    void printHelp();
    void printVersion();
    void run(IPerformanceSink &perfWriter) const;


private:
    std::string _fileName;
    std::string _outputFile;
    std::string _timingFile;
    bool _valid;
    bool _printVersionMode;
    bool _simpleMode;

    const IModeParserFactory &_parserFactory;
    IModeParser * _modeParser;

};

#endif // COMMANDLINEPARSER_H
