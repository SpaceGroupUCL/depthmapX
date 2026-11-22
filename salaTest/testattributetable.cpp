// Copyright (C) 2017 Christian Sailer
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

#include "catch.hpp"
#include "Catch/fakeit.hpp"
#include <salalib/attributetable.h>
#include <salalib/attributetablehelpers.h>
#include <cliTest/selfcleaningfile.h>
#include <fstream>
#include <salalib/mgraph_consts.h>
#include <salalib/layermanagerimpl.h>
#include <salalib/mgraph.h>
#include <mgraph440/mgraph.h>

TEST_CASE("A failed built-in analysis preserves maps on save", "[analysis-recovery]") {
    MetaGraph graph;
    REQUIRE(graph.readFromFile(std::string(DEPTHMAPX_TESTDATA) + "/barnsbury_axial.graph") == MetaGraph::OK);
    REQUIRE(graph.getShapeGraphs().size() == 1);
    const auto shapeCount = graph.getShapeGraphs().front()->getShapeCount();
    Options options;
    options.weighted_measure_col = static_cast<int>(graph.getShapeGraphs().front()->getAttributeTable().getNumColumns());
    REQUIRE_THROWS_AS(graph.analyseAxial(nullptr, options, false), std::out_of_range);

    SelfCleaningFile file("failed-analysis.graph");
    REQUIRE(graph.write(file.Filename(), METAGRAPH_VERSION) == MetaGraph::OK);
    MetaGraph reopened;
    REQUIRE(reopened.readFromFile(file.Filename()) == MetaGraph::OK);
    REQUIRE(reopened.getShapeGraphs().size() == 1);
    REQUIRE(reopened.getShapeGraphs().front()->getShapeCount() == shapeCount);
}

TEST_CASE("Map lookup distinguishes negative two from empty space") {
    MetaGraph graph;
    auto &map = graph.getDataMaps().at(graph.addShapeMap("Signed lookup"));
    graph.setDisplayedDataMapRef(0);
    map.init(1, QtRegion(Point2f(0, 0), Point2f(10, 10)));
    const auto key = AttributeKey(map.makePolyShape(
        {Point2f(1, 1), Point2f(3, 1), Point2f(3, 3), Point2f(1, 3)}, false));
    auto &table = map.getAttributeTable();
    const auto col = table.getOrInsertColumn("Signed");
    map.setDisplayedAttribute(col);
    REQUIRE(graph.getLocationValue(Point2f(2, 2)) == -1.0);
    table.getRow(key).setValue(col, -2.0f);
    REQUIRE(graph.getLocationValue(Point2f(2, 2)) == -2.0);
    REQUIRE_FALSE(graph.getLocationValue(Point2f(9, 9)).has_value());
}

TEST_CASE("Signed attribute values and missing cells remain distinct", "[signed-attributes]") {
    AttributeTable table;
    const auto column = table.getOrInsertColumn("Signed");
    auto &negative = table.addRow(AttributeKey(1)).setValue(column, -3.0f);
    auto &nearZero = table.addRow(AttributeKey(2)).setValue(column, -0.5f);
    table.addRow(AttributeKey(3)).setValue(column, 2.0f);
    table.addRow(AttributeKey(4)).setValue(column, 5.0f);
    auto &missing = table.addRow(AttributeKey(5));
    const auto &attribute = table.getColumn(column);

    REQUIRE(attribute.getStats().count == 4);
    REQUIRE(attribute.getStats().min == -3.0);
    REQUIRE(attribute.getStats().max == 5.0);
    REQUIRE(attribute.getStats().total == 3.5);
    REQUIRE(nearZero.hasValue(column));
    REQUIRE_FALSE(missing.hasValue(column));
    REQUIRE(nearZero.getValue(column) == -0.5f);
    REQUIRE(missing.getValue(column) == -1.0f);
    REQUIRE(nearZero.getNormalisedValue(column) == Approx(0.3125));
    REQUIRE(missing.getNormalisedValue(column) == -1.0f);
    AttributeTableView view(table);
    view.setDisplayColIndex(column);
    REQUIRE(dXreimpl::getDisplayColor(AttributeKey(1), negative, view) !=
            dXreimpl::getDisplayColor(AttributeKey(5), missing, view));

    negative.incrValue(column, 1.0f);
    REQUIRE(negative.getValue(column) == -2.0f);
    REQUIRE(attribute.getStats().min == -2.0);
    REQUIRE(attribute.getStats().total == 4.5);
    table.removeRow(AttributeKey(4));
    REQUIRE(attribute.getStats().max == 2.0);
    REQUIRE(attribute.getStats().total == -0.5);
    table.removeRow(AttributeKey(3));
    REQUIRE(attribute.getStats().max == -0.5);
    REQUIRE(attribute.getStats().total == -2.5);
    nearZero.setValue(column, -1.0f);
    REQUIRE_FALSE(nearZero.hasValue(column));
    REQUIRE(attribute.getStats().count == 1);
    REQUIRE(attribute.getStats().total == -2.0);
    REQUIRE(negative.getNormalisedValue(column) == 0.5f);
    REQUIRE(missing.getNormalisedValue(column) == -1.0f);

    AttributeTable moved(std::move(table));
    moved.getRow(AttributeKey(1)).setValue(column, -7.0f);
    REQUIRE(moved.getColumn(column).getStats().min == -7.0);
    moved.insertOrResetColumn("Signed");
    REQUIRE(moved.getColumn(column).getStats().count == 0);
}

