{ buildDunePackage
, lib
, core
, core_unix
, findlib
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
  src = ./.;
  duneVersion = "3";
  postPatch = ''
    substituteInPlace direnvrc --replace "flake_env" "$out/bin/flake_env"
  '';
  postInstall = ''
    install -m400 -D direnvrc $out/share/flake_env/direnvrc
  '';
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
