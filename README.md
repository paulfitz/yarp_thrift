This project is a way to compile Apache thrift with CMake.

How to use:

    mkdir build && ccmake ..
    point THRIFT_ROOT to thrift source code
    make

If you are not familiar with CMake, try:

    mkdir thrift_me
    cd thrift_me
    svn co http://svn.apache.org/repos/asf/thrift/trunk thrift
    git clone git@github.com:paulfitz/thrift_cmake.git
    mkdir build
    cd build
    cmake ../thrift_cmake
    make
    ./tcc