TEST_CASE("Graph files round-trip signed attribute cells", "[signed-attributes]") {
    MetaGraph graph;
    const auto mapIndex = graph.addShapeMap("Signed values");
    auto &map = graph.getDataMaps().at(mapIndex);
    graph.setDisplayedDataMapRef(mapIndex);
    map.init(2, QtRegion(Point2f(0, 0), Point2f(10, 10)));
    const AttributeKey present(map.makePointShape(Point2f(1, 1)));
    const AttributeKey missing(map.makePointShape(Point2f(2, 2)));
    auto &table = map.getAttributeTable();
    const auto column = table.getOrInsertColumn("Signed");
    table.getRow(present).setValue(column, -3.0f);

    SelfCleaningFile file("signed-attributes.graph");
    REQUIRE(graph.write(file.Filename(), METAGRAPH_VERSION) == MetaGraph::OK);
    MetaGraph loaded;
    REQUIRE(loaded.readFromFile(file.Filename()) == MetaGraph::OK);
    const auto &loadedTable = loaded.getDataMaps().at(0).getAttributeTable();
    REQUIRE(loaded.getVersion() == VERSION_ALWAYS_RECORD_BINDISTANCES);
    REQUIRE(loadedTable.getRow(present).hasValue(column));
    REQUIRE(loadedTable.getRow(present).getValue(column) == -3.0f);
    REQUIRE_FALSE(loadedTable.getRow(missing).hasValue(column));
    REQUIRE(loadedTable.getColumn(column).getStats().count == 1);
    REQUIRE(loadedTable.getColumn(column).getStats().total == -3.0);

    mgraph440::MetaGraph legacy;
    REQUIRE(legacy.read(file.Filename()) == mgraph440::MetaGraph::OK);
}

