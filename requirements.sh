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
## Author: Mark Riddoch
##

if [ $# -eq 1 ]; then
	directory=$1
	if [ ! -d $directory ]; then
		mkdir -p $directory
	fi
else
	directory=~
fi

if [ ! -d $directory/freeopcua ]; then
	cd $directory
	echo Fetching Free OPCUA library
	git clone https://github.com/FreeOpcUa/freeopcua.git
	cd freeopcua
	mkdir build
	sed -e 's/option(SSL_SUPPORT_MBEDTLS "Support rsa-oaep password encryption using mbedtls library " ON)/option(SSL_SUPPORT_MBEDTLS "Support rsa-oaep password encryption using mbedtls library " OFF)/' \
		-e 's/add_library(opcuaclient/add_library(opcuaclient STATIC/' \
		-e 's/add_library(opcuacore/add_library(opcuacore STATIC/' \
		-e 's/add_library(opcuaprotocol/add_library(opcuaprotocol STATIC/' \
		< CMakeLists.txt > CMakeLists.txt.$$ && mv CMakeLists.txt CMakeLists.txt.orig && \
		mv CMakeLists.txt.$$ CMakeLists.txt
	cd build
	echo Installing boost components
	sudo apt install -y libboost-filesystem-dev
	sudo apt install -y libboost-program-options-dev
	cmake ..
	make
	cd ..
	echo Set the environment variable FREEOPCUA to `pwd`
	echo export FREEOPCUA=`pwd`
fi
