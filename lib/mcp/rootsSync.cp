// Origin: Pure-CP Stdlib (Zero-Alloc MCP Workspace Roots Sync & Path Sandbox Guard) | Created: 2026-11-23
// Pure CP zero-heap-allocation Model Context Protocol (MCP 2025-03-26 / 2025-06-18) Workspace Roots
// registry, path normalization, longest-prefix sandbox boundary guard & JSON-RPC 2.0 codec:
//   - Fixed 32-entry `file://` workspace root registry with normalized absolute paths, optional names,
//     read_only / read_write policies & monotonic revision tracking for `notifications/roots/list_changed`
//   - Zero-heap path normalization (`.` / `..` / redundant `/` collapsing, root traversal clamping,
//     NUL / control-char / backslash / relative-path rejection) & strict directory-boundary matching
//     (`/app` never matches `/app2`) with longest-prefix policy precedence
//   - Zero-alloc JSON-RPC 2.0 `roots/list` result encoder & `notifications/roots/list_changed` encoder
//   - Full operational telemetry snapshot (`McpRootsSyncStats`)

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of workspace roots stored inline (zero dynamic heap).
pub const MCP_ROOTS_MAX_ENTRIES: usize = 32;

/// Maximum byte length of a normalized absolute POSIX path.
pub const MCP_ROOTS_PATH_MAX_LEN: usize = 256;

/// Maximum byte length of a formatted `file://` root URI (`"file://"` + path).
pub const MCP_ROOTS_URI_MAX_LEN: usize = 264;

/// Maximum byte length of an optional human-readable root display name.
pub const MCP_ROOTS_NAME_MAX_LEN: usize = 64;

/// Access policy attached to a registered workspace root.
pub enum McpRootPolicy {
    read_only,
    read_write,
}

/// Outcome of a registry mutation operation (`addRoot`, `upsertRoot`, `removeRoot`, `setPolicy`).
pub enum McpRootOpStatus {
    ok,
    invalid_uri,
    name_too_long,
    already_exists,
    not_found,
    capacity_full,
}

/// Sandbox authorization verdict returned by `authorizePath` / `authorizeUri`.
pub enum McpPathAccessStatus {
    allowed,
    denied_outside_roots,
    denied_read_only,
    denied_invalid_path,
}

/// Inline workspace root entry stored in `McpRootsRegistry`.
pub struct McpRootEntry {
    active: bool = false;
    path_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
    path_len: usize = 0;
    name_buf: [MCP_ROOTS_NAME_MAX_LEN]u8 = [0; MCP_ROOTS_NAME_MAX_LEN];
    name_len: usize = 0;
    policy: McpRootPolicy = McpRootPolicy.read_only;

    pub fn pathSlice(): []const u8 {
        return path_buf[0..path_len];
    }

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }

    pub fn hasName(): bool {
        return name_len > 0;
    }
}

/// Detailed path sandbox authorization decision (includes normalized path copy).
pub struct McpPathAuthDecision {
    status: McpPathAccessStatus = McpPathAccessStatus.denied_outside_roots;
    allowed: bool = false;
    matched_slot: usize = 0;
    has_match: bool = false;
    matched_policy: McpRootPolicy = McpRootPolicy.read_only;
    normalized_len: usize = 0;
    normalized_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];

    pub fn normalizedSlice(): []const u8 {
        return normalized_buf[0..normalized_len];
    }
}

/// Operational telemetry snapshot for `McpRootsRegistry`.
pub struct McpRootsSyncStats {
    active_roots: usize = 0;
    capacity: usize = MCP_ROOTS_MAX_ENTRIES;
    revision: u64 = 0;
    list_changed_pending: bool = false;
    allowed_count: u64 = 0;
    denied_count: u64 = 0;
    write_denials: u64 = 0;
    outside_root_denials: u64 = 0;
    invalid_path_denials: u64 = 0;
}

// ============================================================================
// 2. Internal Byte Helpers & Zero-Heap Path Normalization
// ============================================================================

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hasPrefix(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return bytesEq(s[0..prefix.len], prefix);
}

