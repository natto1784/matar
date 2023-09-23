{ ... }: {
  perSystem = { pkgs, src, ... }:
    let
      libraries = with pkgs; [
        (pkgs.fmt.override { enableShared = false; }).dev
        catch2_3.out
      ];
    in
    {
      packages.matar = pkgs.callPackage ./build.nix { inherit src libraries; };
      devShells.matar = pkgs.callPackage ./shell.nix { inherit libraries; };
    };
}
