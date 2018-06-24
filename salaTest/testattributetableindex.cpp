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

#include "catch.hpp"
#include <salalib/attributetableindex.h>

TEST_CASE("Check index creation")
{
    using namespace dXreimpl;
    AttributeTable<SerialisedPixelRef> table;
    table.getOrInsertColumn("col1");
    table.getOrInsertColumn("col2");

    auto& row0 = table.addRow(0);
    auto& row1 = table.addRow(1);
    auto& row2 = table.addRow(2);
    auto& row3 = table.addRow(3);

    row0.setValue(0, 10.0);
    row1.setValue(0, 8.5);
    row2.setValue(0, 11.0);
    row3.setValue(0, 4.5);

    auto index = makeAttributeIndex<AttributeIndexItem<SerialisedPixelRef>>(table, 0);
    REQUIRE(index.size() == 4);
    REQUIRE(index[0].key.value == 3);
    REQUIRE(index[1].key.value == 1);
    REQUIRE(index[2].key.value == 0);
    REQUIRE(index[3].key.value == 2);

    index[3].mutable_row->setValue(1, 1.5);
    REQUIRE(table.getRow(2).getValue(1) == Approx(1.5));
}

