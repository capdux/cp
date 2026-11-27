import { test, expect_eq } from "cap:test";
import { EnvMap, parseDotenv, expandEnvValue } from "../../packages/dotenv-expand/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/dotenv-expand — parseDotenv comments, quotes & export prefix", () => {
        let src = "# header comment\nHOST=localhost # inline\nexport PORT=\"8080\"\nSINGLE='raw # keep'\n";
        let env = parseDotenv(src);
        expect_eq(env.len(), 3);
        expect_eq(env.get("HOST") ?? "", "localhost");
        expect_eq(env.get("PORT") ?? "", "8080");
        expect_eq(env.get("SINGLE") ?? "", "raw # keep");
    });

    test("2. @cap/dotenv-expand — Bare $VAR and braced ${VAR} expansion", () => {
        let env = EnvMap.new();
        env.set("HOST", "api.capdux.org");
        env.set("PORT", "443");
        let out: [128]u8 = undefined;
        let n = expandEnvValue("https://$HOST:${PORT}/v1", env, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "https://api.capdux.org:443/v1"), true);
    });

    test("3. @cap/dotenv-expand — Default fallback ${VAR:-default} vs ${VAR-default}", () => {
        let env = EnvMap.new();
        env.set("EMPTY", "");
        let out: [128]u8 = undefined;
        let n1 = expandEnvValue("${MISSING:-fallback}", env, out[0..128]);
        expect_eq(eqBuf(out[0..n1], n1, "fallback"), true);
        let n2 = expandEnvValue("${EMPTY:-nonempty}", env, out[0..128]);
        expect_eq(eqBuf(out[0..n2], n2, "nonempty"), true);
        let n3 = expandEnvValue("${EMPTY-nonempty}", env, out[0..128]);
        expect_eq(n3, 0);
    });

    test("4. @cap/dotenv-expand — Escaped \\$ preserves literal dollar sign", () => {
        let env = EnvMap.new();
        env.set("PRICE", "100");
        let out: [64]u8 = undefined;
        let n = expandEnvValue("Cost: \\$PRICE = $$PRICE", env, out[0..64]);
        expect_eq(eqBuf(out[0..n], n, "Cost: $PRICE = $100"), true);
    });

    test("5. @cap/dotenv-expand — Missing variable without default expands to empty", () => {
        let env = EnvMap.new();
        let out: [64]u8 = undefined;
        let n = expandEnvValue("pre-${NOPE}-post-$ALSO_NOPE", env, out[0..64]);
        expect_eq(eqBuf(out[0..n], n, "pre--post-"), true);
    });

    test("6. @cap/dotenv-expand — EnvMap overwrite & has check", () => {
        let env = parseDotenv("A=1\nA=2\n");
        expect_eq(env.len(), 1);
        expect_eq(env.has("A"), true);
        expect_eq(env.has("B"), false);
        expect_eq(env.get("A") ?? "", "2");
    });
}
