import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NODE_FALLBACK_IMPORT_PATH,
    NODE_FALLBACK_VERSION,
    NODE_FALLBACK_COUNT,
    NodeFallbackModule,
    lookupNodeFallback,
    hasNodeFallback,
    isNodeFallbackVfsPath,
    resolveNodeFallbackFromVfsPath,
    formatNodeFallbackEntryPath,
    formatNodeFallbackPackageJsonPath,
} from "cap:resolver";

fn main() {
    test("resolver_node_fallbacks: constants alignment & all 23 browser polyfills", () => {
        expect_eq(NODE_FALLBACK_IMPORT_PATH.len % 8, 0 as usize);
        expect_eq(std.mem.eql(u8, NODE_FALLBACK_VERSION, "0.0.0-polyfill"), true);
        expect_eq(NODE_FALLBACK_COUNT, 23 as usize);

        const all_names: [23][]const u8 = [
            "assert", "buffer", "console", "constants", "crypto",
            "domain", "events", "http", "https", "net",
            "os", "path", "process", "punycode", "querystring",
            "stream", "string_decoder", "sys", "timers", "tty",
            "url", "util", "zlib",
        ];
        let i: usize = 0;
        while (i < all_names.len) : (i += 1) {
            const mod: NodeFallbackModule = lookupNodeFallback(all_names[i]).?;
            expect_eq(std.mem.eql(u8, mod.name, all_names[i]), true);
            expect_eq(mod.side_effects, false);
            expect_eq(mod.is_esm, true);
        }
    });

    test("resolver_node_fallbacks: accepts node: prefix & rejects non-polyfillable host modules", () => {
        const m1: NodeFallbackModule = lookupNodeFallback("node:buffer").?;
        expect_eq(std.mem.eql(u8, m1.name, "buffer"), true);
        expect_eq(std.mem.eql(u8, m1.code_path, "node-fallbacks/buffer.js"), true);
        expect_eq(hasNodeFallback("node:crypto"), true);
        expect_eq(hasNodeFallback("string_decoder"), true);

        // Host-only Node modules do NOT have browser fallbacks
        expect_eq(hasNodeFallback("fs"), false);
        expect_eq(hasNodeFallback("node:fs"), false);
        expect_eq(hasNodeFallback("child_process"), false);
        expect_eq(hasNodeFallback("worker_threads"), false);
        expect_eq(hasNodeFallback("dgram"), false);
        // Case sensitivity & empty/prefix-only
        expect_eq(hasNodeFallback("Buffer"), false);
        expect_eq(hasNodeFallback("node:"), false);
        expect_eq(hasNodeFallback(""), false);
    });

    test("resolver_node_fallbacks: format VFS entry & package.json paths", () => {
        let buf1: [128]u8 = [0; 128];
        let buf2: [128]u8 = [0; 128];

        const entry = formatNodeFallbackEntryPath("node:stream", buf1[0..]).?;
        expect_eq(std.mem.eql(u8, entry, "/bun-vfs$$/node_modules/stream/index.js"), true);
        expect_eq(isNodeFallbackVfsPath(entry), true);

        const pkg = formatNodeFallbackPackageJsonPath("util", buf2[0..]).?;
        expect_eq(std.mem.eql(u8, pkg, "/bun-vfs$$/node_modules/util/package.json"), true);

        // Unknown module returns null
        expect_eq(formatNodeFallbackEntryPath("fs", buf1[0..]) == null, true);
        // Buffer too small returns null safely
        let tiny: [16]u8 = [0; 16];
        expect_eq(formatNodeFallbackEntryPath("buffer", tiny[0..]) == null, true);
    });

    test("resolver_node_fallbacks: resolveNodeFallbackFromVfsPath round-trip", () => {
        let buf: [128]u8 = [0; 128];
        const entry = formatNodeFallbackEntryPath("querystring", buf[0..]).?;
        const resolved: NodeFallbackModule = resolveNodeFallbackFromVfsPath(entry).?;
        expect_eq(std.mem.eql(u8, resolved.name, "querystring"), true);
        expect_eq(std.mem.eql(u8, resolved.code_path, "node-fallbacks/querystring.js"), true);

        // Also works without trailing subpath
        const bare: NodeFallbackModule = resolveNodeFallbackFromVfsPath("/bun-vfs$$/node_modules/zlib").?;
        expect_eq(std.mem.eql(u8, bare.name, "zlib"), true);
    });

    test("resolver_node_fallbacks: rejects invalid & traversal VFS paths", () => {
        expect_eq(isNodeFallbackVfsPath("/node_modules/buffer/index.js"), false);
        expect_eq(resolveNodeFallbackFromVfsPath("/node_modules/buffer/index.js") == null, true);
        expect_eq(resolveNodeFallbackFromVfsPath("/bun-vfs$$/node_modules/") == null, true);
        expect_eq(resolveNodeFallbackFromVfsPath("/bun-vfs$$/node_modules//buffer") == null, true);
        expect_eq(resolveNodeFallbackFromVfsPath("/bun-vfs$$/node_modules/../etc/passwd") == null, true);
        expect_eq(resolveNodeFallbackFromVfsPath("/bun-vfs$$/node_modules/fs/index.js") == null, true);
    });
}
