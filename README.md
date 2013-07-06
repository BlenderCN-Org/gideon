Gideon
==========================

Gideon is a runtime-compiled language for programmable raytracing, with the ability to define both shaders and the entire
rendering system. The underlying engine implements a small set of critical features (ray traversal, sampling, vector math, etc.),
that can be used to build a fully functional raytracer in the language. Languages features include:
 * Ability to trace a ray through the scene.
 * User-defined BSDFs (call ```distributions``` in the language), which can be both evaluated and sampled.
 * Per-primitive shader functions that combine BSDFs with basic arithmetic operations.
 * Code can be cleanly organized into a ```module```, which can then be loaded using the ```load``` statement.
 * Just-in-time compilation using LLVM.

The goal of Gideon is to allow the user to rapidly iterate on new rendering techniques. The code complexity of a production
renderer may make it difficult to add new features for experimentation. With Gideon this is no problem, a new renderer
for testing purposes can be written in minutes.
 
## Usage

Interaction with Gideon is currently available as a Blender plugin. Enable Gideon from the Addon section of the
User Preferences UI, then select it as the current Render Engine. In the Render section of the Properties Panel,
you can choose which source code files to include in the renderer. Once that's done, the "Refresh Function List"
button will compile the code and build a list of available functions. Then create new materials, selecting the
appropriate shader function from the drop-down list.

For an language example, check out ```path_tracer.gdl``` in the ```examples``` directory.

## Roadmap

Gideon is in a very early stage of development. Most of the major compiler features have been implemented, but it is
lacking in a lot of areas. This is my first attempt at compiler of this nature, so the code could use a lot of cleaning up.
Aside from that, current areas of work are:
 * Warning/error messages in the compiler (it handles most errors nicely but line # reporting is not great).
 * Better sample generation with low-discrepancy sequences.
 * More complete definition of the distribution's functions (I want it usable for BSSRDF and Volumes as well).
 * User-defined types (at the moment it's a parsing problem).

Once the compiler is more complete, I plan to start filling out the standard library:
 * A larger set of standard distributions (including volumes).
 * Texture mapping (probably using OpenImageIO under the hood).
 * Query of object/primitive/scene attributes.

Along with more core engine features: 
 * Automatic differentiation.
 * Multi-threaded rendering.
 * Integration with Blender's layer system.
 * Investigate using LLVM's CUDA backend to see if GPU rendering would be possible.

## Installation

In the top-level directory, run:

```
mkdir build; cd build
cmake -DBLENDER_ADDON_ROOT=<path/to/blenders/addon/dir> <path/to/gideon/root>
make
make install
```
## Contributing

## Requirements

Gideon uses C++11 and depends on libboost and LLVM 3.3. The build system uses CMake, and Blender is required to
use the plugin.

##License
