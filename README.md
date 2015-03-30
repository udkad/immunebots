Current version: 1.000860

To run immunebots, you first need to install the pre-req libraries. These are:
 1. Multilib
 2. Boost
 3. Optional: GLUT
 4. Optional: AntTweakBar

## Installing prereqs
 
On Ubuntu install most of the pre-reqs with:
`sudo apt-get install gcc-multilib g++-multilib libboost-all-dev`

Note: This works on Ubuntu 14.04 (32 & 64 bit), tested Dec 2014. Even if visualisation is not required, the GLUT libraries are needed to compile successfully.

## Optional prereqs: Graphics

To visualise the simulation, you will need the following libraries installed:
 1. GLUT
 2. AntTweakBar

With the `IMMUNEBOTS_NOGL` preprocessor directive, neither are required for compiling or running.

The OpenGL libraries can be installed with:
`sudo apt-get install freeglut3-dev libglew-dev`

For AntTweakBar, first install the following prereqs:
`sudo apt-get install libx11-dev mesa-common-dev libglu1-mesa-dev`

Instructions for downloading and installing AntTweakBar are here:
http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:download
```
# Download the latest version (tested with 1.16), unzip and then:
cd src
make
# Then copy the AntTweakBar header file to the includes directory:
sudo cp ../include/AntTweakBar.h /usr/include/
# .. and copy the lib to the system lib dir (or add the path to LD_LIBRARY_PATH):
sudo cp ../lib/libAntTweakBar.so /usr/lib/i386-linux-gnu/ # For 32bit systems, or:
sudo cp ../lib/libAntTweakBar.so /usr/lib/x86_64-linux-gnu/ # for 64bit systems
```

## Building immunebots

In the base directory, run:
```
cd Release # or cd Release-GLUT for the GL-enabled binary (required for real-time visualisations)
make clean
make all
```

Note: To allow real-time visualisation, GLUT and AntTweakBar must be installed. For those wanting more technical detail, the "`-DIMMUNEBOTS_NOGL`" switch (found in the '`src/subdir.mk`' file) is a preprocessor directive which "hides" all graphics code from the C/C++ compiler. Resulting binaries are therefore much smaller. The other preprocessor directive, "`-DIMMUNEBOTS_NOSERIALISATION`" is from legacy code which is deprecated and should be removed entirely.

## Running immunebots

The simulator needs a "`reports/`" directory in the working directory. To run in headless mode with a config file and custom run ID:

`./immunebots -d -c CONFIG_FILE`

This will create the results files (as specified in the config file) in the "`reports/`" directory. Program output will give useful information concerning output files. 


