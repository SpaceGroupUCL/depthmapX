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
#include "Catch/fakeit.hpp"
#include <salalib/attributetable.h>
#include <cliTest/selfcleaningfile.h>
#include <fstream>
#include <salalib/mgraph_consts.h>
#include <salalib/layermanagerimpl.h>

TEST_CASE("test attribute column")
{
    using namespace dXreimpl;
    AttributeColumnImpl col("colName");
    REQUIRE(col.getName() == "colName");
    REQUIRE(col.getFormula() == "");

    REQUIRE_FALSE(col.isHidden());
    REQUIRE_FALSE(col.isLocked());

    col.setLock(true);
    REQUIRE_FALSE(col.isHidden());
    REQUIRE(col.isLocked());

    col.setHidden(true);
    REQUIRE(col.isHidden());
    REQUIRE(col.isLocked());

    REQUIRE(col.m_stats.max == -1.0);
    REQUIRE(col.m_stats.min == -1.0);
    REQUIRE(col.m_stats.total == -1.0);
    REQUIRE(col.m_stats.visibleMax == -1.0);
    REQUIRE(col.m_stats.visibleMin == -1.0);
    REQUIRE(col.m_stats.visibleTotal == -1.0);

    col.updateStats(1.2f);
    REQUIRE(col.m_stats.max == Approx(1.2));
    REQUIRE(col.m_stats.min == Approx(1.2));
    REQUIRE(col.m_stats.total == Approx(1.2));
    REQUIRE(col.m_stats.visibleMax == -1.0);
    REQUIRE(col.m_stats.visibleMin == -1.0);
    REQUIRE(col.m_stats.visibleTotal == -1.0);

    col.updateStats(2.0f);
    REQUIRE(col.m_stats.max == Approx(2.0));
    REQUIRE(col.m_stats.min == Approx(1.2));
    REQUIRE(col.m_stats.total == Approx(3.2));
    REQUIRE(col.m_stats.visibleMax == -1.0);
    REQUIRE(col.m_stats.visibleMin == -1.0);
    REQUIRE(col.m_stats.visibleTotal == -1.0);

    col.updateStats(3.0f,1.2f);
    REQUIRE(col.m_stats.max == Approx(3.0));
    REQUIRE(col.m_stats.min == Approx(1.2));
    REQUIRE(col.m_stats.total == Approx(5));
    REQUIRE(col.m_stats.visibleMax == -1.0);
    REQUIRE(col.m_stats.visibleMin == -1.0);
    REQUIRE(col.m_stats.visibleTotal == -1.0);

    // test read/write
    SelfCleaningFile scf("column.bin");
    {
        std::ofstream outfile(scf.Filename());
        col.write(outfile, 0);
    }
    AttributeColumnImpl copy("");
    {
        std::ifstream infile(scf.Filename());
        copy.read(infile, METAGRAPH_VERSION);
    }
    REQUIRE(copy.getName() == "colName");
    REQUIRE(copy.getFormula() == "");
    REQUIRE(copy.isHidden());
    REQUIRE(copy.isLocked());
    REQUIRE(copy.m_stats.max == Approx(3.0));
    REQUIRE(copy.m_stats.min == Approx(1.2));
    REQUIRE(copy.m_stats.total == Approx(5));
    REQUIRE(copy.m_stats.visibleMax == -1.0);
    REQUIRE(copy.m_stats.visibleMin == -1.0);
    REQUIRE(copy.m_stats.visibleTotal == -1.0);


}

