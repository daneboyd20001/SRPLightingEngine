# SRPLightingEngine

A custom SDF (signed distance field) engine written in C++ and GLSL on the Raylib framework.

![preview](https://github.com/daneboyd20001/SRPLightingEngine/blob/main/include/gifs/daneGif.gif)
![preview](https://github.com/daneboyd20001/SRPLightingEngine/blob/main/include/gifs/gyroidGif.gif)
![preview](https://github.com/daneboyd20001/SRPLightingEngine/blob/main/include/gifs/noiseGif1.gif)
![preview](https://github.com/daneboyd20001/SRPLightingEngine/blob/main/include/gifs/noiseGif2.gif)

## Resources Used

- ### Simplex

  - #### [GLSL Learning](https://www.shader-learn.com/learn/noise/noise-functions)

## Build Instructions
### Linux

This project uses CMake. To compile and build, create a directory named build in the root of the project, navigate into it, and execute the following commands:

```
mkdir build
cd build
cmake ..
make
./SDF_Engine
```
