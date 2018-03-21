#include "mgraph440/spacepix.h"

namespace mgraph440 {

PixelRef SpacePixel::pixelate( const Point2f& p, bool constrain, int ) const
{
   PixelRef r;

   Point2f p1 = p;
   p1.normalScale(m_region);

   r.x = short(p1.x * double(m_cols-1e-9));
   if (constrain) {
      if (r.x >= m_cols)
         r.x = m_cols - 1;
      else if (r.x < 0)
         r.x = 0;
   }
   r.y = short(p1.y * double(m_rows-1e-9));
   if (constrain) {
      if (r.y >= m_rows)
         r.y = m_rows - 1;
      else if (r.y < 0)
         r.y = 0;
   }

   return r;
}

bool SpacePixel::read( ifstream& stream, int version )
{
   // clear anything that was there:
   if (m_pixel_lines)
   {
      for (int i = 0; i < m_cols; i++) {
         delete [] m_pixel_lines[i];
      }
      delete [] m_pixel_lines;
      m_pixel_lines = NULL;
   }
   if (m_display_lines) {
      delete [] m_display_lines;
      m_display_lines = NULL;
   }
   m_lines.clear();

   // read name:
   if (version >= VERSION_SPACEPIXELGROUPS) {
      m_name = dXstring440::readString(stream );
      stream.read( (char *) &m_show, sizeof(m_show) );
   }
   else {
      m_name = "<unknown>";
   }
   if (m_name.empty()) {
      m_name = "<unknown>";
   }

   m_edit = false; // <- just default to not editable on read

   if (version >= VERSION_LAYERCOLORS) {
      stream.read( (char *) &m_color, sizeof(m_color) );
   }

   // read extents:
   stream.read( (char *) &m_region, sizeof(m_region) );

   // read rows / cols
   stream.read( (char *) &m_rows, sizeof(m_rows) );
   stream.read( (char *) &m_cols, sizeof(m_cols) );

   // could work these two out on the fly, but it's easier to have them stored:
   //m_pixel_height = m_region.height() / double(m_rows);
   //m_pixel_width  = m_region.width()  / double(m_cols);

   // prepare loader:
   m_pixel_lines = new pvecint *[m_cols];
   for (int i = 0; i < m_cols; i++) {
      m_pixel_lines[i] = new pvecint[m_rows];
   }

   if (version < VERSION_DYNAMICLINES) {
      // read lines as refvec...
      prefvec<Line> lines;
      lines.read( stream );
      // ... and transfer to new system:
      m_ref = -1;
      for (size_t i = 0; i < lines.size(); i++) {
         m_lines.add(++m_ref, LineTest(lines[i],0));
      }
   }
   else {
      stream.read((char *) &m_ref, sizeof(m_ref));
      m_lines.read( stream );
   }

   if (version < VERSION_SPACEPIXELGROUPS) {
      // Scale up lines (should just work)
      for (size_t i = 0; i < m_lines.size(); i++) {
         m_lines[i].line.denormalScale(m_region);
      }
   }

   // now load into structure:
   for (size_t n = 0; n < m_lines.size(); n++) {

      PixelRefVector list = pixelateLine( m_lines[n].line );

      for (size_t m = 0; m < list.size(); m++) {
         // note: m_pixel_lines is an *ordered* list! --- used by other ops.
         m_pixel_lines[list[m].x][list[m].y].push_back( n );
      }
   }

   return true;
}

}