TEST_CASE("test attribute row")
{
    using namespace dXreimpl;
    using namespace fakeit;
    Mock<AttributeColumnManager> colMan;
    When(Method(colMan,getColumnIndex).Using(std::string("col1"))).AlwaysReturn(0);
    When(Method(colMan,getColumnIndex).Using(std::string("col2"))).AlwaysReturn(1);
    When(Method(colMan,getColumnIndex).Using(std::string("colx"))).AlwaysThrow(std::out_of_range("mock out of range"));

    When(Method(colMan,getNumColumns)).Return(2);

    Mock<AttributeColumn> col1;
    Mock<AttributeColumn> col2;
    When(Method(colMan, getColumn).Using(0)).AlwaysReturn(col1.get());
    When(Method(colMan, getColumn).Using(1)).AlwaysReturn(col2.get());
    When(Method(col1,updateStats)).AlwaysReturn();
    When(Method(col2,updateStats)).AlwaysReturn();

    AttributeRowImpl row(colMan.get());
    row.setValue("col1", 1.2f);
    REQUIRE(row.getValue("col1") == Approx(1.2f));
    REQUIRE(row.getValue(0) == Approx(1.2f));

    row.setValue(1, 2.2f);
    REQUIRE(row.getValue("col2") == Approx(2.2f));
    REQUIRE(row.getValue(1) == Approx(2.2f));

    row.setValue(1, 3.2f);
    REQUIRE(row.getValue("col2") == Approx(3.2f));
    REQUIRE(row.getValue(1) == Approx(3.2f));


    Verify(Method(col1,updateStats).Using(1.2f,0.0f)).Once();
    Verify(Method(col2,updateStats).Using(2.2f,0.0f)).Once();
    Verify(Method(col2,updateStats).Using(3.2f,2.2f)).Once();

    REQUIRE_THROWS_AS(row.setValue("colx", 1.1f), std::out_of_range);
    REQUIRE_THROWS_AS(row.setValue(2, 1.2f), std::out_of_range);
    REQUIRE_THROWS_AS(row.getValue("colx"), std::out_of_range);
    REQUIRE_THROWS_AS(row.getValue(2), std::out_of_range);

    // test attribute row impl only methods
    // note that these do not affect the column manager - that will have to
    // be handled by the caller - that's why these are impl only!
    row.addColumn();
    REQUIRE(row.getValue(2) == -1.0f);

    row.removeColumn(1);
    REQUIRE(row.getValue(1) == -1.0f);
    REQUIRE(row.getValue(0) == Approx(1.2f));
    REQUIRE_THROWS_AS(row.getValue(2), std::out_of_range);

    //test reading/writing
    SelfCleaningFile scf("rowfile.bin");
    {
        std::ofstream outfile(scf.Filename());
        row.write(outfile);
    }
    Mock<AttributeColumnManager> copiedColMan;
    When(Method(copiedColMan,getNumColumns)).Return(2);
    AttributeRowImpl copiedRow(copiedColMan.get());
    {
        std::ifstream infile(scf.Filename());
        copiedRow.read(infile, METAGRAPH_VERSION);
    }

    REQUIRE(copiedRow.getValue(0) == Approx(1.2f));

    row.incrValue(0, 1.0f);
    REQUIRE(row.getValue(0) == Approx(2.2f));
    Verify(Method(col1,updateStats).Using(2.2f,1.2f)).Once();

    AttributeRow& ifRef = row;
    ifRef.incrValue(0);
    REQUIRE(row.getValue(0) == Approx(3.2f));
    Verify(Method(col1,updateStats).Using(3.2f,2.2f)).Once();


}

