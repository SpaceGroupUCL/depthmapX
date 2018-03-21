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



#include <math.h>
#include <float.h>
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/comm.h>  // For communicator

#include <salalib/mgraph.h> // purely for the version info --- as phased out should replace
#include <salalib/axialmap.h>

#include "genlib/stringutils.h"
#include "topomet.h"

bool ShapeGraph::analyseTopoMet(Communicator *comm, int analysis_type, double radius, bool sel_only)
{
   bool retvar = true;

   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif   

   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, (sel_only ? m_selection_set.size() : m_connectors.size()) );
   }
   int reccount = 0;

   // record axial line refs for topological analysis
   pvecint axialrefs;
   // quick through to find the longest seg length
   pvecfloat seglengths;
   float maxseglength = 0.0f;
   for (size_t cursor = 0; cursor < getShapeCount(); cursor++)
   {
      axialrefs.push_back(m_attributes.getValue(cursor,"Axial Line Ref"));
      seglengths.push_back(m_attributes.getValue(cursor,"Segment Length"));
      if (seglengths.tail() > maxseglength) {
         maxseglength = seglengths.tail();
      }
   }

   std::string prefix, suffix;
   int maxbin;
   if (analysis_type == TOPOMET_METHOD_METRIC) {
      prefix = "Metric ";
      maxbin = 512;
   }
   else {
      prefix = "Topological ";
      maxbin = 2;
   }
   if (radius != -1.0) {
      suffix = dXstring::formatString(radius," R%.f metric");
   }
   std::string choicecol = prefix + "Choice" + suffix;
   std::string wchoicecol = prefix + "Choice [SLW]" + suffix;
   std::string meandepthcol = prefix + "Mean Depth" + suffix;
   std::string wmeandepthcol = prefix + std::string("Mean Depth [SLW]") + suffix;
   std::string totaldcol = prefix + "Total Depth" + suffix;
   std::string totalcol = prefix + "Total Nodes" + suffix;
   std::string wtotalcol = prefix + "Total Length" + suffix;
   //
   if (!sel_only) {
      m_attributes.insertColumn(choicecol.c_str());
      m_attributes.insertColumn(wchoicecol.c_str());
   }
   m_attributes.insertColumn(meandepthcol.c_str());
   m_attributes.insertColumn(wmeandepthcol.c_str());
   m_attributes.insertColumn(totaldcol.c_str());
   m_attributes.insertColumn(totalcol.c_str());
   m_attributes.insertColumn(wtotalcol.c_str());
   //
   unsigned int *seen = new unsigned int[getShapeCount()];
   TopoMetSegmentRef *audittrail = new TopoMetSegmentRef[getShapeCount()];
   TopoMetSegmentChoice *choicevals = new TopoMetSegmentChoice[getShapeCount()];
   for (size_t cursor = 0; cursor < getShapeCount(); cursor++)
   {
      if (sel_only && !m_attributes.isSelected(cursor)) {
         continue;
      }
      for (size_t i = 0; i < getShapeCount(); i++) {
         seen[i] = 0xffffffff;
      }
      pvecint list[512]; // 512 bins!
      int bin = 0;
      list[bin].push_back(cursor);
      double rootseglength = seglengths[cursor];
      audittrail[cursor] = TopoMetSegmentRef(cursor,Connector::SEG_CONN_ALL,rootseglength*0.5,-1);
      int open = 1;
      unsigned int segdepth = 0;
      double metdepth = 0.0, total = 0.0, wtotal = 0.0, wtotaldepth = 0.0, totalsegdepth = 0.0, totalmetdepth = 0.0;
      while (open != 0) {
         while (list[bin].size() == 0) {
            bin++;
            segdepth += 1;
            if (bin == maxbin) {
               bin = 0;
            }
         }
         //
         TopoMetSegmentRef& here = audittrail[list[bin].tail()];
         list[bin].pop_back();
         open--;
         //
         if (here.done) {
            continue;
         }
         else {
            here.done = true;
         }
         //
         double len = seglengths[here.ref];
         totalsegdepth += segdepth;
         totalmetdepth += here.dist - len*0.5; // preloaded with length ahead
         wtotal += len;
         if (analysis_type == TOPOMET_METHOD_METRIC) {
            wtotaldepth += len * (here.dist - len*0.5);
         }
         else {
            wtotaldepth += len * segdepth;
         }
         total += 1;
         //
         Connector& axline = m_connectors.at(here.ref);
         axline.first();
         int connected_cursor = -1;
         connected_cursor = axline.cursor(here.dir);
         while (connected_cursor != -1) {
            if (seen[connected_cursor] > segdepth && connected_cursor != cursor) {
               bool seenalready = (seen[connected_cursor] == 0xffffffff) ? false : true;
               float length = seglengths[connected_cursor];
               int axialref = axialrefs[connected_cursor];
               audittrail[connected_cursor] = TopoMetSegmentRef(connected_cursor,here.dir,here.dist+length,here.ref);
               seen[connected_cursor] = segdepth;
               if (radius == -1 || here.dist + length < radius) {
                  // puts in a suitable bin ahead of us...
                  open++;
                  //
                  if (analysis_type == TOPOMET_METHOD_METRIC) {
                     // better to divide by 511 but have 512 bins...
                     list[(bin + int(floor(0.5+511*length/maxseglength)))%512].push_back(connected_cursor);
                  }
                  else {   // topological
                     if (axialrefs[here.ref] == axialref) {
                        list[bin].push_back(connected_cursor);
                     }
                     else {
                        list[(bin+1)%2].push_back(connected_cursor);
                        seen[connected_cursor] = segdepth + 1; // this is so if another node is connected directly to this one but is found later it is still handled -- note it can result in the connected cursor being added twice
                     }
                  }
               }
               // not sure why this is outside the radius restriction
               // (sel_only: with restricted selection set, not all lines will be labelled)
               // (seenalready: need to check that we're not doing this twice, given the seen can go twice)

               // Quick mod - TV
               if (!sel_only && connected_cursor > (int)cursor && !seenalready) { // only one way paths, saves doing this twice
                  int subcur = connected_cursor;
                  while (subcur != -1) {
                     // in this method of choice, start and end lines are included
                     choicevals[subcur].choice += 1;
                     choicevals[subcur].wchoice += (rootseglength * length);
                     subcur = audittrail[subcur].previous;
                  }
               }
            }
            axline.next();
            connected_cursor = axline.cursor(here.dir);
         }
      }
      // also put in mean depth:
      //
      if (analysis_type == TOPOMET_METHOD_METRIC) {
         m_attributes.setValue(cursor,meandepthcol.c_str(),totalmetdepth/(total-1));
         m_attributes.setValue(cursor,totaldcol.c_str(),totalmetdepth);
      }
      else {
         m_attributes.setValue(cursor,meandepthcol.c_str(),totalsegdepth/(total-1));
         m_attributes.setValue(cursor,totaldcol.c_str(),totalsegdepth);
      }
      m_attributes.setValue(cursor,wmeandepthcol.c_str(),wtotaldepth/(wtotal-rootseglength));
      m_attributes.setValue(cursor,totalcol.c_str(),total);
      m_attributes.setValue(cursor,wtotalcol.c_str(),wtotal);
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               delete [] seen;
               delete [] audittrail;
               delete [] choicevals;
               throw Communicator::CancelledException();
            }
         }
         comm->CommPostMessage( Communicator::CURRENT_RECORD, reccount );
      }
      reccount++;
   }
   if (!sel_only) {
      // note, I've stopped sel only from calculating choice values:
      for (size_t cursor = 0; cursor < getShapeCount(); cursor++)
      {
         m_attributes.setValue(cursor,choicecol.c_str(),choicevals[cursor].choice);
         m_attributes.setValue(cursor,wchoicecol.c_str(),choicevals[cursor].wchoice);
      }
   }
   delete [] seen;
   delete [] audittrail;
   delete [] choicevals;

   if (!sel_only) {
      setDisplayedAttribute(m_attributes.getColumnIndex(choicecol.c_str()));
   }
   else {
      setDisplayedAttribute(m_attributes.getColumnIndex(meandepthcol.c_str()));
   }

   return retvar;
}

