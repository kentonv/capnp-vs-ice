#include "calc.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/debug.h>
#include <iostream>

// Some helpers needed to deal with unwrapping `Value` objects and then re-wrapping the result.
// This is needed because at present only capabilities (interface implementations) can be carried
// over from a response to a pipelined request, so we wrap every result in a capability.  We expect
// a future version to include syntax sugar that simplifies this, but the end result is the same.
struct ResolvedInputs {
  int left;
  int right;
};
kj::Promise<int> resolveInput(Calculator::Input::Reader reader) {
  switch (reader.which()) {
    case Calculator::Input::LITERAL:
      return reader.getLiteral();
    case Calculator::Input::VALUE:
      return reader.getValue().readRequest().send().then(
          [](capnp::Response<Calculator::Value::ReadResults>&& response) {
        return response.getValue();
      });
  }
  KJ_FAIL_ASSERT("Unknown input type.");
}
kj::Promise<ResolvedInputs> resolveInputs(Calculator::InputPair::Reader input) {
  auto pair = kj::heapArrayBuilder<kj::Promise<int>>(2);
  pair.add(resolveInput(input.getLeft()));
  pair.add(resolveInput(input.getRight()));
  return kj::joinPromises(pair.finish()).then(
      [](kj::Array<int>&& values) {
    return ResolvedInputs { values[0], values[1] };
  });
}
class ValueImpl: public Calculator::Value::Server {
public:
  ValueImpl(int value): value(value) {}

  kj::Promise<void> read(ReadContext context) {
    context.getResults().setValue(value);
    return kj::READY_NOW;
  }

private:
  int value;
};

// OK, here's the actual server impl.
class CalculatorImpl: public Calculator::Server {
public:
  kj::Promise<void> add(AddContext context) {
    return resolveInputs(context.getParams()).then(
        [context](ResolvedInputs inputs) mutable {
      context.getResults().setValue(kj::heap<ValueImpl>(inputs.left + inputs.right));
    });
  }
  kj::Promise<void> sub(AddContext context) {
    return resolveInputs(context.getParams()).then(
        [context](ResolvedInputs inputs) mutable {
      context.getResults().setValue(kj::heap<ValueImpl>(inputs.left - inputs.right));
    });
  }
  kj::Promise<void> mult(AddContext context) {
    return resolveInputs(context.getParams()).then(
        [context](ResolvedInputs inputs) mutable {
      context.getResults().setValue(kj::heap<ValueImpl>(inputs.left * inputs.right));
    });
  }
  kj::Promise<void> div(AddContext context) {
    return resolveInputs(context.getParams()).then(
        [context](ResolvedInputs inputs) mutable {
      context.getResults().setValue(kj::heap<ValueImpl>(inputs.left / inputs.right));
    });
  }
};

int main(int argc, char* argv[]) {
  capnp::EzRpcServer server(argc == 2 ? argv[1] : "localhost", 10000);
  server.exportCap("calc", kj::heap<CalculatorImpl>());
  kj::NEVER_DONE.wait(server.getWaitScope());
}
