// Source: Absorbed from /data/dev/bun/src/resolve_builtins/HardcodedModule.cp | Synced: 2026-11-21T12:57:20Z

/// 目标运行时枚举
pub enum RuntimeTarget: u8 {
    node = 0,
    bun = 1,
    browser = 2,

    fn isNode(): bool {
        return _ == .node;
    }

    fn isBun(): bool {
        return _ == .bun;
    }

    fn isBrowser(): bool {
        return _ == .browser;
    }
}

/// 内置模块类别
pub enum BuiltinKind: u8 {
    node_builtin = 0,
    bun_builtin = 1,
    runtime_override = 2,
}

/// 内置模块解析选项
pub struct ResolveConfig {
    rewrite_jest_for_tests: bool = false,
}

/// 解析后的规范化内置模块描述符
pub struct ResolvedBuiltin {
    canonical: string,
    kind: BuiltinKind = BuiltinKind.node_builtin,
    node_builtin: bool = false,
    node_only_prefix: bool = false,

    fn isNodeBuiltin(): bool {
        return _.node_builtin;
    }

    fn isBunBuiltin(): bool {
        return _.kind == BuiltinKind.bun_builtin;
    }

    fn requiresNodePrefix(): bool {
        return _.node_only_prefix;
    }
}

/// 判断模块说明符是否带 `node:` 前缀
pub fn hasNodePrefix(specifier: string): bool {
    return specifier.len >= 5 && std.mem.eql(u8, specifier[0..5], "node:");
}

/// 判断模块说明符是否带 `bun:` 前缀
pub fn hasBunPrefix(specifier: string): bool {
    return specifier.len >= 4 && std.mem.eql(u8, specifier[0..4], "bun:");
}

/// 剥离 `node:` 前缀（无前缀时原样返回）
pub fn stripNodePrefix(specifier: string): string {
    if (hasNodePrefix(specifier)) {
        return specifier[5..];
    }
    return specifier;
}

/// 确保模块说明符带 `node:` 前缀并写入目标缓冲区（空串或缓冲区不足返回 `null`）
pub fn ensureNodePrefix(buf: []u8, specifier: string): string? {
    const bare = stripNodePrefix(specifier);
    if (bare.len == 0) {
        return null;
    }
    const need: usize = usize(5) + bare.len;
    if (buf.len < need) {
        return null;
    }
    @memcpy(buf[0..5], "node:");
    @memcpy(buf[5..need], bare);
    return buf[0..need];
}

struct NodeSpecPair {
    bare: string,
    canonical: string,
}

