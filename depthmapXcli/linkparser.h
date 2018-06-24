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

#ifndef LINKPARSER_H
#define LINKPARSER_H

#include "salalib/mgraph.h"
#include "imodeparser.h"
#include "commandlineparser.h"
#include <string>
#include <vector>

class LinkParser : public IModeParser
{
public:
    virtual std::string getModeName() const
    {
        return "LINK";
    }

    virtual std::string getHelp() const
    {
        return  "Mode options for LINK:\n"\
                "-lf <links file>\n"\
                "-lnk <single link coordinates> provided in csv (x1,y1,x2,y2) for example \"0.1,0.2,0.2,0.4\" "\
                "to create a link from 0.1,0.2 to 0.2,0.4. Provide multiple times for multiple links\n";

    }

public:
    virtual void parse(int argc, char *argv[]);
    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    //link options
    const std::vector<Line> & getMergeLines() const { return _mergeLines; }
private:
    std::vector<Line> _mergeLines;
};

#endif // LINKPARSER_H
