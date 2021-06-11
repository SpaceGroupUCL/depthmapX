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

#include "vgametricdepth.h"

#include "genlib/stringutils.h"

bool VGAMetricDepth::run(Communicator *) {

    AttributeTable &attributes = m_map.getAttributeTable();

    // n.b., insert columns sets values to -1 if the column already exists
    int path_angle_col = attributes.insertOrResetColumn("Metric Step Shortest-Path Angle");
    int path_length_col = attributes.insertOrResetColumn("Metric Step Shortest-Path Length");
    int dist_col = -1;
    if (m_map.getSelSet().size() == 1) {
        // Note: Euclidean distance is currently only calculated from a single point
        dist_col = attributes.insertOrResetColumn("Metric Straight-Line Distance");
    }

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        m_map.getPoint(pix).m_misc = 0;
        m_map.getPoint(pix).m_dist = -1.0f;
        m_map.getPoint(pix).m_cumangle = 0.0f;
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    for (auto &sel : m_map.getSelSet()) {
        search_list.insert(MetricTriple(0.0f, sel, NoPixel));
    }

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        Point &p = m_map.getPoint(here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractMetric(search_list, &m_map, here);
            p.m_misc = ~0;
            AttributeRow &row = m_map.getAttributeTable().getRow(AttributeKey(here.pixel));
            row.setValue(path_length_col, float(m_map.getSpacing() * here.dist));
            row.setValue(path_angle_col, float(p.m_cumangle));
            if (m_map.getSelSet().size() == 1) {
                // Note: Euclidean distance is currently only calculated from a single point
                row.setValue(dist_col, float(m_map.getSpacing() * dist(here.pixel, *m_map.getSelSet().begin())));
            }
            if (!p.getMergePixel().empty()) {
                Point &p2 = m_map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    p2.m_cumangle = p.m_cumangle;
                    AttributeRow &mergePixelRow = m_map.getAttributeTable().getRow(AttributeKey(p.getMergePixel()));
                    mergePixelRow.setValue(path_length_col, float(m_map.getSpacing() * here.dist));
                    mergePixelRow.setValue(path_angle_col, float(p2.m_cumangle));
                    if (m_map.getSelSet().size() == 1) {
                        // Note: Euclidean distance is currently only calculated from a single point
                        mergePixelRow.setValue(
                            dist_col, float(m_map.getSpacing() * dist(p.getMergePixel(), *m_map.getSelSet().begin())));
                    }
                    p2.getNode().extractMetric(search_list, &m_map,
                                               MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                    p2.m_misc = ~0;
                }
            }
        }
    }

    m_map.setDisplayedAttribute(-2);
    m_map.setDisplayedAttribute(path_length_col);

    return true;
}
