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

class VGAAngularOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gates_only;

    struct DataPoint {
        float total_depth, mean_depth, count;
    };

    void extractAngular(Node &node, std::set<AngularTriple> &pixels, PointMap *pointdata, const AngularTriple &curs,
                        depthmapX::RowMatrix<int> &miscs, depthmapX::RowMatrix<float> &cumangles);

  public:
    VGAAngularOpenMP(PointMap &map, double radius, bool gates_only)
        : m_map(map), m_radius(radius), m_gates_only(gates_only) {}
    std::string getAnalysisName() const override { return "Angular Analysis (OpenMP)"; }
    bool run(Communicator *comm) override;
};
