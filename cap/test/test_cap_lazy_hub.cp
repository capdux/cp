// Regression test for `import cap from "cap";` compile-time lazy namespace mounting
// (covers single-file mode via `cap test` and modular mode via `cpc --emit-dir`)

import { test, expect_eq } from "cap:test";
import std from "std";
import cap from "cap";
import c from "cap";

const cap_at = @import("cap");

// Comments and strings mentioning unused namespaces must NEVER mount them:
// cap.css.Unused cap.http.Unused cap.sql.Unused cap.shell.Unused
const _ignored_str: []const u8 = "cap.md.Unused cap.s3.Unused cap.pack.Unused";
const _ignored_tpl: []const u8 = `cap.redis.Unused cap.elf.Unused`;

fn testCorePrimitives(): !void {
    expect_eq(cap.Environment.isLinux || cap.Environment.isMac || cap.Environment.isWindows, true);
    expect_eq(cap.Environment.is64Bit, true);

    expect_eq(cap.clamp(15, 0, 10), 10);
    expect_eq(cap.clamp(-5, 0, 10), 0);
    expect_eq(cap.clamp(7, 0, 10), 7);

    // handleOom unwraps OOM!T cleanly
    const buf = cap.handleOom(cap.default_allocator.alloc(u8, 16));
    defer cap.default_allocator.free(buf);
    expect_eq(buf.len, 16 as usize);

    // create / destroy / take / zero
    const ptr = cap.create(cap.default_allocator, i32, 99);
    expect_eq(ptr.*, 99);
    cap.destroy(ptr);

    let opt_val: i32? = 42;
    const taken = cap.take(&opt_val);
    expect_eq(taken.?, 42);
    expect_eq(opt_val == null, true);

    const z = cap.zero(i64);
    expect_eq(z, 0 as i64);

    const fd: cap.FD = cap.invalid_fd;
    expect_eq(fd, -1 as i32);
    expect_eq(cap.MAX_PATH_BYTES, 4096 as usize);
    expect_eq(cap.O.RDONLY, 0 as u32);
    expect_eq(cap.S.IRUSR != 0, true);

    // ComptimeStringMap & fmtIntSize
    const MethodMap = cap.ComptimeStringMap(i32, .{
        .{ "GET", 1 },
        .{ "POST", 2 },
    });
    expect_eq(MethodMap.get("POST").?, 2);
    expect_eq(MethodMap.has("DELETE"), false);

    let sz_buf: [64]u8 = undefined;
    const sz_str = cap.fmtIntSize(2048, &sz_buf);
    expect_eq(sz_str.len > 0, true);
}

fn testLazyNamespaces(): !void {
    expect_eq(cap.bits.isPowerOfTwo(64), true);
    expect_eq(cap.bits.isPowerOfTwo(63), false);
    expect_eq(cap_at.bits.alignUp(17, 16), 32 as usize);

    const e1 = cap.errno.SystemErrno.ENOENT;
    const e2 = c.errno.SystemErrno.ENOENT;
    expect_eq(e1 == e2, true);
    expect_eq(@intFromEnum(e1), 2 as u16);

    const h1 = cap.hash.Hash.fnv1a("cp");
    const h2 = c.hash.Hash.fnv1a("cp");
    const h3 = cap_at.hash.Hash.fnv1a("zig");
    expect_eq(h1 == h2, true);
    expect_eq(h1 != 0, true);
    expect_eq(h1 != h3, true);
}

fn main() {
    test("cap micro-core: Environment, clamp, handleOom, memory & low-level types", () => {
        testCorePrimitives()?;
    });

    test("cap lazy mounting: cap.bits, cap.errno, cap.hash via default & custom aliases", () => {
        testLazyNamespaces()?;
    });
}
