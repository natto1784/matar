{ stdenv
, meson
, ninja
, pkg-config
, src ? "../."
, libraries ? [ ]
}:

stdenv.mkDerivation {
  name = "matar";
  version = "0.1";
  inherit src;

  outputs = [ "out" "dev" ];

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
  ] ++ libraries;

  enableParallelBuilding = true;
}
