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
        "i686-linux"
      ];
      eachSystem = with nixpkgs.lib; f: foldAttrs mergeAttrs { }
        (map (s: mapAttrs (_: v: { ${s} = v; }) (f s)) systems);
    in
    eachSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages;
        stdenv = llvm.libcxxStdenv;

        nativeBuildInputs = with pkgs; [ meson ninja ];
      in
      {
        packages = rec {
          matar = stdenv.mkDerivation rec {
            name = "matar";
            version = "0.1";
            src = pkgs.lib.sourceFilesBySuffices ./. [
              ".hh"
              ".cc"
              ".build"
            ];
            outputs = [ "dev" "out" ];

            inherit nativeBuildInputs;

            enableParallelBuilding = true;
          };
          default = matar;
        };

        devShells = rec {
          matar = pkgs.mkShell.override { inherit stdenv; } {
            name = "matar";
            packages = nativeBuildInputs ++ (with pkgs; [
              clang-tools
            ]);
          };
          default = matar;
        };

        formatter = pkgs.nixpkgs-fmt;
      });
}
