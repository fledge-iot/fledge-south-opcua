========================================================================
OPC UA C/C++ South plugin 
========================================================================

A simple asynchronous OPC UA plugin that registers for change events on
OPC UA objects.

NOTE:

This plugin assumes the freeopcua is available at a specified location
in the file system, see below.

Configuration
-------------

This configuration of this plugin requires 3 parameters to be set

asset
  An asset name prefix that is added to the OPC UA variables retrieved from the OPC UA server

url
  The URL used to connect the server, of the form opc.tcp://<hostname>:<port>/...

subscriptions
  An array of OPC UA node names that will control the subscription to
  variables in the OPC UA server. The array may be empty, in which case
  all variables are subscribed to in the server and will create assets in
  FogLAMP. Although simple subscribing to everything will return a lot of
  data that may not be of use. Alternatively a set of string may be give,
  the format of the strings is <namespace>:<name>. If the namespace is
  not requied then the name can simply be given. The plugin will traverse
  the node tree of the server and subscribe to all variables that live
  below the named nodes in the subscriptions array.
  
  Configuration examples:

.. code-block:: console

    {"subscriptions":["5:Simulation","2:MyLevel"]}
    {"subscriptions":["5:Sinusoid1","2:MyLevel","5:Sawtooth1"]}
    {"subscriptions":["2:Random.Double","2:Random.Boolean"]}

In the above examples
 - 5:Simulation is a node name under ObjectsNode
 - 5:Sinusoid1 and 5:Sawtooth1 are variables under ObjectsNode/Simulation 
 - 2:MyLevel is a variable under ObjectsNode/MyObjects/MyDevice
 - Random.Double and Random.Boolean are variables under ObjectsNode/Demo
 - 5 and 2 are the NamespaceIndex values of a node or a variable

It's also possible to specify an empty subscription array:

.. code-block:: console

    {"subscriptions":[]}

Note: depending on OPC UA server configuration (number of objects, number of variables)
this empty configuration might take a while to be loaded.

Object names, variable names and NamespaceIndexes can be easily retrieved
browsing the given OPC UA server using OPC UA clients, such as UaExpert

https://www.unified-automation.com/downloads/opc-ua-clients.html


As an examle the UA client shows:

.. code-block:: console

    Node:

    NodeId ns=5;s=85/0:Simulation
    NodeClass [Object]
    BrowseName 5:Simulation

    Variables:

    NodeId ns=5;s=Sinusoid1
    NodeClass [Variable]
    BrowseName 5:Sinusoid1

    NodeId ns=2;s=MyLevel
    NodeClass [Variable]
    BrowseName 2:MyLevel

Most examples come from Object in ProSys OPC UA simulation server:

https://www.prosysopc.com/products/opc-ua-simulation-server/

Building freeopuca
------------------

To build freeopcua you must clone the freeopcua repository to a directory of your choice.

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

If you require to place the freeopcua code elsewhere you may pass the requirements.sh script an argument
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
