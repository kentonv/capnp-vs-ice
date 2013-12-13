#include <Ice/Ice.h>
#include "calc.h"

using namespace Benchmark;

class CalculatorImpl: public Calculator {
public:
  int add(int a, int b, const ::Ice::Current& = ::Ice::Current()) override {
    return a + b;
  }
  int sub(int a, int b, const ::Ice::Current& = ::Ice::Current()) override {
    return a - b;
  }
  int mult(int a, int b, const ::Ice::Current& = ::Ice::Current()) override {
    return a * b;
  }
  int div(int a, int b, const ::Ice::Current& = ::Ice::Current()) override {
    return a / b;
  }
};

int main(int argc, char* argv[])
{
  Ice::CommunicatorPtr communicator = Ice::initialize(argc, argv);
  Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints("Calculator", "tcp -h localhost -p 10000");
  adapter->add(new CalculatorImpl, communicator->stringToIdentity("calculator"));
  adapter->activate();
  communicator->waitForShutdown();
  communicator->destroy();

  return 0;
}