TEST_CASE("Version 440 graph fixtures still open and round-trip", "[signed-attributes]") {
    const char *files[] = {
        "all_line_noncont_keys.graph", "axmap_noncont_keys.graph", "barnsbury_axial.graph",
        "barnsbury_drawing.graph", "barnsbury_extended1_axial.graph", "barnsbury_extended1_segment.graph",
        "barnsbury_extended2_axial.graph", "barnsbury_extended2_drawing.graph", "barnsbury_segment.graph",
        "gallery_connected.graph", "gallery_connected_with_isovist.graph", "gallery_empty.graph",
        "gallery_two_pointmaps.graph", "polygons_drawing.graph", "polywall.graph", "rect1x1.graph",
        "simple_axlines.graph", "turns_connected.graph"
    };
    auto checkTable = [](const AttributeTable &original, const AttributeTable &copy) {
        REQUIRE(copy.getNumRows() == original.getNumRows());
        REQUIRE(copy.getNumColumns() == original.getNumColumns());
        for (size_t col = 0; col < original.getNumColumns(); ++col) {
            REQUIRE(copy.getColumnName(col) == original.getColumnName(col));
        }
        size_t mismatches = 0;
        auto copiedRow = copy.begin();
        for (const auto &originalRow : original) {
            if (originalRow.getKey().value != copiedRow->getKey().value) {
                ++mismatches;
            }
            for (size_t col = 0; col < original.getNumColumns(); ++col) {
                const auto &left = originalRow.getRow();
                const auto &right = copiedRow->getRow();
                if (left.hasValue(col) != right.hasValue(col) ||
                    (left.hasValue(col) && (left.getValue(col) == -1.0f || left.getValue(col) != right.getValue(col)))) {
                    ++mismatches;
                }
            }
            ++copiedRow;
        }
        REQUIRE(mismatches == 0);
    };

    for (const auto *name : files) {
        CAPTURE(name);
        MetaGraph original;
        REQUIRE(original.readFromFile(std::string(DEPTHMAPX_TESTDATA) + "/" + name) == MetaGraph::OK);
        REQUIRE(original.getVersion() == VERSION_ALWAYS_RECORD_BINDISTANCES);
        SelfCleaningFile file("legacy-attribute-roundtrip.graph");
        REQUIRE(original.write(file.Filename(), METAGRAPH_VERSION) == MetaGraph::OK);
        {
            mgraph440::MetaGraph legacy;
            REQUIRE(legacy.read(file.Filename()) == mgraph440::MetaGraph::OK);
        }
        MetaGraph copy;
        REQUIRE(copy.readFromFile(file.Filename()) == MetaGraph::OK);
        REQUIRE(copy.getPointMaps().size() == original.getPointMaps().size());
        REQUIRE(copy.getShapeGraphs().size() == original.getShapeGraphs().size());
        REQUIRE(copy.getDataMaps().size() == original.getDataMaps().size());
        for (size_t i = 0; i < original.getPointMaps().size(); ++i) {
            checkTable(original.getPointMaps()[i].getAttributeTable(), copy.getPointMaps()[i].getAttributeTable());
        }
        for (size_t i = 0; i < original.getShapeGraphs().size(); ++i) {
            checkTable(original.getShapeGraphs()[i]->getAttributeTable(), copy.getShapeGraphs()[i]->getAttributeTable());
        }
        for (size_t i = 0; i < original.getDataMaps().size(); ++i) {
            checkTable(original.getDataMaps()[i].getAttributeTable(), copy.getDataMaps()[i].getAttributeTable());
        }
    }
}

