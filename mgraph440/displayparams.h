#pragma once

// For my colour scheme... some parameters to pass, and my own colour class

namespace mgraph440 {

struct DisplayParams
{
   enum { AXMANESQUE = 0, GREYSCALE = 1, MONOCHROME = 2, DEPTHMAPCLASSIC = 3, PURPLEORANGE = 4, BLUERED = 5 };
   float blue;
   float red;
   int colorscale;
   DisplayParams()
   { blue = 0.0f; red = 1.0f; colorscale = 0; }
};

}
