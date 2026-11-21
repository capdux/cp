import { test, expect_eq } from "cap:test";
import {
    RuntimeTarget,
    BuiltinKind,
    ResolveConfig,
    hasNodePrefix,
    hasBunPrefix,
    stripNodePrefix,
    ensureNodePrefix,
    resolveBuiltin,
    isBuiltin,
    isNodeBuiltin,
} from "cap:builtin_modules";

test("1. Node.js core builtins, subpaths & deprecated aliases resolution", () => {
    const cfg = ResolveConfig{};

    const fs_bare = resolveBuiltin("fs", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, fs_bare.canonical, "node:fs"), true);
    expect_eq(fs_bare.isNodeBuiltin(), true);
    expect_eq(fs_bare.requiresNodePrefix(), false);

    const fs_pref = resolveBuiltin("node:fs/promises", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, fs_pref.canonical, "node:fs/promises"), true);

    const path_posix = resolveBuiltin("path/posix", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, path_posix.canonical, "node:path/posix"), true);

    const stream_web = resolveBuiltin("node:stream/web", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, stream_web.canonical, "node:stream/web"), true);

    // Deprecated aliases: sys -> node:util, _tls_wrap -> node:tls
    const sys_mod = resolveBuiltin("sys", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, sys_mod.canonical, "node:util"), true);

    const tls_wrap = resolveBuiltin("node:_tls_wrap", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, tls_wrap.canonical, "node:tls"), true);

    expect_eq(isNodeBuiltin("crypto"), true);
    expect_eq(isNodeBuiltin("node:worker_threads"), true);
    expect_eq(isNodeBuiltin("lodash"), false);
});

test("2. Prefix-only Node.js builtins (node:test, node:sqlite, node:sea) reject bare names", () => {
    const cfg = ResolveConfig{};

    const nt = resolveBuiltin("node:test", RuntimeTarget.node, cfg).?;
    expect_eq(std.mem.eql(u8, nt.canonical, "node:test"), true);
    expect_eq(nt.requiresNodePrefix(), true);
    expect_eq(resolveBuiltin("test", RuntimeTarget.node, cfg) == null, true);

    const ns = resolveBuiltin("node:sqlite", RuntimeTarget.node, cfg).?;
    expect_eq(ns.requiresNodePrefix(), true);
    expect_eq(resolveBuiltin("sqlite", RuntimeTarget.node, cfg) == null, true);

    const nsea = resolveBuiltin("node:sea", RuntimeTarget.bun, cfg).?;
    expect_eq(nsea.requiresNodePrefix(), true);
    expect_eq(resolveBuiltin("sea", RuntimeTarget.bun, cfg) == null, true);
});

test("3. RuntimeTarget differences across node, bun & browser", () => {
    const cfg = ResolveConfig{};

    // Browser target never resolves Node or Bun builtins
    expect_eq(RuntimeTarget.browser.isBrowser(), true);
    expect_eq(resolveBuiltin("fs", RuntimeTarget.browser, cfg) == null, true);
    expect_eq(resolveBuiltin("node:fs", RuntimeTarget.browser, cfg) == null, true);
    expect_eq(resolveBuiltin("bun:sqlite", RuntimeTarget.browser, cfg) == null, true);

    // Node target resolves Node builtins, rejects Bun-specific builtins & overrides
    expect_eq(isBuiltin("node:http2", RuntimeTarget.node, cfg), true);
    expect_eq(isBuiltin("bun:sqlite", RuntimeTarget.node, cfg), false);
    expect_eq(isBuiltin("ws/lib/websocket", RuntimeTarget.node, cfg), false);

    // Bun target resolves both Node builtins, Bun builtins & runtime overrides
    const b_sql = resolveBuiltin("bun:sqlite", RuntimeTarget.bun, cfg).?;
    expect_eq(b_sql.isBunBuiltin(), true);
    expect_eq(std.mem.eql(u8, b_sql.canonical, "bun:sqlite"), true);

    const ffi_alias = resolveBuiltin("ffi", RuntimeTarget.bun, cfg).?;
    expect_eq(std.mem.eql(u8, ffi_alias.canonical, "bun:ffi"), true);

    const ws_ov = resolveBuiltin("next/dist/compiled/ws", RuntimeTarget.bun, cfg).?;
    expect_eq(ws_ov.kind == BuiltinKind.runtime_override, true);
    expect_eq(std.mem.eql(u8, ws_ov.canonical, "ws"), true);
});

test("4. Jest & Vitest test rewrite switch (rewrite_jest_for_tests)", () => {
    const off_cfg = ResolveConfig{ rewrite_jest_for_tests: false };
    const on_cfg = ResolveConfig{ rewrite_jest_for_tests: true };

    expect_eq(resolveBuiltin("@jest/globals", RuntimeTarget.bun, off_cfg) == null, true);
    expect_eq(resolveBuiltin("vitest", RuntimeTarget.bun, off_cfg) == null, true);

    const jest_res = resolveBuiltin("@jest/globals", RuntimeTarget.bun, on_cfg).?;
    expect_eq(std.mem.eql(u8, jest_res.canonical, "bun:test"), true);

    const vitest_res = resolveBuiltin("vitest", RuntimeTarget.bun, on_cfg).?;
    expect_eq(std.mem.eql(u8, vitest_res.canonical, "bun:test"), true);

    // Node target ignores Bun test rewrite
    expect_eq(resolveBuiltin("@jest/globals", RuntimeTarget.node, on_cfg) == null, true);
});

test("5. Prefix utilities (stripNodePrefix, ensureNodePrefix, hasNodePrefix, hasBunPrefix) & edge cases", () => {
    expect_eq(hasNodePrefix("node:fs"), true);
    expect_eq(hasNodePrefix("fs"), false);
    expect_eq(hasBunPrefix("bun:test"), true);
    expect_eq(hasBunPrefix("bun"), false);

    expect_eq(std.mem.eql(u8, stripNodePrefix("node:path/win32"), "path/win32"), true);
    expect_eq(std.mem.eql(u8, stripNodePrefix("path/win32"), "path/win32"), true);
    expect_eq(std.mem.eql(u8, stripNodePrefix(""), ""), true);

    let buf: [32]u8 = std.mem.zeroes([32]u8);
    expect_eq(std.mem.eql(u8, ensureNodePrefix(buf[0..], "fs").?, "node:fs"), true);
    expect_eq(std.mem.eql(u8, ensureNodePrefix(buf[0..], "node:fs").?, "node:fs"), true);

    // Edge cases: empty specifier, bare "node:", tiny buffer
    expect_eq(ensureNodePrefix(buf[0..], "") == null, true);
    expect_eq(ensureNodePrefix(buf[0..], "node:") == null, true);
    expect_eq(ensureNodePrefix(buf[0..4], "fs") == null, true);
    expect_eq(isNodeBuiltin(""), false);
    expect_eq(isNodeBuiltin("node:"), false);
    expect_eq(isBuiltin("node:nonexistent", RuntimeTarget.bun, ResolveConfig{}), false);
});
