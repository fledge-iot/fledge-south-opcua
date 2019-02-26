========================================================================
OPC UA C/C++ South plugin 
========================================================================

A simple asynchronous OPC UA plugin that registers for change events on OPC UA objects.

NOTE:

This plugin assumes the freeopcua is available at a specified location in the file system. To build you
must clone the freeopcua repository to a directory of your choice.

.. code-block:: console

  $ git clone https://github.com/FreeOpcUa/freeopcua.git
  $ cd freeopcua
  $ export FREEOPCUA=`pwd`
  $ mkdir build

Edit the OPCUA CMakeFiles.txt file and find the line


.. code-block:: console

  option(SSL_SUPPORT_MBEDTLS "Support rsa-oaep password encryption using mbedtls library " ON)

and set it to OFF.

.. code-block:: console

  option(SSL_SUPPORT_MBEDTLS "Support rsa-oaep password encryption using mbedtls library " OFF)

.. code-block:: console

The build options for the OPCUA libraries must be changed to create static libraries. To do this
find the occurences of the add_library directive for opcuaclient, opcuacode and opcuaprotocol
and add the option STATIC to it


.. code-block:: console

  add_library(opcuaclient STATIC
  ...

  add_library(opcuacore STATIC
  ...

  add_library(opcuaprotocol STATIC
  ...

.. code-block:: console

  $ cd build
  $ cmake ..
  $ make

The freeopcua library requires boost libraries that are not available in packaged form for the
Raspbery Pi. Therefore it can not be built for the Raspbery Pi without first building these boost
libraries.

Alternatively run the script requirements.sh to automate this and place a copy of the freeopcua
project in your home directory.

.. code-block:: console

  requirements.sh

If you require to place the freeopcua code elsewhere you may pass the requirements.sh script anrargument
of a directory name to use.

.. code-block:: console

  requirements.sh ~/projects

Build
-----

To build the opcua plugin run the commands:

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the FogLAMP develop package header files and libraries
  are expected to be located in /usr/include/foglamp and /usr/lib/foglamp
- If **FOGLAMP_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FOGLAMP_ROOT directory.
  Please note that you must first run 'make' in the FOGLAMP_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FOGLAMP_SRC** sets the path of a FogLAMP source tree
- **FOGLAMP_INCLUDE** sets the path to FogLAMP header files
- **FOGLAMP_LIB sets** the path to FogLAMP libraries
- **FOGLAMP_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FOGLAMP_INCLUDE** option should point to a location where all the FogLAMP 
   header files have been installed in a single directory.
 - The **FOGLAMP_LIB** option should point to a location where all the FogLAMP
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FOGLAMP_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FOGLAMP_ROOT set

  $ export FOGLAMP_ROOT=/some_foglamp_setup

  $ cmake ..

- set FOGLAMP_SRC

  $ cmake -DFOGLAMP_SRC=/home/source/develop/FogLAMP  ..

- set FOGLAMP_INCLUDE

  $ cmake -DFOGLAMP_INCLUDE=/dev-package/include ..
- set FOGLAMP_LIB

  $ cmake -DFOGLAMP_LIB=/home/dev/package/lib ..
- set FOGLAMP_INSTALL

  $ cmake -DFOGLAMP_INSTALL=/home/source/develop/FogLAMP ..

  $ cmake -DFOGLAMP_INSTALL=/usr/local/foglamp ..

******************************
Packaging for 'opcua' south
******************************

This repo contains the scripts used to create a foglamp-south-opcua Debian package.

The make_deb script
===================

Run the make_deb command:

.. code-block:: console

  $ ./make_deb help
  make_deb [help|clean|cleanall]
  This script is used to create the Debian package of FoglAMP C++ 'opcua' south plugin
  Arguments:
   help     - Display this help text
   clean    - Remove all the old versions saved in format .XXXX
   cleanall - Remove all the versions, including the last one
  $

Building a Package
==================

Finally, run the ``make_deb`` command:

.. code-block:: console

   $ ./make_deb
   The package root directory is   : /home/ubuntu/source/foglamp-south-opcua
   The FogLAMP required version    : >=1.4
   The package will be built in    : /home/ubuntu/source/foglamp-south-opcua/packages/build
   The architecture is set as      : x86_64
   The package name is             : foglamp-south-opcua-1.0.0-x86_64

   Populating the package and updating version file...Done.
   Building the new package...
   dpkg-deb: building package 'foglamp-south-opcua' in 'foglamp-south-opcua-1.0.0-x86_64.deb'.
   Building Complete.
   $

Cleaning the Package Folder
===========================

Use the ``clean`` option to remove all the old packages and the files used to make the package.

Use the ``cleanall`` option to remove all the packages and the files used to make the package.