const NODE_CORE_MODULES: [66]NodeSpecPair = [
    NodeSpecPair{ bare: "assert", canonical: "node:assert" },
    NodeSpecPair{ bare: "assert/strict", canonical: "node:assert/strict" },
    NodeSpecPair{ bare: "async_hooks", canonical: "node:async_hooks" },
    NodeSpecPair{ bare: "buffer", canonical: "node:buffer" },
    NodeSpecPair{ bare: "child_process", canonical: "node:child_process" },
    NodeSpecPair{ bare: "cluster", canonical: "node:cluster" },
    NodeSpecPair{ bare: "console", canonical: "node:console" },
    NodeSpecPair{ bare: "constants", canonical: "node:constants" },
    NodeSpecPair{ bare: "crypto", canonical: "node:crypto" },
    NodeSpecPair{ bare: "dgram", canonical: "node:dgram" },
    NodeSpecPair{ bare: "diagnostics_channel", canonical: "node:diagnostics_channel" },
    NodeSpecPair{ bare: "dns", canonical: "node:dns" },
    NodeSpecPair{ bare: "dns/promises", canonical: "node:dns/promises" },
    NodeSpecPair{ bare: "domain", canonical: "node:domain" },
    NodeSpecPair{ bare: "events", canonical: "node:events" },
    NodeSpecPair{ bare: "fs", canonical: "node:fs" },
    NodeSpecPair{ bare: "fs/promises", canonical: "node:fs/promises" },
    NodeSpecPair{ bare: "http", canonical: "node:http" },
    NodeSpecPair{ bare: "http2", canonical: "node:http2" },
    NodeSpecPair{ bare: "https", canonical: "node:https" },
    NodeSpecPair{ bare: "inspector", canonical: "node:inspector" },
    NodeSpecPair{ bare: "inspector/promises", canonical: "node:inspector/promises" },
    NodeSpecPair{ bare: "module", canonical: "node:module" },
    NodeSpecPair{ bare: "net", canonical: "node:net" },
    NodeSpecPair{ bare: "os", canonical: "node:os" },
    NodeSpecPair{ bare: "path", canonical: "node:path" },
    NodeSpecPair{ bare: "path/posix", canonical: "node:path/posix" },
    NodeSpecPair{ bare: "path/win32", canonical: "node:path/win32" },
    NodeSpecPair{ bare: "perf_hooks", canonical: "node:perf_hooks" },
    NodeSpecPair{ bare: "process", canonical: "node:process" },
    NodeSpecPair{ bare: "punycode", canonical: "node:punycode" },
    NodeSpecPair{ bare: "querystring", canonical: "node:querystring" },
    NodeSpecPair{ bare: "readline", canonical: "node:readline" },
    NodeSpecPair{ bare: "readline/promises", canonical: "node:readline/promises" },
    NodeSpecPair{ bare: "repl", canonical: "node:repl" },
    NodeSpecPair{ bare: "stream", canonical: "node:stream" },
    NodeSpecPair{ bare: "stream/consumers", canonical: "node:stream/consumers" },
    NodeSpecPair{ bare: "stream/promises", canonical: "node:stream/promises" },
    NodeSpecPair{ bare: "stream/web", canonical: "node:stream/web" },
    NodeSpecPair{ bare: "string_decoder", canonical: "node:string_decoder" },
    NodeSpecPair{ bare: "timers", canonical: "node:timers" },
    NodeSpecPair{ bare: "timers/promises", canonical: "node:timers/promises" },
    NodeSpecPair{ bare: "tls", canonical: "node:tls" },
    NodeSpecPair{ bare: "trace_events", canonical: "node:trace_events" },
    NodeSpecPair{ bare: "tty", canonical: "node:tty" },
    NodeSpecPair{ bare: "url", canonical: "node:url" },
    NodeSpecPair{ bare: "util", canonical: "node:util" },
    NodeSpecPair{ bare: "util/types", canonical: "node:util/types" },
    NodeSpecPair{ bare: "v8", canonical: "node:v8" },
    NodeSpecPair{ bare: "vm", canonical: "node:vm" },
    NodeSpecPair{ bare: "wasi", canonical: "node:wasi" },
    NodeSpecPair{ bare: "worker_threads", canonical: "node:worker_threads" },
    NodeSpecPair{ bare: "zlib", canonical: "node:zlib" },
    // 内部兼容模块与弃用别名
    NodeSpecPair{ bare: "_http_agent", canonical: "node:_http_agent" },
    NodeSpecPair{ bare: "_http_client", canonical: "node:_http_client" },
    NodeSpecPair{ bare: "_http_common", canonical: "node:_http_common" },
    NodeSpecPair{ bare: "_http_incoming", canonical: "node:_http_incoming" },
    NodeSpecPair{ bare: "_http_outgoing", canonical: "node:_http_outgoing" },
    NodeSpecPair{ bare: "_http_server", canonical: "node:_http_server" },
    NodeSpecPair{ bare: "_stream_duplex", canonical: "node:_stream_duplex" },
    NodeSpecPair{ bare: "_stream_passthrough", canonical: "node:_stream_passthrough" },
    NodeSpecPair{ bare: "_stream_readable", canonical: "node:_stream_readable" },
    NodeSpecPair{ bare: "_stream_transform", canonical: "node:_stream_transform" },
    NodeSpecPair{ bare: "_stream_wrap", canonical: "node:_stream_wrap" },
    NodeSpecPair{ bare: "_stream_writable", canonical: "node:_stream_writable" },
    NodeSpecPair{ bare: "_tls_common", canonical: "node:_tls_common" },
];

const NODE_ONLY_PREFIX_MODULES: [3]string = [
    "node:test",
    "node:sqlite",
    "node:sea",
];

