// Copyright (C) 2017 Christian Sailer
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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>

namespace depthmapX440
{
    class BaseException : public std::exception
    {
    public:
        BaseException(){}
        BaseException(std::string message) : _message(message)
        {}
        virtual const char * what() const noexcept
        {
            return _message.c_str();
        }
    private:
        std::string _message;
    };

    class RuntimeException: public BaseException
    {
    public:
        RuntimeException(std::string message) : BaseException(message)
        {}
    };
}
#endif // EXCEPTIONS_H
