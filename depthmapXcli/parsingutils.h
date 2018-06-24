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

#pragma once
#define PASTE(arg) arg
#define ENFORCE_ARGUMENT(flag, counter)\
    if ( ++ PASTE(counter) >= argc \
    || (argv[ PASTE(counter)][0] == '-' && !isdigit(argv[ PASTE(counter)][1]) && argv[ PASTE(counter)][1] != '.')  )\
    {\
        throw CommandLineException(flag  " requires an argument");\
    }\

#include <vector>
#include <string>

namespace depthmapX{

    inline bool has_only_digits(const std::string &s){
      return s.find_first_not_of( "0123456789" ) == std::string::npos;
    }

    inline bool has_only_digits_dots_commas(const std::string &s){
        return s.find_first_not_of( "0123456789,.-" ) == std::string::npos;
    }

    std::vector<double> parseRadiusList(const std::string &radiusList);

}
