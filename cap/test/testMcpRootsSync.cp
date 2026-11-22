// Origin: Pure-CP Stdlib Test Suite (lib/mcp/rootsSync.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP Workspace Roots Sync & Path Boundary Sandbox Guard:
//   1. Zero-heap path normalization (., .., redundant slashes, root clamp, NUL/relative/backslash guard)
//   2. file:// URI parsing/formatting & strict directory-boundary matching (/app vs /app2)
//   3. Workspace roots registry CRUD, deduplication, upsert, policy mutation & monotonic revision tracking
//   4. Longest-prefix root matching with nested read_only / read_write overrides & traversal protection
//   5. Zero-alloc JSON-RPC 2.0 `roots/list` result & `notifications/roots/list_changed` encoding + escaping
//   6. Full 32-entry capacity saturation, slot reuse after removal, telemetry stats & cap.mcp facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_ROOTS_MAX_ENTRIES,
    MCP_ROOTS_PATH_MAX_LEN,
    MCP_ROOTS_URI_MAX_LEN,
    MCP_ROOTS_NAME_MAX_LEN,
    McpRootPolicy,
    McpRootOpStatus,
    McpPathAccessStatus,
    McpRootEntry,
    McpPathAuthDecision,
    McpRootsSyncStats,
    normalizeAbsolutePath,
    parseFileRootUri,
    formatFileRootUri,
    isPathWithinRoot,
    encodeRootsListChangedNotification,
    McpRootsRegistry,
} from "cap:mcp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("RootsSync: zero-heap path normalization & malformed/NUL/relative rejection") {
    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];

    const n1 = normalizeAbsolutePath("///workspace//./src/../lib/./mcp/", *out_s) ?? 0;
    expect_true(bytesEq(out[0..n1], "/workspace/lib/mcp"));

    // Leading ".." traversal clamped at "/"
    const n2 = normalizeAbsolutePath("/../../etc/./hosts", *out_s) ?? 0;
    expect_true(bytesEq(out[0..n2], "/etc/hosts"));

    const n3 = normalizeAbsolutePath("/a/b/../../../..", *out_s) ?? 0;
    expect_true(bytesEq(out[0..n3], "/"));

    const n4 = normalizeAbsolutePath("////", *out_s) ?? 0;
    expect_true(bytesEq(out[0..n4], "/"));

    // Reject empty, relative paths, backslashes, control chars, NUL bytes, and tiny buffers
    expect_true(normalizeAbsolutePath("", *out_s) == null);
    expect_true(normalizeAbsolutePath("relative/path.txt", *out_s) == null);
    expect_true(normalizeAbsolutePath("./local", *out_s) == null);
    expect_true(normalizeAbsolutePath("../parent", *out_s) == null);
    expect_true(normalizeAbsolutePath("/app\\secret", *out_s) == null);
    expect_true(normalizeAbsolutePath("/app/line\nbreak", *out_s) == null);

    let nul_path: [8]u8 = [0x2F, 0x61, 0x70, 0x70, 0x00, 0x2F, 0x78, 0x79]; // "/app\0/xy"
    expect_true(normalizeAbsolutePath(nul_path[0..], *out_s) == null);

    let tiny: [3]u8 = [0; 3];
    let tiny_s: []u8 = tiny[0..];
    expect_true(normalizeAbsolutePath("/workspace", *tiny_s) == null);
}

