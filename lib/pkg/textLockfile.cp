// Source: Absorbed from /data/dev/bun/src/install/lockfile/bun.lock.cp | Synced: 2026-11-22T12:47:25Z
// ============================================================================
// lib/pkg/textLockfile.cp — Human-Readable JSONC Text Lockfile Parser, Formatter & Diff
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven engine for `cap.lock` (JSONC format):
//   1. Complete text lockfile data model (`TextLockfile`) covering
//      `lockfileVersion`, `configVersion`, `workspaces` (deps/dev/optional/peer),
//      `overrides`, `patchedDependencies`, `trustedDependencies`, `catalog`/`catalogs`,
//      and `packages` tuples (`["name@ver", "url", { dependencies, os, cpu, bin }, "sha512-..."]`);
//   2. Zero-heap `cap:simd`-accelerated JSONC parser (`parseTextLockfile`) with
//      `//` & `/* */` comments, trailing commas, depth guard, version validation,
//      and exact 1-based `(line, col)` error diagnostics;
//   3. Deterministic canonical formatter (`formatTextLockfile`) producing clean,
//      Git-diff-friendly 2-space indented JSON with lexicographically sorted keys
//      guaranteeing 100% `parse -> format -> parse` roundtrip idempotence;
//   4. Semantic lockfile package diff engine (`diffTextLockfiles`).

import { Simd } from "cap:simd";

/// Current canonical `cap.lock` text lockfile format version.
pub const TEXT_LOCK_VERSION_CURRENT: u32 = 1;

/// Maximum text lockfile version supported by this parser.
pub const TEXT_LOCK_MAX_SUPPORTED_VERSION: u32 = 1;

/// Current default `configVersion` field.
pub const TEXT_LOCK_CONFIG_VERSION_CURRENT: u32 = 1;

/// Maximum JSONC nesting depth allowed when skipping unknown extension fields.
pub const TEXT_LOCK_MAX_NESTING_DEPTH: u32 = 64;

/// Error set for text lockfile parsing, validation, formatting, and diffing.
pub err TextLockfileError {
    SyntaxError,
    UnexpectedEof,
    UnsupportedLockfileVersion,
    InvalidPackageTuple,
    InvalidFieldType,
    NestingTooDeep,
    SliceCapacityExceeded,
    BufferTooSmall,
    DuplicatePackageKey,
}

// ============================================================================
// Internal Zero-Alloc Byte & JSON Formatting Helpers
// ============================================================================

const TL_HEX_LOWER: []const u8 = "0123456789abcdef";

fn tlBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn tlBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn tlAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn tlAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn tlAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tlAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        tlAppendByte(out, pos, tmp[n])?;
    }
}

fn tlAppendIndent(out: []u8, pos: *usize, level: u32)! {
    let i: u32 = 0;
    while (i < level) {
        tlAppendBytes(out, pos, "  ")?;
        i += 1;
    }
}

