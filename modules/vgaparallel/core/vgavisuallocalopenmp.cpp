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

#include "vgavisuallocalopenmp.h"

#include "genlib/stringutils.h"

#include <omp.h>

bool VGAVisualLocalOpenMP::run(Communicator *comm) {
    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    AttributeTable &attributes = m_map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<AttributeRow *> rows;

    for (int i = 0; i < m_map.getCols(); i++) {
        for (int j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (m_map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowPtr(AttributeKey(curs)));
            }
        }
    }

    int count = 0;

    count = 0;

    std::vector<DataPoint> col_data(filled.size());

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
    }
    std::vector<std::set<int>> hoods(filled.size());

    int i, N = int(filled.size());
    std::map<PixelRef, int> refToFilled;
    for (i = 0; i < N; ++i) {
        refToFilled.insert(std::make_pair(filled[size_t(i)], i));
    }
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; ++i) {
        Point &p = m_map.getPoint(filled[size_t(i)]);
        std::set<PixelRef> neighbourhood;
#pragma omp critical(dumpNeighbourhood)
        { dumpNeighbourhood(p.getNode(), neighbourhood); }
        for (auto &neighbour : neighbourhood) {
            if (m_map.getPoint(neighbour).hasNode()) {
                hoods[size_t(i)].insert(refToFilled[neighbour]);
            }
        }
    }

#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; ++i) {

        DataPoint &dp = col_data[i];

        Point &p = m_map.getPoint(filled[size_t(i)]);
        if ((p.contextfilled() && !filled[size_t(i)].iseven())) {
            count++;
            continue;
        }

        // This is much easier to do with a straight forward list:
        std::set<int> &neighbourhood = hoods[size_t(i)];
        std::set<int> totalneighbourhood;
        int cluster = 0;
        float control = 0.0f;

        for (auto &neighbour : neighbourhood) {
            std::set<int> &retneighbourhood = hoods[size_t(neighbour)];
            std::set<int> intersect;
            std::set_intersection(neighbourhood.begin(), neighbourhood.end(), retneighbourhood.begin(),
                                  retneighbourhood.end(), std::inserter(intersect, intersect.begin()));
            totalneighbourhood.insert(retneighbourhood.begin(), retneighbourhood.end());
            control += 1.0f / float(retneighbourhood.size());
            cluster += intersect.size();
        }
#pragma omp critical(add_to_col)
        {
            if (neighbourhood.size() > 1) {
                dp.cluster = float(cluster / double(neighbourhood.size() * (neighbourhood.size() - 1.0)));
                dp.control = float(control);
                dp.controllability = float(double(neighbourhood.size()) / double(totalneighbourhood.size()));
            } else {
                dp.cluster = -1.0f;
                dp.control = -1.0f;
                dp.controllability = -1.0f;
            }
        }

#pragma omp critical(count)
        {
            count++; // <- increment count
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

    int cluster_col = attributes.insertOrResetColumn("Visual Clustering Coefficient");
    int control_col = attributes.insertOrResetColumn("Visual Control");
    int controllability_col = attributes.insertOrResetColumn("Visual Controllability");

    auto dataIter = col_data.begin();
    for (auto row : rows) {
        row->setValue(cluster_col, dataIter->cluster);
        row->setValue(control_col, dataIter->control);
        row->setValue(controllability_col, dataIter->controllability);
        dataIter++;
    }
    m_map.setDisplayedAttribute(cluster_col);

    return true;
}

void VGAVisualLocalOpenMP::dumpNeighbourhood(Node &node, std::set<PixelRef> &hood) const {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                hood.insert(pix);
                pix.move(bin.m_dir);
            }
        }
    }
}