test("RootsSync: file:// URI parsing/formatting & strict directory boundary checks") {
    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];

    const p1 = parseFileRootUri("file:///data/dev/cp/", *out_s) ?? 0;
    expect_true(bytesEq(out[0..p1], "/data/dev/cp"));

    const p2 = parseFileRootUri("file://localhost/var//log/../tmp/", *out_s) ?? 0;
    expect_true(bytesEq(out[0..p2], "/var/tmp"));

    // Reject non-file scheme, remote host, and relative file URI
    expect_true(parseFileRootUri("https://example.com/app", *out_s) == null);
    expect_true(parseFileRootUri("file://remote-host/app", *out_s) == null);
    expect_true(parseFileRootUri("file://relative", *out_s) == null);
    expect_true(parseFileRootUri("file://", *out_s) == null);

    // Format back to canonical URI
    let uri_out: [128]u8 = [0; 128];
    let uri_s: []u8 = uri_out[0..];
    const ulen = formatFileRootUri("/data/dev/cp", *uri_s) ?? 0;
    expect_true(bytesEq(uri_out[0..ulen], "file:///data/dev/cp"));
    expect_true(formatFileRootUri("relative", *uri_s) == null);

    // Strict directory boundary: /app matches /app & /app/x, NEVER /app2 or /app_bak
    expect_true(isPathWithinRoot("/app", "/app"));
    expect_true(isPathWithinRoot("/app", "/app/src/main.cp"));
    expect_eq(isPathWithinRoot("/app", "/app2"), false);
    expect_eq(isPathWithinRoot("/app", "/app2/secret.env"), false);
    expect_eq(isPathWithinRoot("/app", "/app_backup"), false);
    expect_eq(isPathWithinRoot("/app/src", "/app"), false);
    expect_true(isPathWithinRoot("/", "/anything/nested"));
}

test("RootsSync: registry CRUD, deduplication, upsert & monotonic revision tracking") {
    let reg = McpRootsRegistry.init();
    expect_eq(reg.count(), 0 as usize);
    expect_eq(reg.revisionNumber(), 0 as u64);
    expect_eq(reg.hasPendingListChanged(), false);

    // Add first root (with trailing slash & redundant dots -> normalized to /workspace)
    expect_eq(
        reg.addRoot("file:///workspace/./", "Main Workspace", McpRootPolicy.read_write),
        McpRootOpStatus.ok,
    );
    expect_eq(reg.count(), 1 as usize);
    expect_eq(reg.revisionNumber(), 1 as u64);
    expect_true(reg.hasPendingListChanged());

    // Duplicate detection after normalization
    expect_eq(
        reg.addRoot("file://localhost/workspace//", "Dup", McpRootPolicy.read_only),
        McpRootOpStatus.already_exists,
    );
    expect_eq(reg.revisionNumber(), 1 as u64);

    // Reject invalid URI & overly long name
    expect_eq(reg.addRoot("http:///workspace", "Bad", McpRootPolicy.read_only), McpRootOpStatus.invalid_uri);
    let long_name: [MCP_ROOTS_NAME_MAX_LEN + 2]u8 = [0x61; MCP_ROOTS_NAME_MAX_LEN + 2];
    expect_eq(
        reg.addRoot("file:///tmp", long_name[0..], McpRootPolicy.read_only),
        McpRootOpStatus.name_too_long,
    );

    // Acknowledge list_changed clears pending flag
    expect_eq(reg.ackListChanged(), 1 as u64);
    expect_eq(reg.hasPendingListChanged(), false);

    // No-op upsert does not bump revision
    expect_eq(
        reg.upsertRoot("file:///workspace", "Main Workspace", McpRootPolicy.read_write),
        McpRootOpStatus.ok,
    );
    expect_eq(reg.revisionNumber(), 1 as u64);
    expect_eq(reg.hasPendingListChanged(), false);

    // Policy change via setPolicy bumps revision
    expect_eq(reg.setPolicy("/workspace", McpRootPolicy.read_only), McpRootOpStatus.ok);
    expect_eq(reg.revisionNumber(), 2 as u64);
    expect_true(reg.hasPendingListChanged());

    // Lookup & remove
    const slot = reg.findRoot("file:///workspace/") ?? 999;
    expect_true(slot < MCP_ROOTS_MAX_ENTRIES);
    if (entry := reg.getRoot(slot)) {
        expect_true(bytesEq(entry.pathSlice(), "/workspace"));
        expect_true(bytesEq(entry.nameSlice(), "Main Workspace"));
        expect_eq(entry.policy, McpRootPolicy.read_only);
    } else {
        expect_true(false);
    }

    expect_eq(reg.removeRoot("file:///workspace"), McpRootOpStatus.ok);
    expect_eq(reg.count(), 0 as usize);
    expect_eq(reg.revisionNumber(), 3 as u64);
    expect_eq(reg.removeRoot("/workspace"), McpRootOpStatus.not_found);
}

