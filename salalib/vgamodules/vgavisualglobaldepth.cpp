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

#include "salalib/vgamodules/vgavisualglobaldepth.h"

#include "genlib/stringutils.h"

bool VGAVisualGlobalDepth::run(Communicator *, PointMap &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    // n.b., insert columns sets values to -1 if the column already exists
    int col = attributes.insertOrResetColumn("Visual Step Depth");

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_extent = pix;
    }

    std::vector<PixelRefVector> search_tree;
    search_tree.push_back(PixelRefVector());
    for (auto &sel : map.getSelSet()) {
        // need to convert from ints (m_selection_set) to pixelrefs for this op:
        search_tree.back().push_back(sel);
    }

    size_t level = 0;
    while (search_tree[level].size()) {
        search_tree.push_back(PixelRefVector());
        const PixelRefVector& searchTreeAtLevel = search_tree[level];
        for (auto currLvlIter = searchTreeAtLevel.rbegin(); currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
            Point &p = map.getPoint(*currLvlIter);
            if (p.filled() && p.m_misc != ~0) {
                AttributeRow &row = attributes.getRow(AttributeKey(*currLvlIter));
                row.setValue(col, float(level));
                if (!p.contextfilled() || currLvlIter->iseven() || level == 0) {
                    p.getNode().extractUnseen(search_tree[level + 1], &map);
                    p.m_misc = ~0;
                    if (!p.getMergePixel().empty()) {
                        Point &p2 = map.getPoint(p.getMergePixel());
                        if (p2.m_misc != ~0) {
                            AttributeRow &mergePixelRow = attributes.getRow(AttributeKey(p.getMergePixel()));
                            mergePixelRow.setValue(col, float(level));
                            p2.getNode().extractUnseen(search_tree[level + 1], &map); // did say p.misc
                            p2.m_misc = ~0;
                        }
                    }
                } else {
                    p.m_misc = ~0;
                }
            }
        }
        level++;
    }

    // force redisplay:
    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(col);

    return true;
}

void VGAVisualGlobalDepth::extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
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
