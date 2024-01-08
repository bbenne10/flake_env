open Core;
module Unix = Core_unix;

module Util = Flake_env__util;

type t = {
  major: int,
  minor: int,
  point: int,
};

let init = (major, minor, point) => {major, minor, point};

let required_direnv_version = init(2, 21, 3);

let required_nix_version = init(2, 10, 0);

let semver_re = Re.compile(Re.Posix.re({|([0-9]+)\.([0-9]+)\.([0-9]+)|}));

let compare = (a, b) => {
  switch (a, b) {
  | (a, b) when a.major == b.major && a.minor == b.minor && a.point == b.point => 0
  | (a, b) when a.major < b.major => (-1)
  | (a, b) when a.major == b.major && a.minor < b.minor => (-1)
  | (a, b) when a.major == b.major && a.minor == b.minor && a.point < b.point => (-1)
  | _ => 1
  };
};

let extract_version_number = cmd => {
  switch (Util.run_process(cmd, ["--version"])) {
  | (Ok (), stdout) when String.length(stdout) > 0 =>
    switch (Re.exec(semver_re, stdout)) {
    | exception Stdlib.Not_found =>
      Error(
        Printf.sprintf(
          "Stdout did not contain a version number for `%s --version`",
          cmd,
        ),
      )
    | substrings =>
      let groups = Re.Group.all(substrings);
      Ok({
        major: groups[1] |> int_of_string,
        minor: groups[2] |> int_of_string,
        point: groups[3] |> int_of_string,
      });
    }
  | _ => Error(Printf.sprintf("Failed executing '%s'", cmd))
  };
};

let is_new_enough = (cur, needed) => {
  switch (cur) {
  | Ok(cur) =>
    switch (compare(cur, needed)) {
    | x when x < 0 => Ok(false)
    | _ => Ok(true)
    }
  | Error(e) => Error(e)
  };
};

let in_direnv = () =>
  switch (Sys.getenv("direnv")) {
  | Some(_) => true
  | None => false
  };

let preflight_versions = () => {
  let is_nix_new_enough =
    is_new_enough(extract_version_number("nix"), required_nix_version);
  let is_direnv_new_enough =
    is_new_enough(extract_version_number("direnv"), required_direnv_version);

  switch (in_direnv(), is_direnv_new_enough, is_nix_new_enough) {
  | (false, _, _) => Error("Not in direnv!")
  | (_, Ok(false), _) => Error("Direnv version is not new enough")
  | (_, _, Ok(false)) => Error("Nix version is not new enough")
  | (_, Error(e), _) => Error(e)
  | (_, _, Error(e)) => Error(e)
  | (true, Ok(true), Ok(true)) => Ok()
  };
};
