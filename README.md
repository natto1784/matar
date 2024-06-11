nothing to be seen here yet. LEAVE

But if you are curious (probably not), read ahead

# Dependencies
## Tested toolchains

- LLVM 18.1.7
- GCC 14.1.0

In theory, any toolchain supporting at least the c++23 standard should work.
I am using LLVM's clang and libcxx as the primary toolchain.

## Static libraries

| Name   | Version   | Required? |
|:------:|:----------|:---------:|
| catch2 | >= 3.4    | for tests |

This goes without saying but using a different toolchain to compile these libraries before linking probably won't work.

-----

# LOG
- June 11, 2024: After almost an year, I have come back to this silly abandoned project, will probably complete it soon.
