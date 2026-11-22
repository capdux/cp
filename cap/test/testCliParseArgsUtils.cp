// Production deep regression tests for lib/cli/parseArgsUtils.cp (cap:cli)

import { test, expect_eq } from "cap:test";
import {
    PARSE_ARGS_MAX_TOKENS,
    ParseArgsError,
    ParseArgsValueType,
    ParseArgsOptionDef,
    ParseArgsTokenSubtype,
    ParseArgsTokenKind,
    ParseArgsToken,
    ParseArgsConfig,
    isOptionLikeValue,
    findOptionByShortName,
    findOptionByLongName,
    classifyArgToken,
    splitLongOption,
    parseArgsTokens,
} from "cap:cli";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("parseArgsUtils: classifyArgToken & option lookup rules", () => {
    expect_eq(PARSE_ARGS_MAX_TOKENS, 128);

    const defs: [4]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "verbose", short_name: "v", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "file", short_name: "f", value_type: .String },
        ParseArgsOptionDef{ long_name: "x", short_name: "", value_type: .String },
        ParseArgsOptionDef{ long_name: "port", short_name: "p", value_type: .String },
    ];

    // Exact short_name match and 1-char long_name fallback
    expect_eq(findOptionByShortName("v", defs[0..]) ?? 99, 0);
    expect_eq(findOptionByShortName("f", defs[0..]) ?? 99, 1);
    expect_eq(findOptionByShortName("x", defs[0..]) ?? 99, 2);
    expect_eq(findOptionByShortName("z", defs[0..]) == null, true);
    expect_eq(findOptionByShortName("", defs[0..]) == null, true);
    expect_eq(findOptionByLongName("port", defs[0..]) ?? 99, 3);
    expect_eq(findOptionByLongName("missing", defs[0..]) == null, true);

    // Lexical subtype classification (exact parity with Bun parse_args_utils.cp)
    expect_eq(classifyArgToken("--", defs[0..]) == ParseArgsTokenSubtype.OptionTerminator, true);
    expect_eq(classifyArgToken("-v", defs[0..]) == ParseArgsTokenSubtype.LoneShortOption, true);
    expect_eq(classifyArgToken("-vf", defs[0..]) == ParseArgsTokenSubtype.ShortOptionGroup, true);
    expect_eq(classifyArgToken("-fconfig.json", defs[0..]) == ParseArgsTokenSubtype.ShortOptionAndValue, true);
    expect_eq(classifyArgToken("-xval", defs[0..]) == ParseArgsTokenSubtype.ShortOptionAndValue, true);
    expect_eq(classifyArgToken("--verbose", defs[0..]) == ParseArgsTokenSubtype.LoneLongOption, true);
    expect_eq(classifyArgToken("--port=8080", defs[0..]) == ParseArgsTokenSubtype.LongOptionAndValue, true);
    // "--=abc" has '=' at index 2 (< 3), so classified as LoneLongOption
    expect_eq(classifyArgToken("--=abc", defs[0..]) == ParseArgsTokenSubtype.LoneLongOption, true);
    // Positionals: empty, single "-", normal words
    expect_eq(classifyArgToken("", defs[0..]) == ParseArgsTokenSubtype.Positional, true);
    expect_eq(classifyArgToken("-", defs[0..]) == ParseArgsTokenSubtype.Positional, true);
    expect_eq(classifyArgToken("app.cp", defs[0..]) == ParseArgsTokenSubtype.Positional, true);

    // isOptionLikeValue checks
    expect_eq(isOptionLikeValue("--verbose"), true);
    expect_eq(isOptionLikeValue("-v"), true);
    expect_eq(isOptionLikeValue("-"), false);
    expect_eq(isOptionLikeValue(""), false);
    expect_eq(isOptionLikeValue("8080"), false);

    // splitLongOption checks
    const s1 = splitLongOption("--port=8080");
    expect_eq(s1.has_value, true);
    expect_eq(strEq(s1.name, "port"), true);
    expect_eq(strEq(s1.raw_name, "--port"), true);
    expect_eq(strEq(s1.value, "8080"), true);

    const s2 = splitLongOption("--verbose");
    expect_eq(s2.has_value, false);
    expect_eq(strEq(s2.name, "verbose"), true);
    expect_eq(strEq(s2.raw_name, "--verbose"), true);
});

