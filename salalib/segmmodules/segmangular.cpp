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

#include "salalib/segmmodules/segmangular.h"
#include "salalib/options.h"

#include "genlib/stringutils.h"

bool SegmentAngular::run(Communicator *comm, ShapeGraph &map, bool) {

    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return false;
    }

    AttributeTable &attributes = map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getConnections().size());
    }

    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radius_n = false;
    std::vector<double> radii;
    for (double radius : m_radius_set) {
        if (radius < 0) {
            radius_n = true;
        } else {
            radii.push_back(radius);
        }
    }
    if (radius_n) {
        radii.push_back(-1.0);
    }

    std::vector<int> depth_col, count_col, total_col;
    // first enter table values
    for (int radius : radii) {
        std::string radius_text = makeRadiusText(Options::RADIUS_ANGULAR, radius);
        std::string depth_col_text = std::string("Angular Mean Depth") + radius_text;
        attributes.insertOrResetColumn(depth_col_text.c_str());
        std::string count_col_text = std::string("Angular Node Count") + radius_text;
        attributes.insertOrResetColumn(count_col_text.c_str());
        std::string total_col_text = std::string("Angular Total Depth") + radius_text;
        attributes.insertOrResetColumn(total_col_text.c_str());
    }

    for (int radius : radii) {
        std::string radius_text = makeRadiusText(Options::RADIUS_ANGULAR, radius);
        std::string depth_col_text = std::string("Angular Mean Depth") + radius_text;
        depth_col.push_back(attributes.getColumnIndex(depth_col_text.c_str()));
        std::string count_col_text = std::string("Angular Node Count") + radius_text;
        count_col.push_back(attributes.getColumnIndex(count_col_text.c_str()));
        std::string total_col_text = std::string("Angular Total Depth") + radius_text;
        total_col.push_back(attributes.getColumnIndex(total_col_text.c_str()));
    }

    std::vector<bool> covered(map.getShapeCount());
    size_t i = 0;
    for (auto & iter : attributes){
        for (size_t j = 0; j < map.getShapeCount(); j++) {
            covered[j] = false;
        }
        std::vector<std::pair<float, SegmentData>> anglebins;
        anglebins.push_back(std::make_pair(0.0f, SegmentData(0, i, SegmentRef(), 0, 0.0, 0)));

        std::vector<double> total_depth;
        std::vector<int> node_count;
        for (size_t r = 0; r < radii.size(); r++) {
            total_depth.push_back(0.0);
            node_count.push_back(0);
        }
        // node_count includes this one, but will be added in next algo:
        while (anglebins.size()) {
            auto iter = anglebins.begin();
            SegmentData lineindex = iter->second;
            if (!covered[lineindex.ref]) {
                covered[lineindex.ref] = true;
                double depth_to_line = iter->first;
                total_depth[lineindex.coverage] += depth_to_line;
                node_count[lineindex.coverage] += 1;
                anglebins.erase(iter);
                Connector &line = map.getConnections()[lineindex.ref];
                if (lineindex.dir != -1) {
                    for (auto &segconn : line.m_forward_segconns) {
                        if (!covered[segconn.first.ref]) {
                            double angle = depth_to_line + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radii.size() && radii[rbin] != -1 && angle > radii[rbin]) {
                                rbin++;
                            }
                            if (rbin != radii.size()) {
                                depthmapX::insert_sorted(
                                    anglebins, std::make_pair(float(angle),
                                                              SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (auto &segconn : line.m_back_segconns) {
                        if (!covered[segconn.first.ref]) {
                            double angle = depth_to_line + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radii.size() && radii[rbin] != -1 && angle > radii[rbin]) {
                                rbin++;
                            }
                            if (rbin != radii.size()) {
                                depthmapX::insert_sorted(
                                    anglebins, std::make_pair(float(angle),
                                                              SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
            } else {
                anglebins.erase(iter);
            }
        }
        AttributeRow &row = iter.getRow();
        // set the attributes for this node:
        int curs_node_count = 0;
        double curs_total_depth = 0.0;
        for (size_t r = 0; r < radii.size(); r++) {
            curs_node_count += node_count[r];
            curs_total_depth += total_depth[r];
            row.setValue(count_col[r], float(curs_node_count));
            if (curs_node_count > 1) {
                // note -- node_count includes this one -- mean depth as per p.108 Social Logic of Space
                double mean_depth = curs_total_depth / double(curs_node_count - 1);
                row.setValue(depth_col[r], float(mean_depth));
                row.setValue(total_col[r], float(curs_total_depth));
            } else {
                row.setValue(depth_col[r], -1);
                row.setValue(total_col[r], -1);
            }
        }
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
        i++;
    }

    map.setDisplayedAttribute(-2); // <- override if it's already showing
    map.setDisplayedAttribute(depth_col.back());

    return true;
}
