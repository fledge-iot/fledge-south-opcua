#!/bin/sh

# Pass any cmake options this way:

# ./build_rhel_centos_7.sh -DFOGLAMP_INSTALL=/some_path/FogLAMP

export CC=$(scl enable devtoolset-7 "command -v gcc")
export CXX=$(scl enable devtoolset-7 "command -v g++")

echo ${FREEOPCUA}
echo ${FOGLAMP_ROOT}

rm -rf build/
mkdir build
cd build/
cmake $@ ..
make
