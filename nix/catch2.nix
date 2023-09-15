{ stdenv, fetchFromGitHub, meson, ninja }:

stdenv.mkDerivation rec {
  name = "catch2";
  version = "3.4.0";

  src = fetchFromGitHub {
    owner = "catchorg";
    repo = "Catch2";
    rev = "v${version}";
    sha256 = "sha256-DqGGfNjKPW9HFJrX9arFHyNYjB61uoL6NabZatTWrr0=";
  };

  nativeBuildInputs = [ meson ninja ];

  outputs = [ "out" "dev" ];
}
