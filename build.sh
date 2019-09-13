#!/bin/bash

# Pass any cmake options this way:

# ./build.sh -DFOGLAMP_INSTALL=/some_path/FogLAMP
#

os_name=`(grep -o '^NAME=.*' /etc/os-release | cut -f2 -d\" | sed 's/"//g')`
os_version=`(grep -o '^VERSION_ID=.*' /etc/os-release | cut -f2 -d\" | sed 's/"//g')`

if [[ ( $os_name == *"Red Hat"* || $os_name == *"CentOS"* ) &&  $os_version == *"7"* ]]; then
	if [ -f build_rhel.sh ]; then
		echo "Custom build for platform is ${os_name}, Version: ${os_version}"
		./build_rhel.sh $@
	fi
elif apt --version 2>/dev/null; then
	if [ -f build_deb.sh ]; then
		echo "Custom build for platform is ${os_name}, Version: ${os_version}"
		./build_deb.sh $@
	fi
else
	echo "Custom build script not yet implemented for platform ${os_name}, Version: ${os_version}"
fi
