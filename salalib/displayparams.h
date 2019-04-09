#pragma once

struct DisplayParams
{
   enum { AXMANESQUE = 0, GREYSCALE = 1, MONOCHROME = 2, DEPTHMAPCLASSIC = 3, PURPLEORANGE = 4, BLUERED = 5, HUEONLYAXMANESQUE = 6 };
   float blue;
   float red;
   int colorscale;
   DisplayParams()
   { blue = 0.0f; red = 1.0f; colorscale = 0; }
};
