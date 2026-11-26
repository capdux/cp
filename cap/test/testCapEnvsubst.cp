// cap/test/testCapEnvsubst.cp
// Comprehensive unit tests for @cap/envsubst

import { test, expect_eq } from "cap:test";
import {
    EnvMap,
    ExpandError,
    expandEnv,
    expandEnvStr,
    hasEnvVars,
} from "../../packages/@cap/envsubst/src/index.cp";

fn main() {
    test("1. @cap/envsubst — basic $VAR and ${VAR} substitution & EnvMap operations", () => {
        let env = EnvMap.init();
        env.set("HOST", "localhost");
        env.set("PORT", "8080");
        env.set("PORT", "9090"); // overwrite existing key

        expect_eq(env.has("HOST"), true);
        expect_eq(env.has("MISSING"), false);
        expect_eq(env.get("PORT").?, "9090");

        let buf: [256]u8 = undefined;
        const out = expandEnvStr("http://$HOST:${PORT}/api/$MISSING", env, buf[0..256]);
        expect_eq(out, "http://localhost:9090/api/");
    });

    test("2. @cap/envsubst — :- and - default value semantics (unset vs empty)", () => {
        let env = EnvMap.init();
        env.set("EMPTY", "");
        env.set("SET", "prod");

        let buf: [256]u8 = undefined;
        // :- triggers on unset OR empty
        expect_eq(expandEnvStr("${UNSET:-fallback}", env, buf[0..256]), "fallback");
        expect_eq(expandEnvStr("${EMPTY:-fallback}", env, buf[0..256]), "fallback");
        expect_eq(expandEnvStr("${SET:-fallback}", env, buf[0..256]), "prod");

        // - triggers ONLY on unset (empty stays empty)
        expect_eq(expandEnvStr("${UNSET-fallback}", env, buf[0..256]), "fallback");
        expect_eq(expandEnvStr("${EMPTY-fallback}", env, buf[0..256]), "");
        expect_eq(expandEnvStr("${SET-fallback}", env, buf[0..256]), "prod");

        // Nested default expansion
        expect_eq(expandEnvStr("${A:-${B:-$SET}}", env, buf[0..256]), "prod");
    });

    test("3. @cap/envsubst — :+ and + alternate value semantics", () => {
        let env = EnvMap.init();
        env.set("EMPTY", "");
        env.set("TLS", "on");

        let buf: [256]u8 = undefined;
        // :+ triggers only when set AND non-empty
        expect_eq(expandEnvStr("${TLS:+--tls}", env, buf[0..256]), "--tls");
        expect_eq(expandEnvStr("${EMPTY:+--tls}", env, buf[0..256]), "");
        expect_eq(expandEnvStr("${UNSET:+--tls}", env, buf[0..256]), "");

        // + triggers whenever set (even if empty)
        expect_eq(expandEnvStr("${TLS+--tls}", env, buf[0..256]), "--tls");
        expect_eq(expandEnvStr("${EMPTY+--empty-set}", env, buf[0..256]), "--empty-set");
        expect_eq(expandEnvStr("${UNSET+--unset}", env, buf[0..256]), "");
    });

    test("4. @cap/envsubst — :? required variable error reporting", () => {
        let env = EnvMap.init();
        env.set("API_KEY", "secret-123");
        env.set("EMPTY_SECRET", "");

        let buf: [256]u8 = undefined;
        const ok_res = expandEnv("Bearer ${API_KEY:?missing api key}", env, buf[0..256]);
        expect_eq(ok_res.ok, true);
        expect_eq(ok_res.err, ExpandError.None);
        expect_eq(buf[0..ok_res.len], "Bearer secret-123");

        const err_unset = expandEnv("${DB_PASS:?db password required}", env, buf[0..256]);
        expect_eq(err_unset.ok, false);
        expect_eq(err_unset.err, ExpandError.MissingRequired);
        expect_eq(err_unset.missing_var, "DB_PASS");

        const err_empty = expandEnv("${EMPTY_SECRET:?must not be empty}", env, buf[0..256]);
        expect_eq(err_empty.ok, false);
        expect_eq(err_empty.err, ExpandError.MissingRequired);
        expect_eq(err_empty.missing_var, "EMPTY_SECRET");
    });

    test("5. @cap/envsubst — $$ and \\$ literal escapes & hasEnvVars", () => {
        let env = EnvMap.init();
        env.set("USER", "alice");

        let buf: [256]u8 = undefined;
        const out = expandEnvStr("cost=$$100 path=\\$USER real=$USER lone=$", env, buf[0..256]);
        expect_eq(out, "cost=$100 path=$USER real=alice lone=$");

        expect_eq(hasEnvVars("plain text only"), false);
        expect_eq(hasEnvVars("escaped $$VAR and \\$HOME and $5"), false);
        expect_eq(hasEnvVars("hello $USER"), true);
        expect_eq(hasEnvVars("port=${PORT:-8080}"), true);
    });

    test("6. @cap/envsubst — UnclosedBrace, BufferOverflow & full multi-var DSN template", () => {
        let env = EnvMap.init();
        env.set("HOST", "db.prod.local");
        env.set("DB", "analytics");
        env.set("SSL", "1");

        let buf: [256]u8 = undefined;
        const unclosed = expandEnv("prefix ${UNCLOSED_VAR", env, buf[0..256]);
        expect_eq(unclosed.ok, false);
        expect_eq(unclosed.err, ExpandError.UnclosedBrace);

        let tiny: [4]u8 = undefined;
        const overflow = expandEnv("host=$HOST", env, tiny[0..4]);
        expect_eq(overflow.ok, false);
        expect_eq(overflow.err, ExpandError.BufferOverflow);

        const dsn = expandEnvStr(
            "postgres://${USER:-postgres}@$HOST:${PORT:-5432}/${DB}${SSL:+?sslmode=require}",
            env,
            buf[0..256],
        );
        expect_eq(dsn, "postgres://postgres@db.prod.local:5432/analytics?sslmode=require");
    });
}
