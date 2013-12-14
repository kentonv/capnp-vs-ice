# Dependent Calls:  Cap'n Proto vs. Thrift vs. Ice

_**Note:**  Only tested on Linux, though "should" work on others._

This is a test/benchmark designed to demonstrate how promise pipelining can
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
uses promises, and Ice supports something future-like.  Thrift is a little
stranger in that its calls are FIFO, but for the purpose of this test that
allows us to parallelize just as well as Ice.

The test measures the number of network round trips taken to complete the
operation.

To run the test, first install Thrift, Ice and Cap'n Proto, then simply run:

    ./runtest.sh

As expected, Thrift and Ice take four round trips.

Cap'n Proto, however, takes **only one** round trip, due to promise
pipelining.

To understand how this works, [see the docs](http://kentonv.github.io/capnproto/rpc.html).
