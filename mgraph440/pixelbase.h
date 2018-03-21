#pragma once

#include "mgraph440/p2dpoly.h"
#include "mgraph440/pixelref.h"

namespace mgraph440 {

class PixelBase
{
public:
   int m_rows;
   int m_cols;
   QtRegion m_region;
   PixelBase() {;}
   // constrain is constrain to bounding box (i.e., in row / col bounds)
   virtual PixelRef pixelate(const Point2f&, bool constrain = true, int scalefactor = 1 ) const = 0;
   PixelRefVector pixelateLine( Line l, int scalefactor = 1 ) const;
   bool includes(const PixelRef pix) const {
      return (pix.x >= 0 && pix.x < m_cols && pix.y >= 0 && pix.y < m_rows);
   }
   const QtRegion& getRegion() const {
      return m_region;
   }
};

}
