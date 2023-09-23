{ ... }: {
  imports = [
    ./matar.nix
    ./matar-clang.nix
  ];

  perSystem = { self', pkgs, ... }: {
    packages.default = self'.packages.matar-clang;
    devShells.default = self'.devShells.matar-clang;
  };
}