fn tlAppendJsonString(out: []u8, pos: *usize, s: []const u8)! {
    tlAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            tlAppendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            tlAppendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            tlAppendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            tlAppendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            tlAppendBytes(out, pos, "\\t")?;
        } else if (c < 0x20) {
            tlAppendBytes(out, pos, "\\u00")?;
            tlAppendByte(out, pos, TL_HEX_LOWER[@as(usize, (c >> 4) & 0x0F)])?;
            tlAppendByte(out, pos, TL_HEX_LOWER[@as(usize, c & 0x0F)])?;
        } else {
            tlAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    tlAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: Complete Text Lockfile Data Model (`TextLockfile`)
// ============================================================================

/// Generic key-value dependency/override/patch/bin entry.
pub struct TextLockKvPair {
    key: []const u8 = "",
    value: []const u8 = "",

    static fn empty(): TextLockKvPair {
        return TextLockKvPair{};
    }
}

/// Catalog dependency entry (`catalog_name == ""` for default `catalog`, non-empty for named `catalogs`).
pub struct TextLockCatalogEntry {
    catalog_name: []const u8 = "",
    pkg_name: []const u8 = "",
    spec: []const u8 = "",

    static fn empty(): TextLockCatalogEntry {
        return TextLockCatalogEntry{};
    }
}

/// Workspace entry inside `"workspaces": { "<rel_path>": { ... } }`.
pub struct TextLockWorkspace {
    rel_path: []const u8 = "",
    name: []const u8 = "",
    deps_start: usize = 0,
    deps_len: usize = 0,
    dev_deps_start: usize = 0,
    dev_deps_len: usize = 0,
    opt_deps_start: usize = 0,
    opt_deps_len: usize = 0,
    peer_deps_start: usize = 0,
    peer_deps_len: usize = 0,

    static fn empty(): TextLockWorkspace {
        return TextLockWorkspace{};
    }
}

/// Package tuple entry inside `"packages": { "<key>": ["name@ver", "url", { ... }, "sha512-..."] }`.
pub struct TextLockPackage {
    key: []const u8 = "",
    name_and_version: []const u8 = "",
    tarball_url: []const u8 = "",
    integrity: []const u8 = "",
    deps_start: usize = 0,
    deps_len: usize = 0,
    os_start: usize = 0,
    os_len: usize = 0,
    cpu_start: usize = 0,
    cpu_len: usize = 0,
    bin_start: usize = 0,
    bin_len: usize = 0,
    has_meta: bool = false,

    static fn empty(): TextLockPackage {
        return TextLockPackage{};
    }
}

/// Caller-owned flat slice storage backing a parsed `TextLockfile`.
pub struct TextLockfileBuffers {
    workspaces: []TextLockWorkspace,
    packages: []TextLockPackage,
    kv_pairs: []TextLockKvPair,
    string_items: [][]const u8,
    catalogs: []TextLockCatalogEntry,
}

/// Parsed text lockfile metadata and slice indices into `TextLockfileBuffers`.
pub struct TextLockfile {
    lockfile_version: u32 = TEXT_LOCK_VERSION_CURRENT,
    config_version: u32 = TEXT_LOCK_CONFIG_VERSION_CURRENT,
    workspace_count: usize = 0,
    package_count: usize = 0,
    overrides_start: usize = 0,
    overrides_len: usize = 0,
    patched_start: usize = 0,
    patched_len: usize = 0,
    trusted_start: usize = 0,
    trusted_len: usize = 0,
    catalog_count: usize = 0,
    kv_used: usize = 0,
    strings_used: usize = 0,
}

/// Exact 1-based `(line, col)` parse diagnostic populated on failure.
pub struct TextLockParseDiag {
    line: u32 = 1,
    col: u32 = 1,
    offset: usize = 0,
    message: []const u8 = "",
}

// ============================================================================
// Section 2: Zero-Heap SIMD JSONC Parser (`parseTextLockfile`)
// ============================================================================

struct JsoncParser {
    src: []const u8,
    pos: usize = 0,
    diag: *TextLockParseDiag,

    fn setDiag(*_, msg: []const u8) {
        const off = if (_.pos <= _.src.len) _.pos else _.src.len;
        const nl_count = Simd.countByte(_.src[0..off], '\n');
        let last_nl: usize = 0;
        let found_nl = false;
        let i = off;
        while (i > 0) {
            i -= 1;
            if (_.src[i] == '\n') {
                last_nl = i;
                found_nl = true;
                break;
            }
        }
        const col: u32 = if (found_nl) @as(u32, @intCast(off - last_nl)) else @as(u32, @intCast(off + 1));
        _.diag.* = TextLockParseDiag{
            line: @as(u32, @intCast(nl_count)) + 1,
            col: col,
            offset: off,
            message: msg,
        };
    }

    fn skipWsAndComments(*_)! {
        while (_.pos < _.src.len) {
            const ws = Simd.skipWhitespace(_.src[_.pos..]);
            _.pos += ws;
            if (_.pos >= _.src.len) return;

            if (_.src[_.pos] == '/' && _.pos + 1 < _.src.len) {
                if (_.src[_.pos + 1] == '/') {
                    _.pos += 2;
                    if (nl := Simd.indexOfByte(_.src[_.pos..], '\n')) {
                        _.pos += nl + 1;
                    } else {
                        _.pos = _.src.len;
                    }
                    continue;
                }
                if (_.src[_.pos + 1] == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < _.src.len) {
                        if (_.src[_.pos] == '*' && _.src[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) {
                        _.setDiag("unterminated block comment");
                        return err.UnexpectedEof;
                    }
                    continue;
                }
            }
            return;
        }
    }

    fn peek(*_): TextLockfileError!u8 {
        _.skipWsAndComments()?;
        if (_.pos >= _.src.len) {
            _.setDiag("unexpected end of input");
            return err.UnexpectedEof;
        }
        return _.src[_.pos];
    }

    fn expectByte(*_, expected: u8, msg: []const u8)! {
        const c = _.peek()?;
        if (c != expected) {
            _.setDiag(msg);
            return err.SyntaxError;
        }
        _.pos += 1;
    }

    fn parseString(*_): TextLockfileError![]const u8 {
        const c = _.peek()?;
        if (c != '"') {
            _.setDiag("expected string literal");
            return err.InvalidFieldType;
        }
        _.pos += 1;
        const start = _.pos;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == '\\') {
                if (_.pos + 1 >= _.src.len) {
                    _.setDiag("unterminated escape sequence");
                    return err.UnexpectedEof;
                }
                _.pos += 2;
                continue;
            }
            if (ch == '"') {
                const slice = _.src[start.._.pos];
                _.pos += 1;
                return slice;
            }
            if (ch == '\n' || ch == '\r') {
                _.setDiag("unterminated string literal");
                return err.SyntaxError;
            }
            _.pos += 1;
        }
        _.setDiag("unterminated string literal");
        return err.UnexpectedEof;
    }

    fn parseU32(*_): TextLockfileError!u32 {
        _.skipWsAndComments()?;
        if (_.pos >= _.src.len) {
            _.setDiag("expected integer");
            return err.UnexpectedEof;
        }
        const start = _.pos;
        let val: u64 = 0;
        let digits: usize = 0;
        while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
            digits += 1;
            if (digits > 10) {
                _.setDiag("integer overflow");
                return err.InvalidFieldType;
            }
            val = val * 10 + @as(u64, _.src[_.pos] - '0');
            if (val > 0xFFFFFFFF) {
                _.setDiag("integer overflow");
                return err.InvalidFieldType;
            }
            _.pos += 1;
        }
        if (_.pos == start) {
            _.setDiag("expected unsigned integer");
            return err.InvalidFieldType;
        }
        return @as(u32, @intCast(val));
    }

    fn skipValue(*_, depth: u32)! {
        if (depth > TEXT_LOCK_MAX_NESTING_DEPTH) {
            _.setDiag("JSONC nesting depth exceeded");
            return err.NestingTooDeep;
        }
        const c = _.peek()?;
        if (c == '"') {
            * = _.parseString()?;
            return;
        }
        if (c == '{') {
            _.pos += 1;
            while (true) {
                const nxt = _.peek()?;
                if (nxt == '}') {
                    _.pos += 1;
                    return;
                }
                * = _.parseString()?;
                _.expectByte(':', "expected ':' after object key")?;
                _.skipValue(depth + 1)?;
                const sep = _.peek()?;
                if (sep == ',') {
                    _.pos += 1;
                    continue;
                }
                if (sep == '}') {
                    _.pos += 1;
                    return;
                }
                _.setDiag("expected ',' or '}' in object");
                return err.SyntaxError;
            }
        }
        if (c == '[') {
            _.pos += 1;
            while (true) {
                const nxt = _.peek()?;
                if (nxt == ']') {
                    _.pos += 1;
                    return;
                }
                _.skipValue(depth + 1)?;
                const sep = _.peek()?;
                if (sep == ',') {
                    _.pos += 1;
                    continue;
                }
                if (sep == ']') {
                    _.pos += 1;
                    return;
                }
                _.setDiag("expected ',' or ']' in array");
                return err.SyntaxError;
            }
        }
        // Primitive token: number, true, false, null
        const start = _.pos;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '/') {
                break;
            }
            _.pos += 1;
        }
        if (_.pos == start) {
            _.setDiag("unexpected token");
            return err.SyntaxError;
        }
    }
}

