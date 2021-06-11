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

#include "vgathroughvision.h"
#include "salalib/agents/agenthelpers.h"

#include "genlib/stringutils.h"

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

bool VGAThroughVision::run(Communicator *comm) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    AttributeTable &attributes = m_map.getAttributeTable();

    // current version (not sure of differences!)
    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            m_map.getPoint(curs).m_misc = 0;
        }
    }

    bool hasGateColumn = m_map.getAttributeTable().hasColumn(g_col_gate);

    int count = 0;
    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            std::vector<int> seengates;
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            Point &p = m_map.getPoint(curs);
            if (m_map.getPoint(curs).filled()) {
                p.getNode().first();
                while (!p.getNode().is_tail()) {
                    PixelRef x = p.getNode().cursor();
                    PixelRefVector pixels = m_map.quickPixelateLine(x, curs);
                    for (size_t k = 1; k < pixels.size() - 1; k++) {
                        PixelRef key = pixels[k];
                        m_map.getPoint(key).m_misc += 1;

                        // TODO: Undocumented functionality. Shows how many times a gate is passed?
                        if (hasGateColumn) {
                            auto iter = attributes.find(AttributeKey(key));
                            if (iter != attributes.end()) {
                                int gate = static_cast<int>(iter->getRow().getValue(g_col_gate));
                                if (gate != -1) {
                                    auto gateIter = depthmapX::findBinary(seengates, gate);
                                    if (gateIter == seengates.end()) {
                                        iter->getRow().incrValue(g_col_gate_counts);
                                        seengates.insert(gateIter, int(gate));
                                    }
                                }
                            }
                        }
                    }
                    p.getNode().next();
                }
                // only increment count for actual filled points
                count++;
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

    int col = attributes.getOrInsertColumn("Through vision");

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        iter->getRow().setValue(col, static_cast<float>(m_map.getPoint(pix).m_misc));
        m_map.getPoint(pix).m_misc = 0;
    }

    m_map.overrideDisplayedAttribute(-2);
    m_map.setDisplayedAttribute(col);

    return true;
}
