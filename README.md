This project is a way to use Apache thrift with YARP.

## How to compile ##

    git clone git://github.com/paulfitz/yarp_thrift.git
    cd yarp_thrift
    git submodule update --init
    mkdir build && cd build && cmake .. && make

If you have the Thrift source tree, you can skip the submodule
step and set the `THRIFT_ROOT` variable.

## Programs created ##

Two programs are created:

* tcc - a copy of the thrift compiler, with a YARP generator added
* test_demo - a test of YARP communication (start YARP nameserver before running this)

## Structure ##

* The `thrift` directory should contain an unmodified, unconfigured checkout of the Apache thrift source code.
* The `compiler` directory contains the CMake logic to compile the thrift compiler, while adding in an extra generator to the set of generators thrift already has.
* The `yarp` directory contains some helper classes that are candidates for going into YARP trunk.
* The `tests` directory has material for some basic tests that compilation was ok.
* The `demo` directory shows how Thrift may be used with YARP.
    - File `demo/demo.thrift` defines an example structure and service.
    - File `demo/demo.cpp` uses the compiled classes to send some test messages over YARP.
    - The generated classes are not checked in, they will appear in `build/gen`.

## Roadmap ##

* Full coverage of types is needed (currently there's just enough for a demo).
* Generated classes should be split into .h and .cpp.
* Implementation is function dispatching should be done efficiently.
