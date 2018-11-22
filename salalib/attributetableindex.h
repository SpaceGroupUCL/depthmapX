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
    class ConstAttributeIndexItem
    {
    public:
        ConstAttributeIndexItem(const AttributeKey &k, double v, const AttributeRow &r) : key(k), value(v), row(&r)
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

        AttributeKey key;
        double value;
        const AttributeRow *  row;
    };

    class AttributeIndexItem : public ConstAttributeIndexItem
    {
    public:
        AttributeIndexItem( const AttributeKey &k, double v, AttributeRow &r) : ConstAttributeIndexItem(k,v,r), mutable_row(&r)
        {}
        AttributeIndexItem( const AttributeIndexItem &other) : ConstAttributeIndexItem(other), mutable_row(other.mutable_row)
        {}
        AttributeIndexItem &operator = (const AttributeIndexItem &other)
        {
            if ( this == &other)
            {
                return *this;
            }
            ConstAttributeIndexItem::operator =(other);
            mutable_row = other.mutable_row;
            return *this;
        }

        AttributeRow *mutable_row;
    };

    inline bool operator < (const ConstAttributeIndexItem &lhs, const ConstAttributeIndexItem &rhs)
    {
        return lhs.value < rhs.value;
    }

    std::vector<ConstAttributeIndexItem> makeAttributeIndex(const AttributeTable &table, int colIndex);
    std::vector<AttributeIndexItem> makeAttributeIndex(AttributeTable &table, int colIndex);
}
