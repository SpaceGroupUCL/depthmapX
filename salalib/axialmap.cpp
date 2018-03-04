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



// This is my code to make a set of axial lines from a set of boundary lines

#include <math.h>
#include <float.h>
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/comm.h>  // For communicator

#include <salalib/mgraph.h> // purely for the version info --- as phased out should replace
#include <salalib/axialmap.h>

#include <salalib/pointdata.h>   // need the pointdata for the convert boundary graph to axial map routine
#include <salalib/ngraph.h>      // ditto ngraph
#include "MapInfoData.h"

#include "genlib/stringutils.h"
#include "genlib/containerutils.h"

#ifndef _WIN32
#define _finite finite
#endif

static const double TOLERANCE_A = 1e-9;
static const double TOLERANCE_B = 1e-12;
static const double TOLERANCE_C = 1e-6;

////////////////////////////////////////////////////////////////////////////////////////////

static int compareValueTriplet(const void *p1, const void *p2)
{
   ValueTriplet *vp1 = (ValueTriplet *) p1;
   ValueTriplet *vp2 = (ValueTriplet *) p2;
   int v = vp1->value1 - vp2->value1;
   return (vp1->value1 > vp2->value1 ? 1 : vp1->value1 < vp2->value1 ? -1 :
          (vp1->value2 > vp2->value2 ? 1 : vp1->value2 < vp2->value2 ? -1 : 0));
}

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

////////////////////////////////////////////////////////////////////////////////////////////

AxialPolygons::AxialPolygons()
{
   m_pixel_polys = NULL;
}

AxialPolygons::~AxialPolygons()
{
   if (m_pixel_polys) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_polys[i];
      }
      delete [] m_pixel_polys;
      m_pixel_polys = NULL;
   }
}

AxialVertex AxialPolygons::makeVertex(const AxialVertexKey& vertexkey, const Point2f& openspace)
{
   AxialVertex av(vertexkey, m_vertex_possibles.key(vertexkey.m_ref_key), openspace);

   // n.b., at this point, vertex key m_a and m_b are unfixed
   pqvector<Point2f>& pointlist = m_vertex_possibles.value(vertexkey.m_ref_key);
   if (pointlist.size() < 2) {
      return av;
   }

   Point2f o = av.m_point - av.m_openspace;

   // using an anglemap means that there are now no anti-clockwise vertices...
   pmap<double,int> anglemap;
   for (size_t i = 0; i < pointlist.size(); i++) {
      anglemap.add( angle(openspace,av.m_point,pointlist[i]), i );
   }

   av.m_ref_a = anglemap.head();
   av.m_ref_a = anglemap.tail();
   Point2f a = av.m_point - pointlist.at( anglemap.head() );
   Point2f b = pointlist.at( anglemap.tail() ) - av.m_point;
   av.m_a = a;
   av.m_b = b;
   a.normalise();
   b.normalise();

   double oa = det(o,a);
   double ob = det(o,b);
   double ab = det(a,b);

   // can't handle these cases
   if (fabs(oa) < TOLERANCE_A || fabs(ob) < TOLERANCE_A || fabs(ab) < TOLERANCE_A) {
      // although note that if ab == 0 and you've already checked intersection, it can't be convex
      return av;
   }

   // ADDED 4-Nov-04 -- In order to stop too many lines being generated, don't include
   // points that do not change surface direction:  -- notice: will create problems with circles 
   if (fabs(dot(a,b)) > 0.999) {
      return av;
   }


   if (sgn(oa) == sgn(ob)) {
      // headon collision
      if (sgn(oa) == 1) {
         if (sgn(ab) == 1) {
            // convex clockwise
            av.m_convex = true;
            av.m_clockwise = true;
            av.m_axial = true;
         }
         else {
            // n.b., these are turned away for axial formation
            // concave clockwise
            av.m_convex = false;
            av.m_clockwise = true;
            av.m_axial = false;
         }
      }
   }
   else {
      // glancing blow
      // concave clockwise
      av.m_convex = false;
      av.m_clockwise = true;
      av.m_axial = true;
   }

   av.m_initialised = true;

   return av;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void AxialPolygons::clear()
{
   // clear any existing data
   if (m_pixel_polys) {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_polys[i];
      }
      delete [] m_pixel_polys;
      m_pixel_polys = NULL;
   }

   m_vertex_possibles.clear();
   m_vertex_polys.clear();
   m_handled_list.clear();
}

void AxialPolygons::init(prefvec<Line>& lines, const QtRegion& region)
{
   // init pixelbase members
   m_region = region;

   // now tidy
   TidyLines tidier;
   tidier.tidy(lines, m_region);

   // for easier debugging, the axial code is reused to make segments
   ShapeGraph firstpass;
   firstpass.init(lines.size(),m_region); // used to be double density
   size_t i;
   for (i = 0; i < lines.size(); i++) {
      firstpass.makeLineShape(lines[i]);
   }
   firstpass.makeConnections();

   lines.clear();
   prefvec<Connector> connectionset;

   // interesting... 1.0 may or may not work as intended
   firstpass.makeSegmentMap(lines, connectionset, 1.0);

   // now we have a set of lines and a set of connections...
   // ...for the second pass, a bit of retro fitting to my original code is 
   // required
   makeVertexPossibles(lines, connectionset);

   initLines(lines.size(), m_region.bottom_left, m_region.top_right, 2);
   // need to init before making pixel polys...
   makePixelPolys();
   // now also add lines
   for (i = 0; i < m_vertex_possibles.size(); i++) {
      for (size_t j = 0; j < m_vertex_possibles.value(i).size(); j++) {
         addLine(Line(m_vertex_possibles.key(i),m_vertex_possibles.value(i).at(j)));
      }
   }
   sortPixelLines();
}

void AxialPolygons::makeVertexPossibles(const prefvec<Line>& lines, const prefvec<Connector>& connectionset)
{
   m_vertex_possibles.clear();
   m_vertex_polys.clear();
   int currpoly = -1;

   size_t i = 0;

   int *found[2];
   found[0] = new int [lines.size()];
   found[1] = new int [lines.size()];
   for (i = 0; i < lines.size(); i++) {
      found[0][i] = -1;
      found[1][i] = -1;
   }
   pqvector<Point2f> pointlookup;
   // three pass operation: (1) stack the lines
   for (i = 0; i < lines.size(); i++) {
      if (found[0][i] == -1) {
         pointlookup.push_back(lines[i].start());
         m_vertex_possibles.add(pointlookup.tail(),pqvector<Point2f>());
         m_vertex_polys.push_back(-1); // <- n.b., dummy entry for now, maintain with vertex possibles
         found[0][i] = pointlookup.size() - 1;
         for (size_t j = 0; j < connectionset[i].m_back_segconns.size(); j++) {
            const SegmentRef& segref = connectionset[i].m_back_segconns.key(j);
            int forwback = (segref.dir == 1) ? 0 : 1;
            found[forwback][segref.ref] = found[0][i];
         }
      }
      if (found[1][i] == -1) {
         pointlookup.push_back(lines[i].end());
         m_vertex_possibles.add(pointlookup.tail(),pqvector<Point2f>());
         m_vertex_polys.push_back(-1); // <- n.b., dummy entry for now, maintain with vertex possibles
         found[1][i] = pointlookup.size() - 1;
         for (size_t j = 0; j < connectionset[i].m_forward_segconns.size(); j++) {
            const SegmentRef& segref = connectionset[i].m_forward_segconns.key(j);
            int forwback = (segref.dir == 1) ? 0 : 1;
            found[forwback][segref.ref] = found[1][i];
         }
      }
   }
   // three pass operation: (2) connect up vertex possibles
   for (i = 0; i < lines.size(); i++) {
      if (found[0][i] == -1 || found[1][i] == -1) {
         throw 1;
      }
      size_t index0 = m_vertex_possibles.searchindex(pointlookup.at(found[0][i]));
      size_t index1 = m_vertex_possibles.searchindex(pointlookup.at(found[1][i]));
      if (index0 == paftl::npos || index1 == paftl::npos) {
         throw 2;
      }
      m_vertex_possibles.value(index0).add(pointlookup.at(found[1][i]));
      m_vertex_possibles.value(index1).add(pointlookup.at(found[0][i]));
   }
   delete [] found[0];
   delete [] found[1];
   // three pass operation: (3) create vertex poly entries
   int current_poly = -1;
   for (i = 0; i < m_vertex_possibles.size(); i++) {
      if (m_vertex_polys[i] == -1) {
         current_poly++;
         pvecint addlist;
         addlist.push_back(i);
         while (addlist.size()) {
            m_vertex_polys[addlist.tail()] = current_poly;
            pqvector<Point2f>& connections = m_vertex_possibles.value(addlist.tail());
            addlist.pop_back();
            for (size_t j = 0; j < connections.size(); j++) {
               size_t index = m_vertex_possibles.searchindex(connections[j]);
               if (index == paftl::npos) {
                  throw 3;
               }
               if (m_vertex_polys[index] == -1) {
                  addlist.push_back(index);
               }
            }
         }
      }
   }
}

