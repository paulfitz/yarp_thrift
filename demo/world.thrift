
// World port for simulator

struct WPoint {
1: double x;
2: double y;
3: double z;
}

struct WSize {
1: double x;
2: double y;
3: double z;
}

struct WColor {
1: double r;
2: double g;
3: double b;
}

struct Hand {
1: WPoint location;
}

service World {

  bool world_mk(1: string name, 2: WSize size, 3: WPoint location, 
		4: WColor color);
  bool world_mk_box(1: WSize size, 2: WPoint location, 3: WColor color);

  bool world_set(1: string name, 2: WPoint location);
  bool world_set_box(1: WPoint location);

  Hand world_get_lhand();
}
