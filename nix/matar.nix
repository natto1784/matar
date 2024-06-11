{ ... }: {
  perSystem = { pkgs, src, ... }:
    let
      stdenv = pkgs.gcc14Stdenv;

      libraries = with pkgs; [
        (catch2_3.override { inherit stdenv; }).out
      ];
    in
    {
      packages.matar = pkgs.callPackage ./build.nix { inherit src libraries stdenv; };
      devShells.matar = pkgs.callPackage ./shell.nix { inherit libraries stdenv; };
    };
}