fn isSafeName(name: []const u8): bool {
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        const c: u8 = name[i];
        if (c == 0x00) return false;
    }
    return true;
}

/// Normalizes an absolute POSIX path in zero heap memory:
///   - Rejects empty strings, relative paths (not starting with `/`), NUL bytes, ASCII control
///     characters (`< 0x20` or `0x7F`), and backslashes (`\`).
///   - Collapses redundant `/` separators, drops `.` segments, resolves `..` segments, and clamps
///     leading `..` traversal at `/`.
///   - Strips trailing `/` except for the root path `"/"`.
/// Returns the byte length written to `out_buf`, or `null` if invalid or `out_buf` is too small.
pub fn normalizeAbsolutePath(raw_path: []const u8, out_buf: *[]u8): usize? {
    if (raw_path.len == 0 || raw_path[0] != 0x2F) return null; // must start with '/'
    if (out_buf.len == 0) return null;

    // Validate bytes up front (reject NUL, control chars, backslash).
    let scan: usize = 0;
    while (scan < raw_path.len) : (scan += 1) {
        const c: u8 = raw_path[scan];
        if (c < 0x20 || c == 0x7F || c == 0x5C) return null;
    }

    let out_len: usize = 0;
    let i: usize = 0;

    while (i < raw_path.len) {
        // Skip consecutive '/'
        while (i < raw_path.len && raw_path[i] == 0x2F) : (i += 1) {}
        if (i >= raw_path.len) break;

        const seg_start: usize = i;
        while (i < raw_path.len && raw_path[i] != 0x2F) : (i += 1) {}
        const seg_len: usize = i - seg_start;

        // "." -> current directory (ignore)
        if (seg_len == 1 && raw_path[seg_start] == 0x2E) {
            continue;
        }

        // ".." -> pop previous segment (clamp at root if already empty)
        if (seg_len == 2 && raw_path[seg_start] == 0x2E && raw_path[seg_start + 1] == 0x2E) {
            if (out_len > 0) {
                while (out_len > 0 && out_buf[out_len - 1] != 0x2F) {
                    out_len -= 1;
                }
                if (out_len > 0 && out_buf[out_len - 1] == 0x2F) {
                    out_len -= 1;
                }
            }
            continue;
        }

        // Append "/" + segment
        if (out_len + 1 + seg_len > out_buf.len) return null;
        out_buf[out_len] = 0x2F;
        out_len += 1;
        let k: usize = 0;
        while (k < seg_len) : (k += 1) {
            out_buf[out_len] = raw_path[seg_start + k];
            out_len += 1;
        }
    }

    if (out_len == 0) {
        out_buf[0] = 0x2F;
        return 1;
    }
    return out_len;
}

/// Parses a `file://` root URI (`file:///abs/path` or `file://localhost/abs/path`) and writes the
/// normalized absolute path into `out_path`. Returns normalized length, or `null` if invalid.
pub fn parseFileRootUri(uri: []const u8, out_path: *[]u8): usize? {
    if (hasPrefix(uri, "file:///")) {
        // Keep the third '/' as the leading slash of the absolute path.
        return normalizeAbsolutePath(uri[7..], out_path);
    }
    if (hasPrefix(uri, "file://localhost/")) {
        // Keep the '/' after "localhost" as the leading slash.
        return normalizeAbsolutePath(uri[16..], out_path);
    }
    return null;
}

/// Formats a normalized absolute path (starting with `/`) as a canonical `file://` URI.
/// Returns total bytes written, or `null` if invalid or `out_uri` is too small.
pub fn formatFileRootUri(norm_path: []const u8, out_uri: *[]u8): usize? {
    if (norm_path.len == 0 || norm_path[0] != 0x2F) return null;
    const prefix = "file://";
    const total: usize = prefix.len + norm_path.len;
    if (total > out_uri.len) return null;

    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        out_uri[i] = prefix[i];
    }
    let j: usize = 0;
    while (j < norm_path.len) : (j += 1) {
        out_uri[prefix.len + j] = norm_path[j];
    }
    return total;
}

