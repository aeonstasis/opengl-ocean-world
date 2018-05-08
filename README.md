# OpenGL Ocean World

## Feature List (Planned)
- Ocean water simulation following Tessendorf
- Volumetric clouds
- Screen-space reflections, refractions, Fresnel
- Precipitation particle system
- Boat mesh and buoyant force modeling

## Building
```shell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make && ./bin/sea-of-thieves
```

Note that OpenGL is required to be installed. Tested on Ubuntu 16.04, 17.10.

## Controls
- Move with WASD
- Turn with Mouse
- Press the '+=' button to increment the day by 1 hour
- Press the '1' (one) button to toggle stormy mode

## Authors
Aaron Zou
Calvin Ly
