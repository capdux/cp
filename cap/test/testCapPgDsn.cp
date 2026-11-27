// cap/test/testCapPgDsn.cp
// Comprehensive unit tests for @cap/pg-dsn (PostgreSQL URI & libpq DSN)

import { test, expect_eq } from "cap:test";
import {
    PgConfig,
    parseUri,
    parseKeyValue,
    parse,
    isValidSslMode,
    formatUri,
} from "../../packages/pg-connection-string/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/pg-dsn — full postgresql:// URI parsing", () => {
        const cfg: PgConfig = parseUri("postgresql://alice:s3cr3t@db.example.com:5439/prod?sslmode=require");
        expect_eq(cfg.valid, true);
        expect_eq(sliceEq(cfg.user, "alice"), true);
        expect_eq(sliceEq(cfg.password, "s3cr3t"), true);
        expect_eq(sliceEq(cfg.host, "db.example.com"), true);
        expect_eq(cfg.port, 5439 as u16);
        expect_eq(sliceEq(cfg.database, "prod"), true);
        expect_eq(sliceEq(cfg.sslmode, "require"), true);
    });

    test("2. @cap/pg-dsn — default port postgres:// & IPv6 [::1]:5433", () => {
        const c1: PgConfig = parseUri("postgres://postgres@localhost/mydb");
        expect_eq(c1.valid, true);
        expect_eq(sliceEq(c1.user, "postgres"), true);
        expect_eq(c1.password.len, 0 as usize);
        expect_eq(sliceEq(c1.host, "localhost"), true);
        expect_eq(c1.port, 5432 as u16);
        expect_eq(sliceEq(c1.database, "mydb"), true);

        const c2: PgConfig = parseUri("postgresql://[::1]:5433/v6db");
        expect_eq(c2.valid, true);
        expect_eq(sliceEq(c2.host, "::1"), true);
        expect_eq(c2.port, 5433 as u16);
        expect_eq(sliceEq(c2.database, "v6db"), true);
    });

    test("3. @cap/pg-dsn — libpq key=value with single-quoted password", () => {
        const dsn = "host=10.0.0.5 port=6432 user=pguser password='my secret pass' dbname=analytics sslmode=verify-full";
        const cfg: PgConfig = parseKeyValue(dsn);
        expect_eq(cfg.valid, true);
        expect_eq(sliceEq(cfg.host, "10.0.0.5"), true);
        expect_eq(cfg.port, 6432 as u16);
        expect_eq(sliceEq(cfg.user, "pguser"), true);
        expect_eq(sliceEq(cfg.password, "my secret pass"), true);
        expect_eq(sliceEq(cfg.database, "analytics"), true);
        expect_eq(sliceEq(cfg.sslmode, "verify-full"), true);
    });

    test("4. @cap/pg-dsn — parse() auto-routes URI vs key=value", () => {
        const c1: PgConfig = parse("  postgres://u@h/d  ");
        expect_eq(c1.valid, true);
        expect_eq(sliceEq(c1.host, "h"), true);
        expect_eq(sliceEq(c1.database, "d"), true);

        const c2: PgConfig = parse("  host=127.0.0.1 dbname=local  ");
        expect_eq(c2.valid, true);
        expect_eq(sliceEq(c2.host, "127.0.0.1"), true);
        expect_eq(sliceEq(c2.database, "local"), true);
        expect_eq(c2.port, 5432 as u16);
    });

    test("5. @cap/pg-dsn — formatUri with password & redacted log mode", () => {
        const cfg: PgConfig = parse("postgresql://admin:topsecret@::1:5432/app?sslmode=require");
        const cfg_v6: PgConfig = parse("postgresql://admin:topsecret@[::1]:5432/app?sslmode=require");
        * = cfg;
        expect_eq(cfg_v6.valid, true);

        let buf: [128]u8 = undefined;
        const n_full: usize = formatUri(cfg_v6, true, buf[0..128]);
        expect_eq(
            sliceEq(buf[0..n_full], "postgresql://admin:topsecret@[::1]:5432/app?sslmode=require"),
            true,
        );

        const n_safe: usize = formatUri(cfg_v6, false, buf[0..128]);
        expect_eq(
            sliceEq(buf[0..n_safe], "postgresql://admin@[::1]:5432/app?sslmode=require"),
            true,
        );
    });

    test("6. @cap/pg-dsn — isValidSslMode, invalid ports, empty input & tiny buffer", () => {
        expect_eq(isValidSslMode("disable"), true);
        expect_eq(isValidSslMode("prefer"), true);
        expect_eq(isValidSslMode("require"), true);
        expect_eq(isValidSslMode("verify-ca"), true);
        expect_eq(isValidSslMode("verify-full"), true);
        expect_eq(isValidSslMode("bogus"), false);

        expect_eq(parse("").valid, false);
        expect_eq(parseUri("mysql://localhost/db").valid, false);
        expect_eq(parseUri("postgresql://localhost:99999/db").valid, false);
        expect_eq(parseUri("postgresql://localhost:0/db").valid, false);
        expect_eq(parseUri("postgresql://localhost/db?sslmode=bad").valid, false);
        expect_eq(parseKeyValue("host=a port=abc").valid, false);
        expect_eq(parseKeyValue("password='unterminated").valid, false);

        const ok: PgConfig = parse("postgres://localhost/db");
        let tiny: [8]u8 = undefined;
        expect_eq(formatUri(ok, false, tiny[0..8]), 0 as usize);
    });
}