/// Checks whether normalized `norm_path` lies within normalized `norm_root` respecting strict
/// directory boundaries (`/app` matches `/app` and `/app/file`, but never `/app2`).
pub fn isPathWithinRoot(norm_root: []const u8, norm_path: []const u8): bool {
    if (norm_root.len == 0 || norm_path.len == 0) return false;
    if (norm_root[0] != 0x2F || norm_path[0] != 0x2F) return false;

    // Root "/" encompasses every normalized absolute path.
    if (norm_root.len == 1) return true;

    if (norm_path.len < norm_root.len) return false;
    if (!bytesEq(norm_path[0..norm_root.len], norm_root)) return false;
    if (norm_path.len == norm_root.len) return true;
    return norm_path[norm_root.len] == 0x2F;
}

// ============================================================================
// 3. Zero-Alloc JSON-RPC 2.0 Encoding Helpers
// ============================================================================

fn appendRaw(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* = pos.* + s.len;
    return true;
}

fn appendJsonEscaped(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (!appendRaw(out, pos, "\"")) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c == 0x22) {
            if (!appendRaw(out, pos, "\\\"")) return false;
        } else if (c == 0x5C) {
            if (!appendRaw(out, pos, "\\\\")) return false;
        } else if (c == 0x0A) {
            if (!appendRaw(out, pos, "\\n")) return false;
        } else if (c == 0x0D) {
            if (!appendRaw(out, pos, "\\r")) return false;
        } else if (c == 0x09) {
            if (!appendRaw(out, pos, "\\t")) return false;
        } else if (c < 0x20) {
            // Escape other ASCII control bytes as \u00XX
            if (pos.* + 6 > out.len) return false;
            const hex = "0123456789abcdef";
            out[pos.*] = 0x5C;
            out[pos.* + 1] = 0x75;
            out[pos.* + 2] = 0x30;
            out[pos.* + 3] = 0x30;
            out[pos.* + 4] = hex[(c >> 4) as usize];
            out[pos.* + 5] = hex[(c & 0x0F) as usize];
            pos.* = pos.* + 6;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* = pos.* + 1;
        }
    }
    return appendRaw(out, pos, "\"");
}

