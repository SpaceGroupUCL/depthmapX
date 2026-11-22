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

#include "segmtopologicalshortestpath.h"

#include "genlib/stringutils.h"

bool SegmentTopologicalShortestPath::run(Communicator *comm) {
    Communicator::checkCancelled(comm);

    AttributeTable &attributes = m_map.getAttributeTable();
    int shapeCount = m_map.getShapeCount();

    bool retvar = true;

    const size_t resultCount = m_map.getShapeCount();
    std::vector<float> costs(resultCount, -1.0f);
    std::vector<int> order(resultCount, -1);

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < shapeCount; cursor++) {
        Communicator::checkCancelled(comm);
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(row.getValue("Axial Line Ref"));
        seglengths.push_back(row.getValue("Segment Length"));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    int maxbin = 2;

    std::vector<unsigned int> seen(shapeCount, 0xffffffff);
    std::vector<TopoMetSegmentRef> audittrail(shapeCount);
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    auto &selected = m_map.getSelSet();
    if (selected.size() != 2) {
        return false;
    }
    int refFrom = *selected.begin();
    int refTo = *selected.rbegin();

    seen[refFrom] = 0;
    open++;
    double length = seglengths[refFrom];
    audittrail[refFrom] = TopoMetSegmentRef(refFrom, Connector::SEG_CONN_ALL, length * 0.5, -1);
    list[0].push_back(refFrom);
    costs[refFrom] = 0;

    unsigned int segdepth = 0;
    int bin = 0;

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    Communicator *progress = comm;
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
    }
    while (open != 0) {
        Communicator::checkCancelled(comm);
        if (progress) {
            progress->CommPostMessage(Communicator::CURRENT_STEP, 1);
            progress = nullptr;
            Communicator::checkCancelled(comm);
        }
        while (list[bin].empty()) {
            bin++;
            segdepth += 1;
            if (bin == maxbin) {
                bin = 0;
            }
        }
        //
        TopoMetSegmentRef &here = audittrail[list[bin].back()];
        list[bin].pop_back();
        open--;
        // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
        if (here.done) {
            continue;
        } else {
            here.done = true;
        }

        Connector &axline = m_map.getConnections().at(here.ref);
        int connected_cursor = -2;

        auto iter = axline.m_back_segconns.begin();
        bool backsegs = true;

        while (connected_cursor != -1) {
            Communicator::checkCancelled(comm);
            if (backsegs && iter == axline.m_back_segconns.end()) {
                iter = axline.m_forward_segconns.begin();
                backsegs = false;
            }
            if (!backsegs && iter == axline.m_forward_segconns.end()) {
                break;
            }

            connected_cursor = iter->first.ref;
            if (seen[connected_cursor] > segdepth) {
                float length = seglengths[connected_cursor];
                int axialref = axialrefs[connected_cursor];
                seen[connected_cursor] = segdepth;
                audittrail[connected_cursor] =
                    TopoMetSegmentRef(connected_cursor, here.dir, here.dist + length, here.ref);
                // puts in a suitable bin ahead of us...
                open++;
                //
                if (axialrefs[here.ref] == axialref) {
                    list[bin].push_back(connected_cursor);
                    costs[connected_cursor] = segdepth;
                } else {
                    list[(bin + 1) % 2].push_back(connected_cursor);
                    seen[connected_cursor] =
                        segdepth +
                        1; // this is so if another node is connected directly to this one but is found later it is
                           // still handled -- note it can result in the connected cursor being added twice
                    costs[connected_cursor] = segdepth + 1;
                }
                if(parents.find(connected_cursor) == parents.end()) {
                    parents[connected_cursor] = here.ref;
                }
            }
            if (connected_cursor == refTo) {
                refFound = true;
                break;
            }
            iter++;
        }
        if (refFound)
            break;
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
    const auto depth_col = attributes.insertOrResetColumn("Topological Shortest Path Depth");
    const auto path_col = attributes.insertOrResetColumn("Topological Shortest Path Order");
    size_t index = 0;
    for (auto &item : attributes) {
        if (order[index] >= 0) {
            item.getRow().setValue(depth_col, costs[index]);
            item.getRow().setValue(path_col, counter - order[index]);
        }
        ++index;
    }

    m_map.overrideDisplayedAttribute(-2); // <- override if it's already showing
    m_map.setDisplayedAttribute(depth_col);

    return retvar;
}
