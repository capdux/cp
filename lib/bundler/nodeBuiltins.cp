// Source: absorbed from bun/src/resolve_builtins/HardcodedModule.cp (cap:node_builtins)
const std = @import("std");

pub const RuntimeTarget = enum {
    Node,
    Bun,
    Browser,
};

pub const ModuleTag = enum {
    builtin,
    bun,
};

pub const BuiltinAlias = struct {
    path: []const u8,
    tag: ModuleTag = .builtin,
    node_builtin: bool = false,
    node_only_prefix: bool = false,
};

pub type OptBuiltinAlias = BuiltinAlias?;

fn nodeEntry(comptime p: []const u8): BuiltinAlias {
    const canon = if (p.len > 5 and std.mem.eql(u8, p[0..5], "node:")) p else "node:" ++ p;
    return BuiltinAlias{ .path = canon, .node_builtin = true };
}

fn nodeOnlyPrefix(comptime p: []const u8): BuiltinAlias {
    const canon = if (p.len > 5 and std.mem.eql(u8, p[0..5], "node:")) p else "node:" ++ p;
    return BuiltinAlias{ .path = canon, .node_builtin = true, .node_only_prefix = true };
}

const node_map = std.StaticStringMap(BuiltinAlias).initComptime(.{
    .{ "node:assert", nodeEntry("node:assert") },
    .{ "node:assert/strict", nodeEntry("node:assert/strict") },
    .{ "node:async_hooks", nodeEntry("node:async_hooks") },
    .{ "node:buffer", nodeEntry("node:buffer") },
    .{ "node:child_process", nodeEntry("node:child_process") },
    .{ "node:cluster", nodeEntry("node:cluster") },
    .{ "node:console", nodeEntry("node:console") },
    .{ "node:constants", nodeEntry("node:constants") },
    .{ "node:crypto", nodeEntry("node:crypto") },
    .{ "node:dgram", nodeEntry("node:dgram") },
    .{ "node:diagnostics_channel", nodeEntry("node:diagnostics_channel") },
    .{ "node:dns", nodeEntry("node:dns") },
    .{ "node:dns/promises", nodeEntry("node:dns/promises") },
    .{ "node:domain", nodeEntry("node:domain") },
    .{ "node:events", nodeEntry("node:events") },
    .{ "node:fs", nodeEntry("node:fs") },
    .{ "node:fs/promises", nodeEntry("node:fs/promises") },
    .{ "node:http", nodeEntry("node:http") },
    .{ "node:http2", nodeEntry("node:http2") },
    .{ "node:https", nodeEntry("node:https") },
    .{ "node:inspector", nodeEntry("node:inspector") },
    .{ "node:inspector/promises", nodeEntry("node:inspector/promises") },
    .{ "node:module", nodeEntry("node:module") },
    .{ "node:net", nodeEntry("node:net") },
    .{ "node:os", nodeEntry("node:os") },
    .{ "node:path", nodeEntry("node:path") },
    .{ "node:path/posix", nodeEntry("node:path/posix") },
    .{ "node:path/win32", nodeEntry("node:path/win32") },
    .{ "node:perf_hooks", nodeEntry("node:perf_hooks") },
    .{ "node:process", nodeEntry("node:process") },
    .{ "node:punycode", nodeEntry("node:punycode") },
    .{ "node:querystring", nodeEntry("node:querystring") },
    .{ "node:readline", nodeEntry("node:readline") },
    .{ "node:readline/promises", nodeEntry("node:readline/promises") },
    .{ "node:repl", nodeEntry("node:repl") },
    .{ "node:stream", nodeEntry("node:stream") },
    .{ "node:stream/consumers", nodeEntry("node:stream/consumers") },
    .{ "node:stream/promises", nodeEntry("node:stream/promises") },
    .{ "node:stream/web", nodeEntry("node:stream/web") },
    .{ "node:string_decoder", nodeEntry("node:string_decoder") },
    .{ "node:timers", nodeEntry("node:timers") },
    .{ "node:timers/promises", nodeEntry("node:timers/promises") },
    .{ "node:tls", nodeEntry("node:tls") },
    .{ "node:trace_events", nodeEntry("node:trace_events") },
    .{ "node:tty", nodeEntry("node:tty") },
    .{ "node:url", nodeEntry("node:url") },
    .{ "node:util", nodeEntry("node:util") },
    .{ "node:util/types", nodeEntry("node:util/types") },
    .{ "node:v8", nodeEntry("node:v8") },
    .{ "node:vm", nodeEntry("node:vm") },
    .{ "node:wasi", nodeEntry("node:wasi") },
    .{ "node:worker_threads", nodeEntry("node:worker_threads") },
    .{ "node:zlib", nodeEntry("node:zlib") },
    .{ "node:test", nodeOnlyPrefix("node:test") },
    .{ "node:sqlite", nodeOnlyPrefix("node:sqlite") },

    // Bare Node.js specifiers
    .{ "assert", nodeEntry("assert") },
    .{ "assert/strict", nodeEntry("assert/strict") },
    .{ "async_hooks", nodeEntry("async_hooks") },
    .{ "buffer", nodeEntry("buffer") },
    .{ "child_process", nodeEntry("child_process") },
    .{ "cluster", nodeEntry("cluster") },
    .{ "console", nodeEntry("console") },
    .{ "constants", nodeEntry("constants") },
    .{ "crypto", nodeEntry("crypto") },
    .{ "dgram", nodeEntry("dgram") },
    .{ "diagnostics_channel", nodeEntry("diagnostics_channel") },
    .{ "dns", nodeEntry("dns") },
    .{ "dns/promises", nodeEntry("dns/promises") },
    .{ "domain", nodeEntry("domain") },
    .{ "events", nodeEntry("events") },
    .{ "fs", nodeEntry("fs") },
    .{ "fs/promises", nodeEntry("fs/promises") },
    .{ "http", nodeEntry("http") },
    .{ "http2", nodeEntry("http2") },
    .{ "https", nodeEntry("https") },
    .{ "inspector", nodeEntry("inspector") },
    .{ "inspector/promises", nodeEntry("inspector/promises") },
    .{ "module", nodeEntry("module") },
    .{ "net", nodeEntry("net") },
    .{ "os", nodeEntry("os") },
    .{ "path", nodeEntry("path") },
    .{ "path/posix", nodeEntry("path/posix") },
    .{ "path/win32", nodeEntry("path/win32") },
    .{ "perf_hooks", nodeEntry("perf_hooks") },
    .{ "process", nodeEntry("process") },
    .{ "punycode", nodeEntry("punycode") },
    .{ "querystring", nodeEntry("querystring") },
    .{ "readline", nodeEntry("readline") },
    .{ "readline/promises", nodeEntry("readline/promises") },
    .{ "repl", nodeEntry("repl") },
    .{ "stream", nodeEntry("stream") },
    .{ "stream/consumers", nodeEntry("stream/consumers") },
    .{ "stream/promises", nodeEntry("stream/promises") },
    .{ "stream/web", nodeEntry("stream/web") },
    .{ "string_decoder", nodeEntry("string_decoder") },
    .{ "timers", nodeEntry("timers") },
    .{ "timers/promises", nodeEntry("timers/promises") },
    .{ "tls", nodeEntry("tls") },
    .{ "trace_events", nodeEntry("trace_events") },
    .{ "tty", nodeEntry("tty") },
    .{ "url", nodeEntry("url") },
    .{ "util", nodeEntry("util") },
    .{ "util/types", nodeEntry("util/types") },
    .{ "v8", nodeEntry("v8") },
    .{ "vm", nodeEntry("vm") },
    .{ "wasi", nodeEntry("wasi") },
    .{ "worker_threads", nodeEntry("worker_threads") },
    .{ "zlib", nodeEntry("zlib") },

    // Deprecated & internal aliases
    .{ "sys", BuiltinAlias{ .path = "node:util", .node_builtin = true } },
    .{ "node:sys", BuiltinAlias{ .path = "node:util", .node_builtin = true } },
    .{ "node:_http_agent", nodeEntry("node:_http_agent") },
    .{ "node:_http_client", nodeEntry("node:_http_client") },
    .{ "node:_http_common", nodeEntry("node:_http_common") },
    .{ "node:_http_incoming", nodeEntry("node:_http_incoming") },
    .{ "node:_http_outgoing", nodeEntry("node:_http_outgoing") },
    .{ "node:_http_server", nodeEntry("node:_http_server") },
    .{ "_http_agent", nodeEntry("_http_agent") },
    .{ "_http_client", nodeEntry("_http_client") },
    .{ "_http_common", nodeEntry("_http_common") },
    .{ "_http_incoming", nodeEntry("_http_incoming") },
    .{ "_http_outgoing", nodeEntry("_http_outgoing") },
    .{ "_http_server", nodeEntry("_http_server") },
    .{ "node:_tls_wrap", BuiltinAlias{ .path = "node:tls", .node_builtin = true } },
    .{ "_tls_wrap", BuiltinAlias{ .path = "node:tls", .node_builtin = true } },
});

