// genlib - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2017, Petros Koutsolampros

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

#include <map>
#include <algorithm>

namespace depthmapX {

template<typename T>
void findAndErase(std::vector<T> &vec, T element) {
    auto it = std::find(vec.begin(), vec.end(), element);
    if(it != vec.end())
        vec.erase(it);
}

template<typename T>
void addIfNotExists(std::vector<T> &vec, T element) {
    auto it = std::find(vec.begin(), vec.end(), element);
    if(it == vec.end())
        vec.push_back(element);
}


template<typename K, typename V>
bool addIfNotExists(std::map<K, V> &map, const K &key, const V &value) {
    auto it = map.find(key);
    if(it == map.end()) {
        map[key] = value;
        return true;
    }
    return false;
}

template<typename K, typename V>
typename std::map<K, V>::const_iterator getMapAtIndex(const std::map<K, V> &m, size_t idx) {
    auto iter = m.begin();
    std::advance(iter, idx);
    return iter;
}

template<typename K, typename V>
typename std::map<K, V>::iterator getMapAtIndex(std::map<K, V> &m, size_t idx) {
    auto iter = m.begin();
    std::advance(iter, idx);
    return iter;
}

template<typename K, typename V>
int findIndexFromKey(const std::map<K, V> &m, K key) {
    auto iter = m.find(key);
    return iter == m.end() ? -1 : std::distance(m.begin(), iter);
}

template< typename TContainer, typename TValue> typename TContainer::iterator findBinary( TContainer& container, const TValue val){
    auto res = std::lower_bound(container.begin(), container.end(), val);
    if (res  == container.end() || val < *res )
    {
        return container.end();
    }
    return res;
}

template< typename TContainer, typename TValue> typename TContainer::const_iterator findBinary( const TContainer& container, const TValue val){
    auto res = std::lower_bound(container.begin(), container.end(), val);
    if (res  == container.end() || val < *res )
    {
        return container.end();
    }
    return res;
}

template< typename T >
typename std::vector<T>::iterator
   insert_sorted( std::vector<T> & vec, T const& item ) {
    return vec.insert(
            std::upper_bound( vec.begin(), vec.end(), item ),
            item);
}


}
