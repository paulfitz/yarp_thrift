#include "Motor.h"
#include <stdio.h>
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

// see: motor.thrift
class MotorImpl : public Motor {
public:
  double x[2];
  
  MotorImpl() {
    x[0] = x[1] = 0.0;
  }

  virtual int32_t get_axes() {
    return 2;
  }

  virtual bool set_pos(const int32_t iAxisNumber, const double fPosition) {
    if (iAxisNumber<0||iAxisNumber>1) return false;
    x[iAxisNumber] = fPosition;
    return true;
  }

  virtual double get_enc(const int32_t iAxisNumber) {
    if (iAxisNumber<0||iAxisNumber>1) return 0;
    return x[iAxisNumber];
  }

  virtual std::vector<double> get_encs() {
    std::vector<double> result(2);
    result[0] = x[0];
    result[1] = x[1];
    return result;
  }

  virtual bool set_poss(const std::vector<double> & poss) {
    if (poss.size()!=2) return false;
    x[0] = poss[0];
    x[1] = poss[1];
    return true;
  }
};

int main(int argc, char *argv[]) {
  Network yarp;
  Port portRpc, portCommand, portState;
  if (!portRpc.open("/motor/server/rpc:i")) {
    fprintf(stderr,"Failed to open a port, maybe run: yarpserver\n");
    return 1;
  }
  portCommand.open("/motor/server/command:i");
  portState.open("/motor/server/state:o");
  MotorImpl motor;
  motor.serve(portRpc);
  //motor.serve(portCommand);
  double start = Time::now();
  while (true) {
    double now = Time::now();
    if (now-start>10) {
      printf("Motor server running happily\n");
      start += 10;
      if (now-start>10) start = now;
    }
    // stream motor state every now and then
    Bottle b;
    b.addDouble(motor.get_enc(0));
    b.addDouble(motor.get_enc(1));
    portState.write(b);
    Time::delay(0.02);
  }
  return 0;
}