void AxialPolygons::makePixelPolys()
{
   int i = 0;

   // record all of this onto the pixel polygons
   if (m_pixel_polys)
   {
      for (i = 0; i < m_cols; i++) {
         delete [] m_pixel_polys[i];
      }
      delete [] m_pixel_polys;
      m_pixel_polys = NULL;
   }
   m_pixel_polys = new pvecint *[m_cols];
   for (i = 0; i < m_cols; i++) {
      m_pixel_polys[i] = new pvecint[m_rows];
   }
   // now register the vertices in each pixel...
   for (size_t j = 0; j < m_vertex_possibles.size(); j++) {
      PixelRef pix = pixelate(m_vertex_possibles.key(j));
      m_pixel_polys[pix.x][pix.y].push_back(j);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////

// almost identical to original!

AxialVertexKey AxialPolygons::seedVertex(const Point2f& seed)
{
   AxialVertexKey seedvertex = NoVertex;
   PixelRef seedref = pixelate(seed);
   bool foundvertex = false;
   // for spiralling outwards to find a vertex:
   int dir = PixelRef::HORIZONTAL;
   int sidelength = 1;
   int runlength = 0;
   int allboundaries = 0;

   while (!foundvertex) {
      for (size_t i = 0; i < m_pixel_polys[seedref.x][seedref.y].size(); i++) {
         int vertexref = m_pixel_polys[seedref.x][seedref.y][i];
         const Point2f& trialpoint = m_vertex_possibles.key(vertexref);
         if (!intersect_exclude(Line(seed,trialpoint))) {
            // yay... ...but wait... we need to see if it's a proper polygon vertex first...
            seedvertex = vertexref;
            foundvertex = true;
         }
      }
      if (!foundvertex) {
         seedref = seedref.move(dir);
         // spiral outwards:
         if (++runlength == sidelength) {
            switch (dir) {
            case PixelRef::HORIZONTAL:
               dir = PixelRef::VERTICAL; runlength = 0;
               break;
            case PixelRef::VERTICAL:
               dir = PixelRef::NEGHORIZONTAL; runlength = 0; sidelength++;
               break;
            case PixelRef::NEGHORIZONTAL:
               dir = PixelRef::NEGVERTICAL; runlength = 0;
               break;
            case PixelRef::NEGVERTICAL:
               dir = PixelRef::HORIZONTAL; runlength = 0; sidelength++;
               break;
            }
         }
         // check to make sure not off edge of system:
         if (seedref.x < 0) {
            allboundaries |= 0x01; seedref.x = 0;
         }
         if (seedref.y < 0) {
            allboundaries |= 0x02; seedref.y = 0;
         }
         if (seedref.x >= m_cols) {
            allboundaries |= 0x04; seedref.x = m_cols - 1;
         }
         if (seedref.y >= m_rows) {
            allboundaries |= 0x08; seedref.y = m_rows - 1;
         }
         if (allboundaries == 0x0f) {
            return NoVertex; 
         }
      }
   }
   return seedvertex;
}

// adds any axial lines from this point to the list of lines, adds any unhandled visible vertices it finds to the openvertices list
// axial lines themselves are added to the lines list - the axial line is only there to record the key vertices that comprise the line
void AxialPolygons::makeAxialLines(pqvector<AxialVertex>& openvertices, prefvec<Line>& lines, prefvec<pvecint>& keyvertices, prefvec<PolyConnector>& poly_connections, pqvector<RadialLine>& radial_lines)
{
   AxialVertex vertex = openvertices.tail();
   openvertices.pop_back();

   m_handled_list.add(vertex);

   for (size_t i = 0; i < m_vertex_possibles.size(); i++) {
      if (i == vertex.m_ref_key) {
         continue;
      }
      bool possible = false, stubpossible = false;
      Point2f p = m_vertex_possibles.key(i) - vertex.m_point;
      if (vertex.m_convex) {
         if (det(vertex.m_a,p) > 0 && det(vertex.m_b,p) > 0) {
            possible = true;
         }
      }
      else {
         // left of b and right of a or left of a and right of b
         if (det(p,vertex.m_a) * det(p,vertex.m_b) < 0) {
            possible = true;
         }
         else if (det(p,vertex.m_a) < TOLERANCE_A && det(p,vertex.m_b) < TOLERANCE_A) {
            stubpossible = true;
         }
      }
      if (possible || stubpossible) {
         Line line(m_vertex_possibles.key(i),vertex.m_point);
         if (!intersect_exclude(line)) {
            AxialVertex next_vertex = makeVertex(AxialVertexKey(i),vertex.m_point);
            if (next_vertex.m_initialised && m_handled_list.searchindex(next_vertex) == paftl::npos) {
               openvertices.add(next_vertex); // <- note, add ignores duplicate adds (each vertex tends to be added multiple times before this vertex is handled itself)
               bool shortline_segend = false;
               Line shortline = line;
               if (!vertex.m_convex && possible) {
                  Line ext(line.t_end(), line.t_end() + (line.t_end() - line.t_start()));
                  ext.ray(1, m_region);
                  cutLine(ext, 1);
                  line = Line(line.t_start(), ext.t_end());
                  // for radial line segend calc:
                  if (det(-p,vertex.m_b) < 0) {
                     shortline_segend = true;
                  }
               }
               if (m_vertex_polys[vertex.m_ref_key] != m_vertex_polys[next_vertex.m_ref_key]) { // must be on separate polygons
                  // radial line(s) (for new point)
                  RadialLine radialshort(next_vertex, shortline_segend, vertex.m_point,next_vertex.m_point,next_vertex.m_point+next_vertex.m_b);
                  poly_connections.push_back( PolyConnector(shortline, (RadialKey)radialshort) );
                  radial_lines.add(radialshort);
                  if (!vertex.m_convex && possible) {
                     Line longline = Line(m_vertex_possibles.key(i),line.t_end());
                     RadialLine radiallong(radialshort);
                     radiallong.segend = shortline_segend ? 0 : 1;
                     poly_connections.push_back( PolyConnector(longline, (RadialKey)radiallong) );
                     radial_lines.add(radiallong);
                  }
               }
               shortline_segend = false;
               if (!next_vertex.m_convex && next_vertex.m_axial) {
                  Line ext(line.t_start() - (line.t_end() - line.t_start()), line.t_start());
                  ext.ray(0, m_region);
                  cutLine(ext, 0);
                  line = Line(ext.t_start(), line.t_end());
                  // for radial line segend calc:
                  if (det(p,next_vertex.m_b) < 0) {
                     shortline_segend = true;
                  }
               }
               if (m_vertex_polys[vertex.m_ref_key] != m_vertex_polys[next_vertex.m_ref_key]) { // must be on separate polygons
                  // radial line(s) (for original point)
                  RadialLine radialshort(vertex, shortline_segend, next_vertex.m_point,vertex.m_point,vertex.m_point+vertex.m_b);
                  poly_connections.push_back( PolyConnector(shortline, (RadialKey)radialshort) );
                  radial_lines.add(radialshort);
                  if (!next_vertex.m_convex && next_vertex.m_axial) {
                     Line longline = Line(line.t_start(),vertex.m_point);
                     RadialLine radiallong(radialshort);
                     radiallong.segend = shortline_segend ? 0 : 1;
                     poly_connections.push_back( PolyConnector(longline, (RadialKey)radiallong) );
                     radial_lines.add(radiallong);
                  }
               }
               if (possible && next_vertex.m_axial) {
                  // axial line
                  lines.push_back(line);
                  keyvertices.push_back(pvecint());
                  if (vertex.m_convex) {
                     keyvertices.tail().add(vertex.m_ref_key);
                  }
                  if (next_vertex.m_convex) {
                     keyvertices.tail().add(next_vertex.m_ref_key);
                  }
               }
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

// not really used as yet, a feature to make all the polygons from the vertex
// possibles list

void AxialPolygons::makePolygons(prefvec<pqvector<Point2f>>& polygons)
{
   prefvec<pvecint> handled_list;
   for (size_t j = 0; j < m_vertex_possibles.size(); j++) {
      handled_list.push_back(pvecint());
   }

   for (size_t i = 0; i < m_vertex_possibles.size(); i++) {
      if (m_vertex_possibles.value(i).size() == 1) {
         continue;
      }
      for (size_t j = 0; j < m_vertex_possibles.value(i).size(); j++) {
         if (handled_list[i].findindex(j) != paftl::npos) {
            continue;
         }
         handled_list[i].push_back(j);
         Point2f& key = m_vertex_possibles.key(i);
         pqvector<Point2f> polygon;
         polygon.push_back(key);
         Point2f curr = m_vertex_possibles.value(i).at(j);
         Point2f last = key;
         bool good = true;
         while (curr != key) {
            size_t n = m_vertex_possibles.searchindex(curr);
            polygon.push_back(curr);
            // hunt down left most
            int winner = -1, wayback = -1;
            double minangle = 2 * M_PI;
            for (size_t k = 0; k < m_vertex_possibles.value(n).size(); k++) {
               Point2f next = m_vertex_possibles.value(n).at(k);
               if (last != next) {
                  double thisangle = angle(last,curr,next);
                  if (thisangle < minangle) {
                     // check not going to a dead end:
                     if (m_vertex_possibles.search(m_vertex_possibles.value(n).at(k)).size() > 1) {
                        minangle = thisangle;
                        winner = k;
                     }
                  }
               }
               else {
                  wayback = k;
               }
            }
            if (winner == -1) {
               // this happens when you follow a false trail -- go back the way you came!
               winner = wayback;
            }
            handled_list[n].push_back(winner);
            last = curr;
            curr = m_vertex_possibles.value(n).at(winner);
         }
         if (good) {
            polygons.push_back(polygon);
         }
         good = true;
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool RadialLine::cuts(const Line& l) const
{
   if (fabs(det(l.end() - keyvertex,l.end() - l.start())) < TOLERANCE_A) {
      // point on line, check that openspace and next vertex are on opposite sides of the line
      Point2f x = l.end() - keyvertex;
      Point2f y = nextvertex - keyvertex;
      Point2f z = openspace - keyvertex;
      x.normalise();
      y.normalise();
      z.normalise();
      if (sgn(det(x, y)) == sgn(det(x, z)) && fabs(det(x, z)) > TOLERANCE_A) {
         return false;
      }
   }
   // keyvertex not on line... the line's been cut:
   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// for this to work, your polygons should not intersect each other!

bool ShapeGraphs::makeAllLineMap(Communicator *comm, SuperSpacePixel& superspacepix, const Point2f& seed)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 3 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   // this has a nasty habit of crashing if reused...
   // reset everything at the top level, including any existing all-line map:
   m_polygons.clear();
   m_poly_connections.clear();
   m_radial_lines.clear();

   // this is an index to look up the all line map, used by UI to determine if can make fewest line map
   // note: it is not saved for historical reasons
   if (m_all_line_map != -1) {
      removeMap(m_all_line_map);
      m_all_line_map = -1;
   }

   // starting off... finding a polygon...
   // for ease, I'm just going to make a construction line set from all the visible lines...

   QtRegion region;
   int size = 0;

   prefvec<Line> lines;

   // add all visible layers to the set of polygon lines...
   for (size_t i = 0; i < superspacepix.size(); i++) {
      for (size_t j = 0; j < superspacepix.at(i).size(); j++) {
         if (superspacepix.at(i).at(j).isShown()) {
            if (region.atZero()) {
               region = superspacepix.at(i).at(j).getRegion();
            }
            else {
               region = runion(region,superspacepix.at(i).at(j).getRegion());
            }
            auto refShapes = superspacepix.at(i).at(j).getAllShapes();
            for (auto refShape: refShapes) {
                SalaShape& shape = refShape.second;
               if (shape.isLine()) {
                  lines.push_back(shape.getLine());
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     lines.push_back(Line(shape[n],shape[n+1]));
                  }
                  if (shape.isPolygon()) {
                     lines.push_back(Line(shape.tail(),shape.head()));
                  }
               }
            }
         }
      }
   }

   region.grow(1.30);
   m_polygons.init(lines, region);
   m_polygons.m_handled_list.clear();

   // find a corner visible from the seed:
   AxialVertexKey seedvertex = m_polygons.seedVertex( seed );

   if (seedvertex == NoVertex) {
      // oops... can't find a visible vertex
      return false;
   }

   // okay, we've got as far as finding a seed corner, now the real fun begins...
   // test outwards from corner, add other corners to
   // test set...
   prefvec<Line> axiallines;
   prefvec<pvecint> preaxialdata;
   // also poly_connections used in fewest line axial map construction:
   m_poly_connections.clear();
   m_radial_lines.clear();

   AxialVertex vertex = m_polygons.makeVertex(seedvertex, seed);
   if (!vertex.m_initialised) {
      // oops... can't init for some reason
      return false;
   }

   // Quick mod - TV
#if defined(_WIN32)
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif
   int count = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_polygons.m_vertex_possibles.size() );
   }

   pqvector<AxialVertex> openvertices;
   openvertices.add(vertex);
   while (openvertices.size()) {
      m_polygons.makeAxialLines(openvertices, axiallines, preaxialdata, m_poly_connections, m_radial_lines);
      count++;
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
         }         
      }

   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 3 );
      comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
   }

   // cut out duplicates:
   int removed = 0;  // for testing purposes
   for (size_t j = 0; j < axiallines.size(); j++) {
      for (size_t k = axiallines.size() - 1; k > j; k--) {
         double maxdim = __max(region.width(),region.height());
         if (approxeq(axiallines[j].start(), axiallines[k].start(), maxdim * TOLERANCE_B) && approxeq(axiallines[j].end(), axiallines[k].end(), maxdim * TOLERANCE_B)) {
            for (size_t m = 0; m < preaxialdata[k].size(); m++) {
               preaxialdata[j].add(preaxialdata[k][m]);
            }
            preaxialdata.remove_at(k);
            axiallines.remove_at(k);
            removed++;
         }
      }
   }

/*
   // No longer required for ShapeMaps version:
   if (!m_length) {
      // now add to the space pixel:
      m_region = m_polygons.m_region;
   }
   else {
      m_region = runion(m_region, m_polygons.m_region);
   }
   // End No longer required
*/

   // create the all line map layer...
   m_all_line_map = addMap("All-Line Map", ShapeMap::ALLLINEMAP);
   
   ShapeGraph& alllinemap = at(m_all_line_map);
   // make sure it's cleared fully
   alllinemap.clearAll();
   
/*  
   // temp: 
   alllinemap.initLines(m_polygons.m_lines.size(),m_polygons.m_region.bottom_left,m_polygons.m_region.top_right,2);
   for (int k = 0; k < m_polygons.m_lines.size(); k++) {
      alllinemap.makeLineShape(m_polygons.m_lines[k].line);
   }
   alllinemap.sortPixelLines();
   // end temp
*/
   region.grow(0.99); // <- this paired with crop code below to prevent error
   alllinemap.init(axiallines.size(),m_polygons.m_region);  // used to be double density here
   for (size_t k = 0; k < axiallines.size(); k++) {
      axiallines[k].crop(region); // <- should be cropped anyway, but causing an error
      alllinemap.makeLineShape(axiallines[k]);
   }
   
   // n.b. make connections also initialises attributes
   // -> don't know what this was for: alllinemap.sortBins(m_poly_connections);
   alllinemap.makeConnections(preaxialdata);

   alllinemap.m_keyvertexcount = m_polygons.m_vertex_possibles.size();

   // we can stop here for all line axial map!
   setDisplayedMapRef(m_all_line_map);

   return true;
}

bool ShapeGraphs::makeFewestLineMap(Communicator *comm, bool replace_existing)
{
   // no all line map
   if (m_all_line_map == -1) {
      return false;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   pafsrand((unsigned int)time(NULL));

   // make one rld for each radial line...
   pqmap<RadialKey,pvecint> radialdivisions;
   size_t i;
   for (i = 0; i < m_radial_lines.size(); i++) {
      radialdivisions.add( (RadialKey) m_radial_lines[i], pvecint() );
   }

   // also, a list of radial lines cut by each axial line
   pqmap<int,pvecint> ax_radial_cuts;
   pqmap<int,pvecint> ax_seg_cuts;
   for (auto shape: at(m_all_line_map).m_shapes) {
      ax_radial_cuts.add(shape.first, pvecint());
      ax_seg_cuts.add(shape.first, pvecint());
   }

   // make divisions -- this is the slow part and the comm updates
   at(m_all_line_map).makeDivisions(m_poly_connections, m_radial_lines, radialdivisions, ax_radial_cuts, comm);

   // the slow part is over, we're into the final straight... reset the current record flag:
   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
   }

   // a little further setting up is still required...
   pqmap<RadialKey,RadialSegment> radialsegs;

   // now make radial segments from the radial lines... (note, start at 1)
   for (i = 1; i < m_radial_lines.size(); i++) {
      if (m_radial_lines[i].vertex == m_radial_lines[i-1].vertex) {
         if (m_radial_lines[i].ang == m_radial_lines[i-1].ang) {
            continue;
         }
         else {
            // Quick mod - TV
#if defined(_WIN32)
            radialsegs.add( (RadialKey)m_radial_lines[i], (RadialKey)m_radial_lines[i-1]);
#else
            radialsegs.add( (RadialKey)m_radial_lines[i], (RadialSegment)m_radial_lines[i-1]);
#endif
         }
      }
   }

   // and segment divisors from the axial lines...
   for (i = 0; i < at(m_all_line_map).m_shapes.size(); i++) {
      for (size_t j = 1; j < ax_radial_cuts[i].size(); j++) {
         // note similarity to loop above
         RadialKey rk_end = m_radial_lines[ax_radial_cuts[i][j]];
         RadialKey rk_start = m_radial_lines[ax_radial_cuts[i][j-1]];
         if (rk_start.vertex == rk_end.vertex) {
            size_t index = radialsegs.searchindex(rk_end);
            if (index != paftl::npos && rk_start == radialsegs[index].radial_b) {
               radialsegs[index].add(ax_radial_cuts.key(i));
               ax_seg_cuts[i].add(index);
            }
         }
      }
   }

   // and a little more setting up: key vertex relationships
   prefvec<pvecint> keyvertexconns;
   int *keyvertexcounts = new int [at(m_all_line_map).m_keyvertexcount]; 
   for (int x = 0; x < at(m_all_line_map).m_keyvertexcount; x++) {
      keyvertexcounts[x] = 0;
   }
   // this sets up a two step relationship: looks for the key vertices for all lines connected to you
   for (size_t y = 0; y < at(m_all_line_map).m_connectors.size(); y++) {
      keyvertexconns.push_back(pvecint());
      Connector& axa = at(m_all_line_map).m_connectors[y];
      for (size_t z = 0; z < axa.m_connections.size(); z++) {
         pvecint& axb = at(m_all_line_map).m_keyvertices[axa.m_connections[z]];
         for (size_t zz = 0; zz < axb.size(); zz++) {
            if (keyvertexconns[y].searchindex(axb[zz]) == paftl::npos) {
               keyvertexconns[y].add(axb[zz],paftl::ADD_HERE);
               keyvertexcounts[axb[zz]] += 1;
            }
         }
      }
   }

   // ok, after this fairly tedious set up, we are ready to go... 
   // note axradialcuts aren't required anymore...

   AxialMinimiser minimiser(at(m_all_line_map), ax_seg_cuts, radialsegs);

   prefvec<Line> lines_s, lines_m;

   minimiser.removeSubsets(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines)
   size_t k;
   for (k = 0; k < at(m_all_line_map).m_shapes.size(); k++) {
      if (!minimiser.removed(k)) {
         lines_s.push_back( at(m_all_line_map).m_shapes.find(k)->second.getLine() );
      }
   }

   minimiser.fewestLongest(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines
   for (k = 0; k < at(m_all_line_map).m_shapes.size(); k++) {
      if (!minimiser.removed(k)) {
         lines_m.push_back( at(m_all_line_map).m_shapes.find(k)->second.getLine() );
      }
   }

   delete [] keyvertexcounts;

   int subsetmapindex = getMapRef("Fewest-Line Map (Subsets)");
   if (subsetmapindex == -1) {
      // didn't used to have hyphenation, try once more:
      subsetmapindex = getMapRef("Fewest Line Map (Subsets)");
   }
   if (subsetmapindex == -1) {
      // create the fewest line map layer...
      subsetmapindex = addMap("Fewest-Line Map (Subsets)",ShapeMap::AXIALMAP);
      // note: new map has replace_existing set to true to ensure "init"
      replace_existing = true;
   }
   ShapeGraph& fewestlinemap_subsets = at(subsetmapindex);
   // note: new map has replace_existing set to true to ensure "init"
   if (replace_existing) {
      fewestlinemap_subsets.clearAll();
      fewestlinemap_subsets.init(lines_s.size(),m_polygons.m_region); // used to have a '2' for double pixel density
   }
   for (k = 0; k < lines_s.size(); k++) {
      fewestlinemap_subsets.makeLineShape(lines_s[k]);
   }
   fewestlinemap_subsets.makeConnections();

   int minimalmapindex = getMapRef("Fewest-Line Map (Minimal)");
   if (minimalmapindex == -1) {
      // didn't used to have hyphenation, try once more:
      minimalmapindex = getMapRef("Fewest Line Map (Minimal)");
   }
   if (minimalmapindex == -1) {
      // create the fewest line map layer...
      minimalmapindex = addMap("Fewest-Line Map (Minimal)",ShapeMap::AXIALMAP);
      // note: new map has replace_existing set to true to ensure "init"
      replace_existing = true;
   }
   ShapeGraph& fewestlinemap_minimal = at(minimalmapindex);
   // note: new map has replace_existing set to true to ensure "init"
   if (replace_existing) {
      fewestlinemap_minimal.clearAll();
      fewestlinemap_minimal.init(lines_m.size(),m_polygons.m_region); // used to have a '2' for double pixel density
   }
   for (k = 0; k < lines_m.size(); k++) {
      fewestlinemap_minimal.makeLineShape(lines_m[k]);
   }
   fewestlinemap_minimal.makeConnections();

   setDisplayedMapRef(subsetmapindex);

   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

AxialMinimiser::AxialMinimiser(const ShapeGraph& alllinemap, pqmap<int,pvecint>& axsegcuts, pqmap<RadialKey,RadialSegment>& radialsegs)
{
   m_alllinemap = (ShapeGraph *) &alllinemap;

   m_vps = new ValueTriplet[axsegcuts.size()];
   m_removed = new bool [axsegcuts.size()];
   m_affected = new bool [axsegcuts.size()];
   m_vital = new bool [axsegcuts.size()];
   m_radialsegcounts = new int [radialsegs.size()];
}

AxialMinimiser::~AxialMinimiser()
{
   delete [] m_vital;
   delete [] m_affected;   
   delete [] m_radialsegcounts;
   delete [] m_vps;
   delete [] m_removed;
}

// Alan and Bill's algo...

void AxialMinimiser::removeSubsets(pqmap<int,pvecint>& axsegcuts, pqmap<RadialKey,RadialSegment>& radialsegs, pqmap<RadialKey,pvecint>& rlds,  pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts)
{
   bool removedflag = true;
   int counterrors = 0;

   m_axialconns = m_alllinemap->m_connectors;

   for (size_t x = 0; x < radialsegs.size(); x++) {
      m_radialsegcounts[x] = 0;
   }
   for (size_t y = 0; y < axsegcuts.size(); y++) {
      for (size_t z = 0; z < axsegcuts[y].size(); z++) {
         m_radialsegcounts[axsegcuts[y][z]] += 1;
      }
      m_removed[y] = false;
      m_vital[y] = false;
      m_affected[y] = true;
      m_vps[y].index = y;
      double length = m_axialconns[y].m_connections.size();
      m_vps[y].value1 = (int) length;
      length = m_alllinemap->m_shapes.find(y)->second.getLine().length();
      m_vps[y].value2 = (float) length;
   }

   // sort according to number of connections then length
   qsort(m_vps,m_axialconns.size(),sizeof(ValueTriplet),compareValueTriplet);

   while (removedflag) {

      removedflag = false;
      for (size_t i = 0; i < m_axialconns.size(); i++) {
         int ii = m_vps[i].index;
         if (m_removed[ii] || !m_affected[ii] || m_vital[ii]) {
            continue;
         }
         // vital connections code (uses original unaltered connections)
         {
            bool vitalconn = false;
            for (size_t j = 0; j < keyvertexconns[ii].size(); j++) {
               // first check to see if removing this line will cause elimination of a vital connection
               if (keyvertexcounts[keyvertexconns[ii][j]] <= 1) {
                  // connect vital... just go on to the next one:
                  vitalconn = true;
                  break;
               }
            }
            if (vitalconn) {
               m_vital[ii] = true;
               continue;
            }
         }
         //
         Connector& axa = m_axialconns[ii];
         m_affected[ii] = false;
         bool subset = false;
         for (size_t j = 0; j < axa.m_connections.size(); j++) {
            int indextob = axa.m_connections[j];
            if (indextob == ii || m_removed[indextob]) { // <- removed[indextob] should never happen as it should have been removed below
               continue;
            }
            Connector& axb = m_axialconns[indextob];
            if (axa.m_connections.size() <= axb.m_connections.size()) {
               // change to 10.08, coconnecting is 1 -> connection to other line is implicitly handled
               int coconnecting = 1;
               // first check it's a connection subset
               // note that changes in 10.08 mean that lines no longer connect to themselves
               // this means that the subset 1 connects {2,3} and 2 connects {1,3} are equivalent
               for (size_t axai = 0, axbi = 0; axai < axa.m_connections.size() && axbi < axb.m_connections.size(); axai++, axbi++) {
                  // extra 10.08 -> step over connection to b
                  if (axa.m_connections[axai] == indextob) {
                     axai++;
                  }
                  // extra 10.08 add axb.m_connections[axbi] == ii -> step over connection to a
                  while (axbi < axb.m_connections.size() && (axb.m_connections[axbi] == ii || axa.m_connections[axai] > axb.m_connections[axbi])) {
                     axbi++;
                  }
                  if (axbi >= axb.m_connections.size()) {
                     break;
                  }
                  else if (axa.m_connections[axai] == axb.m_connections[axbi]) {
                     coconnecting++;
                  }
                  else if (axa.m_connections[axai] < axb.m_connections[axbi]) {
                     break;
                  }
               }
               if (coconnecting >= (int)axa.m_connections.size()) {
                  subset = true;
                  break;
               }
            }
         }
         if (subset) {
            size_t removeindex = ii;
            // now check removing it won't break any topological loops
            bool presumedvital = false;
            for (size_t k = 0; k < axsegcuts[removeindex].size(); k++) {
               if (m_radialsegcounts[axsegcuts[removeindex][k]] <= 1) {
                  presumedvital = true;
                  break;
               }
            }
            if (presumedvital) {
               presumedvital = checkVital(removeindex,axsegcuts[removeindex],radialsegs,rlds,radial_lines);
            }
            if (presumedvital) {
               m_vital[removeindex] = true;
            }
            // if not, remove it...
            if (!m_vital[removeindex]) {
               m_removed[removeindex] = true;
               pvecint& affectedconnections = m_axialconns[removeindex].m_connections;
               size_t k;
               for (k = 0; k < affectedconnections.size(); k++) {
                  if (!m_removed[affectedconnections[k]]) {
                     pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
                     size_t index = connections.searchindex(removeindex);
                     if (index != paftl::npos) {
                        connections.remove_at(index);
                     }
                     m_affected[affectedconnections[k]] = true;
                  }
               }
               removedflag = true;
               for (k = 0; k < axsegcuts[removeindex].size(); k++) {
                  m_radialsegcounts[axsegcuts[removeindex][k]] -= 1;
               }
               // vital connections
               for (k = 0; k < keyvertexconns[removeindex].size(); k++) {
                  keyvertexcounts[keyvertexconns[removeindex][k]] -= 1;
               }
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

// My algo... v. simple... fewest longest

void AxialMinimiser::fewestLongest(pqmap<int,pvecint>& axsegcuts, pqmap<RadialKey,RadialSegment>& radialsegs, pqmap<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines, prefvec<pvecint>& keyvertexconns, int *keyvertexcounts)
{
   //m_axialconns = m_alllinemap->m_connectors;
   int livecount = 0;

   for (size_t y = 0; y < m_axialconns.size(); y++) {
      if (!m_removed[y] && !m_vital[y]) {
         m_vps[livecount].index = (int) y;
         m_vps[livecount].value1 = (int) m_axialconns[y].m_connections.size();
         m_vps[livecount].value2 = (float) m_alllinemap->m_shapes.find(y)->second.getLine().length();
         livecount++;
      }
   }

   qsort(m_vps,livecount,sizeof(ValueTriplet),compareValueTriplet);

   for (int i = 0; i < livecount; i++) {

      int j = m_vps[i].index;
      // vital connections code (uses original unaltered connections)
      bool vitalconn = false;
      size_t k;
      for (k = 0; k < keyvertexconns[j].size(); k++) {
         // first check to see if removing this line will cause elimination of a vital connection
         if (keyvertexcounts[keyvertexconns[j][k]] <= 1) {
            // connect vital... just go on to the next one:
            vitalconn = true;
            break;
         }
      }
      if (vitalconn) {
         continue;
      }
      //
      bool presumedvital = false;
      for (k = 0; k < axsegcuts[j].size(); k++) {
         if (m_radialsegcounts[axsegcuts[j][k]] <= 1) {
            presumedvital = true;
            break;
         }
      }
      if (presumedvital) {
         presumedvital = checkVital(j,axsegcuts[j],radialsegs,rlds,radial_lines);
      }
      if (!presumedvital) {
         // don't let anything this is connected to go down to zero connections
         pvecint& affectedconnections = m_axialconns[j].m_connections;
         for (size_t k = 0; k < affectedconnections.size(); k++) {
            if (!m_removed[affectedconnections[k]]) {
               pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
               if (connections.size() <= 2) { // <- note number of connections includes itself... so you and one other
                  presumedvital = true;
                  break;
               }
            }
         }
      }
      if (!presumedvital) {
         m_removed[j] = true;
         pvecint& affectedconnections = m_axialconns[j].m_connections;
         size_t k;
         for (k = 0; k < affectedconnections.size(); k++) {
            if (!m_removed[affectedconnections[k]]) {
               pvecint& connections = m_axialconns[affectedconnections[k]].m_connections;
               size_t index = connections.searchindex(j);
               if (index != paftl::npos) {
                  connections.remove_at(index);
               }
               m_affected[affectedconnections[k]] = true;
            }
         }
         for (k = 0; k < axsegcuts[j].size(); k++) {
            m_radialsegcounts[axsegcuts[j][k]] -= 1;
         }
         // vital connections
         for (k = 0; k < keyvertexconns[j].size(); k++) {
            keyvertexcounts[keyvertexconns[j][k]] -= 1;
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool AxialMinimiser::checkVital(int checkindex, pvecint& axsegcuts, pqmap<RadialKey,RadialSegment>& radialsegs, pqmap<RadialKey,pvecint>& rlds, pqvector<RadialLine>& radial_lines)
{
   std::map<int,SalaShape>& axiallines = m_alllinemap->m_shapes;

   bool presumedvital = true;
   int nonvitalcount = 0, vitalsegs = 0;
   // again, this time more rigourously... check any connected pairs don't cover the link...
   for (size_t k = 0; k < axsegcuts.size(); k++) {
      if (m_radialsegcounts[axsegcuts[k]] <= 1) {
         bool nonvitalseg = false;
         vitalsegs++;
         RadialKey& key = radialsegs.key(axsegcuts[k]);
         RadialSegment& seg = radialsegs.value(axsegcuts[k]);
         pvecint& divisorsa = rlds.search(key);
         pvecint& divisorsb = rlds.search(seg.radial_b);
         RadialLine& rlinea = radial_lines.search(key);
         RadialLine& rlineb = radial_lines.search(seg.radial_b);
         for (size_t divi = 0; divi < divisorsa.size(); divi++) {
            if (divisorsa[divi] == checkindex || m_removed[divisorsa[divi]]) {
               continue;
            }
            for (size_t divj = 0; divj < divisorsb.size(); divj++) {
               if (divisorsb[divj] == checkindex || m_removed[divisorsb[divj]]) {
                  continue;
               }
               if (m_axialconns[divisorsa[divi]].m_connections.searchindex(divisorsb[divj]) != paftl::npos) {
                  // as a further challenge, they must link within in the zone of interest, not on the far side of it... arg!
                  Point2f p = intersection_point(axiallines[divisorsa[divi]].getLine(),axiallines[divisorsb[divj]].getLine(),TOLERANCE_A);
                  if (p.insegment(rlinea.keyvertex,rlinea.openspace,rlineb.openspace,TOLERANCE_A)) {
                     nonvitalseg = true;
                  }
               }
            }
         }
         if (nonvitalseg) {
            nonvitalcount++;
         }
      }
   }
   if (nonvitalcount == vitalsegs) {
      presumedvital = false;
   }
   return presumedvital;
}

///////////////////////////////////////////////////////////////////////////////////////////

// convert line layers to an axial map

int ShapeGraphs::convertDrawingToAxial(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   QtRegion region;
   pqmap<int,Line> lines;  // map required for tidy lines, otherwise acts like vector
   pmap<int,int> layers;  // this is used to say which layer it originated from

   bool recordlayer = false;

   // add all visible layers to the set of polygon lines...
   int count = 0;
   for (size_t i = 0; i < superspacepix.size(); i++) {
      for (size_t j = 0; j < superspacepix.at(i).size(); j++) {
         if (superspacepix.at(i).at(j).isShown()) {
            if (region.atZero()) {
               region = superspacepix.at(i).at(j).getRegion();
            }
            else {
               region = runion(region,superspacepix.at(i).at(j).getRegion());
            }
            auto refShapes = superspacepix.at(i).at(j).getAllShapes();
            for (auto refShape: refShapes) {
                SalaShape& shape = refShape.second;
               if (shape.isLine()) {
                  lines.add(count,shape.getLine());
                  layers.add(count,j);
                  count++;
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     lines.add(count,Line(shape[n],shape[n+1]));
                     layers.add(count,j);
                     count++;
                  }
                  if (shape.isPolygon()) {
                     lines.add(count,Line(shape.tail(),shape.head()));
                     layers.add(count,j);
                     count++;
                  }
               }
            }
            superspacepix.at(i).at(j).setShow(false);
         }
         if (j > 0) {
            recordlayer = true;
         }
      }
   }
   if (count == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      return -1;
   }


   /*
   // No longer required
   if (!m_length) {
      // now add to the space pixel:
      m_region = region;
   }
   else {
      m_region = runion(region, m_region);
   }
   // End no longer required
   */

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   /*
   // No longer required
   // now add to the space pixel:
   if (m_region.isempty()) {
      m_region = region;
   }
   else {
      m_region = runion(region,m_region);
   }
   */

   // create map layer...
   int mapref = addMap(name,ShapeMap::AXIALMAP);
   // we can stop here for all line axial map!
   ShapeGraph& usermap = tail();

   usermap.init(lines.size(),region);        // used to be double density
   for (size_t k = 0; k < lines.size(); k++) {
      usermap.makeLineShape(lines[k]);
   }

   // n.b. make connections also initialises attributes
   usermap.makeConnections();

   // record origin layer only if more than one layer:
   if (recordlayer) {
      AttributeTable& table = usermap.getAttributeTable();
      int col = table.insertColumn("Drawing Layer");
      for (size_t k = 0; k < lines.size(); k++) {
         table.setValue(k,col,float(layers.search(lines.key(k))));
      }
   }

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

// create axial map directly from data maps 
// note that actually should be able to merge this code with the line layers, now both use similar code

int ShapeGraphs::convertDataToAxial(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   // add all visible layers to the set of polygon lines...

   pqmap<int,Line> lines;
   pmap<int,int> keys;

   //m_region = shapemap.getRegion();
   QtRegion region = shapemap.getRegion();

   // add all visible layers to the set of polygon lines...

   int count = 0;
   for (auto shape: shapemap.getAllShapes()) {
      int key = shape.first;
      const SalaShape& poly = shape.second;
      if (poly.isLine()) {
         lines.add(count,poly.getLine());
         keys.add(count,key);
         count++;
      }
      else if (poly.isPolyLine()) {
         for (size_t j = 0; j < poly.size() - 1; j++) {
            lines.add(count,Line(poly[j],poly[j+1]));
            keys.add(count,key);
            count++;
         }
      }
   }
   if (lines.size() == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      return -1;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::AXIALMAP);
   // we can stop here for all line axial map!
   ShapeGraph& usermap = tail();

   usermap.init(lines.size(),region);  // used to be double density
   for (size_t k = 0; k < lines.size(); k++) {
      usermap.makeLineShape(lines[k]);
   }

   // n.b. make connections also initialises attributes
   usermap.makeConnections();

   // use property that segments are still in same order as input in order to copy
   // data across from ShapeMap
   if (copydata) {
      AttributeTable& input = shapemap.getAttributeTable();
      AttributeTable& output = usermap.getAttributeTable();
      for (int i = 0; i < input.getColumnCount(); i++) {
         std::string colname = input.getColumnName(i);
         for (size_t k = 1; output.getColumnIndex(colname) != -1; k++) 
            colname = dXstring::formatString((int)k,input.getColumnName(i) + " %d");
         int outcol = output.insertColumn(colname);
         for (size_t j = 0; j < lines.size(); j++) {
            int inrow = input.getRowid(keys.search(lines.key(j)));
            output.setValue(j,outcol,input.getValue(inrow,i));
         }
      }
   }

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.getMapInfoData()) {
      usermap.m_mapinfodata = new MapInfoData;
      usermap.m_mapinfodata->m_coordsys = shapemap.getMapInfoData()->m_coordsys;
      usermap.m_mapinfodata->m_bounds = shapemap.getMapInfoData()->m_bounds;
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( usermap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// yet more conversions, this time polygons to shape elements

int ShapeGraphs::convertDrawingToConvex(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix)
{
   QtRegion region;
   pvecint polygon_refs;

   int mapref = addMap(name,ShapeMap::CONVEXMAP);
   ShapeGraph& usermap = tail();
   int conn_col = usermap.m_attributes.insertLockedColumn("Connectivity");
   
   size_t count = 0;
   size_t i = 0;
   for (i = 0; i < superspacepix.size(); i++) {
      for (size_t j = 0; j < superspacepix.at(i).size(); j++) {
         if (superspacepix.at(i).at(j).isShown()) {
             auto refShapes = superspacepix.at(i).at(j).getAllShapes();
             for (auto refShape: refShapes) {
                 SalaShape& shape = refShape.second;
               if (shape.isPolygon()) {
                  usermap.makeShape(shape);
                  usermap.m_connectors.push_back( Connector() );
                  usermap.m_attributes.setValue(count,conn_col,0);
                  count++;
               }
            }
         }
      }
   }
   if (count == 0) {
      removeMap(mapref);
      return -1;
   }

   for (i = 0; i < superspacepix.size(); i++) {
      for (size_t j = 0; j < superspacepix.at(i).size(); j++) {
         superspacepix.at(i).at(j).setShow(false);
      }
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( -1 );
   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

int ShapeGraphs::convertDataToConvex(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   pvecint polygon_refs;

   int mapref = addMap(name,ShapeMap::CONVEXMAP);
   ShapeGraph& usermap = getMap(mapref);
   int conn_col = usermap.m_attributes.insertLockedColumn("Connectivity");

   pvecint lookup;
   auto refShapes = shapemap.getAllShapes();
   int k = -1;
   for (auto refShape: refShapes) {
      k++;
      SalaShape& shape = refShape.second;
      if (shape.isPolygon()) {
         int n = usermap.makeShape(shape);
         usermap.m_connectors.push_back( Connector() );
         usermap.m_attributes.setValue(n,conn_col,0);
         lookup.push_back(k);
      }
   }
   if (lookup.size() == 0) {
      removeMap(mapref);
      return -1;
   }

   if (copydata) {
      AttributeTable& input = shapemap.getAttributeTable();
      AttributeTable& output = usermap.getAttributeTable();
      for (int i = 0; i < input.getColumnCount(); i++) {
         std::string colname = input.getColumnName(i);
         for (int k = 1; output.getColumnIndex(colname) != -1; k++) 
            colname = dXstring::formatString(k,input.getColumnName(i) + " %d");
         int outcol = output.insertColumn(colname);
         for (size_t j = 0; j < lookup.size(); j++) {
            output.setValue(j,outcol,input.getValue(lookup[j],i));
         }
      }
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( -1 );
   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// create segment map directly from line layers

int ShapeGraphs::convertDrawingToSegment(Communicator *comm, const std::string& name, SuperSpacePixel& superspacepix)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   pqmap<int,Line> lines;  // pqmap for tidier, does not matter elsewhere...
   pmap<int,int> layers;  // this is used to say which layer it originated from
   bool recordlayer = false;

   QtRegion region;

   // add all visible layers to the set of polygon lines...
   int count = 0;
   for (size_t i = 0; i < superspacepix.size(); i++) {
      for (size_t j = 0; j < superspacepix.at(i).size(); j++) {
         if (superspacepix.at(i).at(j).isShown()) {
            if (region.atZero()) {
               region = superspacepix.at(i).at(j).getRegion();
            }
            else {
               region = runion(region,superspacepix.at(i).at(j).getRegion());
            }

            auto refShapes = superspacepix.at(i).at(j).getAllShapes();
            for (auto refShape: refShapes) {
                SalaShape& shape = refShape.second;
               if (shape.isLine()) {
                  lines.add(count,shape.getLine());
                  layers.add(count,j);
                  count++;
               }
               else if (shape.isPolyLine() || shape.isPolygon()) {
                  for (size_t n = 0; n < shape.size() - 1; n++) {
                     lines.add(count,Line(shape[n],shape[n+1]));
                     layers.add(count,j);
                     count++;
                  }
                  if (shape.isPolygon()) { // add closing line
                     lines.add(count,Line(shape.tail(),shape.head()));
                     layers.add(count,j);
                     count++;
                  }
               }
            }
            superspacepix.at(i).at(j).setShow(false);
         }
         if (j > 0) {
            recordlayer = true;
         }
      }
   }
   if (count == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);
   if (lines.size() == 0) {
      return -1;
   }
   
   /*
   // No longer required for ShapeMaps version:
   if (!m_length) {
      // now add to the space pixel:
      m_region = region;
   }
   else {
      m_region = runion(region, m_region);
   }
   // End No longer required
   */

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);
   // we can stop here for all line axial map!
   ShapeGraph& usermap = tail();

   usermap.init(lines.size(),region);

   for (size_t k = 0; k < lines.size(); k++) {
      usermap.makeLineShape(lines[k]);
   }

   // make it!
   usermap.makeNewSegMap();

   // record origin layer only if more than one layer:
   if (recordlayer) {
      AttributeTable& table = usermap.getAttributeTable();
      int col = table.insertColumn("Drawing Layer");
      for (size_t k = 0; k < lines.size(); k++) {
         table.setValue(k,col,float(layers.search(lines.key(k))));
      }
   }

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

// create segment map directly from data maps (ultimately, this will replace the line layers version)

int ShapeGraphs::convertDataToSegment(Communicator *comm, const std::string& name, ShapeMap& shapemap, bool copydata)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   pqmap<int,Line> lines;
   pmap<int,int> keys;

   // no longer requires m_region
   //m_region = shapemap.getRegion();
   QtRegion region = shapemap.getRegion();

   // add all visible layers to the set of polygon lines...

   int count = 0;
   for (auto shape: shapemap.getAllShapes()) {
      int key = shape.first;
      const SalaShape& poly = shape.second;
      if (poly.isLine()) {
         lines.add(count,poly.getLine());
         keys.add(count,key);
         count++;
      }
      else if (poly.isPolyLine()) {
         for (size_t j = 0; j < poly.size() - 1; j++) {
            lines.add(count,Line(poly[j],poly[j+1]));
            keys.add(count,key);
            count++;
         }
      }
   }
   if (lines.size() == 0) {
      return -1;
   }

   // quick tidy removes very short and duplicate lines, but does not merge overlapping lines
   TidyLines tidier;
   tidier.quicktidy(lines, region);

   if (lines.size() == 0) {
      return -1;
   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);

   // note, I may need to reuse this:
   ShapeGraph& usermap = tail();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (shapemap.getMapInfoData()) {
      usermap.m_mapinfodata = new MapInfoData;
      usermap.m_mapinfodata->m_coordsys = shapemap.getMapInfoData()->m_coordsys;
      usermap.m_mapinfodata->m_bounds = shapemap.getMapInfoData()->m_bounds;
   }

   usermap.init(lines.size(),region);
   for (size_t k = 0; k < lines.size(); k++) {
      usermap.makeLineShape(lines[k]);
   }

   // start to be a little bit more efficient about memory now we are hitting the limits
   // from time to time:
   if (!copydata) {
      lines.clear();
   }

   // make it!
   usermap.makeNewSegMap();

   // use property that segments are still in same order as input in order to copy
   // data across from ShapeMap
   if (copydata) {
      AttributeTable& input = shapemap.getAttributeTable();
      AttributeTable& output = usermap.getAttributeTable();
      //
      for (int i = 0; i < input.getColumnCount(); i++) {
         std::string colname = input.getColumnName(i);
         for (int k = 1; output.getColumnIndex(colname) != -1; k++) 
            colname = dXstring::formatString(k,input.getColumnName(i) + " %d");
         int outcol = output.insertColumn(colname);
         for (size_t j = 0; j < lines.size(); j++) {
            int inrow = input.getRowid(keys.search(lines.key(j)));
            output.setValue(j,outcol,input.getValue(inrow,i));
         }
      }
   }

   usermap.m_displayed_attribute = -2; // <- override if it's already showing
   usermap.setDisplayedAttribute( usermap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

#if defined(_WIN32)
#include <windows.h>
#endif

// stubremoval is fraction of overhanging line length before axial "stub" is removed
int ShapeGraphs::convertAxialToSegment(Communicator *comm, const std::string& name, bool keeporiginal, bool copydata, double stubremoval)
{
   if (m_displayed_map == -1) {
      return -1;
   }

   prefvec<Line> lines;
   prefvec<Connector> connectionset;

   ShapeGraph& dispmap = getDisplayedMap();
   dispmap.makeSegmentMap(lines, connectionset, stubremoval);

   // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
   if (!keeporiginal) {
      dispmap.m_shapes.clear();
      dispmap.m_connectors.clear();
   }

   // create map layer...
   int mapref = addMap(name,ShapeMap::SEGMENTMAP);
   ShapeGraph& segmap = getMap(mapref);

   segmap.init(lines.size(),dispmap.m_region);
   for (size_t k = 0; k < lines.size(); k++) {
      segmap.makeLineShape(lines[k]);
   }

   // clear data as soon as we do not need it:
   lines.clear();

   // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
   if (dispmap.m_mapinfodata) {
      segmap.m_mapinfodata = new MapInfoData;
      segmap.m_mapinfodata->m_coordsys = dispmap.m_mapinfodata->m_coordsys;
      segmap.m_mapinfodata->m_bounds = dispmap.m_mapinfodata->m_bounds;
   }

   // initialise attributes now separated from making the connections
   segmap.initSegmentAttributes(connectionset);

   if (copydata) {
      segmap.pushAxialValues(dispmap);
   }
   // destroy unnecessary parts of axial map as quickly as possible in order not to overload memory
   if (!keeporiginal) {
      dispmap.m_attributes.clear();
   }

   // only now make connections, once some memory has been freed
   segmap.makeSegmentConnections(connectionset);

   segmap.m_displayed_attribute = -2; // <- override if it's already showing
   segmap.setDisplayedAttribute( segmap.m_attributes.getColumnIndex("Connectivity") );

   // we can stop here!
   setDisplayedMapRef(mapref);

   return mapref;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool ShapeGraphs::read( istream& stream, int version )
{
   // base class read

   ShapeMaps<ShapeGraph>::read(stream,version);

   // these are additional essentially for all line axial maps
   // should probably be kept *with* the all line axial map...
   m_poly_connections.clear();
   m_poly_connections.read(stream);
   m_radial_lines.clear();
   m_radial_lines.read(stream);

   // this is an index to look up the all line map, used by UI to determine if can make fewest line map
   // note: it is not saved for historical reasons
   // will get confused by more than one all line map
   m_all_line_map = getMapRef("All-Line Map");
   if (m_all_line_map == -1) {
      // used to be called All Line Map
      m_all_line_map = getMapRef("All Line Map");
   }
   if (m_all_line_map != -1) {
      at(m_all_line_map).m_map_type = ShapeMap::ALLLINEMAP;
   }

   // VERSION_AXIAL_REGION_FIX -- this fix is now deprecated
   // some awful things could have gone wrong in the past, but the shapemap read should fix automatically

   return true;
}

// for backward compatibility only:
bool ShapeGraphs::readold( istream& stream, int version )
{
   // this read is based on SpacePixelGroup<ShapeGraph>::read(stream, version);
   dXstring::readString(stream);
   QtRegion dummyregion;
   stream.read( (char *) &dummyregion, sizeof(dummyregion) );
   int count;
   stream.read( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
      push_back(ShapeGraph());
      tail().read(stream,version);
   }
   stream.read((char *)&m_displayed_map,sizeof(m_displayed_map));

   return true;
}

bool ShapeGraphs::write( ofstream& stream, int version, bool displayedmaponly )
{
   // base class write
   ShapeMaps<ShapeGraph>::write(stream, version, displayedmaponly);

   m_poly_connections.write(stream);
   m_radial_lines.write(stream);

   return true;
}

////////////////////////////////////////////////////////////////////////////////////////////

// Axial map helper: convert a radius for angular analysis

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

//////////////////////////////////////////////////////////////////////////////////////////

ShapeGraph::ShapeGraph(const std::string& name, int type) : ShapeMap(name,type)
{
   m_keyvertexcount = 0; 
   m_hasgraph = true;
}

////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

// n.b., also initialises attributes, you must make connections before display map!

void ShapeGraph::makeConnections(const prefvec<pvecint>& keyvertices)
{
   m_connectors.clear();
   m_attributes.clear();
   m_links.clear();
   m_unlinks.clear();
   m_keyvertices.clear();

   // note, expects these to be numbered 0, 1...
   int conn_col = m_attributes.insertLockedColumn("Connectivity");
   int leng_col = m_attributes.insertLockedColumn("Line Length");

   int i = -1;
   for (auto shape: m_shapes) {
      i++;
      int key = shape.first;
      int rowid = m_attributes.insertRow(key);
      // all indices should match...
      m_connectors.push_back( Connector() );
      int connectivity = getLineConnections( key, m_connectors[i].m_connections, TOLERANCE_B*__max(m_region.height(),m_region.width()));
      m_attributes.setValue(rowid, conn_col, (float) connectivity );
      m_attributes.setValue(rowid, leng_col, (float) shape.second.getLine().length() );
      if (keyvertices.size()) {
         // note: depends on lines being recorded in same order as keyvertices...
         m_keyvertices.push_back( keyvertices[i] );
      }
   }

   m_displayed_attribute = -1; // <- override if it's already showing
   setDisplayedAttribute(conn_col);
}

/////////////////////////////////////////////////////////////////////////////////////////

// explicit initialisation of attributes for a gates layer
/*
void ShapeGraph::initAttributes()
{
   m_connectors.clear();
   m_attributes.clear();

   for (int i = 0; i < m_lines.size(); i++) {
      int key = m_lines.key(i);
      // all indices should match...
      int index1 = m_connectors.add( key, Connector() );
      int index2 = m_attributes.insertRow(key);
      // I am going to use this to set the text size soon:
      float textsize = (float) m_lines[i].line.length();
   }

   m_displayed_attribute = -2; // <- override if it's already showing
   // Note: -1 sets it show the ID column:
   setDisplayedAttribute(-1);
}
*/

/////////////////////////////////////////////////////////////////////////////////////////

bool ShapeGraph::outputMifPolygons(ostream& miffile, ostream& midfile) const
{
   // take lines from lines layer and make into regions (using the axial polygons)
   prefvec<Line> lines;
   for (auto shape: m_shapes) {
      lines.push_back(shape.second.getLine());
   }
   AxialPolygons polygons;
   polygons.init(lines, m_region);
   
   prefvec<pqvector<Point2f>> newpolygons;
   polygons.makePolygons(newpolygons);

   MapInfoData mapinfodata;
   if (m_mapinfodata) {
      mapinfodata.m_coordsys = m_mapinfodata->m_coordsys;
      mapinfodata.m_bounds = m_mapinfodata->m_bounds;
   }
   mapinfodata.exportPolygons(miffile, midfile, newpolygons, m_region);

   return true;
}

void ShapeGraph::outputNet(ostream& netfile) const
{
   double maxdim = __max(m_region.width(),m_region.height());
   Point2f offset = Point2f((maxdim - m_region.width())/(2.0*maxdim),(maxdim - m_region.height())/(2.0*maxdim));
   if (isSegmentMap()) {
      netfile << "*Vertices " << m_shapes.size() * 2 << endl;
      int i = -1;
      for (auto shape: m_shapes) {
         i++;
         Line li = shape.second.getLine();
         Point2f p1 = li.start();
         Point2f p2 = li.end();
         p1.x = offset.x + (p1.x - m_region.bottom_left.x) / maxdim;
         p2.x = offset.x + (p2.x - m_region.bottom_left.x) / maxdim;
         p1.y = 1.0 - (offset.y + (p1.y - m_region.bottom_left.y) / maxdim);
         p2.y = 1.0 - (offset.y + (p2.y - m_region.bottom_left.y) / maxdim);
         netfile << (i * 2 + 1) << " \"" << i << "a\" " << p1.x << " " << p1.y << endl;
         netfile << (i * 2 + 2) << " \"" << i << "b\" " << p2.x << " " << p2.y << endl;
      }
      netfile << "*Edges" << endl;
      for (size_t i = 0; i < m_shapes.size(); i++) {
         netfile << (i * 2 + 1) << " " << (i * 2 + 2) << " 2" << endl;
      }
      netfile << "*Arcs" << endl;
      // this makes an assumption about which is the "start" and which is the "end"
      // it works for an automatically converted axial map, I'm not sure it works for others...
      for (size_t j = 0; j < m_connectors.size(); j++) {
         const Connector& conn = m_connectors[j];
         for (size_t k1 = 0; k1 < conn.m_forward_segconns.size(); k1++) {
            SegmentRef ref = conn.m_forward_segconns.key(k1);
            float weight = conn.m_forward_segconns.value(k1);
            netfile << (j * 2 + 1) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " " << weight << endl;
         }
         for (size_t k2 = 0; k2 < conn.m_back_segconns.size(); k2++) {
            SegmentRef ref = conn.m_back_segconns.key(k2);
            float weight = conn.m_back_segconns.value(k2);
            netfile << (j * 2 + 2) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " " << weight << endl;
         }
      }
   }
   else {
      netfile << "*Vertices " << m_shapes.size() << endl;
      int i = -1;
      for (auto shape: m_shapes) {
         i++;
         Point2f p = shape.second.getCentroid();
         p.x = offset.x + (p.x - m_region.bottom_left.x) / maxdim;
         p.y = 1.0 - (offset.y + (p.y - m_region.bottom_left.y) / maxdim);
         netfile << (i + 1) << " \"" << i << "\" " << p.x << " " << p.y << endl;
      }
      netfile << "*Edges" << endl;
      for (size_t j = 0; j < m_connectors.size(); j++) {
         const Connector& conn = m_connectors[j];
         for (size_t k = 0; k < conn.m_connections.size(); k++) {
            size_t to = conn.m_connections[k];
            if (j < to) {
               netfile << (j+1) << " " << (to + 1) << " 1" << endl;
            }
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////////////

void ShapeGraph::makeDivisions(const prefvec<PolyConnector>& polyconnections, const pqvector<RadialLine>& radiallines, pqmap<RadialKey,pvecint>& radialdivisions, pqmap<int,pvecint>& axialdividers, Communicator *comm)
{
    // Quick mod - TV
#if defined(_WIN32)
   __time64_t atime = 0;
#else
    time_t atime = 0;
#endif
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, polyconnections.size() );
   }

   for (size_t i = 0; i < polyconnections.size(); i++) {
      PixelRefVector pixels = pixelateLine(polyconnections[i].line);
      pvecint testedshapes;
      size_t connindex = radialdivisions.searchindex(polyconnections[i].key);
      double tolerance = sqrt(TOLERANCE_A);// * polyconnections[i].line.length();
      for (size_t j = 0; j < pixels.size(); j++) {
         PixelRef pix = pixels[j];
         pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
         for (size_t k = 0; k < shapes.size(); k++) {
            ShapeRef& shape = shapes[k];
            if (testedshapes.searchindex(shape.m_shape_ref) != paftl::npos) {
               continue;
            }
            testedshapes.add(shape.m_shape_ref);
            const Line& line = m_shapes.find(shape.m_shape_ref)->second.getLine();
            //
            if (intersect_region(line, polyconnections[i].line, tolerance * line.length()) ) {
               switch ( intersect_line_distinguish(line, polyconnections[i].line, tolerance * line.length()) ) {
               case 0:
                  break;
               case 2:
                  {
                     size_t index = axialdividers.searchindex(shape.m_shape_ref);
                     if (int(index) != shape.m_shape_ref) {
                        throw 1; // for the code to work later this can't be true!
                     }
                     axialdividers[index].add(connindex);
                     radialdivisions[connindex].add(shape.m_shape_ref);
                  }
                  break;
               case 1:
                  {
                     size_t index = axialdividers.searchindex(shape.m_shape_ref);
                     if (int(index) != shape.m_shape_ref) {
                        throw 1; // for the code to work later this can't be true!
                     }
                     //
                     // this makes sure actually crosses between the line and the openspace properly
                     if (radiallines[connindex].cuts(line)) {
                        axialdividers[index].add(connindex);
                        radialdivisions[connindex].add(shape.m_shape_ref);
                     }
                  }
                  break;
               default:
                  break;
               }
            }
         }
      }
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, i );
         }         
      }
   }
}

void ShapeGraph::cutLines(const prefvec<Line>& lines, pqmap<int,pvecint>& axcuts)
{
   for (size_t i = 0; i < lines.size(); i ++) {
      PixelRefVector pixels = pixelateLine(lines[i]);
      pvecint testedshapes;
      for (size_t j = 0; j < pixels.size(); j++) {
         PixelRef pix = pixels[j];
         pqvector<ShapeRef> &shapes = m_pixel_shapes[pix.x][pix.y];
         for (size_t k = 0; k < shapes.size(); k++) {
            ShapeRef& shape = shapes[k];
            if (testedshapes.searchindex(shape.m_shape_ref) != paftl::npos) {
               continue;
            }
            testedshapes.add(shape.m_shape_ref);
            const Line& line = m_shapes.find(shape.m_shape_ref)->second.getLine();
            //
            if (intersect_region(line, lines[i], TOLERANCE_B * line.length()) ) {
               if (intersect_line(line, lines[i], TOLERANCE_B * line.length())) {
                  axcuts.search(shape.m_shape_ref).add(i);
               }
            }
         }
      }
   }
}

typedef pvector<IntPair> IntPairVector;

// n.b., translate radius list before entry
bool ShapeGraph::integrate(Communicator *comm, const pvecint& radius_list, bool choice, bool local, bool fulloutput, int weighting_col, bool simple_version)
{
   // note, from 10.0, Depthmap no longer includes *self* connections on axial lines
   // self connections are stripped out on loading graph files, as well as no longer made

   // Quick mod - TV
#if defined(_WIN32)
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_connectors.size() );
   }

   // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
   // ...to ensure no mess ups, we'll re-sort here:
   bool radius_n = false;
   pvecint radius;
   for (size_t i = 0; i < radius_list.size(); i++) {
      if (radius_list[i] == -1) {
         radius_n = true;
      }
      else {
         radius.add(radius_list[i]);
      }
   }
   if (radius_n) {
      radius.push_back(-1);
   }

   // retrieve weighted col data, as this may well be overwritten in the new analysis:
   pvecdouble weights;
   std::string weighting_col_text;
   if (weighting_col != -1) {
      weighting_col_text = m_attributes.getColumnName(weighting_col);
      for (size_t i = 0; i < m_connectors.size(); i++) {
         weights.push_back(m_attributes.getValue(i,weighting_col));
      }
   }

   // first enter the required attribute columns:
   size_t r;
   for (r = 0; r < radius.size(); r++) {
      std::string radius_text;
      if (radius[r] != -1) {
         radius_text = std::string(" R") + dXstring::formatString(int(radius[r]),"%d");
      }
      if (choice) {
         std::string choice_col_text = std::string("Choice") + radius_text;
         m_attributes.insertColumn(choice_col_text.c_str());
         std::string n_choice_col_text = std::string("Choice [Norm]") + radius_text;
         m_attributes.insertColumn(n_choice_col_text.c_str());
         if (weighting_col != -1) {
            std::string w_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt]" + radius_text;
            m_attributes.insertColumn(w_choice_col_text.c_str());
            std::string nw_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt][Norm]" + radius_text;
            m_attributes.insertColumn(nw_choice_col_text.c_str());
         }
      }

// dX simple version test // TV
//#define _COMPILE_dX_SIMPLE_VERSION
#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string entropy_col_text = std::string("Entropy") + radius_text;
          m_attributes.insertColumn(entropy_col_text.c_str());
      }
#endif

      std::string integ_dv_col_text = std::string("Integration [HH]") + radius_text;
      m_attributes.insertColumn(integ_dv_col_text.c_str());

#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string integ_pv_col_text = std::string("Integration [P-value]") + radius_text;
          m_attributes.insertColumn(integ_pv_col_text.c_str());
          std::string integ_tk_col_text = std::string("Integration [Tekl]") + radius_text;
          m_attributes.insertColumn(integ_tk_col_text.c_str());
          std::string intensity_col_text = std::string("Intensity") + radius_text;
          m_attributes.insertColumn(intensity_col_text.c_str());
          std::string harmonic_col_text = std::string("Harmonic Mean Depth") + radius_text;
          m_attributes.insertColumn(harmonic_col_text.c_str());
      }
#endif

      std::string depth_col_text = std::string("Mean Depth") + radius_text;
      m_attributes.insertColumn(depth_col_text.c_str());
      std::string count_col_text = std::string("Node Count") + radius_text;
      m_attributes.insertColumn(count_col_text.c_str());

#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string rel_entropy_col_text = std::string("Relativised Entropy") + radius_text;
          m_attributes.insertColumn(rel_entropy_col_text);
      }
#endif

      if (weighting_col != -1) {
         std::string w_md_col_text = std::string("Mean Depth [") + weighting_col_text + " Wgt]" + radius_text;
         m_attributes.insertColumn(w_md_col_text.c_str());
         std::string total_weight_text = std::string("Total ") + weighting_col_text + radius_text;
         m_attributes.insertColumn(total_weight_text.c_str());
      }
      if (fulloutput) {

#ifndef _COMPILE_dX_SIMPLE_VERSION
         if(!simple_version) {
             std::string penn_norm_text = std::string("RA [Penn]") + radius_text;
             m_attributes.insertColumn(penn_norm_text);
         }
#endif
         std::string ra_col_text = std::string("RA") + radius_text;
         m_attributes.insertColumn(ra_col_text.c_str());

#ifndef _COMPILE_dX_SIMPLE_VERSION
         if(!simple_version) {
             std::string rra_col_text = std::string("RRA") + radius_text;
             m_attributes.insertColumn(rra_col_text.c_str());
         }
#endif

         std::string td_col_text = std::string("Total Depth") + radius_text;
         m_attributes.insertColumn(td_col_text.c_str());
      }
      //
   }
   if (local) {
#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          m_attributes.insertColumn("Control");
          m_attributes.insertColumn("Controllability");
      }
#endif
   }
   // then look up all the columns... eek:
   pvecint choice_col, n_choice_col, w_choice_col, nw_choice_col, entropy_col, integ_dv_col, integ_pv_col, integ_tk_col, intensity_col,
           depth_col, count_col, rel_entropy_col, penn_norm_col, w_depth_col, total_weight_col, ra_col, rra_col, td_col, harmonic_col;
   for (r = 0; r < radius.size(); r++) {
      std::string radius_text;
      if (radius[r] != -1) {
         radius_text = std::string(" R") + dXstring::formatString(int(radius[r]),"%d");
      }
      if (choice) {
         std::string choice_col_text = std::string("Choice") + radius_text;
         choice_col.push_back(m_attributes.getColumnIndex(choice_col_text.c_str()));
         std::string n_choice_col_text = std::string("Choice [Norm]") + radius_text;
         n_choice_col.push_back(m_attributes.getColumnIndex(n_choice_col_text.c_str()));
         if (weighting_col != -1) {
            std::string w_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt]" + radius_text;
            w_choice_col.push_back(m_attributes.getColumnIndex(w_choice_col_text.c_str()));
            std::string nw_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt][Norm]" + radius_text;
            nw_choice_col.push_back(m_attributes.getColumnIndex(nw_choice_col_text.c_str()));
         }
      }
#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string entropy_col_text = std::string("Entropy") + radius_text;
          entropy_col.push_back(m_attributes.getColumnIndex(entropy_col_text.c_str()));
      }
#endif

      std::string integ_dv_col_text = std::string("Integration [HH]") + radius_text;
      integ_dv_col.push_back(m_attributes.getColumnIndex(integ_dv_col_text.c_str()));

#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string integ_pv_col_text = std::string("Integration [P-value]") + radius_text;
          integ_pv_col.push_back(m_attributes.getColumnIndex(integ_pv_col_text.c_str()));
          std::string integ_tk_col_text = std::string("Integration [Tekl]") + radius_text;
          integ_tk_col.push_back(m_attributes.getColumnIndex(integ_tk_col_text.c_str()));
          std::string intensity_col_text = std::string("Intensity") + radius_text;
          intensity_col.push_back(m_attributes.getColumnIndex(intensity_col_text.c_str()));
          std::string harmonic_col_text = std::string("Harmonic Mean Depth") + radius_text;
          harmonic_col.push_back(m_attributes.getColumnIndex(harmonic_col_text.c_str()));
      }
#endif

      std::string depth_col_text = std::string("Mean Depth") + radius_text;
      depth_col.push_back(m_attributes.getColumnIndex(depth_col_text.c_str()));
      std::string count_col_text = std::string("Node Count") + radius_text;
      count_col.push_back(m_attributes.getColumnIndex(count_col_text.c_str()));

#ifndef _COMPILE_dX_SIMPLE_VERSION
      if(!simple_version) {
          std::string rel_entropy_col_text = std::string("Relativised Entropy") + radius_text;
          rel_entropy_col.push_back(m_attributes.getColumnIndex(rel_entropy_col_text.c_str()));
      }
#endif

      if (weighting_col != -1) {
         std::string w_md_col_text = std::string("Mean Depth [") + weighting_col_text + " Wgt]" + radius_text;
         w_depth_col.push_back(m_attributes.getColumnIndex(w_md_col_text.c_str()));
         std::string total_weight_col_text = std::string("Total ") + weighting_col_text + radius_text;
         total_weight_col.push_back(m_attributes.getColumnIndex(total_weight_col_text.c_str()));
      }
      if (fulloutput) {
         std::string ra_col_text = std::string("RA") + radius_text;
         ra_col.push_back(m_attributes.getColumnIndex(ra_col_text.c_str()));

#ifndef _COMPILE_dX_SIMPLE_VERSION
         if(!simple_version) {
             std::string penn_norm_text = std::string("RA [Penn]") + radius_text;
             penn_norm_col.push_back(m_attributes.getColumnIndex(penn_norm_text));
             std::string rra_col_text = std::string("RRA") + radius_text;
             rra_col.push_back(m_attributes.getColumnIndex(rra_col_text.c_str()));
         }
#endif

         std::string td_col_text = std::string("Total Depth") + radius_text;
         td_col.push_back(m_attributes.getColumnIndex(td_col_text.c_str()));
      }
   }
   int control_col, controllability_col;
   if (local) {
#ifndef _COMPILE_dX_SIMPLE_VERSION
       if(!simple_version) {
           control_col = m_attributes.getColumnIndex("Control");
           controllability_col = m_attributes.getColumnIndex("Controllability");
       }
#endif
   }

   // for choice
   AnalysisInfo **audittrail;
   if (choice) {
      audittrail = new AnalysisInfo *[m_connectors.size()];
      for (size_t i = 0; i < m_connectors.size(); i++) {
         audittrail[i] = new AnalysisInfo [radius.size()];
      }
   }

   // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
   // has already failed due to this!  when intro hand drawn fewest line (where user may have deleted)
   // it's going to get worse...

   bool *covered = new bool [m_connectors.size()];
   for (size_t i = 0; i < m_connectors.size(); i++) {
      for (size_t j = 0; j < m_connectors.size(); j++) {
         covered[j] = false;
      }
      if (choice) {
         for (size_t k = 0; k < m_connectors.size(); k++) {
            audittrail[k][0].previous.ref = -1; // note, 0th member used as radius doesn't matter
            // note, choice columns are not cleared, but cummulative over all shortest path pairs
         }
      }

      if (local) {
         double control = 0.0;
         pvecint& connections = m_connectors[i].m_connections;
         pvecint totalneighbourhood;
         for (size_t j = 0; j < connections.size(); j++) {
            // n.b., as of Depthmap 10.0, connections[j] and i cannot coexist
            // if (connections[j] != i) {
               totalneighbourhood.add(connections[j]); // <- note add does nothing if member already exists
               int intersect_size = 0, retro_size = 0;
               pvecint retconnectors = m_connectors[connections[j]].m_connections;
               for (size_t k = 0; k < retconnectors.size(); k++) {
                  //if (connections[j] != retconnectors[k]) {
                     retro_size++;
                     /*
                     // used for clustering coeff, but clustering coeff next to useless
                     if (connections.searchindex(retconnectors[k]) != paftl::npos) {
                        intersect_size++;
                     }
                     */
                     totalneighbourhood.add(retconnectors[k]); // <- note add does nothing if member already exists
                  //}
               }
               control += 1.0 / double(retro_size);
            //}
         }

#ifndef _COMPILE_dX_SIMPLE_VERSION
         if(!simple_version) {
             if (connections.size() > 0) {
                 m_attributes.setValue(i, control_col, float(control) );
                 m_attributes.setValue(i, controllability_col, float( double(connections.size()) / double(totalneighbourhood.size()-1)) );
             }
             else {
                 m_attributes.setValue(i, control_col, -1 );
                 m_attributes.setValue(i, controllability_col, -1 );
             }
         }
#endif
      }

      pvecint depthcounts;
      depthcounts.push_back(0);
      Connector& thisline = m_connectors[i];
      pflipper<IntPairVector> foundlist;
      foundlist.a().push_back(IntPair(i,-1));
      covered[i] = true;
      int total_depth = 0, depth = 1, node_count = 1, pos = -1, previous = -1; // node_count includes this 1
      double weight = 0.0, rootweight = 0.0, total_weight = 0.0, w_total_depth = 0.0;
      if (weighting_col != -1) {
         rootweight = weights[i];
         // include this line in total weights (as per nodecount)
         total_weight += rootweight;
      }
      register int index = -1;
      for (size_t r = 0; r < radius.size(); r++) {
         while (foundlist.a().size()) {
            if (!choice) {
               index = foundlist.a().tail().a;
            }
            else {
               pos = pafrand() % foundlist.a().size();
               index = foundlist.a().at(pos).a;
               previous = foundlist.a().at(pos).b;
               audittrail[index][0].previous.ref = previous; // note 0th member used here: can be used individually different radius previous
            }
            Connector& line = m_connectors[index];
            double control = 0;
            for (size_t k = 0; k < line.m_connections.size(); k++) {
               if (!covered[line.m_connections[k]]) {
                  covered[line.m_connections[k]] = true;
                  foundlist.b().push_back(IntPair(line.m_connections[k],index));
                  if (weighting_col != -1) {
                     // the weight is taken from the discovered node:
                     weight = weights[line.m_connections[k]];
                     total_weight += weight;
                     w_total_depth += depth * weight;
                  }
                  if (choice && previous != -1) {
                     // both directional paths are now recorded for choice
                     // (coincidentally fixes choice problem which was completely wrong)
                     int here = index; // note: start counting from index as actually looking ahead here
                     while (here != i) { // not i means not the current root for the path
                        audittrail[here][r].choice += 1;
                        audittrail[here][r].weighted_choice += weight * rootweight;
                        here = audittrail[here][0].previous.ref; // <- note, just using 0th position: radius for the previous doesn't matter in this analysis
                     }
                     if (weighting_col != -1) {
                        // in weighted choice, root node and current node receive values:
                        audittrail[i][r].weighted_choice += (weight * rootweight) * 0.5;
                        audittrail[line.m_connections[k]][r].weighted_choice += (weight * rootweight) * 0.5;
                     }
                  }
                  total_depth += depth;
                  node_count++;
                  depthcounts.tail() += 1;
               }
            }
            if (!choice) 
               foundlist.a().pop_back();
            else
               foundlist.a().remove_at(pos);
            if (!foundlist.a().size()) {
               foundlist.flip();
               depth++;
               depthcounts.push_back(0);
               if (radius[r] != -1 && depth > radius[r]) {
                  break;
               }
            }
         }
         // set the attributes for this node:
         m_attributes.setValue(i,count_col[r],float(node_count));
         if (weighting_col != -1) {
            m_attributes.setValue(i,total_weight_col[r],float(total_weight));
         }
         // node count > 1 to avoid divide by zero (was > 2)
         if (node_count > 1) {
            // note -- node_count includes this one -- mean depth as per p.108 Social Logic of Space
            double mean_depth = double(total_depth) / double(node_count - 1);
            m_attributes.setValue(i,depth_col[r],float(mean_depth));
            if (weighting_col != -1) {
               // weighted mean depth:
               m_attributes.setValue(i,w_depth_col[r],float(w_total_depth/total_weight));
            }
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (node_count > 2 && mean_depth > 1.0) {
               double ra = 2.0 * (mean_depth - 1.0) / double(node_count - 2);
               // d-value / p-value from Depthmap 4 manual, note: node_count includes this one
               double rra_d = ra / dvalue(node_count);
               double rra_p = ra / dvalue(node_count);
               double integ_tk = teklinteg(node_count, total_depth);
               m_attributes.setValue(i,integ_dv_col[r],float(1.0/rra_d));

#ifndef _COMPILE_dX_SIMPLE_VERSION
               if(!simple_version) {
                   m_attributes.setValue(i,integ_pv_col[r],float(1.0/rra_p));
                   if (total_depth - node_count + 1 > 1) {
                       m_attributes.setValue(i,integ_tk_col[r],float(integ_tk));
                   }
                   else {
                       m_attributes.setValue(i,integ_tk_col[r],-1.0f);
                   }
               }
#endif

               if (fulloutput) {
                  m_attributes.setValue(i,ra_col[r],float(ra));

#ifndef _COMPILE_dX_SIMPLE_VERSION
                  if(!simple_version) {
                      m_attributes.setValue(i,rra_col[r],float(rra_d));
                  }
#endif
                  m_attributes.setValue(i,td_col[r],float(total_depth));

#ifndef _COMPILE_dX_SIMPLE_VERSION
                  if(!simple_version) {
                      // alan's palm-tree normalisation: palmtree
                      double dmin = node_count - 1;
                      double dmax = palmtree(node_count, depth - 1);
                      if (dmax != dmin) {
                          m_attributes.setValue(i,penn_norm_col[r],float((dmax - total_depth)/(dmax - dmin)));
                      }
                  }
#endif
               }
            }
            else {
               m_attributes.setValue(i,integ_dv_col[r],-1.0f);

#ifndef _COMPILE_dX_SIMPLE_VERSION
               if(!simple_version) {
                   m_attributes.setValue(i,integ_pv_col[r],-1.0f);
                   m_attributes.setValue(i,integ_tk_col[r],-1.0f);
               }
#endif
               if (fulloutput) {
                  m_attributes.setValue(i,ra_col[r],-1.0f);

#ifndef _COMPILE_dX_SIMPLE_VERSION
                  if(!simple_version) {
                      m_attributes.setValue(i,rra_col[r],-1.0f);
                  }
#endif

                  m_attributes.setValue(i,td_col[r],-1.0f);

#ifndef _COMPILE_dX_SIMPLE_VERSION
                  if(!simple_version) {
                      m_attributes.setValue(i,penn_norm_col[r],-1.0f);
                  }
#endif
               }
            }

#ifndef _COMPILE_dX_SIMPLE_VERSION
            if(!simple_version) {
                double entropy = 0.0, intensity = 0.0, rel_entropy = 0.0, factorial = 1.0, harmonic = 0.0;
                for (size_t k = 0; k < depthcounts.size(); k++) {
                    if (depthcounts[k] != 0) {
                        // some debate over whether or not this should be node count - 1
                        // (i.e., including or not including the node itself)
                        double prob = double(depthcounts[k]) / double(node_count);
                        entropy -= prob * log2( prob );
                        // Formula from Turner 2001, "Depthmap"
                        factorial *= double(k + 1);
                        double q = (pow( mean_depth, double(k) ) / double(factorial)) * exp(-mean_depth);
                        rel_entropy += (double) prob * log2( prob / q );
                        //
                        harmonic += 1.0 / double(depthcounts[k]);
                    }
                }
                harmonic = double(depthcounts.size()) / harmonic;
                if (total_depth > node_count) {
                    intensity = node_count * entropy / (total_depth - node_count);
                }
                else {
                    intensity = -1;
                }
                m_attributes.setValue(i,entropy_col[r],float(entropy));
                m_attributes.setValue(i,rel_entropy_col[r],float(rel_entropy));
                m_attributes.setValue(i,intensity_col[r],float(intensity));
                m_attributes.setValue(i,harmonic_col[r],float(harmonic));
            }
#endif
         }
         else {
            m_attributes.setValue(i,depth_col[r],-1.0f);
            m_attributes.setValue(i,integ_dv_col[r],-1.0f);

#ifndef _COMPILE_dX_SIMPLE_VERSION
            if(!simple_version) {
                m_attributes.setValue(i,integ_pv_col[r],-1.0f);
                m_attributes.setValue(i,integ_tk_col[r],-1.0f);
                m_attributes.setValue(i,entropy_col[r],-1.0f);
                m_attributes.setValue(i,rel_entropy_col[r],-1.0f);
                m_attributes.setValue(i,harmonic_col[r],-1.0f);
            }
#endif
         }

      }
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               delete [] covered;
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, i );
         }         
      }
   }
   delete [] covered;
   if (choice) {
      for (size_t i = 0; i < m_connectors.size(); i++) {
         double total_choice = 0.0, w_total_choice = 0.0;
         for (size_t r = 0; r < radius.size(); r++) {
            total_choice += audittrail[i][r].choice;
            w_total_choice += audittrail[i][r].weighted_choice;
            // n.b., normalise choice according to (n-1)(n-2)/2 (maximum possible through routes)
            double node_count = m_attributes.getValue(i,count_col[r]);
            double total_weight;
            if (weighting_col != -1) {
                total_weight = m_attributes.getValue(i,total_weight_col[r]);
            }
            if (node_count > 2) {
               m_attributes.setValue(i,choice_col[r],float(total_choice));
               m_attributes.setValue(i,n_choice_col[r],float(2.0*total_choice/((node_count-1)*(node_count-2))));
               if (weighting_col != -1) {
                  m_attributes.setValue(i,w_choice_col[r],float(w_total_choice));
                  m_attributes.setValue(i,nw_choice_col[r],float(2.0*w_total_choice/(total_weight*total_weight)));
               }
            }
            else {
               m_attributes.setValue(i,choice_col[r],-1);
               m_attributes.setValue(i,n_choice_col[r],-1);
               if (weighting_col != -1) {
                  m_attributes.setValue(i,w_choice_col[r],-1);
                  m_attributes.setValue(i,nw_choice_col[r],-1);
               }
            }
         }
      }
      for (size_t i = 0; i < m_connectors.size(); i++) {
         delete [] audittrail[i];
      }
      delete [] audittrail;
   }

   m_displayed_attribute = -1; // <- override if it's already showing
   setDisplayedAttribute(integ_dv_col.tail());

   return true;
}

bool ShapeGraph::stepdepth(Communicator *comm)
{
   std::string stepdepth_col_text = std::string("Step Depth");
   int stepdepth_col = m_attributes.insertColumn(stepdepth_col_text.c_str());

   bool *covered = new bool [m_connectors.size()];
   for (size_t i = 0; i < m_connectors.size(); i++) {
      covered[i] = false;
   }
   pflipper<pvecint> foundlist;
   for(auto& lineindex: m_selection_set) {
      foundlist.a().push_back(lineindex);
      covered[lineindex] = true;
      m_attributes.setValue(lineindex,stepdepth_col,0.0f);
   }
   int depth = 1;
   while (foundlist.a().size()) {
      Connector& line = m_connectors[foundlist.a().tail()];
      for (size_t k = 0; k < line.m_connections.size(); k++) {
         if (!covered[line.m_connections[k]]) {
            covered[line.m_connections[k]] = true;
            foundlist.b().push_back(line.m_connections[k]);
            m_attributes.setValue(line.m_connections[k],stepdepth_col,float(depth));
         }
      }
      foundlist.a().pop_back();
      if (!foundlist.a().size()) {
         foundlist.flip();
         depth++;
      }
   }
   delete [] covered;

   m_displayed_attribute = -1; // <- override if it's already showing
   setDisplayedAttribute(stepdepth_col);

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

bool ShapeGraph::read(istream &stream, int version )
{
   m_attributes.clear();
   m_connectors.clear();
   m_selection = false;
   m_map_type = ShapeMap::EMPTYMAP;

   bool segmentmap = false;
   // note that keyvertexcount and keyvertices are different things! (length keyvertices not the same as keyvertexcount!)
   stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
   int size;
   stream.read((char *)&size,sizeof(size));
   for (int i = 0; i < size; i++) {
      m_keyvertices.push_back(pvecint());
      m_keyvertices[i].read(stream);
   }
   // now base class read:
   ShapeMap::read(stream,version);

   return true;
}

bool ShapeGraph::readold( istream& stream, int version )
{
   // read in from old base class
   SpacePixel linemap;
   linemap.read(stream, version);
   const pmap<int,LineTest>& lines = linemap.getAllLines();

   m_name = linemap.getName();

   // now copy to new base class:
   init(lines.size(),linemap.getRegion());
   for (size_t i = 0; i < lines.size(); i++) {
      makeLineShape(lines[i].line);
   }
   // n.b., we now have to reclear attributes!
   m_attributes.clear();

   // continue old read:
   int pushmap = -1;

   char segmentmapc = stream.get();
   if (segmentmapc == '1') {
      m_map_type = ShapeMap::SEGMENTMAP;
   }
   else {
      m_map_type = ShapeMap::AXIALMAP;
   }

   char gatemapc = stream.get();
   if (gatemapc == '1') {
      m_map_type = ShapeMap::DATAMAP;
   }
   stream.read((char *)&pushmap,sizeof(pushmap));


   int displayed_attribute;  // n.b., temp variable necessary to force recalc below
   stream.read((char *)&displayed_attribute,sizeof(displayed_attribute));

   m_attributes.read(stream,version);
   int size;
   stream.read((char *)&size,sizeof(size));
   for (int j = 0; j < size; j++) {
      m_keyvertices.push_back(pvecint());    // <- these were stored with the connector
      int key;
      stream.read((char *)&key,sizeof(key)); // <- key deprecated
      m_connectors.push_back(Connector());
      m_connectors[j].read(stream,version,&(m_keyvertices[j]));
   }
   stream.read((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));


   m_links.read(stream);
   m_unlinks.read(stream);

   // some miscellaneous extra data for mapinfo files
   if (m_mapinfodata) {
      delete m_mapinfodata;
      m_mapinfodata = NULL;
   }

   char x = stream.get();
   if (x == 'm') {
      m_mapinfodata = new MapInfoData;
      m_mapinfodata->read(stream,version);
   }


   // now, as soon as loaded, must recalculate our screen display:
   // note m_displayed_attribute should be -2 in order to force recalc...
   m_displayed_attribute = -2;
   setDisplayedAttribute(displayed_attribute);

   return true;
}

bool ShapeGraph::write( ofstream& stream, int version )
{
   // note keyvertexcount and keyvertices are different things!  (length keyvertices not the same as keyvertexcount!)
   stream.write((char *)&m_keyvertexcount,sizeof(m_keyvertexcount));
   int size = m_keyvertices.size();
   stream.write((char *)&size,sizeof(size));
   for (size_t i = 0; i < m_keyvertices.size(); i++) {
      m_keyvertices[i].write(stream);
   }

   // now simply run base class write:
   ShapeMap::write(stream,version);

   return true;
}

void ShapeGraph::writeAxialConnectionsAsDotGraph(ostream &stream)
{
    const prefvec<Connector>& connectors = ShapeMap::getConnections();

    stream << "strict graph {" << std::endl;

    stream.precision(12);

    for (size_t i = 0; i < connectors.size(); i++) {
        pvecint connections = connectors[i].m_connections;
        for (size_t j = 0; j < connections.size(); j++) {
            stream << "    " << i << " -- " << connections[j] << std::endl;
        }
    }
    stream << "}" << std::endl;
}

void ShapeGraph::writeAxialConnectionsAsPairsCSV(ostream &stream)
{
    const prefvec<Connector>& connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB" << std::endl;

    for (size_t i = 0; i < connectors.size(); i++) {
        pvecint connections = connectors[i].m_connections;
        if (i != 0) stream << std::endl;
        for (size_t j = 0; j < connections.size(); j++) {
            if (j != 0) stream << std::endl;
            stream << i << "," << connections[j];
        }
    }
}

void ShapeGraph::writeSegmentConnectionsAsPairsCSV(ostream &stream)
{
    const prefvec<Connector>& connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB,ss_weight,for_back,dir";

    // directed links
    for (size_t i = 0; i < connectors.size(); i++) {
        size_t cur_size = connectors[i].m_forward_segconns.size();
        for (size_t j = 0; j < cur_size; j++) {
            stream << std::endl;
            stream << i << "," << connectors[i].m_forward_segconns.key(j).ref
                   << "," << connectors[i].m_forward_segconns.value(j)
                   << "," << 0 // forward
                   << "," << int(connectors[i].m_forward_segconns.key(j).dir);
        }

        cur_size = connectors[i].m_back_segconns.size();
        for (size_t j = 0; j < cur_size; j++) {
            stream << std::endl;
            stream << i << "," << connectors[i].m_back_segconns.key(j).ref
                   << "," << connectors[i].m_back_segconns.value(j)
                   << "," << 1 // back
                   << "," << int(connectors[i].m_back_segconns.key(j).dir);
        }
    }
}
////////////////////////////////////////////////////////////////////////////

// this unlink options was originally excised on the version 7 recode
// however, it is *very specific* to axial maps, and so have been reincluded here

void ShapeGraph::unlinkFromShapeMap(const ShapeMap& shapemap)
{
   // used to make a shape map from every axial intersection,

   // find lines in rough vincinity of unlink point, and check for the closest
   // pair to unlink:

   const std::map<int,SalaShape>& polygons = shapemap.getAllShapes();
   for (auto polygon: polygons) {
      // just use the points:
      if (polygon.second.isPoint()) {
         pqvector<Point2f> closepoints;
         prefvec<IntPair> intersections;
         PixelRef pix = pixelate(polygon.second.getPoint());
         pqvector<ShapeRef>& pix_shapes = m_pixel_shapes[pix.x][pix.y];
         size_t j;
         for (j = 0; j < pix_shapes.size(); j++) {
            for (size_t k = j + 1; k < pix_shapes.size(); k++) {
               int a = depthmapX::findIndexFromKey(m_shapes, (int) pix_shapes[j].m_shape_ref);
               int b = depthmapX::findIndexFromKey(m_shapes, (int) pix_shapes[k].m_shape_ref);
               if (a != -1 && b != -1 && m_shapes.find(a)->second.isLine() && m_shapes.find(b)->second.isLine() && m_connectors[a].m_connections.searchindex(b) != -1) {
                  closepoints.push_back( intersection_point(m_shapes.find(a)->second.getLine(), m_shapes.find(b)->second.getLine(), TOLERANCE_A) );
                  intersections.push_back( IntPair((int)a,(int)b) );
               }
            }
         }
         double mindist = -1.0;
         int minpair = -1;
         for (j = 0; j < closepoints.size(); j++) {
            if (minpair == -1 || dist(polygon.second.getPoint(),closepoints[j]) < mindist) {
               mindist = dist(polygon.second.getPoint(),closepoints[j]);
               minpair = j;
            }
         }
         if (minpair != -1) {
            unlinkShapes(intersections[minpair].a, intersections[minpair].b, false);
         }
         else {
            cerr << "eek!";
         }
      }
   }

   // reset displayed attribute if it happens to be "Connectivity":
   int conn_col = m_attributes.getColumnIndex("Connectivity");
   if (getDisplayedAttribute() == conn_col) {
      invalidateDisplayedAttribute();
      setDisplayedAttribute(conn_col);  // <- reflect changes to connectivity counts
   }
}



///////////////////////////////////////////////////////////////////////////////

// Two ways to make a segment map

// Method 1: direct linkage of endpoints where they touch

void ShapeGraph::makeNewSegMap()
{
   // now make a connection set from the ends of lines:
   prefvec<Connector> connectionset;
   pmap<int,Line> lineset;
   for (auto shape: m_shapes) {
      if (shape.second.isLine()) {
         connectionset.push_back(Connector());
         lineset.add(shape.first,shape.second.getLine());
      }
   }

   double maxdim = __max(m_region.width(),m_region.height());

   for (size_t seg_a = 0; seg_a < lineset.size(); seg_a++) {
      // n.b., vector() is based on t_start and t_end, so we must use t_start and t_end here and throughout
      PixelRef pix1 = pixelate(lineset[seg_a].t_start());
      pqvector<ShapeRef> &shapes1 = m_pixel_shapes[pix1.x][pix1.y];
      for (size_t j1 = 0; j1 < shapes1.size(); j1++) {
         size_t seg_b = lineset.searchindex(shapes1[j1].m_shape_ref);
         if (seg_b != paftl::npos && seg_a < seg_b) {
            Point2f alpha = lineset[seg_a].vector();
            Point2f beta  = lineset[seg_b].vector();
            alpha.normalise();
            beta.normalise();
            if (approxeq(lineset[seg_a].t_start(),lineset[seg_b].t_start(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
               connectionset[seg_a].m_back_segconns.add(SegmentRef(1,seg_b),x);
               connectionset[seg_b].m_back_segconns.add(SegmentRef(1,seg_a),x);
            }
            if (approxeq(lineset[seg_a].t_start(),lineset[seg_b].t_end(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(alpha,-beta),-1.0),1.0)) / M_PI);
               connectionset[seg_a].m_back_segconns.add(SegmentRef(-1,seg_b),x);
               connectionset[seg_b].m_forward_segconns.add(SegmentRef(1,seg_a),x);
            }
         }
      }
      PixelRef pix2 = pixelate(m_shapes.find(seg_a)->second.getLine().t_end());
      pqvector<ShapeRef> &shapes2 = m_pixel_shapes[pix2.x][pix2.y];
      for (size_t j2 = 0; j2 < shapes2.size(); j2++) {
         size_t seg_b = lineset.searchindex(shapes2[j2].m_shape_ref);
         if (seg_b != paftl::npos && seg_a < seg_b) {
            Point2f alpha = lineset[seg_a].vector();
            Point2f beta  = lineset[seg_b].vector();
            alpha.normalise();
            beta.normalise();
            if (approxeq(lineset[seg_a].t_end(),lineset[seg_b].t_start(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(-alpha,beta),-1.0),1.0)) / M_PI);
               connectionset[seg_a].m_forward_segconns.add(SegmentRef(1,seg_b),x);
               connectionset[seg_b].m_back_segconns.add(SegmentRef(-1,seg_a),x);
            }
            if (approxeq(lineset[seg_a].t_end(),lineset[seg_b].t_end(),(maxdim*TOLERANCE_B))) {
               float x = float(2.0 * acos(__min(__max(-dot(-alpha,-beta),-1.0),1.0)) / M_PI);
               connectionset[seg_a].m_forward_segconns.add(SegmentRef(-1,seg_b),x);
               connectionset[seg_b].m_forward_segconns.add(SegmentRef(-1,seg_a),x);
            }
         }
      }
   }

   // initialise attributes now separated from making the connections
   initSegmentAttributes(connectionset);
   makeSegmentConnections(connectionset);
}

// Method 2: Making a segment map (in two stages)

// One: take the original axial map and split it up 
// (note: you need to start from an axial map,
//  but the map could have been created from a road-centre-line
//  graph or equivalent -- reason is that you might want to 
//  preserve unlinks in your angular mapping)

// A "linetest" is used in order to use the test component to
// identify the original axial line this line segment is 
// associated with

void ShapeGraph::makeSegmentMap(prefvec<Line>& lineset, prefvec<Connector>& connectionset, double stubremoval)
{
   // the first (key) pair is the line / line intersection, second is the pair of associated segments for the first line
   pqmap<OrderedIntPair,IntPair> segmentlist;

   // this code relies on the polygon order being the same as the connections

   for (size_t i = 0; i < m_connectors.size(); i++) {
      if (!m_shapes.find(i)->second.isLine()) {
         continue;
      }
      const Line& line = m_shapes.find(i)->second.getLine();
      pmap<double,int> breaks;
      int axis = line.width() >= line.height() ? XAXIS : YAXIS;
      // we need the breaks ordered from start to end of the line
      // this is automatic for XAXIS, but on YAXIS, need to know
      // if the line is ascending or decending
      int parity = (axis == XAXIS) ? 1 : line.sign();

      pvecint& connections = m_connectors[i].m_connections;
      for (size_t j = 0; j < connections.size(); j++) {
         // find the intersection point and add...
         // note: more than one break at the same place allowed
         if (i != connections[j] && m_shapes.find(connections[j])->second.isLine()) {
            breaks.add( parity * line.intersection_point( m_shapes.find(connections[j])->second.getLine(), axis, TOLERANCE_A ), connections[j], paftl::ADD_DUPLICATE );
         }
      }
      // okay, now we have a list from one end of the other of lines this line connects with
      Point2f lastpoint = line.start();
      int seg_a = -1, seg_b = -1;
      double neardist;
      // TOLERANCE_C is introduced as of 01.08.2008 although it is a fix to a bug first 
      // found in July 2006.  It has been set "high" deliberately (1e-6 = a millionth of the line height / width)
      // in order to catch small errors made by operators or floating point errors in other systems
      // when drawing, for example, three axial lines intersecting
      if  (stubremoval == 0.0) {
         // if 0, convert to tolerance
         stubremoval = TOLERANCE_C;
      }
      neardist = (axis == XAXIS) ? (line.width() * stubremoval) : (line.height() * stubremoval);
      double overlapdist = (axis == XAXIS) ? (line.width() * TOLERANCE_C) : (line.height() * TOLERANCE_C);
      //
      for (size_t k = 0; k < breaks.size(); ) {
         pvecint keylist;
         if (seg_a == -1) {
            Point2f thispoint = line.point_on_line(parity * breaks.key(k),axis);
            if (fabs(parity * breaks.key(k) - line.start()[axis]) < neardist) {
               seg_a = -1;
               lastpoint = thispoint;
            }
            else  {
               Line segment_a(line.start(),thispoint);
               lineset.push_back(segment_a);
               connectionset.push_back(Connector(i));
               seg_a = lineset.size() - 1;
            }
            lastpoint = thispoint;
         }
         //
         double here = parity * breaks.key(k);
         while (k < breaks.size() && fabs(parity * breaks.key(k) - here) < overlapdist) {
            keylist.push_back(breaks.value(k));
            k++;
         }
         //
         if (k == breaks.size() && fabs(line.end()[axis] - parity * breaks.key(k-1)) < neardist) {
            seg_b = -1;
         }
         else {
            Point2f thispoint;
            if (k < breaks.size()) {
               thispoint = line.point_on_line(parity * breaks.key(k),axis);
            }
            else {
               thispoint = line.end();
            }
            Line segment_b(lastpoint,thispoint);
            lineset.push_back(segment_b);
            connectionset.push_back(Connector(i));
            seg_b = lineset.size() - 1;
            //
            lastpoint = thispoint;
         }
         //
         for (size_t j = 0; j < keylist.size(); j++) {
         //
            if (keylist[j] < (int)i) {
               // other line already segmented, look up in segment list,
               // and join segments together nicely
               size_t index = segmentlist.searchindex(OrderedIntPair(keylist[j],i));
               if (index != paftl::npos) {   // <- if it isn't -1 something has gone badly wrong!
                  int seg_1 = segmentlist[index].a;
                  int seg_2 = segmentlist[index].b;
                  if (seg_a != -1) {
                     if (seg_1 != -1) {
                        Point2f alpha = lineset[seg_a].start() - lineset[seg_a].end();
                        Point2f beta  = lineset[seg_1].start() - lineset[seg_1].end();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        connectionset[seg_a].m_forward_segconns.add(SegmentRef(-1,seg_1),x);
                        connectionset[seg_1].m_forward_segconns.add(SegmentRef(-1,seg_a),x);
                     }
                     if (seg_2 != -1) {
                        Point2f alpha = lineset[seg_a].start() - lineset[seg_a].end();
                        Point2f beta  = lineset[seg_2].end() - lineset[seg_2].start();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        connectionset[seg_a].m_forward_segconns.add(SegmentRef(1,seg_2),x);
                        connectionset[seg_2].m_back_segconns.add(SegmentRef(-1,seg_a),x);
                     }
                  }
                  if (seg_b != -1) {
                     if (seg_1 != -1) {
                        Point2f alpha = lineset[seg_b].end() - lineset[seg_b].start();
                        Point2f beta  = lineset[seg_1].start() - lineset[seg_1].end();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        connectionset[seg_b].m_back_segconns.add(SegmentRef(-1,seg_1),x);
                        connectionset[seg_1].m_forward_segconns.add(SegmentRef(1,seg_b),x);
                     }
                     if (seg_2 != -1) {
                        Point2f alpha = lineset[seg_b].end() - lineset[seg_b].start();
                        Point2f beta  = lineset[seg_2].end() - lineset[seg_2].start();
                        alpha.normalise();
                        beta.normalise();
                        float x = float(2.0 * acos(__min(__max(-dot(alpha,beta),-1.0),1.0)) / M_PI);
                        connectionset[seg_b].m_back_segconns.add(SegmentRef(1,seg_2),x);
                        connectionset[seg_2].m_back_segconns.add(SegmentRef(1,seg_b),x);
                     }
                  }
               }
            }
            else {
               // other line still to be segmented, add ourselves to segment list
               // to be added later
               segmentlist.add( OrderedIntPair(i,keylist[j]), IntPair(seg_a,seg_b) );
            }
         }
         if (seg_a != -1 && seg_b != -1) {
            connectionset[seg_a].m_forward_segconns.add(SegmentRef(1,seg_b),0.0f);
            connectionset[seg_b].m_back_segconns.add(SegmentRef(-1,seg_a),0.0f);
         }
         seg_a = seg_b;
      }
   }
}

// now segments and connections are listed separately...
// put them together in a new map

void ShapeGraph::initSegmentAttributes(prefvec<Connector>& connectionset)
{
   m_attributes.clear();

   // note, expects these in alphabetical order to preserve numbering:
   int ref_col = m_attributes.insertLockedColumn("Axial Line Ref");
   int leng_col = m_attributes.insertLockedColumn("Segment Length");

   int i = -1;
   for (auto shape: m_shapes) {
       i++;
      int key = shape.first;
      int rowid = m_attributes.insertRow(key);
      //
      m_attributes.setValue(rowid, ref_col, (float) connectionset[i].m_segment_axialref );
      m_attributes.setValue(rowid, leng_col, (float) shape.second.getLine().length() );
   }
}

void ShapeGraph::makeSegmentConnections(prefvec<Connector>& connectionset)
{
   m_connectors.clear();

   // note, expects these in alphabetical order to preserve numbering:
   int w_conn_col = m_attributes.insertColumn("Angular Connectivity");
   int uw_conn_col = m_attributes.insertLockedColumn("Connectivity");

   for (size_t i = 0; i < m_shapes.size(); i++) {
      // all indices should match... (including lineset/connectionset versus m_shapes)
      m_connectors.push_back( connectionset[i] );
      float total_weight = 0.0f;
      for (size_t j = 0; j < connectionset[i].m_forward_segconns.size(); j++) {
         total_weight += connectionset[i].m_forward_segconns.value(j);
      }
      for (size_t k = 0; k < connectionset[i].m_back_segconns.size(); k++) {
         total_weight += connectionset[i].m_back_segconns.value(k);
      }
      m_attributes.setValue(i, w_conn_col, (float) total_weight );
      m_attributes.setValue(i, uw_conn_col, (float) (connectionset[i].m_forward_segconns.size() + connectionset[i].m_back_segconns.size()));

      // free up connectionset as we go along:
      connectionset.free_at(i);
   }

   m_displayed_attribute = -2; // <- override if it's already showing
   setDisplayedAttribute(uw_conn_col);
}

// this pushes axial map values to a segment map
// the segment map is 'this', the axial map is passed:

void ShapeGraph::pushAxialValues(ShapeGraph& axialmap)
{
   if (m_attributes.getColumnIndex("Axial Line Ref") == -1) {
      // this should never happen
      // AT: I am converting this to throw an error
      throw MetaGraph::Error("Axial line ref does not exist");
   }

   pvecint colindices;
   for (int i = 0; i < axialmap.m_attributes.getColumnCount(); i++) {
      std::string colname = std::string("Axial ") + axialmap.m_attributes.getColumnName(i);
      colindices.push_back(m_attributes.insertColumn(colname));
   }
   for (int j = 0; j < m_attributes.getRowCount(); j++) {
      int axialref = (int) m_attributes.getValue(j,"Axial Line Ref");
      for (int k = 0; k < axialmap.m_attributes.getColumnCount(); k++) {
         float val = axialmap.m_attributes.getValue(axialref,k);
         // need to look up the column index:
         m_attributes.setValue(j,colindices[k],val);
      }
   }
}

bool ShapeGraph::analyseAngular(Communicator *comm, const pvecdouble& radius_list)
{
   if (m_map_type != ShapeMap::SEGMENTMAP) {
      return false;
   }

   // Quick mod - TV
#if defined(_WIN32)
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_connectors.size() );
   }

   // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
   // ...to ensure no mess ups, we'll re-sort here:
   bool radius_n = false;
   pvecdouble radius;
   for (size_t i = 0; i < radius_list.size(); i++) {
      if (radius_list[i] == -1.0) {
         radius_n = true;
      }
      else {
         radius.add(radius_list[i]);
      }
   }
   if (radius_n) {
      radius.push_back(-1.0);
   }

   pvecint depth_col, count_col, total_col;
   // first enter table values
   size_t r;
   for (r = 0; r < radius.size(); r++) {
      std::string radius_text = makeRadiusText(Options::RADIUS_ANGULAR,radius[r]);
      std::string depth_col_text = std::string("Angular Mean Depth") + radius_text;
      m_attributes.insertColumn(depth_col_text.c_str());
      std::string count_col_text = std::string("Angular Node Count") + radius_text;
      m_attributes.insertColumn(count_col_text.c_str());
      std::string total_col_text = std::string("Angular Total Depth") + radius_text;
      m_attributes.insertColumn(total_col_text.c_str());
   }
   
   for (r = 0; r < radius.size(); r++) {
      std::string radius_text = makeRadiusText(Options::RADIUS_ANGULAR,radius[r]);
      std::string depth_col_text = std::string("Angular Mean Depth") + radius_text;
      depth_col.push_back(m_attributes.getColumnIndex(depth_col_text.c_str()));
      std::string count_col_text = std::string("Angular Node Count") + radius_text;
      count_col.push_back(m_attributes.getColumnIndex(count_col_text.c_str()));
      std::string total_col_text = std::string("Angular Total Depth") + radius_text;
      total_col.push_back(m_attributes.getColumnIndex(total_col_text.c_str()));
   }


   bool *covered = new bool [m_connectors.size()];
   for (size_t i = 0; i < m_connectors.size(); i++) {
      for (size_t j = 0; j < m_connectors.size(); j++) {
         covered[j] = false;
      }
      pmap<float,SegmentData> anglebins;
      anglebins.add(0.0f,SegmentData(0,i,SegmentRef(),0,0.0,0));
      Connector& thisline = m_connectors[i];
      pvecdouble total_depth;
      pvecint node_count;
      size_t r;
      for (r = 0; r < radius.size(); r++) {
         total_depth.push_back(0.0);
         node_count.push_back(0);
      }
      // node_count includes this one, but will be added in next algo:
      while (anglebins.size()) {
         SegmentData lineindex = anglebins.value(0);
         if (!covered[lineindex.ref]) {
            covered[lineindex.ref] = true;
            double depth_to_line = anglebins.key(0);
            total_depth[lineindex.coverage] += depth_to_line;
            node_count[lineindex.coverage] += 1;
            anglebins.remove_at(0);
            Connector& line = m_connectors[lineindex.ref];
            if (lineindex.dir != -1) {
               for (size_t k = 0; k < line.m_forward_segconns.size(); k++) {
                  if (!covered[line.m_forward_segconns.key(k).ref]) {
                     double angle = depth_to_line + line.m_forward_segconns.value(k);
                     int rbin = lineindex.coverage;
                     while (rbin != radius.size() && radius[rbin] != -1 && angle > radius[rbin]) {
                        rbin++;
                     }
                     if (rbin != radius.size()) {
                        anglebins.add(angle, SegmentData(line.m_forward_segconns.key(k),SegmentRef(),0,0.0,rbin), paftl::ADD_DUPLICATE);
                     }
                  }
               }
            }
            if (lineindex.dir != 1) {
               for (size_t k = 0; k < line.m_back_segconns.size(); k++) {
                  if (!covered[line.m_back_segconns.key(k).ref]) {
                     double angle = depth_to_line + line.m_back_segconns.value(k);
                     int rbin = lineindex.coverage;
                     while (rbin != radius.size() && radius[rbin] != -1 && angle > radius[rbin]) {
                        rbin++;
                     }
                     if (rbin != radius.size()) {
                        anglebins.add(angle, SegmentData(line.m_back_segconns.key(k),SegmentRef(),0,0.0,rbin), paftl::ADD_DUPLICATE);
                     }
                  }
               }
            }
         }
         else {
            anglebins.remove_at(0);
         }
      }
      // set the attributes for this node:
      int curs_node_count = 0;
      double curs_total_depth = 0.0;
      for (r = 0; r < radius.size(); r++) {
         curs_node_count += node_count[r];
         curs_total_depth += total_depth[r];
         m_attributes.setValue(i,count_col[r],float(curs_node_count));
         if (curs_node_count > 1) {
            // note -- node_count includes this one -- mean depth as per p.108 Social Logic of Space
            double mean_depth = curs_total_depth / double(curs_node_count - 1);
            m_attributes.setValue(i,depth_col[r],float(mean_depth));
            m_attributes.setValue(i,total_col[r],float(curs_total_depth));
         }
         else {
            m_attributes.setValue(i,depth_col[r],-1);
            m_attributes.setValue(i,total_col[r],-1);
         }
      }
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               delete [] covered;
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, i );
         }         
      }
   }
   delete [] covered;

   m_displayed_attribute = -2; // <- override if it's already showing
   setDisplayedAttribute(depth_col.tail());

   return true;
}

