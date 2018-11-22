// Copyright (C) 2018 Christian Sailer

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

namespace dXreimpl {
    std::vector<ConstAttributeIndexItem> makeAttributeIndex(const AttributeTable &table, int colIndex)
    {
        std::vector<ConstAttributeIndexItem> index;
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

                index.push_back(ConstAttributeIndexItem(item.getKey(), value, item.getRow()));
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

                index.push_back(ConstAttributeIndexItem(item.getKey(), value, item.getRow()));
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

    std::vector<AttributeIndexItem> makeAttributeIndex(AttributeTable &table, int colIndex)
    {
        std::vector<AttributeIndexItem> index;
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

                index.push_back(AttributeIndexItem(item.getKey(), value, item.getRow()));
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

                index.push_back(AttributeIndexItem(item.getKey(), value, item.getRow()));
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
