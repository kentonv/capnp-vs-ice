# Dependent Calls:  Cap'n Proto vs. Ice

Is is a test/benchmark designed to demonstrate how promise pipelining can
significantly reduce latency.

The test defines, using each framework, a server that exports a simple
four-function calculator interface.  The interface simply has methods
`add()`, `sub()`, `mult()`, and `div()`, each taking two integers and
returning an integer.

The client programs each attempt to use this interface to compute the
following expression:

    ((5 * 2) + ((7 - 3) * 10)) / (6 - 4)

Notice that there are six arithmetic operations in the expression, but
with parallelization we can complete it in four steps:

    ((5 * 2) + ((7 - 3) * 10)) / (6 - 4)
      (10    + (   4    * 10)) /    2
      (10    +         40)     /    2
            50                 /    2
                              25

*Both* Ice and Cap'n Proto easily support parallelization.  Cap'n Proto
uses promises, and Ice supports something future-like.

The test measures the number of network round trips taken to complete the
operation.

To run the test, first install Ice and Cap'n Proto, then simply run:

    ./runtest.sh

As expected, Ice takes four round trips.

Cap'n Proto, however, takes **only one** round trip, due to promise
pipelining.

To understand how this works, [see the docs](http://kentonv.github.io/capnproto/rpc.html).

## Why Ice?

I chose Ice for this test simply because it is an RPC framework that
supports C++, code generation, and asynchronous calls.  Ice is,
unfortunately, proprietary software (though apparently available free
of charge).  I wanted to use something open source, but several options
I looked at weren't quite right:

* **Protocol Buffers:**  No official C++ RPC implementation.  Third-party
  options exist but it's unclear which ones can be considered worthy
  competitors vs. random hobby projects.
* **Thrift:**  Only supports synchronous calls in C++ clients.  I would
  have had to use threads to make this a remotely fair test, which would
  have been painful.
* **Msgpack:**  Does not support schemas / code generation.
* **Avro:**  Does not support RPC in C++.
