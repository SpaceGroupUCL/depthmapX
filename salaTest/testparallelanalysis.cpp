// Copyright (C) 2026 depthmapX contributors

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "catch.hpp"

#include "genlib/parallel.h"
#include "salalib/alllinemap.h"
#include "salalib/axialmodules/axialintegration.h"
#include "salalib/mapconverter.h"
#include "salalib/mgraph.h"
#include "salalib/segmmodules/segmangular.h"
#include "salalib/segmmodules/segmmetric.h"
#include "salalib/segmmodules/segmtulip.h"
#include "salalib/segmmodules/segmtopological.h"

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

void setAnalysisThreads(const char *value) {
#ifdef _WIN32
    _putenv_s("DEPTHMAPX_THREADS", value ? value : "");
#else
    if (value) {
        setenv("DEPTHMAPX_THREADS", value, 1);
    } else {
        unsetenv("DEPTHMAPX_THREADS");
    }
#endif
}

class AnalysisThreadSetting {
  public:
    explicit AnalysisThreadSetting(const char *value) {
        const char *current = std::getenv("DEPTHMAPX_THREADS");
        if (current) {
            m_previous = current;
            m_hadPrevious = true;
        }
        setAnalysisThreads(value);
    }

    ~AnalysisThreadSetting() { setAnalysisThreads(m_hadPrevious ? m_previous.c_str() : nullptr); }

  private:
    std::string m_previous;
    bool m_hadPrevious = false;
};