// extra parameters for selection_only and interactive are for parallel process extensions
int ShapeGraph::analyseTulip(Communicator *comm, int tulip_bins, bool choice, int radius_type, const pvecdouble& radius_list, int weighting_col, int weighting_col2, int routeweight_col, bool selection_only, bool interactive)
{
   int processed_rows = 0;

   if (m_map_type != ShapeMap::SEGMENTMAP) {
      return processed_rows;
   }

   // Quick mod - TV
#if defined(_WIN32)
   __time64_t atime = 0;
#else
   time_t atime = 0;
#endif

   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, (selection_only ? m_selection_set.size() : m_connectors.size()) );
   }

   // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
   // ...to ensure no mess ups, we'll re-sort here:
   bool radius_n = false;
   pvecdouble radius_unconverted;
   for (size_t i = 0; i < radius_list.size(); i++) {
      if (radius_list[i] == -1.0) {
         radius_n = true;
      }
      else {
         radius_unconverted.add(radius_list[i]);
      }
   }
   if (radius_n) {
      radius_unconverted.push_back(-1.0);
   }

   // retrieve weighted col data, as this may well be overwritten in the new analysis:
   pvecfloat weights;
   pvecfloat routeweights;  //EF
   std::string weighting_col_text;

   if (weighting_col != -1) {
      weighting_col_text = m_attributes.getColumnName(weighting_col);
      for (size_t i = 0; i < m_connectors.size(); i++) {
         weights.push_back(m_attributes.getValue(i,weighting_col));
      }
   }
   else { // Normal run // TV
      for (size_t i = 0; i < m_connectors.size(); i++) {
         weights.push_back(1.0f);
      }
   }
   //EF routeweight*
   std::string routeweight_col_text;
   if (routeweight_col != -1) {
	   //we normalise the column values between 0 and 1 and reverse it so that high values can be treated as a 'low cost' - similar to the angular cost
	  double max_value = m_attributes.getMaxValue(routeweight_col);
     routeweight_col_text = m_attributes.getColumnName(routeweight_col);
	  for (size_t i = 0; i < m_connectors.size(); i++) {
		 routeweights.push_back(1.0-(m_attributes.getValue(i, routeweight_col)/max_value)); //scale and revert!
      }
   }
   else { // Normal run // TV
      for (size_t i = 0; i < m_connectors.size(); i++) {
         routeweights.push_back(1.0f);
      }
   }
   //*EF routeweight

   //EFEF*
   //for origin-destination weighting
   pvecfloat weights2;
   std::string weighting_col_text2;
   if (weighting_col2 != -1) {
      weighting_col_text2 = m_attributes.getColumnName(weighting_col2);
      for (size_t i = 0; i < m_connectors.size(); i++) {
         weights2.push_back(m_attributes.getValue(i,weighting_col2));
      }
   }
   else { // Normal run // TV
      for (size_t i = 0; i < m_connectors.size(); i++) {
         weights2.push_back(1.0f);
      }
   }
   //*EFEF

   std::string tulip_text = std::string("T") + dXstring::formatString(tulip_bins,"%d");

   // first enter the required attribute columns:
   size_t r;
   for (r = 0; r < radius_unconverted.size(); r++) {
      std::string radius_text = makeRadiusText(radius_type, radius_unconverted[r]);
      int choice_col = -1, n_choice_col = -1, w_choice_col = -1, nw_choice_col = -1;
      if (choice) {	
			//EF routeweight *
			if (routeweight_col != -1) {
                std::string choice_col_text = tulip_text + " Choice [Route weight by " + routeweight_col_text + "]"+ radius_text;
				m_attributes.insertColumn(choice_col_text.c_str());
				if (weighting_col != -1) {					
                    std::string w_choice_col_text = tulip_text + " Choice [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;
					
					m_attributes.insertColumn(w_choice_col_text.c_str());
				}
				//EFEF*
				if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]]" + radius_text;
					
					m_attributes.insertColumn(w_choice_col_text2.c_str());
				}
				//*EFEF
			}
			//*EF routeweight
            else { // Normal run // TV
                std::string choice_col_text = tulip_text + " Choice" + radius_text;
				m_attributes.insertColumn(choice_col_text.c_str());
				if (weighting_col != -1) {
                    std::string w_choice_col_text = tulip_text + " Choice [" + weighting_col_text + " Wgt]" + radius_text;
					m_attributes.insertColumn(w_choice_col_text.c_str());
				}
				//EFEF*
				if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]" + radius_text;
					m_attributes.insertColumn(w_choice_col_text2.c_str());
				}
				//*EFEF
			}
      }

		//EF routeweight *
		if (routeweight_col != -1) {
         std::string integ_col_text = tulip_text + " Integration [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;

         std::string count_col_text = tulip_text + " Node Count [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
			// '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            std::string w_td_text = tulip_text + " Total Depth [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;
            std::string total_weight_text = tulip_text + " Total " + weighting_col_text + " [Route weight by " + routeweight_col_text + "]" +radius_text;

         m_attributes.insertColumn(integ_col_text.c_str());
			m_attributes.insertColumn(count_col_text.c_str());
			m_attributes.insertColumn(td_col_text.c_str());
			if (weighting_col != -1) {
            m_attributes.insertColumn(w_integ_col_text.c_str());
				m_attributes.insertColumn(w_td_text.c_str());         
				m_attributes.insertColumn(total_weight_text.c_str());
			}
		}
		//*EF routeweight
        else { // Normal run // TV
            std::string integ_col_text = tulip_text + " Integration" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [" + weighting_col_text + " Wgt]" + radius_text;

            std::string count_col_text = tulip_text + " Node Count" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth" + radius_text; // <- note, the fact this is a tulip is unnecessary
			// '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            std::string w_td_text = tulip_text + " Total Depth [" + weighting_col_text + " Wgt]" + radius_text;
            std::string total_weight_text = tulip_text + " Total " + weighting_col_text + radius_text;

         m_attributes.insertColumn(integ_col_text.c_str());
			m_attributes.insertColumn(count_col_text.c_str());
			m_attributes.insertColumn(td_col_text.c_str());
			if (weighting_col != -1) {         
            m_attributes.insertColumn(w_integ_col_text.c_str());
				m_attributes.insertColumn(w_td_text.c_str());         
				m_attributes.insertColumn(total_weight_text.c_str());
			}
		}
   }
   pvecint choice_col, w_choice_col, w_choice_col2, count_col, integ_col, w_integ_col, td_col, w_td_col, total_weight_col;
   // then look them up! eek....
   for (r = 0; r < radius_unconverted.size(); r++) {
      std::string radius_text = makeRadiusText(radius_type, radius_unconverted[r]);
      if (choice) {
			//EF routeweight *
			if (routeweight_col != -1) {
                std::string choice_col_text = tulip_text + " Choice [Route weight by " + routeweight_col_text + "]"+ radius_text;
				choice_col.push_back(m_attributes.getColumnIndex(choice_col_text.c_str()));
				if (weighting_col != -1) {
                    std::string w_choice_col_text = tulip_text + " Choice [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;
					w_choice_col.push_back(m_attributes.getColumnIndex(w_choice_col_text.c_str()));
				}
				//EFEF*
				if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]]" + radius_text;
					w_choice_col2.push_back(m_attributes.getColumnIndex(w_choice_col_text2.c_str()));
				}
				//*EFEF
			}
			//* EF routeweight
            else { // Normal run // TV
                std::string choice_col_text = tulip_text + " Choice" + radius_text;
				choice_col.push_back(m_attributes.getColumnIndex(choice_col_text.c_str()));
				if (weighting_col != -1) {
                    std::string w_choice_col_text = tulip_text + " Choice [" + weighting_col_text + " Wgt]" + radius_text;
					w_choice_col.push_back(m_attributes.getColumnIndex(w_choice_col_text.c_str()));
				}
				//EFEF*
				if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]" + radius_text;
					w_choice_col2.push_back(m_attributes.getColumnIndex(w_choice_col_text2.c_str()));
				}
				//*EFEF
			}
		 
      }
		//EF routeweight *
		if (routeweight_col != -1) {
         std::string integ_col_text = tulip_text + " Integration [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;

         std::string count_col_text = tulip_text + " Node Count [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth [Route weight by " + routeweight_col_text + "]"+ radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_td_text = tulip_text + " Total Depth [[Route weight by " + routeweight_col_text + "][" + weighting_col_text + " Wgt]]" + radius_text;
            std::string total_weight_col_text = tulip_text + " Total " + weighting_col_text + " [Route weight by " + routeweight_col_text + "]" +radius_text;

         integ_col.push_back(m_attributes.getColumnIndex(integ_col_text.c_str()));
			count_col.push_back(m_attributes.getColumnIndex(count_col_text.c_str()));
			td_col.push_back(m_attributes.getColumnIndex(td_col_text.c_str()));      
			if (weighting_col != -1) {
				// '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            w_integ_col.push_back(m_attributes.getColumnIndex(w_integ_col_text.c_str()));
				w_td_col.push_back(m_attributes.getColumnIndex(w_td_text.c_str()));
				total_weight_col.push_back(m_attributes.getColumnIndex(total_weight_col_text.c_str()));
			}
		}
		//* EF routeweight
        else { // Normal run // TV
            std::string integ_col_text = tulip_text + " Integration" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [" + weighting_col_text + " Wgt]" + radius_text;

         std::string count_col_text = tulip_text + " Node Count" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_td_text = tulip_text + " Total Depth [" + weighting_col_text + " Wgt]" + radius_text;
            std::string total_weight_col_text = tulip_text + " Total " + weighting_col_text + radius_text;
      
         integ_col.push_back(m_attributes.getColumnIndex(integ_col_text.c_str()));
			count_col.push_back(m_attributes.getColumnIndex(count_col_text.c_str()));
			td_col.push_back(m_attributes.getColumnIndex(td_col_text.c_str()));      
			if (weighting_col != -1) {
				// '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            w_integ_col.push_back(m_attributes.getColumnIndex(w_integ_col_text.c_str()));
				w_td_col.push_back(m_attributes.getColumnIndex(w_td_text.c_str()));
				total_weight_col.push_back(m_attributes.getColumnIndex(total_weight_col_text.c_str()));
			}
		}
   }

   tulip_bins /= 2;  // <- actually use semicircle of tulip bins
   tulip_bins += 1;

   pqvector<SegmentData> *bins = new pqvector<SegmentData>[tulip_bins];

   AnalysisInfo ***audittrail;
   unsigned int **uncovered;
   audittrail = new AnalysisInfo **[m_connectors.size()];
   uncovered = new unsigned int *[m_connectors.size()];
   for (size_t i = 0; i < m_connectors.size(); i++) {
      audittrail[i] = new AnalysisInfo *[radius_unconverted.size()];
      for (size_t j = 0; j < radius_unconverted.size(); j++) {
         audittrail[i][j] = new AnalysisInfo[2];
      }
      uncovered[i] = new unsigned int [2];

   }
   pvecdouble radius;
   for (r = 0; r < radius_unconverted.size(); r++) {
      if (radius_type == Options::RADIUS_ANGULAR && radius_unconverted[r] != -1) {
         radius.push_back(floor(radius_unconverted[r] * tulip_bins * 0.5));
      }
      else {
         radius.push_back(radius_unconverted[r]);
      }
   }
   // entered once for each segment
   int length_col = m_attributes.getColumnIndex("Segment Length");
   pvecfloat lengths;
   if (length_col != -1) {
      for (size_t i = 0; i < m_connectors.size(); i++) {
         lengths.push_back(m_attributes.getValue(i,length_col));
      }
   }

   int radiussize = radius.size();
   int radiusmask = 0;
   for (int i = 0; i < radiussize; i++) {
      radiusmask |= (1 << i);
   }

   for (size_t rowid = 0; rowid < m_connectors.size(); rowid++) {

      if (selection_only) {
         // could use m_selection_set.searchindex(rowid) to find 
         // if this row is selected as m_selection_set is ordered for axial and segment maps, etc
         // BUT, actually quicker to check the tag in the attributes that shows it's selected
         if (!m_attributes.isSelected(rowid)) {
            continue;
         }
      }

      for (int k = 0; k < tulip_bins; k++) {
         bins[k].clear();
      }
      for (size_t j = 0; j < m_connectors.size(); j++) {
         for (int dir = 0; dir < 2; dir++) {
            for (int k = 0; k < radiussize; k++) {
               audittrail[j][k][dir].clearLine();
            }
            uncovered[j][dir] = radiusmask;
         }
      }

      double rootseglength = m_attributes.getValue(rowid,length_col);
      double rootweight = (weighting_col != -1) ? weights[rowid] : 0.0;
		//EFEF
		double rootweight2 = (weighting_col2 != -1) ? weights2[rowid] : 0.0;
		//EFEF

      // setup: direction 0 (both ways), segment i, previous -1, segdepth (step depth) 0, metricdepth 0.5 * rootseglength, bin 0
      bins[0].add(SegmentData(0,rowid,SegmentRef(),0,0.5*rootseglength,radiusmask));
      // this version below is only designed to be used temporarily --
      // could be on an option?
      //bins[0].push_back(SegmentData(0,rowid,SegmentRef(),0,0.0,radiusmask));
      Connector& thisline = m_connectors[rowid];
      pvecint node_count;
      double weight = 0.0;
      int depthlevel = 0;
      int opencount = 1;
      int currentbin = 0;
      while (opencount) {
         while (!bins[currentbin].size()) {
            depthlevel++;
            currentbin++;
            if (currentbin == tulip_bins) {
               currentbin = 0;
            }
         }
         SegmentData lineindex = bins[currentbin].tail();
         bins[currentbin].pop_back();
         //
         opencount--;

         int ref = lineindex.ref;
         int dir = (lineindex.dir == 1) ? 0 : 1;
         int coverage = lineindex.coverage & uncovered[ref][dir];
         if (coverage != 0) {
            register int rbin = 0;
            int rbinbase;
            if (lineindex.previous.ref != -1) {
               uncovered[ref][dir] &= ~coverage;
               while (((coverage >> rbin) & 0x1) == 0)
                  rbin++;
               rbinbase = rbin;
               while (rbin < radiussize) {
                  if (((coverage >> rbin) & 0x1) == 1) {
                     audittrail[ref][rbin][dir].depth = depthlevel;
                     audittrail[ref][rbin][dir].previous = lineindex.previous;
                     audittrail[lineindex.previous.ref][rbin][(lineindex.previous.dir == 1) ? 0 : 1].leaf = false;
                  }
                  rbin++;
               }
            }
            else {
               rbinbase = 0;
               uncovered[ref][0] &= ~coverage;
               uncovered[ref][1] &= ~coverage;
            }
            Connector& line = m_connectors[ref];
            float seglength;
            register int extradepth;
            if (lineindex.dir != -1) {
               for (size_t k = 0; k < line.m_forward_segconns.size(); k++) {
                  rbin = rbinbase;
                  SegmentRef conn = line.m_forward_segconns.key(k);
                  if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
							//EF routeweight*
							if (routeweight_col != -1) {  //EF here we do the weighting of the angular cost by the weight of the next segment
													//note that the content of the routeweights array is scaled between 0 and 1 and is reversed 
													// such that: = 1.0-(m_attributes.getValue(i, routeweight_col)/max_value)
								extradepth = (int) floor(line.m_forward_segconns.value(k) * tulip_bins * 0.5 * routeweights[conn.ref]);					 
							}
							//*EF routeweight
							else {
								extradepth = (int) floor(line.m_forward_segconns.value(k) * tulip_bins * 0.5);
							}
							seglength = lengths[conn.ref];
                     switch (radius_type) {
                     case Options::RADIUS_ANGULAR:
                        while (rbin != radiussize && radius[rbin] != -1 && depthlevel+extradepth > (int) radius[rbin]) {
                           rbin++;
                        }
                        break;
                     case Options::RADIUS_METRIC:
                        while (rbin != radiussize && radius[rbin] != -1 && lineindex.metricdepth+seglength*0.5 > radius[rbin]) {
                           rbin++;
                        }
                        break;
                     case Options::RADIUS_STEPS: 
                        if (rbin != radiussize && radius[rbin] != -1 && lineindex.segdepth >= (int) radius[rbin]) {
                           rbin++;
                        }
                        break;
                     }
                     if ((coverage >> rbin) != 0) {
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].add(
                           SegmentData(conn,SegmentRef(1,lineindex.ref),lineindex.segdepth+1,lineindex.metricdepth+seglength,(coverage >> rbin) << rbin), paftl::ADD_DUPLICATE);
                        opencount++;
                     }
                  }
               }
            }
            if (lineindex.dir != 1) {
               for (size_t k = 0; k < line.m_back_segconns.size(); k++) {
                  rbin = rbinbase;
                  SegmentRef conn = line.m_back_segconns.key(k);
                  if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
							//EF routeweight*
							if (routeweight_col != -1) {  //EF here we do the weighting of the angular cost by the weight of the next segment
													//note that the content of the routeweights array is scaled between 0 and 1 and is reversed 
													// such that: = 1.0-(m_attributes.getValue(i, routeweight_col)/max_value)
								extradepth = (int) floor(line.m_back_segconns.value(k) * tulip_bins * 0.5 * routeweights[conn.ref]);					 
							}
							//*EF routeweight
							else {
								extradepth = (int) floor(line.m_back_segconns.value(k) * tulip_bins * 0.5);
							}
                     seglength = lengths[conn.ref];
                     switch (radius_type) {
                     case Options::RADIUS_ANGULAR:
                        while (rbin != radiussize && radius[rbin] != -1 && depthlevel+extradepth > (int) radius[rbin]) {
                           rbin++;
                        }
                        break;
                     case Options::RADIUS_METRIC:
                        while (rbin != radiussize && radius[rbin] != -1 && lineindex.metricdepth+seglength*0.5 > radius[rbin]) {
                           rbin++;
                        }
                        break;
                     case Options::RADIUS_STEPS: 
                        if (rbin != radiussize && radius[rbin] != -1 && lineindex.segdepth >= (int) radius[rbin]) {
                           rbin++;
                        }
                        break;
                     }
                     if ((coverage >> rbin) != 0) {
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].add(
                           SegmentData(conn,SegmentRef(-1,lineindex.ref),lineindex.segdepth+1,lineindex.metricdepth+seglength,(coverage >> rbin) << rbin), paftl::ADD_DUPLICATE);
                        opencount++;
                     }
                  }
               }
            }
         }
      }
      // set the attributes for this node:
      for (int k = 0; k < radiussize; k++) {
         // note, curs_total_depth must use double as mantissa can get too long for int in large systems
         double curs_node_count = 0.0, curs_total_depth = 0.0;
         double curs_total_weight = 0.0, curs_total_weighted_depth = 0.0;
         size_t j;
         for (j = 0; j < m_connectors.size(); j++) {
            // find dir according 
            bool m0 = ((uncovered[j][0] >> k) & 0x1) == 0;
            bool m1 = ((uncovered[j][1] >> k) & 0x1) == 0;
            if ((m0 | m1) != 0) {
               int dir;
               if (m0 & m1) {
                  // dir is the one with the lowest depth:
                  if (audittrail[j][k][0].depth < audittrail[j][k][1].depth)
                     dir = 0;
                  else
                     dir = 1;
               }
               else {
                  // dir is simply the one that's filled in:
                  dir = m0 ? 0 : 1;
               }
               curs_node_count++;
               curs_total_depth += audittrail[j][k][dir].depth;
               curs_total_weight += weights[j];
               curs_total_weighted_depth += audittrail[j][k][dir].depth * weights[j];
               //
               if (choice && audittrail[j][k][dir].leaf) {
                  // note, graph may be directed (e.g., for one way streets), so both ways must be included from now on:
                  SegmentRef here = SegmentRef(dir == 0 ? 1 : -1,j);
                  if (here.ref != rowid) {
                     int choicecount = 0;
                     double choiceweight = 0.0;
							//EFEF*
							double choiceweight2 = 0.0;
							//*EFEF
                     while (here.ref != rowid) { // not rowid means not the current root for the path
                        int heredir = (here.dir == 1) ? 0 : 1;
                        // each node has the existing choicecount and choiceweight from previously encountered nodes added to it
                        audittrail[here.ref][k][heredir].choice += choicecount;
                        // nb, weighted values calculated anyway to save time on 'if'
                        audittrail[here.ref][k][heredir].weighted_choice += choiceweight;
                        //EFEF*
								audittrail[here.ref][k][heredir].weighted_choice2 += choiceweight2;
								//*EFEF
								// if the node hasn't been encountered before, the choicecount and choiceweight is 
                        // incremented for all remaining nodes to be encountered on the backwards route from it
                        if (!audittrail[here.ref][k][heredir].choicecovered) {
                           // this node has not been encountered before: this adds the choicecount and weight for this
                           // node, and flags it as visited
                           choicecount++;
                           choiceweight += weights[here.ref] * rootweight;
									//EFEF*
									choiceweight2 += weights2[here.ref] * rootweight;//rootweight!
									//*EFEF

                           audittrail[here.ref][k][heredir].choicecovered = true;
                           // note, for weighted choice, the start and end points have choice added to them:
                           if (weighting_col != -1) {
                              audittrail[here.ref][k][heredir].weighted_choice += (weights[here.ref] * rootweight) / 2.0;
										//EFEF*
										if (weighting_col2 != -1) {
											audittrail[here.ref][k][heredir].weighted_choice2 += (weights2[here.ref] * rootweight) / 2.0;  //rootweight!
										}
										//*EFEF
									}
                        }
                        here = audittrail[here.ref][k][heredir].previous;
                     }
                     // note, for weighted choice, the start and end points have choice added to them:
                     // (this is the summed weight for all starting nodes encountered in this path)
                     if (weighting_col != -1) {
                        audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weighted_choice += choiceweight / 2.0;
								//EFEF*
								if (weighting_col2 != -1) {
									audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weighted_choice2 += choiceweight2 / 2.0;
								}
								//*EFEF
							}
                  }
               }
            }
         }
         double total_depth_conv = curs_total_depth / ((tulip_bins - 1.0f) * 0.5f);
         double total_weighted_depth_conv = curs_total_weighted_depth / ((tulip_bins - 1.0f) * 0.5f);
         //
         m_attributes.setValue(rowid,count_col[k],float(curs_node_count));
         if (curs_node_count > 1) {
            // for dmap 8 and above, mean depth simply isn't calculated as for radius measures it is meaningless
            m_attributes.setValue(rowid,td_col[k],total_depth_conv);
            if (weighting_col != -1) {
               m_attributes.setValue(rowid,total_weight_col[k],float(curs_total_weight));
               m_attributes.setValue(rowid,w_td_col[k],float(total_weighted_depth_conv));
            }
         }
         else {
            m_attributes.setValue(rowid,td_col[k],-1);
            if (weighting_col != -1) {
               m_attributes.setValue(rowid,total_weight_col[k],-1.0f);
               m_attributes.setValue(rowid,w_td_col[k],-1.0f);
            }
         }
         // for dmap 10 an above, integration is included!
         if (total_depth_conv > 1e-9) {
            m_attributes.setValue(rowid,integ_col[k],(float)(curs_node_count*curs_node_count/total_depth_conv));
            if (weighting_col != -1) {
               m_attributes.setValue(rowid,w_integ_col[k],(float)(curs_total_weight*curs_total_weight/total_weighted_depth_conv));
            }
         }
         else {
            m_attributes.setValue(rowid,integ_col[k],-1);
            if (weighting_col != -1) {
               m_attributes.setValue(rowid,w_integ_col[k],-1.0f);
            }
         }
      }
      //
      processed_rows++;
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
   				// interactive is usual Depthmap: throw an exception if cancelled
	   			if (interactive) {
                  for (size_t i = 0; i < m_connectors.size(); i++) {
                     for (size_t j = 0; j < size_t(radiussize); j++) {
                        delete [] audittrail[i][j];
                     }
                     delete [] audittrail [i];
                     delete [] uncovered [i];
                  }
                  delete [] audittrail;
                  delete [] uncovered;
                  delete [] bins;
                  throw Communicator::CancelledException();
               }
               else {
                  // in non-interactive mode, retain what's been processed already
                  break;
               }
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, rowid );
         }         
      }
   }
   if (choice) {
      for (size_t rowid = 0; rowid < m_connectors.size(); rowid++) {
         for (size_t r = 0; r < radius.size(); r++) {
            // according to Eva's correction, total choice and total weighted choice
            // should already have been accumulated by radius at this stage
            double total_choice = audittrail[rowid][r][0].choice + audittrail[rowid][r][1].choice;
            double total_weighted_choice = audittrail[rowid][r][0].weighted_choice + audittrail[rowid][r][1].weighted_choice;
            //EFEF*
				double total_weighted_choice2 = audittrail[rowid][r][0].weighted_choice2 + audittrail[rowid][r][1].weighted_choice2;
				//*EFEF
			
				// normalised choice now excluded for two reasons:
            // a) not useful measure, b) in parallel calculations, cannot be calculated at this stage
            // n.b., it is possible through the front end: the new choice takes into account bidirectional routes, 
            // so it should be normalised according to (n-1)(n-2) (maximum possible through routes) not (n-1)(n-2)/2
            // the relativised segment length weighted choice equation was (total_seg_length*total_seg_length-seg_length*seg_length)/2
            // again, drop the divide by 2 for the new implementation
            //
            //
            m_attributes.setValue(rowid,choice_col[r],float(total_choice));
            if (weighting_col != -1) {
               m_attributes.setValue(rowid,w_choice_col[r],float(total_weighted_choice));
					//EFEF*
					if (weighting_col2 != -1) {
						m_attributes.setValue(rowid,w_choice_col2[r],float(total_weighted_choice2));
					}
					//*EFEF
				}
         }
      }
   }
   for (size_t i = 0; i < m_connectors.size(); i++) {
      for (int j = 0; j < radiussize; j++) {
         delete [] audittrail[i][j];
      }
      delete [] audittrail [i];
      delete [] uncovered [i];
   }
   delete [] audittrail;
   delete [] uncovered;
   delete [] bins;

   m_displayed_attribute = -2; // <- override if it's already showing
   if (choice) {
      setDisplayedAttribute(choice_col.tail());
   }
   else {
      setDisplayedAttribute(td_col.tail());
   }
   return processed_rows;
}

