#!/bin/bash

# Pass any cmake options this way:

# ./build_deb.sh -DFLEDGE_INSTALL=/some_path/Fledge

mkdir build
cd build/
cmake $@ ..
make
