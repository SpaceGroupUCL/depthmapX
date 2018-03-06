// Copyright (C) 2017 Christian Sailer
// Copyright (C) 2018 Petros Koutsolampros

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
#include <map>
#include "paftl.h"

namespace genshim
{
    /**
     * Convert a std::vector to a pvec (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class T> pvector<T> toPVector(const std::vector<T> &vec)
    {
        pvector<T> pvec;
        std::for_each(vec.begin(), vec.end(), [&pvec](const T& val)->void{pvec.push_back(val);});
        return pvec;
    }
  
    /**
     * Convert a pvec to a std::vector (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class T> std::vector<T> toSTLVector(pvector<T> &pvec)
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
    template<class T> pqvector<T> toPQVector(const std::vector<T> &vec)
    {
        pqvector<T> pvec;
        std::for_each(vec.begin(), vec.end(), [&pvec](const T& val)->void{pvec.push_back(val);});
        return pvec;
    }

    /**
     * Convert a pmap to a std::map (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class K, class V> std::map<K, V> toSTLMap(pmap<K, V> &pm)
    {
        std::map<K, V> m;
        for(int i = 0; i < pm.size(); i++) {
            m.insert(std::make_pair(pm.key(i), pm.value(i)));
        }
        return m;
    }

    /**
     * Convert a std::map to a pmap (preserving the order of elements)
     * This is expensive as it copies every single element
     */
    template<class K, class V> pmap<K, V> toPMap(const std::map<K, V> &m)
    {
        pmap<K, V> pm;
        for(auto pair: m) { pm.add(pair.first, pair.second); }
        return pm;
    }
}
