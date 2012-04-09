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
  Port port;
  port.open("/motor/server");
  MotorImpl motor;
  motor.attachPort(port);
  while (true) {
    printf("Motor server running happily\n");
    Time::delay(10);
  }

  port.close();
  return 0;
}
