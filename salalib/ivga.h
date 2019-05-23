// Copyright (C) 2018 Petros Koutsolampros

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

// Interface to handle different kinds of VGA analysis

#include "salalib/mgraph.h"
#include "salalib/pointdata.h"

#include "genlib/comm.h"

#include <string>

class IVGA {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual bool run(Communicator *comm, PointMap &map, bool simple_version) = 0;
    virtual ~IVGA() {}
};
