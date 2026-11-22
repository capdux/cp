// Origin: Pure-CP Stdlib (AI Agent Sandbox Policy: Workspace Jail, Command Allowlist & Env Scrub) | Created: 2026-11-22
// Pure CP zero-allocation AI Agent sandbox policy arbiter providing:
// - Lexical path normalization (resolving `.`, `..`, `//`) and segment-boundary root confinement
//   preventing prefix confusion (`/data/app` vs `/data/app2`), parent traversal, and NUL bytes
// - Priority denied-path prefix/segment blocking (e.g. `.env`, `.git/config`) and read-only enforcement
// - Command allowlist/denylist verification and shell metacharacter filtering
// - Subprocess environment variable allowlisting and sensitive key scrubbing

/// Requested filesystem access mode for path sandbox evaluation.
pub enum PathAccessMode {
    Read,
    Write,
    Delete,
}

/// Policy decision returned by path and command sandbox checks.
pub enum SandboxDecision {
    Allow,
    DenyOutsideRoot,
    DenyReadOnly,
    DenyNullByte,
    DenyDeniedPath,
    DenyCommandNotAllowed,
    DenyShellMeta,
}

/// Allowed workspace root rule with granular write/delete permissions.
pub struct PathRule {
    root: string = "",
    allow_write: bool = false,
    allow_delete: bool = false,
}

/// Command execution allowlist/denylist and shell metacharacter policy.
pub struct CommandPolicy {
    allowed_binaries: []const string,
    denied_binaries: []const string,
    forbid_shell_metachars: bool = true,
}

// ============================================================================
// Internal Path, String & Character Helpers
// ============================================================================

fn strEquals(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn containsNullByte(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 0) {
            return true;
        }
    }
    return false;
}

fn isShellMetachar(b: u8): bool {
    return b == ';' || b == '|' || b == '&' || b == '$' || b == '`' ||
        b == '>' || b == '<' || b == '\n' || b == '\r' || b == 0;
}

fn containsShellMeta(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (isShellMetachar(s[i])) {
            return true;
        }
    }
    return false;
}

fn trimTrailingSlashes(s: string): string {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

/// Lexically normalizes `raw` (resolving `//`, `.`, and `..`) into `out_buf`.
/// Returns normalized byte length, or `null` if `raw` is empty or `out_buf` is too small.
fn normalizeLexicalPath(raw: string, out_buf: *[]u8): usize? {
    if (raw.len == 0 || out_buf.len == 0) {
        return null;
    }

    const is_abs = raw[0] == '/';
    let out_len: usize = 0;
    let rel_depth: usize = 0;

    if (is_abs) {
        out_buf[0] = '/';
        out_len = 1;
    }

    let pos: usize = 0;
    while (pos < raw.len) {
        while (pos < raw.len && raw[pos] == '/') {
            pos += 1;
        }
        if (pos >= raw.len) {
            break;
        }
        const seg_start = pos;
        while (pos < raw.len && raw[pos] != '/') {
            pos += 1;
        }
        const seg = raw[seg_start..pos];

        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }

        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (is_abs) {
                if (out_len > 1) {
                    let back = out_len - 1;
                    while (back > 0 && out_buf[back] != '/') {
                        back -= 1;
                    }
                    out_len = if (back == 0) 1 else back;
                }
            } else {
                if (rel_depth > 0) {
                    let back = out_len;
                    while (back > 0 && out_buf[back - 1] != '/') {
                        back -= 1;
                    }
                    if (back > 0) {
                        out_len = back - 1;
                    } else {
                        out_len = 0;
                    }
                    rel_depth -= 1;
                } else {
                    // Unresolved leading `..` on relative path: preserve so root checks reject it
                    if (out_len > 0) {
                        if (out_len + 1 > out_buf.len) return null;
                        out_buf[out_len] = '/';
                        out_len += 1;
                    }
                    if (out_len + 2 > out_buf.len) return null;
                    out_buf[out_len] = '.';
                    out_buf[out_len + 1] = '.';
                    out_len += 2;
                }
            }
            continue;
        }

        // Append normal segment
        if (is_abs) {
            if (out_len > 1) {
                if (out_len + 1 > out_buf.len) return null;
                out_buf[out_len] = '/';
                out_len += 1;
            }
        } else {
            if (out_len > 0) {
                if (out_len + 1 > out_buf.len) return null;
                out_buf[out_len] = '/';
                out_len += 1;
            }
            rel_depth += 1;
        }

        if (out_len + seg.len > out_buf.len) {
            return null;
        }
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            out_buf[out_len + k] = seg[k];
        }
        out_len += seg.len;
    }

    if (!is_abs && out_len == 0) {
        out_buf[0] = '.';
        return 1;
    }
    return out_len;
}

