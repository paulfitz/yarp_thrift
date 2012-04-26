
struct Nummy {
1: i32 num1 = 0;
2: i32 num2;
}


service Ziggy {
  void ping();
  i32 compute(1:i32 id, 2:Nummy nummy);
  oneway void stream();
}
