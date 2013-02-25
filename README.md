Relatively Crazy Raytracer
==========================

Description
-----------

Terminology
-----------

This section describes the components of the raytracing engine that can be described by the raytracer language.

###Distribution Function:
A function that describes how light interacts with a surface or volume. Also called a BSDF (Bidirectional Scattering Distribution Function). The <code>evaluate</code> function takes 4 basic parameters: 

* P_in - Location where light hits the object
* P_out - Location where light exits the object
* w_in - Direction of the incoming light
* w_out - Direction of the outgoing light

The value returned is color float4 value. As the function depends on the local geometry of the surface, in can also take the surface normal as an input. Distribution functions must also implement a <code>sample</code> function, which draws outgoing point and direction according to the distribution.

###Shader:
Describes how a point on a surface or volume should be rendered. Can read the parameters of the local surface to determine values like texture coordinates and vertex color. Using these values, a shader returns an arithmetic combination of distribution functions that completely describes the color/appearance of the object being rendered.

###Integrator:
The high level loop of a raytracing engine. Iterates over pixels, casts rays and evaluates shaders. It's return value is the final rendered image. Integrator programs can also be used to describe render prepasses (such as construction of irradiance caches).