/// Checks if `norm_path` is strictly equal to or inside `root_raw` on a path segment boundary.
/// Prevents `/data/app` from matching `/data/app2`.
fn pathInsideRoot(norm_path: string, root_raw: string): bool {
    if (root_raw.len == 0 || norm_path.len == 0) {
        return false;
    }
    const root = trimTrailingSlashes(root_raw);
    if (root.len == 1 && root[0] == '/') {
        return norm_path[0] == '/';
    }
    if (norm_path.len < root.len) {
        return false;
    }
    let i: usize = 0;
    while (i < root.len) : (i += 1) {
        if (norm_path[i] != root[i]) {
            return false;
        }
    }
    if (norm_path.len == root.len) {
        return true;
    }
    return norm_path[root.len] == '/';
}

/// Checks if `norm_path` matches a denied prefix (absolute path prefix) or relative
/// segment sequence (e.g. `.env`, `.git/config`) strictly on segment boundaries.
fn pathMatchesDenied(norm_path: string, denied_raw: string): bool {
    if (denied_raw.len == 0 || norm_path.len == 0) {
        return false;
    }
    const denied = trimTrailingSlashes(denied_raw);
    if (denied[0] == '/') {
        return pathInsideRoot(norm_path, denied);
    }

    // Relative denied segment(s): match at any segment start boundary in `norm_path`
    if (norm_path.len < denied.len) {
        return false;
    }
    const limit = norm_path.len - denied.len;
    let pos: usize = 0;
    while (pos <= limit) : (pos += 1) {
        const at_seg_start = pos == 0 || norm_path[pos - 1] == '/';
        if (!at_seg_start) {
            continue;
        }
        let ok: bool = true;
        let j: usize = 0;
        while (j < denied.len) : (j += 1) {
            if (norm_path[pos + j] != denied[j]) {
                ok = false;
                break;
            }
        }
        if (ok) {
            const after = pos + denied.len;
            if (after == norm_path.len || norm_path[after] == '/') {
                return true;
            }
        }
    }
    return false;
}

fn pathBasename(path: string): string {
    let i: usize = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/') {
            return path[i + 1..];
        }
    }
    return path;
}

fn matchesBinaryList(binary: string, list: []const string): bool {
    const base = pathBasename(binary);
    let i: usize = 0;
    while (i < list.len) : (i += 1) {
        if (strEquals(binary, list[i]) || strEquals(base, list[i])) {
            return true;
        }
    }
    return false;
}

fn toUpperAscii(b: u8): u8 {
    if (b >= 'a' && b <= 'z') {
        return b - 32;
    }
    return b;
}

fn endsWithIgnoreCase(s: string, suffix_upper: string): bool {
    if (s.len < suffix_upper.len) {
        return false;
    }
    const off = s.len - suffix_upper.len;
    let i: usize = 0;
    while (i < suffix_upper.len) : (i += 1) {
        if (toUpperAscii(s[off + i]) != suffix_upper[i]) {
            return false;
        }
    }
    return true;
}

fn equalsIgnoreCase(s: string, upper_lit: string): bool {
    if (s.len != upper_lit.len) {
        return false;
    }
    let i: usize = 0;
    while (i < upper_lit.len) : (i += 1) {
        if (toUpperAscii(s[i]) != upper_lit[i]) {
            return false;
        }
    }
    return true;
}