fn parseStringMapObject(
    p: *JsoncParser,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
): TextLockfileError![2]usize {
    p.expectByte('{', "expected '{' for string map")?;
    const start = lock.kv_used;
    let count: usize = 0;

    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            break;
        }
        const k = p.parseString()?;
        p.expectByte(':', "expected ':' in string map")?;
        const v = p.parseString()?;
        if (lock.kv_used >= bufs.kv_pairs.len) {
            p.setDiag("kv_pairs capacity exceeded");
            return err.SliceCapacityExceeded;
        }
        bufs.kv_pairs[lock.kv_used] = TextLockKvPair{ key: k, value: v };
        lock.kv_used += 1;
        count += 1;

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == '}') {
            p.pos += 1;
            break;
        }
        p.setDiag("expected ',' or '}' in string map");
        return err.SyntaxError;
    }

    return [start, count];
}

fn parseStringArrayOrSingle(
    p: *JsoncParser,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
): TextLockfileError![2]usize {
    const first = p.peek()?;
    const start = lock.strings_used;
    if (first == '"') {
        const s = p.parseString()?;
        if (lock.strings_used >= bufs.string_items.len) return err.SliceCapacityExceeded;
        bufs.string_items[lock.strings_used] = s;
        lock.strings_used += 1;
        return [start, 1];
    }
    p.expectByte('[', "expected '[' for string array")?;
    let count: usize = 0;
    while (true) {
        const nxt = p.peek()?;
        if (nxt == ']') {
            p.pos += 1;
            break;
        }
        const item = p.parseString()?;
        if (lock.strings_used >= bufs.string_items.len) {
            p.setDiag("string_items capacity exceeded");
            return err.SliceCapacityExceeded;
        }
        bufs.string_items[lock.strings_used] = item;
        lock.strings_used += 1;
        count += 1;

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == ']') {
            p.pos += 1;
            break;
        }
        p.setDiag("expected ',' or ']' in string array");
        return err.SyntaxError;
    }
    return [start, count];
}

fn parseWorkspacesSection(
    p: *JsoncParser,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
)! {
    p.expectByte('{', "expected '{' for workspaces")?;
    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            return;
        }
        const rel_path = p.parseString()?;
        p.expectByte(':', "expected ':' after workspace path")?;
        p.expectByte('{', "expected '{' for workspace entry")?;

        let ws = TextLockWorkspace{ rel_path: rel_path };
        while (true) {
            const f_nxt = p.peek()?;
            if (f_nxt == '}') {
                p.pos += 1;
                break;
            }
            const field = p.parseString()?;
            p.expectByte(':', "expected ':' after workspace field")?;
            if (tlBytesEql(field, "name")) {
                ws.name = p.parseString()?;
            } else if (tlBytesEql(field, "dependencies")) {
                const span = parseStringMapObject(p, bufs, lock)?;
                ws.deps_start = span[0];
                ws.deps_len = span[1];
            } else if (tlBytesEql(field, "devDependencies")) {
                const span = parseStringMapObject(p, bufs, lock)?;
                ws.dev_deps_start = span[0];
                ws.dev_deps_len = span[1];
            } else if (tlBytesEql(field, "optionalDependencies")) {
                const span = parseStringMapObject(p, bufs, lock)?;
                ws.opt_deps_start = span[0];
                ws.opt_deps_len = span[1];
            } else if (tlBytesEql(field, "peerDependencies")) {
                const span = parseStringMapObject(p, bufs, lock)?;
                ws.peer_deps_start = span[0];
                ws.peer_deps_len = span[1];
            } else {
                p.skipValue(1)?;
            }

            const f_sep = p.peek()?;
            if (f_sep == ',') {
                p.pos += 1;
                continue;
            }
            if (f_sep == '}') {
                p.pos += 1;
                break;
            }
            p.setDiag("expected ',' or '}' in workspace entry");
            return err.SyntaxError;
        }

        if (lock.workspace_count >= bufs.workspaces.len) {
            p.setDiag("workspaces capacity exceeded");
            return err.SliceCapacityExceeded;
        }
        bufs.workspaces[lock.workspace_count] = ws;
        lock.workspace_count += 1;

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == '}') {
            p.pos += 1;
            return;
        }
        p.setDiag("expected ',' or '}' in workspaces section");
        return err.SyntaxError;
    }
}