std::unique_ptr<ShapeGraph> makeAxialGraph() {
    std::unique_ptr<MetaGraph> metaGraph(new MetaGraph("Parallel axial test"));
    metaGraph->m_drawingFiles.emplace_back("Drawing");
    metaGraph->m_drawingFiles.back().m_spacePixels.emplace_back("Lines");
    ShapeMap &drawing = metaGraph->m_drawingFiles.back().m_spacePixels.back();

    drawing.makeLineShape(Line(Point2f(0.0, 0.0), Point2f(6.0, 0.0)));
    drawing.makeLineShape(Line(Point2f(1.0, -1.0), Point2f(1.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(2.0, -1.0), Point2f(2.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(3.0, -1.0), Point2f(3.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(4.0, -1.0), Point2f(4.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(5.0, -1.0), Point2f(5.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(1.0, 1.0), Point2f(5.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(1.0, -1.0), Point2f(5.0, -1.0)));

    return MapConverter::convertDrawingToAxial(nullptr, "Axial", metaGraph->m_drawingFiles);
}


std::unique_ptr<AllLineMap> makeAllLineMap() {
    std::vector<SpacePixelFile> drawingLayers;
    drawingLayers.emplace_back("Drawing");
    drawingLayers.back().m_spacePixels.emplace_back("Boundaries");
    ShapeMap &drawing = drawingLayers.back().m_spacePixels.back();

    drawing.makeLineShape(Line(Point2f(0.0, 0.0), Point2f(12.0, 0.0)));
    drawing.makeLineShape(Line(Point2f(12.0, 0.0), Point2f(12.0, 10.0)));
    drawing.makeLineShape(Line(Point2f(12.0, 10.0), Point2f(0.0, 10.0)));
    drawing.makeLineShape(Line(Point2f(0.0, 10.0), Point2f(0.0, 0.0)));

    drawing.makeLineShape(Line(Point2f(4.0, 3.0), Point2f(8.0, 3.0)));
    drawing.makeLineShape(Line(Point2f(8.0, 3.0), Point2f(8.0, 7.0)));
    drawing.makeLineShape(Line(Point2f(8.0, 7.0), Point2f(4.0, 7.0)));
    drawing.makeLineShape(Line(Point2f(4.0, 7.0), Point2f(4.0, 3.0)));

    return std::unique_ptr<AllLineMap>(
        new AllLineMap(nullptr, drawingLayers, Point2f(1.0, 1.0), "All lines"));
}

void requireSameGraphGeometryAndConnections(const ShapeGraph &serial, const ShapeGraph &parallel) {
    REQUIRE(serial.getShapeCount() == parallel.getShapeCount());
    REQUIRE(serial.getConnections().size() == parallel.getConnections().size());

    auto serialShape = serial.getAllShapes().begin();
    auto parallelShape = parallel.getAllShapes().begin();
    for (; serialShape != serial.getAllShapes().end(); ++serialShape, ++parallelShape) {
        REQUIRE(serialShape->first == parallelShape->first);
        const Line &serialLine = serialShape->second.getLine();
        const Line &parallelLine = parallelShape->second.getLine();
        REQUIRE(parallelLine.start().x == Approx(serialLine.start().x));
        REQUIRE(parallelLine.start().y == Approx(serialLine.start().y));
        REQUIRE(parallelLine.end().x == Approx(serialLine.end().x));
        REQUIRE(parallelLine.end().y == Approx(serialLine.end().y));
    }

    for (size_t index = 0; index < serial.getConnections().size(); ++index) {
        REQUIRE(serial.getConnections()[index].m_connections ==
                parallel.getConnections()[index].m_connections);
    }
}

std::unique_ptr<ShapeGraph> makeSegmentGraph() {
    std::unique_ptr<MetaGraph> metaGraph(new MetaGraph("Parallel segment test"));
    metaGraph->m_drawingFiles.emplace_back("Drawing");
    metaGraph->m_drawingFiles.back().m_spacePixels.emplace_back("Lines");
    ShapeMap &drawing = metaGraph->m_drawingFiles.back().m_spacePixels.back();

    drawing.makeLineShape(Line(Point2f(0.0, 0.0), Point2f(1.0, 0.0)));
    drawing.makeLineShape(Line(Point2f(1.0, 0.0), Point2f(2.0, 0.0)));
    drawing.makeLineShape(Line(Point2f(2.0, 0.0), Point2f(3.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(2.0, 0.0), Point2f(3.0, -1.0)));
    drawing.makeLineShape(Line(Point2f(3.0, 1.0), Point2f(4.0, 1.0)));
    drawing.makeLineShape(Line(Point2f(3.0, -1.0), Point2f(4.0, -1.0)));
    drawing.makeLineShape(Line(Point2f(4.0, 1.0), Point2f(5.0, 0.0)));
    drawing.makeLineShape(Line(Point2f(4.0, -1.0), Point2f(5.0, 0.0)));

    return MapConverter::convertDrawingToSegment(nullptr, "Segment", metaGraph->m_drawingFiles);
}

void requireSameSegmentConnections(const ShapeGraph &serial, const ShapeGraph &parallel) {
    const std::vector<Connector> &serialConnections = serial.getConnections();
    const std::vector<Connector> &parallelConnections = parallel.getConnections();
    REQUIRE(serialConnections.size() == parallelConnections.size());

    auto compareDirectional = [](const std::map<SegmentRef, float> &expected,
                                 const std::map<SegmentRef, float> &actual) {
        REQUIRE(expected.size() == actual.size());
        auto expectedIter = expected.begin();
        auto actualIter = actual.begin();
        for (; expectedIter != expected.end(); ++expectedIter, ++actualIter) {
            REQUIRE(expectedIter->first.ref == actualIter->first.ref);
            REQUIRE(expectedIter->first.dir == actualIter->first.dir);
            REQUIRE(actualIter->second == Approx(expectedIter->second));
        }
    };

    for (size_t index = 0; index < serialConnections.size(); ++index) {
        REQUIRE(serialConnections[index].m_segment_axialref ==
                parallelConnections[index].m_segment_axialref);
        compareDirectional(serialConnections[index].m_back_segconns,
                           parallelConnections[index].m_back_segconns);
        compareDirectional(serialConnections[index].m_forward_segconns,
                           parallelConnections[index].m_forward_segconns);
    }
}

void requireSameAttributes(const ShapeGraph &serial, const ShapeGraph &parallel) {
    const AttributeTable &serialAttributes = serial.getAttributeTable();
    const AttributeTable &parallelAttributes = parallel.getAttributeTable();
    REQUIRE(serial.getShapeCount() == parallel.getShapeCount());
    REQUIRE(serialAttributes.getNumColumns() == parallelAttributes.getNumColumns());

    for (size_t column = 0; column < serialAttributes.getNumColumns(); ++column) {
        INFO("Column: " << serialAttributes.getColumnName(column));
        REQUIRE(serialAttributes.getColumnName(column) == parallelAttributes.getColumnName(column));
        for (size_t row = 0; row < serial.getShapeCount(); ++row) {
            const float serialValue = serial.getAttributeRowFromShapeIndex(row).getValue(column);
            const float parallelValue = parallel.getAttributeRowFromShapeIndex(row).getValue(column);
            if (std::isfinite(serialValue) && std::isfinite(parallelValue)) {
                REQUIRE(parallelValue == Approx(serialValue).epsilon(1e-5));
            } else {
                REQUIRE(std::isfinite(serialValue) == std::isfinite(parallelValue));
            }
        }
    }
}


} // namespace

TEST_CASE("Parallel axial analysis matches single-thread analysis") {
    std::unique_ptr<ShapeGraph> serial = makeAxialGraph();
    std::unique_ptr<ShapeGraph> parallel = makeAxialGraph();

    {
        AnalysisThreadSetting threads("1");
        REQUIRE(AxialIntegration({2.0, -1.0}, -1, true, true, true).run(nullptr, *serial, false));
    }
    {
        AnalysisThreadSetting threads("4");
        REQUIRE(AxialIntegration({2.0, -1.0}, -1, true, true, true).run(nullptr, *parallel, false));
    }

    requireSameAttributes(*serial, *parallel);
}

TEST_CASE("Parallel segment analyses match single-thread analyses") {
    std::unique_ptr<ShapeGraph> serial = makeSegmentGraph();
    std::unique_ptr<ShapeGraph> parallel = makeSegmentGraph();

    {
        AnalysisThreadSetting threads("1");
        REQUIRE(SegmentAngular({1.0, -1.0}).run(nullptr, *serial, false));
        REQUIRE(SegmentMetric(-1.0, false).run(nullptr, *serial, false));
        REQUIRE(SegmentTopological(-1.0, false).run(nullptr, *serial, false));
    }
    {
        AnalysisThreadSetting threads("4");
        REQUIRE(SegmentAngular({1.0, -1.0}).run(nullptr, *parallel, false));
        REQUIRE(SegmentMetric(-1.0, false).run(nullptr, *parallel, false));
        REQUIRE(SegmentTopological(-1.0, false).run(nullptr, *parallel, false));
    }

    requireSameAttributes(*serial, *parallel);
}

TEST_CASE("Parallel line connection discovery matches single-thread discovery") {
    std::unique_ptr<ShapeGraph> serial;
    std::unique_ptr<ShapeGraph> parallel;
    {
        AnalysisThreadSetting threads("1");
        serial = makeAxialGraph();
    }
    {
        AnalysisThreadSetting threads("4");
        parallel = makeAxialGraph();
    }
    requireSameGraphGeometryAndConnections(*serial, *parallel);
    requireSameAttributes(*serial, *parallel);
}

TEST_CASE("Parallel All-Line Stage 1 matches single-thread map creation") {
    std::unique_ptr<AllLineMap> serial;
    std::unique_ptr<AllLineMap> parallel;
    {
        AnalysisThreadSetting threads("1");
        serial = makeAllLineMap();
    }
    {
        AnalysisThreadSetting threads("4");
        parallel = makeAllLineMap();
    }
    requireSameGraphGeometryAndConnections(*serial, *parallel);
    requireSameAttributes(*serial, *parallel);

    std::tuple<std::unique_ptr<ShapeGraph>, std::unique_ptr<ShapeGraph>> serialFewest;
    std::tuple<std::unique_ptr<ShapeGraph>, std::unique_ptr<ShapeGraph>> parallelFewest;
    {
        AnalysisThreadSetting threads("1");
        serialFewest = serial->extractFewestLineMaps(nullptr);
    }
    {
        AnalysisThreadSetting threads("4");
        parallelFewest = parallel->extractFewestLineMaps(nullptr);
    }

    requireSameGraphGeometryAndConnections(*std::get<0>(serialFewest),
                                           *std::get<0>(parallelFewest));
    requireSameGraphGeometryAndConnections(*std::get<1>(serialFewest),
                                           *std::get<1>(parallelFewest));
}

TEST_CASE("Parallel direct segment-map creation matches single-thread creation") {
    std::unique_ptr<ShapeGraph> serial;
    std::unique_ptr<ShapeGraph> parallel;
    {
        AnalysisThreadSetting threads("1");
        serial = makeSegmentGraph();
    }
    {
        AnalysisThreadSetting threads("4");
        parallel = makeSegmentGraph();
    }

    requireSameGraphGeometryAndConnections(*serial, *parallel);
    requireSameSegmentConnections(*serial, *parallel);
    requireSameAttributes(*serial, *parallel);
}

TEST_CASE("Parallel axial-to-segment conversion matches single-thread conversion") {
    std::unique_ptr<ShapeGraph> serialAxial;
    std::unique_ptr<ShapeGraph> parallelAxial;
    {
        AnalysisThreadSetting threads("1");
        serialAxial = makeAxialGraph();
        parallelAxial = makeAxialGraph();
    }

    std::unique_ptr<ShapeGraph> serial;
    std::unique_ptr<ShapeGraph> parallel;
    {
        AnalysisThreadSetting threads("1");
        serial = MapConverter::convertAxialToSegment(nullptr, *serialAxial, "Segment", true, false, 0.0);
    }
    {
        AnalysisThreadSetting threads("4");
        parallel = MapConverter::convertAxialToSegment(nullptr, *parallelAxial, "Segment", true, false, 0.0);
    }

    requireSameGraphGeometryAndConnections(*serial, *parallel);
    requireSameSegmentConnections(*serial, *parallel);
    requireSameAttributes(*serial, *parallel);
}


TEST_CASE("Parallel Tulip analysis without Choice matches single-thread analysis") {
    std::unique_ptr<ShapeGraph> serial = makeSegmentGraph();
    std::unique_ptr<ShapeGraph> parallel = makeSegmentGraph();

    const int serialWeight = serial->getAttributeTable().insertOrResetColumn("Tulip Weight");
    const int parallelWeight = parallel->getAttributeTable().insertOrResetColumn("Tulip Weight");
    const int serialRouteWeight = serial->getAttributeTable().insertOrResetColumn("Tulip Route Weight");
    const int parallelRouteWeight = parallel->getAttributeTable().insertOrResetColumn("Tulip Route Weight");
    for (size_t row = 0; row < serial->getShapeCount(); ++row) {
        const float weight = static_cast<float>(1 + (row % 5));
        const float routeWeight = static_cast<float>(1 + (row % 7));
        serial->getAttributeRowFromShapeIndex(row).setValue(serialWeight, weight);
        parallel->getAttributeRowFromShapeIndex(row).setValue(parallelWeight, weight);
        serial->getAttributeRowFromShapeIndex(row).setValue(serialRouteWeight, routeWeight);
        parallel->getAttributeRowFromShapeIndex(row).setValue(parallelRouteWeight, routeWeight);
    }

    {
        AnalysisThreadSetting threads("1");
        REQUIRE(SegmentTulip({1.0, 2.0, -1.0}, false, 1024, serialWeight, Options::RADIUS_STEPS, false)
                    .run(nullptr, *serial, false));
        REQUIRE(SegmentTulip({0.5, 1.0, -1.0}, false, 1024, serialWeight, Options::RADIUS_ANGULAR, false)
                    .run(nullptr, *serial, false));
        REQUIRE(SegmentTulip({2.0, 5.0, -1.0}, false, 1024, serialWeight, Options::RADIUS_METRIC, false, false, -1,
                             serialRouteWeight)
                    .run(nullptr, *serial, false));
    }
    {
        AnalysisThreadSetting threads("4");
        REQUIRE(SegmentTulip({1.0, 2.0, -1.0}, false, 1024, parallelWeight, Options::RADIUS_STEPS, false)
                    .run(nullptr, *parallel, false));
        REQUIRE(SegmentTulip({0.5, 1.0, -1.0}, false, 1024, parallelWeight, Options::RADIUS_ANGULAR, false)
                    .run(nullptr, *parallel, false));
        REQUIRE(SegmentTulip({2.0, 5.0, -1.0}, false, 1024, parallelWeight, Options::RADIUS_METRIC, false, false, -1,
                             parallelRouteWeight)
                    .run(nullptr, *parallel, false));
    }

    requireSameAttributes(*serial, *parallel);
}

TEST_CASE("Tulip Choice remains serial and matches regardless of thread setting") {
    std::unique_ptr<ShapeGraph> serial = makeSegmentGraph();
    std::unique_ptr<ShapeGraph> requestedParallel = makeSegmentGraph();

    {
        AnalysisThreadSetting threads("1");
        REQUIRE(SegmentTulip({1.0, -1.0}, false, 1024, -1, Options::RADIUS_STEPS, true)
                    .run(nullptr, *serial, false));
    }
    {
        AnalysisThreadSetting threads("4");
        REQUIRE(SegmentTulip({1.0, -1.0}, false, 1024, -1, Options::RADIUS_STEPS, true)
                    .run(nullptr, *requestedParallel, false));
    }

    requireSameAttributes(*serial, *requestedParallel);
}
