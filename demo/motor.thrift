
service Motor {
  i32 get_axes();
  i32 get_name(1: i32 iAxisNumber);
  oneway void set_pos(1: i32 iAxisNumber, 2: double fPosition);
  //  list<i32> get_encs();
}
