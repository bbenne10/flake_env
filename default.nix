{ alcotest
, bisect_ppx
, buildDunePackage
, core
, core_unix
, coreutils
, findlib
, lib
, nix-filter
, ocaml
, ppx_yojson_conv
, ppx_yojson_conv_lib
, re
, reason
, sha
}:
buildDunePackage {
  pname = "flake_env";
  version = "0.1";
  src = nix-filter {
    root = ./.;
    include = [
      "bin"
      "lib"
      "tests"
      ./dune-project
      ./flake.nix
      ./default.nix
      ./flake.lock
      ./flake_env.opam
      ./direnvrc
      ./LICENSE
    ];
  };
  duneVersion = "3";
  doCheck = true;
  postPatch = ''
    substituteInPlace direnvrc --replace "@flake_env@" "$out/bin/flake_env"
    substituteInPlace tests/spit*.sh --replace "/usr/bin/env" "${coreutils}/bin/env"
  '';
  postInstall = ''
    install -m400 -D direnvrc $out/share/flake_env/direnvrc
  '';
  checkInputs = [
    alcotest
    bisect_ppx
  ];
  nativeBuildInputs = [
    reason
  ];
  propagatedBuildInputs = [
    core
    core_unix
    findlib
    ocaml
    ppx_yojson_conv
    ppx_yojson_conv_lib
    re
    sha
  ];

  meta = with lib; {
    description = "Yet another flake plugin for direnv";
    homepage = "https://git.sr.ht/~bryan_bennett/flake_env";
    license = licenses.mit;
    platforms = platforms.unix;
  };
}
