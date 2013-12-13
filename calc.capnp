@0xa974578e46a55e21;

interface Calculator {
  # This interface is defined a bit oddly because currently the only parts
  # of an RPC result that can be used in pipelining are the capabilities
  # (interface references).  If we returned a raw int, we couldn't pipeline
  # on it.  So, we wrap each result in a `Value` capability, and also accept
  # these as inputs.  A future version may add syntax sugar to simplify this.
  # In practice, though, it's far more common to want to pipeline on things
  # that are already capabilities anyway, not primitive values.

  add @0 InputPair -> Output;
  sub @1 InputPair -> Output;
  mult @2 InputPair -> Output;
  div @3 InputPair -> Output;

  struct InputPair {
    left @0 :Input;
    right @1 :Input;
  }

  struct Input {
    union {
      literal @0 :Int32;
      value @1 :Value;
    }
  }

  struct Output {
    # To allow pipelining, we return this interface rather than a raw
    # primitive value.
    value @0 :Value;
  }

  interface Value {
    read @0 () -> (value :Int32);
  }
}
