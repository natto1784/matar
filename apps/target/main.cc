#include "matar.hh"
#include <iostream>

int main() {
    std::cout << "Hello" << std::endl;
    if (run() > 0) {
        std::cerr << "Crashed" << std::endl;
        return 1;
    }

    return 0;
}
