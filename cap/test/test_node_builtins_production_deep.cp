// Source: cp/cap/test/test_node_builtins_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { RuntimeTarget, resolveAlias, isBuiltin, canonicalizeNode } from "cap:node_builtins";

test "node_builtins: bare & node:-prefixed canonicalization and deprecated sys alias" {
    try expect_eq(std.mem.eql(u8, canonicalizeNode("fs/promises").?, "node:fs/promises"), true);
    try expect_eq(std.mem.eql(u8, canonicalizeNode("node:crypto").?, "node:crypto"), true);
    try expect_eq(std.mem.eql(u8, canonicalizeNode("sys").?, "node:util"), true);
    try expect_eq(std.mem.eql(u8, canonicalizeNode("_tls_wrap").?, "node:tls"), true);
    try expect_eq(canonicalizeNode("left-pad") == null, true);
}

test "node_builtins: node-only prefix modules (node:test, node:sqlite)" {
    const nt = resolveAlias("node:test", .Node, false).?;
    try expect_eq(nt.node_only_prefix, true);
    try expect_eq(resolveAlias("test", .Node, false) == null, true);
    try expect_eq(resolveAlias("sqlite", .Bun, false) == null, true);
    try expect_eq(resolveAlias("node:sqlite", .Bun, false) != null, true);
}

test "node_builtins: target isolation (Node vs Bun vs Browser) & jest/vitest rewrite" {
    try expect_eq(isBuiltin("bun:sqlite", .Bun), true);
    try expect_eq(isBuiltin("bun:sqlite", .Node), false);
    try expect_eq(isBuiltin("node:fs", .Browser), false);

    try expect_eq(resolveAlias("vitest", .Bun, false) == null, true);
    const v = resolveAlias("vitest", .Bun, true).?;
    try expect_eq(std.mem.eql(u8, v.path, "bun:test"), true);
}

test "node_builtins: third-party polyfill overrides on Bun target" {
    const ws = resolveAlias("ws/lib/websocket", .Bun, false).?;
    try expect_eq(std.mem.eql(u8, ws.path, "ws"), true);
    const ffi = resolveAlias("ffi", .Bun, false).?;
    try expect_eq(std.mem.eql(u8, ffi.path, "bun:ffi"), true);
}

test "node_builtins: cap.node_builtins dynamic mount" {
    try expect_eq(cap.node_builtins.isBuiltin("node:path/posix", .Node), true);
    try expect_eq(std.mem.eql(u8, cap.node_builtins.canonicalizeNode("zlib").?, "node:zlib"), true);
}