TEST_CASE("test attribute table")
{
    using namespace dXreimpl;

    AttributeTable<SerialisedPixelRef> table;

    table.insertOrResetColumn("col1");
    table.getOrInsertColumn("col2");
    table.insertOrResetLockedColumn("lcol1");
    table.getOrInsertLockedColumn("lcol2", "formula");

    REQUIRE(table.getNumColumns() == 4);
    REQUIRE(table.getColumnIndex("col2") == 1);
    REQUIRE(table.getColumnName(1) == "col2");
    REQUIRE(table.getColumn(1).getName() == "col2");
    REQUIRE(table.getColumn(1).isLocked() == false);
    REQUIRE(table.getColumn(3).getName() == "lcol2");
    REQUIRE(table.getColumn(3).isLocked());

    table.addRow(SerialisedPixelRef(0));
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("col1") == -1 );
    table.getRow(SerialisedPixelRef(0)).setValue("col1", 1.2f );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("col1") == Approx(1.2f) );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue(0) == Approx(1.2f) );

    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("lcol2") == -1 );
    table.getRow(SerialisedPixelRef(0)).setValue(3, 1.4f );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("lcol2") == Approx(1.4f) );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue(3) == Approx(1.4f) );

    REQUIRE_THROWS_AS(table.getRow(SerialisedPixelRef(0)).getValue(4), std::out_of_range);

    table.removeColumn(0);
    table.removeColumn(1);

    REQUIRE(table.getNumColumns() == 2);
    REQUIRE(table.getColumn(0).getName() == "col2");
    REQUIRE(table.getColumn(1).getName() == "lcol2");
    REQUIRE(table.getColumnIndex("lcol2") == 1);

    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("col2") == -1.0 );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue(0) == -1.0 );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue("lcol2") == Approx(1.4f) );
    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue(1) == Approx(1.4f) );


    REQUIRE_THROWS_AS(table.getRow(SerialisedPixelRef(0)).getValue(2), std::out_of_range);

    table.addRow(SerialisedPixelRef(1));
    REQUIRE_THROWS_AS(table.getRow(SerialisedPixelRef(1)).getValue(2), std::out_of_range);
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue("col2") == -1.0 );
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue(0) == -1.0 );
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue("lcol2") == -1.0 );
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue(1) == -1.0 );

    table.getRow(SerialisedPixelRef(1)).setValue(0, 2.4f);
    table.getRow(SerialisedPixelRef(1)).setValue("lcol2", 2.6f);
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue("col2") == Approx(2.4) );
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue(1) == Approx(2.6) );

    size_t idx = table.getOrInsertColumn("col2");
    REQUIRE(idx == 0);
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue("col2") == Approx(2.4) );
    REQUIRE(table.getColumn(0).getStats().max == Approx(2.4));

    idx = table.insertOrResetColumn("col2");
    REQUIRE(idx == 0);
    REQUIRE(table.getRow(SerialisedPixelRef(1)).getValue("col2") == -1.0 );
    REQUIRE(table.getColumn(0).getStats().max == -1.0);

    size_t newColIndex = table.getOrInsertColumn("newCol");
    REQUIRE(newColIndex == 2);
    REQUIRE(table.getColumnName(2) == "newCol");
    REQUIRE(table.getColumnIndex("newCol") == 2);
    REQUIRE(table.getColumn(2).getName() == "newCol");

    REQUIRE(table.getRow(SerialisedPixelRef(0)).getValue(2) == -1.0);

    table.renameColumn("col2", "col_foo");
    REQUIRE(table.getColumnName(0) == "col_foo");
    REQUIRE(table.getColumnIndex("col_foo") == 0);
    REQUIRE(table.getColumn(0).getName() == "col_foo");

    REQUIRE_THROWS_AS(table.getColumnIndex("col2"), std::out_of_range);

    table.getRow(0).setSelection(true);

    REQUIRE(table.getRow(0).isSelected());
    auto iter = table.begin();
    REQUIRE(iter->getRow().isSelected());
    ++iter;
    REQUIRE_FALSE(iter->getRow().isSelected());

    table.deselectAllRows();
    for (auto& item : table)
    {
        REQUIRE_FALSE(item.getRow().isSelected());
    }

    // check read/write
    LayerManagerImpl layerManager;
    SelfCleaningFile scf("tablefile.bin");
    {
        std::ofstream outfile(scf.Filename());
        table.write(outfile, layerManager);
    }
}

TEST_CASE("Existing and non-existing rows")
{
    using namespace dXreimpl;
    AttributeTable<SerialisedPixelRef> table;
    table.getOrInsertColumn("col1");
    table.getOrInsertColumn("col2");
    table.addRow(0).setValue(0, 1.0f);
    table.addRow(1).setValue(0, 0.5f);
    table.addRow(2).setValue(0, 2.0f);

    const AttributeTable<SerialisedPixelRef>& constRef = table;

    table.getRow(0);
    constRef.getRow(0);
    REQUIRE_THROWS_AS(table.getRow(5), std::out_of_range);
    REQUIRE_THROWS_AS(constRef.getRow(5), std::out_of_range);

    REQUIRE( table.getRowPtr(1) != 0);
    REQUIRE( constRef.getRowPtr(1) != 0);

    REQUIRE( table.getRowPtr(5) == 0);
    REQUIRE( constRef.getRowPtr(5) == 0);
}

TEST_CASE("normalised values"){
    using namespace dXreimpl;
    AttributeTable<SerialisedPixelRef> table;
    table.getOrInsertColumn("col1");
    table.getOrInsertColumn("col2");
    table.addRow(0).setValue(0, 1.0f);
    table.addRow(1).setValue(0, 0.5f);
    table.addRow(2).setValue(0, 2.0f);

    REQUIRE(table.getRow(0).getNormalisedValue(1) == Approx(0.5f));
    REQUIRE(table.getRow(0).getNormalisedValue(0) == Approx(0.33333f));
    REQUIRE(table.getRow(1).getNormalisedValue(0) == Approx(0.0f));
    REQUIRE(table.getRow(2).getNormalisedValue(0) == Approx(1.0f));

    table.addRow(3).setValue(1,1.0f);

    REQUIRE(table.getRow(1).getNormalisedValue(1) == Approx(0.5f));
    REQUIRE(table.getRow(3).getNormalisedValue(1) == Approx(0.5f));

    table.getRow(0).setValue(1,1.1f);
    REQUIRE(table.getRow(3).getNormalisedValue(1) == Approx(0.0f));
    REQUIRE(table.getRow(1).getNormalisedValue(1) == Approx(-1.0f));


}

