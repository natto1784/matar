{ stdenv
, mkShell
, meson
, ninja
, pkg-config
, libraries ? [ ]
, tools ? [ ]
}:

mkShell.override { inherit stdenv; } {
  name = "matar";

  packages = [
    meson
    ninja
    pkg-config
  ] ++ libraries ++ tools;

  enableParallelBuilding = true;
}
