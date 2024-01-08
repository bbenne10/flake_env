open Core;
module Unix = Core_unix;

let run_process = (name, args) => {
  /*** Run a process [name] with args [args], returning (exit_code, stdout text) */
  let stdout_chan =
    Unix.open_process_in(name ++ " " ++ (args |> String.concat(~sep=" ")));
  let stdout_content = stdout_chan |> In_channel.input_all;
  let exit_code = Unix.close_process_in(stdout_chan);
  (exit_code, stdout_content);
};

let nix = args =>
  run_process(
    "nix",
    ["--extra-experimental-features", "\"nix-command flakes\" ", ...args],
  );

let hash_files = filenames => {
  /*** Hash all entries in [filenames], returning a hex-encoded string of the hash of their contents */
  let ctx = Sha1.init();
  let () =
    filenames
    |> Array.filter(~f=f =>
         switch (Sys_unix.file_exists(f)) {
         | `Yes => true
         | _ => false
         }
       )
    |> Array.iter(~f=f => {
         f
         |> In_channel.create
         |> In_channel.input_all
         |> Sha1.update_string(ctx)
       });
  Sha1.finalize(ctx) |> Sha1.to_hex;
};

let rec rmrf = path => {
  switch (Unix.lstat(path).st_kind) {
  | exception (Unix.Unix_error(_, _, _)) => ()
  | S_REG | S_LNK => Unix.unlink(path)
  | S_DIR =>
    Sys_unix.readdir(path)
    |> Array.iter(~f=name => rmrf(Filename.concat(path, name)));
    Unix.rmdir(path);
  | _ => Printf.eprintf("Unsupported file type (Chr or Block device, FIFO, or Socket)\n")
  };
};

let get_args = argv => {
  switch (Array.length(argv)) {
  | x when x >= 3 =>
    let layout_directory = argv[1];
    let flake_specifier = argv[2];
    let other_args = snd(List.split_n(List.of_array(argv), 3));
    Ok((layout_directory, flake_specifier, other_args));
  | _ => Error()
  };
};