// revised to use tulip bins for faster analysis of large spaces

bool ShapeGraph::angularstepdepth(Communicator *comm)
{
   std::string stepdepth_col_text = "Angular Step Depth";
   int stepdepth_col = m_attributes.insertColumn(stepdepth_col_text.c_str());

   int tulip_bins = 1024;
   // calc so duplicate code above
   tulip_bins /= 2;  // <- actually use semicircle of tulip bins
   tulip_bins += 1;

   bool *covered = new bool [m_connectors.size()];
   for (size_t i = 0; i < m_connectors.size(); i++) {
      covered[i] = false;
   }
   pqvector<SegmentData> *bins = new pqvector<SegmentData>[tulip_bins];

   int opencount = 0;
   for (auto& sel: m_selection_set) {
      int row = m_attributes.getRowid(sel);
      if (row != -1) {
         bins[0].push_back(SegmentData(0,row,SegmentRef(),0,0.0,0));
         opencount++;
      }
   }
   int depthlevel = 0;
   int currentbin = 0;
   while (opencount) {
      while (!bins[currentbin].size()) {
         depthlevel++;
         currentbin++;
         if (currentbin == tulip_bins) {
            currentbin = 0;
         }
      }
      SegmentData lineindex;
      if (bins[currentbin].size() > 1) {
         // it is slightly slower to delete from an arbitrary place in the bin, 
         // but it is necessary to use random paths to even out the number of times through equal paths
         int curr = pafrand() % bins[currentbin].size();
         lineindex = bins[currentbin][curr];
         bins[currentbin].remove_at(curr);
         // note: do not clear choice values here!
      }
      else {
         lineindex = bins[currentbin][0];
         bins[currentbin].pop_back();
      }
      opencount--;
      if (!covered[lineindex.ref]) {
         covered[lineindex.ref] = true;
         Connector& line = m_connectors[lineindex.ref];
         // convert depth from tulip_bins normalised to standard angle
         // (note the -1)
         double depth_to_line = depthlevel / ((tulip_bins - 1) * 0.5);
         m_attributes.setValue(lineindex.ref,stepdepth_col,depth_to_line);
         register int extradepth;
         if (lineindex.dir != -1) {
            for (size_t k = 0; k < line.m_forward_segconns.size(); k++) {
               if (!covered[line.m_forward_segconns.key(k).ref]) {
                  extradepth = (int) floor(line.m_forward_segconns.value(k) * tulip_bins * 0.5);
                  bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                      SegmentData(line.m_forward_segconns.key(k),lineindex.ref,lineindex.segdepth+1,0.0,0));
                  opencount++;
               }
            }
         }
         if (lineindex.dir != 1) {
            for (size_t k = 0; k < line.m_back_segconns.size(); k++) {
               if (!covered[line.m_back_segconns.key(k).ref]) {
                  extradepth = (int) floor(line.m_back_segconns.value(k) * tulip_bins * 0.5);
                  bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                      SegmentData(line.m_back_segconns.key(k),lineindex.ref,lineindex.segdepth+1,0.0,0));
                  opencount++;
                }
            }
         }
      }
   }
   delete [] covered;
   delete [] bins;

   m_displayed_attribute = -2; // <- override if it's already showing
   setDisplayedAttribute(stepdepth_col);

   return true;
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// helper -- a little class to tidy up a set of lines

