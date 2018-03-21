// genlib - a component of the depthmapX - spatial network analysis platform

// Paf Template Library --- a set of useful C++ templates
//
// Copyright (c) 1996-2011 Alasdair Turner (a.turner@ucl.ac.uk)
//
//-----------------------------------------------------------------------------
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See the lgpl.txt file for details
//-----------------------------------------------------------------------------

// a collection of math functions

#ifndef __PAFMATH_H__
#define __PAFMATH_H__

#include <cmath>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

inline double sqr(double a)
{
   return (a*a);
}

inline int sgn(double a)
{
   return (a < 0) ? -1 : 1;
}

#ifndef M_ROOT_1_2
#define M_ROOT_1_2 0.70710678118654752440084436210485
#endif

#ifndef M_1_LN2
#define M_1_LN2 1.4426950408889634073599246810019
#endif

const unsigned int PAF_RAND_MAX = 0x0FFFFFFF;
void pafsrand(unsigned int seed, int set = 0);
unsigned int pafrand(int set = 0);

// a random number from 0 to 1
inline double prandom(int set = 0)
{
   return double(pafrand(set)) / double(PAF_RAND_MAX);
}

// a random number from 0 to just less than 1

inline double prandomr(int set = 0)
{
   return double(pafrand(set)) / double(PAF_RAND_MAX + 1);
}

// note, in order to stop confusing myself I have ln defined:
#define ln(X) log(X)

inline double log2(double a)
{
   return (ln(a) * M_1_LN2);
}

// Hillier Hanson dvalue
/*
inline double dvalue(double k)
{
   return 2.0 * (3.3231 * k * log10(k+2) - 2.5863 * k + 1.0) / ((k - 1.0) * (k - 2.0));
}
*/

// Hillier Hanson dvalue (from Kruger 1989 -- see Teklenburg et al)
inline double dvalue(double k)
{
   return 2.0 * (k * (log2((k+2.0)/3.0) - 1.0) + 1.0) / ((k - 1.0) * (k - 2.0));
}

// Hillier Hanson pvalue
inline double pvalue(double k)
{
   return 2.0 * (k - log2(k) - 1.0) / ((k - 1.0) * (k - 2.0));
}

// Teklenburg integration (correction 31.01.11 due to Ulrich Thaler
inline double teklinteg(double nodecount, double totaldepth)
{
   return ln(0.5 * (nodecount - 2.0)) / ln(double(totaldepth - nodecount + 1));
}

// Penn palmtree

inline double palmtree(double n, double r)
{
   if (n > r) {
      return r * (n - 0.5 * (r+1));
   }
   else {
      return 0.5 * n * (n - 1);
   }
}

double poisson(int x, double lambda);
double cumpoisson(int x, double lambda);
int invcumpoisson(double p, double lambda);

#endif
