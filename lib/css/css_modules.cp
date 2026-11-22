// Source: Absorbed from /data/dev/bun/src/css/css_modules.cp | Synced: 2026-11-22T00:22:00Z (Zero-Alloc CSS Modules Naming Pattern, Hash & Composes Parser)

const MAX_PATTERN_SEGMENTS: usize = 16;
const B64_URL_TABLE: [64]u8 = [
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_',
];

/// Configuration flags controlling which CSS identifiers are locally scoped (mirrors Bun `css_modules.Config`).
pub struct CssModuleConfig {
    dashed_idents: bool,
    animation: bool,
    grid: bool,
    custom_idents: bool,

    fn default(): CssModuleConfig {
        return {
            dashed_idents: false,
            animation: true,
            grid: true,
            custom_idents: true,
        };
    }
}

/// Kind of segment in a CSS Modules naming `Pattern`.
pub enum PatternKind: u8 {
    literal = 0,
    name = 1,
    local = 2,
    hash = 3,
}

/// Single segment of a parsed CSS Modules naming pattern.
pub struct PatternSegment {
    kind: PatternKind,
    literal: []const u8,
}

/// Zero-allocation compiled CSS Modules class-name pattern (e.g. `[local]_[hash]` or `[name]__[local]--[hash]`).
pub struct Pattern {
    segments: [16]PatternSegment,
    len: usize,

    /// Default Bun CSS Modules pattern: `[local]_[hash]`.
    fn default(): Pattern {
        let segs: [16]PatternSegment = [{ kind: PatternKind.literal, literal: "" }; 16];
        segs[0] = { kind: PatternKind.local, literal: "" };
        segs[1] = { kind: PatternKind.literal, literal: "_" };
        segs[2] = { kind: PatternKind.hash, literal: "" };
        return { segments: segs, len: 3 };
    }

    /// Parses a template string containing `[name]`, `[local]`, `[hash]` placeholders and literal text.
    /// Returns `null` on empty template, unclosed `[`, unknown placeholder, or more than 16 segments.
    fn parse(template: []const u8): Pattern? {
        if (template.len == 0) return null;
        let segs: [16]PatternSegment = [{ kind: PatternKind.literal, literal: "" }; 16];
        let count: usize = 0;
        let i: usize = 0;

        while (i < template.len) {
            if (template[i] == '[') {
                let close = i + 1;
                while (close < template.len && template[close] != ']') {
                    close += 1;
                }
                if (close >= template.len) return null;
                const tag = template[(i + 1)..close];
                let kind = PatternKind.literal;
                if (std.mem.eql(u8, tag, "name")) {
                    kind = PatternKind.name;
                } else if (std.mem.eql(u8, tag, "local")) {
                    kind = PatternKind.local;
                } else if (std.mem.eql(u8, tag, "hash")) {
                    kind = PatternKind.hash;
                } else {
                    return null;
                }
                if (count >= MAX_PATTERN_SEGMENTS) return null;
                segs[count] = { kind: kind, literal: "" };
                count += 1;
                i = close + 1;
            } else {
                const start = i;
                while (i < template.len && template[i] != '[') {
                    if (template[i] == ']') return null;
                    i += 1;
                }
                if (count >= MAX_PATTERN_SEGMENTS) return null;
                segs[count] = { kind: PatternKind.literal, literal: template[start..i] };
                count += 1;
            }
        }

        if (count == 0) return null;
        return { segments: segs, len: count };
    }

    /// Returns `true` if the pattern begins with `[hash]`, requiring CSS identifier leading-digit protection.
    fn startsWithHash(): bool {
        return _.len > 0 && _.segments[0].kind == PatternKind.hash;
    }

