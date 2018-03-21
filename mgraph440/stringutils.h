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

// Collection of utility functions that are required to add pstring functionality
// to std::strings (i.e. splitting and compatible serialisation/deserialisation)
#include <vector>
#include <string>
#include <istream>
#include <ostream>

#pragma once
namespace dXstring440 {
    std::vector<std::string> split(const std::string &s, char delim, bool skipEmptyTokens = false);
    std::string readString(std::istream & stream);
    void writeString(std::ostream &stream, const std::string &s);
    std::string formatString(double value, const std::string &format = "%+.16le");
    std::string formatString(int value, const std::string &format = "% 16d");
    /// Inplace conversion to lower case
    std::string &toLower(std::string &str);
    void ltrim(std::string &s, char c = ' ');
    void rtrim(std::string &s, char c = ' ');
    void makeInitCaps(std::string &s);
    bool isDouble(const std::string &s);
    template<class T> bool beginsWith(const T &input, const T match)
    {
        return input.size() >= match.size() &&
                equal(match.begin(), match.end(), input.begin());
    }


}
