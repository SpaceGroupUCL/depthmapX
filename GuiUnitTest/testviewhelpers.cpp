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

#include "depthmapX/views/viewhelpers.h"
#include "depthmapX/views/plotview/plotview.h"
#include "catch.hpp"
#include <QImage>
#include <QPainter>
#include <time.h>
#include <sstream>
#include <iomanip>

TEST_CASE("A scatter plot includes negative pairs and skips incomplete pairs", "[signed-attributes]") {
    std::unique_ptr<MetaGraph> graph;
    QGraphDoc document(QString{}, QString{});
    graph.reset(document.m_meta_graph);
    const auto mapIndex = graph->addShapeMap("Signed values");
    auto &map = graph->getDataMaps().at(mapIndex);
    graph->setDisplayedDataMapRef(mapIndex);
    map.init(3, QtRegion(Point2f(0, 0), Point2f(10, 10)));
    auto &table = map.getAttributeTable();
    const auto x = table.getOrInsertColumn("X");
    const auto y = table.getOrInsertColumn("Y");
    table.getRow(AttributeKey(map.makePointShape(Point2f(1, 1)))).setValue(x, -3.0f).setValue(y, -5.0f);
    table.getRow(AttributeKey(map.makePointShape(Point2f(2, 2)))).setValue(x, -0.5f).setValue(y, -2.0f);
    table.getRow(AttributeKey(map.makePointShape(Point2f(3, 3)))).setValue(x, -100.0f);

    QPlotView plot;
    plot.pDoc = &document;
    plot.m_x_axis = x;
    plot.m_y_axis = y;
    plot.m_foreground = qRgb(0, 0, 0);
    plot.m_background = qRgb(255, 255, 255);
    plot.resize(320, 240);
    QImage image(plot.size(), QImage::Format_RGB32);
    image.fill(plot.m_background);
    QPainter painter(&image);

    REQUIRE(plot.Output(&painter, &document, true));
    REQUIRE(plot.m_data_bounds.bottom_left.x == -3.0);
    REQUIRE(plot.m_data_bounds.top_right.x == -0.5);
    REQUIRE(plot.m_data_bounds.bottom_left.y == -5.0);
    REQUIRE(plot.m_data_bounds.top_right.y == -2.0);
    plot.ResetRegression();
    REQUIRE(plot.m_regression.n == 2.0);
}

TEST_CASE("Calculating the new center", "[calculateCenter]"){
    auto point = QPoint(100, 100);
    auto oldCenter = QPoint(200,200);
    auto newCenter = Point2f(150,150);

    REQUIRE(ViewHelpers::calculateCenter(point, oldCenter, 0.5) == newCenter);

    newCenter.x = 300;
    newCenter.y = 300;

    REQUIRE(ViewHelpers::calculateCenter(point, oldCenter, 2.0) == newCenter);

}

TEST_CASE("Date string format", "[getCurrentDate]"){
    auto now = time(NULL);
    const tm* ltime = localtime(&now);
    std::stringstream sstream;
    sstream << ltime->tm_year + 1900 << "/" <<
            std::setfill('0') << std::setw(2) << ltime->tm_mon + 1 << "/" <<
            std::setfill('0') << std::setw(2) << ltime->tm_mday << std::flush;

    REQUIRE(ViewHelpers::getCurrentDate() ==  sstream.str().c_str());
}

