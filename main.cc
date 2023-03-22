#include "pico/stdlib.h"
#include <iostream>

int main() {
  stdio_init_all();
  while (true) {
    std::cout << "sup" << std::endl;
    sleep_ms(1000);
  }
  return 0;
}
