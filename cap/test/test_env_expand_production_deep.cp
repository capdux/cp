import std from "std";
import { test, expect_eq } from "cap:test";
import { EnvPair, lookupEnv, expandEnv, hasEnvRefs } from "cap:dotenv";

test("1. lookupEnv last-wins override and basic $VAR / ${VAR} expansion", () => {
    const pairs: [3]EnvPair = [
        EnvPair{ key: "HOST", value: "old.local" },
        EnvPair{ key: "PORT", value: "9000" },
        EnvPair{ key: "HOST", value: "api.cp.dev" },
    ];

    expect_eq(std.mem.eql(u8, lookupEnv(&pairs, "HOST") ?? unreachable, "api.cp.dev"), true);
    expect_eq(lookupEnv(&pairs, "MISSING") == null, true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    const expanded = expandEnv("https://$HOST:${PORT}/v1/$MISSING", &pairs, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, expanded, "https://api.cp.dev:9000/v1/"), true);
});

test("2. POSIX :-, -, :+, and + parameter modifiers on unset, empty, and non-empty vars", () => {
    const pairs: [2]EnvPair = [
        EnvPair{ key: "SET_VAL", value: "prod" },
        EnvPair{ key: "EMPTY_VAL", value: "" },
    ];
    let out: [128]u8 = std.mem.zeroes([128]u8);

    // :- 未设置或为空均使用默认值
    expect_eq(std.mem.eql(u8, expandEnv("${UNSET:-def}", &pairs, &out) ?? unreachable, "def"), true);
    expect_eq(std.mem.eql(u8, expandEnv("${EMPTY_VAL:-def}", &pairs, &out) ?? unreachable, "def"), true);
    expect_eq(std.mem.eql(u8, expandEnv("${SET_VAL:-def}", &pairs, &out) ?? unreachable, "prod"), true);

    // - 仅未设置时使用默认值，空串保持为空
    expect_eq(std.mem.eql(u8, expandEnv("${UNSET-def}", &pairs, &out) ?? unreachable, "def"), true);
    expect_eq(std.mem.eql(u8, expandEnv("${EMPTY_VAL-def}", &pairs, &out) ?? unreachable, ""), true);

    // :+ 仅已设置且非空时使用替代值
    expect_eq(std.mem.eql(u8, expandEnv("${UNSET:+alt}", &pairs, &out) ?? unreachable, ""), true);
    expect_eq(std.mem.eql(u8, expandEnv("${EMPTY_VAL:+alt}", &pairs, &out) ?? unreachable, ""), true);
    expect_eq(std.mem.eql(u8, expandEnv("${SET_VAL:+--env=$SET_VAL}", &pairs, &out) ?? unreachable, "--env=prod"), true);

    // + 只要已设置（含空串）即使用替代值
    expect_eq(std.mem.eql(u8, expandEnv("${UNSET+alt}", &pairs, &out) ?? unreachable, ""), true);
    expect_eq(std.mem.eql(u8, expandEnv("${EMPTY_VAL+alt}", &pairs, &out) ?? unreachable, "alt"), true);
});

test("3. $$ and \\$ escaping and hasEnvRefs detection", () => {
    const pairs: [1]EnvPair = [EnvPair{ key: "A", value: "1" }];
    let out: [128]u8 = std.mem.zeroes([128]u8);

    const escaped = expandEnv("price=$$100 and \\$A and $A$", &pairs, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, escaped, "price=$100 and $A and 1$"), true);

    expect_eq(hasEnvRefs("plain text"), false);
    expect_eq(hasEnvRefs("$$100 \\$VAR trailing$"), false);
    expect_eq(hasEnvRefs("hello $USER"), true);
    expect_eq(hasEnvRefs("hello ${USER:-anon}"), true);
});

test("4. nested default expression expansion", () => {
    const pairs: [1]EnvPair = [EnvPair{ key: "FALLBACK", value: "us-west-2" }];
    let out: [128]u8 = std.mem.zeroes([128]u8);

    const res = expandEnv("${PRIMARY_REGION:-${FALLBACK:-us-east-1}}", &pairs, &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, res, "us-west-2"), true);
});

test("5. unclosed braces, invalid syntax, and buffer-too-small return null", () => {
    const pairs: [1]EnvPair = [EnvPair{ key: "A", value: "long_value_12345" }];
    let out: [64]u8 = std.mem.zeroes([64]u8);

    expect_eq(expandEnv("prefix ${UNCLOSED", &pairs, &out) == null, true);
    expect_eq(expandEnv("${}", &pairs, &out) == null, true);
    expect_eq(expandEnv("${123}", &pairs, &out) == null, true);
    expect_eq(expandEnv("${A?invalid}", &pairs, &out) == null, true);

    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(expandEnv("$A", &pairs, &tiny) == null, true);
});
