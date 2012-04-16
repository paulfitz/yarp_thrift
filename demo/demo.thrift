
struct PointD {
1: i32 x;
2: i32 y;
3: i32 z;
}

service Demo {
  i32 add_one(1:i32 x);
  i32 double_down(1: i32 x);
  string add_x(1: string x);
  PointD add_point(1: PointD x, 2: PointD y);
}