fn parseCatalogMapInto(
    p: *JsoncParser,
    catalog_name: []const u8,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
)! {
    p.expectByte('{', "expected '{' for catalog")?;
    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            return;
        }
        const pkg_name = p.parseString()?;
        p.expectByte(':', "expected ':' in catalog")?;
        const spec = p.parseString()?;
        if (lock.catalog_count >= bufs.catalogs.len) {
            p.setDiag("catalogs capacity exceeded");
            return err.SliceCapacityExceeded;
        }
        bufs.catalogs[lock.catalog_count] = TextLockCatalogEntry{
            catalog_name: catalog_name,
            pkg_name: pkg_name,
            spec: spec,
        };
        lock.catalog_count += 1;

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == '}') {
            p.pos += 1;
            return;
        }
        p.setDiag("expected ',' or '}' in catalog");
        return err.SyntaxError;
    }
}

fn parsePackagesSection(
    p: *JsoncParser,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
)! {
    p.expectByte('{', "expected '{' for packages")?;
    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            return;
        }
        const pkg_key = p.parseString()?;
        p.expectByte(':', "expected ':' after package key")?;

        // Duplicate package key guard
        let dup_i: usize = 0;
        while (dup_i < lock.package_count) {
            if (tlBytesEql(bufs.packages[dup_i].key, pkg_key)) {
                p.setDiag("duplicate package key in lockfile");
                return err.DuplicatePackageKey;
            }
            dup_i += 1;
        }

        // Parse tuple: ["name@ver", "tarball_url", { meta }, "sha512-..."]
        p.expectByte('[', "expected '[' for package tuple")?;
        let pkg = TextLockPackage{ key: pkg_key };

        // Tuple[0]: name@version (mandatory)
        pkg.name_and_version = p.parseString()?;

        let elem_idx: u32 = 1;
        while (true) {
            const sep = p.peek()?;
            if (sep == ']') {
                p.pos += 1;
                break;
            }
            if (sep != ',') {
                p.setDiag("expected ',' or ']' in package tuple");
                return err.InvalidPackageTuple;
            }
            p.pos += 1;
            const after_comma = p.peek()?;
            if (after_comma == ']') {
                p.pos += 1;
                break;
            }

            if (elem_idx == 1) {
                // Tuple[1]: tarball_url string (or meta object in compact 2-elem workspace/folder form)
                if (after_comma == '"') {
                    pkg.tarball_url = p.parseString()?;
                } else if (after_comma == '{') {
                    pkg.has_meta = true;
                    p.expectByte('{', "expected '{'")?;
                    parsePackageMetaFields(p, bufs, lock, &pkg)?;
                } else {
                    p.setDiag("invalid package tuple element 1");
                    return err.InvalidPackageTuple;
                }
            } else if (elem_idx == 2) {
                // Tuple[2]: metadata object { dependencies, os, cpu, bin }
                if (after_comma != '{') {
                    p.setDiag("expected metadata object at package tuple[2]");
                    return err.InvalidPackageTuple;
                }
                pkg.has_meta = true;
                p.pos += 1;
                parsePackageMetaFields(p, bufs, lock, &pkg)?;
            } else if (elem_idx == 3) {
                // Tuple[3]: integrity hash string
                pkg.integrity = p.parseString()?;
            } else {
                p.skipValue(1)?;
            }
            elem_idx += 1;
        }

        if (lock.package_count >= bufs.packages.len) {
            p.setDiag("packages capacity exceeded");
            return err.SliceCapacityExceeded;
        }
        bufs.packages[lock.package_count] = pkg;
        lock.package_count += 1;

        const p_sep = p.peek()?;
        if (p_sep == ',') {
            p.pos += 1;
            continue;
        }
        if (p_sep == '}') {
            p.pos += 1;
            return;
        }
        p.setDiag("expected ',' or '}' in packages section");
        return err.SyntaxError;
    }
}

fn parsePackageMetaFields(
    p: *JsoncParser,
    bufs: *TextLockfileBuffers,
    lock: *TextLockfile,
    pkg: *TextLockPackage,
)! {
    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            return;
        }
        const field = p.parseString()?;
        p.expectByte(':', "expected ':' in package meta")?;

        if (tlBytesEql(field, "dependencies")) {
            const span = parseStringMapObject(p, bufs, lock)?;
            pkg.deps_start = span[0];
            pkg.deps_len = span[1];
        } else if (tlBytesEql(field, "os")) {
            const span = parseStringArrayOrSingle(p, bufs, lock)?;
            pkg.os_start = span[0];
            pkg.os_len = span[1];
        } else if (tlBytesEql(field, "cpu")) {
            const span = parseStringArrayOrSingle(p, bufs, lock)?;
            pkg.cpu_start = span[0];
            pkg.cpu_len = span[1];
        } else if (tlBytesEql(field, "bin")) {
            const c = p.peek()?;
            if (c == '"') {
                const bin_val = p.parseString()?;
                if (lock.kv_used >= bufs.kv_pairs.len) return err.SliceCapacityExceeded;
                pkg.bin_start = lock.kv_used;
                pkg.bin_len = 1;
                bufs.kv_pairs[lock.kv_used] = TextLockKvPair{ key: "", value: bin_val };
                lock.kv_used += 1;
            } else {
                const span = parseStringMapObject(p, bufs, lock)?;
                pkg.bin_start = span[0];
                pkg.bin_len = span[1];
            }
        } else {
            p.skipValue(1)?;
        }

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == '}') {
            p.pos += 1;
            return;
        }
        p.setDiag("expected ',' or '}' in package meta");
        return err.SyntaxError;
    }
}

