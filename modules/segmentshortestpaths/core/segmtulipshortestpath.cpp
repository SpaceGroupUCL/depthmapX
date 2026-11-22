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

#include "segmtulipshortestpath.h"

#include "genlib/stringutils.h"

// revised to use tulip bins for faster analysis of large spaces

bool SegmentTulipShortestPath::run(Communicator *comm) {
    Communicator::checkCancelled(comm);

    AttributeTable &attributes = m_map.getAttributeTable();

    const size_t resultCount = m_map.getShapeCount();
    std::vector<float> costs(resultCount, -1.0f);
    std::vector<int> order(resultCount, -1);

    // The original code set tulip_bins to 1024, divided by two and added one
    // in order to duplicate previous code (using a semicircle of tulip bins)
    size_t tulip_bins = 513;

    std::vector<bool> covered(m_map.getConnections().size());
    for (size_t i = 0; i < m_map.getConnections().size(); i++) {
        covered[i] = false;
    }
    std::vector<std::vector<SegmentData>> bins(tulip_bins);

    auto &selected = m_map.getSelSet();
    if (selected.size() != 2) {
        return false;
    }
    int refFrom = *selected.begin();
    int refTo = *selected.rbegin();

    int opencount = 0;

    int row = std::distance(m_map.getAllShapes().begin(), m_map.getAllShapes().find(refFrom));
    if (row != -1) {
        bins[0].push_back(SegmentData(0, row, SegmentRef(), 0, 0.0, 0));
        opencount++;
    }

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    int depthlevel = 0;
    auto binIter = bins.begin();
    int currentbin = 0;
    Communicator *progress = comm;
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
    }
    while (opencount && !refFound) {
        Communicator::checkCancelled(comm);
        if (progress) {
            progress->CommPostMessage(Communicator::CURRENT_STEP, 1);
            progress = nullptr;
            Communicator::checkCancelled(comm);
        }
        while (binIter->empty()) {
            depthlevel++;
            binIter++;
            currentbin++;
            if (binIter == bins.end()) {
                binIter = bins.begin();
            }
        }
        SegmentData lineindex;
        if (binIter->size() > 1) {
            // it is slightly slower to delete from an arbitrary place in the bin,
            // but it is necessary to use random paths to even out the number of times through equal paths
            int curr = pafrand() % binIter->size();
            auto currIter = binIter->begin() + curr;
            lineindex = *currIter;
            binIter->erase(currIter);
            // note: do not clear choice values here!
        } else {
            lineindex = binIter->front();
            binIter->pop_back();
        }
        opencount--;
        if (!covered[lineindex.ref]) {
            covered[lineindex.ref] = true;
            Connector &line = m_map.getConnections()[lineindex.ref];
            // convert depth from tulip_bins normalised to standard angle
            // (note the -1)
            double depth_to_line = depthlevel / ((tulip_bins - 1) * 0.5);
            costs[lineindex.ref] = depth_to_line;
            int extradepth;
            if (lineindex.dir != -1) {
                for (auto &segconn : line.m_forward_segconns) {
                    Communicator::checkCancelled(comm);
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0, 0));
                        if(parents.find(segconn.first.ref) == parents.end()) {
                            parents[segconn.first.ref] = lineindex.ref;
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.dir != 1) {
                for (auto &segconn : line.m_back_segconns) {
                    Communicator::checkCancelled(comm);
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0, 0));
                        if(parents.find(segconn.first.ref) == parents.end()) {
                            parents[segconn.first.ref] = lineindex.ref;
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.ref == refTo) {
                refFound = true;
                break;
            }
        }
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
    }
    auto refToParent = parents.find(refTo);
    int counter = 0;
    while (refToParent != parents.end()) {
        Communicator::checkCancelled(comm);
        order[refToParent->first] = counter++;
        refToParent = parents.find(refToParent->second);
    }
    order[refFrom] = counter;

    // Publish complete results only once the analysis has passed its last cancellation check.
    Communicator::checkCancelled(comm);
    const auto angle_col = attributes.insertOrResetColumn("Angular Shortest Path Angle");
    const auto path_col = attributes.insertOrResetColumn("Angular Shortest Path Order");
    size_t index = 0;
    for (auto &item : attributes) {
        if (order[index] >= 0) {
            item.getRow().setValue(angle_col, costs[index]);
            item.getRow().setValue(path_col, counter - order[index]);
        }
        ++index;
    }

    m_map.overrideDisplayedAttribute(-2); // <- override if it's already showing
    m_map.setDisplayedAttribute(angle_col);

    return true;
}
