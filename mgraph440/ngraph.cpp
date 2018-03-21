#include "mgraph440/ngraph.h"
#include "mgraph440/mgraph_consts.h"
#include "mgraph440/legacyconverters.h"

namespace mgraph440 {

inline int processoctant(int bin)
{
   int q = -1;
   switch (bin) {
   case 0: case 1: case 2: case 3: case 4:
      q = 1; break;
   case 5: case 6: case 7:
      q = 7; break;
   case 8: case 9: case 10: case 11:
      q = 6; break;
   case 12: case 13: case 14: case 15: case 16:
      q = 0; break;
   case 17: case 18: case 19: case 20:
      q = 2; break;
   case 21: case 22: case 23:
      q = 4; break;
   case 24: case 25: case 26: case 27:
      q = 5; break;
   case 28: case 29: case 30: case 31:
      q = 3; break;
   }

   return (1 << q);
}

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

void Bin::make(const PixelRefVector& pixels, char dir)
{
   if (m_pixel_vecs) {
      delete [] m_pixel_vecs;
      m_pixel_vecs = NULL;
   }
   m_length = 0;
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

         m_length = 1;
         m_pixel_vecs = new PixelVec[1];
         m_pixel_vecs[0] = cur;
         m_node_count = pixels.size();
      }
      else {
         prefvec<PixelVec> pixel_vecs;
         // Reorder the pixels:
         if (m_dir == PixelRef::HORIZONTAL) {
            pvector<PixelRefH> pixels_h;
            for (size_t i = 0; i < pixels.size(); i++) {
               pixels_h.add(pixels[i]);
            }
            // this looks like a simple bubble sort
            pixel_vecs.push_back(PixelVec(pixels_h[0],pixels_h[0]));
            for (size_t j = 1; j < pixels_h.size(); j++) {
               if (pixels_h[j-1].y != pixels_h[j].y || pixels_h[j-1].x + 1 != pixels_h[j].x) {
                  pixel_vecs.tail().m_end = pixels_h[j-1];
                  pixel_vecs.push_back(PixelVec(pixels_h[j],pixels_h[j]));
               }
            }
            pixel_vecs.tail().m_end = pixels_h.tail();
         }
         if (m_dir == PixelRef::VERTICAL) {
            pvector<PixelRefV> pixels_v;
            for (size_t i = 0; i < pixels.size(); i++) {
               pixels_v.add(pixels[i]);
            }
            // this looks like a simple bubble sort
            pixel_vecs.push_back(PixelVec(pixels_v[0],pixels_v[0]));
            for (size_t j = 1; j < pixels_v.size(); j++) {
               if (pixels_v[j-1].x != pixels_v[j].x || pixels_v[j-1].y + 1 != pixels_v[j].y) {
                  pixel_vecs.tail().m_end = pixels_v[j-1];
                  pixel_vecs.push_back(PixelVec(pixels_v[j],pixels_v[j]));
               }
            }
            pixel_vecs.tail().m_end = pixels_v.tail();
         }

         // Now compact the representation:
         m_length = pixel_vecs.size();
         m_pixel_vecs = new PixelVec[m_length];
         for (int k = 0; k < m_length; k++) {
            m_pixel_vecs[k] = pixel_vecs[k];
         }
         m_node_count = pixels.size();
      }
   }
}
void Bin::first() const
{
   m_curvec = 0;
   if (m_length)
      m_curpix = m_pixel_vecs[m_curvec].m_start;
}

void Bin::next() const
{
   if (m_curpix.move(m_dir).col(m_dir) > m_pixel_vecs[m_curvec].end().col(m_dir)) {
      m_curvec++;
      if (m_curvec < m_length)
         m_curpix = m_pixel_vecs[m_curvec].m_start;
   }
}

bool Bin::is_tail() const
{
   return m_curvec >= m_length;
}