fn containsIgnoreCase(s: string, needle_upper: string): bool {
    if (needle_upper.len == 0 || s.len < needle_upper.len) {
        return false;
    }
    const limit = s.len - needle_upper.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle_upper.len) : (j += 1) {
            if (toUpperAscii(s[i + j]) != needle_upper[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

// ============================================================================
// Public Sandbox Policy Arbitration APIs
// ============================================================================

/// Evaluates whether `target_path` is permitted under `mode` given `allowed_roots`
/// and `denied_prefixes`. Lexically normalizes `target_path` into `norm_scratch` first.
/// Precedence:
/// 1. NUL byte check -> `DenyNullByte`
/// 2. Empty path / normalization overflow -> `DenyOutsideRoot`
/// 3. `denied_prefixes` (segment-boundary match) -> `DenyDeniedPath`
/// 4. Longest-matching `allowed_roots` entry -> `DenyOutsideRoot` / `DenyReadOnly` / `Allow`
pub fn checkPathSandbox(
    target_path: string,
    mode: PathAccessMode,
    allowed_roots: []const PathRule,
    denied_prefixes: []const string,
    norm_scratch: *[]u8,
): SandboxDecision {
    if (containsNullByte(target_path)) {
        return SandboxDecision.DenyNullByte;
    }

    let norm_len: usize = 0;
    if (n := normalizeLexicalPath(target_path, norm_scratch)) {
        norm_len = n;
    } else {
        return SandboxDecision.DenyOutsideRoot;
    }

    const norm_path: string = norm_scratch[0..norm_len];

    // Denied prefixes take strict precedence
    let d: usize = 0;
    while (d < denied_prefixes.len) : (d += 1) {
        if (pathMatchesDenied(norm_path, denied_prefixes[d])) {
            return SandboxDecision.DenyDeniedPath;
        }
    }

    // Select the most specific (longest) matching allowed root
    let best_idx: usize = 0;
    let best_len: usize = 0;
    let matched: bool = false;

    let r: usize = 0;
    while (r < allowed_roots.len) : (r += 1) {
        const root = allowed_roots[r].root;
        if (pathInsideRoot(norm_path, root)) {
            const eff_len = trimTrailingSlashes(root).len;
            if (!matched || eff_len > best_len) {
                matched = true;
                best_len = eff_len;
                best_idx = r;
            }
        }
    }

    if (!matched) {
        return SandboxDecision.DenyOutsideRoot;
    }

    const rule = allowed_roots[best_idx];
    if (mode == PathAccessMode.Write && !rule.allow_write) {
        return SandboxDecision.DenyReadOnly;
    }
    if (mode == PathAccessMode.Delete && !rule.allow_delete) {
        return SandboxDecision.DenyReadOnly;
    }

    return SandboxDecision.Allow;
}

/// Evaluates whether executing `binary` with `args` is permitted under `policy`.
/// Checks NUL bytes first, then `denied_binaries`, then `allowed_binaries` (when non-empty),
/// and finally shell metacharacters (when `policy.forbid_shell_metachars == true`).
pub fn checkCommandSandbox(
    binary: string,
    args: []const string,
    policy: CommandPolicy,
): SandboxDecision {
    if (containsNullByte(binary)) {
        return SandboxDecision.DenyNullByte;
    }
    let i: usize = 0;
    while (i < args.len) : (i += 1) {
        if (containsNullByte(args[i])) {
            return SandboxDecision.DenyNullByte;
        }
    }

    if (binary.len == 0) {
        return SandboxDecision.DenyCommandNotAllowed;
    }

    if (matchesBinaryList(binary, policy.denied_binaries)) {
        return SandboxDecision.DenyCommandNotAllowed;
    }

    if (policy.allowed_binaries.len > 0 && !matchesBinaryList(binary, policy.allowed_binaries)) {
        return SandboxDecision.DenyCommandNotAllowed;
    }

    if (policy.forbid_shell_metachars) {
        if (containsShellMeta(binary)) {
            return SandboxDecision.DenyShellMeta;
        }
        let a: usize = 0;
        while (a < args.len) : (a += 1) {
            if (containsShellMeta(args[a])) {
                return SandboxDecision.DenyShellMeta;
            }
        }
    }

    return SandboxDecision.Allow;
}

/// Returns `true` if `key` matches common sensitive credential/secret environment variable
/// naming patterns (case-insensitive suffixes like `_KEY`, `_SECRET`, `_TOKEN`, `_PASSWORD`,
/// `_PASSWD`, `_CREDENTIAL`, `_AUTH`, or exact/embedded secret identifiers).
pub fn isSensitiveEnvKey(key: string): bool {
    if (key.len == 0) {
        return false;
    }
    if (equalsIgnoreCase(key, "SECRET") ||
        equalsIgnoreCase(key, "TOKEN") ||
        equalsIgnoreCase(key, "PASSWORD") ||
        equalsIgnoreCase(key, "PASSWD") ||
        equalsIgnoreCase(key, "APIKEY")) {
        return true;
    }
    if (endsWithIgnoreCase(key, "_KEY") ||
        endsWithIgnoreCase(key, "_SECRET") ||
        endsWithIgnoreCase(key, "_TOKEN") ||
        endsWithIgnoreCase(key, "_PASSWORD") ||
        endsWithIgnoreCase(key, "_PASSWD") ||
        endsWithIgnoreCase(key, "_AUTH") ||
        endsWithIgnoreCase(key, "_CREDENTIAL") ||
        endsWithIgnoreCase(key, "_CREDENTIALS")) {
        return true;
    }
    if (containsIgnoreCase(key, "SECRET") ||
        containsIgnoreCase(key, "PASSWORD") ||
        containsIgnoreCase(key, "API_KEY") ||
        containsIgnoreCase(key, "ACCESS_KEY") ||
        containsIgnoreCase(key, "PRIVATE_KEY") ||
        containsIgnoreCase(key, "AUTH_TOKEN")) {
        return true;
    }
    return false;
}

/// Filters environment variable `keys`, writing the indices of permitted entries into `out_indices`.
/// When `strip_sensitive` is `true`, keys matching `isSensitiveEnvKey` are always excluded.
/// When `allow_list` is non-empty, only keys present in `allow_list` are retained.
/// Returns the number of indices written into `out_indices`.
pub fn filterEnvIndices(
    keys: []const string,
    allow_list: []const string,
    strip_sensitive: bool,
    out_indices: *[]usize,
): usize {
    let out_count: usize = 0;
    let i: usize = 0;
    while (i < keys.len && out_count < out_indices.len) : (i += 1) {
        const k = keys[i];
        if (k.len == 0) {
            continue;
        }
        if (strip_sensitive && isSensitiveEnvKey(k)) {
            continue;
        }
        if (allow_list.len > 0) {
            let allowed: bool = false;
            let j: usize = 0;
            while (j < allow_list.len) : (j += 1) {
                if (strEquals(k, allow_list[j])) {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) {
                continue;
            }
        }
        out_indices[out_count] = i;
        out_count += 1;
    }
    return out_count;
}
