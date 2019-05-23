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

#include "salalib/vgamodules/vgaisovist.h"
#include "salalib/isovist.h"

#include "genlib/stringutils.h"

bool VGAIsovist::run(Communicator *comm, PointMap &map, bool simple_version) {
    map.m_hasIsovistAnalysis = false;
    // note, BSP tree plays with comm counting...
    comm->CommPostMessage(Communicator::NUM_STEPS, 2);
    comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    BSPNode bspRoot = makeBSPtree(comm, map.getDrawingFiles());

    AttributeTable &attributes = map.getAttributeTable();

    comm->CommPostMessage(Communicator::CURRENT_STEP, 2);

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }
    int count = 0;

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (map.getPoint(curs).filled()) {
                count++;
                if (map.getPoint(curs).contextfilled() && !curs.iseven()) {
                    continue;
                }
                Isovist isovist;
                isovist.makeit(&bspRoot, map.depixelate(curs), map.getRegion(), 0, 0);

                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                isovist.setData(attributes, row, simple_version);
                Node &node = map.getPoint(curs).getNode();
                std::vector<PixelRef> *occ = node.m_occlusion_bins;
                for (size_t k = 0; k < 32; k++) {
                    occ[k].clear();
                    node.bin(static_cast<int>(k)).setOccDistance(0.0f);
                }
                for (size_t k = 0; k < isovist.getOcclusionPoints().size(); k++) {
                    const PointDist &pointdist = isovist.getOcclusionPoints().at(k);
                    int bin = whichbin(pointdist.m_point - map.depixelate(curs));
                    // only occlusion bins with a certain distance recorded (arbitrary scale note!)
                    if (pointdist.m_dist > 1.5) {
                        PixelRef pix = map.pixelate(pointdist.m_point);
                        if (pix != curs) {
                            occ[bin].push_back(pix);
                        }
                    }
                    node.bin(bin).setOccDistance(static_cast<float>(pointdist.m_dist));
                }
            }
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
    map.m_hasIsovistAnalysis = true;

    return true;
}

BSPNode VGAIsovist::makeBSPtree(Communicator *communicator, const std::vector<SpacePixelFile>& drawingFiles) {
    std::vector<TaggedLine> partitionlines;
    for (const auto &pixelGroup : drawingFiles) {
        for (const auto &pixel : pixelGroup.m_spacePixels) {
            // chooses the first editable layer it can find:
            if (pixel.isShown()) {
                auto refShapes = pixel.getAllShapes();
                int k = -1;
                for (const auto &refShape : refShapes) {
                    k++;
                    std::vector<Line> newLines = refShape.second.getAsLines();
                    // I'm not sure what the tagging was meant for any more,
                    // tagging at the moment tags the *polygon* it was original attached to
                    // must check it is not a zero length line:
                    for (const Line &line : newLines) {
                        if (line.length() > 0.0) {
                            partitionlines.push_back(TaggedLine(line, k));
                        }
                    }
                }
            }
        }
    }

    BSPNode bspRoot;
    if (partitionlines.size()) {

        time_t atime = 0;
        communicator->CommPostMessage(Communicator::NUM_RECORDS, static_cast<int>(partitionlines.size()));
        qtimer(atime, 0);

        BSPTree::make(communicator, atime, partitionlines, &bspRoot);
    }

    return bspRoot;
}
