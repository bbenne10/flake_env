open Core;
module Unix = Core_unix;

open Lib.Util;

let _pp_exit_or_signal = (pp_fmt, e) =>
  Fmt.pf(pp_fmt, "%s", Unix.Exit_or_signal.to_string_hum(e));
let _exit_or_signal_eq = (a, b) => Unix.Exit_or_signal.compare(a, b) == 0;
let testable_exit_or_signal =
  Alcotest.testable(_pp_exit_or_signal, _exit_or_signal_eq);

let _syst_to_bool =
  fun
  | `Yes => true
  | _ => false;

let check_exit_or_signal =
  Alcotest.(check(Alcotest.pair(testable_exit_or_signal, string)));
let check_string = Alcotest.(check(string));
let check_bool = Alcotest.(check(bool));
let check_get_args =
  Alcotest.(
    check(
      Alcotest.result(Alcotest.triple(string, string, list(string)), unit),
    )
  );

let test_run_process_success = () =>
  check_exit_or_signal(
    "Returns expected",
    (Ok(), ""),
    run_process("true", []),
  );

let test_run_process_failure = () =>
  check_exit_or_signal(
    "Returns expected",
    (Error(`Exit_non_zero(1)), ""),
    run_process("false", []),
  );

let test_run_process_stdout = () =>
  check_exit_or_signal(
    "Returns expected",
    (Ok(), "echoed\n"),
    run_process("echo", ["echoed"]),
  );

let test_hash_one = () => {
  check_string(
    "Hash matches",
    hash_files([|"../LICENSE"|]),
    "b43cf2e824eb66ba0e8f939c08072a8e307b5e5f",
  );
};

let test_hash_multiple = () => {
  check_string(
    "Hash matches",
    hash_files([|"../LICENSE", "../LICENSE"|]),
    "08304d8baeed02722f81252952b00f6ac011ce0c",
  );
};

let test_hash_filters_nonexistent = () => {
  check_string(
    "Hash matches",
    hash_files([|"../LICENSE", "FOOBARBAZ"|]),
    "b43cf2e824eb66ba0e8f939c08072a8e307b5e5f",
  );
};

let test_rmrf_file = () => {
  let tmp_file_name = Filename_unix.temp_file("test", "txt");
  rmrf(tmp_file_name);

  check_bool(
    "File removed",
    false,
    _syst_to_bool(Sys_unix.is_file(tmp_file_name)),
  );
};

let test_rmrf_dir = () => {
  let temp_dir_name = Filename_unix.temp_dir("test", "d");
  let _ = Filename_unix.temp_file(~in_dir=temp_dir_name, "test", "txt");

  rmrf(temp_dir_name);

  check_bool(
    "File removed",
    false,
    _syst_to_bool(Sys_unix.file_exists(temp_dir_name)),
  );
};

let test_get_args_simple = () => {
  check_get_args(
    "Parses successfully",
    Ok(("foo", "bar", ["oof", "rab", "zab"])),
    get_args([|"000", "foo", "bar", "oof", "rab", "zab"|]),
  );
};

let test_get_args_just_enough = () => {
  check_get_args(
    "Parses just enough args",
    Ok(("foo", "bar", [])),
    get_args([|"000", "foo", "bar"|]),
  );
};

let test_get_args_error = () => {
  check_get_args(
    "Errors on too few args",
    Error(),
    get_args([|"000", "111"|]),
  );
};

let () =
  Alcotest.(
    run(
      "Watches",
      [
        (
          "run_process",
          [
            test_case("Capture's Stdout", `Quick, test_run_process_stdout),
            test_case("Success", `Quick, test_run_process_success),
            test_case("Failure", `Quick, test_run_process_failure),
          ],
        ),
        (
          "hash_files",
          [
            test_case("Hashes one file", `Quick, test_hash_one),
            test_case("Hashes multiple files", `Quick, test_hash_multiple),
            test_case(
              "Filters non-existent",
              `Quick,
              test_hash_filters_nonexistent,
            ),
          ],
        ),
        (
          "rmrf helper",
          [
            test_case("Removes file", `Quick, test_rmrf_file),
            test_case("Removes dir", `Quick, test_rmrf_dir),
          ],
        ),
        (
          "get_args",
          [
            test_case("Parses Args", `Quick, test_get_args_simple),
            test_case(
              "Parses just enough args",
              `Quick,
              test_get_args_just_enough,
            ),
            test_case("Handles too few args", `Quick, test_get_args_error),
          ],
        ),
      ],
    )
  );
