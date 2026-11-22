// Source: Absorbed from /data/dev/bun/src/resolver/node_fallbacks.cp | Synced: 2026-11-22T01:50:00Z
// Pure-CP zero-allocation Node.js browser-target virtual polyfill registry & VFS path router
// (mirrors Bun `src/resolver/node_fallbacks.cp`).

/// Virtual filesystem prefix for browser-target Node.js polyfill modules (24 bytes, 8-byte aligned).
pub const NODE_FALLBACK_IMPORT_PATH: []const u8 = "/bun-vfs$$/node_modules/";
/// Virtual package version assigned to built-in polyfill modules.
pub const NODE_FALLBACK_VERSION: []const u8 = "0.0.0-polyfill";
/// Number of Node.js built-in modules with browser polyfill fallbacks.
pub const NODE_FALLBACK_COUNT: usize = 23;

/// Metadata for a single Node.js browser polyfill module.
pub struct NodeFallbackModule {
    /// Canonical module name (e.g. `"buffer"`, `"crypto"`, `"stream"`).
    name: []const u8 = "",
    /// Embedded polyfill asset path (e.g. `"node-fallbacks/buffer.js"`).
    code_path: []const u8 = "",
    /// Whether the polyfill module is declared side-effect-free (`sideEffects: false`).
    side_effects: bool = false,
    /// Virtual module format (`true` = ESM).
    is_esm: bool = true,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return strEq(s[0..prefix.len], prefix);
}

fn stripNodePrefix(specifier: []const u8): []const u8 {
    if (startsWith(specifier, "node:")) {
        return specifier[5..];
    }
    return specifier;
}

/// Look up a browser polyfill fallback for a Node.js module specifier (accepts both `"buffer"` and `"node:buffer"`).
/// Returns `null` for non-polyfillable host modules (e.g. `"fs"`, `"child_process"`) or unknown specifiers.
pub fn lookupNodeFallback(specifier: []const u8): NodeFallbackModule? {
    const name = stripNodePrefix(specifier);
    if (name.len == 0) return null;

    if (strEq(name, "assert")) return NodeFallbackModule{ name: "assert", code_path: "node-fallbacks/assert.js" };
    if (strEq(name, "buffer")) return NodeFallbackModule{ name: "buffer", code_path: "node-fallbacks/buffer.js" };
    if (strEq(name, "console")) return NodeFallbackModule{ name: "console", code_path: "node-fallbacks/console.js" };
    if (strEq(name, "constants")) return NodeFallbackModule{ name: "constants", code_path: "node-fallbacks/constants.js" };
    if (strEq(name, "crypto")) return NodeFallbackModule{ name: "crypto", code_path: "node-fallbacks/crypto.js" };
    if (strEq(name, "domain")) return NodeFallbackModule{ name: "domain", code_path: "node-fallbacks/domain.js" };
    if (strEq(name, "events")) return NodeFallbackModule{ name: "events", code_path: "node-fallbacks/events.js" };
    if (strEq(name, "http")) return NodeFallbackModule{ name: "http", code_path: "node-fallbacks/http.js" };
    if (strEq(name, "https")) return NodeFallbackModule{ name: "https", code_path: "node-fallbacks/https.js" };
    if (strEq(name, "net")) return NodeFallbackModule{ name: "net", code_path: "node-fallbacks/net.js" };
    if (strEq(name, "os")) return NodeFallbackModule{ name: "os", code_path: "node-fallbacks/os.js" };
    if (strEq(name, "path")) return NodeFallbackModule{ name: "path", code_path: "node-fallbacks/path.js" };
    if (strEq(name, "process")) return NodeFallbackModule{ name: "process", code_path: "node-fallbacks/process.js" };
    if (strEq(name, "punycode")) return NodeFallbackModule{ name: "punycode", code_path: "node-fallbacks/punycode.js" };
    if (strEq(name, "querystring")) return NodeFallbackModule{ name: "querystring", code_path: "node-fallbacks/querystring.js" };
    if (strEq(name, "stream")) return NodeFallbackModule{ name: "stream", code_path: "node-fallbacks/stream.js" };
    if (strEq(name, "string_decoder")) return NodeFallbackModule{ name: "string_decoder", code_path: "node-fallbacks/string_decoder.js" };
    if (strEq(name, "sys")) return NodeFallbackModule{ name: "sys", code_path: "node-fallbacks/sys.js" };
    if (strEq(name, "timers")) return NodeFallbackModule{ name: "timers", code_path: "node-fallbacks/timers.js" };
    if (strEq(name, "tty")) return NodeFallbackModule{ name: "tty", code_path: "node-fallbacks/tty.js" };
    if (strEq(name, "url")) return NodeFallbackModule{ name: "url", code_path: "node-fallbacks/url.js" };
    if (strEq(name, "util")) return NodeFallbackModule{ name: "util", code_path: "node-fallbacks/util.js" };
    if (strEq(name, "zlib")) return NodeFallbackModule{ name: "zlib", code_path: "node-fallbacks/zlib.js" };
    return null;
}

