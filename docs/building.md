# Building depthmapX

## Building natively

To build depthmapX on your machine, you need the following dependencies:
- A C++ compiler that supports C++-11 or later. Tested compilers are 
  - MSVC 2015 or later (on Windows)
  - Clang (on MacOS and Linux)
  - g++ (on Linux)
- Qt 5.7 or later
- cmake 3.13 or later - this is a fairly recent version

If these dependencies are available, then building depthmapX should be
```
mkdir build
cd build
cmake ..
make
```

## Building in docker

depthmapX can be built in a docker container that provides an image including
all the required dependencies. It will build depthmapX on Unbuntu 18.04, i.e. 
it will produce a linux executable. The docker image can also be used to run 
the command line app interactively.
In order to use the docker build image, docker must be running on the computer
in use - please refer to the docker documentation at www.docker.com.

### Windows

To run the docker build on windows, use this command line:
```
docker run -v <path to your code location>\depthmapX:/mnt/code/ -it blackseamonster/depthmapx-buildenv:0.3 bash -c ci/build.sh
```

To run the build environment interactively, use
```
docker run -v <path to your code location>\depthmapX:/mnt/code/ -it blackseamonster/depthmapx-buildenv:0.3 bash
```

### Mac/Linux

To run the docker build on a Unix based system, use this command line:
```
docker run --security-opt seccomp:unconfined --user $UID -v $PWD:/mnt/code blackseamonster/depthmapx-buildenv:0.3 bash -c ci/build.sh
```

To run the build environment interactively, use:
```
docker run --security-opt seccomp:unconfined --user $UID -v $PWD:/mnt/code blackseamonster/depthmapx-buildenv:0.3 bash
```

## Using an IDE

As depthmapX uses cmake as build toolchain, any IDE that supports cmake should be usable.
