#include "salalib/spacepixfile.h"

void SpacePixelFile::makeViewportShapes( const QtRegion& viewport ) const
{
   m_current_layer = -1;
   for (size_t i = m_spacePixels.size() - 1; static_cast<int>(i) != -1; i--) {
      if (m_spacePixels[i].isShown()) {
         m_current_layer = (int) i;
         m_spacePixels[i].makeViewportShapes( (viewport.atZero() ? m_region : viewport) );
      }
   }
}

bool SpacePixelFile::findNextShape(bool& nextlayer) const
{
   if (m_current_layer == -1)
      return false;
   while (!m_spacePixels[m_current_layer].findNextShape(nextlayer)) {
      while (++m_current_layer < (int)m_spacePixels.size() && !m_spacePixels[m_current_layer].isShown());
      if (m_current_layer == static_cast<int>(m_spacePixels.size())) {
         m_current_layer = -1;
         return false;
      }
   }
   return true;
}

bool SpacePixelFile::read( std::istream& stream )
{
   m_name = dXstring::readString(stream);
   stream.read( (char *) &m_region, sizeof(m_region) );
   int count;
   stream.read( (char *) &count, sizeof(count) );
   for (int i = 0; i < count; i++) {
       m_spacePixels.emplace_back();
       m_spacePixels.back().read(stream);
   }

   if (m_name.empty()) {
      m_name = "<unknown>";
   }
   return true;
}

bool SpacePixelFile::write( std::ofstream& stream )
{
   dXstring::writeString(stream, m_name);
   stream.write( (char *) &m_region, sizeof(m_region) );

   // Quick mod - TV
   int count = m_spacePixels.size();
   stream.write( (char *) &count, sizeof(count) );
   for (auto& spacePixel: m_spacePixels) {
      spacePixel.write(stream);
   }
   return true;
}