test("RootsSync: longest-prefix matching, nested read_only/read_write & boundary escape guard") {
    let reg = McpRootsRegistry.init();
    expect_eq(reg.addRoot("file:///app", "App Root", McpRootPolicy.read_write), McpRootOpStatus.ok);
    expect_eq(reg.addRoot("file:///app/.git", "Git Locked", McpRootPolicy.read_only), McpRootOpStatus.ok);
    expect_eq(reg.addRoot("file:///app/.git/tmp", "Git Scratch", McpRootPolicy.read_write), McpRootOpStatus.ok);
    expect_eq(reg.addRoot("file:///docs", "Docs", McpRootPolicy.read_only), McpRootOpStatus.ok);

    // Normal write inside /app
    const d1 = reg.authorizePath("/app/src/../README.md", true);
    expect_true(d1.allowed);
    expect_eq(d1.status, McpPathAccessStatus.allowed);
    expect_true(bytesEq(d1.normalizedSlice(), "/app/README.md"));
    expect_eq(d1.matched_policy, McpRootPolicy.read_write);

    // Nested read_only override (/app/.git) blocks writes but allows reads
    const d2_w = reg.authorizePath("/app/.git/config", true);
    expect_eq(d2_w.allowed, false);
    expect_eq(d2_w.status, McpPathAccessStatus.denied_read_only);
    expect_true(d2_w.has_match);

    const d2_r = reg.authorizePath("/app/.git/config", false);
    expect_true(d2_r.allowed);
    expect_eq(d2_r.status, McpPathAccessStatus.allowed);

    // Deeper read_write override (/app/.git/tmp) wins over /app/.git
    const d3 = reg.authorizePath("/app/.git/tmp/lock", true);
    expect_true(d3.allowed);
    expect_eq(d3.matched_policy, McpRootPolicy.read_write);

    // Strict boundary: /app2 must NOT match /app
    const d4 = reg.authorizePath("/app2/secret.key", false);
    expect_eq(d4.allowed, false);
    expect_eq(d4.status, McpPathAccessStatus.denied_outside_roots);

    // Traversal escaping /app -> /etc/passwd must be denied_outside_roots
    const d5 = reg.authorizePath("/app/../../etc/passwd", false);
    expect_eq(d5.allowed, false);
    expect_eq(d5.status, McpPathAccessStatus.denied_outside_roots);
    expect_true(bytesEq(d5.normalizedSlice(), "/etc/passwd"));

    // URI authorization also enforces sandbox & read_only policy
    const d6 = reg.authorizeUri("file:///docs/guide.md", false);
    expect_true(d6.allowed);
    const d7 = reg.authorizeUri("file:///docs/guide.md", true);
    expect_eq(d7.status, McpPathAccessStatus.denied_read_only);

    // Malformed relative path & invalid URI -> denied_invalid_path
    const d8 = reg.authorizePath("relative/file.txt", false);
    expect_eq(d8.status, McpPathAccessStatus.denied_invalid_path);
    const d9 = reg.authorizeUri("ftp:///app/file.txt", false);
    expect_eq(d9.status, McpPathAccessStatus.denied_invalid_path);

    const st = reg.stats();
    expect_eq(st.active_roots, 4 as usize);
    expect_eq(st.allowed_count, 4 as u64);
    expect_eq(st.denied_count, 6 as u64);
    expect_eq(st.write_denials, 2 as u64);
    expect_eq(st.outside_root_denials, 2 as u64);
    expect_eq(st.invalid_path_denials, 2 as u64);
}