/// Returns true if `specifier` has a built-in browser polyfill fallback.
pub fn hasNodeFallback(specifier: []const u8): bool {
    return lookupNodeFallback(specifier) != null;
}

/// Returns true if `vfs_path` starts with `NODE_FALLBACK_IMPORT_PATH` (`"/bun-vfs$$/node_modules/"`).
pub fn isNodeFallbackVfsPath(vfs_path: []const u8): bool {
    return startsWith(vfs_path, NODE_FALLBACK_IMPORT_PATH);
}

/// Extract and validate the canonical fallback module from a virtual path
/// (e.g. `"/bun-vfs$$/node_modules/buffer/index.js"` -> `NodeFallbackModule{ name: "buffer", ... }`).
/// Mirrors Bun `node_fallbacks.contentsFromPath`.
pub fn resolveNodeFallbackFromVfsPath(vfs_path: []const u8): NodeFallbackModule? {
    if (!isNodeFallbackVfsPath(vfs_path)) return null;
    const rest = vfs_path[NODE_FALLBACK_IMPORT_PATH.len..];
    if (rest.len == 0) return null;

    let end: usize = 0;
    while (end < rest.len && rest[end] != '/') : (end += 1) {}
    if (end == 0) return null;

    return lookupNodeFallback(rest[0..end]);
}

fn formatVfsPath(specifier: []const u8, suffix: []const u8, out: []u8): ([]const u8)? {
    const mod = lookupNodeFallback(specifier) ?? return null;
    const total = NODE_FALLBACK_IMPORT_PATH.len + mod.name.len + suffix.len;
    if (out.len < total) return null;

    let pos: usize = 0;
    let i: usize = 0;
    while (i < NODE_FALLBACK_IMPORT_PATH.len) : (i += 1) {
        out[pos] = NODE_FALLBACK_IMPORT_PATH[i];
        pos += 1;
    }
    i = 0;
    while (i < mod.name.len) : (i += 1) {
        out[pos] = mod.name[i];
        pos += 1;
    }
    i = 0;
    while (i < suffix.len) : (i += 1) {
        out[pos] = suffix[i];
        pos += 1;
    }
    return out[0..pos];
}

/// Format the virtual entry path `"/bun-vfs$$/node_modules/<name>/index.js"` into `out`.
pub fn formatNodeFallbackEntryPath(specifier: []const u8, out: []u8): ([]const u8)? {
    return formatVfsPath(specifier, "/index.js", out);
}

/// Format the virtual manifest path `"/bun-vfs$$/node_modules/<name>/package.json"` into `out`.
pub fn formatNodeFallbackPackageJsonPath(specifier: []const u8, out: []u8): ([]const u8)? {
    return formatVfsPath(specifier, "/package.json", out);
}
