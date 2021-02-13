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

#include "extractlinkdata.h"

#include "genlib/stringutils.h"

bool ExtractLinkData::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    int angular_cost_col = attributes.insertOrResetColumn("Link Angular Cost");
    int metric_cost_col = attributes.insertOrResetColumn("Link Metric Cost");
    int link_to_col = attributes.insertOrResetColumn("Link To");
    int visual_cost_col = attributes.insertOrResetColumn("Link Visual Cost");

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        Point &p = m_map.getPoint(pix);
        PixelRef mergePixel = p.getMergePixel();
        if (!mergePixel.empty()) {
            row.getRow().setValue(link_to_col, static_cast<int>(mergePixel));
            row.getRow().setValue(visual_cost_col, 1);
            row.getRow().setValue(metric_cost_col, dist(pix, mergePixel) * m_map.getSpacing());
            row.getRow().setValue(angular_cost_col, 1);
        }
    }

    return true;
}
