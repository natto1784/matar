{
  description = "matar";
  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/nixpkgs-unstable;
  };
  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      eachSystem = with nixpkgs.lib; f: foldAttrs mergeAttrs { }
        (map (s: mapAttrs (_: v: { ${s} = v; }) (f s)) systems);
    in
    eachSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # aliases
        llvm = pkgs.llvmPackages_16;
        stdenv = llvm.libcxxStdenv;

        # packages
        catch2_v3 = pkgs.callPackage ./nix/catch2.nix { inherit stdenv; };

        #dependencies
        nativeBuildInputs = with pkgs; [
          meson
          ninja

          # libraries
          pkg-config
          fmt.dev
          catch2_v3.dev
        ];
      in
      rec {
        packages = rec {
          matar = stdenv.mkDerivation rec {
            name = "matar";
            version = "0.1";
            src = pkgs.lib.sourceFilesBySuffices ./. [
              ".hh"
              ".cc"
              ".build"
            ];
            outputs = [ "out" "dev" ];

            inherit nativeBuildInputs;

            enableParallelBuilding = true;
          };
          default = matar;
        };

        devShells = rec {
          matar = pkgs.mkShell.override { inherit stdenv; } {
            name = "matar";
            packages = nativeBuildInputs ++ (with pkgs; [
              llvm.libcxx

              # dev tools
              clang-tools_16
            ]);
          };
          default = matar;
        };

        formatter = pkgs.nixpkgs-fmt;
      });
}