test("parseArgsUtils: argv guide example with flags & positionals", () => {
    const defs: [2]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "flag1", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "flag2", value_type: .String },
    ];
    const argv: [5]([]const u8) = [
        "/path/to/bun",
        "/path/to/cli.ts",
        "--flag1",
        "--flag2",
        "value",
    ];
    let out: [16]ParseArgsToken = [ParseArgsToken{}; 16];
    const n = parseArgsTokens(
        argv[0..],
        defs[0..],
        ParseArgsConfig{ strict: true, allow_positionals: true, allow_negative: false },
        out[0..],
    ) catch 0;

    expect_eq(n, 4);
    expect_eq(out[0].kind == ParseArgsTokenKind.Positional, true);
    expect_eq(strEq(out[0].value, "/path/to/bun"), true);
    expect_eq(out[1].kind == ParseArgsTokenKind.Positional, true);
    expect_eq(strEq(out[1].value, "/path/to/cli.ts"), true);
    expect_eq(out[2].kind == ParseArgsTokenKind.Option, true);
    expect_eq(strEq(out[2].name, "flag1"), true);
    expect_eq(out[2].has_value, false);
    expect_eq(out[3].kind == ParseArgsTokenKind.Option, true);
    expect_eq(strEq(out[3].name, "flag2"), true);
    expect_eq(out[3].has_value, true);
    expect_eq(out[3].inline_value, false);
    expect_eq(strEq(out[3].value, "value"), true);
});

test("parseArgsUtils: short option clusters, inline values & terminator", () => {
    const defs: [3]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "verbose", short_name: "v", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "quiet", short_name: "q", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "output", short_name: "o", value_type: .String },
    ];
    const argv: [6]([]const u8) = [
        "-vq",
        "-voout.txt",
        "-o",
        "next.txt",
        "--",
        "--not-an-option",
    ];
    let out: [16]ParseArgsToken = [ParseArgsToken{}; 16];
    const n = parseArgsTokens(
        argv[0..],
        defs[0..],
        ParseArgsConfig{ strict: true, allow_positionals: true, allow_negative: false },
        out[0..],
    ) catch 0;

    // -vq (2) + -voout.txt (2: v + o=out.txt) + -o next.txt (1) + -- (1) + --not-an-option (1) = 7
    expect_eq(n, 7);
    expect_eq(strEq(out[0].name, "verbose"), true);
    expect_eq(strEq(out[1].name, "quiet"), true);
    expect_eq(strEq(out[2].name, "verbose"), true);
    expect_eq(strEq(out[3].name, "output"), true);
    expect_eq(out[3].inline_value, true);
    expect_eq(strEq(out[3].value, "out.txt"), true);
    expect_eq(strEq(out[4].name, "output"), true);
    expect_eq(out[4].inline_value, false);
    expect_eq(strEq(out[4].value, "next.txt"), true);
    expect_eq(out[5].kind == ParseArgsTokenKind.OptionTerminator, true);
    expect_eq(out[6].kind == ParseArgsTokenKind.Positional, true);
    expect_eq(strEq(out[6].value, "--not-an-option"), true);
});

test("parseArgsUtils: allow_negative (--no-flag) and literal precedence", () => {
    const defs: [3]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "color", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "no-cache", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "port", value_type: .String },
    ];
    const argv: [2]([]const u8) = ["--no-color", "--no-cache"];
    let out: [8]ParseArgsToken = [ParseArgsToken{}; 8];
    const n = parseArgsTokens(
        argv[0..],
        defs[0..],
        ParseArgsConfig{ strict: true, allow_positionals: false, allow_negative: true },
        out[0..],
    ) catch 0;

    expect_eq(n, 2);
    expect_eq(strEq(out[0].name, "color"), true);
    expect_eq(out[0].negative, true);
    expect_eq(out[0].option_idx ?? 99, 0);

    // Explicit "no-cache" matches literally with negative = false
    expect_eq(strEq(out[1].name, "no-cache"), true);
    expect_eq(out[1].negative, false);
    expect_eq(out[1].option_idx ?? 99, 1);

    // Negating a String option in strict mode must fail with NegativeNotAllowed
    const bad_neg: [1]([]const u8) = ["--no-port"];
    let neg_rejected = false;
    * = parseArgsTokens(
        bad_neg[0..],
        defs[0..],
        ParseArgsConfig{ strict: true, allow_positionals: false, allow_negative: true },
        out[0..],
    ) catch |e| {
        neg_rejected = (e == ParseArgsError.NegativeNotAllowed);
        0;
    };
    expect_eq(neg_rejected, true);
});

