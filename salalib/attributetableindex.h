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
#include "attributetable.h"
#include <algorithm>

namespace dXreimpl
{
    template<typename RowKeyType>
    class ConstAttributeIndexItem
    {
    public:
        ConstAttributeIndexItem(const RowKeyType &k, double v, const AttributeRow &r) : key(k), value(v), row(&r)
        {}

        ConstAttributeIndexItem(const ConstAttributeIndexItem& other) : key(other.key), value(other.value), row(other.row)
        {}
        ConstAttributeIndexItem &operator = (const ConstAttributeIndexItem& other)
        {
            if ( this == &other)
            {
                return *this;
            }
            key = other.key;
            value = other.value;
            row = other.row;
            return *this;
        }

        RowKeyType key;
        double value;
        const AttributeRow *  row;
    };

    template<typename RowKeyType>
    class AttributeIndexItem : public ConstAttributeIndexItem<RowKeyType>
    {
    public:
        AttributeIndexItem( const RowKeyType &k, double v, AttributeRow &r) : ConstAttributeIndexItem<RowKeyType>(k,v,r), mutable_row(&r)
        {}
        AttributeIndexItem( const AttributeIndexItem &other) : ConstAttributeIndexItem<RowKeyType>(other), mutable_row(other.mutable_row)
        {}
        AttributeIndexItem &operator = (const AttributeIndexItem &other)
        {
            if ( this == &other)
            {
                return *this;
            }
            ConstAttributeIndexItem<RowKeyType>::operator =(other);
            mutable_row = other.mutable_row;
            return *this;
        }

        AttributeRow *mutable_row;
    };

    template<typename T1>
    bool operator < (const ConstAttributeIndexItem<T1> &lhs, const ConstAttributeIndexItem<T1> &rhs)
    {
        return lhs.value < rhs.value;
    }

    template<typename ItemType, typename TableType>
    std::vector<ItemType> makeAttributeIndex(TableType &table, int colIndex)
    {
        std::vector<ItemType> index;
        size_t numRows = table.getNumRows();
        if (numRows == 0)
        {
            return index;
        }
        index.reserve(numRows);
        // perturb the values to be sorted by so same values will be in order of appearence in the map
        size_t idx = 0;
        if ( colIndex == -1 )
        {
            double perturbationFactor = 1e-9 / numRows;
            for (auto& item: table)
            {
                double value = (double)item.getKey().value;
                value += idx * perturbationFactor;

                index.push_back(ItemType(item.getKey(), value, item.getRow()));
                ++idx;
            }
        }
        else if (colIndex >= 0 )
        {
            double perturbationFactor = table.getColumn(colIndex).getStats().max * 1e-9 / numRows;
            for (auto & item : table)
            {
                double value = item.getRow().getValue(colIndex);
                value += idx * perturbationFactor;

                index.push_back(ItemType(item.getKey(), value, item.getRow()));
                ++idx;
            }
        }
        else
        {
            throw std::out_of_range("Column index out of range");
        }
        std::sort(index.begin(), index.end());
        return index;
    }
}
