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

#include "parsingutils.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include "exceptions.h"

std::vector<double> depthmapX::parseRadiusList(const std::string &radiusList)
{
    std::vector<double> result;
    std::stringstream stream(radiusList);
    bool addN = false;

    while(stream.good())
    {
        std::string value;
        getline(stream, value, ',');
        if ( value == "n" || value == "N")
        {
            addN = true;
        }
        else
        {
            char *end;
            long int val = std::strtol(value.c_str(), &end, 10 );
            if (val == 0 )
            {
                std::stringstream message;
                message << "Found either 0 or unparsable radius " << value << std::flush;
                throw CommandLineException(message.str());
            }
            if (val < 0)
            {
                throw CommandLineException("Radius must be either n or a positive integer");
            }
            if (strlen(end) > 0)
            {
                std::stringstream message;
                message << "Found non integer radius " << value << std::flush;
                throw CommandLineException(message.str());
            }
            result.push_back((double)val);
        }
    }

    std::sort(result.begin(), result.end());
    if (result.empty() || addN)
    {
        result.push_back(-1.0);
    }
    return result;
}