    /// Formats the substituted CSS identifier into `out`. Returns `null` if `out` is too small.
    fn format(
        hash_str: []const u8,
        file_path: []const u8,
        local: []const u8,
        out: *[]u8,
    ): ([]const u8)? {
        let w: usize = 0;
        let idx: usize = 0;
        while (idx < _.len) {
            const seg = _.segments[idx];
            match (seg.kind) {
                PatternKind.literal => {
                    if (w + seg.literal.len > out.*.len) return null;
                    let k: usize = 0;
                    while (k < seg.literal.len) {
                        out.*[w + k] = seg.literal[k];
                        k += 1;
                    }
                    w += seg.literal.len;
                },
                PatternKind.local => {
                    if (w + local.len > out.*.len) return null;
                    let k: usize = 0;
                    while (k < local.len) {
                        out.*[w + k] = local[k];
                        k += 1;
                    }
                    w += local.len;
                },
                PatternKind.hash => {
                    if (w + hash_str.len > out.*.len) return null;
                    let k: usize = 0;
                    while (k < hash_str.len) {
                        out.*[w + k] = hash_str[k];
                        k += 1;
                    }
                    w += hash_str.len;
                },
                PatternKind.name => {
                    let rem = out.*[w..];
                    if (stem := extractFileStemSanitized(file_path, *rem)) {
                        w += stem.len;
                    } else {
                        return null;
                    }
                },
            }
            idx += 1;
        }
        return out.*[0..w];
    }
}

/// Kind of CSS Modules `composes` reference (mirrors Bun `CssModuleReference`).
pub enum ReferenceKind: u8 {
    local = 0,
    global = 1,
    dependency = 2,
}

/// Single parsed reference from a CSS Modules `composes:` declaration.
pub struct CssModuleRef {
    kind: ReferenceKind,
    name: []const u8,
    specifier: []const u8,

    fn eql(other: CssModuleRef): bool {
        if (_.kind != other.kind) return false;
        if (!std.mem.eql(u8, _.name, other.name)) return false;
        return std.mem.eql(u8, _.specifier, other.specifier);
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

/// Checks whether `ident` is a valid unescaped ASCII CSS identifier (cannot start with digit or `-<digit>`).
pub fn isValidCssIdent(ident: []const u8): bool {
    if (ident.len == 0) return false;
    const c0 = ident[0];
    if (c0 >= '0' && c0 <= '9') return false;
    if (c0 == '-' && ident.len >= 2 && ident[1] >= '0' && ident[1] <= '9') return false;
    let i: usize = 0;
    while (i < ident.len) {
        if (!isIdentChar(ident[i])) return false;
        i += 1;
    }
    return true;
}

/// Extracts the base filename stem from `path` (stripping directory and last extension)
/// and replaces any remaining `.` with `-` (e.g. `src/Button.module.css` -> `Button-module`).
pub fn extractFileStemSanitized(path: []const u8, out: *[]u8): ([]const u8)? {
    let base_start: usize = 0;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '/' || path[i] == '\\') {
            base_start = i + 1;
        }
        i += 1;
    }
    const base = path[base_start..];
    let dot_pos = base.len;
    let j = base.len;
    while (j > 0) {
        j -= 1;
        if (base[j] == '.') {
            if (j > 0) {
                dot_pos = j;
            }
            break;
        }
    }
    const stem = base[0..dot_pos];
    if (stem.len > out.*.len) return null;
    let k: usize = 0;
    while (k < stem.len) {
        out.*[k] = if (stem[k] == '.') '-' else stem[k];
        k += 1;
    }
    return out.*[0..stem.len];
}