bool ShapeGraph::analyseTopoMetPD(Communicator *comm, int analysis_type)
{
   bool retvar = true;

   // record axial line refs for topological analysis
   pvecint axialrefs;
   // quick through to find the longest seg length
   pvecfloat seglengths;
   float maxseglength = 0.0f;
   for (size_t cursor = 0; cursor < getShapeCount(); cursor++)
   {
      axialrefs.push_back(m_attributes.getValue(cursor,"Axial Line Ref"));
      seglengths.push_back(m_attributes.getValue(cursor,"Segment Length"));
      if (seglengths.tail() > maxseglength) {
         maxseglength = seglengths.tail();
      }
   }

   int maxbin;
   std::string prefix;
   if (analysis_type == TOPOMET_METHOD_METRIC) {
      prefix = "Metric ";
      maxbin = 512;
   }
   else {
      prefix = "Topological ";
      maxbin = 2;
   }
   std::string depthcol = prefix + "Step Depth";

   m_attributes.insertColumn(depthcol.c_str());

   unsigned int *seen = new unsigned int[getShapeCount()];
   TopoMetSegmentRef *audittrail = new TopoMetSegmentRef[getShapeCount()];
   pvecint list[512]; // 512 bins!
   int open = 0;

   for (size_t i = 0; i < getShapeCount(); i++)
   {
      seen[i] = 0xffffffff;
   }
   for (auto& cursor: getSelSet())
   {
      seen[cursor] = 0;
      open++;
      double length = seglengths[cursor];
      audittrail[cursor] = TopoMetSegmentRef(cursor,Connector::SEG_CONN_ALL,length*0.5,-1);
      if (analysis_type == TOPOMET_METHOD_METRIC) {
         // better to divide by 511 but have 512 bins...
         list[(int(floor(0.5+511*length/maxseglength)))%512].push_back(cursor);
      }
      else {
         list[0].push_back(cursor);
      }
      m_attributes.setValue(cursor,depthcol.c_str(),0);
   }
   
   unsigned int segdepth = 0;
   int bin = 0;

   while (open != 0) {
      while (list[bin].size() == 0) {
         bin++;
         segdepth += 1;
         if (bin == maxbin) {
            bin = 0;
         }
      }
      //
      TopoMetSegmentRef& here = audittrail[list[bin].tail()];
      list[bin].pop_back();
      open--;
      // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
      if (here.done) {
         continue;
      }
      else {
         here.done = true;
      }
      //
      double len = seglengths[here.ref];
      Connector& axline = m_connectors.at(here.ref);
      axline.first();
      int connected_cursor = -1;
      connected_cursor = axline.cursor(here.dir);
      while (connected_cursor != -1) {
         if (seen[connected_cursor] > segdepth) {
            float length = seglengths[connected_cursor];
            int axialref = axialrefs[connected_cursor];
            seen[connected_cursor] = segdepth;
            audittrail[connected_cursor] = TopoMetSegmentRef(connected_cursor,here.dir,here.dist+length,here.ref);
            // puts in a suitable bin ahead of us...
            open++;
            //
            if (analysis_type == TOPOMET_METHOD_METRIC) {
               // better to divide by 511 but have 512 bins...
               list[(bin + int(floor(0.5+511*length/maxseglength)))%512].push_back(connected_cursor);
               m_attributes.setValue(connected_cursor,depthcol.c_str(),here.dist+length*0.5);
            }
            else {   // topological 
               if (axialrefs[here.ref] == axialref) {
                  list[bin].push_back(connected_cursor);
                  m_attributes.setValue(connected_cursor,depthcol.c_str(),segdepth);
               }
               else {
                  list[(bin+1)%2].push_back(connected_cursor);
                  seen[connected_cursor] = segdepth + 1; // this is so if another node is connected directly to this one but is found later it is still handled -- note it can result in the connected cursor being added twice
                  m_attributes.setValue(connected_cursor,depthcol.c_str(),segdepth+1);
               }
            }
         }
         axline.next();
         connected_cursor = axline.cursor(here.dir);
      }
   }

   delete [] seen;
   delete [] audittrail;

   setDisplayedAttribute(m_attributes.getColumnIndex(depthcol.c_str()));

   return retvar;
}
