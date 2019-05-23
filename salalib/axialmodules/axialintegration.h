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

#include "salalib/iaxial.h"

class AxialIntegration : IAxial {
  private:
    std::set<double> m_radius_set;
    int m_weighted_measure_col;
    bool m_choice;
    bool m_fulloutput;
    bool m_local;

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    bool run(Communicator *, ShapeGraph &map, bool) override;
    AxialIntegration(std::set<double> radius_set, int weighted_measure_col, bool choice, bool fulloutput, bool local)
        : m_radius_set(radius_set), m_weighted_measure_col(weighted_measure_col), m_choice(choice),
          m_fulloutput(fulloutput), m_local(local) {}
};
