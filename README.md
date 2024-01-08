# flake_env

Yet another direnv plugin for flakes.
This one is kind of experimental.

## Why not `nix-direnv`?

I am one of the core contributors on nix_direnv, but wanted to try a new approach.

nix-direnv got held up a bit by depending on differing behaviors of external programs not shipped with bash
(or things that *are* shipped with bash, but a differing implementation ended up in front of the bash version in PATH).

This is an attempt to simplify a bit.
I ported the nix-direnv `use_flake` function to ReasonML.
Implementing most things by hand in ReasonML is pretty simple and (more importantly) portable.
This removes *most* of the dependencies besides bash and nix.
The bash that exists is pretty portable between versions, since it only does environment manipulation.

## Installation & Usage

### Installation
These details are evolving. 
I'll make sure to update this document when things change.

For now, there is no NixOS, nix-darwin, or home-manager module that points at this tool.
The only thing you can really do is use this repo's flake as an input.
Then you'll probably need to source `${flake_env}/share/flake_env/direnvrc`. 
You should be able to do that with:

* `programs.direnv.direnvrcExtra` on NixOS
* `programs.direnv.stdlib` if using home-manager


### Usage

Example `.envrc`:

```sh
watch_file **/*.nix
use flake_env .
```

## Developing

This repo uses [just](https://just.systems/) to manage simple build jobs.
After activating the development environment in the repo, run `just -l`  to list tasks.

Please remember to format code before issuing patches.

## Credits

This takes huge inspiration (and literal code-chunks) from nix-direnv. 
Thanks to Mic92 for nix-direnv and kingarrrt for their recent contributions.

