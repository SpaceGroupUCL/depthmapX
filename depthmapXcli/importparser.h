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

#pragma once

#include "imodeparser.h"
#include "commandlineparser.h"
#include <string>
#include <vector>

class ImportParser : public IModeParser
{
public:
    virtual std::string getModeName() const
    {
        return "IMPORT";
    }

    virtual std::string getHelp() const
    {
        return  "Mode options for IMPORT:\n"\
                "   The file provided by -f here will be used as the base. If that file"\
                "is not a graph, a new graph will be created and the file will be imported\n"\
                "   -if <file(s) to import> one or more files to import\n"\
                "   -iaa will import and attach attributes to an existing map\n";
    }

public:
    virtual void parse(int argc, char *argv[]);
    virtual void run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const;

    const std::vector<std::string> & getFilesToImport() const { return m_filesToImport; }
    const bool toImportAsAttrbiutes() const { return m_importAsAttributes; }

private:
    std::vector<std::string> m_filesToImport;
    bool m_importAsAttributes = false;
};