fn appendU64(out: *[]u8, pos: *usize, val_in: u64): bool {
    if (val_in == 0) return appendRaw(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* = pos.* + n;
    return true;
}

/// Encodes a standard MCP JSON-RPC 2.0 `notifications/roots/list_changed` notification into `out_buf`.
/// Returns bytes written, or `null` if `out_buf` is too small.
pub fn encodeRootsListChangedNotification(out_buf: *[]u8): usize? {
    let pos: usize = 0;
    if (!appendRaw(out_buf, *pos, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/roots/list_changed\"}")) {
        return null;
    }
    return pos;
}

// ============================================================================
// 4. Core Registry & Path Boundary Sandbox Guard (`McpRootsRegistry`)
// ============================================================================

/// Zero-heap MCP workspace roots registry with monotonic revision tracking and
/// longest-prefix directory-boundary sandbox enforcement.
pub struct McpRootsRegistry {
    entries: [MCP_ROOTS_MAX_ENTRIES]McpRootEntry = [McpRootEntry{}; MCP_ROOTS_MAX_ENTRIES];
    active_count: usize = 0;
    revision: u64 = 0;
    last_notified_revision: u64 = 0;
    allowed_count: u64 = 0;
    denied_count: u64 = 0;
    write_denials: u64 = 0;
    outside_root_denials: u64 = 0;
    invalid_path_denials: u64 = 0;

    /// Initializes an empty workspace roots registry at revision 0.
    pub fn init(): McpRootsRegistry {
        return McpRootsRegistry{};
    }

    fn normalizeUriOrPath(input: []const u8, out_buf: *[]u8): usize? {
        if (hasPrefix(input, "file://")) {
            return parseFileRootUri(input, out_buf);
        }
        return normalizeAbsolutePath(input, out_buf);
    }

    fn findByNormalizedPath(norm_path: []const u8): usize? {
        let i: usize = 0;
        while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            if (bytesEq(entries[i].pathSlice(), norm_path)) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) return i;
        }
        return null;
    }

    /// Registers a new `file://` workspace root with optional display `name` and access `policy`.
    /// Rejects malformed URIs, names exceeding `MCP_ROOTS_NAME_MAX_LEN`, duplicates, and full table.
    pub fn addRoot(*_, uri: []const u8, name: []const u8, policy: McpRootPolicy): McpRootOpStatus {
        if (name.len > MCP_ROOTS_NAME_MAX_LEN || !isSafeName(name)) {
            return McpRootOpStatus.name_too_long;
        }

        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = parseFileRootUri(uri, *norm_s) ?? return McpRootOpStatus.invalid_uri;
        const norm_path = norm_buf[0..norm_len];

        if (findByNormalizedPath(norm_path) != null) {
            return McpRootOpStatus.already_exists;
        }
        const slot = findFreeSlot() ?? return McpRootOpStatus.capacity_full;

        let entry = McpRootEntry{};
        entry.active = true;
        entry.path_len = norm_len;
        let k: usize = 0;
        while (k < norm_len) : (k += 1) {
            entry.path_buf[k] = norm_path[k];
        }
        entry.name_len = name.len;
        let n: usize = 0;
        while (n < name.len) : (n += 1) {
            entry.name_buf[n] = name[n];
        }
        entry.policy = policy;

        entries[slot] = entry;
        active_count += 1;
        revision += 1;
        return McpRootOpStatus.ok;
    }

    /// Inserts a new `file://` workspace root or updates its `name` and `policy` if already present.
    /// Increments `revision` only when a root is added or its metadata/policy actually changes.
    pub fn upsertRoot(*_, uri: []const u8, name: []const u8, policy: McpRootPolicy): McpRootOpStatus {
        if (name.len > MCP_ROOTS_NAME_MAX_LEN || !isSafeName(name)) {
            return McpRootOpStatus.name_too_long;
        }

        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = parseFileRootUri(uri, *norm_s) ?? return McpRootOpStatus.invalid_uri;
        const norm_path = norm_buf[0..norm_len];

        if (slot := findByNormalizedPath(norm_path)) {
            const changed = !bytesEq(entries[slot].nameSlice(), name) || entries[slot].policy != policy;
            if (changed) {
                entries[slot].name_len = name.len;
                let n: usize = 0;
                while (n < name.len) : (n += 1) {
                    entries[slot].name_buf[n] = name[n];
                }
                entries[slot].policy = policy;
                revision += 1;
            }
            return McpRootOpStatus.ok;
        }

        const free_slot = findFreeSlot() ?? return McpRootOpStatus.capacity_full;
        let entry = McpRootEntry{};
        entry.active = true;
        entry.path_len = norm_len;
        let k: usize = 0;
        while (k < norm_len) : (k += 1) {
            entry.path_buf[k] = norm_path[k];
        }
        entry.name_len = name.len;
        let n: usize = 0;
        while (n < name.len) : (n += 1) {
            entry.name_buf[n] = name[n];
        }
        entry.policy = policy;

        entries[free_slot] = entry;
        active_count += 1;
        revision += 1;
        return McpRootOpStatus.ok;
    }

    /// Removes an existing workspace root identified by `file://` URI or absolute path.
    pub fn removeRoot(*_, uri_or_path: []const u8): McpRootOpStatus {
        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = normalizeUriOrPath(uri_or_path, *norm_s) ?? return McpRootOpStatus.invalid_uri;
        const slot = findByNormalizedPath(norm_buf[0..norm_len]) ?? return McpRootOpStatus.not_found;

        entries[slot] = McpRootEntry{};
        active_count -= 1;
        revision += 1;
        return McpRootOpStatus.ok;
    }

    /// Updates the access policy of an existing workspace root. Increments `revision` if changed.
    pub fn setPolicy(*_, uri_or_path: []const u8, policy: McpRootPolicy): McpRootOpStatus {
        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = normalizeUriOrPath(uri_or_path, *norm_s) ?? return McpRootOpStatus.invalid_uri;
        const slot = findByNormalizedPath(norm_buf[0..norm_len]) ?? return McpRootOpStatus.not_found;

        if (entries[slot].policy != policy) {
            entries[slot].policy = policy;
            revision += 1;
        }
        return McpRootOpStatus.ok;
    }

    /// Clears all active workspace roots. Returns how many roots were removed.
    pub fn clear(*_): usize {
        if (active_count == 0) return 0;
        const removed = active_count;
        let i: usize = 0;
        while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
            entries[i] = McpRootEntry{};
        }
        active_count = 0;
        revision += 1;
        return removed;
    }

    /// Looks up the slot index of an active root by `file://` URI or absolute path.
    pub fn findRoot(uri_or_path: []const u8): usize? {
        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = normalizeUriOrPath(uri_or_path, *norm_s) ?? return null;
        return findByNormalizedPath(norm_buf[0..norm_len]);
    }

    /// Returns a read-only copy of the root at `slot`, or `null` if out of bounds or inactive.
    pub fn getRoot(slot: usize): McpRootEntry? {
        if (slot >= MCP_ROOTS_MAX_ENTRIES || !entries[slot].active) return null;
        return entries[slot];
    }

    /// Returns the number of active workspace roots.
    pub fn count(): usize {
        return active_count;
    }

    /// Returns the monotonic revision number (incremented on every effective root set change).
    pub fn revisionNumber(): u64 {
        return revision;
    }

    /// Returns `true` if `revision` has advanced since the last acknowledged `list_changed` event.
    pub fn hasPendingListChanged(): bool {
        return revision != last_notified_revision;
    }

    /// Marks the current `revision` as notified and returns it.
    pub fn ackListChanged(*_): u64 {
        last_notified_revision = revision;
        return revision;
    }

    fn evaluateNormalizedPath(*_, norm_path: []const u8, need_write: bool): McpPathAuthDecision {
        let decision = McpPathAuthDecision{};
        decision.normalized_len = norm_path.len;
        let k: usize = 0;
        while (k < norm_path.len) : (k += 1) {
            decision.normalized_buf[k] = norm_path[k];
        }

        // Longest-prefix matching across all active workspace roots.
        let found: bool = false;
        let best_slot: usize = 0;
        let best_len: usize = 0;

        let i: usize = 0;
        while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            const root_path = entries[i].pathSlice();
            if (!isPathWithinRoot(root_path, norm_path)) continue;
            if (!found || root_path.len > best_len) {
                found = true;
                best_slot = i;
                best_len = root_path.len;
            }
        }

        if (!found) {
            decision.status = McpPathAccessStatus.denied_outside_roots;
            decision.allowed = false;
            decision.has_match = false;
            denied_count += 1;
            outside_root_denials += 1;
            return decision;
        }

        const matched_policy = entries[best_slot].policy;
        decision.has_match = true;
        decision.matched_slot = best_slot;
        decision.matched_policy = matched_policy;

        if (need_write && matched_policy == McpRootPolicy.read_only) {
            decision.status = McpPathAccessStatus.denied_read_only;
            decision.allowed = false;
            denied_count += 1;
            write_denials += 1;
            return decision;
        }

        decision.status = McpPathAccessStatus.allowed;
        decision.allowed = true;
        allowed_count += 1;
        return decision;
    }

    /// Normalizes `raw_path` and authorizes read/write access against the longest matching root.
    /// Rejects relative paths, NUL/control bytes, paths outside root boundaries, and writes to
    /// `read_only` roots.
    pub fn authorizePath(*_, raw_path: []const u8, need_write: bool): McpPathAuthDecision {
        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        if (norm_len := normalizeAbsolutePath(raw_path, *norm_s)) {
            return evaluateNormalizedPath(norm_buf[0..norm_len], need_write);
        }
        denied_count += 1;
        invalid_path_denials += 1;
        return McpPathAuthDecision{
            status: McpPathAccessStatus.denied_invalid_path,
            allowed: false,
        };
    }

    /// Parses a `file://` URI and authorizes read/write access against the longest matching root.
    pub fn authorizeUri(*_, raw_uri: []const u8, need_write: bool): McpPathAuthDecision {
        let norm_buf: [MCP_ROOTS_PATH_MAX_LEN]u8 = [0; MCP_ROOTS_PATH_MAX_LEN];
        let norm_s: []u8 = norm_buf[0..];
        if (norm_len := parseFileRootUri(raw_uri, *norm_s)) {
            return evaluateNormalizedPath(norm_buf[0..norm_len], need_write);
        }
        denied_count += 1;
        invalid_path_denials += 1;
        return McpPathAuthDecision{
            status: McpPathAccessStatus.denied_invalid_path,
            allowed: false,
        };
    }

    /// Encodes a complete JSON-RPC 2.0 `roots/list` result payload into `out_buf`:
    /// `{"jsonrpc":"2.0","id":<id>,"result":{"roots":[{"uri":"file:///...","name":"..."}]}}`.
    /// Returns bytes written, or `null` if `request_id == 0` or `out_buf` is too small.
    pub fn encodeRootsListResult(request_id: u64, out_buf: *[]u8): usize? {
        if (request_id == 0) return null;
        let pos: usize = 0;
        if (!appendRaw(out_buf, *pos, "{\"jsonrpc\":\"2.0\",\"id\":")) return null;
        if (!appendU64(out_buf, *pos, request_id)) return null;
        if (!appendRaw(out_buf, *pos, ",\"result\":{\"roots\":[")) return null;

        let written: usize = 0;
        let uri_buf: [MCP_ROOTS_URI_MAX_LEN]u8 = [0; MCP_ROOTS_URI_MAX_LEN];
        let uri_s: []u8 = uri_buf[0..];

        let i: usize = 0;
        while (i < MCP_ROOTS_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            if (written > 0) {
                if (!appendRaw(out_buf, *pos, ",")) return null;
            }
            const uri_len = formatFileRootUri(entries[i].pathSlice(), *uri_s) ?? return null;
            if (!appendRaw(out_buf, *pos, "{\"uri\":")) return null;
            if (!appendJsonEscaped(out_buf, *pos, uri_buf[0..uri_len])) return null;
            if (entries[i].hasName()) {
                if (!appendRaw(out_buf, *pos, ",\"name\":")) return null;
                if (!appendJsonEscaped(out_buf, *pos, entries[i].nameSlice())) return null;
            }
            if (!appendRaw(out_buf, *pos, "}")) return null;
            written += 1;
        }

        if (!appendRaw(out_buf, *pos, "]}}")) return null;
        return pos;
    }

    /// If `revision` has advanced since the last notification, encodes
    /// `notifications/roots/list_changed` into `out_buf` and acknowledges the revision.
    /// Returns `null` if no change is pending or `out_buf` is too small.
    pub fn encodePendingListChanged(*_, out_buf: *[]u8): usize? {
        if (!hasPendingListChanged()) return null;
        const n = encodeRootsListChangedNotification(out_buf) ?? return null;
        last_notified_revision = revision;
        return n;
    }

    /// Returns a point-in-time operational telemetry snapshot.
    pub fn stats(): McpRootsSyncStats {
        return McpRootsSyncStats{
            active_roots: active_count,
            capacity: MCP_ROOTS_MAX_ENTRIES,
            revision: revision,
            list_changed_pending: hasPendingListChanged(),
            allowed_count: allowed_count,
            denied_count: denied_count,
            write_denials: write_denials,
            outside_root_denials: outside_root_denials,
            invalid_path_denials: invalid_path_denials,
        };
    }
}
