open Core;
module Unix = Core_unix;

type version_number = {
  major: int,
  minor: int,
  point: int,
};

[@deriving yojson]
type watch = {
  exists: bool,
  modtime: int,
  path: string
};

[@deriving yojson]
type watches = array<watch>;

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

let semver_re = Re.compile(Re.Posix.re({|([0-9]+)\.([0-9]+)\.([0-9]+)|}));

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
  | None => Error(Printf.sprintf("Failed executing '%s'", cmd))
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

let preflight = (layout_directory) => {
  switch (preflight_versions()) {
  | Ok(_) => {
      switch (Sys_unix.is_directory(layout_directory)) {
      | `Yes => Ok()
      | _ => {
          Unix.mkdir_p(layout_directory);
          Ok()
        }
      }
    }
  | err => err
  }

};

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

let get_watches = () => {
  let direnv_watch_str = Sys.getenv("DIRENV_WATCHES") |> Option.value_exn(~message="Environment missing DIRENV_WATCHES");
  let proc_info = Unix.create_process(~prog="direnv", ~args=["show_dump", direnv_watch_str]);
  let sub_stdout = Unix.in_channel_of_descr(proc_info.stdout);
  switch (Unix.waitpid(proc_info.pid)) {
  | Ok() => Ok(watches_of_yojson(Yojson.Safe.from_channel(sub_stdout)))
  | _ => Error("Failed to parse watches")
  }
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

let nix = (args) => {
  let stdout_chan = Unix.open_process_in(
    "nix --extra-experimental-features \"nix-command flakes\" " ++ (args |> String.concat(~sep=" ")))
    
  let stdout_content = stdout_chan |> In_channel.input_all;
  let exit_code = Unix.close_process_in(stdout_chan);
  (exit_code, stdout_content)
}

let freshen_cache = (layout_dir, hash, flake_specifier) => {
  clean_old_gcroots(layout_dir);
  let tmp_profile = layout_dir ++ "flake-tmp-profile."  ++ Core.Pid.to_string(Core_unix.getpid());
  let (exit_code, stdout_content) = nix(["print-dev-env", "--profile", tmp_profile, flake_specifier]);

  let profile = layout_dir ++ "/flake-profile-" ++ hash;
  let profile_rc = profile ++ ".rc";

  switch (exit_code) {
  | Ok() => {
      Out_channel.with_file(~f=f=> Out_channel.output_string(f, stdout_content), profile_rc);
      // TODO: flake inputs!
      switch (nix(["build", "--out-link", profile, tmp_profile])) {
      | (Ok(), _) => {
          Sys_unix.remove(tmp_profile);
          print_cur_cache(profile_rc);
        }
      | (err, _) => {
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
            switch (get_watches()) {
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
