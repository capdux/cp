// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Pixar OpenUSD Crate .usdc & .usda Parser)
// Pure CP zero-heap-allocation Pixar OpenUSD container parser:
// .usdc Crate 88B Bootstrap + TOC section table, .usda 1.0 stage metadata header,
// and .usdz uncompressed ZIP primary asset detection.

const std = @import("std");

/// 8-byte Pixar OpenUSD Crate binary magic `"PXR-USDC"`.
pub const USDC_MAGIC: []const u8 = "PXR-USDC";

/// Fixed 88-byte Crate Bootstrap header size (8B magic + 8B version + 8B toc_offset + 64B reserved).
pub const USDC_BOOTSTRAP_SIZE: usize = 88;

/// Fixed 16-byte NUL-padded section name length in Crate TOC entries.
pub const USDC_SECTION_NAME_LEN: usize = 16;

/// Fixed 32-byte Crate TOC section entry size (16B name + 8B start + 8B size).
pub const USDC_SECTION_ENTRY_SIZE: usize = 32;

/// 6-byte ASCII OpenUSD header magic `"#usda "`.
pub const USDA_MAGIC: []const u8 = "#usda ";

/// Detected OpenUSD file container format.
pub enum UsdFormat {
    usdc,
    usda,
    usdz,
    unknown,
}

/// Parsed 88-byte OpenUSD Crate (`.usdc`) Bootstrap header + TOC section count.
pub struct UsdcBootstrap {
    version: [8]u8 = [0; 8],
    version_major: u8 = 0,
    version_minor: u8 = 0,
    version_patch: u8 = 0,
    toc_offset: u64 = 0,
    section_count: u64 = 0,
}

/// Zero-copy section entry decoded from a `.usdc` Crate TOC.
pub struct UsdcSection {
    name: []const u8 = "",
    start: u64 = 0,
    size: u64 = 0,
    payload: []const u8 = "",
}

/// Zero-copy stage-level metadata parsed from a `.usda` header (`#usda 1.0 (...)`).
pub struct UsdaHeader {
    version: []const u8 = "",
    default_prim: []const u8 = "",
    up_axis: []const u8 = "",
    meters_per_unit: f64 = 0.0,
    has_meters_per_unit: bool = false,
}

// ---------------------------------------------------------------------------
// Internal Little-Endian & Scanning Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = readU32Le(data, off + 4) as u64;
    return lo | (hi << 32);
}

