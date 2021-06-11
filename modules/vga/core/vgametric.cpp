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

#include "vgametric.h"

#include "genlib/stringutils.h"

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

bool VGAMetric::run(Communicator *comm) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    std::string radius_text;
    if (m_radius != -1.0) {
        if (m_radius > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.f");
        } else if (m_map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.2f");
        }
    }
    AttributeTable &attributes = m_map.getAttributeTable();

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mspa_col_text = std::string("Metric Mean Shortest-Path Angle") + radius_text;
    int mspa_col = attributes.insertOrResetColumn(mspa_col_text.c_str());
    std::string mspl_col_text = std::string("Metric Mean Shortest-Path Distance") + radius_text;
    int mspl_col = attributes.insertOrResetColumn(mspl_col_text.c_str());
    std::string dist_col_text = std::string("Metric Mean Straight-Line Distance") + radius_text;
    int dist_col = attributes.insertOrResetColumn(dist_col_text.c_str());
    std::string count_col_text = std::string("Metric Node Count") + radius_text;
    int count_col = attributes.insertOrResetColumn(count_col_text.c_str());

    int count = 0;

    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            if (m_map.getPoint(curs).filled()) {

                if (m_gates_only) {
                    count++;
                    continue;
                }

                // TODO: Break out miscs/dist/cumangle into local variables and remove from Point class
                for (auto &point : m_map.getPoints()) {
                    point.m_misc = 0;
                    point.m_dist = -1.0f;
                    point.m_cumangle = 0.0f;
                }

                float euclid_depth = 0.0f;
                float total_depth = 0.0f;
                float total_angle = 0.0f;
                int total_nodes = 0;

                // note that m_misc is used in a different manner to analyseGraph / PointDepth
                // here it marks the node as used in calculation only

                std::set<MetricTriple> search_list;
                search_list.insert(MetricTriple(0.0f, curs, NoPixel));
                while (search_list.size()) {
                    std::set<MetricTriple>::iterator it = search_list.begin();
                    MetricTriple here = *it;
                    search_list.erase(it);
                    if (m_radius != -1.0 && (here.dist * m_map.getSpacing()) > m_radius) {
                        break;
                    }
                    Point &p = m_map.getPoint(here.pixel);
                    // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
                    if (p.filled() && p.m_misc != ~0) {
                        p.getNode().extractMetric(search_list, &m_map, here);
                        p.m_misc = ~0;
                        if (!p.getMergePixel().empty()) {
                            Point &p2 = m_map.getPoint(p.getMergePixel());
                            if (p2.m_misc != ~0) {
                                p2.m_cumangle = p.m_cumangle;
                                p2.getNode().extractMetric(search_list, &m_map,
                                                           MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                                p2.m_misc = ~0;
                            }
                        }
                        total_depth += float(here.dist * m_map.getSpacing());
                        total_angle += p.m_cumangle;
                        euclid_depth += float(m_map.getSpacing() * dist(here.pixel, curs));
                        total_nodes += 1;
                    }
                }

                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                row.setValue(mspa_col, float(double(total_angle) / double(total_nodes)));
                row.setValue(mspl_col, float(double(total_depth) / double(total_nodes)));
                row.setValue(dist_col, float(double(euclid_depth) / double(total_nodes)));
                row.setValue(count_col, float(total_nodes));

                count++; // <- increment count
            }
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
        }
    }

    m_map.overrideDisplayedAttribute(-2);
    m_map.setDisplayedAttribute(mspl_col);

    return true;
}
