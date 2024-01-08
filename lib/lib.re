open Core;
module Unix = Core_unix;

module Util = Flake_env__util;
module Watches = Flake_env__watches;
module Versions = Flake_env__versions;

let print_cur_cache = profile_rc => {
  In_channel.read_all(profile_rc) |> Printf.printf("%s");
};

let clean_old_gcroots = layout_dir => {
  Util.rmrf(layout_dir ++ "/flake-inputs/");
  Util.rmrf(layout_dir);
  Unix.mkdir_p(layout_dir ++ "/flake-inputs/");
};

let add_gcroot = (store_path, symlink) => {
  switch (Util.nix(["build", "--out-link", symlink, store_path])) {
  | (Ok (), _) => Ok()
  | (err, _) => err
  };
};

let freshen_cache = (layout_dir, hash, flake_specifier, other_args) => {
  clean_old_gcroots(layout_dir);
  let tmp_profile =
    layout_dir
    ++ "flake-tmp-profile."
    ++ Core.Pid.to_string(Core_unix.getpid());

  let pde_args = [
    "print-dev-env",
    "--profile",
    tmp_profile,
    flake_specifier,
    ...other_args,
  ];
  let (exit_code, stdout_content) = Util.nix(pde_args);

  let profile = layout_dir ++ "/flake-profile-" ++ hash;
  let profile_rc = profile ++ ".rc";

  switch (exit_code) {
  | Ok () =>
    Out_channel.with_file(
      ~f=f => Out_channel.output_string(f, stdout_content),
      profile_rc,
    );
    switch (add_gcroot(tmp_profile, profile)) {
    | Ok () =>
      Sys_unix.remove(tmp_profile);
      let flake_input_cache_path = layout_dir ++ "/flake-inputs/";
      let flake_inputs = Watches.get_input_paths();
      flake_inputs
      |> List.iter(~f=inpt => {
           switch (
             add_gcroot("/nix/store/" ++ inpt, flake_input_cache_path ++ inpt)
           ) {
           | Ok () => ()
           | err =>
             Printf.eprintf(
               "Failed creating flake-input gcroot: %s\n",
               Core_unix.Exit_or_signal.to_string_hum(err),
             )
           }
         });
      print_cur_cache(profile_rc);
    | err =>
      Printf.eprintf(
        "Failed creating gcroot: %s\n",
        Core_unix.Exit_or_signal.to_string_hum(err),
      );
      exit(1);
    };
  | err =>
    Printf.eprintf(
      "Failed evaluating flake: %s\n",
      Core_unix.Exit_or_signal.to_string_hum(err),
    );
    exit(1);
  };
};

let preflight = layout_directory => {
  switch (
    Versions.preflight_versions(),
    Sys_unix.is_directory(layout_directory),
  ) {
  | (Ok(_), `Yes) => Ok()
  | (Ok(_), _) =>
    Unix.mkdir_p(layout_directory);
    Ok();
  | (err, _) => err
  };
};
