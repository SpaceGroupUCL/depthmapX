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



// ngraph.cpp

#include <salalib/mgraph.h>
#include <salalib/spacepix.h>
#include <salalib/pointdata.h>
#include <salalib/ngraph.h>
#include "genlib/legacyconverters.h"
#include "genlib/containerutils.h"

void Node::make(const PixelRef pix, PixelRefVector *bins, float *bin_far_dists, int q_octants)
{
   m_pixel = pix;

   for (int i = 0; i < 32; i++) {

      if (q_octants != 0x00FF) {
         // now, an octant filter has been used... note that the exact q-octants that
         // will have been processed rely on adjacenies in the q_octants...
         if (!(q_octants & processoctant(i))) {
            continue;
         }
      }

      m_bins[i].m_distance = bin_far_dists[i];

      if (i == 4 || i == 20) {
         m_bins[i].make(bins[i], PixelRef::POSDIAGONAL);
      }
      else if (i == 12 || i == 28) {
         m_bins[i].make(bins[i], PixelRef::NEGDIAGONAL);
      }
      else if ((i > 4 && i < 12) || (i > 20 && i < 28)) {
         m_bins[i].make(bins[i], PixelRef::VERTICAL);
      }
      else {
         m_bins[i].make(bins[i], PixelRef::HORIZONTAL);
      }
      // Now clear the bin!
      bins[i].clear();
   }
}

void Node::extractUnseen(PixelRefVector& pixels, PointMap *pointdata, int binmark)
{
   for (int i = 0; i < 32; i++) {
//      if (~binmark & (1 << i)) {  // <- DON'T USE THIS, IT CAUSES TOO MANY ERRORS!
         m_bins[i].extractUnseen(pixels, pointdata, (1 << i));
//      }
   }
}

void Node::extractMetric(std::set<MetricTriple>& pixels, PointMap *pointdata, const MetricTriple& curs)
{
   //if (dist == 0.0f || concaveConnected()) { // increases effiency but is too inaccurate
   //if (dist == 0.0f || !fullyConnected()) { // increases effiency but can miss lines
   if (curs.dist == 0.0f || pointdata->getPoint(curs.pixel).blocked() || pointdata->blockedAdjacent(curs.pixel)) { 
      for (int i = 0; i < 32; i++) {
         m_bins[i].extractMetric(pixels, pointdata, curs);
      }
   }
}

// based on extract metric

void Node::extractAngular(std::set<AngularTriple>& pixels, PointMap *pointdata, const AngularTriple& curs)
{
   if (curs.angle == 0.0f || pointdata->getPoint(curs.pixel).blocked() || pointdata->blockedAdjacent(curs.pixel)) {
      for (int i = 0; i < 32; i++) {
         m_bins[i].extractAngular(pixels, pointdata, curs);
      }
   }
}

bool Node::concaveConnected()
{
   // not quite correct -- sometimes at corners you 'see through' the very first connection
   // but a useful approximation: to be concave connected, you need less than 3 in a row somewhere:
   unsigned int test = 0;
   // note wraps around
   test |= (m_bins[0].count())  ? 0 : 0x101; 
   test |= (m_bins[4].count())  ? 0 : 0x202;
   test |= (m_bins[8].count())  ? 0 : 0x404; 
   test |= (m_bins[12].count()) ? 0 : 0x808;
   test |= (m_bins[16].count()) ? 0 : 0x010;
   test |= (m_bins[20].count()) ? 0 : 0x020;
   test |= (m_bins[24].count()) ? 0 : 0x040;
   test |= (m_bins[28].count()) ? 0 : 0x080;
   if (test != 0) {
      for (int i = 0; i < 8; i++) {
         if (((~test) & 1) && (test & 4) && ((~test) & 12)) { // less than 3 in a row test
            return true;
         }
         test >>= 1;
      }
   }
   return false;
}

bool Node::fullyConnected()
{
   // not quite correct -- sometimes at corners you 'see through' the very first connection
   return (m_bins[0].count() && m_bins[4].count() && m_bins[8].count() &&
           m_bins[12].count() && m_bins[16].count() && m_bins[20].count() &&
           m_bins[24].count() && m_bins[28].count());
}

//////////////////////////////////////////////////////////////////////////////////

bool Node::containsPoint(const PixelRef pixel) const
{
   bool found = false;
   int start, end;

   // This should really calculate which bin it ought to be in, but for now,
   // we'll reduce by quadrant:
   if (pixel.x > m_pixel.x) {
      if (pixel.y >= m_pixel.y) {
         start = 0; end = 7;
      }
      else {
         start = 25; end = 31;
      }
   }
   else {
      if (pixel.y > m_pixel.y) {
         start = 8; end = 15;
      }
      else {
         start = 16; end = 24;
      }
   }
   for (int i = start; i <= end; i++) {
      if (m_bins[i].containsPoint(pixel)) {
         found = true;
         break;
      }
   }
   return found;
}

