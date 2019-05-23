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

#include "salalib/segmmodules/segmmetric.h"

#include "genlib/stringutils.h"

bool SegmentMetric::run(Communicator *comm, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    bool retvar = true;

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              (m_sel_only ? map.getSelSet().size() : map.getConnections().size()));
    }
    int reccount = 0;

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow& row = map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(row.getValue(attributes.getColumnIndex("Axial Line Ref")));
        seglengths.push_back(row.getValue(attributes.getColumnIndex("Segment Length")));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    std::string prefix, suffix;
    int maxbin = 512;
    prefix = "Metric ";

    if (m_radius != -1.0) {
        suffix = dXstring::formatString(m_radius, " R%.f metric");
    }
    std::string choicecol = prefix + "Choice" + suffix;
    std::string wchoicecol = prefix + "Choice [SLW]" + suffix;
    std::string meandepthcol = prefix + "Mean Depth" + suffix;
    std::string wmeandepthcol = prefix + std::string("Mean Depth [SLW]") + suffix;
    std::string totaldcol = prefix + "Total Depth" + suffix;
    std::string totalcol = prefix + "Total Nodes" + suffix;
    std::string wtotalcol = prefix + "Total Length" + suffix;
    //
    if (!m_sel_only) {
        attributes.insertOrResetColumn(choicecol.c_str());
        attributes.insertOrResetColumn(wchoicecol.c_str());
    }
    attributes.insertOrResetColumn(meandepthcol.c_str());
    attributes.insertOrResetColumn(wmeandepthcol.c_str());
    attributes.insertOrResetColumn(totaldcol.c_str());
    attributes.insertOrResetColumn(totalcol.c_str());
    attributes.insertOrResetColumn(wtotalcol.c_str());
    //
    std::vector<unsigned int> seen(map.getShapeCount());
    std::vector<TopoMetSegmentRef> audittrail(map.getShapeCount());
    std::vector<TopoMetSegmentChoice> choicevals(map.getShapeCount());
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow& row = map.getAttributeRowFromShapeIndex(cursor);
        if (m_sel_only && !row.isSelected()) {
            continue;
        }
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            seen[i] = 0xffffffff;
        }
        std::vector<int> list[512]; // 512 bins!
        int bin = 0;
        list[bin].push_back(cursor);
        double rootseglength = seglengths[cursor];
        audittrail[cursor] = TopoMetSegmentRef(cursor, Connector::SEG_CONN_ALL, rootseglength * 0.5, -1);
        int open = 1;
        unsigned int segdepth = 0;
        double total = 0.0, wtotal = 0.0, wtotaldepth = 0.0, totalsegdepth = 0.0, totalmetdepth = 0.0;
        while (open != 0) {
            while (list[bin].size() == 0) {
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
            //
            if (here.done) {
                continue;
            } else {
                here.done = true;
            }
            //
            double len = seglengths[here.ref];
            totalsegdepth += segdepth;
            totalmetdepth += here.dist - len * 0.5; // preloaded with length ahead
            wtotal += len;
            wtotaldepth += len * (here.dist - len * 0.5);
            total += 1;
            //
            Connector &axline = map.getConnections().at(here.ref);
            int connected_cursor = -2;

            auto iter = axline.m_back_segconns.begin();
            bool backsegs = true;

            while (connected_cursor != -1) {
                if (backsegs && iter == axline.m_back_segconns.end()) {
                    iter = axline.m_forward_segconns.begin();
                    backsegs = false;
                }
                if (!backsegs && iter == axline.m_forward_segconns.end()) {
                    break;
                }

                connected_cursor = iter->first.ref;

                if (seen[connected_cursor] > segdepth && static_cast<size_t>(connected_cursor) != cursor) {
                    bool seenalready = (seen[connected_cursor] == 0xffffffff) ? false : true;
                    float length = seglengths[connected_cursor];
                    audittrail[connected_cursor] =
                        TopoMetSegmentRef(connected_cursor, here.dir, here.dist + length, here.ref);
                    seen[connected_cursor] = segdepth;
                    if (m_radius == -1 || here.dist + length < m_radius) {
                        // puts in a suitable bin ahead of us...
                        open++;
                        //
                        // better to divide by 511 but have 512 bins...
                        list[(bin + int(floor(0.5 + 511 * length / maxseglength))) % 512].push_back(connected_cursor);
                    }
                    // not sure why this is outside the radius restriction
                    // (sel_only: with restricted selection set, not all lines will be labelled)
                    // (seenalready: need to check that we're not doing this twice, given the seen can go twice)

                    // Quick mod - TV
                    if (!m_sel_only && connected_cursor > int(cursor) &&
                        !seenalready) { // only one way paths, saves doing this twice
                        int subcur = connected_cursor;
                        while (subcur != -1) {
                            // in this method of choice, start and end lines are included
                            choicevals[subcur].choice += 1;
                            choicevals[subcur].wchoice += (rootseglength * length);
                            subcur = audittrail[subcur].previous;
                        }
                    }
                }
                iter++;
            }
        }
        // also put in mean depth:
        //
        row.setValue(meandepthcol.c_str(), totalmetdepth / (total - 1));
        row.setValue(totaldcol.c_str(), totalmetdepth);
        row.setValue(wmeandepthcol.c_str(), wtotaldepth / (wtotal - rootseglength));
        row.setValue(totalcol.c_str(), total);
        row.setValue(wtotalcol.c_str(), wtotal);
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
            }
            comm->CommPostMessage(Communicator::CURRENT_RECORD, reccount);
        }
        reccount++;
    }
    if (!m_sel_only) {
        // note, I've stopped sel only from calculating choice values:
        for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
            AttributeRow& row = map.getAttributeRowFromShapeIndex(cursor);
            row.setValue(choicecol.c_str(), choicevals[cursor].choice);
            row.setValue(wchoicecol.c_str(), choicevals[cursor].wchoice);
        }
    }

    if (!m_sel_only) {
        map.setDisplayedAttribute(attributes.getColumnIndex(choicecol.c_str()));
    } else {
        map.setDisplayedAttribute(attributes.getColumnIndex(meandepthcol.c_str()));
    }

    return retvar;
}
