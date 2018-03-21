// genlib - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010 University College London, Alasdair Turner

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

#pragma once

#include "genlib/paftl.h"

// linear regression
// T should be int, float or double
template <class T> struct LinReg
{
   double S_x;
   double S_y;
   double S_xx;
   double S_yy;
   double S_xy;
   double n;
   //
   bool cached;
   double ca;
   double cb;
   //
   LinReg()
   {
      S_x = 0; S_y = 0; S_xx = 0; S_yy = 0; S_xy = 0; n = 0; cached = false;
   }
   LinReg(const pvector<T>& X, const pvector<T>& Y)
   {
      S_x = 0; S_y = 0; S_xx = 0; S_yy = 0; S_xy = 0;
      n = __min(X.size(),Y.size());
      for (int i = 0; i < n; i++) {
         S_x += double(X[i]); S_y += double(Y[i]);
         S_xx += sqr(double(X[i])); S_yy += sqr(double(Y[i]));
         S_xy += double(X[i]) * double(Y[i]);
      }
      cached = false;
   }
   void clear()
   {
      S_x = 0; S_y = 0; S_xx = 0; S_yy = 0; S_xy = 0; n = 0;
      cached = false;
   }
   void add(T x, T y)
   {
      n++;
      S_x += double(x); S_y += double(y);
      S_xx += sqr(double(x)); S_yy += sqr(double(y));
      S_xy += double(x) * double(y);
      cached = false;
   }
   void makecache()
   {
      cb = (n*S_xy-S_x*S_y)/(n*S_xx-S_x*S_x); ca = (S_y - cb * S_x) / n; cached = true;
   }
   // Y = bX + a
   double a()
   { if (!cached) makecache(); return ca; }
   double b()
   { if (!cached) makecache(); return cb; }
   double r()
   { return (n*S_xy-S_x*S_y)/sqrt( (n*S_xx-sqr(S_x))*(n*S_yy-sqr(S_y)) ); }
   double mu_x()
   { return S_x / n; }
   double mu_y()
   { return S_y / n; }
   // note you will need to keep the vectors if you need
   // to calculate the residuals: e[i] = y[i] - b() * x[i] - a();
   double model(double x)
   { if (!cached) makecache(); return cb * x + ca; }
   double invmodel(double y)
   { if (!cached) makecache(); return (y -ca) / cb ; }
};
