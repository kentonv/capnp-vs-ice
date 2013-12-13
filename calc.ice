#pragma once

module Benchmark {
  interface Calculator
  {
    idempotent int add(int a, int b);
    idempotent int sub(int a, int b);
    idempotent int mult(int a, int b);
    idempotent int div(int a, int b);
  };
};
