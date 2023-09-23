{
  description = "matar";

  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/nixpkgs-unstable;
    flake-parts.url = github:hercules-ci/flake-parts;
  };

  outputs = inputs@{ self, nixpkgs, flake-parts }:
    flake-parts.lib.mkFlake { inherit inputs; } {

      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      imports = [
        ./nix
      ];

      perSystem = { self', system, ... }:
        let
          pkgs = import nixpkgs { inherit system; };

          src = pkgs.lib.sourceFilesBySuffices ./. [
            ".hh"
            ".cc"
            ".build"
            "meson_options.txt"
          ];
        in
        rec {
          _module.args = {
            inherit src pkgs;
          };

          formatter = pkgs.nixpkgs-fmt;
        };
    };
}
