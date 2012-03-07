#include <stdio.h>
#include "Point.h"
#include "Demo.h"
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

// see: demo.thrift
class RemoteDemo : public Demo {
public:
  virtual int32_t add_one(const int32_t x) {
    printf("Server::add_one called with %d\n", x);
    return x+1;
  }

  virtual int32_t double_down(const int32_t x) {
    printf("Server::double_down called with %d\n", x);
    return x*2;
  }

  virtual string add_x(const string& x) {
    printf("Server::add_x called with %s\n", x.c_str());
    return x + "x";
  }

  virtual Point add_point(const Point& x, const Point& y) {
    printf("Server::add_point called\n");
    Point z;
    z.x = x.x + y.x;
    z.y = x.y + y.y;
    z.z = x.z + y.z;
    return z;
  }
};

int main(int argc, char *argv[]) {
  Network yarp;
  Port server_port, client_port;
  server_port.open("/demo");
  client_port.open("/demo/client");
  yarp.connect("/demo/client","/demo");
  RemoteDemo server;
  server.attachPort(server_port);
  Demo demo;
  Point point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  Point offset;
  offset.x = 1;
  offset.y = 2;
  offset.z = 3;
  demo.attachPort(client_port);
  int ct = 1;
  int seq = 1;
  string xs = "";
  while (true) {
    ct = demo.add_one(ct);
    seq = demo.double_down(seq);
    xs = demo.add_x(xs);
    point = demo.add_point(point,offset);
    printf("Count %d / sequence %d / xs %s / point %d %d %d\n", ct, seq, xs.c_str(), point.x, point.y, point.z);
    Time::delay(1);
  }
  return 0;
}
