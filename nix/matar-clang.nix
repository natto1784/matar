{ ... }: {
  perSystem = { pkgs, src, ... }:
    let
      llvm = pkgs.llvmPackages_16;
      stdenv = llvm.libcxxStdenv;

      libraries = with pkgs; [
        ((pkgs.fmt.override {
          inherit stdenv;
          enableShared = false;
        }).overrideAttrs (oa: {
          cmakeFlags = oa.cmakeFlags ++ [ "-DFMT_TEST=off" ];
        })).dev

        (catch2_3.override { inherit stdenv; }).out
      ];
    in
    {
      packages.matar-clang = pkgs.callPackage ./build.nix { inherit src libraries stdenv; };
      devShells.matar-clang = pkgs.callPackage ./shell.nix {
        inherit libraries stdenv;
        tools = with pkgs; [ (clang-tools_16.override { enableLibcxx = true; }) ];
      };
    };
}