test("RootsSync: zero-alloc JSON-RPC 2.0 roots/list & notifications/roots/list_changed encoding") {
    let reg = McpRootsRegistry.init();
    expect_eq(reg.addRoot("file:///workspace", "Core \"CP\"\nRepo", McpRootPolicy.read_write), McpRootOpStatus.ok);
    expect_eq(reg.addRoot("file:///readonly", "", McpRootPolicy.read_only), McpRootOpStatus.ok);

    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];

    // request_id == 0 rejected
    expect_true(reg.encodeRootsListResult(0, *buf_s) == null);

    const n = reg.encodeRootsListResult(42, *buf_s) ?? 0;
    expect_true(n > 0);
    const payload = buf[0..n];
    expect_true(containsSubstr(payload, "\"jsonrpc\":\"2.0\",\"id\":42,\"result\":{\"roots\":["));
    expect_true(containsSubstr(payload, "{\"uri\":\"file:///workspace\",\"name\":\"Core \\\"CP\\\"\\nRepo\"}"));
    // Empty name omits "name" field per MCP spec
    expect_true(containsSubstr(payload, "{\"uri\":\"file:///readonly\"}"));

    // Buffer-too-small returns null without corruption
    let small: [16]u8 = [0; 16];
    let small_s: []u8 = small[0..];
    expect_true(reg.encodeRootsListResult(42, *small_s) == null);

    // Pending list_changed notification emitted once and clears pending flag
    expect_true(reg.hasPendingListChanged());
    const n_notif = reg.encodePendingListChanged(*buf_s) ?? 0;
    expect_true(bytesEq(
        buf[0..n_notif],
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/roots/list_changed\"}",
    ));
    expect_eq(reg.hasPendingListChanged(), false);
    expect_true(reg.encodePendingListChanged(*buf_s) == null);

    // Standalone encoder also works and guards small buffer
    const n_raw = encodeRootsListChangedNotification(*buf_s) ?? 0;
    expect_eq(n_raw, n_notif);
    expect_true(encodeRootsListChangedNotification(*small_s) == null);
}

test("RootsSync: 32-entry capacity saturation, slot reuse, clear & cap.mcp facade parity") {
    let reg = McpRootsRegistry.init();
    let uri_buf: [64]u8 = [0; 64];

    // Fill all 32 slots: file:///r00 .. file:///r31
    let i: usize = 0;
    while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
        const prefix = "file:///r";
        let k: usize = 0;
        while (k < prefix.len) : (k += 1) {
            uri_buf[k] = prefix[k];
        }
        uri_buf[prefix.len] = (0x30 as u8) + ((i / 10) as u8);
        uri_buf[prefix.len + 1] = (0x30 as u8) + ((i % 10) as u8);
        const st = reg.addRoot(uri_buf[0 .. prefix.len + 2], "", McpRootPolicy.read_only);
        expect_eq(st, McpRootOpStatus.ok);
    }

    expect_eq(reg.count(), MCP_ROOTS_MAX_ENTRIES);
    expect_eq(reg.addRoot("file:///overflow", "Overflow", McpRootPolicy.read_only), McpRootOpStatus.capacity_full);

    // Remove one root and verify slot reuse
    expect_eq(reg.removeRoot("file:///r15"), McpRootOpStatus.ok);
    expect_eq(reg.count(), MCP_ROOTS_MAX_ENTRIES - 1);
    expect_eq(reg.addRoot("file:///reused", "Reused", McpRootPolicy.read_write), McpRootOpStatus.ok);
    expect_eq(reg.count(), MCP_ROOTS_MAX_ENTRIES);
    expect_true(reg.authorizePath("/reused/ok.cp", true).allowed);

    // Clear all roots
    const cleared = reg.clear();
    expect_eq(cleared, MCP_ROOTS_MAX_ENTRIES);
    expect_eq(reg.count(), 0 as usize);
    expect_eq(reg.clear(), 0 as usize);

    // Verify cap.mcp namespace facade parity
    let facade_reg = cap.mcp.McpRootsRegistry.init();
    expect_eq(
        facade_reg.addRoot("file:///facade", "Facade", cap.mcp.McpRootPolicy.read_write),
        cap.mcp.McpRootOpStatus.ok,
    );
    expect_true(facade_reg.authorizePath("/facade/a.cp", true).allowed);
    expect_eq(MCP_ROOTS_PATH_MAX_LEN, 256 as usize);
    expect_eq(MCP_ROOTS_URI_MAX_LEN, 264 as usize);
}
