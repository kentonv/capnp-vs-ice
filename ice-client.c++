#include <Ice/Ice.h>
#include "calc.h"
#include <iostream>

using namespace Benchmark;
using namespace std;

int main(int argc, char* argv[]) {
  cout << "setup..." << endl;

  Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
  CalculatorPrx calc = CalculatorPrx::checkedCast(
      communicator->stringToProxy("calculator:tcp -h localhost -p 9999"));

  // We calculate the following expression.  The variable names correspond to the operator they
  // appear under here:
  // ((5 * 2) + ((7 - 3) * 10)) / (6 - 4)
  //     a    e     b    d      f    c
  // The result should be 25.

  cout << "starting requests..." << endl;

  auto a = calc->begin_mult(5, 2);
  auto b = calc->begin_sub(7, 3);
  auto c = calc->begin_sub(6, 4);
  auto d = calc->begin_mult(calc->end_sub(b), 10);
  auto e = calc->begin_add(calc->end_mult(a), calc->end_mult(d));
  auto f = calc->begin_div(calc->end_add(e), calc->end_sub(c));

  int result = calc->end_div(f);

  cout << "result = " << result << endl;

  communicator->destroy();

  return 0;
}
