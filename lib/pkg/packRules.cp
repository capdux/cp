// Source: Absorbed from /data/dev/bun/src/cli/pack_command.cp | Synced: 2026-11-22T12:34:46Z
// ============================================================================
// lib/pkg/packRules.cp — Production-Grade `cap pack` / `cap publish` File
// Inclusion/Exclusion Rule Engine, Glob Matcher, Path Hygiene & Tarball
// Manifest Planner
// ============================================================================
// Absorbed from Bun `src/cli/pack_command.cp`. Designed with strict
// external-slice-driven zero-allocation architecture (no hardcoded small file
// or rule limits, zero ReDoS glob matching, full path-traversal protection,
// deterministic tarball file ordering & manifest digest).
//
// Rule precedence (aligned with Bun `pack_command.cp` & npm pack):
//   1. Hard never-overridable excludes: `.git`, `.hg`, `.npmrc`, `node_modules`
//      (unless `is_bundled_dep`), `cap_modules`, `.cap`
//   2. Always-included root metadata: `package.json`, `cap.json`, `README*`,
//      `LICENSE*`/`LICENCE*`, `CHANGELOG*`/`CHANGES*`/`HISTORY*`
//   3. Declared package entrypoints (`main`, `bin` targets)
//   4. `files` whitelist (when present, only matching files + `!negation` rules)
//   5. Overridable default ignores (lockfiles, `.DS_Store`, `._*`, `.*.swp`,
//      `.gitignore`, `.npmignore`, `.svn`, `CVS`, `.env.production`, `bunfig.toml`)
//   6. Directory-scoped `.npmignore` (takes precedence over `.gitignore` in the
//      same directory) and `.gitignore` layered from root to deepest ancestor.

pub const PACK_TARBALL_PREFIX: []const u8 = "package/";
pub const PACK_MAX_PATH_LEN: usize = 4096;

pub err PackRulesError {
    EmptyPath,
    UnsafePath,
    InvalidPackageName,
    RuleBufferFull,
    EntrypointBufferFull,
    PlanBufferFull,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc String, ASCII & Hashing Helpers
// ============================================================================

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return strEqlIgnoreCase(s[0..prefix.len], prefix);
}

