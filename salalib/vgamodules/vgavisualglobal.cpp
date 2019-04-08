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

#include "salalib/vgamodules/vgavisualglobal.h"

#include "genlib/stringutils.h"

bool VGAVisualGlobal::run(Communicator *comm, const Options &options, PointMap &map, bool simple_version) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }
    AttributeTable &attributes = map.getAttributeTable();

    int entropy_col, rel_entropy_col, integ_dv_col, integ_pv_col, integ_tk_col, depth_col, count_col;
    std::string radius_text;
    if (options.radius != -1) {
        radius_text = std::string(" R") + dXstring::formatString(int(options.radius), "%d");
    }

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
#ifndef _COMPILE_dX_SIMPLE_VERSION
    if (!simple_version) {
        std::string entropy_col_text = std::string("Visual Entropy") + radius_text;
        entropy_col = attributes.insertOrResetColumn(entropy_col_text.c_str());
    }
#endif

    std::string integ_dv_col_text = std::string("Visual Integration [HH]") + radius_text;
    integ_dv_col = attributes.insertOrResetColumn(integ_dv_col_text.c_str());

#ifndef _COMPILE_dX_SIMPLE_VERSION
    if (!simple_version) {
        std::string integ_pv_col_text = std::string("Visual Integration [P-value]") + radius_text;
        integ_pv_col = attributes.insertOrResetColumn(integ_pv_col_text.c_str());
        std::string integ_tk_col_text = std::string("Visual Integration [Tekl]") + radius_text;
        integ_tk_col = attributes.insertOrResetColumn(integ_tk_col_text.c_str());
        std::string depth_col_text = std::string("Visual Mean Depth") + radius_text;
        depth_col = attributes.insertOrResetColumn(depth_col_text.c_str());
        std::string count_col_text = std::string("Visual Node Count") + radius_text;
        count_col = attributes.insertOrResetColumn(count_col_text.c_str());
        std::string rel_entropy_col_text = std::string("Visual Relativised Entropy") + radius_text;
        rel_entropy_col = attributes.insertOrResetColumn(rel_entropy_col_text.c_str());
    }
#endif

    int count = 0;

    depthmapX::RowMatrix<int> miscs(map.getRows(), map.getCols());
    depthmapX::RowMatrix<PixelRef> extents(map.getRows(), map.getCols());

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(i, j);
            if (map.getPoint(curs).filled()) {

                if ((map.getPoint(curs).contextfilled() && !curs.iseven()) || (options.gates_only)) {
                    count++;
                    continue;
                }

                for (size_t ii = 0; ii < map.getCols(); ii++) {
                    for (size_t jj = 0; jj < map.getRows(); jj++) {
                        miscs(jj, ii) = 0;
                        extents(jj, ii) = PixelRef(ii, jj);
                    }
                }

                int total_depth = 0;
                int total_nodes = 0;

                std::vector<int> distribution;
                std::vector<PixelRefVector> search_tree;
                search_tree.push_back(PixelRefVector());
                search_tree.back().push_back(curs);

                int level = 0;
                while (search_tree[level].size()) {
                    search_tree.push_back(PixelRefVector());
                    const PixelRefVector& searchTreeAtLevel = search_tree[level];
                    distribution.push_back(0);
                    for (auto currLvlIter = searchTreeAtLevel.rbegin(); currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                        int &pmisc = miscs(currLvlIter->y, currLvlIter->x);
                        Point &p = map.getPoint(*currLvlIter);
                        if (p.filled() && pmisc != ~0) {
                            total_depth += level;
                            total_nodes += 1;
                            distribution.back() += 1;
                            if ((int)options.radius == -1 ||
                                (level < (int)options.radius &&
                                    (!p.contextfilled() || currLvlIter->iseven()))) {
                                extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents);
                                pmisc = ~0;
                                if (!p.getMergePixel().empty()) {
                                    PixelRef mergePixel = p.getMergePixel();
                                    int &p2misc = miscs(mergePixel.y, mergePixel.x);
                                    Point &p2 = map.getPoint(mergePixel);
                                    if (p2misc != ~0) {
                                        extractUnseen(p2.getNode(), search_tree[level + 1], miscs,
                                                      extents); // did say p.misc
                                        p2misc = ~0;
                                    }
                                }
                            } else {
                                pmisc = ~0;
                            }
                        }
                        search_tree[level].pop_back();
                    }
                    level++;
                }
                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                // only set to single float precision after divide
                // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of Space
                if (!simple_version) {
                    row.setValue(count_col, float(total_nodes)); // note: total nodes includes this one
                }
                // ERROR !!!!!!
                if (total_nodes > 1) {
                    double mean_depth = double(total_depth) / double(total_nodes - 1);
                    if (!simple_version) {
                        row.setValue(depth_col, float(mean_depth));
                    }
                    // total nodes > 2 to avoid divide by 0 (was > 3)
                    if (total_nodes > 2 && mean_depth > 1.0) {
                        double ra = 2.0 * (mean_depth - 1.0) / double(total_nodes - 2);
                        // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                        double rra_d = ra / dvalue(total_nodes);
                        double rra_p = ra / pvalue(total_nodes);
                        double integ_tk = teklinteg(total_nodes, total_depth);
                        row.setValue(integ_dv_col, float(1.0 / rra_d));
                        if (!simple_version) {
                            row.setValue(integ_pv_col, float(1.0 / rra_p));
                        }
                        if (total_depth - total_nodes + 1 > 1) {
                            if (!simple_version) {
                                row.setValue(integ_tk_col, float(integ_tk));
                            }
                        } else {
                            if (!simple_version) {
                                row.setValue(integ_tk_col, -1.0f);
                            }
                        }
                    } else {
                        row.setValue(integ_dv_col, (float)-1);
                        if (!simple_version) {
                            row.setValue(integ_pv_col, (float)-1);
                            row.setValue(integ_tk_col, (float)-1);
                        }
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
                    if (!simple_version) {
                        row.setValue(entropy_col, float(entropy));
                        row.setValue(rel_entropy_col, float(rel_entropy));
                    }
                } else {
                    if (!simple_version) {
                        row.setValue(depth_col, (float)-1);
                        row.setValue(entropy_col, (float)-1);
                        row.setValue(rel_entropy_col, (float)-1);
                    }
                }
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
        }
    }
    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            map.getPoint(curs).m_misc = miscs(j, i);
            map.getPoint(curs).m_extent = extents(j, i);
        }
    }
    map.setDisplayedAttribute(integ_dv_col);

    return true;
}

void VGAVisualGlobal::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
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
