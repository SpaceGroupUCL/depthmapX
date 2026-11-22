// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2026, Tasos Varoudis
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

#include "salalib/vgamodules/vgametricdepth.h"

#include "genlib/stringutils.h"

#include <algorithm>

bool VGAMetricDepth::run(Communicator *comm, PointMap &map, bool) {
    Communicator::checkCancelled(comm);

    AttributeTable &attributes = map.getAttributeTable();

    // n.b., insert columns sets values to -1 if the column already exists
    int penn_dist_col = -1;
    if (map.getSelSet().size() == 1) {
        penn_dist_col = attributes.insertOrResetColumn("Metric Step Penn Distance");
    }
    int path_angle_col = attributes.insertOrResetColumn("Metric Step Shortest-Path Angle");
    int path_length_col = attributes.insertOrResetColumn("Metric Step Shortest-Path Length");
    int dist_col = -1;
    if (map.getSelSet().size() == 1) {
        // Note: Euclidean distance is currently only calculated from a single point
        dist_col = attributes.insertOrResetColumn("Metric Straight-Line Distance");
    }

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        Communicator::checkCancelled(comm);
        PixelRef pix = iter->getKey().value;
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_dist = -1.0f;
        map.getPoint(pix).m_cumangle = 0.0f;
    }

    // The previous pixel in each triple is used to accumulate the angle along the metric shortest path.
    std::set<MetricTriple> search_list; // contains root point

    for (auto &sel : map.getSelSet()) {
        Communicator::checkCancelled(comm);
        search_list.insert(MetricTriple(0.0f, sel, NoPixel));
    }

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    while (search_list.size()) {
        Communicator::checkCancelled(comm);
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        Point &p = map.getPoint(here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractMetric(search_list, &map, here);
            p.m_misc = ~0;
            AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(here.pixel));
            const float path_length = float(map.getSpacing() * here.dist);
            row.setValue(path_length_col, path_length);
            row.setValue(path_angle_col, float(p.m_cumangle));
            if (map.getSelSet().size() == 1) {
                // Note: Euclidean distance is currently only calculated from a single point
                const float straight_line_distance =
                    float(map.getSpacing() * dist(here.pixel, *map.getSelSet().begin()));
                row.setValue(dist_col, straight_line_distance);
                row.setValue(penn_dist_col, std::max(0.0f, path_length - straight_line_distance));
            }
            if (!p.getMergePixel().empty()) {
                Point &p2 = map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    p2.m_cumangle = p.m_cumangle;
                    AttributeRow &mergePixelRow =
                        map.getAttributeTable().getRow(AttributeKey(p.getMergePixel()));
                    const float path_length = float(map.getSpacing() * here.dist);
                    mergePixelRow.setValue(path_length_col, path_length);
                    mergePixelRow.setValue(path_angle_col, float(p2.m_cumangle));
                    if (map.getSelSet().size() == 1) {
                        // Note: Euclidean distance is currently only calculated from a single point
                        const float straight_line_distance =
                            float(map.getSpacing() * dist(p.getMergePixel(), *map.getSelSet().begin()));
                        mergePixelRow.setValue(dist_col, straight_line_distance);
                        mergePixelRow.setValue(penn_dist_col,
                                               std::max(0.0f, path_length - straight_line_distance));
                    }
                    p2.getNode().extractMetric(search_list, &map, MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                    p2.m_misc = ~0;
                }
            }
        }
    }

    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(path_length_col);

    return true;
}