TEST_CASE("attibute table iterations")
{
    using namespace dXreimpl;
    AttributeTable<SerialisedPixelRef> table;

    table.insertOrResetColumn("col1");
    table.getOrInsertColumn("col2");

    auto& row = table.addRow(SerialisedPixelRef(0));
    row.setValue(0, 0.5f);
    auto& row2 = table.addRow(SerialisedPixelRef(1));
    row2.setValue(0, 1.0f);

    AttributeTable<SerialisedPixelRef>::iterator iter = table.begin();
    REQUIRE((*iter).getKey().value == 0);
    REQUIRE(iter->getRow().getValue(0) == Approx(0.5));
    iter++;
    REQUIRE((*iter).getKey().value == 1);
    REQUIRE(iter->getRow().getValue(0) == Approx(1.0));
    iter++;

    REQUIRE(iter == table.end());

    for( auto& item :  table)
    {
        item.getRow().setValue(1, 2.0f);
    }

    REQUIRE(table.getRow(0).getValue(1) == Approx(2.0));
    REQUIRE(table.getRow(1).getValue(1) == Approx(2.0));

    const AttributeTable<SerialisedPixelRef>& const_table = table;

    auto citer = const_table.begin();
    REQUIRE((*citer).getKey().value == 0);
    REQUIRE(citer->getRow().getValue(0) == Approx(0.5));
    citer++;
    REQUIRE((*citer).getKey().value == 1);
    REQUIRE(citer->getRow().getValue(0) == Approx(1.0));
    citer++;

    auto cend = const_table.end();
    REQUIRE(citer == cend);
    REQUIRE(citer == table.end());

    AttributeTable<SerialisedPixelRef>::iterator foo(iter);
    AttributeTable<SerialisedPixelRef>::const_iterator cfoo(iter);
    AttributeTable<SerialisedPixelRef>::const_iterator ccfoo(citer);

    REQUIRE(iter == foo);
    REQUIRE(cfoo == iter);
    REQUIRE(ccfoo == iter);

    cfoo = table.end();
    foo = table.begin();

    cfoo = foo;

    foo->getRow().setValue(1,2.2f);
    ++foo;
    foo->getRow().setValue(1, 3.2f);

    REQUIRE(table.getRow(0).getValue(1) == Approx(2.2));
    REQUIRE(table.getRow(1).getValue(1) == Approx(3.2));
}

#include "salalib/attributes.h"
#include <salalib/attributetablehelpers.h>

