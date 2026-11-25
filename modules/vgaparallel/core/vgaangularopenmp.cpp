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

#include "vgaangularopenmp.h"

#include "genlib/stringutils.h"

#include <omp.h>

bool VGAAngularOpenMP::run(Communicator *comm) {
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

    std::vector<DataPoint> col_data(filled.size());

    int i, N = int(filled.size());
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
    for (i = 0; i < N; i++) {
        if (m_gates_only) {
            count++;
            continue;
        }

        DataPoint &dp = col_data[i];

        depthmapX::RowMatrix<int> miscs(m_map.getRows(), m_map.getCols());
        depthmapX::RowMatrix<float> cumangles(m_map.getRows(), m_map.getCols());

        miscs.initialiseValues(0);
        cumangles.initialiseValues(-1.0f);

        float total_angle = 0.0f;
        int total_nodes = 0;

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only

        std::set<AngularTriple> search_list;
        search_list.insert(AngularTriple(0.0f, filled[size_t(i)], NoPixel));
        cumangles(filled[size_t(i)].y, filled[size_t(i)].x) = 0.0f;

        while (search_list.size()) {
            std::set<AngularTriple>::iterator it = search_list.begin();
            AngularTriple here = *it;
            search_list.erase(it);
            if (int(m_radius) != -1 && double(here.angle) > m_radius) {
                break;
            }
            Point &p = m_map.getPoint(here.pixel);
            int &p1misc = miscs(here.pixel.y, here.pixel.x);
            float &p1cumangle = cumangles(here.pixel.y, here.pixel.x);
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && p1misc != ~0) {
                extractAngular(p.getNode(), search_list, &m_map, here, miscs, cumangles);
                p1misc = ~0;
                if (!p.getMergePixel().empty()) {
                    Point &p2 = m_map.getPoint(p.getMergePixel());
                    int &p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                    float &p2cumangle = cumangles(p.getMergePixel().y, p.getMergePixel().x);
                    if (p2misc != ~0) {
                        p2cumangle = p1cumangle;
                        extractAngular(p2.getNode(), search_list, &m_map,
                                       AngularTriple(here.angle, p.getMergePixel(), NoPixel), miscs, cumangles);
                        p2misc = ~0;
                    }
                }
                total_angle += p1cumangle;
                total_nodes += 1;
            }
        }

        if (total_nodes > 0) {
            dp.mean_depth = float(double(total_angle) / double(total_nodes));
        }
        dp.total_depth = total_angle;
        dp.count = float(total_nodes);

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
        m_map.getPoint(filled[size_t(i)]).m_misc = miscs(filled[size_t(i)].y, filled[size_t(i)].x);
        m_map.getPoint(filled[size_t(i)]).m_cumangle = cumangles(filled[size_t(i)].y, filled[size_t(i)].x);
    }

    std::string radius_text;
    if (int(m_radius) != -1) {
        if (m_map.getRegion().width() > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.f");
        } else if (m_map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.2f");
        }
    }
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mean_depth_col_text = std::string("Angular Mean Depth") + radius_text;
    int mean_depth_col = attributes.getOrInsertColumn(mean_depth_col_text.c_str());
    std::string total_detph_col_text = std::string("Angular Total Depth") + radius_text;
    int total_depth_col = attributes.getOrInsertColumn(total_detph_col_text.c_str());
    std::string count_col_text = std::string("Angular Node Count") + radius_text;
    int count_col = attributes.getOrInsertColumn(count_col_text.c_str());

    auto dataIter = col_data.begin();
    for (auto row : rows) {
        row->setValue(total_depth_col, dataIter->mean_depth);
        row->setValue(total_depth_col, dataIter->total_depth);
        row->setValue(count_col, dataIter->count);
        dataIter++;
    }

    m_map.overrideDisplayedAttribute(-2);
    m_map.setDisplayedAttribute(mean_depth_col);

    return true;
}

void VGAAngularOpenMP::extractAngular(Node &node, std::set<AngularTriple> &pixels, PointMap *pointdata,
                                      const AngularTriple &curs, depthmapX::RowMatrix<int> &miscs,
                                      depthmapX::RowMatrix<float> &cumangles) {
    if (curs.angle == 0.0f || pointdata->getPoint(curs.pixel).blocked() || pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = node.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start(); pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    if (miscs(pix.y, pix.x) == 0) {
                        // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                        float ang = (curs.lastpixel == NoPixel)
                                        ? 0.0f
                                        : (float)(angle(pix, curs.pixel, curs.lastpixel) / (M_PI * 0.5));
                        float &cumangle = cumangles(pix.y, pix.x);
                        if (cumangle == -1.0 || curs.angle + ang < cumangle) {
                            cumangle = cumangles(curs.pixel.y, curs.pixel.x) + ang;
                            pixels.insert(AngularTriple(cumangle, pix, curs.pixel));
                        }
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