/// Parses a JSONC `cap.lock` / `bun.lock` text lockfile into `TextLockfile` and
/// caller-provided `bufs` with zero heap allocation.
/// On error, populates `diag` with 1-based `(line, col)` and a diagnostic message.
pub fn parseTextLockfile(
    src: []const u8,
    bufs: *TextLockfileBuffers,
    diag: *TextLockParseDiag,
): TextLockfileError!TextLockfile {
    diag.* = TextLockParseDiag{};
    let lock = TextLockfile{};
    let p = JsoncParser{ src: src, pos: 0, diag: diag };

    p.expectByte('{', "lockfile root must be a JSONC object")?;

    while (true) {
        const nxt = p.peek()?;
        if (nxt == '}') {
            p.pos += 1;
            break;
        }
        const key = p.parseString()?;
        p.expectByte(':', "expected ':' after top-level key")?;

        if (tlBytesEql(key, "lockfileVersion")) {
            const ver = p.parseU32()?;
            if (ver > TEXT_LOCK_MAX_SUPPORTED_VERSION) {
                p.setDiag("unsupported lockfileVersion");
                return err.UnsupportedLockfileVersion;
            }
            lock.lockfile_version = ver;
        } else if (tlBytesEql(key, "configVersion")) {
            lock.config_version = p.parseU32()?;
        } else if (tlBytesEql(key, "workspaces")) {
            parseWorkspacesSection(&p, bufs, &lock)?;
        } else if (tlBytesEql(key, "overrides")) {
            const span = parseStringMapObject(&p, bufs, &lock)?;
            lock.overrides_start = span[0];
            lock.overrides_len = span[1];
        } else if (tlBytesEql(key, "patchedDependencies")) {
            const span = parseStringMapObject(&p, bufs, &lock)?;
            lock.patched_start = span[0];
            lock.patched_len = span[1];
        } else if (tlBytesEql(key, "trustedDependencies")) {
            const span = parseStringArrayOrSingle(&p, bufs, &lock)?;
            lock.trusted_start = span[0];
            lock.trusted_len = span[1];
        } else if (tlBytesEql(key, "catalog")) {
            parseCatalogMapInto(&p, "", bufs, &lock)?;
        } else if (tlBytesEql(key, "catalogs")) {
            p.expectByte('{', "expected '{' for catalogs")?;
            while (true) {
                const c_nxt = p.peek()?;
                if (c_nxt == '}') {
                    p.pos += 1;
                    break;
                }
                const cname = p.parseString()?;
                p.expectByte(':', "expected ':' after catalog name")?;
                parseCatalogMapInto(&p, cname, bufs, &lock)?;
                const c_sep = p.peek()?;
                if (c_sep == ',') {
                    p.pos += 1;
                    continue;
                }
                if (c_sep == '}') {
                    p.pos += 1;
                    break;
                }
                p.setDiag("expected ',' or '}' in catalogs");
                return err.SyntaxError;
            }
        } else if (tlBytesEql(key, "packages")) {
            parsePackagesSection(&p, bufs, &lock)?;
        } else {
            p.skipValue(1)?;
        }

        const sep = p.peek()?;
        if (sep == ',') {
            p.pos += 1;
            continue;
        }
        if (sep == '}') {
            p.pos += 1;
            break;
        }
        p.setDiag("expected ',' or '}' at top level");
        return err.SyntaxError;
    }

    // Ensure no trailing non-comment garbage
    p.skipWsAndComments()?;
    if (p.pos < src.len) {
        p.setDiag("unexpected trailing tokens after lockfile object");
        return err.SyntaxError;
    }

    return lock;
}

// ============================================================================
// Section 3: Deterministic Canonical Formatter (`formatTextLockfile`)
// ============================================================================

fn formatSortedKvObject(
    kvs: []const TextLockKvPair,
    indent: u32,
    scratch: []usize,
    out: []u8,
    pos: *usize,
)! {
    const n = kvs.len;
    if (n > scratch.len) return err.SliceCapacityExceeded;
    let i: usize = 0;
    while (i < n) {
        scratch[i] = i;
        i += 1;
    }
    let s: usize = 1;
    while (s < n) {
        const idx = scratch[s];
        let j = s;
        while (j > 0 && tlBytesLessThan(kvs[idx].key, kvs[scratch[j - 1]].key)) {
            scratch[j] = scratch[j - 1];
            j -= 1;
        }
        scratch[j] = idx;
        s += 1;
    }

    tlAppendBytes(out, pos, "{\n")?;
    let k: usize = 0;
    while (k < n) {
        const kv = kvs[scratch[k]];
        tlAppendIndent(out, pos, indent + 1)?;
        tlAppendJsonString(out, pos, kv.key)?;
        tlAppendBytes(out, pos, ": ")?;
        tlAppendJsonString(out, pos, kv.value)?;
        if (k + 1 < n) tlAppendByte(out, pos, ',')?;
        tlAppendByte(out, pos, '\n')?;
        k += 1;
    }
    tlAppendIndent(out, pos, indent)?;
    tlAppendByte(out, pos, '}')?;
}