void TidyLines::tidy(prefvec<Line>& lines, const QtRegion& region)
{
   size_t i = 0;
   m_region = region;
   double maxdim = __max(m_region.width(),m_region.height());

   // simple first pass -- remove very short lines
   pvecint removelist;
   for (i = 0; i < lines.size(); i++) {
      if (lines[i].length() < maxdim * TOLERANCE_B) {
         removelist.add(i);
      }
   }
   lines.remove_at(removelist);
   removelist.clear();  // always clear this list, it's reused

   // now load up m_lines...
   initLines(lines.size(),m_region.bottom_left,m_region.top_right);
   for (i = 0; i < lines.size(); i++) {
      addLine(lines[i]);
   }
   sortPixelLines();

   for (i = 0; i < lines.size(); i++) {
      // n.b., as m_lines have just been made, note that what's in m_lines matches whats in lines
      // we will use this later!
      m_test++;
      m_lines[i].test = m_test;
      PixelRefVector list = pixelateLine( m_lines[i].line );
      for (size_t a = 0; a < list.size(); a++) {
         for (size_t b = 0; b < m_pixel_lines[ list[a].x ][ list[a].y ].size(); b++) {
            int j = m_pixel_lines[ list[a].x ][ list[a].y ][b];
            if (m_lines[j].test != m_test && j > (int)i && intersect_region(lines[i],lines[j],TOLERANCE_B * maxdim)) {
               m_lines[j].test = m_test;
               int axis_i = (lines[i].width() >= lines[i].height()) ? XAXIS : YAXIS;
               int axis_j = (lines[j].width() >= lines[j].height()) ? XAXIS : YAXIS;
               int axis_reverse = (axis_i == XAXIS) ? YAXIS : XAXIS;
               if (axis_i == axis_j && fabs(lines[i].grad(axis_reverse) - lines[j].grad(axis_reverse)) < TOLERANCE_A
                                    && fabs(lines[i].constant(axis_reverse) - lines[j].constant(axis_reverse)) < (TOLERANCE_B * maxdim)) {
                  // check for overlap and merge
                  int parity = (axis_i == XAXIS) ? 1 : lines[i].sign();
                  if ((lines[i].start()[axis_i] * parity + TOLERANCE_B * maxdim) > (lines[j].start()[axis_j] * parity) &&
                      (lines[i].start()[axis_i] * parity) < (lines[j].end()[axis_j] * parity + TOLERANCE_B * maxdim)) {
                     int end = ((lines[i].end()[axis_i] * parity) > (lines[j].end()[axis_j] * parity)) ? i : j;
                     lines[j].bx() = lines[end].bx();
                     lines[j].by() = lines[end].by();
                     removelist.add(i);
                     continue; // <- don't do this any more, we've zapped it and replaced it with the later line
                  }
                  if ((lines[j].start()[axis_j] * parity + TOLERANCE_B * maxdim) > (lines[i].start()[axis_i] * parity) &&
                      (lines[j].start()[axis_j] * parity) < (lines[i].end()[axis_i]  * parity + TOLERANCE_B * maxdim)) {
                     int end = ((lines[i].end()[axis_i] * parity) > (lines[j].end()[axis_j] * parity)) ? i : j;
                     lines[j].ax() = lines[i].ax();
                     lines[j].ay() = lines[i].ay();
                     lines[j].bx() = lines[end].bx();
                     lines[j].by() = lines[end].by();
                     removelist.add(i);
                     continue; // <- don't do this any more, we've zapped it and replaced it with the later line
                  }
               }
            }
         }
      }
   }
   lines.remove_at(removelist);
   removelist.clear();  // always clear this list, it's reused
}

