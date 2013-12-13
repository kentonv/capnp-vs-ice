# To build, you'll need to install ZeroC Ice's C++ as well as Cap'n Proto.

all: ice-client ice-server capnp-client capnp-server fake-latency

clean:
	rm -f calc.h calc.cpp ice-client ice-server calc.capnp.h calc.capnp.c++ capnp-client capnp-server fake-latency

calc.cpp: calc.ice
	slice2cpp calc.ice

ice-client: ice-client.c++ calc.cpp
	g++ -std=c++11 -O2 -Wall -I. calc.cpp ice-client.c++ -o ice-client -lIce -lIceUtil -pthread

ice-server: ice-server.c++ calc.cpp
	g++ -std=c++11 -O2 -Wall -I. calc.cpp ice-server.c++ -o ice-server -lIce -lIceUtil -pthread

calc.capnp.c++: calc.capnp
	capnp compile -oc++ calc.capnp

capnp-client: capnp-client.c++ calc.capnp.c++
	g++ -std=c++11 -O2 -Wall capnp-client.c++ calc.capnp.c++ -o capnp-client `pkg-config --cflags --libs capnp-rpc`

capnp-server: capnp-server.c++ calc.capnp.c++
	g++ -std=c++11 -O2 -Wall capnp-server.c++ calc.capnp.c++ -o capnp-server `pkg-config --cflags --libs capnp-rpc`

fake-latency: fake-latency.c++
	g++ -std=c++11 -O2 -Wall fake-latency.c++ -o fake-latency -pthread

.PHONY: all clean
