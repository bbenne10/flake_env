open Core;
module Unix = Core_unix;


let preflight = (layout_directory) => {
  switch (Lib.Versions.preflight_versions(), Sys_unix.is_directory(layout_directory)) {
    | (Ok(_), `Yes) => Ok()
    | (Ok(_), _) => {
      Unix.mkdir_p(layout_directory);
      Ok()
    }
    | (err, _) => err
  }
}

let hash_files = (filenames) => {
  let ctx = Sha1.init();
  let () = filenames
  |> Array.filter(~f=f => switch (Sys_unix.file_exists(f)) {
      | `Yes => true
      | _ => false
    })
  |> Array.iter(~f=(f) => {
    f |> In_channel.create |> In_channel.input_all |> Sha1.update_string(ctx);
  });
  Sha1.finalize(ctx) |> Sha1.to_hex
};

// TODO: Maybe make this more terse?
let rec rmrf = (path) => {
  switch (Unix.lstat(path).st_kind) {
  | exception Unix.Unix_error(_, _, _) => ()
  | S_REG => Unix.unlink(path)
  | S_LNK => Unix.unlink(path)
  | S_DIR => {
      Sys_unix.readdir(path) |> Array.iter(~f=name => rmrf(Filename.concat(path, name)));
      Unix.rmdir(path)
    }
  | S_CHR => Printf.eprintf("Don't know how to handle Character Device file\n")
  | S_BLK => Printf.eprintf("Don't know how to handle Block Device file\n")
  | S_FIFO => Printf.eprintf("Don't know how to handle FIFO file\n")
  | S_SOCK => Printf.eprintf("Don't know how to handle Socket file\n")
  }
};

let clean_old_gcroots = (layout_dir) => {
  rmrf(layout_dir ++ "/flake-inputs/");
  rmrf(layout_dir);
  Unix.mkdir_p(layout_dir ++ "/flake-inputs/");
};

let print_cur_cache = (profile_rc) => {
  In_channel.read_all(profile_rc) |> Printf.printf("%s")
};

let add_gcroot = (store_path, symlink) => {
  switch (Lib.Util.nix(["build", "--out-link", symlink, store_path])) {
  | (Ok(), _) => Ok()
  | (err, _) => err
  }
}

let freshen_cache = (layout_dir, hash, flake_specifier) => {
  clean_old_gcroots(layout_dir);
  let tmp_profile = layout_dir ++ "flake-tmp-profile."  ++ Core.Pid.to_string(Core_unix.getpid());
  let (exit_code, stdout_content) = Lib.Util.nix(["print-dev-env", "--profile", tmp_profile, flake_specifier]);

  let profile = layout_dir ++ "/flake-profile-" ++ hash;
  let profile_rc = profile ++ ".rc";

  switch (exit_code) {
  | Ok() => {
      Out_channel.with_file(~f=f=> Out_channel.output_string(f, stdout_content), profile_rc);
      switch (add_gcroot(tmp_profile, profile)) {
        | Ok() => {
          Sys_unix.remove(tmp_profile);
          let flake_input_cache_path = layout_dir ++ "/flake-inputs/"
          let flake_inputs = Lib.Watches.get_input_paths();
          flake_inputs |> List.iter(~f=(inpt) => {
            switch (add_gcroot("/nix/store/" ++ inpt, flake_input_cache_path ++ inpt)) {
            | Ok() => ()
            | err => Printf.eprintf("Failed creating flake-input gcroot: %s\n", Core_unix.Exit_or_signal.to_string_hum(err));
            };
          });
          print_cur_cache(profile_rc);
        }
        | err => {
          Printf.eprintf("Failed creating gcroot: %s\n", Core_unix.Exit_or_signal.to_string_hum(err));
          exit(1);
        }
      };
    }
  | err => {
      Printf.eprintf("Failed evaluating flake: %s\n", Core_unix.Exit_or_signal.to_string_hum(err));
      exit(1);
    }
  };

};

// TODO: Extend to add support for additional flags, maybe?
let main = () => {
  let argv = Sys.get_argv();
  switch (Array.length(argv)) {
  | 3 => {
      let flake_specifier = argv[1];
      let layout_directory = argv[2];
      switch (preflight(layout_directory)) {
        | Ok() => {
            switch (Lib.Watches.get()) {
            | Ok(watches) => {
                let paths = Array.map(~f=watch => watch.path, watches);
                let hash = hash_files(paths);
                let profile = layout_directory ++ "/flake-profile-" ++ hash;
                let profile_rc = profile ++ ".rc";

                switch ((Sys_unix.is_file(profile_rc), Sys_unix.is_file(profile))) {
                | (`Yes, `Yes) => {
                    let profile_rc_mtime = Unix.stat(profile_rc).st_mtime;
                    let all_older= Array.map(~f=watch => watch.modtime, watches)
                    |> Array.for_all(~f=watch_mtime => watch_mtime <= int_of_float(profile_rc_mtime))
                    switch (all_older) {
                    | true => print_cur_cache(profile_rc)
                    | false => freshen_cache(layout_directory, hash, flake_specifier)
                    }
                  }
                | _ => freshen_cache(layout_directory, hash, flake_specifier)
                }
              };
            | Error(e) => {
                Printf.eprintf("%s\n", e);
                exit(1);
              }
            }
          }
      | Error(e) => {
          Printf.eprintf("%s\n", e);
          exit(1);
        }
      };
    }
  | _ => {
      Printf.eprintf("%s <flake specifier> <layout_directory>\n", argv[0]);
      exit(1);
      }
  }
}

let () = main();
