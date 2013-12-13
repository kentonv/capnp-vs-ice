#include "calc.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/debug.h>
#include <iostream>

using namespace std;

// Some helpers to deal with initializing requests.  These are needed at present partly to enable
// pipelining (pipelined results will be in the form of a RemotePromise rather than an integer)
// and partly because Cap'n Proto's API requires you to use a Builder to construct a request
// in-place (to avoid copies) rather than use regular parameter passing when calling a method.
// We expect to improve both of these issues in a future version, so the API will become simpler
// and these helpers will no longer be needed (although people wanting ultimate zero-copy
// performance may still want to do it this way).
void initOperand(Calculator::Input::Builder builder, int i) {
  builder.setLiteral(i);
}
void initOperand(Calculator::Input::Builder builder, capnp::RemotePromise<Calculator::Output> out) {
  builder.setValue(out.getValue());
}
template <typename Left, typename Right>
capnp::RemotePromise<Calculator::Output> initRequest(
    capnp::Request<Calculator::InputPair, Calculator::Output> input,
    Left&& left, Right&& right) {
  initOperand(input.getLeft(), kj::fwd<Left>(left));
  initOperand(input.getRight(), kj::fwd<Right>(right));
  return input.send();
}

int main(int argc, char* argv[]) {
  cout << "setup..." << endl;

  capnp::EzRpcClient client(argc == 2 ? argv[1] : "localhost", 9999);

  Calculator::Client calc = client.importCap<Calculator>("calc");

  // We calculate the following expression.  The variable names correspond to the operator they
  // appear under here:
  // ((5 * 2) + ((7 - 3) * 10)) / (6 - 4)
  //     a    e     b    d      f    c
  // The result should be 25.

  cout << "starting requests..." << endl;

  auto a = initRequest(calc.multRequest(), 5, 2);
  auto b = initRequest(calc.subRequest(), 7, 3);
  auto c = initRequest(calc.subRequest(), 6, 4);
  auto d = initRequest(calc.multRequest(), kj::mv(b), 10);
  auto e = initRequest(calc.addRequest(), kj::mv(a), kj::mv(d));
  auto f = initRequest(calc.divRequest(), kj::mv(e), kj::mv(c));

  int result = f.getValue().readRequest().send().wait(client.getWaitScope()).getValue();

  cout << "result = " << result << endl;
}
