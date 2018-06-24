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

#include <vector>
#include <iostream>
#include "exceptions.h"

namespace dXvector
{
    // The read/write methods can only be used for vectors of stack allocated types (basic data, POD)
    // read in vector data and write to an existing vector (overwriting its previous contents)
    template<typename T> size_t readIntoVector(std::istream& stream, std::vector<T>& vec)
    {
        vec.clear();
        unsigned int size;
        stream.read((char *)&size, sizeof(size));
        if (size > 0)
        {
            vec.resize(size);
            stream.read((char *)vec.data(), sizeof(T) * std::streamsize(size));
        }
        return size;
    }
    // read in a vector into a new vector
    template<typename T> std::vector<T> readVector(std::istream& stream)
    {
        std::vector<T> vec;
        readIntoVector(stream,vec);
        return vec;
    }

    template<typename T> void writeVector(std::ostream& stream, const std::vector<T>& vec)
    {
        // READ / WRITE USES 32-bit LENGTHS (number of elements) for compatibility reasons

        if ( vec.size() > size_t((unsigned int)-1))
        {
            throw new depthmapX::RuntimeException("Vector exceeded max size for streaming");
        }
        unsigned int length = (unsigned int)vec.size();
        stream.write((char *)&length, sizeof(length));
        if (length > 0)
        {
            stream.write((char *)vec.data(), sizeof(T) * std::streamsize(length));
        }
    }
}
