This project is a way to use Apache thrift with YARP.

It creates two programs:
* tcc - a copy of the thrift compiler, with a YARP generator added
* test_demo - a test of YARP communication (start YARP nameserver before 
running this.

How to compile:

    git clone git://github.com/paulfitz/yarp_thrift.git
    cd yarp_thrift
    git submodule update --init
    mkdir build && cd build && cmake .. && make

If you have the Thrift source tree, you can skip the submodule
step and set the THRIFT_ROOT variable.
