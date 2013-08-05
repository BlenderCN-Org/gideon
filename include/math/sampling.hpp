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

#ifndef RT_SAMPLING_HPP
#define RT_SAMPLING_HPP

#include "math/vector.hpp"

#include <vector>
#include <boost/function.hpp>

namespace raytrace {

  const float pi = 3.14159265359f;

  /* Disk sampling methods taken from Physically-Based Rendering */
  
  float2 sample_unit_disk(float rand_u, float rand_v);
  float2 concentric_sample_unit_disk(float rand_u, float rand_v);
  
  float3 cosine_sample_hemisphere(const float3 &N, float rand_u, float rand_v);

  float3 uniform_sample_sphere(float rand_u, float rand_v);

  /* A container for a generated sequence of samples. */
  class sampler {
  public:

    sampler();
    
    typedef unsigned int sample_id;
    typedef boost::function<void (unsigned int, unsigned int, unsigned int, unsigned int, float *)> sample_generator;
    
    sample_id add(unsigned int dim, unsigned int N, const sample_generator &generator);
    
    void setup(unsigned int width, unsigned int height, unsigned int samples_per_pixel,
	       const sample_generator &generator);
    
    void next_sample(unsigned int x, unsigned int y,
		     /* out */ float2 *image_sample);

    unsigned int get_offset(sample_id s) const;

    float access_1d(unsigned int idx) const;
    float2 access_2d(unsigned int idx) const;

    float random();
    unsigned int random_uint();

    //Sample Generation Strategies
    
    sample_generator uniform();
    sample_generator latin_hypercube();
    
    sample_generator select_generator(const std::string &name);

  private:

    boost::function<float ()> rng;
    unsigned int width, height, samples_per_pixel;

    unsigned int current_x, current_y;
    unsigned int current_pixel_sample;
    
    std::vector<unsigned int> sample_offset, sample_dimensions;

    std::vector<float> sample_values;
    std::vector<float> image_samples;
    
    std::vector<sample_generator> sample_generators;
    sample_generator image_sample_generator;
    
    void prepare_samples(unsigned int x, unsigned int y);
  };

};

#endif
