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
#include <Catch/fakeit.hpp>
#include <salalib/attributetablehelpers.h>


TEST_CASE("push to layer")
{
    using namespace dXreimpl;
    using namespace fakeit;
    Mock<LayerManager> layMan;
    AttributeTable table;
    When(Method(layMan,addLayer)).Do([](const std::string &name)->size_t{REQUIRE(name == "testlayer"); return 1;});
    When(Method(layMan,getKey).Using(1)).AlwaysReturn(2);
    When(Method(layMan,isVisible).Using(1)).AlwaysReturn(true);
    When(Method(layMan,setLayerVisible)).AlwaysReturn();

    table.insertOrResetColumn("col1");
    table.getOrInsertColumn("col2");

    auto& row = table.addRow(AttributeKey(0));
    row.setValue(0, 0.5f);
    row.setSelection(true);
    auto& row2 = table.addRow(AttributeKey(1));
    row2.setValue(0, 1.0f);

   pushSelectionToLayer(table, layMan.get(), "testlayer");
   Verify(Method(layMan,addLayer)).Once();
   Verify(Method(layMan,getKey).Using(1)).Once();
   Verify(Method(layMan,setLayerVisible).Using(1, true)).Once();

   REQUIRE(row.getLayerKey()== 3);
   REQUIRE(row2.getLayerKey() == 1);
}

