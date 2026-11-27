import { test, expect_eq } from "cap:test";
import {
    ARG_POSITIONAL,
    ARG_LONG_FLAG,
    ARG_LONG_VALUE,
    ARG_SHORT_FLAG,
    ParsedArg,
    parseArgs,
    hasFlag,
    getOption,
    countPositionals,
} from "../../packages/mri/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/proc-args — --key=val, --key val & -o val extraction", () => {
        const argv: [5]string = ["--port=8080", "--host", "127.0.0.1", "-o", "out.txt"];
        expect_eq(testEq(getOption(argv[0..5], "port", 0), "8080"), true);
        expect_eq(testEq(getOption(argv[0..5], "host", 0), "127.0.0.1"), true);
        expect_eq(testEq(getOption(argv[0..5], "output", 111), "out.txt"), true); // 'o' = 111
        expect_eq(testEq(getOption(argv[0..5], "missing", 120), ""), true);

        let parsed: [8]ParsedArg = undefined;
        const n = parseArgs(argv[0..5], parsed[0..8]);
        expect_eq(n, 5 as usize);
        expect_eq(parsed[0].kind, ARG_LONG_VALUE);
        expect_eq(testEq(parsed[0].name, "port"), true);
        expect_eq(testEq(parsed[0].value, "8080"), true);
        expect_eq(parsed[1].kind, ARG_LONG_FLAG);
        expect_eq(testEq(parsed[1].name, "host"), true);
    });

    test("2. @cap/proc-args — Short flag cluster -abc splitting & hasFlag", () => {
        const argv: [3]string = ["-abc", "--verbose", "--debug=false"];
        let parsed: [8]ParsedArg = undefined;
        const n = parseArgs(argv[0..3], parsed[0..8]);
        expect_eq(n, 5 as usize);
        expect_eq(parsed[0].kind, ARG_SHORT_FLAG);
        expect_eq(testEq(parsed[0].name, "a"), true);
        expect_eq(testEq(parsed[1].name, "b"), true);
        expect_eq(testEq(parsed[2].name, "c"), true);

        expect_eq(hasFlag(argv[0..3], "all", 97), true);       // 'a'
        expect_eq(hasFlag(argv[0..3], "", 98), true);          // 'b'
        expect_eq(hasFlag(argv[0..3], "verbose", 118), true);  // --verbose
        expect_eq(hasFlag(argv[0..3], "debug", 0), false);     // --debug=false
        expect_eq(hasFlag(argv[0..3], "quiet", 113), false);
    });

    test("3. @cap/proc-args — '--' terminator protects positional arguments", () => {
        const argv: [5]string = ["-v", "--", "--not-a-flag", "-x", "file.cp"];
        expect_eq(hasFlag(argv[0..5], "", 118), true);
        expect_eq(hasFlag(argv[0..5], "not-a-flag", 120), false);

        let parsed: [8]ParsedArg = undefined;
        const n = parseArgs(argv[0..5], parsed[0..8]);
        expect_eq(n, 4 as usize); // "-v", "--not-a-flag", "-x", "file.cp"
        expect_eq(parsed[0].kind, ARG_SHORT_FLAG);
        expect_eq(parsed[1].kind, ARG_POSITIONAL);
        expect_eq(testEq(parsed[1].value, "--not-a-flag"), true);
        expect_eq(parsed[2].kind, ARG_POSITIONAL);
        expect_eq(testEq(parsed[2].value, "-x"), true);
        expect_eq(parsed[3].kind, ARG_POSITIONAL);
    });

    test("4. @cap/proc-args — countPositionals across flags & '--'", () => {
        const argv: [6]string = ["build", "--release", "src/a.cp", "--", "--raw", "b.cp"];
        expect_eq(countPositionals(argv[0..6]), 4 as usize);
    });

    test("5. @cap/proc-args — Lone '-' treated as stdin positional", () => {
        const argv: [3]string = ["cat", "-", "--number"];
        expect_eq(countPositionals(argv[0..3]), 2 as usize);

        let parsed: [4]ParsedArg = undefined;
        const n = parseArgs(argv[0..3], parsed[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(parsed[1].kind, ARG_POSITIONAL);
        expect_eq(testEq(parsed[1].value, "-"), true);
    });

    test("6. @cap/proc-args — Empty argv & small output slice truncation guard", () => {
        const dummy: [1]string = ["x"];
        let parsed: [2]ParsedArg = undefined;
        expect_eq(parseArgs(dummy[0..0], parsed[0..2]), 0 as usize);
        expect_eq(countPositionals(dummy[0..0]), 0 as usize);
        expect_eq(hasFlag(dummy[0..0], "v", 118), false);

        // Truncates cleanly to out.len == 2
        const many: [3]string = ["-abcd", "pos1", "pos2"];
        expect_eq(parseArgs(many[0..3], parsed[0..2]), 2 as usize);
        expect_eq(testEq(parsed[0].name, "a"), true);
        expect_eq(testEq(parsed[1].name, "b"), true);
    });
}
