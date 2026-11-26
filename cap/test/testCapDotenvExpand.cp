import { test, expect_eq } from "cap:test";
import { EnvVar, hasVar, lookupVar, expandValue } from "../../packages/@cap/dotenv-expand/src/index.cp";

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
    test("1. @cap/dotenv-expand — Basic $VAR and ${VAR} interpolation", () => {
        const vars: [3]EnvVar = [
            EnvVar { key: "HOST", val: "api.cap.dev" },
            EnvVar { key: "PORT", val: "8443" },
            EnvVar { key: "VER", val: "v2" },
        ];
        expect_eq(hasVar(vars[0..3], "HOST"), true);
        expect_eq(hasVar(vars[0..3], "MISSING"), false);
        expect_eq(testEq(lookupVar(vars[0..3], "PORT"), "8443"), true);

        let buf: [128]u8 = undefined;
        const n = expandValue("https://${HOST}:${PORT}/$VER/users", vars[0..3], buf[0..128]);
        const expected = "https://api.cap.dev:8443/v2/users";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("2. @cap/dotenv-expand — ${VAR:-default} vs ${VAR-default} on empty & unset vars", () => {
        const vars: [2]EnvVar = [
            EnvVar { key: "EMPTY", val: "" },
            EnvVar { key: "SET", val: "prod" },
        ];
        let buf: [128]u8 = undefined;

        // :- triggers on empty string
        const n1 = expandValue("${EMPTY:-fallback}", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n1 as usize], "fallback"), true);

        // - preserves empty string when var is defined
        const n2 = expandValue("[${EMPTY-fallback}]", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n2 as usize], "[]"), true);

        // - triggers when var is unset
        const n3 = expandValue("${UNSET-fallback}", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n3 as usize], "fallback"), true);

        // Nested fallback ${UNSET:-${EMPTY:-deep_default}}
        const n4 = expandValue("${UNSET:-${EMPTY:-deep_default}}", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n4 as usize], "deep_default"), true);
    });

    test("3. @cap/dotenv-expand — ${VAR:+alt} alternate substitution", () => {
        const vars: [2]EnvVar = [
            EnvVar { key: "DEBUG", val: "1" },
            EnvVar { key: "EMPTY", val: "" },
        ];
        let buf: [128]u8 = undefined;

        const n1 = expandValue("flags=${DEBUG:+--verbose}", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n1 as usize], "flags=--verbose"), true);

        const n2 = expandValue("flags=${EMPTY:+--verbose}${UNSET:+--trace}", vars[0..2], buf[0..128]);
        expect_eq(testEq(buf[0..n2 as usize], "flags="), true);
    });

    test("4. @cap/dotenv-expand — Recursive expansion & self-cycle A=${A} safe termination", () => {
        const vars: [4]EnvVar = [
            EnvVar { key: "PROTO", val: "https" },
            EnvVar { key: "HOST", val: "cp.local" },
            EnvVar { key: "URL", val: "${PROTO}://${HOST}/rpc" },
            EnvVar { key: "LOOP", val: "${LOOP}" },
        ];
        let buf: [128]u8 = undefined;

        const n1 = expandValue("endpoint=$URL", vars[0..4], buf[0..128]);
        expect_eq(testEq(buf[0..n1 as usize], "endpoint=https://cp.local/rpc"), true);

        // Cycle must terminate cleanly without hanging or overflowing
        const n2 = expandValue("x=${LOOP}y", vars[0..4], buf[0..128]);
        expect_eq(n2 >= 0, true);
        expect_eq(testEq(buf[0..n2 as usize], "x=y"), true);
    });

    test("5. @cap/dotenv-expand — Escaped \\$ preserves literal dollar sign", () => {
        const vars: [1]EnvVar = [EnvVar { key: "PRICE", val: "99" }];
        let buf: [128]u8 = undefined;

        const n = expandValue("cost=\\$PRICE or \\$${PRICE}", vars[0..1], buf[0..128]);
        expect_eq(testEq(buf[0..n as usize], "cost=$PRICE or $99"), true);
    });

    test("6. @cap/dotenv-expand — Unclosed ${ & undersized buffer return -1", () => {
        const vars: [1]EnvVar = [EnvVar { key: "LONG", val: "abcdefghijklmnop" }];
        let buf: [64]u8 = undefined;
        expect_eq(expandValue("prefix_${LONG", vars[0..1], buf[0..64]), -1 as i64);
        expect_eq(expandValue("${}", vars[0..1], buf[0..64]), -1 as i64);

        let tiny: [5]u8 = undefined;
        expect_eq(expandValue("${LONG}", vars[0..1], tiny[0..5]), -1 as i64);
    });
}