test("parseArgsUtils: strict mode error guards (unknown, option-like, missing, positional, bool value)", () => {
    const defs: [2]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "verbose", short_name: "v", value_type: .Boolean },
        ParseArgsOptionDef{ long_name: "port", short_name: "p", value_type: .String },
    ];
    const strict_cfg = ParseArgsConfig{ strict: true, allow_positionals: false, allow_negative: false };
    let out: [8]ParseArgsToken = [ParseArgsToken{}; 8];

    // 1. UnknownOption
    const a1: [1]([]const u8) = ["--unknown"];
    let e1 = false;
    * = parseArgsTokens(a1[0..], defs[0..], strict_cfg, out[0..]) catch |e| {
        e1 = (e == ParseArgsError.UnknownOption);
        0;
    };
    expect_eq(e1, true);

    // 2. OptionLikeValue (--port --verbose)
    const a2: [2]([]const u8) = ["--port", "--verbose"];
    let e2 = false;
    * = parseArgsTokens(a2[0..], defs[0..], strict_cfg, out[0..]) catch |e| {
        e2 = (e == ParseArgsError.OptionLikeValue);
        0;
    };
    expect_eq(e2, true);

    // 3. MissingArgument (--port at end)
    const a3: [1]([]const u8) = ["--port"];
    let e3 = false;
    * = parseArgsTokens(a3[0..], defs[0..], strict_cfg, out[0..]) catch |e| {
        e3 = (e == ParseArgsError.MissingArgument);
        0;
    };
    expect_eq(e3, true);

    // 4. UnexpectedArgument (--verbose=true)
    const a4: [1]([]const u8) = ["--verbose=true"];
    let e4 = false;
    * = parseArgsTokens(a4[0..], defs[0..], strict_cfg, out[0..]) catch |e| {
        e4 = (e == ParseArgsError.UnexpectedArgument);
        0;
    };
    expect_eq(e4, true);

    // 5. UnexpectedPositional
    const a5: [1]([]const u8) = ["file.cp"];
    let e5 = false;
    * = parseArgsTokens(a5[0..], defs[0..], strict_cfg, out[0..]) catch |e| {
        e5 = (e == ParseArgsError.UnexpectedPositional);
        0;
    };
    expect_eq(e5, true);
});

test("parseArgsUtils: non-strict mode & OutputBufferTooSmall boundary", () => {
    const defs: [1]ParseArgsOptionDef = [
        ParseArgsOptionDef{ long_name: "port", short_name: "p", value_type: .String },
    ];
    const loose_cfg = ParseArgsConfig{ strict: false, allow_positionals: true, allow_negative: true };
    const argv: [4]([]const u8) = ["--custom=42", "-xz", "--no-extra", "pos1"];
    let out: [8]ParseArgsToken = [ParseArgsToken{}; 8];

    const n = parseArgsTokens(argv[0..], defs[0..], loose_cfg, out[0..]) catch 0;
    // --custom=42 (1) + -x (1) + -z (1) + --no-extra (1) + pos1 (1) = 5
    expect_eq(n, 5);
    expect_eq(strEq(out[0].name, "custom"), true);
    expect_eq(strEq(out[0].value, "42"), true);
    expect_eq(out[0].inline_value, true);
    expect_eq(strEq(out[1].name, "x"), true);
    expect_eq(strEq(out[2].name, "z"), true);
    expect_eq(strEq(out[3].name, "extra"), true);
    expect_eq(out[3].negative, true);
    expect_eq(out[4].kind == ParseArgsTokenKind.Positional, true);
    expect_eq(strEq(out[4].value, "pos1"), true);

    // Tiny output buffer triggers OutputBufferTooSmall cleanly
    let tiny: [2]ParseArgsToken = [ParseArgsToken{}; 2];
    let overflowed = false;
    * = parseArgsTokens(argv[0..], defs[0..], loose_cfg, tiny[0..]) catch |e| {
        overflowed = (e == ParseArgsError.OutputBufferTooSmall);
        0;
    };
    expect_eq(overflowed, true);
});
