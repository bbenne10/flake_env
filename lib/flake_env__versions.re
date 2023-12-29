open Core;
module Unix = Core_unix;

type t = {
  major: int,
  minor: int,
  point: int,
};

let semver_re = Re.compile(Re.Posix.re({|([0-9]+)\.([0-9]+)\.([0-9]+)|}));

let required_direnv_version = {
  major: 2,
  minor: 21,
  point: 3
};

let required_nix_version = {
  major: 2,
  minor: 10,
  point: 0
};

// TODO: test this
let compare_version_number = (a, b) => {
  switch (a, b) {
  | (a, b) when a.major == b.major && a.minor == b.minor && a.point == b.point => 0
  | (a, b) when a.major < b.major => -1
  | (a, b) when a.major == b.major && a.minor < b.minor => -1
  | (a, b) when a.major == b.major && a.minor == b.minor && a.point < b.point => -1
  | _ => 1
  }
}

let%test_unit "compare_version_number" = [%test_eq: int](compare_version_number({major: 1, minor: 0, point: 0}, {major: 2, minor: 0, point: 0}), -1);

let extract_version_number = (cmd) => {
  let full_cmd = cmd ++ " --version";
  switch (Core_unix.open_process_in(full_cmd) |> In_channel.input_line) {
  | Some(stdout) => {
      let substrings = Re.exec(semver_re, stdout);
      let groups = Re.Group.all(substrings);
      Ok({
        major: groups[1] |> int_of_string,
        minor: groups[2] |> int_of_string,
        point: groups[3] |> int_of_string
      })
    }
  | None => Error(Printf.sprintf("Failed executing '%s'\n", cmd))
  }
};

let is_version_new_enough = (cur, needed) => {
  switch (cur) {
    | Ok(cur) => {
        switch (compare_version_number(cur, needed)) {
        | x when x < 0 => Ok(false)
        | _ => Ok(true)
        }
      }
    | Error(e) => Error(e)
  }
}

let preflight_versions = () => {
  let in_direnv = switch (Sys.getenv("direnv")) {
    | Some(_) => true
    | None => false
  };

  let is_nix_new_enough = is_version_new_enough(extract_version_number("nix"), required_nix_version);
  let is_direnv_new_enough = is_version_new_enough(extract_version_number("direnv"), required_direnv_version);

  switch (in_direnv, is_direnv_new_enough, is_nix_new_enough) {
    | (false, _, _) => Error("Not in direnv!")
    | (_, Ok(false), _) => Error("Direnv version is not new enough")
    | (_, _, Ok(false)) => Error("Nix version is not new enough")
    | (_, Error(e), _) => Error(e)
    | (_, _, Error(e)) => Error(e)
    | (true, Ok(true), Ok(true)) => Ok()
  }
};