fn asciiLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const base = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (asciiLower(s[base + i]) != asciiLower(suffix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripBom(data: []const u8): []const u8 {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return data[3..];
    }
    return data;
}

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isIdentChar(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_';
}

// ---------------------------------------------------------------------------
// 1. Format Detection: USDC / USDA / USDZ
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with the 8-byte `"PXR-USDC"` magic and has >= 88 bytes.
pub fn isUsdc(data: []const u8): bool {
    return data.len >= USDC_BOOTSTRAP_SIZE && std.mem.eql(u8, data[0..8], USDC_MAGIC);
}

/// Returns `true` if `data` (after optional UTF-8 BOM) starts with `"#usda "`.
pub fn isUsda(data: []const u8): bool {
    const s = stripBom(data);
    return s.len >= USDA_MAGIC.len && std.mem.eql(u8, s[0..USDA_MAGIC.len], USDA_MAGIC);
}

/// Returns `true` if `data` starts with a ZIP Local File Header (`PK\x03\x04`) whose first
/// entry is uncompressed (`compression_method == 0`) with a `.usd`/`.usdc`/`.usda` filename.
pub fn isUsdz(data: []const u8): bool {
    if (data.len < 30) {
        return false;
    }
    if (data[0] != 'P' || data[1] != 'K' || data[2] != 0x03 || data[3] != 0x04) {
        return false;
    }
    // USDZ spec requires uncompressed (Store = 0) archive entries
    const comp_method = readU16Le(data, 8);
    if (comp_method != 0) {
        return false;
    }
    const fname_len = readU16Le(data, 26) as usize;
    if (fname_len < 4 || fname_len > data.len - 30) {
        return false;
    }
    const fname = data[30 .. 30 + fname_len];
    return endsWithIgnoreCase(fname, ".usd") ||
        endsWithIgnoreCase(fname, ".usdc") ||
        endsWithIgnoreCase(fname, ".usda");
}

/// Detects the OpenUSD container format (`usdc`, `usda`, `usdz`, or `unknown`).
pub fn detectUsdFormat(data: []const u8): UsdFormat {
    if (isUsdc(data)) return UsdFormat.usdc;
    if (isUsda(data)) return UsdFormat.usda;
    if (isUsdz(data)) return UsdFormat.usdz;
    return UsdFormat.unknown;
}

// ---------------------------------------------------------------------------
// 2. .usdc Crate Bootstrap & TOC Section Table
// ---------------------------------------------------------------------------

/// Parses the 88-byte `.usdc` Crate Bootstrap header and validates TOC bounds (`toc_offset`
/// and `section_count`) against `data.len` without integer overflow.
pub fn parseUsdcBootstrap(data: []const u8): UsdcBootstrap? {
    if (!isUsdc(data)) {
        return null;
    }

    let boot = UsdcBootstrap {};
    let vi: usize = 0;
    while (vi < 8) {
        boot.version[vi] = data[8 + vi];
        vi += 1;
    }
    boot.version_major = boot.version[0];
    boot.version_minor = boot.version[1];
    boot.version_patch = boot.version[2];

    const toc_offset = readU64Le(data, 16);
    if (toc_offset < (USDC_BOOTSTRAP_SIZE as u64)) {
        return null;
    }
    const data_len_u64 = data.len as u64;
    if (toc_offset > data_len_u64 || (data_len_u64 - toc_offset) < 8) {
        return null;
    }

    const toc_off_usize = toc_offset as usize;
    const section_count = readU64Le(data, toc_off_usize);
    const bytes_after_count = data.len - toc_off_usize - 8;
    const max_sections: u64 = (bytes_after_count / USDC_SECTION_ENTRY_SIZE) as u64;
    if (section_count > max_sections) {
        return null;
    }

    boot.toc_offset = toc_offset;
    boot.section_count = section_count;
    return boot;
}

/// Zero-allocation iterator over 32-byte TOC section entries in a `.usdc` Crate file.
pub struct UsdcSectionIter {
    data: []const u8 = "",
    entry_offset: usize = 0,
    remaining: u64 = 0,
    done: bool = true,

    fn init(data: []const u8, boot: UsdcBootstrap): UsdcSectionIter {
        const data_len_u64 = data.len as u64;
        if (boot.toc_offset < (USDC_BOOTSTRAP_SIZE as u64) ||
            boot.toc_offset > data_len_u64 ||
            (data_len_u64 - boot.toc_offset) < 8)
        {
            return UsdcSectionIter { data: data, entry_offset: 0, remaining: 0, done: true };
        }
        const entries_start = (boot.toc_offset as usize) + 8;
        const max_sections: u64 = ((data.len - entries_start) / USDC_SECTION_ENTRY_SIZE) as u64;
        if (boot.section_count > max_sections) {
            return UsdcSectionIter { data: data, entry_offset: 0, remaining: 0, done: true };
        }
        return UsdcSectionIter {
            data: data,
            entry_offset: entries_start,
            remaining: boot.section_count,
            done: boot.section_count == 0,
        };
    }

    /// Decodes and returns the next `UsdcSection` with zero-copy `payload`, or `null` at end / out-of-bounds.
    fn next(*_): UsdcSection? {
        if (_.done || _.remaining == 0) {
            _.done = true;
            return null;
        }
        if (_.entry_offset + USDC_SECTION_ENTRY_SIZE > _.data.len) {
            _.done = true;
            return null;
        }

        const off = _.entry_offset;
        let name_len: usize = 0;
        while (name_len < USDC_SECTION_NAME_LEN && _.data[off + name_len] != 0) {
            name_len += 1;
        }
        const name = _.data[off .. off + name_len];
        const start = readU64Le(_.data, off + 16);
        const size = readU64Le(_.data, off + 24);

        const data_len_u64 = _.data.len as u64;
        if (start > data_len_u64 || size > (data_len_u64 - start)) {
            _.done = true;
            return null;
        }

        const start_u = start as usize;
        const size_u = size as usize;
        _.entry_offset += USDC_SECTION_ENTRY_SIZE;
        _.remaining -= 1;
        if (_.remaining == 0) {
            _.done = true;
        }

        return UsdcSection {
            name: name,
            start: start,
            size: size,
            payload: _.data[start_u .. start_u + size_u],
        };
    }
}

/// Looks up a named `.usdc` TOC section (e.g. `"TOKENS"`, `"STRINGS"`, `"FIELDS"`, `"PATHS"`, `"SPECS"`).
pub fn findUsdcSection(data: []const u8, boot: UsdcBootstrap, name: []const u8): UsdcSection? {
    let it = UsdcSectionIter.init(data, boot);
    while (sec := it.next()) {
        if (std.mem.eql(u8, sec.name, name)) {
            return sec;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// 3. .usda Text Header & Stage Metadata Parser
// ---------------------------------------------------------------------------

fn skipUsdaWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isWs(s[i])) {
            i += 1;
            continue;
        }
        if (s[i] == '#') {
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
            continue;
        }
        break;
    }
    return i;
}

fn scanUsdaString(s: []const u8, start: usize): usize? {
    if (start >= s.len || s[start] != '"') {
        return null;
    }
    // Triple-quoted string """..."""
    if (start + 3 <= s.len && s[start + 1] == '"' && s[start + 2] == '"') {
        let i = start + 3;
        while (i + 2 < s.len) {
            if (s[i] == '"' && s[i + 1] == '"' && s[i + 2] == '"') {
                return i + 3;
            }
            i += 1;
        }
        return null;
    }
    let i = start + 1;
    while (i < s.len) {
        if (s[i] == '\\') {
            i += 2;
            if (i > s.len) return null;
            continue;
        }
        if (s[i] == '"') {
            return i + 1;
        }
        i += 1;
    }
    return null;
}

/// Skips a single USDA metadata value expression (string, number, identifier, or nested {...}/[...]/(...)).
fn skipUsdaMetaValue(s: []const u8, start: usize): usize? {
    let i = skipUsdaWsAndComments(s, start);
    if (i >= s.len) {
        return null;
    }
    const b = s[i];
    if (b == '"') {
        return scanUsdaString(s, i);
    }
    if (b == '{' || b == '[' || b == '(') {
        let depth: usize = 1;
        i += 1;
        while (i < s.len && depth > 0) {
            const c = s[i];
            if (c == '"') {
                i = scanUsdaString(s, i) ?? return null;
                continue;
            }
            if (c == '#') {
                while (i < s.len && s[i] != '\n') {
                    i += 1;
                }
                continue;
            }
            if (c == '{' || c == '[' || c == '(') {
                depth += 1;
            } else if (c == '}' || c == ']' || c == ')') {
                depth -= 1;
            }
            i += 1;
        }
        if (depth != 0) {
            return null;
        }
        return i;
    }
    // Scalar token (number / identifier / @asset@)
    while (i < s.len && !isWs(s[i]) && s[i] != ')' && s[i] != ',' && s[i] != ';' && s[i] != '#') {
        i += 1;
    }
    return i;
}

/// Parses a `.usda` text header (`#usda 1.0` + optional stage-level `(...)` metadata block)
/// extracting `version`, `default_prim`, `up_axis`, and `meters_per_unit`.
pub fn parseUsdaHeader(text: []const u8): UsdaHeader? {
    const s = stripBom(text);
    if (!isUsda(s)) {
        return null;
    }
    let i: usize = USDA_MAGIC.len;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t')) {
        i += 1;
    }
    const ver_start = i;
    while (i < s.len && !isWs(s[i]) && s[i] != '(' && s[i] != '#') {
        i += 1;
    }
    if (i == ver_start) {
        return null;
    }

    let hdr = UsdaHeader {
        version: s[ver_start..i],
    };

    i = skipUsdaWsAndComments(s, i);
    if (i >= s.len || s[i] != '(') {
        return hdr;
    }
    i += 1;

    while (i < s.len) {
        i = skipUsdaWsAndComments(s, i);
        if (i >= s.len) {
            return null;
        }
        if (s[i] == ')') {
            return hdr;
        }
        // Skip optional standalone doc string inside (...)
        if (s[i] == '"') {
            i = scanUsdaString(s, i) ?? return null;
            continue;
        }

        const key_start = i;
        while (i < s.len && isIdentChar(s[i])) {
            i += 1;
        }
        if (i == key_start) {
            return null;
        }
        const key = s[key_start..i];

        i = skipUsdaWsAndComments(s, i);
        if (i >= s.len || s[i] != '=') {
            return null;
        }
        const val_start = skipUsdaWsAndComments(s, i + 1);
        const val_end = skipUsdaMetaValue(s, val_start) ?? return null;
        const val_slice = s[val_start..val_end];

        if (std.mem.eql(u8, key, "defaultPrim")) {
            if (val_slice.len >= 2 && val_slice[0] == '"' && val_slice[val_slice.len - 1] == '"') {
                hdr.default_prim = val_slice[1 .. val_slice.len - 1];
            }
        } else if (std.mem.eql(u8, key, "upAxis")) {
            if (val_slice.len >= 2 && val_slice[0] == '"' && val_slice[val_slice.len - 1] == '"') {
                hdr.up_axis = val_slice[1 .. val_slice.len - 1];
            }
        } else if (std.mem.eql(u8, key, "metersPerUnit")) {
            const parsed = std.fmt.parseFloat(f64, val_slice) catch return null;
            if (!std.math.isFinite(parsed)) {
                return null;
            }
            hdr.meters_per_unit = parsed;
            hdr.has_meters_per_unit = true;
        }

        i = val_end;
    }

    return null;
}
