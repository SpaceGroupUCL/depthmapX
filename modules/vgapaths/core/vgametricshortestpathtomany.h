// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2018, Petros Koutsolampros

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

#include "salalib/ianalysis.h"
#include "salalib/options.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAMetricShortestPathToMany : public IAnalysis {
  private:
    PointMap &m_map;
    const std::set<PixelRef> m_pixelsFrom;
    const std::set<PixelRef> m_pixelsTo;

    struct MetricPoint {
        Point *m_point = nullptr;
        float m_linkCost = 0;
        float m_dist = -1.0f;
        float m_cumdist = -1.0f;
        bool m_unseen = true;
    };
    MetricPoint &getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return (metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
    void extractMetric(Node n, depthmapX::ColumnMatrix<MetricPoint> &metricPoints, std::set<MetricTriple> &pixels,
                       PointMap *pointdata, const MetricTriple &curs);

  public:
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    bool run(Communicator *) override;
    VGAMetricShortestPathToMany(PointMap &map, std::set<PixelRef> pixelsFrom, std::set<PixelRef> pixelsTo)
        : m_map(map), m_pixelsFrom(pixelsFrom), m_pixelsTo(pixelsTo) {}
};