fn formatInlineKvObject(
    kvs: []const TextLockKvPair,
    scratch: []usize,
    out: []u8,
    pos: *usize,
)! {
    const n = kvs.len;
    if (n > scratch.len) return err.SliceCapacityExceeded;
    let i: usize = 0;
    while (i < n) {
        scratch[i] = i;
        i += 1;
    }
    let s: usize = 1;
    while (s < n) {
        const idx = scratch[s];
        let j = s;
        while (j > 0 && tlBytesLessThan(kvs[idx].key, kvs[scratch[j - 1]].key)) {
            scratch[j] = scratch[j - 1];
            j -= 1;
        }
        scratch[j] = idx;
        s += 1;
    }

    tlAppendBytes(out, pos, "{ ")?;
    let k: usize = 0;
    while (k < n) {
        if (k > 0) tlAppendBytes(out, pos, ", ")?;
        const kv = kvs[scratch[k]];
        tlAppendJsonString(out, pos, kv.key)?;
        tlAppendBytes(out, pos, ": ")?;
        tlAppendJsonString(out, pos, kv.value)?;
        k += 1;
    }
    tlAppendBytes(out, pos, " }")?;
}

fn formatInlineStringList(
    items: []const []const u8,
    out: []u8,
    pos: *usize,
)! {
    tlAppendByte(out, pos, '[')?;
    let i: usize = 0;
    while (i < items.len) {
        if (i > 0) tlAppendBytes(out, pos, ", ")?;
        tlAppendJsonString(out, pos, items[i])?;
        i += 1;
    }
    tlAppendByte(out, pos, ']')?;
}