//////////////////////////////////////////////////////////////////////////////////

void Node::first() const
{
   m_curbin = 0;
   do {
      m_bins[m_curbin].first();
   } while (m_bins[m_curbin].is_tail() && ++m_curbin < 32);
}

void Node::next() const
{
   m_bins[m_curbin].next();
   while (m_bins[m_curbin].is_tail() && ++m_curbin < 32) {
      m_bins[m_curbin].first();
   }
}

bool Node::is_tail() const
{
   return m_curbin == 32;
}

PixelRef Node::cursor() const
{
   return m_bins[m_curbin].cursor();
}

void Node::contents(PixelRefVector& hood) const
{
   first();
   while (!is_tail()) {
      depthmapX::addIfNotExists(hood, cursor());
      next();
   }
}

//////////////////////////////////////////////////////////////////////////////////

std::istream& Node::read(std::istream& stream, int version)
{
   int i;
   for (i = 0; i < 32; i++) {
      m_bins[i].read(stream, version);
   }

   for (i = 0; i < 32; i++) {
      pvector<PixelRef> tempPvector;
      tempPvector.read(stream);
      m_occlusion_bins[i] = genshim::toSTLVector(tempPvector);
   }

   return stream;
}

std::ofstream& Node::write(std::ofstream& stream, int version)
{
   int i;
   for (i = 0; i < 32; i++) {
      m_bins[i].write(stream,version);
   }

   for (i = 0; i < 32; i++) {
      pvector<PixelRef> tempPvector = genshim::toPVector(m_occlusion_bins[i]);
      tempPvector.write(stream);
   }
   return stream;
}

