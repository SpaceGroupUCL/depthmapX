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
typename std::map<K, V>::const_iterator getMapAtIndex(const std::map<K, V> &m, int idx) {
    auto iter = m.begin();
    std::advance(iter, idx);
    return iter;
}

template<typename K, typename V>
typename std::map<K, V>::iterator getMapAtIndex(std::map<K, V> &m, int idx) {
    auto iter = m.begin();
    std::advance(iter, idx);
    return iter;
}

template<typename K, typename V>
int findIndexFromKey(const std::map<K, V> &m, K key) {
    auto iter = m.find(key);
    return iter == m.end() ? -1 : std::distance(m.begin(), iter);
}

template<typename K, typename V>
int insertAndGetIndex(std::map<K, V> &m, K key, V value) {
    m.insert(std::make_pair(key, value));
    return findIndexFromKey(m, key);
}

}