/// Formats `lock` deterministically into 2-space indented canonical JSON with
/// lexicographically sorted workspaces, overrides, patches, trusted deps, and
/// packages (`parse -> format -> parse` idempotent).
/// `sort_scratch` must be at least as large as `max(workspace_count, package_count, max_kv_len)`.
pub fn formatTextLockfile(
    lock: TextLockfile,
    bufs: TextLockfileBuffers,
    sort_scratch: []usize,
    out_buf: []u8,
): TextLockfileError!usize {
    let pos: usize = 0;
    tlAppendBytes(out_buf, &pos, "{\n")?;

    // lockfileVersion
    tlAppendBytes(out_buf, &pos, "  \"lockfileVersion\": ")?;
    tlAppendU64(out_buf, &pos, @as(u64, lock.lockfile_version))?;
    tlAppendBytes(out_buf, &pos, ",\n")?;

    // configVersion
    tlAppendBytes(out_buf, &pos, "  \"configVersion\": ")?;
    tlAppendU64(out_buf, &pos, @as(u64, lock.config_version))?;
    tlAppendBytes(out_buf, &pos, ",\n")?;

    // workspaces (sorted by rel_path, "" root first)
    tlAppendBytes(out_buf, &pos, "  \"workspaces\": {\n")?;
    const ws_n = lock.workspace_count;
    if (ws_n > sort_scratch.len) return err.SliceCapacityExceeded;
    let wi: usize = 0;
    while (wi < ws_n) {
        sort_scratch[wi] = wi;
        wi += 1;
    }
    let ws_s: usize = 1;
    while (ws_s < ws_n) {
        const idx = sort_scratch[ws_s];
        let j = ws_s;
        while (j > 0 && tlBytesLessThan(bufs.workspaces[idx].rel_path, bufs.workspaces[sort_scratch[j - 1]].rel_path)) {
            sort_scratch[j] = sort_scratch[j - 1];
            j -= 1;
        }
        sort_scratch[j] = idx;
        ws_s += 1;
    }

    let w: usize = 0;
    while (w < ws_n) {
        const ws = bufs.workspaces[sort_scratch[w]];
        tlAppendBytes(out_buf, &pos, "    ")?;
        tlAppendJsonString(out_buf, &pos, ws.rel_path)?;
        tlAppendBytes(out_buf, &pos, ": {\n")?;

        let wrote_field = false;
        if (ws.name.len > 0) {
            tlAppendBytes(out_buf, &pos, "      \"name\": ")?;
            tlAppendJsonString(out_buf, &pos, ws.name)?;
            wrote_field = true;
        }
        if (ws.deps_len > 0) {
            if (wrote_field) tlAppendBytes(out_buf, &pos, ",\n")?;
            tlAppendBytes(out_buf, &pos, "      \"dependencies\": ")?;
            formatSortedKvObject(bufs.kv_pairs[ws.deps_start..ws.deps_start + ws.deps_len], 3, sort_scratch[ws_n..], out_buf, &pos)?;
            wrote_field = true;
        }
        if (ws.dev_deps_len > 0) {
            if (wrote_field) tlAppendBytes(out_buf, &pos, ",\n")?;
            tlAppendBytes(out_buf, &pos, "      \"devDependencies\": ")?;
            formatSortedKvObject(bufs.kv_pairs[ws.dev_deps_start..ws.dev_deps_start + ws.dev_deps_len], 3, sort_scratch[ws_n..], out_buf, &pos)?;
            wrote_field = true;
        }
        if (ws.opt_deps_len > 0) {
            if (wrote_field) tlAppendBytes(out_buf, &pos, ",\n")?;
            tlAppendBytes(out_buf, &pos, "      \"optionalDependencies\": ")?;
            formatSortedKvObject(bufs.kv_pairs[ws.opt_deps_start..ws.opt_deps_start + ws.opt_deps_len], 3, sort_scratch[ws_n..], out_buf, &pos)?;
            wrote_field = true;
        }
        if (ws.peer_deps_len > 0) {
            if (wrote_field) tlAppendBytes(out_buf, &pos, ",\n")?;
            tlAppendBytes(out_buf, &pos, "      \"peerDependencies\": ")?;
            formatSortedKvObject(bufs.kv_pairs[ws.peer_deps_start..ws.peer_deps_start + ws.peer_deps_len], 3, sort_scratch[ws_n..], out_buf, &pos)?;
            wrote_field = true;
        }
        if (wrote_field) tlAppendByte(out_buf, &pos, '\n')?;
        tlAppendBytes(out_buf, &pos, "    }")?;
        if (w + 1 < ws_n) tlAppendByte(out_buf, &pos, ',')?;
        tlAppendByte(out_buf, &pos, '\n')?;
        w += 1;
    }
    tlAppendBytes(out_buf, &pos, "  }")?;

    // Optional overrides
    if (lock.overrides_len > 0) {
        tlAppendBytes(out_buf, &pos, ",\n  \"overrides\": ")?;
        formatSortedKvObject(bufs.kv_pairs[lock.overrides_start..lock.overrides_start + lock.overrides_len], 1, sort_scratch, out_buf, &pos)?;
    }

    // Optional patchedDependencies
    if (lock.patched_len > 0) {
        tlAppendBytes(out_buf, &pos, ",\n  \"patchedDependencies\": ")?;
        formatSortedKvObject(bufs.kv_pairs[lock.patched_start..lock.patched_start + lock.patched_len], 1, sort_scratch, out_buf, &pos)?;
    }

    // Optional trustedDependencies
    if (lock.trusted_len > 0) {
        tlAppendBytes(out_buf, &pos, ",\n  \"trustedDependencies\": ")?;
        formatInlineStringList(bufs.string_items[lock.trusted_start..lock.trusted_start + lock.trusted_len], out_buf, &pos)?;
    }

    // Optional default catalog
    let has_default_cat = false;
    let ci: usize = 0;
    let default_cat_count: usize = 0;
    while (ci < lock.catalog_count) {
        if (bufs.catalogs[ci].catalog_name.len == 0) {
            default_cat_count += 1;
        }
        ci += 1;
    }
    * = has_default_cat;
    if (default_cat_count > 0) {
        tlAppendBytes(out_buf, &pos, ",\n  \"catalog\": {\n")?;
        let emitted: usize = 0;
        let k: usize = 0;
        while (k < lock.catalog_count) {
            const c_entry = bufs.catalogs[k];
            if (c_entry.catalog_name.len == 0) {
                tlAppendBytes(out_buf, &pos, "    ")?;
                tlAppendJsonString(out_buf, &pos, c_entry.pkg_name)?;
                tlAppendBytes(out_buf, &pos, ": ")?;
                tlAppendJsonString(out_buf, &pos, c_entry.spec)?;
                emitted += 1;
                if (emitted < default_cat_count) tlAppendByte(out_buf, &pos, ',')?;
                tlAppendByte(out_buf, &pos, '\n')?;
            }
            k += 1;
        }
        tlAppendBytes(out_buf, &pos, "  }")?;
    }

    // packages (sorted lexicographically by key)
    tlAppendBytes(out_buf, &pos, ",\n  \"packages\": {\n")?;
    const pkg_n = lock.package_count;
    if (pkg_n > sort_scratch.len) return err.SliceCapacityExceeded;
    let pi: usize = 0;
    while (pi < pkg_n) {
        sort_scratch[pi] = pi;
        pi += 1;
    }
    let ps: usize = 1;
    while (ps < pkg_n) {
        const idx = sort_scratch[ps];
        let j = ps;
        while (j > 0 && tlBytesLessThan(bufs.packages[idx].key, bufs.packages[sort_scratch[j - 1]].key)) {
            sort_scratch[j] = sort_scratch[j - 1];
            j -= 1;
        }
        sort_scratch[j] = idx;
        ps += 1;
    }

    let p_idx: usize = 0;
    while (p_idx < pkg_n) {
        const pkg = bufs.packages[sort_scratch[p_idx]];
        tlAppendBytes(out_buf, &pos, "    ")?;
        tlAppendJsonString(out_buf, &pos, pkg.key)?;
        tlAppendBytes(out_buf, &pos, ": [")?;
        tlAppendJsonString(out_buf, &pos, pkg.name_and_version)?;
        tlAppendBytes(out_buf, &pos, ", ")?;
        tlAppendJsonString(out_buf, &pos, pkg.tarball_url)?;
        tlAppendBytes(out_buf, &pos, ", {")?;

        let meta_first = true;
        if (pkg.deps_len > 0) {
            tlAppendBytes(out_buf, &pos, " \"dependencies\": ")?;
            formatInlineKvObject(bufs.kv_pairs[pkg.deps_start..pkg.deps_start + pkg.deps_len], sort_scratch[pkg_n..], out_buf, &pos)?;
            meta_first = false;
        }
        if (pkg.os_len > 0) {
            if (!meta_first) tlAppendByte(out_buf, &pos, ',')?;
            tlAppendBytes(out_buf, &pos, " \"os\": ")?;
            formatInlineStringList(bufs.string_items[pkg.os_start..pkg.os_start + pkg.os_len], out_buf, &pos)?;
            meta_first = false;
        }
        if (pkg.cpu_len > 0) {
            if (!meta_first) tlAppendByte(out_buf, &pos, ',')?;
            tlAppendBytes(out_buf, &pos, " \"cpu\": ")?;
            formatInlineStringList(bufs.string_items[pkg.cpu_start..pkg.cpu_start + pkg.cpu_len], out_buf, &pos)?;
            meta_first = false;
        }
        if (pkg.bin_len > 0) {
            if (!meta_first) tlAppendByte(out_buf, &pos, ',')?;
            tlAppendBytes(out_buf, &pos, " \"bin\": ")?;
            if (pkg.bin_len == 1 && bufs.kv_pairs[pkg.bin_start].key.len == 0) {
                tlAppendJsonString(out_buf, &pos, bufs.kv_pairs[pkg.bin_start].value)?;
            } else {
                formatInlineKvObject(bufs.kv_pairs[pkg.bin_start..pkg.bin_start + pkg.bin_len], sort_scratch[pkg_n..], out_buf, &pos)?;
            }
            meta_first = false;
        }
        if (!meta_first) tlAppendByte(out_buf, &pos, ' ')?;
        tlAppendBytes(out_buf, &pos, "}, ")?;
        tlAppendJsonString(out_buf, &pos, pkg.integrity)?;
        tlAppendByte(out_buf, &pos, ']')?;
        if (p_idx + 1 < pkg_n) tlAppendByte(out_buf, &pos, ',')?;
        tlAppendByte(out_buf, &pos, '\n')?;
        p_idx += 1;
    }

    tlAppendBytes(out_buf, &pos, "  }\n}\n")?;
    return pos;
}