/// Computes a deterministic CSS-safe URL-base64 Wyhash digest (6 chars, or 7 chars with leading `_`
/// when `at_start == true` and the raw hash would start with a digit or `-<digit>`).
pub fn computeModuleHash(input: []const u8, at_start: bool, out: *[]u8): ([]const u8)? {
    const h64 = std.hash.Wyhash.hash(0, input);
    const h32 = (h64 & 0xFFFFFFFF) as u32;
    const b0 = (h32 & 0xFF) as u8;
    const b1 = ((h32 >> 8) & 0xFF) as u8;
    const b2 = ((h32 >> 16) & 0xFF) as u8;
    const b3 = ((h32 >> 24) & 0xFF) as u8;

    // 4 bytes -> 6 URL-safe base64 characters (no '=' padding)
    const raw: [6]u8 = [
        B64_URL_TABLE[(b0 >> 2) as usize],
        B64_URL_TABLE[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize],
        B64_URL_TABLE[(((b1 & 0x0F) << 2) | (b2 >> 6)) as usize],
        B64_URL_TABLE[(b2 & 0x3F) as usize],
        B64_URL_TABLE[(b3 >> 2) as usize],
        B64_URL_TABLE[((b3 & 0x03) << 4) as usize],
    ];

    const needs_prefix = at_start && (
        (raw[0] >= '0' && raw[0] <= '9') ||
        (raw[0] == '-' && raw[1] >= '0' && raw[1] <= '9')
    );
    const total_len: usize = if (needs_prefix) 7 else 6;
    if (out.*.len < total_len) return null;

    let offset: usize = 0;
    if (needs_prefix) {
        out.*[0] = '_';
        offset = 1;
    }
    let k: usize = 0;
    while (k < 6) {
        out.*[offset + k] = raw[k];
        k += 1;
    }
    return out.*[0..total_len];
}

/// One-shot zero-allocation CSS Modules class name scoping combining `computeModuleHash` and `pattern.format`.
pub fn scopeClassName(
    pattern: Pattern,
    file_path: []const u8,
    local: []const u8,
    out: *[]u8,
): ([]const u8)? {
    if (local.len == 0) return null;
    let hash_buf: [8]u8 = [0; 8];
    let hash_slice: []u8 = hash_buf[0..];
    if (h := computeModuleHash(file_path, pattern.startsWithHash(), *hash_slice)) {
        return pattern.format(h, file_path, local, out);
    }
    return null;
}

/// Parses a CSS Modules `composes:` property value into `out`, returning the number of parsed references.
/// Supports:
/// - local: `"btn primary"`
/// - global: `"reset-btn from global"`
/// - dependency: `"card shadow from './base.module.css'"`
/// Returns `null` on empty/malformed declaration or insufficient output buffer.
pub fn parseComposesValue(value: []const u8, out: *[]CssModuleRef): usize? {
    let i: usize = 0;
    let n_names: usize = 0;
    let found_from = false;

    while (i < value.len) {
        while (i < value.len && isSpace(value[i])) {
            i += 1;
        }
        if (i >= value.len) break;
        const start = i;
        while (i < value.len && !isSpace(value[i])) {
            i += 1;
        }
        const tok = value[start..i];
        if (std.mem.eql(u8, tok, "from")) {
            found_from = true;
            break;
        }
        if (!isValidCssIdent(tok)) return null;
        if (n_names >= out.*.len) return null;
        out.*[n_names] = {
            kind: ReferenceKind.local,
            name: tok,
            specifier: "",
        };
        n_names += 1;
    }

    if (n_names == 0) return null;

    if (found_from) {
        while (i < value.len && isSpace(value[i])) {
            i += 1;
        }
        if (i >= value.len) return null;

        let kind = ReferenceKind.dependency;
        let spec: []const u8 = "";

        if (value[i] == '\'' || value[i] == '"') {
            const quote = value[i];
            i += 1;
            const s_start = i;
            while (i < value.len && value[i] != quote) {
                i += 1;
            }
            if (i >= value.len) return null;
            spec = value[s_start..i];
            if (spec.len == 0) return null;
            i += 1;
            kind = ReferenceKind.dependency;
        } else {
            const s_start = i;
            while (i < value.len && !isSpace(value[i])) {
                i += 1;
            }
            const target = value[s_start..i];
            if (std.mem.eql(u8, target, "global")) {
                kind = ReferenceKind.global;
                spec = "";
            } else {
                return null;
            }
        }

        // Ensure no trailing garbage after `from <target>`
        while (i < value.len && isSpace(value[i])) {
            i += 1;
        }
        if (i != value.len) return null;

        let k: usize = 0;
        while (k < n_names) {
            out.*[k].kind = kind;
            out.*[k].specifier = spec;
            k += 1;
        }
    }

    return n_names;
}
