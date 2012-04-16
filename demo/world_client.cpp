#include "world_index.h"
#include <stdio.h>
#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

int main(int argc, char *argv[]) {
  if (argc!=2) {
    fprintf(stderr,"please supply name of world port to talk to\n");
    return 1;
  }
  Network yarp;
  Port port;
  if (!port.open("/world/client")) {
    fprintf(stderr,"Failed to open a port, maybe run: yarpserver\n");
    return 1;
  }

  if (!yarp.connect("/world/client",argv[1])) {
    fprintf(stderr,"Failed to connect to target\n");
    return 1;
  }
  World world;
  world.query(port);
  world.world_mk_box(WSize(0.03,0.03,0.03),WPoint(0.3,0.2,1),WColor(1,0,0));
  world.world_mk("cyl",WSize(0.03,0.03,0.03),WPoint(0.3,1,1),WColor(0,1,0));
  Hand hand = world.world_get_lhand();
  printf("Hand location %g %g %g\n", hand.location.x,
	 hand.location.y,
	 hand.location.z);
  port.close();
  return 0;
}
