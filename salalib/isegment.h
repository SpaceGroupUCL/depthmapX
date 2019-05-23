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

#include "salalib/axialmap.h"
#include "salalib/options.h"

#include "genlib/comm.h"

#include <string>

class ISegment
{
public:
    virtual std::string getAnalysisName() const = 0;
    virtual bool run(Communicator *comm, ShapeGraph &map, bool simple_version) = 0;
    virtual ~ISegment(){}

    // Axial map helper: convert a radius for angular analysis

    static std::string makeFloatRadiusText(double radius)
    {
       std::string radius_text;
       if (radius > 100.0) {
          radius_text = dXstring::formatString(radius,"%.f");
       }
       else if (radius < 0.1) {
          radius_text = dXstring::formatString(radius,"%.4f");
       }
       else {
          radius_text = dXstring::formatString(radius,"%.2f");
       }
       return radius_text;
    }

    static std::string makeRadiusText(int radius_type, double radius)
    {
       std::string radius_text;
       if (radius != -1) {
          if (radius_type == Options::RADIUS_STEPS) {
             radius_text = std::string(" R") + dXstring::formatString(int(radius),"%d") + " step";
          }
          else if (radius_type == Options::RADIUS_METRIC) {
             radius_text = std::string(" R") + makeFloatRadiusText(radius) + " metric";
          }
          else { // radius angular
             radius_text = std::string(" R") + makeFloatRadiusText(radius);
          }
       }
       return radius_text;
    }
};