TEST_CASE("Attribute Table - serialisation")
{
    LayerManagerImpl layerManager;
    layerManager.addLayer("extra layer");
    REQUIRE(layerManager.getLayerIndex("extra layer") == 1);

    dXreimpl::AttributeTable<dXreimpl::SerialisedPixelRef> newTable;
    size_t colIndex1 = newTable.getOrInsertColumn("foo", "foo formula");
    size_t colIndex2 = newTable.getOrInsertColumn("bar");

    DisplayParams overAllDp;
    overAllDp.blue = 1.2f;
    overAllDp.red  = 1.3f;

    DisplayParams fooDp;
    fooDp.blue = 2.2f;
    fooDp.red = 2.3f;

    DisplayParams barDp;

    newTable.getColumn(colIndex1).setDisplayParams(fooDp);
    newTable.getColumn(colIndex2).setDisplayParams(barDp);
    newTable.setDisplayParams(overAllDp);

    auto& row = newTable.addRow(0);
    auto& row2 = newTable.addRow(10);

    row.setValue(0,1.0f);
    row.setValue(1,2.0f);

    row2.setValue(0, 11.0f);
    row2.setValue(1, 12.0f);
    row2.setSelection(true);

    dXreimpl::pushSelectionToLayer(newTable, layerManager, "sel layer");
    REQUIRE(isObjectVisible(layerManager, row2));
    REQUIRE_FALSE(isObjectVisible(layerManager, row));



    SelfCleaningFile newTableFile("newtable.bin");
    SelfCleaningFile legacyTableFile("legacytable.bin");

    {
        std::ofstream outfile(newTableFile.Filename());
        newTable.write(outfile, layerManager);
    }

    dXreimpl::AttributeTable<dXreimpl::SerialisedPixelRef> copyTable;
    LayerManagerImpl copyLayerManager;
    {
        std::ifstream infile(newTableFile.Filename());
        copyTable.read(infile, copyLayerManager, METAGRAPH_VERSION);
    }

    auto& copyRow = copyTable.getRow(0);
    REQUIRE(copyRow.getValue(0) == Approx(1.0f));
    REQUIRE(copyRow.getValue(1) == Approx(2.0f));

    auto& copyRow2 = copyTable.getRow(10);
    REQUIRE(copyRow2.getValue(0) == Approx(11.0f));
    REQUIRE(copyRow2.getValue(1) == Approx(12.0f));

    REQUIRE(isObjectVisible(copyLayerManager, copyRow2));
    REQUIRE_FALSE(isObjectVisible(copyLayerManager, copyRow));

    REQUIRE(copyTable.getColumnIndex("foo") == colIndex1);
    REQUIRE(copyTable.getColumnIndex("bar") == colIndex2);

    REQUIRE(copyTable.getColumn(colIndex1).getDisplayParams().blue == Approx(fooDp.blue));
    REQUIRE(copyTable.getDisplayParams().blue == Approx(overAllDp.blue));


    AttributeTable oldTable;
    {
        std::ifstream infile(newTableFile.Filename());
        oldTable.read(infile, METAGRAPH_VERSION);
    }

    int row1Ind = oldTable.getRowid(0);
    REQUIRE(row1Ind == 0);
    int row2Ind = oldTable.getRowid(10);
    REQUIRE(row2Ind == 1);

    REQUIRE(oldTable.getValue(0, "foo") == Approx(1.0));
    REQUIRE(oldTable.getValue(1, "foo") == Approx(11.0));
    REQUIRE(oldTable.getValue(0, "bar") == Approx(2.0));
    REQUIRE(oldTable.getValue(1, "bar") == Approx(12.0));

    REQUIRE(oldTable.getColumnIndex("foo") == 1);

    REQUIRE(oldTable.isVisible(1));
    REQUIRE_FALSE(oldTable.isVisible(0));

    auto fooOldColIndex = oldTable.getColumnIndex("foo");
    REQUIRE(fooOldColIndex == 1);
    REQUIRE(oldTable.getColumnFormula(fooOldColIndex) == "foo formula");

    auto barOldColIndex = oldTable.getColumnIndex("bar");
    REQUIRE(barOldColIndex == 0);


    // the old attribute table is a bit rubbish, and what display params you see is stateful
    // the index you pass in is ignored.
    REQUIRE(oldTable.getDisplayParams(527).blue == Approx(overAllDp.blue));

    oldTable.setDisplayColumn(fooOldColIndex);
    REQUIRE(oldTable.getDisplayParams(527).blue == Approx(fooDp.blue));


    {
        std::ofstream outfile(legacyTableFile.Filename());
        oldTable.write(outfile, METAGRAPH_VERSION);
    }


    dXreimpl::AttributeTable<dXreimpl::SerialisedPixelRef> roundTripTable;
    LayerManagerImpl roundTripManager;
    {
        std::ifstream infile(legacyTableFile.Filename());
        roundTripTable.read(infile, roundTripManager, METAGRAPH_VERSION);
    }

    auto& roundtripRow = roundTripTable.getRow(0);
    REQUIRE(roundtripRow.getValue(0) == Approx(1.0f));
    REQUIRE(roundtripRow.getValue(1) == Approx(2.0f));

    auto& roundtripRow2 = roundTripTable.getRow(10);
    REQUIRE(roundtripRow2.getValue(0) == Approx(11.0f));
    REQUIRE(roundtripRow2.getValue(1) == Approx(12.0f));

    REQUIRE(isObjectVisible(roundTripManager, roundtripRow2));
    REQUIRE_FALSE(isObjectVisible(roundTripManager, roundtripRow));

    REQUIRE(roundTripTable.getColumn(colIndex1).getDisplayParams().blue == Approx(fooDp.blue));
    // the overall display params have gone AWOL in the old implementation :-/
    REQUIRE(roundTripTable.getDisplayParams().blue == Approx(fooDp.blue));


}

