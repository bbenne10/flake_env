open Lib;

let pprint = (pp_fmt, version) => {
  Versions.(
    Fmt.pf(
      pp_fmt,
      "{ major: %d, minor: %d, point: %d }",
      version.major,
      version.minor,
      version.point,
    )
  );
};
let testable_version =
  Alcotest.testable(pprint, (a, b) => Versions.compare(a, b) == 0);

let check_version =
  Alcotest.(check(Alcotest.result(testable_version, string)));

let test_compare_equal = () => {
  let a = Versions.init(1, 0, 0);
  Alcotest.(check(int))("equal", 0, Versions.compare(a, a));
};

let test_compare_first_major_greater = () => {
  let a = Versions.init(2, 0, 0);
  let b = Versions.init(1, 0, 0);
  Alcotest.(check(int))("First major greater", 1, Versions.compare(a, b));
};

let test_compare_first_minor_greater = () => {
  let a = Versions.init(1, 1, 0);
  let b = Versions.init(1, 0, 0);
  Alcotest.(check(int))("First minor greater", 1, Versions.compare(a, b));
};

let test_compare_first_point_greater = () => {
  let a = Versions.init(1, 0, 1);
  let b = Versions.init(1, 0, 0);
  Alcotest.(check(int))("First point greater", 1, Versions.compare(a, b));
};

let test_compare_second_major_greater = () => {
  let a = Versions.init(1, 0, 0);
  let b = Versions.init(2, 0, 0);
  Alcotest.(check(int))("Second major greater", -1, Versions.compare(a, b));
};

let test_compare_second_minor_greater = () => {
  let a = Versions.init(1, 0, 0);
  let b = Versions.init(1, 1, 0);
  Alcotest.(check(int))("Second minor greater", -1, Versions.compare(a, b));
};

let test_compare_second_point_greater = () => {
  let a = Versions.init(1, 0, 0);
  let b = Versions.init(1, 0, 1);
  Alcotest.(check(int))("Second point greater", -1, Versions.compare(a, b));
};

let test_ine_cur_newer = () => {
  let a = Ok(Versions.init(2, 0, 0));
  let b = Versions.init(1, 0, 0);
  let ine = Versions.is_new_enough(a, b);
  Alcotest.(check(bool))("Curr newer", true, ine |> Result.get_ok);
};

let test_ine_cur_older = () => {
  let a = Ok(Versions.init(1, 0, 0));
  let b = Versions.init(2, 0, 0);
  let ine = Versions.is_new_enough(a, b);
  Alcotest.(check(bool))("Curr older", false, ine |> Result.get_ok);
};

let test_ine_cur_equal = () => {
  let a = Versions.init(1, 0, 0);
  let ine = Versions.is_new_enough(Ok(a), a);
  Alcotest.(check(bool))("Curr equal", true, ine |> Result.get_ok);
};

let test_ine_error = () => {
  let a = Error("foobarbaz");
  let ine = Versions.is_new_enough(a, Versions.init(1, 0, 0));
  Alcotest.(check(Alcotest.result(bool, string)))(
    "Error bubbled",
    Error("foobarbaz"),
    ine,
  );
};

let test_in_direnv_true = () => {
  Core_unix.putenv(~key="direnv", ~data="direnv");
  Alcotest.(check(bool))("In direnv", true, Versions.in_direnv());
};

let test_in_direnv_false = () => {
  Core_unix.unsetenv("direnv");
  Alcotest.(check(bool))("Not in direnv", false, Versions.in_direnv());
};

let test_extract_version_number_success = () => {
  let result = Versions.extract_version_number("../tests/spit_version.sh");
  check_version("Versions", Ok(Versions.init(1, 1, 1)), result);
};

let test_extract_version_number_no_version = () => {
  let result = Versions.extract_version_number("../tests/spit_gibberish.sh");
  check_version("Versions", Error("Stdout did not contain a version number for `../tests/spit_gibberish.sh --version`"), result);
};

let test_extract_version_number_nonexistent = () => {
  let result = Versions.extract_version_number("nonexistent.sh");
  check_version("Versions", Error("Failed executing 'nonexistent.sh'"), result);
};


// TODO: Test:
// * preflight_versions? impure, but m
let () =
  Alcotest.(
    run(
      "Versions",
      [
        (
          "compare",
          [
            test_case("Versions Equal", `Quick, test_compare_equal),
            test_case(
              "First Major Greater",
              `Quick,
              test_compare_first_major_greater,
            ),
            test_case(
              "First Minor Greater",
              `Quick,
              test_compare_first_minor_greater,
            ),
            test_case(
              "First Point Greater",
              `Quick,
              test_compare_first_point_greater,
            ),
            test_case(
              "Second Major Greater",
              `Quick,
              test_compare_second_major_greater,
            ),
            test_case(
              "Second Minor Greater",
              `Quick,
              test_compare_second_minor_greater,
            ),
            test_case(
              "Second Point Greater",
              `Quick,
              test_compare_second_point_greater,
            ),
          ],
        ),
        (
          "is_new_enough",
          [
            test_case("Curr Newer", `Quick, test_ine_cur_newer),
            test_case("Curr Older", `Quick, test_ine_cur_older),
            test_case("Curr Equal", `Quick, test_ine_cur_equal),
            test_case("Error", `Quick, test_ine_error),
          ],
        ),
        (
          "in_direnv",
          [
            test_case("true", `Quick, test_in_direnv_true),
            test_case("false", `Quick, test_in_direnv_false),
          ],
        ),
        (
          "extract_version_number",
          [
            test_case("success", `Quick, test_extract_version_number_success),
            test_case("no version number", `Quick, test_extract_version_number_no_version),
            test_case("missing binary", `Quick, test_extract_version_number_nonexistent),
          ],
        ),
      ],
    )
  );
