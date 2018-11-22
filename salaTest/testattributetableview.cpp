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

#include <catch.hpp>
#include <salalib/attributetableview.h>

TEST_CASE("Test Attribute view"){
    using namespace dXreimpl;
    AttributeTable table;

    table.insertOrResetColumn("foo");
    table.insertOrResetColumn("bar");
    table.addRow(AttributeKey(0)).setValue(0,1.0f).setValue(1, 1.1f);
    table.addRow(AttributeKey(7)).setValue(0,0.7f).setValue(1,1.7f);

    AttributeTableView view(table);
    view.setDisplayColIndex(0);

    REQUIRE(view.getConstTableIndex().front().key.value == 7);

    REQUIRE(view.getNormalisedValue(view.getConstTableIndex().front().key, *view.getConstTableIndex().front().row) == Approx(0.0f));

    REQUIRE(&view.getDisplayParams() != &table.getDisplayParams());
    REQUIRE(&view.getDisplayParams() == &table.getColumn(0).getDisplayParams());


    table.addRow(AttributeKey(3));
    view.setDisplayColIndex(-1);
    REQUIRE(view.getNormalisedValue(AttributeKey(3), table.getRow(AttributeKey(3))) == Approx(3.0/7));
    REQUIRE(view.getConstTableIndex().size() == 3);

    REQUIRE(&table.getDisplayParams() == &view.getDisplayParams());

    view.setDisplayColIndex(-2);
    REQUIRE(view.getNormalisedValue(AttributeKey(3), table.getRow(AttributeKey(3))) == Approx(3.0/7));
    REQUIRE(view.getConstTableIndex().empty());

    REQUIRE(&table.getDisplayParams() == &view.getDisplayParams());

}


TEST_CASE("Test attribute table handle")
{
    using namespace  dXreimpl;
    AttributeTable table;

    table.insertOrResetColumn("foo");
    table.insertOrResetColumn("bar");
    table.addRow(AttributeKey(0)).setValue(0,1.0f).setValue(1, 1.1f);
    table.addRow(AttributeKey(7)).setValue(0,0.7f).setValue(1,1.7f);

    AttributeTableHandle handle(table);
    handle.setDisplayColIndex(0);

    REQUIRE(handle.getTableIndex().front().key.value == 7);
    REQUIRE(handle.getConstTableIndex().front().key.value == 7);


    handle.getTableIndex().front().mutable_row->setValue(0, 0.8f);

    REQUIRE(table.getRow(AttributeKey(7)).getValue(0) == Approx(0.8));

    handle.setDisplayColIndex(-1);
    REQUIRE(handle.getTableIndex().size() == 2);

    REQUIRE(&table.getDisplayParams() == &handle.getDisplayParams());

    handle.setDisplayColIndex(-2);
    REQUIRE(handle.getTableIndex().empty());

    REQUIRE(&table.getDisplayParams() == &handle.getDisplayParams());


}
