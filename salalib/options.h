// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

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

// Options for mean depth calculations
struct Options
{
   enum output_t { OUTPUT_ISOVIST, OUTPUT_VISUAL, OUTPUT_METRIC, OUTPUT_ANGULAR, OUTPUT_THRU_VISION, OUTPUT_CLIQUE_GRAPH,
      OUTPUT_KERNEL_GRAPH, OUTPUT_MATRIX_REDUCTION };
   // Output type, see above
   int output_type;
   // Options for the summary type:
   int local;
   int global;
   int cliques;
   //
   bool choice;
   // include measures that can be derived: RA, RRA and total depth
   bool fulloutput;
   //
   enum { RADIUS_STEPS, RADIUS_METRIC, RADIUS_ANGULAR };
   int radius_type;
   double radius; // <- n.b. for metric integ radius is floating point
   // radius has to go up to a list (phase out radius as is)
   pvecdouble radius_list;
   //
   int point_depth_selection;
   int tulip_bins;
   bool process_in_memory;
   bool sel_only;
   bool gates_only;
   // for pushing to a gates layer
   int gatelayer;
   // a column to weight measures by:
   int weighted_measure_col;
   int weighted_measure_col2;  //EFEF
    int routeweight_col;			//EFEF
   std::string output_file; // To save an output graph (for example)
   // default values
   Options()
   { local = 0; global = 1; cliques = 0;
     choice = false; fulloutput = false; point_depth_selection = 0;
     tulip_bins = 1024;
     radius = -1; radius_type = 0;
     output_type = OUTPUT_ISOVIST; process_in_memory = false; gates_only = false; sel_only = false;
     gatelayer = -1;
     weighted_measure_col = -1;}
};
