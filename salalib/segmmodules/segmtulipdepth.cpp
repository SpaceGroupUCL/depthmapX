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

#include "salalib/segmmodules/segmtulipdepth.h"

#include "genlib/stringutils.h"

// revised to use tulip bins for faster analysis of large spaces

bool SegmentTulipDepth::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    std::string stepdepth_col_text = "Angular Step Depth";
    int stepdepth_col = attributes.insertOrResetColumn(stepdepth_col_text.c_str());

    // The original code set tulip_bins to 1024, divided by two and added one
    // in order to duplicate previous code (using a semicircle of tulip bins)
    size_t tulip_bins = 513;

    std::vector<bool> covered(map.getConnections().size());
    for (size_t i = 0; i < map.getConnections().size(); i++) {
       covered[i] = false;
    }
    std::vector<std::vector<SegmentData> > bins(tulip_bins);

    int opencount = 0;
    for (auto& sel: map.getSelSet()) {
       int row = depthmapX::getMapAtIndex(map.getAllShapes(), sel)->first;
       if (row != -1) {
          bins[0].push_back(SegmentData(0,row,SegmentRef(),0,0.0,0));
          opencount++;
       }
    }
    int depthlevel = 0;
    auto binIter = bins.begin();
    int currentbin = 0;
    while (opencount) {
       while (binIter->empty()) {
          depthlevel++;
          binIter++;
          currentbin++;
          if (binIter == bins.end()) {
             binIter = bins.begin();
          }
       }
       SegmentData lineindex;
       if (binIter->size() > 1) {
          // it is slightly slower to delete from an arbitrary place in the bin,
          // but it is necessary to use random paths to even out the number of times through equal paths
          int curr = pafrand() % binIter->size();
          auto currIter = binIter->begin() + curr;
          lineindex = *currIter;
          binIter->erase(currIter);
          // note: do not clear choice values here!
       }
       else {
          lineindex = binIter->front();
          binIter->pop_back();
       }
       opencount--;
       if (!covered[lineindex.ref]) {
          covered[lineindex.ref] = true;
          Connector& line = map.getConnections()[lineindex.ref];
          // convert depth from tulip_bins normalised to standard angle
          // (note the -1)
          double depth_to_line = depthlevel / ((tulip_bins - 1) * 0.5);
          map.getAttributeRowFromShapeIndex(lineindex.ref).setValue(stepdepth_col,depth_to_line);
          int extradepth;
          if (lineindex.dir != -1) {
             for (auto& segconn: line.m_forward_segconns) {
                if (!covered[segconn.first.ref]) {
                   extradepth = (int) floor(segconn.second * tulip_bins * 0.5);
                   bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                       SegmentData(segconn.first,lineindex.ref,lineindex.segdepth+1,0.0,0));
                   opencount++;
                }
             }
          }
          if (lineindex.dir != 1) {
             for (auto& segconn: line.m_back_segconns) {
                if (!covered[segconn.first.ref]) {
                   extradepth = (int) floor(segconn.second * tulip_bins * 0.5);
                   bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                       SegmentData(segconn.first,lineindex.ref,lineindex.segdepth+1,0.0,0));
                   opencount++;
                 }
             }
          }
       }
    }

    map.setDisplayedAttribute(-2); // <- override if it's already showing
    map.setDisplayedAttribute(stepdepth_col);

    return true;
}