TEST_CASE("test attribute column")
{
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
        copy.read(infile);
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


    Verify(Method(col1,updateStats).Using(1.2f,AttributeValue::MISSING)).Once();
    Verify(Method(col2,updateStats).Using(2.2f,AttributeValue::MISSING)).Once();
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
        copiedRow.read(infile);
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
    AttributeTable table;

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

    table.addRow(AttributeKey(0));
    REQUIRE(table.getRow(AttributeKey(0)).getValue("col1") == -1 );
    table.getRow(AttributeKey(0)).setValue("col1", 1.2f );
    REQUIRE(table.getRow(AttributeKey(0)).getValue("col1") == Approx(1.2f) );
    REQUIRE(table.getRow(AttributeKey(0)).getValue(0) == Approx(1.2f) );

    REQUIRE(table.getRow(AttributeKey(0)).getValue("lcol2") == -1 );
    table.getRow(AttributeKey(0)).setValue(3, 1.4f );
    REQUIRE(table.getRow(AttributeKey(0)).getValue("lcol2") == Approx(1.4f) );
    REQUIRE(table.getRow(AttributeKey(0)).getValue(3) == Approx(1.4f) );

    REQUIRE_THROWS_AS(table.getRow(AttributeKey(0)).getValue(4), std::out_of_range);

    table.removeColumn(0);
    table.removeColumn(1);

    REQUIRE(table.getNumColumns() == 2);
    REQUIRE(table.getColumn(0).getName() == "col2");
    REQUIRE(table.getColumn(1).getName() == "lcol2");
    REQUIRE(table.getColumnIndex("lcol2") == 1);

    REQUIRE(table.getRow(AttributeKey(0)).getValue("col2") == -1.0 );
    REQUIRE(table.getRow(AttributeKey(0)).getValue(0) == -1.0 );
    REQUIRE(table.getRow(AttributeKey(0)).getValue("lcol2") == Approx(1.4f) );
    REQUIRE(table.getRow(AttributeKey(0)).getValue(1) == Approx(1.4f) );


    REQUIRE_THROWS_AS(table.getRow(AttributeKey(0)).getValue(2), std::out_of_range);

    table.addRow(AttributeKey(1));
    REQUIRE_THROWS_AS(table.getRow(AttributeKey(1)).getValue(2), std::out_of_range);
    REQUIRE(table.getRow(AttributeKey(1)).getValue("col2") == -1.0 );
    REQUIRE(table.getRow(AttributeKey(1)).getValue(0) == -1.0 );
    REQUIRE(table.getRow(AttributeKey(1)).getValue("lcol2") == -1.0 );
    REQUIRE(table.getRow(AttributeKey(1)).getValue(1) == -1.0 );

    table.getRow(AttributeKey(1)).setValue(0, 2.4f);
    table.getRow(AttributeKey(1)).setValue("lcol2", 2.6f);
    REQUIRE(table.getRow(AttributeKey(1)).getValue("col2") == Approx(2.4) );
    REQUIRE(table.getRow(AttributeKey(1)).getValue(1) == Approx(2.6) );

    size_t idx = table.getOrInsertColumn("col2");
    REQUIRE(idx == 0);
    REQUIRE(table.getRow(AttributeKey(1)).getValue("col2") == Approx(2.4) );
    REQUIRE(table.getColumn(0).getStats().max == Approx(2.4));

    idx = table.insertOrResetColumn("col2");
    REQUIRE(idx == 0);
    REQUIRE(table.getRow(AttributeKey(1)).getValue("col2") == -1.0 );
    REQUIRE(table.getColumn(0).getStats().max == -1.0);

    size_t newColIndex = table.getOrInsertColumn("newCol");
    REQUIRE(newColIndex == 2);
    REQUIRE(table.getColumnName(2) == "newCol");
    REQUIRE(table.getColumnIndex("newCol") == 2);
    REQUIRE(table.getColumn(2).getName() == "newCol");

    REQUIRE(table.getRow(AttributeKey(0)).getValue(2) == -1.0);

    table.renameColumn("col2", "col_foo");
    REQUIRE(table.getColumnName(0) == "col_foo");
    REQUIRE(table.getColumnIndex("col_foo") == 0);
    REQUIRE(table.getColumn(0).getName() == "col_foo");

    REQUIRE_THROWS_AS(table.getColumnIndex("col2"), std::out_of_range);

    table.getRow(AttributeKey(0)).setSelection(true);

    REQUIRE(table.getRow(AttributeKey(0)).isSelected());
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
    AttributeTable table;
    table.getOrInsertColumn("col1");
    table.getOrInsertColumn("col2");
    table.addRow(AttributeKey(0)).setValue(0, 1.0f);
    table.addRow(AttributeKey(1)).setValue(0, 0.5f);
    table.addRow(AttributeKey(2)).setValue(0, 2.0f);

    const AttributeTable& constRef = table;

    table.getRow(AttributeKey(0));
    constRef.getRow(AttributeKey(0));
    REQUIRE_THROWS_AS(table.getRow(AttributeKey(5)), std::out_of_range);
    REQUIRE_THROWS_AS(constRef.getRow(AttributeKey(5)), std::out_of_range);

    REQUIRE( table.getRowPtr(AttributeKey(1)) != 0);
    REQUIRE( constRef.getRowPtr(AttributeKey(1)) != 0);

    REQUIRE( table.getRowPtr(AttributeKey(5)) == 0);
    REQUIRE( constRef.getRowPtr(AttributeKey(5)) == 0);
}

TEST_CASE("normalised values"){
    AttributeTable table;
    table.getOrInsertColumn("col1");
    table.getOrInsertColumn("col2");
    table.addRow(AttributeKey(0)).setValue(0, 1.0f);
    table.addRow(AttributeKey(1)).setValue(0, 0.5f);
    table.addRow(AttributeKey(2)).setValue(0, 2.0f);

    REQUIRE(table.getRow(AttributeKey(0)).getNormalisedValue(1) == -1.0f);
    REQUIRE(table.getRow(AttributeKey(0)).getNormalisedValue(0) == Approx(0.33333f));
    REQUIRE(table.getRow(AttributeKey(1)).getNormalisedValue(0) == Approx(0.0f));
    REQUIRE(table.getRow(AttributeKey(2)).getNormalisedValue(0) == Approx(1.0f));

    table.addRow(AttributeKey(3)).setValue(1,1.0f);

    REQUIRE(table.getRow(AttributeKey(1)).getNormalisedValue(1) == -1.0f);
    REQUIRE(table.getRow(AttributeKey(3)).getNormalisedValue(1) == Approx(0.5f));

    table.getRow(AttributeKey(0)).setValue(1,1.1f);
    REQUIRE(table.getRow(AttributeKey(3)).getNormalisedValue(1) == Approx(0.0f));
    REQUIRE(table.getRow(AttributeKey(1)).getNormalisedValue(1) == Approx(-1.0f));


}

