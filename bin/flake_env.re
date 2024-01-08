open Core;
module Unix = Core_unix;

open Lib;

let main = () => {
  let argv = Sys.get_argv();
  switch (Util.get_args(argv)) {
  | Ok((layout_directory, flake_specifier, other_args)) =>
    switch (preflight(layout_directory)) {
    | Ok () =>
      switch (Lib.Watches.get()) {
      | Ok(watches) =>
        let paths = Array.map(~f=watch => watch.path, watches);
        let hash = Util.hash_files(paths);
        let profile = layout_directory ++ "/flake-profile-" ++ hash;
        let profile_rc = profile ++ ".rc";

        switch (Sys_unix.is_file(profile_rc), Sys_unix.is_file(profile)) {
        | (`Yes, `Yes) =>
          let profile_rc_mtime = Unix.stat(profile_rc).st_mtime;
          let all_older =
            Array.map(~f=watch => watch.modtime, watches)
            |> Array.for_all(~f=watch_mtime =>
                 watch_mtime <= int_of_float(profile_rc_mtime)
               );
          if (all_older) {
            print_cur_cache(profile_rc);
          } else {
            freshen_cache(
              layout_directory,
              hash,
              flake_specifier,
              other_args,
            );
          };
        | _ =>
          freshen_cache(layout_directory, hash, flake_specifier, other_args)
        };
      | Error(e) =>
        Printf.eprintf("%s\n", e);
        exit(1);
      }
    | Error(e) =>
      Printf.eprintf("%s\n", e);
      exit(1);
    }
  | Error () =>
    Printf.eprintf(
      "%s  <layout_directory> <flake specifier> <...args>\n",
      argv[0],
    );
    exit(1);
  };
};

let () = main();
