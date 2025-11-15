// Horcrux - Hello World Example
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include <iostream>

auto main() -> int {
  std::cout << "Hello from Horcrux Build System!\n";
  std::cout << "This is a simple example target.\n";
#include <iostream>

#include "lib.h"

int main() {
  std::cout << "Hello from Horcrux!\n";
  greet("World");
  return 0;
}
