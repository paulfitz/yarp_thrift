#include "Motor.h"
#include <stdio.h>
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

int main(int argc, char *argv[]) {
  Network yarp;
  Port port;
  port.open("/motor/client");
  yarp.connect("/motor/client","/controlboard/rpc:i");
  Motor motor;
  motor.attachPort(port);
  printf("Number of axes is %d\n", motor.get_axes());
  port.close();
  return 0;
}
