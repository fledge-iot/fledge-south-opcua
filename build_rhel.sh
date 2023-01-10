#!/bin/bash

# Pass any cmake options this way:

# ./build_rhel.sh -DFLEDGE_INSTALL=/some_path/Fledge

os_version=$(grep -o '^VERSION_ID=.*' /etc/os-release | cut -f2 -d\" | sed 's/"//g')
# Use scl_source & devtoolset only if OS is RedHat/CentOS 7
if [[ ${os_version} == *"7"* ]]
then
    source scl_source enable devtoolset-7
    export CC=$(scl enable devtoolset-7 "command -v gcc")
    export CXX=$(scl enable devtoolset-7 "command -v g++")
fi

mkdir -p build
cd build/
cmake $@ ..
make
