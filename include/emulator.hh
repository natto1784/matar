#ifndef EMULATOR_HH
#define EMULATOR_HH

// Why do I have a public API? We will know that in the future

#include <string>

namespace emulator {
void
run(std::string filepath);

void
run(std::ifstream& ifile);
}

#endif /* EMULATOR_HH */
