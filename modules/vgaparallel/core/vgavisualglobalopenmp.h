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

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/options.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

#include "genlib/simplematrix.h"

class VGAVisualGlobalOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gatesOnly;

    struct DataPoint {
        float count, depth, integ_dv, integ_pv;
        float integ_tk, entropy, rel_entropy;
    };
    void extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<PixelRef> &extents);

  public:
    VGAVisualGlobalOpenMP(PointMap &map, double radius, bool gatesOnly)
        : m_map(map), m_radius(radius), m_gatesOnly(gatesOnly) {}
    std::string getAnalysisName() const override { return "Global Visibility Analysis (OpenMP)"; }
    bool run(Communicator *) override;
};
