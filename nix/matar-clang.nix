{ ... }: {
  perSystem = { pkgs, src, ... }:
    let
      llvm = pkgs.llvmPackages_18;
      stdenv = llvm.libcxxStdenv;

      libraries = with pkgs; [
        (catch2_3.override { inherit stdenv; }).out
      ];
    in
    {
      packages.matar-clang = pkgs.callPackage ./build.nix { inherit src libraries stdenv; };
      devShells.matar-clang = pkgs.callPackage ./shell.nix {
        inherit libraries stdenv;
        tools = with pkgs; [ (clang-tools_18.override { enableLibcxx = true; }) ];
      };
    };
}
