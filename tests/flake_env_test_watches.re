open Lib.Watches;

let test_get_path_removes_prefix = () => {
  let input = `Assoc([
    ("path", `String("aaaaaaaaaaabbbbb"))
  ]);
  Alcotest.(check(string))("Prefix removed", "bbbbb", get_path(input))
};

let test_get_paths_from_doc = () => {
  let input = `Assoc([
    ("path", `String("aaaaaaaaaaabbbbb")),
    ("inputs", `Assoc([
    ("foo", `Assoc([
      ("path", `String("aaaaaaaaaaaccccc")),
      ("inputs", `Assoc([
      ("bar", `Assoc([
        ("path", `String("aaaaaaaaaaaddddd")),
        ("inputs", `Assoc([]))
      ]))
    ]))
    ]))
  ]))
  ]);
  Alcotest.(check(list(string)))("Gathers all inputs", ["bbbbb", "ccccc", "ddddd"], get_paths_from_doc(input, []))
};

let () =
  Alcotest.(
  run(
    "Watches",
    [("get_path",
      [
      test_case("Removes prefix", `Quick, test_get_path_removes_prefix),
    ]),
     ("get_paths_from_doc",
      [
       test_case("Collects all paths", `Quick, test_get_paths_from_doc),
     ])
  ]),
);
