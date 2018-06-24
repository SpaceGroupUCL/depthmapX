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



// isovist.h

#ifndef __ISOVIST_H__
#define __ISOVIST_H__

#include "genlib/bsptree.h"
#include <set>

// this is very much like sparksieve:

struct IsoSeg
{
   mutable bool tagdelete;
   double startangle;
   double endangle;
   Point2f startpoint;
   Point2f endpoint;
   char quadrant;
   int tag;
   IsoSeg(double start = 0.0, double end = 0.0, char q = 0, int t = -1) 
   { startangle = start; endangle = end; tagdelete = false; quadrant = q; tag = t; }
   IsoSeg(double start, double end, const Point2f& pstart, Point2f& pend, int t = -1) 
   { startangle = start; endangle = end; startpoint = pstart; endpoint = pend; tagdelete = false; tag = t; }
   friend bool operator == (const IsoSeg& b1, const IsoSeg& b2);
   friend bool operator > (const IsoSeg& b1, const IsoSeg& b2);
   friend bool operator < (const IsoSeg& b1, const IsoSeg& b2);
};
inline bool operator == (const IsoSeg& b1, const IsoSeg& b2)
{ return (b1.startangle == b2.startangle && b1.endangle == b2.endangle); }
inline bool operator > (const IsoSeg& b1, const IsoSeg& b2)
{ return b1.startangle == b2.startangle ? b1.endangle > b2.endangle : b1.startangle > b2.startangle; }
inline bool operator < (const IsoSeg& b1, const IsoSeg& b2)
{ return b1.startangle == b2.startangle ? b1.endangle < b2.endangle : b1.startangle < b2.startangle; }

class AttributeTable;

struct PointDist {
   Point2f m_point;
   double m_dist;
   PointDist(const Point2f& p = Point2f(), double d = 0.0)
   { m_point = p; m_dist = d; }
};

class Isovist
{
protected:
   Point2f m_centre;
   std::set<IsoSeg> m_blocks;
   std::set<IsoSeg> m_gaps;
   std::vector<Point2f> m_poly;
   prefvec<PointDist> m_occlusion_points;
   double m_perimeter;
   double m_occluded_perimeter;
   double m_max_radial;
   double m_min_radial;
public:
   Isovist() {;}
   const std::vector<Point2f>& getPolygon() const { return m_poly; }
   const prefvec<PointDist>& getOcclusionPoints() const { return m_occlusion_points; }
   const Point2f& getCentre() const { return m_centre; }
   //
   void makeit(BSPNode *root, const Point2f& p, const QtRegion& region, double startangle = 0.0, double endangle = 0.0);
   void make(BSPNode *here);
   void drawnode(const Line& li, int tag);
   void addBlock(const Line& li, int tag, double startangle, double endangle);
   void setData(AttributeTable& table, int row, bool simple_version);
   //
   int getClosestLine(BSPNode *root, const Point2f& p);
};

#endif
