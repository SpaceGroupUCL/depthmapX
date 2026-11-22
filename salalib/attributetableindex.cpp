// Copyright (C) 2018 Christian Sailer
// Copyright (C) 2011-2026, Tasos Varoudis

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


#include "salalib/attributetableindex.h"

#include <limits>

namespace {
template <typename IndexItem, typename Table>
std::vector<IndexItem> buildAttributeIndex(Table &table, int colIndex)
{
    if (colIndex < -1) {
        throw std::out_of_range("Column index out of range");
    }
    std::vector<IndexItem> index;
    index.reserve(table.getNumRows());
    for (auto &item : table) {
        const auto &key = item.getKey();
        auto &row = item.getRow();
        double value = key.value;
        if (colIndex >= 0) {
            value = row.hasValue(colIndex) ? row.getValue(colIndex) : -std::numeric_limits<double>::infinity();
        }
        index.emplace_back(key, value, row);
    }
    // Keep the original row order for equal values, including missing cells.
    std::stable_sort(index.begin(), index.end());
    return index;
}
} // namespace

std::vector<ConstAttributeIndexItem> makeAttributeIndex(const AttributeTable &table, int colIndex)
{
    return buildAttributeIndex<ConstAttributeIndexItem>(table, colIndex);
}

std::vector<AttributeIndexItem> makeAttributeIndex(AttributeTable &table, int colIndex)
{
    return buildAttributeIndex<AttributeIndexItem>(table, colIndex);
}

std::pair<std::vector<AttributeIndexItem>::iterator, std::vector<AttributeIndexItem>::iterator>
getIndexItemsInValueRange(std::vector<AttributeIndexItem> &index, AttributeTable &table, float fromValue,
                          float toValue) {
    AttributeKey dummykey(-1);
    AttributeRowImpl dummyrow(table);
    return std::pair<std::vector<AttributeIndexItem>::iterator, std::vector<AttributeIndexItem>::iterator>(
        std::lower_bound(index.begin(), index.end(), AttributeIndexItem(dummykey, fromValue, dummyrow)),
        std::upper_bound(index.begin(), index.end(), AttributeIndexItem(dummykey, toValue, dummyrow)));
}
