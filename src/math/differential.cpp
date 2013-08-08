/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "math/differential.hpp"
#include <cmath>

using namespace raytrace;

float3 raytrace::differential_transfer_position(const float3 &P, const float3 &dPdx,
						const float3 &D, const float3 &dDdx,
						const float t, const float3 &N) {
  //compute dt/dx
  float3 d0 = dPdx + t*dDdx;
  float dtdx = -dot(d0, N) / dot(D, N);

  return d0 + dtdx*D;
}

void raytrace::differential_dudv(const float3 &dPdx, const float3 &dPdy,
				 const float3 &dPdu, const float3 &dPdv, const float3 &N,
				 /* out */ float &dudx, /* out */ float &dudy,
				 /* out */ float &dvdx, /* out */ float &dvdy) {
  //determine which axis to project onto 2D
  float xn = fabsf(N.x);
  float yn = fabsf(N.y);
  float zn = fabsf(N.z);

  int largest_axis = 0; //use xn as temporary variable for max value
  if (yn > xn) {
    largest_axis = 1;
    xn = yn;
  }

  if (zn > xn) largest_axis = 2;

  int x_coord = (largest_axis + 1) % 3;
  int y_coord = (largest_axis + 2) % 3;

  float2 dpdx{dPdx[x_coord], dPdx[y_coord]};
  float2 dpdy{dPdy[x_coord], dPdy[y_coord]};
  float2 dpdu{dPdu[x_coord], dPdu[y_coord]};
  float2 dpdv{dPdv[x_coord], dPdv[y_coord]};

  /*
    We want to solve the system of vector equations:

    dpdx = dpdu * dudx + dpdv * dvdx
    dpdy = dpdu * dudy + dpdv * dvdy

    Where we want to solve for 4 variables: dudx, dvdx, dudy, dvdy.
    Consider each equation a 2x2 linear system, which can be solved
    using Cramer's Rule. Because the coefficients are always dpdu and dpdv,
    the two systems share the same denominator.
   */

  float detA = (dpdu.x * dpdv.y - dpdv.x * dpdu.y);
  if (detA != 0.0f) detA = 1.0f / detA;

  dudx = (dpdx.x*dpdv.y - dpdv.x*dpdx.y) * detA;
  dvdx = (dpdu.x*dpdx.y - dpdx.x*dpdu.y) * detA;

  dudy = (dpdy.x*dpdv.y - dpdv.x*dpdy.y) * detA;
  dvdy = (dpdu.x*dpdy.y - dpdy.x*dpdu.y) * detA;
}
