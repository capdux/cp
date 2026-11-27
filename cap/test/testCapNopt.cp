import { test, expect_eq } from "cap:test";
import { NoptFlag, NoptShorthand, NoptResult, parseNopt } from "../../packages/nopt/src/index.cp";

fn main() {
    test("1. @cap/nopt — Long options (--key=val, --key val, --flag, --no-flag)", () => {
        const args: [6]string = [
            "--host=localhost",
            "--port",
            "8080",
            "--verbose",
            "--no-color",
            "index.cp",
        ];
        const empty_sh: []const NoptShorthand = [];
        const res: NoptResult = parseNopt(args[0..], empty_sh);

        expect_eq(res.has("host"), true);
        expect_eq(res.getString("host").?, "localhost");
        expect_eq(res.getString("port").?, "8080");
        expect_eq(res.getBool("verbose").?, true);
        expect_eq(res.getBool("color").?, false);
        expect_eq(res.has("missing"), false);
        expect_eq(res.getString("missing") == null, true);
        expect_eq(res.getBool("missing") == null, true);
        expect_eq(res.remainCount(), 1);
        expect_eq(res.remainAt(0), "index.cp");
    });

    test("2. @cap/nopt — Shorthand expansion (-s val, -s=val, -f, --no-* shorthand)", () => {
        const sh: [4]NoptShorthand = [
            NoptShorthand { short_key: "p", long_key: "port" },
            NoptShorthand { short_key: "-o", long_key: "--output" },
            NoptShorthand { short_key: "v", long_key: "verbose" },
            NoptShorthand { short_key: "q", long_key: "--no-color" },
        ];
        const args: [5]string = [
            "-p",
            "3000",
            "-o=dist/app",
            "-v",
            "-q",
        ];
        const res: NoptResult = parseNopt(args[0..], sh[0..]);

        expect_eq(res.getString("port").?, "3000");
        expect_eq(res.getString("output").?, "dist/app");
        expect_eq(res.getBool("verbose").?, true);
        expect_eq(res.getBool("color").?, false);
        expect_eq(res.remainCount(), 0);
    });

    test("3. @cap/nopt — Double-dash (--) terminator preserves all remaining args", () => {
        const args: [6]string = [
            "build",
            "--debug",
            "--",
            "--not-a-flag",
            "-x",
            "file.cp",
        ];
        const empty_sh: []const NoptShorthand = [];
        const res: NoptResult = parseNopt(args[0..], empty_sh);

        expect_eq(res.getBool("debug").?, true);
        expect_eq(res.has("not-a-flag"), false);
        expect_eq(res.has("x"), false);
        expect_eq(res.remainCount(), 4);
        expect_eq(res.remainAt(0), "build");
        expect_eq(res.remainAt(1), "--not-a-flag");
        expect_eq(res.remainAt(2), "-x");
        expect_eq(res.remainAt(3), "file.cp");
        expect_eq(res.remainAt(4), "");
    });

    test("4. @cap/nopt — Bundled short boolean flags (-abc)", () => {
        const sh: [2]NoptShorthand = [
            NoptShorthand { short_key: "a", long_key: "all" },
            NoptShorthand { short_key: "b", long_key: "brief" },
        ];
        const args: [1]string = ["-abc"];
        const res: NoptResult = parseNopt(args[0..], sh[0..]);

        expect_eq(res.getBool("all").?, true);
        expect_eq(res.getBool("brief").?, true);
        expect_eq(res.getBool("c").?, true);
    });

    test("5. @cap/nopt — Duplicate flags overwrite with last-wins semantics", () => {
        const args: [4]string = [
            "--mode=dev",
            "--color",
            "--mode=prod",
            "--no-color",
        ];
        const empty_sh: []const NoptShorthand = [];
        const res: NoptResult = parseNopt(args[0..], empty_sh);

        expect_eq(res.getString("mode").?, "prod");
        expect_eq(res.getBool("color").?, false);
    });

    test("6. @cap/nopt — Single dash and empty args edge cases", () => {
        const args: [2]string = ["-", "input.txt"];
        const empty_sh: []const NoptShorthand = [];
        const res: NoptResult = parseNopt(args[0..], empty_sh);

        expect_eq(res.remainCount(), 2);
        expect_eq(res.remainAt(0), "-");
        expect_eq(res.remainAt(1), "input.txt");

        const empty_args: []const string = [];
        const empty_res: NoptResult = parseNopt(empty_args, empty_sh);
        expect_eq(empty_res.remainCount(), 0);
        expect_eq(empty_res.has("anything"), false);
        const sample: NoptFlag = NoptFlag { key: "k", value: "v", is_bool: false, bool_val: false };
        expect_eq(sample.key, "k");
    });
}
