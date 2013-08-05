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

#include "math/sampling.hpp"

#include <algorithm>
#include <iostream>
#include <math.h>

using namespace std;
using namespace raytrace;

float2 raytrace::sample_unit_disk(float rand_u, float rand_v) {
  float r = sqrtf(rand_u);
  float theta = 2.0f * pi * rand_v;

  return {r * cosf(theta), r * sinf(theta)};
}

float2 raytrace::concentric_sample_unit_disk(float rand_u, float rand_v) {
  float r, theta;
  float sx = 2.0f * rand_u - 1.0f;
  float sy = 2.0f * rand_v - 1.0f;

  //map the square to (r, theta)
  if (sx == 0.0 && sy == 0.0) return { 0.0f, 0.0f };
  
  if (sx >= -sy) {
    if (sx > sy) {
      //first region of disk
      r = sx;
      if (sy > 0.0f) theta = sy / r;
      else theta = 8.0f + (sy / r);
    
    }
    else {
      //second region
      r = sy;
      theta = 2.0f - (sx / r);
    }
  }
  else {
    if (sx <= sy) {
      //third region
      r = -sx;
      theta = 4.0f - (sy / r);
    }
    else {
      //fourth region
      r = -sy;
      theta = 6.0f + (sx / r);
    }
  }

  theta *= (pi * 0.25f);
  return {r * cosf(theta), r * sinf(theta)};
}

float3 raytrace::cosine_sample_hemisphere(const float3 &N, float rand_u, float rand_v) {
  float2 v = concentric_sample_unit_disk(rand_u, rand_v);
  float z = sqrtf(std::max(0.0f, 1.0f - (v.x*v.x) - (v.y*v.y)));

  float3 T, B;
  make_orthonormals(N, T, B);
  return (v.x * T) + (v.y * B) + (z * N);
}

float3 raytrace::uniform_sample_sphere(float rand_u, float rand_v) {
  float z = 2.0f * (rand_u - 1.0f);
  float t = rand_v * 2.0f * pi;
  
  float r = sqrtf(1.0f - (z*z));
  return {r*cosf(t), r*sinf(t), z};
}

/* Sampler Implementation */

sampler::sampler() :
  rng(bind(uniform_real_distribution<float>(0.0f, 1.0f),
	   mt19937(current_pixel_sample))),
  width(0), height(0), samples_per_pixel(0),
  current_x(0), current_y(0),
  current_pixel_sample(0)
{}

void sampler::setup(unsigned int width, unsigned int height, unsigned int samples_per_pixel,
		    const sample_generator &generator) {
  sample_offset.clear();
  sample_dimensions.clear();
  sample_generators.clear();
  sample_values.clear();

  image_samples.resize(2*samples_per_pixel);

  this->width = width;
  this->height = height;
  this->samples_per_pixel = samples_per_pixel;
  image_sample_generator = generator;

  current_x = current_y = 0;
  current_pixel_sample = 0;
}

sampler::sample_id sampler::add(unsigned int dim, unsigned int N, const sample_generator &generator) {
  unsigned int offset = sample_values.size();
  sample_id id = static_cast<sample_id>(sample_offset.size());
  
  sample_values.resize(sample_values.size() + dim*N);
  sample_offset.push_back(offset);
  sample_dimensions.push_back(dim);
  sample_generators.push_back(generator);
  
  return id;
}

unsigned int sampler::get_offset(sample_id s) const { return sample_offset[static_cast<unsigned int>(s)]; }

float sampler::access_1d(unsigned int idx) const { return sample_values[idx]; }
float2 sampler::access_2d(unsigned int idx) const { return float2{sample_values[idx], sample_values[idx+1]}; }

void sampler::next_sample(unsigned int x, unsigned int y,
			  /* out */ float2 *image_sample) {
  if (current_pixel_sample == samples_per_pixel) {
    //generate samples for current pixel
    image_sample_generator(x, y, 2, samples_per_pixel, &image_samples[0]);
    current_pixel_sample = 0;
    *image_sample = float2{image_samples[0], image_samples[1]};
  }
  else {
    *image_sample = float2{image_samples[current_pixel_sample], image_samples[current_pixel_sample+1]};
    ++current_pixel_sample;
  }


  prepare_samples(x, y);
}

void sampler::prepare_samples(unsigned int x, unsigned int y) {
  unsigned int sample_idx = 0;
  for (auto gen_it = sample_generators.begin(); gen_it != sample_generators.end(); ++gen_it, ++sample_idx) {
    unsigned int start = sample_offset[sample_idx];
    unsigned int end = (sample_idx == sample_offset.size() - 1) ? sample_values.size() : sample_offset[sample_idx + 1];

    (*gen_it)(x, y, sample_dimensions[sample_idx], end - start, &sample_values[start]);
  }
}

float sampler::random() { return rng(); }

unsigned int sampler::random_uint() { return static_cast<unsigned int>(rand()); }

sampler::sample_generator sampler::uniform() {
  return [this] (unsigned int x, unsigned int y,
		 unsigned int dim, unsigned int N,
		 /* out */ float *samples) -> void {
    for (unsigned int i = 0; i < dim*N; ++i) samples[i] = random();
  };
}

sampler::sample_generator sampler::latin_hypercube() {
  return [this] (unsigned int x, unsigned int y,
		 unsigned int dim, unsigned int N,
		 /* out */ float *samples) -> void {
    float delta = 1.0 / N;
    //generate samples along diagonal
    for (unsigned int i = 0; i < N; ++i) {
      for (unsigned int d = 0; d < dim; ++d) {
	samples[dim*i + d] = (i + random()) * delta;
      }
    }

    //permute samples in each dimension
    for (unsigned int d = 0; d < dim; ++d) {
      for (unsigned int j = 0; j < N; ++j) {
	unsigned int other = random_uint() % N;
	swap(samples[dim*j + d], samples[dim*other + d]);	     
      }
    }
  };
}

sampler::sample_generator sampler::select_generator(const string &name) {
  if (name == "uniform") return uniform();
  if (name == "lhs") return latin_hypercube();
  throw runtime_error("Unsupport sample generation strategy.");
}
