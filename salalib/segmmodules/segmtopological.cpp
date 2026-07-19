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

#include "salalib/segmmodules/segmtopological.h"

#include "genlib/parallel.h"
#include "genlib/stringutils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
struct SegmentTopologicalResult {
    double meanDepth = -1.0;
    double weightedMeanDepth = -1.0;
    double totalDepth = -1.0;
    double totalNodes = 0.0;
    double totalLength = 0.0;
};

struct SegmentTopologicalWorkerState {
    std::vector<unsigned int> seen;
    std::vector<TopoMetSegmentRef> audittrail;
    std::array<std::vector<int>, 512> bins;
    std::vector<TopoMetSegmentChoice> choice;

    SegmentTopologicalWorkerState(size_t shapeCount, bool calculateChoice)
        : seen(shapeCount), audittrail(shapeCount), choice(calculateChoice ? shapeCount : 0) {}
};
} // namespace

bool SegmentTopological::run(Communicator *comm, ShapeGraph &map, bool) {
    AttributeTable &attributes = map.getAttributeTable();
    const size_t shapeCount = map.getShapeCount();

    std::vector<int> axialrefs;
    axialrefs.reserve(shapeCount);
    std::vector<float> seglengths;
    seglengths.reserve(shapeCount);
    std::vector<size_t> roots;
    roots.reserve(shapeCount);
    for (size_t cursor = 0; cursor < shapeCount; ++cursor) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(static_cast<int>(row.getValue(attributes.getColumnIndex("Axial Line Ref"))));
        const float length = row.getValue(attributes.getColumnIndex("Segment Length"));
        seglengths.push_back(length);
        if (!m_sel_only || row.isSelected()) {
            roots.push_back(cursor);
        }
    }

    std::string suffix;
    if (m_radius != -1.0) {
        suffix = dXstring::formatString(m_radius, " R%.f metric");
    }
    const std::string prefix = "Topological ";
    const std::string choicecol = prefix + "Choice" + suffix;
    const std::string wchoicecol = prefix + "Choice [SLW]" + suffix;
    const std::string meandepthcol = prefix + "Mean Depth" + suffix;
    const std::string wmeandepthcol = prefix + "Mean Depth [SLW]" + suffix;
    const std::string totaldcol = prefix + "Total Depth" + suffix;
    const std::string totalcol = prefix + "Total Nodes" + suffix;
    const std::string wtotalcol = prefix + "Total Length" + suffix;

    if (!m_sel_only) {
        attributes.insertOrResetColumn(choicecol);
        attributes.insertOrResetColumn(wchoicecol);
    }
    attributes.insertOrResetColumn(meandepthcol);
    attributes.insertOrResetColumn(wmeandepthcol);
    attributes.insertOrResetColumn(totaldcol);
    attributes.insertOrResetColumn(totalcol);
    attributes.insertOrResetColumn(wtotalcol);

    if (comm) {
        comm->CommPostMessage(Communicator::NUM_RECORDS, static_cast<int>(roots.size()));
    }

    std::vector<SegmentTopologicalResult> results(shapeCount);
    const size_t estimatedBytesPerWorker =
        shapeCount * (sizeof(unsigned int) + sizeof(TopoMetSegmentRef) + (!m_sel_only ? sizeof(TopoMetSegmentChoice) : 0));
    const size_t workerCount = depthmapX::configuredAnalysisThreadCount(roots.size(), estimatedBytesPerWorker);

    std::vector<SegmentTopologicalWorkerState> workerStates;
    workerStates.reserve(workerCount);
    for (size_t worker = 0; worker < workerCount; ++worker) {
        workerStates.emplace_back(shapeCount, !m_sel_only);
    }

    depthmapX::parallelFor(roots.size(), workerCount, comm, [&](size_t task, size_t workerIndex) {
        const size_t cursor = roots[task];
        SegmentTopologicalWorkerState &state = workerStates[workerIndex];
        std::fill(state.seen.begin(), state.seen.end(), std::numeric_limits<unsigned int>::max());
        for (auto &bin : state.bins) {
            bin.clear();
        }

        int bin = 0;
        state.bins[bin].push_back(static_cast<int>(cursor));
        const double rootseglength = seglengths[cursor];
        state.audittrail[cursor] =
            TopoMetSegmentRef(static_cast<int>(cursor), Connector::SEG_CONN_ALL, rootseglength * 0.5, -1);

        int open = 1;
        unsigned int segdepth = 0;
        double total = 0.0;
        double wtotal = 0.0;
        double wtotaldepth = 0.0;
        double totalmetdepth = 0.0;
        size_t processed = 0;

        while (open != 0) {
            if (comm && ((processed++ & 0x3ff) == 0) && comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            while (state.bins[bin].empty()) {
                bin = (bin + 1) % 2;
                ++segdepth;
            }

            TopoMetSegmentRef &here = state.audittrail[state.bins[bin].back()];
            state.bins[bin].pop_back();
            --open;
            if (here.done) {
                continue;
            }
            here.done = true;

            const double len = seglengths[here.ref];
            totalmetdepth += segdepth;
            wtotal += len;
            wtotaldepth += len * segdepth;
            total += 1.0;

            const Connector &axline = map.getConnections().at(here.ref);
            auto processConnection = [&](int connected_cursor) {
                if (connected_cursor < 0 || static_cast<size_t>(connected_cursor) >= shapeCount) {
                    return;
                }
                if (state.seen[connected_cursor] > segdepth && static_cast<size_t>(connected_cursor) != cursor) {
                    const bool seenalready = state.seen[connected_cursor] != std::numeric_limits<unsigned int>::max();
                    const float length = seglengths[connected_cursor];
                    state.audittrail[connected_cursor] =
                        TopoMetSegmentRef(connected_cursor, here.dir, here.dist + length, here.ref);
                    state.seen[connected_cursor] = segdepth;
                    if (m_radius == -1 || here.dist + length < m_radius) {
                        ++open;
                        const int axialref = axialrefs[connected_cursor];
                        if (axialrefs[here.ref] == axialref) {
                            state.bins[bin].push_back(connected_cursor);
                        } else {
                            state.bins[(bin + 1) % 2].push_back(connected_cursor);
                            state.seen[connected_cursor] = segdepth + 1;
                        }
                    }
                    if (!m_sel_only && connected_cursor > static_cast<int>(cursor) && !seenalready) {
                        int subcur = connected_cursor;
                        while (subcur != -1) {
                            state.choice[subcur].choice += 1.0;
                            state.choice[subcur].wchoice += rootseglength * length;
                            subcur = state.audittrail[subcur].previous;
                        }
                    }
                }
            };

            for (const auto &connection : axline.m_back_segconns) {
                processConnection(connection.first.ref);
            }
            for (const auto &connection : axline.m_forward_segconns) {
                processConnection(connection.first.ref);
            }
        }

        SegmentTopologicalResult &result = results[cursor];
        result.totalDepth = totalmetdepth;
        result.totalNodes = total;
        result.totalLength = wtotal;
        if (total > 1.0) {
            result.meanDepth = totalmetdepth / (total - 1.0);
        }
        if (wtotal > rootseglength) {
            result.weightedMeanDepth = wtotaldepth / (wtotal - rootseglength);
        }
    });

    for (size_t cursor : roots) {
        if (comm && ((cursor & 0x3ff) == 0) && comm->IsCancelled()) {
            throw Communicator::CancelledException();
        }
        const SegmentTopologicalResult &result = results[cursor];
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        row.setValue(meandepthcol, float(result.meanDepth));
        row.setValue(totaldcol, float(result.totalDepth));
        row.setValue(wmeandepthcol, float(result.weightedMeanDepth));
        row.setValue(totalcol, float(result.totalNodes));
        row.setValue(wtotalcol, float(result.totalLength));
    }

    if (!m_sel_only) {
        for (size_t cursor = 0; cursor < shapeCount; ++cursor) {
            if (comm && ((cursor & 0x3ff) == 0) && comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            double choice = 0.0;
            double weightedChoice = 0.0;
            for (size_t worker = 0; worker < workerCount; ++worker) {
                choice += workerStates[worker].choice[cursor].choice;
                weightedChoice += workerStates[worker].choice[cursor].wchoice;
            }
            AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
            row.setValue(choicecol, float(choice));
            row.setValue(wchoicecol, float(weightedChoice));
        }
        map.setDisplayedAttribute(attributes.getColumnIndex(choicecol));
    } else {
        map.setDisplayedAttribute(attributes.getColumnIndex(meandepthcol));
    }

    return true;
}
