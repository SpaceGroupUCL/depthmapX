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

#include "vgavisualglobalopenmp.h"

#include "genlib/stringutils.h"

#include <omp.h>

bool VGAVisualGlobalOpenMP::run(Communicator *comm) {
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
            PixelRef curs = PixelRef(i, j);
            if (m_map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowPtr(AttributeKey(curs)));
            }
        }
    }

    int count = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
    }
    std::vector<DataPoint> col_data(filled.size());

#pragma omp parallel for
    for (int i = 0; i < filled.size(); i++) {

        if ((m_map.getPoint(filled[i]).contextfilled() && !filled[i].iseven()) || (m_gatesOnly)) {
            count++;
            continue;
        }
        DataPoint &dp = col_data[i];

        depthmapX::RowMatrix<int> miscs(m_map.getRows(), m_map.getCols());
        depthmapX::RowMatrix<PixelRef> extents(m_map.getRows(), m_map.getCols());

        for (int ii = 0; ii < m_map.getCols(); ii++) {
            for (int jj = 0; jj < m_map.getRows(); jj++) {
                miscs(jj, ii) = 0;
                extents(jj, ii) = PixelRef(ii, jj);
            }
        }

        int total_depth = 0;
        int total_nodes = 0;

        std::vector<int> distribution;
        std::vector<PixelRefVector> search_tree;
        search_tree.push_back(PixelRefVector());
        search_tree.back().push_back(filled[i]);

        int level = 0;
        while (search_tree[level].size()) {
            search_tree.push_back(PixelRefVector());
            distribution.push_back(0);
            for (size_t n = search_tree[level].size() - 1; n != -1; n--) {
                PixelRef curr = search_tree[level][n];
                Point &p = m_map.getPoint(curr);
                int &p1misc = miscs(curr.y, curr.x);
                if (p.filled() && p1misc != ~0) {
                    total_depth += level;
                    total_nodes += 1;
                    distribution.back() += 1;
                    if ((int)m_radius == -1 ||
                        (level < (int)m_radius && (!p.contextfilled() || search_tree[level][n].iseven()))) {
                        extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents);
                        p1misc = ~0;
                        if (!p.getMergePixel().empty()) {
                            Point &p2 = m_map.getPoint(p.getMergePixel());
                            int &p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                            if (p2misc != ~0) {
                                extractUnseen(p2.getNode(), search_tree[level + 1], miscs, extents);
                                p2misc = ~0;
                            }
                        }
                    } else {
                        p1misc = ~0;
                    }
                }
                search_tree[level].pop_back();
            }
            level++;
        }

        // only set to single float precision after divide
        // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of Space

        dp.count = float(total_nodes); // note: total nodes includes this one;

        // ERROR !!!!!!
        if (total_nodes > 1) {
            double mean_depth = double(total_depth) / double(total_nodes - 1);
            dp.depth = float(mean_depth);
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (total_nodes > 2 && mean_depth > 1.0) {
                double ra = 2.0 * (mean_depth - 1.0) / double(total_nodes - 2);
                // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                double rra_d = ra / dvalue(total_nodes);
                double rra_p = ra / pvalue(total_nodes);
                double integ_tk = teklinteg(total_nodes, total_depth);
                dp.integ_dv = float(1.0 / rra_d);
                dp.integ_pv = float(1.0 / rra_p);

                if (total_depth - total_nodes + 1 > 1) {
                    dp.integ_tk = float(integ_tk);
                } else {
                    dp.integ_tk = -1.0f;
                }
            } else {
                dp.integ_dv = -1.0f;
                dp.integ_pv = -1.0f;
                dp.integ_tk = -1.0f;
            }
            double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
            // n.b., this distribution contains the root node itself in distribution[0]
            // -> chopped from entropy to avoid divide by zero if only one node
            for (size_t k = 1; k < distribution.size(); k++) {
                if (distribution[k] > 0) {
                    double prob = double(distribution[k]) / double(total_nodes - 1);
                    entropy -= prob * log2(prob);
                    // Formula from Turner 2001, "Depthmap"
                    factorial *= double(k + 1);
                    double q = (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                    rel_entropy += (float)prob * log2(prob / q);
                }
            }
            dp.entropy = float(entropy);
            dp.rel_entropy = float(rel_entropy);
        } else {
            dp.depth = -1.0f;
            dp.entropy = -1.0f;
            dp.rel_entropy = -1.0f;
        }
        count++; // <- increment count

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }

        // kept to achieve parity in binary comparison with old versions
        // TODO: Remove at next version of .graph file
        size_t filledIdx = size_t(filled[i].y * m_map.getCols() + filled[i].x);
        m_map.getPoint(filled[i]).m_misc = miscs(filled[i].y, filled[i].x);
        m_map.getPoint(filled[i]).m_extent = extents(filled[i].y, filled[i].x);
    }

    int entropy_col, rel_entropy_col, integ_dv_col, integ_pv_col, integ_tk_col, depth_col, count_col;

    std::string radius_text;
    if (m_radius != -1) {
        radius_text = std::string(" R") + dXstring::formatString(int(m_radius), "%d");
    }

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
    std::string entropy_col_text = std::string("Visual Entropy") + radius_text;
    std::string integ_dv_col_text = std::string("Visual Integration [HH]") + radius_text;
    std::string integ_pv_col_text = std::string("Visual Integration [P-value]") + radius_text;
    std::string integ_tk_col_text = std::string("Visual Integration [Tekl]") + radius_text;
    std::string depth_col_text = std::string("Visual Mean Depth") + radius_text;
    std::string count_col_text = std::string("Visual Node Count") + radius_text;
    std::string rel_entropy_col_text = std::string("Visual Relativised Entropy") + radius_text;

    attributes.insertOrResetColumn(integ_dv_col_text.c_str());

    attributes.insertOrResetColumn(entropy_col_text.c_str());
    attributes.insertOrResetColumn(integ_pv_col_text.c_str());
    attributes.insertOrResetColumn(integ_tk_col_text.c_str());
    attributes.insertOrResetColumn(depth_col_text.c_str());
    attributes.insertOrResetColumn(count_col_text.c_str());
    attributes.insertOrResetColumn(rel_entropy_col_text.c_str());

    integ_dv_col = attributes.getOrInsertColumn(integ_dv_col_text.c_str());

    entropy_col = attributes.getOrInsertColumn(entropy_col_text.c_str());
    integ_pv_col = attributes.getOrInsertColumn(integ_pv_col_text.c_str());
    integ_tk_col = attributes.getOrInsertColumn(integ_tk_col_text.c_str());
    depth_col = attributes.getOrInsertColumn(depth_col_text.c_str());
    count_col = attributes.getOrInsertColumn(count_col_text.c_str());
    rel_entropy_col = attributes.getOrInsertColumn(rel_entropy_col_text.c_str());

    auto dataIter = col_data.begin();
    for (auto row : rows) {
        row->setValue(integ_dv_col, dataIter->integ_dv);
        row->setValue(integ_pv_col, dataIter->integ_pv);
        row->setValue(integ_tk_col, dataIter->integ_tk);
        row->setValue(count_col, dataIter->count);
        row->setValue(depth_col, dataIter->depth);
        row->setValue(entropy_col, dataIter->entropy);
        row->setValue(rel_entropy_col, dataIter->rel_entropy);
        dataIter++;
    }

    m_map.setDisplayedAttribute(integ_dv_col);

    return true;
}

void VGAVisualGlobalOpenMP::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                                          depthmapX::RowMatrix<PixelRef> &extents) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                int &misc = miscs(pix.y, pix.x);
                PixelRef &extent = extents(pix.y, pix.x);
                if (misc == 0) {
                    pixels.push_back(pix);
                    misc |= (1 << i);
                }
                // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or horizontal
                if (!(bin.m_dir & PixelRef::DIAGONAL)) {
                    if (extent.col(bin.m_dir) >= pixVec.end().col(bin.m_dir))
                        break;
                    extent.col(bin.m_dir) = pixVec.end().col(bin.m_dir);
                }
                pix.move(bin.m_dir);
            }
        }
    }
}
