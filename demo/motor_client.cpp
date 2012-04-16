#include "Motor.h"
#include <stdio.h>
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

int main(int argc, char *argv[]) {
  if (argc!=2) {
    fprintf(stderr,"please supply name of rpc port to talk to\n");
    return 1;
  }
  Network yarp;
  Port port;
  if (!port.open("/motor/client")) {
    fprintf(stderr,"Failed to open a port, maybe run: yarpserver\n");
    return 1;
  }

  //yarp.connect("/motor/client",argv[1],"text_ack");
  yarp.connect("/motor/client",argv[1]);
  Motor motor;
  motor.query(port);
  int nj = motor.get_axes();

  vector<double> poss;
  for (int i=0; i<nj; i++) {
    poss.push_back(0.5*i);
  }
  motor.set_poss(poss);
  for (int i=0; i<10; i++) {
    motor.link().stream();
    motor.set_pos(0,i);
    motor.link().query();
    double v = motor.get_enc(0);
    printf("Motor %d at %g of %d axes\n", 0, v, motor.get_axes());
    vector<double> all = motor.get_encs();
    printf("  all encs:");
    for (int i=0; i<(int)all.size(); i++) {
      printf(" %d:%g", i, all[i]);
    }
    printf("\n");
  }

  port.close();
  return 0;
}