fn strCmp(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn fnv1a64Update(h_in: u64, data: []const u8): u64 {
    let h = h_in;
    let i: usize = 0;
    while (i < data.len) {
        h ^= @as(u64, data[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn fnv1a64UpdateU64(h_in: u64, val: u64): u64 {
    let h = h_in;
    let i: u6 = 0;
    while (i < 8) {
        const shift: u6 = i * 8;
        h ^= (val >> shift) & 0xFF;
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn appendByte(out: []u8, pos: usize, ch: u8): PackRulesError!usize {
    if (pos >= out.len) return PackRulesError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): PackRulesError!usize {
    if (src.len > out.len - pos) return PackRulesError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeU64(out: []u8, pos: usize, val_in: u64): PackRulesError!usize {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return PackRulesError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

fn writeHex64(out: []u8, pos: usize, val: u64): PackRulesError!usize {
    if (16 > out.len - pos) return PackRulesError.BufferTooSmall;
    const hex_chars = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = @as(u6, @intCast((15 - i) * 4));
        const nibble: usize = @as(usize, @truncate((val >> shift) & 0xF));
        out[pos + i] = hex_chars[nibble];
        i += 1;
    }
    return pos + 16;
}

// ============================================================================
// Phase 1: Path Normalization, Traversal Guards & Tarball Naming
// ============================================================================

/// Normalizes a package-relative path into clean POSIX form (`src/index.cp`),
/// stripping leading `./` and collapsing duplicate `/`.
/// Rejects empty paths, absolute paths, backslashes, control chars, null bytes,
/// and `..` parent-directory traversal segments.
pub fn normalizePackRelPath(raw: []const u8, out: []u8): PackRulesError!usize {
    if (raw.len == 0) return PackRulesError.EmptyPath;
    if (raw.len > PACK_MAX_PATH_LEN) return PackRulesError.UnsafePath;
    if (raw[0] == '/' || raw[0] == '\\') return PackRulesError.UnsafePath;

    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        while (i < raw.len && raw[i] == '/') i += 1;
        if (i >= raw.len) break;

        const seg_start = i;
        while (i < raw.len && raw[i] != '/') {
            const c = raw[i];
            if (c == 0 || c == '\\' || c < 0x20) return PackRulesError.UnsafePath;
            i += 1;
        }
        const seg = raw[seg_start..i];
        if (strEql(seg, "..")) return PackRulesError.UnsafePath;
        if (strEql(seg, ".")) continue;

        if (pos > 0) {
            pos = appendByte(out, pos, '/')?;
        }
        pos = appendSlice(out, pos, seg)?;
    }

    if (pos == 0) return PackRulesError.EmptyPath;
    return pos;
}

/// Formats an archive member path inside the npm/cap tarball: `package/<rel_path>`.
pub fn formatTarballEntryPath(rel_path: []const u8, out: []u8): PackRulesError!usize {
    let pos = appendSlice(out, 0, PACK_TARBALL_PREFIX)?;
    const norm_len = normalizePackRelPath(rel_path, out[pos..])?;
    return pos + norm_len;
}

/// Formats canonical npm/Bun tarball filename into `out`:
/// - Unscoped: `<name>-<version>.tgz` (e.g. `lodash-4.17.21.tgz`)
/// - Scoped: `<scope>-<name>-<version>.tgz` (e.g. `@scope/ui` + `1.2.3` -> `scope-ui-1.2.3.tgz`)
pub fn formatTarballFileName(pkg_name: []const u8, version: []const u8, out: []u8): PackRulesError!usize {
    if (pkg_name.len == 0 || version.len == 0) return PackRulesError.InvalidPackageName;
    let pos: usize = 0;
    let i: usize = 0;
    if (pkg_name[0] == '@') {
        i = 1;
        if (i >= pkg_name.len) return PackRulesError.InvalidPackageName;
    }
    let saw_char = false;
    while (i < pkg_name.len) {
        const c = pkg_name[i];
        if (c == 0 || c == '\\' || c < 0x20) return PackRulesError.InvalidPackageName;
        if (c == '.' && i + 1 < pkg_name.len && pkg_name[i + 1] == '.') {
            return PackRulesError.InvalidPackageName;
        }
        pos = appendByte(out, pos, if (c == '/') '-' else c)?;
        saw_char = true;
        i += 1;
    }
    if (!saw_char) return PackRulesError.InvalidPackageName;

    pos = appendByte(out, pos, '-')?;
    i = 0;
    while (i < version.len) {
        const c = version[i];
        if (c == 0 || c == '/' || c == '\\' || c < 0x20) return PackRulesError.InvalidPackageName;
        pos = appendByte(out, pos, c)?;
        i += 1;
    }
    return appendSlice(out, pos, ".tgz");
}

fn pathBasename(rel_path: []const u8): []const u8 {
    let i = rel_path.len;
    while (i > 0) {
        i -= 1;
        if (rel_path[i] == '/') return rel_path[(i + 1)..];
    }
    return rel_path;
}

fn pathHasSlash(rel_path: []const u8): bool {
    let i: usize = 0;
    while (i < rel_path.len) {
        if (rel_path[i] == '/') return true;
        i += 1;
    }
    return false;
}

// ============================================================================
// Phase 2: Zero-Alloc Bounded Glob Matcher (Zero ReDoS Risk)
// ============================================================================

/// Matches a single path segment (no `/`) against a glob pattern containing
/// `*`, `?`, and `[abc]`/`[a-z]` character classes in O(pat * text) time.
fn matchSegmentGlob(pat: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = pat.len + 1;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pat.len && pat[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (pi < pat.len && pat[pi] == '?') {
            pi += 1;
            ti += 1;
        } else if (pi < pat.len && pat[pi] == '[') {
            // Character class `[...]`
            let ci = pi + 1;
            let negate = false;
            if (ci < pat.len && (pat[ci] == '!' || pat[ci] == '^')) {
                negate = true;
                ci += 1;
            }
            let matched = false;
            let closed = false;
            const ch = text[ti];
            while (ci < pat.len) {
                if (pat[ci] == ']') {
                    closed = true;
                    ci += 1;
                    break;
                }
                if (ci + 2 < pat.len && pat[ci + 1] == '-' && pat[ci + 2] != ']') {
                    if (ch >= pat[ci] && ch <= pat[ci + 2]) matched = true;
                    ci += 3;
                } else {
                    if (ch == pat[ci]) matched = true;
                    ci += 1;
                }
            }
            if (!closed) {
                // Treat unclosed '[' literally
                if (text[ti] == '[') {
                    pi += 1;
                    ti += 1;
                } else if (star_pi <= pat.len) {
                    pi = star_pi + 1;
                    star_ti += 1;
                    ti = star_ti;
                } else {
                    return false;
                }
            } else {
                const ok = if (negate) !matched else matched;
                if (ok) {
                    pi = ci;
                    ti += 1;
                } else if (star_pi <= pat.len) {
                    pi = star_pi + 1;
                    star_ti += 1;
                    ti = star_ti;
                } else {
                    return false;
                }
            }
        } else if (pi < pat.len && pat[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (star_pi <= pat.len) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < pat.len && pat[pi] == '*') pi += 1;
    return pi == pat.len;
}

fn nextSegEnd(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len && s[i] != '/') i += 1;
    return i;
}

/// Full path glob matcher supporting `*`, `?`, `[a-z]`, and `**` (zero or more
/// path segments) using bounded backtracking over path segments (zero allocation).
pub fn matchPackGlob(pattern: []const u8, path: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let gs_pi: usize = pattern.len + 1;
    let gs_ti: usize = 0;

    while (ti < path.len) {
        if (pi < pattern.len) {
            const p_end = nextSegEnd(pattern, pi);
            const p_seg = pattern[pi..p_end];
            const p_next = if (p_end < pattern.len) (p_end + 1) else p_end;

            if (strEql(p_seg, "**")) {
                gs_pi = p_next;
                gs_ti = ti;
                pi = p_next;
                continue;
            }

            const t_end = nextSegEnd(path, ti);
            const t_seg = path[ti..t_end];
            const t_next = if (t_end < path.len) (t_end + 1) else t_end;

            if (matchSegmentGlob(p_seg, t_seg)) {
                pi = p_next;
                ti = t_next;
                continue;
            }
        }

        if (gs_pi <= pattern.len) {
            const skip_end = nextSegEnd(path, gs_ti);
            gs_ti = if (skip_end < path.len) (skip_end + 1) else skip_end;
            pi = gs_pi;
            ti = gs_ti;
        } else {
            return false;
        }
    }

    // Consume trailing `**` segments in pattern
    while (pi < pattern.len) {
        const p_end = nextSegEnd(pattern, pi);
        const p_seg = pattern[pi..p_end];
        if (!strEql(p_seg, "**")) return false;
        pi = if (p_end < pattern.len) (p_end + 1) else p_end;
    }
    return true;
}

// ============================================================================
// Phase 3: Always-Included, Hard-Excluded & Default-Ignored Classification
// ============================================================================

fn isMetaStemMatch(basename: []const u8, stem: []const u8): bool {
    if (strEqlIgnoreCase(basename, stem)) return true;
    if (basename.len > stem.len && startsWithIgnoreCase(basename, stem) && basename[stem.len] == '.') {
        return true;
    }
    return false;
}

/// True if `rel_path` is a root metadata file that npm/Bun `pack` ALWAYS includes
/// (`package.json`, `cap.json`, `README*`, `LICENSE*`/`LICENCE*`, `CHANGELOG*`/`CHANGES*`/`HISTORY*`).
pub fn isAlwaysIncludedPackFile(rel_path: []const u8): bool {
    if (pathHasSlash(rel_path)) return false;
    if (strEql(rel_path, "package.json") || strEql(rel_path, "cap.json")) return true;
    if (isMetaStemMatch(rel_path, "README")) return true;
    if (isMetaStemMatch(rel_path, "LICENSE") || isMetaStemMatch(rel_path, "LICENCE")) return true;
    if (isMetaStemMatch(rel_path, "CHANGELOG")
        || isMetaStemMatch(rel_path, "CHANGES")
        || isMetaStemMatch(rel_path, "HISTORY"))
    {
        return true;
    }
    return false;
}

/// True if `rel_path` is a README file at package root.
pub fn isRootReadmeFile(rel_path: []const u8): bool {
    return !pathHasSlash(rel_path) && isMetaStemMatch(rel_path, "README");
}

/// True if `rel_path` is a LICENSE/LICENCE file at package root.
pub fn isRootLicenseFile(rel_path: []const u8): bool {
    if (pathHasSlash(rel_path)) return false;
    return isMetaStemMatch(rel_path, "LICENSE") || isMetaStemMatch(rel_path, "LICENCE");
}

/// True if `rel_path` contains a never-overridable hard-excluded segment
/// (`.git`, `.hg`, `.npmrc`, `node_modules`, `cap_modules`, `.cap`).
/// When `is_bundled_dep == true`, `node_modules` / `cap_modules` are permitted.
pub fn isHardExcludedPackPath(rel_path: []const u8, is_bundled_dep: bool): bool {
    let i: usize = 0;
    while (i < rel_path.len) {
        const end = nextSegEnd(rel_path, i);
        const seg = rel_path[i..end];
        if (strEql(seg, ".git") || strEql(seg, ".hg") || strEql(seg, ".npmrc") || strEql(seg, ".cap")) {
            return true;
        }
        if (!is_bundled_dep && (strEql(seg, "node_modules") || strEql(seg, "cap_modules"))) {
            return true;
        }
        i = if (end < rel_path.len) (end + 1) else end;
    }
    return false;
}

/// True if `rel_path` matches Bun/npm overridable default ignore patterns
/// (lockfiles at root, `.DS_Store`, `._*`, `.*.swp`, `.gitignore`, `.npmignore`,
/// `.lock-wscript`, `.svn`, `.wafpickle-*`, `CVS`, `npm-debug.log`,
/// `.env.production`, `bunfig.toml`).
pub fn isDefaultIgnoredPackPath(rel_path: []const u8): bool {
    // Root-only lockfiles
    if (!pathHasSlash(rel_path)) {
        if (strEql(rel_path, "package-lock.json")
            || strEql(rel_path, "yarn.lock")
            || strEql(rel_path, "pnpm-lock.yaml")
            || strEql(rel_path, "bun.lockb")
            || strEql(rel_path, "bun.lock")
            || strEql(rel_path, "cap.lock"))
        {
            return true;
        }
    }

    // Any-depth segment checks
    let i: usize = 0;
    while (i < rel_path.len) {
        const end = nextSegEnd(rel_path, i);
        const seg = rel_path[i..end];
        if (strEql(seg, ".DS_Store")
            || strEql(seg, ".gitignore")
            || strEql(seg, ".npmignore")
            || strEql(seg, ".lock-wscript")
            || strEql(seg, ".svn")
            || strEql(seg, "CVS")
            || strEql(seg, "npm-debug.log")
            || strEql(seg, ".env.production")
            || strEql(seg, "bunfig.toml"))
        {
            return true;
        }
        if (matchSegmentGlob("._*", seg)
            || matchSegmentGlob(".*.swp", seg)
            || matchSegmentGlob(".wafpickle-*", seg))
        {
            return true;
        }
        i = if (end < rel_path.len) (end + 1) else end;
    }
    return false;
}

// ============================================================================
// Phase 4: Ignore File & `files` Field Rule Parser
// ============================================================================

/// Source kind of a pack inclusion/exclusion rule.
pub enum PackRuleKind: u8 {
    npm_ignore = 0,
    git_ignore = 1,
    files_field = 2,
}

/// Compiled rule entry from `.npmignore`, `.gitignore`, or `package.json` `"files"`.
pub struct PackRulePattern {
    pattern: []const u8 = "",
    base_dir: []const u8 = "",
    depth: u16 = 0,
    kind: PackRuleKind = PackRuleKind.npm_ignore,
    negated: bool = false,
    dirs_only: bool = false,
    anchored: bool = false,

    static fn empty(): PackRulePattern {
        return PackRulePattern{};
    }

    /// Evaluates whether this rule matches candidate `(rel_path, is_dir)`.
    fn matches(rel_path: []const u8, is_dir: bool): bool {
        // Check directory scope (`base_dir`)
        let sub_path = rel_path;
        if (_.base_dir.len > 0) {
            if (rel_path.len <= _.base_dir.len) return false;
            if (!strEql(rel_path[0.._.base_dir.len], _.base_dir)) return false;
            if (rel_path[_.base_dir.len] != '/') return false;
            sub_path = rel_path[(_.base_dir.len + 1)..];
        }

        if (_.kind == PackRuleKind.files_field) {
            // In `"files"`, a directory entry like `dist` matches `dist` AND `dist/...`
            if (matchPackGlob(_.pattern, sub_path)) return true;
            if (sub_path.len > _.pattern.len
                && strEql(sub_path[0.._.pattern.len], _.pattern)
                && sub_path[_.pattern.len] == '/')
            {
                return true;
            }
            if (!_.anchored && !pathHasSlash(_.pattern)) {
                if (matchSegmentGlob(_.pattern, pathBasename(sub_path))) return true;
            }
            return false;
        }

        // Ignore rule semantics (`.npmignore` / `.gitignore`)
        if (_.dirs_only) {
            // Matches a directory itself OR any file underneath a matching directory prefix
            if (is_dir && _.matchesPathInner(sub_path)) return true;
            let i: usize = 0;
            while (i < sub_path.len) {
                if (sub_path[i] == '/') {
                    if (_.matchesPathInner(sub_path[0..i])) return true;
                }
                i += 1;
            }
            return false;
        }

        // Non-dirs-only rule also excludes everything inside a matched directory prefix
        if (_.matchesPathInner(sub_path)) return true;
        let i: usize = 0;
        while (i < sub_path.len) {
            if (sub_path[i] == '/') {
                if (_.matchesPathInner(sub_path[0..i])) return true;
            }
            i += 1;
        }
        return false;
    }

    fn matchesPathInner(sub_path: []const u8): bool {
        if (_.anchored) {
            return matchPackGlob(_.pattern, sub_path);
        }
        if (matchPackGlob(_.pattern, sub_path)) return true;
        return matchSegmentGlob(_.pattern, pathBasename(sub_path));
    }
}

fn trimLineCr(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == ' ' || line[end - 1] == '\t')) {
        if (end >= 2 && line[end - 2] == '\\') break;
        end -= 1;
    }
    return line[0..end];
}

/// Parses `.npmignore` or `.gitignore` contents into caller-provided `out_rules`.
/// Returns the number of rules parsed.
pub fn parsePackIgnoreFile(
    contents: []const u8,
    base_dir: []const u8,
    depth: u16,
    kind: PackRuleKind,
    out_rules: []PackRulePattern,
): PackRulesError!usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < contents.len) {
        const line_start = i;
        while (i < contents.len && contents[i] != '\n') i += 1;
        let line = trimLineCr(contents[line_start..i]);
        if (i < contents.len && contents[i] == '\n') i += 1;

        if (kind == PackRuleKind.npm_ignore) {
            // npm trims leading whitespace; git preserves it
            let s: usize = 0;
            while (s < line.len && (line[s] == ' ' || line[s] == '\t')) s += 1;
            line = line[s..];
        }
        if (line.len == 0 || line[0] == '#') continue;

        let negated = false;
        if (line[0] == '!') {
            negated = true;
            line = line[1..];
        } else if (line.len >= 2 && line[0] == '\\' && (line[1] == '#' || line[1] == '!')) {
            line = line[1..];
        }
        if (line.len == 0) continue;

        let dirs_only = false;
        if (line[line.len - 1] == '/') {
            dirs_only = true;
            line = line[0..(line.len - 1)];
        }
        if (line.len == 0) continue;

        let anchored = false;
        if (line[0] == '/') {
            anchored = true;
            line = line[1..];
        } else if (line.len >= 2 && line[0] == '.' && line[1] == '/') {
            anchored = true;
            line = line[2..];
        } else if (pathHasSlash(line)) {
            anchored = true;
        }
        if (line.len == 0) continue;

        if (count >= out_rules.len) return PackRulesError.RuleBufferFull;
        out_rules[count] = PackRulePattern{
            pattern: line,
            base_dir: base_dir,
            depth: depth,
            kind: kind,
            negated: negated,
            dirs_only: dirs_only,
            anchored: anchored,
        };
        count += 1;
    }
    return count;
}

/// Parses a single `package.json`/`cap.json` `"files"` array entry into a `PackRulePattern`.
pub fn parseFilesFieldRule(raw_entry: []const u8): PackRulesError!PackRulePattern {
    let s: usize = 0;
    let e: usize = raw_entry.len;
    while (s < e && (raw_entry[s] == ' ' || raw_entry[s] == '\t')) s += 1;
    while (e > s && (raw_entry[e - 1] == ' ' || raw_entry[e - 1] == '\t')) e -= 1;
    let pat = raw_entry[s..e];
    if (pat.len == 0) return PackRulesError.EmptyPath;

    let negated = false;
    if (pat[0] == '!') {
        negated = true;
        pat = pat[1..];
    }
    let dirs_only = false;
    if (pat.len > 0 && pat[pat.len - 1] == '/') {
        dirs_only = true;
        pat = pat[0..(pat.len - 1)];
    }
    if (pat.len >= 2 && pat[0] == '.' && pat[1] == '/') {
        pat = pat[2..];
    } else if (pat.len >= 1 && pat[0] == '/') {
        pat = pat[1..];
    }
    if (pat.len == 0) return PackRulesError.EmptyPath;

    return PackRulePattern{
        pattern: pat,
        base_dir: "",
        depth: 0,
        kind: PackRuleKind.files_field,
        negated: negated,
        dirs_only: dirs_only,
        anchored: pathHasSlash(pat),
    };
}

// ============================================================================
// Phase 5: External-Slice-Driven `PackPlanner` & Tarball Manifest Digest
// ============================================================================

/// Candidate file/entry discovered during package tree walk.
pub struct PackFileCandidate {
    rel_path: []const u8 = "",
    size_bytes: u64 = 0,
    is_dir: bool = false,
    is_executable: bool = false,
    is_bundled_dep: bool = false,

    static fn file(rel_path: []const u8, size_bytes: u64): PackFileCandidate {
        return PackFileCandidate{
            rel_path: rel_path,
            size_bytes: size_bytes,
            is_dir: false,
            is_executable: false,
            is_bundled_dep: false,
        };
    }

    static fn binFile(rel_path: []const u8, size_bytes: u64): PackFileCandidate {
        return PackFileCandidate{
            rel_path: rel_path,
            size_bytes: size_bytes,
            is_dir: false,
            is_executable: true,
            is_bundled_dep: false,
        };
    }

    static fn bundled(rel_path: []const u8, size_bytes: u64): PackFileCandidate {
        return PackFileCandidate{
            rel_path: rel_path,
            size_bytes: size_bytes,
            is_dir: false,
            is_executable: false,
            is_bundled_dep: true,
        };
    }
}

/// Explainable reason why a candidate was included or excluded.
pub enum PackDecisionReason: u8 {
    always_included = 0,
    entrypoint_included = 1,
    bundled_dep_included = 2,
    files_whitelist_included = 3,
    default_included = 4,
    negated_reincluded = 5,
    hard_excluded = 6,
    default_ignored = 7,
    ignore_rule_excluded = 8,
    not_in_files_whitelist = 9,

    fn isIncluded(): bool {
        return @intFromEnum(_) <= @intFromEnum(PackDecisionReason.negated_reincluded);
    }

    fn asStr(): []const u8 {
        if (_ == PackDecisionReason.always_included) return "always_included";
        if (_ == PackDecisionReason.entrypoint_included) return "entrypoint_included";
        if (_ == PackDecisionReason.bundled_dep_included) return "bundled_dep_included";
        if (_ == PackDecisionReason.files_whitelist_included) return "files_whitelist_included";
        if (_ == PackDecisionReason.default_included) return "default_included";
        if (_ == PackDecisionReason.negated_reincluded) return "negated_reincluded";
        if (_ == PackDecisionReason.hard_excluded) return "hard_excluded";
        if (_ == PackDecisionReason.default_ignored) return "default_ignored";
        if (_ == PackDecisionReason.ignore_rule_excluded) return "ignore_rule_excluded";
        return "not_in_files_whitelist";
    }
}

/// Planned tarball member emitted by `PackPlanner`.
pub struct PackPlannedFile {
    rel_path: []const u8 = "",
    size_bytes: u64 = 0,
    is_executable: bool = false,
    is_bundled_dep: bool = false,
    reason: PackDecisionReason = PackDecisionReason.default_included,

    static fn empty(): PackPlannedFile {
        return PackPlannedFile{};
    }
}

/// Aggregate tarball summary & deterministic manifest fingerprint.
pub struct PackPlanSummary {
    total_candidates: usize = 0,
    packed_files: usize = 0,
    bundled_files: usize = 0,
    ignored_files: usize = 0,
    unpacked_bytes: u64 = 0,
    has_readme: bool = false,
    has_license: bool = false,
    manifest_hash64: u64 = 0,
}

/// External-slice-driven `cap pack` / `cap publish` file filtering & manifest engine.
pub struct PackPlanner {
    files_rules: []PackRulePattern,
    ignore_rules: []PackRulePattern,
    entrypoints: [][]const u8,
    planned: []PackPlannedFile,
    files_rule_len: usize = 0,
    ignore_rule_len: usize = 0,
    entrypoint_len: usize = 0,
    planned_len: usize = 0,
    has_files_whitelist: bool = false,
    summary: PackPlanSummary = PackPlanSummary{},

    static fn init(
        files_rules: []PackRulePattern,
        ignore_rules: []PackRulePattern,
        entrypoints: [][]const u8,
        planned: []PackPlannedFile,
    ): PackPlanner {
        return PackPlanner{
            files_rules: files_rules,
            ignore_rules: ignore_rules,
            entrypoints: entrypoints,
            planned: planned,
        };
    }

    fn plannedCount(): usize {
        return _.planned_len;
    }

    /// Registers a declared `main` or `bin` entrypoint (always included unless hard-excluded).
    fn addEntrypoint(*_, rel_path: []const u8): PackRulesError!void {
        if (rel_path.len == 0) return PackRulesError.EmptyPath;
        if (_.entrypoint_len >= _.entrypoints.len) return PackRulesError.EntrypointBufferFull;
        _.entrypoints[_.entrypoint_len] = rel_path;
        _.entrypoint_len += 1;
    }

    /// Appends a `package.json`/`cap.json` `"files"` entry and enables whitelist mode.
    fn addFilesEntry(*_, raw_entry: []const u8): PackRulesError!void {
        if (_.files_rule_len >= _.files_rules.len) return PackRulesError.RuleBufferFull;
        const rule = parseFilesFieldRule(raw_entry)?;
        _.files_rules[_.files_rule_len] = rule;
        _.files_rule_len += 1;
        _.has_files_whitelist = true;
    }

    /// Parses and appends rules from a `.npmignore` or `.gitignore` file at `(base_dir, depth)`.
    /// Enforces npm/Bun directory precedence: if `.npmignore` already exists for `base_dir`,
    /// `.gitignore` rules for that exact same directory are skipped.
    fn addIgnoreFile(*_, contents: []const u8, base_dir: []const u8, depth: u16, kind: PackRuleKind): PackRulesError!usize {
        if (kind == PackRuleKind.git_ignore && _.hasNpmIgnoreInDir(base_dir)) {
            return 0;
        }
        const remaining = _.ignore_rules[_.ignore_rule_len..];
        const added = parsePackIgnoreFile(contents, base_dir, depth, kind, remaining)?;
        _.ignore_rule_len += added;
        return added;
    }

    fn hasNpmIgnoreInDir(base_dir: []const u8): bool {
        let i: usize = 0;
        while (i < _.ignore_rule_len) {
            const r = _.ignore_rules[i];
            if (r.kind == PackRuleKind.npm_ignore && strEql(r.base_dir, base_dir)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn isDeclaredEntrypoint(rel_path: []const u8): bool {
        let i: usize = 0;
        while (i < _.entrypoint_len) {
            if (strEql(_.entrypoints[i], rel_path)) return true;
            i += 1;
        }
        return false;
    }

    /// Evaluates a single candidate path against the full Bun/npm precedence stack.
    fn evaluateCandidate(cand: PackFileCandidate): PackRulesError!PackDecisionReason {
        let norm_buf: [512]u8 = [0; 512];
        const norm_len = normalizePackRelPath(cand.rel_path, norm_buf[0..])?;
        const rel = norm_buf[0..norm_len];

        // 1. Hard never-overridable excludes
        if (isHardExcludedPackPath(rel, cand.is_bundled_dep)) {
            return PackDecisionReason.hard_excluded;
        }

        // 2. Always-included root metadata
        if (!cand.is_bundled_dep && isAlwaysIncludedPackFile(rel)) {
            return PackDecisionReason.always_included;
        }

        // 3. Bundled dependencies (already filtered against `.git`/`.npmrc`)
        if (cand.is_bundled_dep) {
            return PackDecisionReason.bundled_dep_included;
        }

        // 4. Declared `main` / `bin` entrypoints
        if (_.isDeclaredEntrypoint(rel)) {
            return PackDecisionReason.entrypoint_included;
        }

        // 5. `files` whitelist mode vs default-include mode
        let included = !_.has_files_whitelist;
        let reason = if (_.has_files_whitelist)
            PackDecisionReason.not_in_files_whitelist
        else
            PackDecisionReason.default_included;

        if (_.has_files_whitelist) {
            let i: usize = 0;
            while (i < _.files_rule_len) {
                const r = _.files_rules[i];
                if (r.matches(rel, cand.is_dir)) {
                    if (r.negated) {
                        included = false;
                        reason = PackDecisionReason.not_in_files_whitelist;
                    } else {
                        included = true;
                        reason = PackDecisionReason.files_whitelist_included;
                    }
                }
                i += 1;
            }
            if (!included) return reason;
        }

        // 6. Overridable default ignores (unless explicitly re-included by ignore `!` or `files`)
        if (isDefaultIgnoredPackPath(rel)) {
            included = false;
            reason = PackDecisionReason.default_ignored;
        }

        // 7. Layered `.npmignore` / `.gitignore` evaluation (last matching rule wins)
        let j: usize = 0;
        while (j < _.ignore_rule_len) {
            const r = _.ignore_rules[j];
            if (r.matches(rel, cand.is_dir)) {
                if (r.negated) {
                    included = true;
                    reason = PackDecisionReason.negated_reincluded;
                } else {
                    included = false;
                    reason = PackDecisionReason.ignore_rule_excluded;
                }
            }
            j += 1;
        }

        return reason;
    }

    fn sortPlannedDeterministic(*_) {
        let i: usize = 1;
        while (i < _.planned_len) {
            const key = _.planned[i];
            let j = i;
            while (j > 0 && strCmp(key.rel_path, _.planned[j - 1].rel_path) < 0) {
                _.planned[j] = _.planned[j - 1];
                j -= 1;
            }
            _.planned[j] = key;
            i += 1;
        }
    }

    /// Evaluates all candidates, sorts included files deterministically, and computes
    /// the tarball `PackPlanSummary` including deterministic `manifest_hash64`.
    fn planCandidates(*_, candidates: []const PackFileCandidate): PackRulesError!PackPlanSummary {
        _.planned_len = 0;
        _.summary = PackPlanSummary{
            total_candidates: candidates.len,
        };

        let i: usize = 0;
        while (i < candidates.len) {
            const cand = candidates[i];
            const reason = _.evaluateCandidate(cand)?;
            if (reason.isIncluded() && !cand.is_dir) {
                if (_.planned_len >= _.planned.len) return PackRulesError.PlanBufferFull;
                _.planned[_.planned_len] = PackPlannedFile{
                    rel_path: cand.rel_path,
                    size_bytes: cand.size_bytes,
                    is_executable: cand.is_executable,
                    is_bundled_dep: cand.is_bundled_dep,
                    reason: reason,
                };
                _.planned_len += 1;
                _.summary.unpacked_bytes += cand.size_bytes;
                if (cand.is_bundled_dep) {
                    _.summary.bundled_files += 1;
                }
                if (isRootReadmeFile(cand.rel_path)) _.summary.has_readme = true;
                if (isRootLicenseFile(cand.rel_path)) _.summary.has_license = true;
            } else {
                _.summary.ignored_files += 1;
            }
            i += 1;
        }

        _.sortPlannedDeterministic();
        _.summary.packed_files = _.planned_len;

        // Deterministic 64-bit manifest digest over sorted archive entries
        let h: u64 = 14695981039346656037;
        i = 0;
        while (i < _.planned_len) {
            const f = _.planned[i];
            h = fnv1a64Update(h, f.rel_path);
            h = fnv1a64UpdateU64(h, f.size_bytes);
            h = fnv1a64UpdateU64(h, if (f.is_executable) 1 else 0);
            i += 1;
        }
        _.summary.manifest_hash64 = h;
        return _.summary;
    }

    /// Formats a Bun/npm-aligned pack summary report into caller-provided `out`.
    fn formatPackReport(pkg_name: []const u8, version: []const u8, out: []u8): PackRulesError!usize {
        let pos = appendSlice(out, 0, "Tarball: ")?;
        const tgz_len = formatTarballFileName(pkg_name, version, out[pos..])?;
        pos += tgz_len;
        pos = appendSlice(out, pos, "\nTotal files: ")?;
        pos = writeU64(out, pos, @as(u64, _.summary.packed_files))?;
        pos = appendSlice(out, pos, "\nUnpacked size: ")?;
        pos = writeU64(out, pos, _.summary.unpacked_bytes)?;
        pos = appendSlice(out, pos, " B\nManifest hash: ")?;
        pos = writeHex64(out, pos, _.summary.manifest_hash64)?;
        pos = appendByte(out, pos, '\n')?;

        let i: usize = 0;
        while (i < _.planned_len) {
            const f = _.planned[i];
            pos = appendSlice(out, pos, "  ")?;
            pos = appendSlice(out, pos, f.rel_path)?;
            pos = appendSlice(out, pos, " (")?;
            pos = writeU64(out, pos, f.size_bytes)?;
            pos = appendSlice(out, pos, " B)\n")?;
            i += 1;
        }
        return pos;
    }
}
