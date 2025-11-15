// Math Utilities Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "math_utils.h"

namespace horcrux::examples {

int factorial(int n) {
  if (n <= 1) {
    return 1;
  }
  int result = 1;
  for (int i = 2; i <= n; ++i) {
    result *= i;
  }
  return result;
}

bool is_prime(int n) {
  if (n <= 1) {
    return false;
  }
  if (n <= 3) {
    return true;
  }
  if (n % 2 == 0 || n % 3 == 0) {
    return false;
  }
  for (int i = 5; i * i <= n; i += 6) {
    if (n % i == 0 || n % (i + 2) == 0) {
      return false;
    }
  }
  return true;
}

int fibonacci(int n) {
  if (n <= 1) {
    return n;
  }
  int a = 0;
  int b = 1;
  for (int i = 2; i <= n; ++i) {
    int temp = a + b;
    a = b;
    b = temp;
  }
  return b;
}

} // namespace horcrux::examples