std::ostream& operator << (std::ostream& stream, const Node& node)
{
   for (int i = 0; i < 32; i++) {
      if (node.m_bins[i].count()) {
         stream << "    " << node.m_bins[i] << std::endl;
      }
   }
   return stream;
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::make(const PixelRefVector& pixels, char dir)
{
   m_pixel_vecs.clear();
   m_node_count = 0;

   if (pixels.size()) {

      m_dir = dir;

      if (m_dir & PixelRef::DIAGONAL) {
   
         PixelVec cur( pixels[0], pixels[0] );

         // Special, the diagonal should be pixels directly along the diagonal
         // Both posdiagonal and negdiagonal are positive in the x direction
         // Note that it is ordered anyway, so no need for anything too fancy:
         if (pixels.back().x < cur.start().x) {
            cur.m_start = pixels.back();
         }
         if (pixels.back().x > cur.end().x) {
            cur.m_end = pixels.back();
         }

         m_pixel_vecs.push_back(cur);
         m_node_count = pixels.size();
      }
      else {
         // Reorder the pixels:
         if (m_dir == PixelRef::HORIZONTAL) {
            std::set<PixelRefH> pixels_h;
            for (size_t i = 0; i < pixels.size(); i++) {
               pixels_h.insert(PixelRefH(pixels[i]));
            }
            // this looks like a simple bubble sort
            auto curr = pixels_h.begin();
            m_pixel_vecs.push_back(PixelVec(*curr, *curr));
            ++curr;
            auto prev = pixels_h.begin();
            for (;curr != pixels_h.end(); ++curr) {
               if (prev->y != curr->y || prev->x + 1 != curr->x) {
                  m_pixel_vecs.back().m_end = *prev;
                  m_pixel_vecs.push_back(PixelVec(*curr, *curr));
               }
               prev = curr;
            }
            m_pixel_vecs.back().m_end = *pixels_h.rbegin();
         }
         if (m_dir == PixelRef::VERTICAL) {
            std::set<PixelRefV> pixels_v;
            for (size_t i = 0; i < pixels.size(); i++) {
               pixels_v.insert(PixelRefV(pixels[i]));
            }
            // this looks like a simple bubble sort
            auto curr = pixels_v.begin();
            m_pixel_vecs.push_back(PixelVec(*curr, *curr));
            ++curr;
            auto prev = pixels_v.begin();
            for (;curr != pixels_v.end(); ++curr) {
               if (prev->x != curr->x || prev->y + 1 != curr->y) {
                  m_pixel_vecs.back().m_end = *prev;
                  m_pixel_vecs.push_back(PixelVec(*curr, *curr));
               }
               prev = curr;
            }
            m_pixel_vecs.back().m_end = *pixels_v.rbegin();
         }

         m_node_count = pixels.size();
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::extractUnseen(PixelRefVector& pixels, PointMap *pointdata, int binmark)
{
   for (auto pixVec: m_pixel_vecs) {
      for (PixelRef pix = pixVec.start(); pix.col(m_dir) <= pixVec.end().col(m_dir); ) {
         Point& pt = pointdata->getPoint(pix);
         if (pointdata->getPoint(pix).m_misc == 0) {
            pixels.push_back(pix);
            pointdata->getPoint(pix).m_misc |= binmark;
         }
         // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or horizontal
         if (!(m_dir & PixelRef::DIAGONAL)) {
            if (pt.m_extent.col(m_dir) >= pixVec.end().col(m_dir))
               break;
            pt.m_extent.col(m_dir) = pixVec.end().col(m_dir);
         }
         pix.move(m_dir);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::extractMetric(std::set<MetricTriple>& pixels, PointMap *pointdata, const MetricTriple& curs)
{
   for (auto pixVec: m_pixel_vecs) {
      for (PixelRef pix = pixVec.start(); pix.col(m_dir) <= pixVec.end().col(m_dir); ) {
         Point& pt = pointdata->getPoint(pix);
         if (pt.m_misc == 0 && 
            (pt.m_dist == -1.0 || (curs.dist + dist(pix,curs.pixel) < pt.m_dist))) {
            pt.m_dist = curs.dist + (float) dist(pix,curs.pixel);
            // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
            pt.m_cumangle = pointdata->getPoint(curs.pixel).m_cumangle + (curs.lastpixel == NoPixel ? 0.0f : (float) (angle(pix,curs.pixel,curs.lastpixel) / (M_PI * 0.5)));
            pixels.insert(MetricTriple(pt.m_dist, pix, curs.pixel));
         }
         pix.move(m_dir);
      }
   }
}

// based on metric

void Bin::extractAngular(std::set<AngularTriple>& pixels, PointMap *pointdata, const AngularTriple& curs)
{
   for (auto pixVec: m_pixel_vecs) {
      for (PixelRef pix = pixVec.start(); pix.col(m_dir) <= pixVec.end().col(m_dir); ) {
         Point& pt = pointdata->getPoint(pix);
         if (pt.m_misc == 0) {
            // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
            float ang = (curs.lastpixel == NoPixel) ? 0.0f : (float) (angle(pix,curs.pixel,curs.lastpixel) / (M_PI * 0.5));
            if (pt.m_cumangle == -1.0 || curs.angle + ang < pt.m_cumangle) {
               pt.m_cumangle = pointdata->getPoint(curs.pixel).m_cumangle + ang;
               pixels.insert(AngularTriple(pt.m_cumangle, pix, curs.pixel));
            }
         }
         pix.move(m_dir);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////

bool Bin::containsPoint(const PixelRef p) const
{
   for (auto pixVec: m_pixel_vecs) {
      if (m_dir & PixelRef::DIAGONAL) {
         // note abs is only allowed if you have pre-checked you are in the right quadrant!
         if (p.x >= pixVec.start().x && p.x <= pixVec.end().x &&
             abs(p.y - pixVec.start().y) == p.x - pixVec.start().x) {
            return true;
         }
      }
      else {
         if (p.row(m_dir) == pixVec.start().row(m_dir) &&
             p.col(m_dir) >= pixVec.start().col(m_dir) &&
             p.col(m_dir) <= pixVec.end().col(m_dir)) {
            return true;
         }
      }
   }
   return false;
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::contents(PixelRefVector& hood)
{
   first();
   while (!is_tail()) {
      depthmapX::addIfNotExists(hood, m_curpix);
      next();
   }
}

void Bin::first() const
{
   m_curvec = 0;
   if (!m_pixel_vecs.empty())
      m_curpix = m_pixel_vecs[m_curvec].m_start;
}

void Bin::next() const
{
   if (m_curpix.move(m_dir).col(m_dir) > m_pixel_vecs[m_curvec].end().col(m_dir)) {
      m_curvec++;
      if (m_curvec < m_pixel_vecs.size())
         m_curpix = m_pixel_vecs[m_curvec].m_start;
   }
}

bool Bin::is_tail() const
{
   return m_curvec >= m_pixel_vecs.size();
}

PixelRef Bin::cursor() const
{
   return (int) m_curpix;
}

///////////////////////////////////////////////////////////////////////////////////////

std::istream& Bin::read(std::istream& stream, int version)
{
   stream.read( (char *) &m_dir, sizeof(m_dir) );
   stream.read( (char *) &m_node_count, sizeof(m_node_count) );

   stream.read( (char *) &m_distance, sizeof(m_distance) );
   stream.read( (char *) &m_occ_distance, sizeof(m_occ_distance) );

   if (m_node_count) {
      if (m_dir & PixelRef::DIAGONAL) {
         m_pixel_vecs = std::vector<PixelVec>(1);
         m_pixel_vecs[0].read(stream, version, m_dir);
      }
      else {
         unsigned short length;
         stream.read( (char *) &length, sizeof(length) );
         m_pixel_vecs = std::vector<PixelVec>(length);
         m_pixel_vecs[0].read(stream, version, m_dir);
         for (int i = 1; i < length; i++) {
            m_pixel_vecs[i].read(stream, version, m_dir,m_pixel_vecs[i-1]);
         }
      }
   }

   return stream;
}

std::ofstream& Bin::write(std::ofstream& stream, int version)
{
   stream.write( (char *) &m_dir, sizeof(m_dir) );
   stream.write( (char *) &m_node_count, sizeof(m_node_count) );

   stream.write( (char *) &m_distance, sizeof(m_distance) );
   stream.write( (char *) &m_occ_distance, sizeof(m_occ_distance) );

   if (m_node_count) {

      if (m_dir & PixelRef::DIAGONAL) {
         m_pixel_vecs[0].write(stream,m_dir);
      }
      else {
         // TODO: Remove this limitation in the next version of the .graph format
         unsigned short length = m_pixel_vecs.size();
         stream.write( (char *) &length, sizeof(length) );
         m_pixel_vecs[0].write(stream,m_dir);
         for (int i = 1; i < length; i++) {
            m_pixel_vecs[i].write(stream,m_dir,m_pixel_vecs[i-1]);
         }
      }
   }

   return stream;
}

std::ostream& operator << (std::ostream& stream, const Bin& bin)
{
   int c = 0;
   for (auto pixVec: bin.m_pixel_vecs) {
      for (PixelRef p = pixVec.m_start;
           p.col(bin.m_dir) <= pixVec.end().col(bin.m_dir); p.move(bin.m_dir)) {
         if (++c % 10 == 0) {
            stream << "\n    ";
         }
         stream << p << ",";
      }
   }
   return stream;
}

///////////////////////////////////////////////////////////////////////////////////////

std::istream& PixelVec::read(std::istream& stream, int version, const char dir)
{
   unsigned short runlength;
   stream.read((char *) &m_start, sizeof(m_start));
   stream.read((char *) &runlength, sizeof(runlength));
   switch (dir) {
      case PixelRef::POSDIAGONAL:
         m_end.x = m_start.x + runlength;
         m_end.y = m_start.y + runlength;
         break;
      case PixelRef::NEGDIAGONAL:
         m_end.x = m_start.x + runlength;
         m_end.y = m_start.y - runlength;
         break;
      case PixelRef::HORIZONTAL:
         m_end.x = m_start.x + runlength;
         m_end.y = m_start.y;
         break;
      case PixelRef::VERTICAL:
         m_end.x = m_start.x;
         m_end.y = m_start.y + runlength;
         break;
   }
   return stream;
}

std::ofstream& PixelVec::write(std::ofstream& stream, const char dir)
{
   stream.write((char *) &m_start, sizeof(m_start));
   unsigned short runlength;
   switch (dir) {
      case PixelRef::HORIZONTAL:
      case PixelRef::POSDIAGONAL:
      case PixelRef::NEGDIAGONAL:
         runlength = m_end.x - m_start.x;
         break;
      case PixelRef::VERTICAL:
         runlength = m_end.y - m_start.y;
         break;
   }
   stream.write((char *) &runlength, sizeof(runlength));

   return stream;
}

struct ShiftLength {
   unsigned short shift : 4;
   unsigned short runlength : 12;
};

std::istream& PixelVec::read(std::istream& stream, int version, const char dir, const PixelVec& context)
{
   short primary;
   ShiftLength shiftlength;
   stream.read((char *) &primary, sizeof(primary));
   stream.read((char *) &shiftlength, sizeof(shiftlength));
   switch (dir) {
      case PixelRef::HORIZONTAL:
         m_start.x = primary;
         m_start.y = context.m_start.y + shiftlength.shift;
         m_end.x = m_start.x + shiftlength.runlength;
         m_end.y = m_start.y;
         break;
      case PixelRef::VERTICAL:
         m_start.x = context.m_start.x + shiftlength.shift;
         m_start.y = primary;
         m_end.x = m_start.x;
         m_end.y = m_start.y + shiftlength.runlength;
         break;
   }

   return stream;
}
   
std::ofstream& PixelVec::write(std::ofstream& stream, const char dir, const PixelVec& context)
{
   ShiftLength shiftlength;
   switch (dir) {
      case PixelRef::HORIZONTAL:
         stream.write((char *) &(m_start.x), sizeof(m_start.x));
         shiftlength.runlength = m_end.x - m_start.x;
         shiftlength.shift = m_start.y - context.m_start.y;
         break;
      case PixelRef::VERTICAL:
         stream.write((char *) &(m_start.y), sizeof(m_start.y));
         shiftlength.runlength = m_end.y - m_start.y;
         shiftlength.shift = m_start.x - context.m_start.x;
         break;
   }
   stream.write((char *) &shiftlength, sizeof(shiftlength));

   return stream;
}