const bun_extra_map = std.StaticStringMap(BuiltinAlias).initComptime(.{
    .{ "bun", BuiltinAlias{ .path = "bun", .tag = .bun } },
    .{ "bun:test", BuiltinAlias{ .path = "bun:test" } },
    .{ "bun:app", BuiltinAlias{ .path = "bun:app" } },
    .{ "bun:ffi", BuiltinAlias{ .path = "bun:ffi" } },
    .{ "bun:jsc", BuiltinAlias{ .path = "bun:jsc" } },
    .{ "bun:main", BuiltinAlias{ .path = "bun:main" } },
    .{ "bun:sqlite", BuiltinAlias{ .path = "bun:sqlite" } },
    .{ "bun:wrap", BuiltinAlias{ .path = "bun:wrap" } },
    .{ "ffi", BuiltinAlias{ .path = "bun:ffi" } },
    .{ "undici", BuiltinAlias{ .path = "undici" } },
    .{ "node-fetch", BuiltinAlias{ .path = "node-fetch" } },
    .{ "isomorphic-fetch", BuiltinAlias{ .path = "isomorphic-fetch" } },
    .{ "ws", BuiltinAlias{ .path = "ws" } },
    .{ "ws/lib/websocket", BuiltinAlias{ .path = "ws" } },
    .{ "abort-controller", BuiltinAlias{ .path = "abort-controller" } },
});

const bun_test_map = std.StaticStringMap(BuiltinAlias).initComptime(.{
    .{ "@jest/globals", BuiltinAlias{ .path = "bun:test" } },
    .{ "vitest", BuiltinAlias{ .path = "bun:test" } },
});

pub fn resolveAlias(name: []const u8, target: RuntimeTarget, rewrite_jest_for_tests: bool): BuiltinAlias? {
    switch (target) {
        .Browser => return null,
        .Node => return node_map.get(name),
        .Bun => {
            if (rewrite_jest_for_tests) {
                if (t := bun_test_map.get(name)) { return t; }
            }
            if (b := bun_extra_map.get(name)) { return b; }
            return node_map.get(name);
        },
    }
}

pub fn isBuiltin(name: []const u8, target: RuntimeTarget): bool {
    return resolveAlias(name, target, false) != null;
}

pub fn canonicalizeNode(name: []const u8): ([]const u8)? {
    if (entry := node_map.get(name)) {
        return entry.path;
    }
    return null;
}