TEST_CASE("attibute table iterations")
{
    AttributeTable table;

    table.insertOrResetColumn("col1");
    table.getOrInsertColumn("col2");

    auto& row = table.addRow(AttributeKey(0));
    row.setValue(0, 0.5f);
    auto& row2 = table.addRow(AttributeKey(1));
    row2.setValue(0, 1.0f);

    AttributeTable::iterator iter = table.begin();
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

    REQUIRE(table.getRow(AttributeKey(0)).getValue(1) == Approx(2.0));
    REQUIRE(table.getRow(AttributeKey(1)).getValue(1) == Approx(2.0));

    const AttributeTable& const_table = table;

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

    AttributeTable::iterator foo(iter);
    AttributeTable::const_iterator cfoo(iter);
    AttributeTable::const_iterator ccfoo(citer);

    REQUIRE(iter == foo);
    REQUIRE(cfoo == iter);
    REQUIRE(ccfoo == iter);

    cfoo = table.end();
    foo = table.begin();

    cfoo = foo;

    foo->getRow().setValue(1,2.2f);
    ++foo;
    foo->getRow().setValue(1, 3.2f);

    REQUIRE(table.getRow(AttributeKey(0)).getValue(1) == Approx(2.2));
    REQUIRE(table.getRow(AttributeKey(1)).getValue(1) == Approx(3.2));
}

#include <salalib/attributetablehelpers.h>

TEST_CASE("Attribute Table - serialisation")
{
    LayerManagerImpl layerManager;
    layerManager.addLayer("extra layer");
    REQUIRE(layerManager.getLayerIndex("extra layer") == 1);

    AttributeTable newTable;
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

    auto& row = newTable.addRow(AttributeKey(0));
    auto& row2 = newTable.addRow(AttributeKey(10));

    row.setValue(0,1.0f);
    row.setValue(1,2.0f);

    row2.setValue(0, 11.0f);
    row2.setValue(1, 12.0f);
    row2.setSelection(true);

    dXreimpl::pushSelectionToLayer(newTable, layerManager, "sel layer");
    REQUIRE(isObjectVisible(layerManager, row2));
    REQUIRE_FALSE(isObjectVisible(layerManager, row));



    SelfCleaningFile newTableFile("newtable.bin");

    {
        std::ofstream outfile(newTableFile.Filename());
        newTable.write(outfile, layerManager);
    }

    AttributeTable copyTable;
    LayerManagerImpl copyLayerManager;
    {
        std::ifstream infile(newTableFile.Filename());
        copyTable.read(infile, copyLayerManager);
    }

    auto& copyRow = copyTable.getRow(AttributeKey(0));
    REQUIRE(copyRow.getValue(0) == Approx(1.0f));
    REQUIRE(copyRow.getValue(1) == Approx(2.0f));

    auto& copyRow2 = copyTable.getRow(AttributeKey(10));
    REQUIRE(copyRow2.getValue(0) == Approx(11.0f));
    REQUIRE(copyRow2.getValue(1) == Approx(12.0f));

    REQUIRE(isObjectVisible(copyLayerManager, copyRow2));
    REQUIRE_FALSE(isObjectVisible(copyLayerManager, copyRow));

    REQUIRE(copyTable.getColumnIndex("foo") == colIndex1);
    REQUIRE(copyTable.getColumnIndex("bar") == colIndex2);

    REQUIRE(copyTable.getColumn(colIndex1).getDisplayParams().blue == Approx(fooDp.blue));
    REQUIRE(copyTable.getDisplayParams().blue == Approx(overAllDp.blue));

}

