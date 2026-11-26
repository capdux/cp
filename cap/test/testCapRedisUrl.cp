// cap/test/testCapRedisUrl.cp
// Comprehensive unit tests for @cap/redis-url

import { test, expect_eq } from "cap:test";
import {
    parseRedisUrl,
    formatRedisUrl,
} from "../../packages/@cap/redis-url/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/redis-url — Standard redis:// with default port 6379 & db 0", () => {
        const cfg = parseRedisUrl("redis://localhost");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.tls, false);
        expect_eq(cfg.is_unix, false);
        expect_eq(sliceEq(cfg.host, "localhost"), true);
        expect_eq(cfg.port, 6379 as u16);
        expect_eq(cfg.db, 0 as u16);
        expect_eq(cfg.username.len, 0 as usize);
        expect_eq(cfg.password.len, 0 as usize);
    });

    test("2. @cap/redis-url — rediss:// ACL username:password, custom port & /db + ?db= override", () => {
        const cfg = parseRedisUrl("rediss://alice:s3cr3t@redis.prod:6380/3");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.tls, true);
        expect_eq(sliceEq(cfg.username, "alice"), true);
        expect_eq(sliceEq(cfg.password, "s3cr3t"), true);
        expect_eq(sliceEq(cfg.host, "redis.prod"), true);
        expect_eq(cfg.port, 6380 as u16);
        expect_eq(cfg.db, 3 as u16);

        const cfgQ = parseRedisUrl("redis://redis.prod:6379/1?db=5");
        expect_eq(cfgQ.valid, true);
        expect_eq(cfgQ.db, 5 as u16);
    });

    test("3. @cap/redis-url — Password-only redis://:onlypass@host", () => {
        const cfg = parseRedisUrl("redis://:onlypass@127.0.0.1:6379/2");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.username.len, 0 as usize);
        expect_eq(sliceEq(cfg.password, "onlypass"), true);
        expect_eq(sliceEq(cfg.host, "127.0.0.1"), true);
        expect_eq(cfg.port, 6379 as u16);
        expect_eq(cfg.db, 2 as u16);
    });

    test("4. @cap/redis-url — IPv6 [::1]:6380 & Unix domain socket redis+unix:// / unix://", () => {
        const v6 = parseRedisUrl("redis://[::1]:6380/4");
        expect_eq(v6.valid, true);
        expect_eq(sliceEq(v6.host, "::1"), true);
        expect_eq(v6.port, 6380 as u16);
        expect_eq(v6.db, 4 as u16);

        const ux = parseRedisUrl("redis+unix:///tmp/redis.sock?db=7");
        expect_eq(ux.valid, true);
        expect_eq(ux.is_unix, true);
        expect_eq(sliceEq(ux.socket_path, "/tmp/redis.sock"), true);
        expect_eq(ux.db, 7 as u16);

        const ux2 = parseRedisUrl("unix:///var/run/valkey.sock");
        expect_eq(ux2.valid, true);
        expect_eq(ux2.is_unix, true);
        expect_eq(sliceEq(ux2.socket_path, "/var/run/valkey.sock"), true);
    });

    test("5. @cap/redis-url — formatRedisUrl verbatim & password-redacted output", () => {
        const cfg = parseRedisUrl("rediss://alice:topsecret@cache.internal:6380/3");
        let rawBuf: [128]u8 = undefined;
        const nRaw = formatRedisUrl(cfg, false, rawBuf[0..128]);
        expect_eq(sliceEq(rawBuf[0..nRaw], "rediss://alice:topsecret@cache.internal:6380/3"), true);

        let redBuf: [128]u8 = undefined;
        const nRed = formatRedisUrl(cfg, true, redBuf[0..128]);
        expect_eq(sliceEq(redBuf[0..nRed], "rediss://alice:***@cache.internal:6380/3"), true);

        const v6 = parseRedisUrl("redis://[::1]:6380");
        let v6Buf: [64]u8 = undefined;
        const nv6 = formatRedisUrl(v6, false, v6Buf[0..64]);
        expect_eq(sliceEq(v6Buf[0..nv6], "redis://[::1]:6380"), true);
    });

    test("6. @cap/redis-url — Reject invalid scheme, port, db & tiny output buffer", () => {
        expect_eq(parseRedisUrl("http://localhost:6379").valid, false);
        expect_eq(parseRedisUrl("redis://localhost:0").valid, false);
        expect_eq(parseRedisUrl("redis://localhost:70000").valid, false);
        expect_eq(parseRedisUrl("redis://localhost:abc").valid, false);
        expect_eq(parseRedisUrl("redis://localhost/notadb").valid, false);
        expect_eq(parseRedisUrl("redis://localhost?db=bad").valid, false);
        expect_eq(parseRedisUrl("redis://").valid, false);

        const cfg = parseRedisUrl("redis://localhost:6379");
        let tiny: [8]u8 = undefined;
        expect_eq(formatRedisUrl(cfg, false, tiny[0..8]), 0 as usize);
    });
}