PixelRef Bin::cursor() const
{
   return (int) m_curpix;
}
::std::ifstream& Node::read(::std::ifstream& stream, int version)
{
   int i;
   for (i = 0; i < 32; i++) {
      m_bins[i].read(stream, version);
   }
   if (version >= VERSION_OCCLUSIONS) {
      for (i = 0; i < 32; i++) {
         pvector<PixelRef> tempPvector;
         tempPvector.read(stream);
         m_occlusion_bins[i] = genshim440::toSTLVector(tempPvector);
      }
   }
   return stream;
}

std::ostream& Node::write(std::ostream& stream, int version)
{
   int i;
   for (i = 0; i < 32; i++) {
      m_bins[i].write(stream,version);
   }

   for (i = 0; i < 32; i++) {
      pvector<PixelRef> tempPvector = genshim440::toPVector(m_occlusion_bins[i]);
      tempPvector.write(stream);
   }
   return stream;
}

::std::ifstream& Bin::read(::std::ifstream& stream, int version)
{
   stream.read( (char *) &m_dir, sizeof(m_dir) );
   stream.read( (char *) &m_node_count, sizeof(m_node_count) );
   if (version >= VERSION_FILE_COMPRESSION) {
      if (version >= VERSION_ALWAYS_RECORD_BINDISTANCES) {
         stream.read( (char *) &m_distance, sizeof(m_distance) );
         stream.read( (char *) &m_occ_distance, sizeof(m_occ_distance) );
      }
      if (m_node_count) {
         if (version < VERSION_ALWAYS_RECORD_BINDISTANCES) {
            stream.read( (char *) &m_distance, sizeof(m_distance) );
         }
         if (m_dir & PixelRef::DIAGONAL) {
            m_length = 1;
            m_pixel_vecs = new PixelVec [m_length];
            m_pixel_vecs[0].read(stream, version, m_dir);
         }
         else {
            stream.read( (char *) &m_length, sizeof(m_length) );
            m_pixel_vecs = new PixelVec [m_length];
            m_pixel_vecs[0].read(stream, version, m_dir);
            for (int i = 1; i < m_length; i++) {
               m_pixel_vecs[i].read(stream, version, m_dir,m_pixel_vecs[i-1]);
            }
         }
         if (version < VERSION_ALWAYS_RECORD_BINDISTANCES) {
            if (version >= VERSION_OCCDISTANCES) {
               stream.read( (char *) &m_occ_distance, sizeof(m_occ_distance) );
            }
            else {
               m_occ_distance = 0.0f;
            }
         }
      }
      else {
         m_pixel_vecs = NULL;
         m_length = 0;
         if (version < VERSION_ALWAYS_RECORD_BINDISTANCES) {
            m_distance = 0.0f;
            m_occ_distance = 0.0f;
         }
      }
   }
   else {
      stream.read( (char *) &m_length, sizeof(m_length) );
      if (version >= VERSION_BINDISTANCES) {
         stream.read( (char *) &m_distance, sizeof(m_distance) );
      }
      else {
         m_distance = 0.0f;
      }
      if (m_pixel_vecs) {
         delete m_pixel_vecs;
         m_pixel_vecs = NULL;
      }
      if (m_length) {
         m_pixel_vecs = new PixelVec [m_length];
         stream.read( (char *) m_pixel_vecs, sizeof(PixelVec) * m_length);
      }
   }

   return stream;
}

std::ostream& Bin::write(std::ostream& stream, int version)
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
         stream.write( (char *) &m_length, sizeof(m_length) );
         m_pixel_vecs[0].write(stream,m_dir);
         for (int i = 1; i < m_length; i++) {
            m_pixel_vecs[i].write(stream,m_dir,m_pixel_vecs[i-1]);
         }
      }
   }

   return stream;
}

::std::ifstream& PixelVec::read(::std::ifstream& stream, int version, const char dir)
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

struct ShiftLength {
   unsigned short shift : 4;
   unsigned short runlength : 12;
};

::std::ifstream& PixelVec::read(::std::ifstream& stream, int version, const char dir, const PixelVec& context)
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

std::ostream& PixelVec::write(std::ostream& stream, const char dir)
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

std::ostream& PixelVec::write(std::ostream& stream, const char dir, const PixelVec& context)
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

PixelRef Node::cursor() const
{
   return m_bins[m_curbin].cursor();
}

}
