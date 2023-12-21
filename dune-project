(lang dune 3.11)

(name flake_env)

(generate_opam_files true)

(source
 (sourcehut bryan_bennett/flake_env))

(authors "Bryan Bennett")

(maintainers "Bryan Bennett")

(license MIT)

(documentation https://git.sr.ht/~bryan_bennett/flake_env)

(package
 (name flake_env)
 (allow_empty)
 (synopsis "Yet another flake plugin for direnv")
 (depends ocaml dune)
 (tags
  ("direnv" "nix" "flake")))