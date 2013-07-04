Gideon
==========================

Gideon is a runtime-compiled language for programmable shading in raytracers, with the ability to define both shaders and the entire
rendering system. The underlying engine implements a small set of critical features (ray traversal, sampling, vector math, etc.),
that can be used to build a fully functional raytracer in the language. Languages features include:
 * Ability to trace a ray through the scene.
 * User-defined BSDFs (call ```distributions``` in the language), which can be both evaluated and sampled.
 * Per-primitive shader functions that combine BSDFs with basic arithmetic operations.
 * Code can be cleanly organized into a ```module```, which can then be loaded using the ```load``` statement.

The goal of Gideon is to allow the user to rapidly iterate on new rendering techniques. The code complexity of a production
renderer may make it difficult to add new features for experimentation. With Gideon this is no problem, a new renderer
for testing purposes can be written in minutes.
 
## Usage

Interaction with Gideon is currently available as a Blender plugin. Enable Gideon from the Addon section of the
User Preferences UI, then select it as the current Render Engine. In the Render section of the Properties Panel,
you can choose which source code files to include in the renderer. Once that's done, the "Refresh Function List"
button will compile the code and build a list of available functions. Then create new materials, selecting the
appropriate shader function from the drop-down list.

## Installation

In the top-level directory, run:

```
mkdir build; cd build
cmake ..
make
make install
```
## Contributing

## Requirements

Gideon uses C++11 and depends on libboost and LLVM 3.2.

##License