void TidyLines::quicktidy(pqmap<int,Line>& lines, const QtRegion& region)
{
   m_region = region;

   double avglen = 0.0;
   size_t i;
   for (i = 0; i < lines.size(); i++) {
      avglen += lines[i].length();
   }
   avglen /= lines.size();

   double tolerance = avglen * 10e-6;

   // simple first pass -- remove very short lines
   pvecint removelist;
   for (i = 0; i < lines.size(); i++) {
      if (lines[i].length() < tolerance) {
         removelist.add(i);
      }
   }
   if (removelist.size()) {
      lines.remove_at(removelist);
   }
   removelist.clear();  // always clear this list, it's reused

   // now load up m_lines...
   initLines(lines.size(),m_region.bottom_left,m_region.top_right);
   for (i = 0; i < lines.size(); i++) {
      addLine(lines[i]);
   }
   sortPixelLines();

   // and chop duplicate lines:
   for (i = 0; i < lines.size(); i++) {
      PixelRef start = pixelate(lines[i].start());
      for (size_t j = 0; j < m_pixel_lines[start.x][start.y].size(); j++) {
         int k = m_pixel_lines[start.x][start.y][j];
         if (k > (int)i && approxeq(m_lines[i].line.start(),m_lines[k].line.start(),tolerance)) {
            if (approxeq(m_lines[i].line.end(),m_lines[k].line.end(),tolerance)) {
               removelist.add(i);
               break;
            }
         }
      }
   }
   lines.remove_at(removelist);
   removelist.clear();  // always clear this list, it's reused}
}

/////////////////////////////////////////////////////
