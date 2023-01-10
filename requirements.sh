#!/usr/bin/env bash

##--------------------------------------------------------------------
## Copyright (c) 2019 Dianomic Systems
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##--------------------------------------------------------------------

##
## Author: Mark Riddoch, Massimiliano Pinto
##

set -e

fledge_location=$(pwd)
os_name=$(grep -o '^NAME=.*' /etc/os-release | cut -f2 -d\" | sed 's/"//g')
os_version=$(grep -o '^VERSION_ID=.*' /etc/os-release | cut -f2 -d\" | sed 's/"//g')
echo "Platform is ${os_name}, Version: ${os_version}"

if [[ ( ${os_name} == *"Red Hat"* || ${os_name} == *"CentOS"* ) ]]; then
    echo "Installing boost components..."
    sudo yum install -y boost-filesystem boost-program-options

    if [[ ${os_version} == *"7"* ]]; then
        echo "Installing development tools 7 components..."
        sudo yum install -y yum-utils
        sudo yum-config-manager --enable rhel-server-rhscl-7-rpms
        sudo yum install -y devtoolset-7
        source scl_source enable devtoolset-7
        export CC=/opt/rh/devtoolset-7/root/usr/bin/gcc
        export CXX=/opt/rh/devtoolset-7/root/usr/bin/g++
    fi
elif apt --version 2>/dev/null; then
    echo Installing boost components
    sudo apt install -y libboost-filesystem-dev
    sudo apt install -y libboost-program-options-dev
    sudo apt install -y libmbedtls-dev
else
    echo "Requirements cannot be automatically installed, please refer README.rst to install requirements manually"
fi

if [[ $# -eq 1 ]]; then
	directory=$1
	if [[ ! -d $directory ]]; then mkdir -p $directory; fi
else
	directory=~
fi

cd $directory
if [[ -d freeopcua ]]; then rm -rf freeopcua; fi

echo Fetching Free OPCUA library
git clone https://github.com/dianomic/freeopcua.git
cd freeopcua
git checkout Kapsch
mkdir build
sed \
	-e 's/add_library(opcuaclient/add_library(opcuaclient STATIC/' \
	-e 's/add_library(opcuacore/add_library(opcuacore STATIC/' \
	-e 's/add_library(opcuaprotocol/add_library(opcuaprotocol STATIC/' \
	-e 's/add_library(opcuaserver/add_library(opcuaserver STATIC/' \
	< CMakeLists.txt > CMakeLists.txt.$$ && mv CMakeLists.txt CMakeLists.txt.orig && \
	mv CMakeLists.txt.$$ CMakeLists.txt
cd build

cmake ..
make
cd ..
echo Set the environment variable FREEOPCUA to $(pwd)
echo export FREEOPCUA=$(pwd)
