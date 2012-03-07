#include <stdio.h>
#include "Demo.h"

#include <yarp/os/all.h>
using namespace yarp::os;
using namespace std;

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
  demo.attachPort(client_port);
  int ct = 1;
  int seq = 1;
  string xs = "";
  while (true) {
    ct = demo.add_one(ct);
    seq = demo.double_down(seq);
    xs = demo.add_x(xs);
    printf("Count %d / sequence %d / xs %s\n", ct, seq, xs.c_str());
    Time::delay(1);
  }
  return 0;
}
