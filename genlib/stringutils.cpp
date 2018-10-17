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

#include "stringutils.h"
#include <sstream>
#include <memory>
#include <cstring>
#include <algorithm>
#include <ctype.h>

namespace dXstring {
    std::vector<std::string> split(const std::string &s, char delim, bool skipEmptyTokens)
    {
        std::vector<std::string> elems;
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            if (skipEmptyTokens && item.empty())
            {
                continue;
            }
            elems.push_back(item);
        }

        return elems;
    }

    std::string readString(std::istream & stream)
    {
        unsigned int length;
        stream.read(reinterpret_cast<char *>(&length), sizeof(length));
        if ( length == 0)
        {
            return std::string();
        }
        std::string result( length, '\0');
        char *ptr  = &result[0];
        stream.read(ptr, length);
        return result;
    }

    void writeString(std::ostream &stream, const std::string &s)
    {
        unsigned int length = static_cast<unsigned int>(s.length());
        stream.write(reinterpret_cast<char *>(&length), sizeof(unsigned int));
        if (length > 0)
        {
            stream.write(s.data(), length);
        }
    }

    std::string formatString(double value, const std::string &format)
    {
        size_t bufferLength = 24 + format.length();
        std::vector<char> buffer(bufferLength, '\0');
        snprintf( &buffer[0], bufferLength, format.c_str(), value );
        return std::string(&buffer[0]);
    }

    std::string formatString(int value, const std::string &format)
    {
        size_t bufferLength = 24 + format.length();
        std::vector<char> buffer(bufferLength, '\0');
        snprintf( &buffer[0], bufferLength, format.c_str(), value );
        return std::string(&buffer[0]);
    }


    std::string& toLower(std::string &str)
    {
        std::transform(str.begin(), str.end(), str.begin(), tolower);
        return str;
    }

    // trim from start (in place)
    void ltrim(std::string &s, char c) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&c](int ch) {
            return ch != c;
        }));
    }

    // trim from end (in place)
    void rtrim(std::string &s, char c) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [&c](int ch) {
            return ch != c;
        }).base(), s.end());
    }

    void makeInitCaps(std::string &s)
    {
        bool literal = false;
        bool reset = true;
        for( auto& c : s)
        {
            if (!isalpha(c))
            {
                if ( c == '"')
                {
                    literal = !literal;
                }
                reset = true;
            }
            else
            {
                if (!literal)
                {
                    if (reset)
                    {
                        c = static_cast<char>(toupper(c));
                    }
                    else
                    {
                        c = static_cast<char>(tolower(c));
                    }
                }
                reset = false;
            }
        }
    }

    bool isDouble(const std::string &s)
    {
        // nasty const cast to satisfy the function signature - we will not change the value of endPtr
        char *endPtr = const_cast<char *>(&s[0]);
        strtod(s.c_str(), &endPtr);
        return endPtr != &s[0];
    }

    // handles all three line endings ("\r", "\n" and "\r\n"). taken from:
    // https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
    std::istream& safeGetline(std::istream& is, std::string& t)
    {
        t.clear();

        // The characters in the stream are read one-by-one using a std::streambuf.
        // That is faster than reading them one-by-one using the std::istream.
        // Code that uses streambuf this way must be guarded by a sentry object.
        // The sentry object performs various tasks,
        // such as thread synchronization and updating the stream state.

        std::istream::sentry se(is, true);
        std::streambuf* sb = is.rdbuf();

        for(;;) {
            int c = sb->sbumpc();
            switch (c) {
            case '\n':
                return is;
            case '\r':
                if(sb->sgetc() == '\n')
                    sb->sbumpc();
                return is;
            case std::streambuf::traits_type::eof():
                // Also handle the case when the last line has no line ending
                if(t.empty())
                    is.setstate(std::ios::eofbit);
                return is;
            default:
                t += (char)c;
            }
        }
    }
}
