
namespace cpp tutorial

struct Test {
1: i32 num1 = 0;
2: i32 num2;
}


service TestService {
  void ping();
  i32 compute(1:i32 id, 2:Test test);
  oneway void stream();
}
