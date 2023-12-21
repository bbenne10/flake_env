{
  description = "Yet another flake plugin for direnv";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs = inputs @ { flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; }
      ({ lib, ... }: {
        systems = [
          "aarch64-linux"
          "x86_64-linux"

          "x86_64-darwin"
          "aarch64-darwin"
        ];
        perSystem = { config, pkgs, self', ... }: {
          packages = {
            flake_env = pkgs.ocamlPackages.callPackage ./default.nix { };
            default = config.packages.flake_env;
          };
          devShells.default = pkgs.mkShell {
            inputsFrom = [ self'.packages.default ];
            packages = [
              pkgs.ocamlPackages.dune_3
              pkgs.ocamlPackages.findlib
              pkgs.ocamlPackages.ocaml
              pkgs.ocamlPackages.ocaml-lsp
              pkgs.ocamlPackages.ocamlformat
              pkgs.ocamlPackages.ocamlformat-rpc-lib
              pkgs.ocamlPackages.utop
            ];
          };
        };
        flake = {
          overlays.default = final: _prev: {
            flake_env = final.ocamlPackages.callPackage ./default.nix { };
          };
        };
      });
}
