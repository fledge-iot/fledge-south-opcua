#!/bin/bash

# Pass any cmake options this way:

# ./build_deb.sh -DFOGLAMP_INSTALL=/some_path/FogLAMP

mkdir build
cd build/
cmake $@ ..
make
