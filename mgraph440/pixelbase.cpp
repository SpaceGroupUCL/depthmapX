#include "mgraph440/pixelbase.h"

namespace mgraph440 {

PixelRefVector PixelBase::pixelateLine( Line l, int scalefactor ) const
{
   PixelRefVector pixel_list;

   // this is *not* correct for lines that are off the edge...
   // should use non-constrained version (false), and find where line enters the region
   PixelRef a = pixelate( l.start(), true, scalefactor );
   PixelRef b = pixelate( l.end(), true, scalefactor );

   l.normalScale( m_region );

   pixel_list.push_back( a );

   int scaledcols = m_cols * scalefactor;
   int scaledrows = m_rows * scalefactor;

   int parity = 1;   // Line goes upwards
   if (a.y > b.y) {
      parity = -1;   // Line goes downwards
      a.y *= -1;
      b.y *= -1;     // Set ay and by saves work on comparisons later on
   }

   // special case 1
   if (a.x == b.x) {
      while (a.y < b.y ) {
         a.y += 1;
         pixel_list.push_back( PixelRef(a.x,parity * a.y) );
      }
   }
   else if (a.y == b.y) {
      while ( a.x < b.x ) {
         a.x += 1;
         pixel_list.push_back( PixelRef(a.x,parity * a.y) );     // Lines always go left to right
      }
   }
   else {

      double hw_ratio = l.height() / l.width(); // Working all of these out leaves less scope for floating point error
      double wh_ratio = l.width() / l.height();
      double x0_const = l.ay() - double(parity) * hw_ratio * l.ax();
      double y0_const = l.ax() - double(parity) * wh_ratio * l.ay();

      while (a.x < b.x || a.y < b.y) {
         PixelRef e;
         e.y = parity * int( double(scaledrows) * (x0_const + parity * hw_ratio * (double(a.x + 1) / double(scaledcols))) );
         // Note when decending 1.5 -> 1 and ascending 1.5 -> 2
         if (parity < 0) {
            e.x = int( double(scaledcols) * (y0_const + wh_ratio * ( double(a.y)   / double(scaledrows))) );
         }
         else {
            e.x = int( double(scaledcols) * (y0_const + wh_ratio * (double(a.y + 1) / double(scaledrows))) );
         }

         if (a.y < e.y) {
            while (a.y < e.y && a.y < b.y) {
               a.y += 1;
               pixel_list.push_back( PixelRef(a.x,parity * a.y) );
            }
            if (a.x < b.x) {
               a.x += 1;
               pixel_list.push_back( PixelRef(a.x,parity * a.y) );
            }
         }
         else if (a.x < e.x) {
            while (a.x < e.x && a.x < b.x) {
               a.x += 1;
               pixel_list.push_back( PixelRef(a.x,parity * a.y) );
            }
            if (a.y < b.y) {
               a.y += 1;
               pixel_list.push_back( PixelRef(a.x,parity * a.y) );
            }
         }
         else {
            // Special case: exactly diagonal step (should only require one step):
            // (Should actually never happen) (Doesn't: checked with RFH)
            a.x += 1;
            pixel_list.push_back( PixelRef(a.x,parity * a.y) );
            a.y += 1;
            pixel_list.push_back( PixelRef(a.x,parity * a.y) );
         }
      }
   }
   return pixel_list;
}

}
