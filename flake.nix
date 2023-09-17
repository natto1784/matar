{
  description = "matar";

  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/master;
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


        # TODO: this is ugly
        #dependencies
        nativeBuildInputs = with pkgs;
          [
            meson
            ninja

            # libraries
            pkg-config
            cmake

            ((pkgs.fmt.override {
              inherit stdenv;
              enableShared = false;
            }).overrideAttrs (oa: {
              cmakeFlags = oa.cmakeFlags ++ [ "-DFMT_TEST=off" ];
            })).dev
            (catch2_3.override { inherit stdenv; }).out
          ];
      in
      rec {
        packages = rec {
          inherit (llvm) libcxxabi;
          matar = stdenv.mkDerivation rec {
            name = "matar";
            version = "0.1";
            src = pkgs.lib.sourceFilesBySuffices ./. [
              ".hh"
              ".cc"
              ".build"
              "meson_options.txt"
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
              # lsp
              clang-tools_16
            ]);
          };
          default = matar;
        };

        formatter = pkgs.nixpkgs-fmt;
      });
}
