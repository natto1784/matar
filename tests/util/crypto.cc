#include "util/crypto.hh"
#include <catch2/catch_test_macros.hpp>

static constexpr auto TAG = "[util][crypto]";

TEST_CASE("sha256 matar", TAG) {
    std::array<uint8_t, 5> data = { 'm', 'a', 't', 'a', 'r' };

    CHECK(crypto::sha256(data) ==
          "3b02a908fd5743c0e868675bb6ae77d2a62b3b5f7637413238e2a1e0e94b6a53");
}

TEST_CASE("sha256 forgis", TAG) {
    std::array<uint8_t, 32> data = { 'i', ' ', 'p', 'u', 't', ' ', 't', 'h',
                                     'e', ' ', 'n', 'e', 'w', ' ', 'f', 'o',
                                     'r', 'g', 'i', 's', ' ', 'o', 'n', ' ',
                                     't', 'h', 'e', ' ', 'j', 'e', 'e', 'p' };

    CHECK(crypto::sha256(data) ==
          "cfddca2ce2673f355518cbe2df2a8522693c54723a469e8b36a4f68b90d2b759");
}
