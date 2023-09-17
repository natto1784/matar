nothing to be seen here yet. LEAVE

But if you are curious (probably not), read ahead

# Dependencies
## Tested toolchains

- LLVM 16.0.6
- GCC 12.3.0

In theory, any toolchain supporting at least the C++20 standard should work.
I am using LLVM's clang and libcxx as the primary toolchain.

## Static libraries

| Name   | Version   | Required? |
|:------:|:----------|:---------:|
| fmt    | >= 10.1.1 | yes       |
| catch2 | >= 3.4    | for tests |

This goes without saying but using a different toolchain to compile these libraries before linking probably won't work.
I will add meson wrap support once LLVM 17 is out, since I want to get rid of fmt.
