build:
	@dune build

test:
	@dune test -f
	@bisect-ppx-report html
	@bisect-ppx-report summary --per-file

fmt:
	@dune build @fmt --auto-promote
