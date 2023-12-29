open Core;
open Yojson.Safe.Util;

module Unix = Core_unix;
module StringSet = Set.Make(String);
module Util = Flake_env__util;

[@deriving yojson]
type watch = {
  exists: bool,
  modtime: int,
  path: string
};

[@deriving yojson]
type watches = array<watch>;

let get = () => {
  let direnv_watch_str = Sys.getenv("DIRENV_WATCHES") |> Option.value_exn(~message="Environment missing DIRENV_WATCHES");
  let proc_info = Unix.create_process(~prog="direnv", ~args=["show_dump", direnv_watch_str]);
  let sub_stdout = Unix.in_channel_of_descr(proc_info.stdout);

  switch (Unix.waitpid(proc_info.pid)) {
    | Ok() => Ok(watches_of_yojson(Yojson.Safe.from_channel(sub_stdout)))
    | _ => Error("Failed to parse watches")
  }
};

let get_path = (doc) => String.drop_prefix(doc |> member("path") |> to_string, 11);

let rec get_paths_from_doc = (doc, paths) => {
  let p = get_path(doc);
  let sub_paths = List.concat(
    doc |> member("inputs")
        |> to_assoc
        |> List.map(~f=((_k, v)) => get_paths_from_doc(v, paths)));
  List.concat([[p], sub_paths])
};

let get_input_paths = () => {
  switch (Util.nix(["flake", "archive", "--json", "--no-write-lock-file"])) {
  | (Ok(), output) => get_paths_from_doc(Yojson.Safe.from_string(output), [])
  | (Error(_), _) => {
      Printf.eprintf("Failed to parse output of `nix flake archive --json`. Ignorning flake inputs. \n");
      []
    }
  }
};
