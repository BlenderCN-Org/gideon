#ifndef RT_DIFFERENTIAL_HPP
#define RT_DIFFERENTIAL_HPP

#include "math/vector.hpp"

namespace raytrace {

  /* A set of functions for computing ray differentials. */

  float3 differential_transfer_position(const float3 &P, const float3 &dP,
					const float3 &D, const float3 &dD,
					const float t, const float3 &N);

  void differential_dudv(const float3 &dPdx, const float3 &dPdy,
			 const float3 &dPdu, const float3 &dPdv, const float3 &N,
			 /* out */ float &dudx, /* out */ float &dudy,
			 /* out */ float &dvdx, /* out */ float &dvdy);
			 

};

#endif