// ============================================================================
// Section 4: Semantic Text Lockfile Package Diff Engine
// ============================================================================

/// Kind of package change detected between two `TextLockfile` snapshots.
pub enum TextLockDiffKind: u8 {
    Added = 0,
    Removed = 1,
    Changed = 2,
}

/// Single package difference entry produced by `diffTextLockfiles`.
pub struct TextLockDiffEntry {
    kind: TextLockDiffKind = TextLockDiffKind.Added,
    package_key: []const u8 = "",
    old_spec: []const u8 = "",
    new_spec: []const u8 = "",
    old_integrity: []const u8 = "",
    new_integrity: []const u8 = "",

    static fn empty(): TextLockDiffEntry {
        return TextLockDiffEntry{};
    }
}

/// Aggregate summary returned by `diffTextLockfiles`.
pub struct TextLockDiffSummary {
    added: u32 = 0,
    removed: u32 = 0,
    changed: u32 = 0,
    unchanged: u32 = 0,
    total_diffs: usize = 0,
}

fn findPackageByKey(pkgs: []const TextLockPackage, key: []const u8): usize? {
    let i: usize = 0;
    while (i < pkgs.len) {
        if (tlBytesEql(pkgs[i].key, key)) return i;
        i += 1;
    }
    return null;
}

/// Computes semantic package diffs (`Added`, `Removed`, `Changed`) between `old_pkgs`
/// and `new_pkgs` into caller-provided `out_diffs` with zero heap allocation.
pub fn diffTextLockfiles(
    old_pkgs: []const TextLockPackage,
    new_pkgs: []const TextLockPackage,
    out_diffs: []TextLockDiffEntry,
): TextLockfileError!TextLockDiffSummary {
    let sum = TextLockDiffSummary{};

    // Detect removed and changed packages
    let i: usize = 0;
    while (i < old_pkgs.len) {
        const old_p = old_pkgs[i];
        if (n_idx := findPackageByKey(new_pkgs, old_p.key)) {
            const new_p = new_pkgs[n_idx];
            if (!tlBytesEql(old_p.name_and_version, new_p.name_and_version) ||
                !tlBytesEql(old_p.integrity, new_p.integrity) ||
                !tlBytesEql(old_p.tarball_url, new_p.tarball_url))
            {
                if (sum.total_diffs >= out_diffs.len) return err.SliceCapacityExceeded;
                out_diffs[sum.total_diffs] = TextLockDiffEntry{
                    kind: TextLockDiffKind.Changed,
                    package_key: old_p.key,
                    old_spec: old_p.name_and_version,
                    new_spec: new_p.name_and_version,
                    old_integrity: old_p.integrity,
                    new_integrity: new_p.integrity,
                };
                sum.total_diffs += 1;
                sum.changed += 1;
            } else {
                sum.unchanged += 1;
            }
        } else {
            if (sum.total_diffs >= out_diffs.len) return err.SliceCapacityExceeded;
            out_diffs[sum.total_diffs] = TextLockDiffEntry{
                kind: TextLockDiffKind.Removed,
                package_key: old_p.key,
                old_spec: old_p.name_and_version,
                old_integrity: old_p.integrity,
            };
            sum.total_diffs += 1;
            sum.removed += 1;
        }
        i += 1;
    }

    // Detect newly added packages
    let k: usize = 0;
    while (k < new_pkgs.len) {
        const new_p = new_pkgs[k];
        if (findPackageByKey(old_pkgs, new_p.key) == null) {
            if (sum.total_diffs >= out_diffs.len) return err.SliceCapacityExceeded;
            out_diffs[sum.total_diffs] = TextLockDiffEntry{
                kind: TextLockDiffKind.Added,
                package_key: new_p.key,
                new_spec: new_p.name_and_version,
                new_integrity: new_p.integrity,
            };
            sum.total_diffs += 1;
            sum.added += 1;
        }
        k += 1;
    }

    return sum;
}
