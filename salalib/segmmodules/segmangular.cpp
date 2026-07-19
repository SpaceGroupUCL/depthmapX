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

#include "genlib/parallel.h"
#include "genlib/stringutils.h"

#include <algorithm>
#include <cstdint>

bool SegmentAngular::run(Communicator *comm, ShapeGraph &map, bool) {

    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return false;
    }

    AttributeTable &attributes = map.getAttributeTable();
    const size_t shapeCount = map.getShapeCount();

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
    for (double radius : radii) {
        std::string radius_text = makeRadiusText(Options::RADIUS_ANGULAR, radius);
        depth_col.push_back(attributes.insertOrResetColumn(std::string("Angular Mean Depth") + radius_text));
        count_col.push_back(attributes.insertOrResetColumn(std::string("Angular Node Count") + radius_text));
        total_col.push_back(attributes.insertOrResetColumn(std::string("Angular Total Depth") + radius_text));
    }

    if (comm) {
        comm->CommPostMessage(Communicator::NUM_RECORDS, static_cast<int>(shapeCount));
    }

    const size_t radiusCount = radii.size();
    std::vector<double> resultDepth(shapeCount * radiusCount, -1.0);
    std::vector<int> resultCount(shapeCount * radiusCount, 0);

    const size_t bytesPerWorker = shapeCount * sizeof(std::uint8_t);
    const size_t workerCount = depthmapX::configuredAnalysisThreadCount(shapeCount, bytesPerWorker);
    std::vector<std::vector<std::uint8_t>> covered(workerCount, std::vector<std::uint8_t>(shapeCount));

    depthmapX::parallelFor(shapeCount, workerCount, comm, [&](size_t root, size_t worker) {
        std::vector<std::uint8_t> &workerCovered = covered[worker];
        std::fill(workerCovered.begin(), workerCovered.end(), std::uint8_t(0));

        std::vector<std::pair<float, SegmentData>> anglebins;
        anglebins.push_back(std::make_pair(0.0f, SegmentData(0, root, SegmentRef(), 0, 0.0, 0)));

        std::vector<double> totalDepth(radiusCount, 0.0);
        std::vector<int> nodeCount(radiusCount, 0);
        size_t processed = 0;

        while (!anglebins.empty()) {
            if (comm && ((processed++ & 0x3ff) == 0) && comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }

            auto angleIter = anglebins.begin();
            SegmentData lineindex = angleIter->second;
            if (!workerCovered[lineindex.ref]) {
                workerCovered[lineindex.ref] = 1;
                const double depthToLine = angleIter->first;
                totalDepth[lineindex.coverage] += depthToLine;
                nodeCount[lineindex.coverage] += 1;
                anglebins.erase(angleIter);

                const Connector &line = map.getConnections()[lineindex.ref];
                if (lineindex.dir != -1) {
                    for (const auto &segconn : line.m_forward_segconns) {
                        if (!workerCovered[segconn.first.ref]) {
                            const double angle = depthToLine + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radiusCount && radii[rbin] != -1 && angle > radii[rbin]) {
                                ++rbin;
                            }
                            if (rbin != radiusCount) {
                                depthmapX::insert_sorted(
                                    anglebins,
                                    std::make_pair(float(angle), SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (const auto &segconn : line.m_back_segconns) {
                        if (!workerCovered[segconn.first.ref]) {
                            const double angle = depthToLine + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radiusCount && radii[rbin] != -1 && angle > radii[rbin]) {
                                ++rbin;
                            }
                            if (rbin != radiusCount) {
                                depthmapX::insert_sorted(
                                    anglebins,
                                    std::make_pair(float(angle), SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
            } else {
                anglebins.erase(angleIter);
            }
        }

        int cumulativeCount = 0;
        double cumulativeDepth = 0.0;
        for (size_t radiusIndex = 0; radiusIndex < radiusCount; ++radiusIndex) {
            cumulativeCount += nodeCount[radiusIndex];
            cumulativeDepth += totalDepth[radiusIndex];
            const size_t resultIndex = root * radiusCount + radiusIndex;
            resultCount[resultIndex] = cumulativeCount;
            resultDepth[resultIndex] = cumulativeDepth;
        }
    });

    size_t root = 0;
    for (auto &attributeIter : attributes) {
        if (comm && ((root & 0x3ff) == 0) && comm->IsCancelled()) {
            throw Communicator::CancelledException();
        }
        AttributeRow &row = attributeIter.getRow();
        for (size_t radiusIndex = 0; radiusIndex < radiusCount; ++radiusIndex) {
            const size_t resultIndex = root * radiusCount + radiusIndex;
            const int nodeCount = resultCount[resultIndex];
            const double totalDepth = resultDepth[resultIndex];
            row.setValue(count_col[radiusIndex], float(nodeCount));
            if (nodeCount > 1) {
                row.setValue(depth_col[radiusIndex], float(totalDepth / double(nodeCount - 1)));
                row.setValue(total_col[radiusIndex], float(totalDepth));
            } else {
                row.setValue(depth_col[radiusIndex], -1.0f);
                row.setValue(total_col[radiusIndex], -1.0f);
            }
        }
        ++root;
    }

    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(depth_col.back());

    return true;
}