fn resolveNodeBuiltin(specifier: string): ResolvedBuiltin? {
    if (specifier.len == 0) {
        return null;
    }

    // 仅限 `node:` 前缀的新 Node.js 内置模块（禁止裸名解析）
    for (only_mod of NODE_ONLY_PREFIX_MODULES) {
        if (std.mem.eql(u8, specifier, only_mod)) {
            return ResolvedBuiltin{
                canonical: only_mod,
                kind: BuiltinKind.node_builtin,
                node_builtin: true,
                node_only_prefix: true,
            };
        }
    }

    const bare = stripNodePrefix(specifier);
    if (bare.len == 0) {
        return null;
    }

    // 弃用别名映射
    if (std.mem.eql(u8, bare, "sys")) {
        return ResolvedBuiltin{
            canonical: "node:util",
            kind: BuiltinKind.node_builtin,
            node_builtin: true,
            node_only_prefix: false,
        };
    }
    if (std.mem.eql(u8, bare, "_tls_wrap")) {
        return ResolvedBuiltin{
            canonical: "node:tls",
            kind: BuiltinKind.node_builtin,
            node_builtin: true,
            node_only_prefix: false,
        };
    }

    for (entry of NODE_CORE_MODULES) {
        if (std.mem.eql(u8, bare, entry.bare)) {
            return ResolvedBuiltin{
                canonical: entry.canonical,
                kind: BuiltinKind.node_builtin,
                node_builtin: true,
                node_only_prefix: false,
            };
        }
    }
    return null;
}

struct AliasPair {
    input: string,
    canonical: string,
    kind: BuiltinKind,
}

const BUN_EXTRA_ALIASES: [22]AliasPair = [
    AliasPair{ input: "bun", canonical: "bun", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:test", canonical: "bun:test", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:app", canonical: "bun:app", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:ffi", canonical: "bun:ffi", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:jsc", canonical: "bun:jsc", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:main", canonical: "bun:main", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:sqlite", canonical: "bun:sqlite", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:wrap", canonical: "bun:wrap", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "bun:internal-for-testing", canonical: "bun:internal-for-testing", kind: BuiltinKind.bun_builtin },
    AliasPair{ input: "ffi", canonical: "bun:ffi", kind: BuiltinKind.bun_builtin },
    // 第三方原生替换与 Polyfill 覆盖
    AliasPair{ input: "@vercel/fetch", canonical: "@vercel/fetch", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "isomorphic-fetch", canonical: "isomorphic-fetch", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "node-fetch", canonical: "node-fetch", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "undici", canonical: "undici", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "utf-8-validate", canonical: "utf-8-validate", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "ws", canonical: "ws", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "ws/lib/websocket", canonical: "ws", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "abort-controller", canonical: "abort-controller", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "abort-controller/polyfill", canonical: "abort-controller", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "next/dist/compiled/ws", canonical: "ws", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "next/dist/compiled/node-fetch", canonical: "node-fetch", kind: BuiltinKind.runtime_override },
    AliasPair{ input: "next/dist/compiled/undici", canonical: "undici", kind: BuiltinKind.runtime_override },
];

fn resolveBunExtra(specifier: string, cfg: ResolveConfig): ResolvedBuiltin? {
    for (entry of BUN_EXTRA_ALIASES) {
        if (std.mem.eql(u8, specifier, entry.input)) {
            return ResolvedBuiltin{
                canonical: entry.canonical,
                kind: entry.kind,
                node_builtin: false,
                node_only_prefix: false,
            };
        }
    }
    if (cfg.rewrite_jest_for_tests) {
        if (std.mem.eql(u8, specifier, "@jest/globals") || std.mem.eql(u8, specifier, "vitest")) {
            return ResolvedBuiltin{
                canonical: "bun:test",
                kind: BuiltinKind.runtime_override,
                node_builtin: false,
                node_only_prefix: false,
            };
        }
    }
    return null;
}

/// 按目标运行时（`node` / `bun` / `browser`）与解析配置解析内置模块，非内置模块返回 `null`。
pub fn resolveBuiltin(specifier: string, target: RuntimeTarget, cfg: ResolveConfig): ResolvedBuiltin? {
    if (target == RuntimeTarget.browser) {
        return null;
    }
    if (target == RuntimeTarget.node) {
        return resolveNodeBuiltin(specifier);
    }
    if (node_res := resolveNodeBuiltin(specifier)) {
        return node_res;
    }
    return resolveBunExtra(specifier, cfg);
}

/// 判断给定模块说明符在目标运行时下是否为内置模块
pub fn isBuiltin(specifier: string, target: RuntimeTarget, cfg: ResolveConfig): bool {
    return resolveBuiltin(specifier, target, cfg) != null;
}

/// 判断给定模块说明符是否为 Node.js 标准内置模块
pub fn isNodeBuiltin(specifier: string): bool {
    return resolveNodeBuiltin(specifier) != null;
}
