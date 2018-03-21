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
#include <algorithm>
#include "mgraph440/paftl.h"

namespace genshim440
{
    /**
     * Convert a std::vector to a pvec (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class T> mgraph440::pvector<T> toPVector(const std::vector<T> &vec)
    {
        mgraph440::pvector<T> pvec;
        std::for_each(vec.begin(), vec.end(), [&pvec](const T& val)->void{pvec.push_back(val);});
        return pvec;
    }

    /**
     * Convert a pvec to a std::vector (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class T> std::vector<T> toSTLVector(mgraph440::pvector<T> &pvec)
    {
        std::vector<T> vec;
        for(int i = 0; i < pvec.size(); i++)
        {
            vec.push_back(pvec[i]);
        }
        return vec;
    }

    /**
     * Convert a std::vector to a pqvector (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class T> mgraph440::pqvector<T> toPQVector(const std::vector<T> &vec)
    {
        mgraph440::pqvector<T> pvec;
        std::for_each(vec.begin(), vec.end(), [&pvec](const T& val)->void{pvec.push_back(val);});
        return pvec;
    }
}
