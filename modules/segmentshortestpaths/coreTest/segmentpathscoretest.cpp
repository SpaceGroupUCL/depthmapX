// Copyright (C) 2020 Petros Koutsolampros
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
#include "modules/segmentshortestpaths/core/segmmetricshortestpath.h"
#include "modules/segmentshortestpaths/core/segmtopologicalshortestpath.h"
#include "modules/segmentshortestpaths/core/segmtulipshortestpath.h"
#include "salalib/axialmap.h"
#include "salalib/mapconverter.h"

TEST_CASE("Shortest paths working examples", "") {
    const float EPSILON = 0.001;

    // construct an axial map which will result in three different paths for the three types
    ShapeGraph axialMap("Dummy drawing map", ShapeMap::AXIALMAP);
    axialMap.initialiseAttributesAxial();
    std::vector<Line> lines;
    lines.push_back(Line(Point2f(1.05000000, 1.00000000), Point2f(3.60000000, 1.00000000)));
    lines.push_back(Line(Point2f(3.43455142, 2.92439257), Point2f(4.15448579, 3.75607430)));
    lines.push_back(Line(Point2f(2.40000000, 3.00000000), Point2f(3.60000000, 3.00000000)));
    lines.push_back(Line(Point2f(1.15022677, 0.90136061), Point2f(1.34977323, 2.09863939)));
    lines.push_back(Line(Point2f(3.50000000, 3.10000000), Point2f(3.50000000, 0.90000000)));
    lines.push_back(Line(Point2f(1.24560093, 1.95201016), Point2f(2.11199711, 2.42593102)));
    lines.push_back(Line(Point2f(1.96351621, 2.29850806), Point2f(2.56074850, 3.07943312)));

    lines.push_back(Line(Point2f(1.28848772, 1.91061952), Point2f(1.75546653, 2.84134127)));
    lines.push_back(Line(Point2f(1.61521977, 2.72198377), Point2f(2.59540115, 3.02997701)));
    lines.push_back(Line(Point2f(1.23737734, 1.07071068), Point2f(0.45955989, 0.29289322)));
    for (Line line : lines) {
        axialMap.makeLineShape(line);
    }
    axialMap.makeConnections();

    REQUIRE(axialMap.getShapeCount() == 10);

    std::unique_ptr<ShapeGraph> segmentMap =
        MapConverter::convertAxialToSegment(nullptr, axialMap, "Dummy segment map", true, true, 0.4);

    REQUIRE(segmentMap->getShapeCount() == 10);

    // select the two edges
    QtRegion selRegion(lines[1].midpoint(), lines[1].midpoint());
    segmentMap->setCurSel(selRegion, false);
    selRegion.bottom_left = lines[9].midpoint();
    selRegion.top_right = lines[9].midpoint();
    segmentMap->setCurSel(selRegion, true);
    REQUIRE(segmentMap->getSelCount() == 2);

    SECTION("Cancellation after entering the search preserves existing results") {
        struct CancelOnSearch : ICommunicator {
            mutable std::vector<int> steps;
            void CommPostMessage(int message, int value) const override {
                if (message == CURRENT_STEP) {
                    steps.push_back(value);
                    if (value == 1) {
                        const_cast<CancelOnSearch *>(this)->Cancel();
                    }
                }
            }
        };
        auto &table = segmentMap->getAttributeTable();
        auto &row = segmentMap->getAttributeRowFromShapeIndex(0);
        auto check = [&](IAnalysis &analysis, const char *name) {
            const auto column = table.getOrInsertColumn(name);
            row.setValue(column, 42.0f);
            const auto columns = table.getNumColumns();
            CancelOnSearch comm;
            REQUIRE_THROWS_AS(analysis.run(&comm), Communicator::CancelledException);
            REQUIRE(comm.steps == std::vector<int>{1});
            REQUIRE(table.getNumColumns() == columns);
            REQUIRE(row.getValue(column) == 42.0f);
        };
        SegmentTulipShortestPath angular(*segmentMap);
        SegmentMetricShortestPath metric(*segmentMap);
        SegmentTopologicalShortestPath topological(*segmentMap);
        check(angular, "Angular Shortest Path Angle");
        check(metric, "Metric Shortest Path Distance");
        check(topological, "Topological Shortest Path Depth");
    }

    SECTION("Completed analyses publish the expected paths") {
        {
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Angular Shortest Path Angle"));
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Angular Shortest Path Order"));
            SegmentTulipShortestPath(*segmentMap.get()).run(nullptr);
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Angular Shortest Path Angle"));
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Angular Shortest Path Order"));
            int angleColIdx = segmentMap->getAttributeTable().getColumnIndex("Angular Shortest Path Angle");
            int orderColIdx = segmentMap->getAttributeTable().getColumnIndex("Angular Shortest Path Order");
            std::vector<double> expectedAngles = {-1, 0, 0.54297, 1.42969, -1, -1, -1, 1.24219, 0.734375, 1.82422};
            std::vector<int> expectedOrder = {-1, 0, 1, 4, -1, -1, -1, 3, 2, 5};
            for (int i = 0; i < lines.size(); i++) {
                QtRegion selRegion(lines[i].midpoint(), lines[i].midpoint());
                AttributeRow &shapeRow =
                    segmentMap->getAttributeRowFromShapeIndex(segmentMap->getShapesInRegion(selRegion).begin()->first);

                REQUIRE(shapeRow.getValue(angleColIdx) == Approx(expectedAngles[i]).epsilon(EPSILON));
                REQUIRE(shapeRow.getValue(orderColIdx) == expectedOrder[i]);
            }
        }

        {
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Metric Shortest Path Distance"));
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Metric Shortest Path Order"));
            SegmentMetricShortestPath(*segmentMap.get()).run(nullptr);
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Metric Shortest Path Distance"));
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Metric Shortest Path Order"));
            int distanceColIdx = segmentMap->getAttributeTable().getColumnIndex("Metric Shortest Path Distance");
            int orderColIdx = segmentMap->getAttributeTable().getColumnIndex("Metric Shortest Path Order");
            std::vector<double> expectedDistances = {-1, 0, 1, 3.57756, -1, 2.67689, 1.89156, -1, -1, 4.58446};
            std::vector<int> expectedOrder = {-1, 0, 1, 4, -1, 3, 2, -1, -1, 5};
            for (int i = 0; i < lines.size(); i++) {
                QtRegion selRegion(lines[i].midpoint(), lines[i].midpoint());
                AttributeRow &shapeRow =
                    segmentMap->getAttributeRowFromShapeIndex(segmentMap->getShapesInRegion(selRegion).begin()->first);
                REQUIRE(shapeRow.getValue(distanceColIdx) == Approx(expectedDistances[i]).epsilon(EPSILON));
                REQUIRE(shapeRow.getValue(orderColIdx) == expectedOrder[i]);
            }
        }

        {
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Topological Shortest Path Depth"));
            REQUIRE_FALSE(segmentMap->getAttributeTable().hasColumn("Topological Shortest Path Order"));
            SegmentTopologicalShortestPath(*segmentMap.get()).run(nullptr);
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Topological Shortest Path Depth"));
            REQUIRE(segmentMap->getAttributeTable().hasColumn("Topological Shortest Path Order"));
            int depthColIdx = segmentMap->getAttributeTable().getColumnIndex("Topological Shortest Path Depth");
            int orderColIdx = segmentMap->getAttributeTable().getColumnIndex("Topological Shortest Path Order");
            std::vector<double> expectedDepths = {2, 0, -1, -1, 1, -1, -1, -1, -1, 3};
            std::vector<int> expectedOrder = {2, 0, -1, -1, 1, -1, -1, -1, -1, 3};
            for (int i = 0; i < lines.size(); i++) {
                QtRegion selRegion(lines[i].midpoint(), lines[i].midpoint());
                AttributeRow &shapeRow =
                    segmentMap->getAttributeRowFromShapeIndex(segmentMap->getShapesInRegion(selRegion).begin()->first);
                REQUIRE(shapeRow.getValue(depthColIdx) == Approx(expectedDepths[i]).epsilon(EPSILON));
                REQUIRE(shapeRow.getValue(orderColIdx) == expectedOrder[i]);
            }
        }
    }
}

TEST_CASE("Cancelling shortest paths keeps existing results", "[shortest-path-cancel]") {
    ShapeGraph map("Segment map", ShapeMap::SEGMENTMAP);
    auto &attributes = map.getAttributeTable();
    const char *columnNames[] = {"Angular Shortest Path Angle", "Metric Shortest Path Distance",
                                 "Topological Shortest Path Depth"};
    const AttributeKey key(0);
    auto &row = attributes.addRow(key);
    for (const auto *name : columnNames) {
        row.setValue(attributes.getOrInsertColumn(name), 42.0f);
    }

    ICommunicator comm;
    comm.Cancel();
    REQUIRE_THROWS_AS(SegmentTulipShortestPath(map).run(&comm), Communicator::CancelledException);
    REQUIRE_THROWS_AS(SegmentMetricShortestPath(map).run(&comm), Communicator::CancelledException);
    REQUIRE_THROWS_AS(SegmentTopologicalShortestPath(map).run(&comm), Communicator::CancelledException);
    REQUIRE(attributes.getNumColumns() == 3);
    for (const auto *name : columnNames) {
        REQUIRE(row.getValue(name) == 42.0f);
    }
}
