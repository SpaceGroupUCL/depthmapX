// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2019, Petros Koutsolampros

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

#include "vgaisovistzone.h"

#include "salalib/salaprogram.h"

#include "genlib/stringutils.h"

#include <iomanip>

bool VGAIsovistZone::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    if (m_originPointSets.empty()) {
        return false;
    }
    int zoneColumnIndex = -1;
    for (auto originPointSet : m_originPointSets) {
        std::string zoneColumnName = "Isovist Zone Distance";
        std::string inverseZoneColumnName = "Isovist Zone Inverse Square Distance";

        std::string originPointSetName = originPointSet.first;
        auto originPoints = originPointSet.second;

        if (m_originPointSets.size() > 1) {
            zoneColumnName += " [" + originPointSetName + "]";
            inverseZoneColumnName += " [" + originPointSetName + "]";
        }

        if (m_restrictDistance > 0) {

            std::stringstream restrictionText;
            restrictionText << std::fixed << std::setprecision(2) << " (" << m_restrictDistance << ")" << std::flush;

            zoneColumnName += restrictionText.str();
            inverseZoneColumnName += restrictionText.str();
        }
        zoneColumnIndex = attributes.insertOrResetColumn(zoneColumnName);

        for (PixelRef ref : originPoints) {
            AttributeRow &row = attributes.getRow(AttributeKey(ref));
            row.setValue(zoneColumnIndex, 0);
            Point &lp = m_map.getPoint(ref);
            std::set<MetricTriple> newPixels;
            extractMetric(lp.getNode(), newPixels, m_map, MetricTriple(0.0f, ref, NoPixel));
            for (auto &zonePixel : newPixels) {
                auto *zonePixelRow = attributes.getRowPtr(AttributeKey(zonePixel.pixel));
                if (zonePixelRow != 0) {
                    double zoneLineDist = dist(ref, zonePixel.pixel) * m_map.getSpacing();
                    float currZonePixelVal = zonePixelRow->getValue(zoneColumnIndex);
                    if ((currZonePixelVal == -1 || zoneLineDist < currZonePixelVal) &&
                        (m_restrictDistance <= 0 || (m_restrictDistance > 0 && zoneLineDist < m_restrictDistance))) {
                        zonePixelRow->setValue(zoneColumnIndex, zoneLineDist);
                    }
                }
            }
        }
        int inverseZoneColumnIndex = attributes.insertOrResetColumn(inverseZoneColumnName);
        setColumnFormulaAndUpdate(m_map, inverseZoneColumnIndex, "1/((value(\"" + zoneColumnName + "\") + 1) ^ 2)",
                                  false);
    }
    m_map.overrideDisplayedAttribute(-2);
    m_map.setDisplayedAttribute(zoneColumnIndex);

    return true;
}

void VGAIsovistZone::extractMetric(Node n, std::set<MetricTriple> &pixels, PointMap &map, const MetricTriple &curs) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = n.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                Point &pt = map.getPoint(pix);
                if (pt.filled()) {
                    pixels.insert(MetricTriple(0, pix, curs.pixel));
                }
                pix.move(bin.m_dir);
            }
        }
    }
}

void VGAIsovistZone::setColumnFormulaAndUpdate(PointMap &pointmap, int columnIndex, std::string formula,
                                               bool selectionOnly) {
    SalaObj program_context;
    SalaGrf graph;
    graph.map.point = &pointmap;
    program_context = SalaObj(SalaObj::S_POINTMAPOBJ, graph);

    std::istringstream stream(formula);
    SalaProgram proggy(program_context);
    if (!proggy.parse(stream)) {
        throw depthmapX::RuntimeException("There was an error parsing your formula:\n\n" +
                                          proggy.getLastErrorMessage());
    } else {
        bool programCompleted;
        if (selectionOnly) {
            programCompleted = proggy.runupdate(columnIndex, pointmap.getSelSet());
        } else {
            programCompleted = proggy.runupdate(columnIndex);
        }
        if (!programCompleted) {
            throw depthmapX::RuntimeException("There was an error parsing your formula:\n\n" +
                                              proggy.getLastErrorMessage());
        }
    }
    program_context.getTable()->getColumn(columnIndex).setFormula(formula);
}
