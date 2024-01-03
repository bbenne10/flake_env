open Core;
module Unix = Core_unix;

let nix = (args) => {
  let stdout_chan = Unix.open_process_in(
    "nix --extra-experimental-features \"nix-command flakes\" " ++ (args |> String.concat(~sep=" ")));
  let stdout_content = stdout_chan |> In_channel.input_all;
  let exit_code = Unix.close_process_in(stdout_chan);
  (exit_code, stdout_content)
}
