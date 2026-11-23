// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateCompileResultForJSChunk.cp | Synced: 2026-11-22T10:58:57Z
// ============================================================================
// lib/bundler/jsChunkResult.cp — JS Chunk Finalizer, SHA-256 SRI & Debug ID Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven final JS chunk packaging engine:
//   1. FIPS 180-4 SHA-256 + Base64 SRI (`sha256-...`) & deterministic RFC 4122
//      `debugId` UUID computation (`computeJsChunkIntegrity`) with zero heap
//      allocation for HTML `<script integrity="...">` and Asset Manifests;
//   2. `cap:simd`-accelerated Shebang extraction (`splitLeadingShebang`) and
//      deterministic sorted/deduplicated cross-chunk import header emitter
//      (`emitSortedCrossChunkImports`) supporting ESM `import` & CJS `require`;
//   3. Complete JS chunk finalizer (`finalizeJsChunkResult`) supporting `Esm`,
//      `Cjs`, and `Iife` formats with Shebang, Banner/Footer, export tails,
//      `//# debugId=<uuid>`, `//# sourceMappingURL=...`, and `Simd.countByte`
//      line tracking.

import { Simd } from "cap:simd";

/// Default executable Shebang line for standalone Cap CLI bundles.
pub const JS_CHUNK_DEFAULT_SHEBANG: []const u8 = "#!/usr/bin/env cap";

/// Prefix for TC39 / Sentry JS Debug ID trailer comments.
pub const JS_CHUNK_DEBUG_ID_PREFIX: []const u8 = "//# debugId=";

/// Prefix for JS SourceMap V3 trailer comments.
pub const JS_CHUNK_SOURCEMAP_PREFIX: []const u8 = "//# sourceMappingURL=";

/// Subresource Integrity algorithm prefix (`sha256-`).
pub const JS_CHUNK_SRI_PREFIX: []const u8 = "sha256-";

/// Exact byte length of a `sha256-<44-char-base64>` SRI digest string.
pub const JS_CHUNK_SRI_LEN: usize = 51;

/// Exact byte length of a canonical UUID string (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
pub const JS_CHUNK_UUID_LEN: usize = 36;

/// Error set for JS chunk finalization, import remapping, and formatting.
pub err JsChunkResultError {
    BufferTooSmall,
    CapacityExceeded,
    InvalidShebang,
    InvalidImportSpecifier,
    InvalidExportBinding,
}

// ============================================================================
// Internal Zero-Alloc Byte, Comparison, Base64, and SHA-256 Helpers
// ============================================================================

const JCR_HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

const JCR_B64_TABLE: [64]u8 = [
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
];

const SHA256_K: [64]u32 = [
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
];

fn jcrRotr32(x: u32, n: u5): u32 {
    return (x >> n) | (x << (@as(u5, 0) -% n));
}

fn jcrReadU32Be(block: []const u8, off: usize): u32 {
    return (@as(u32, block[off]) << 24) |
        (@as(u32, block[off + 1]) << 16) |
        (@as(u32, block[off + 2]) << 8) |
        @as(u32, block[off + 3]);
}

fn jcrSha256Compress(state: *[8]u32, block: []const u8) {
    let w: [64]u32 = [0; 64];
    let t: usize = 0;
    while (t < 16) {
        w[t] = jcrReadU32Be(block, t * 4);
        t += 1;
    }
    while (t < 64) {
        const s0 = jcrRotr32(w[t - 15], 7) ^ jcrRotr32(w[t - 15], 18) ^ (w[t - 15] >> 3);
        const s1 = jcrRotr32(w[t - 2], 17) ^ jcrRotr32(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] +% s0 +% w[t - 7] +% s1;
        t += 1;
    }

    let a = state.*[0];
    let b = state.*[1];
    let c = state.*[2];
    let d = state.*[3];
    let e = state.*[4];
    let f = state.*[5];
    let g = state.*[6];
    let h = state.*[7];

    let r: usize = 0;
    while (r < 64) {
        const s1 = jcrRotr32(e, 6) ^ jcrRotr32(e, 11) ^ jcrRotr32(e, 25);
        const ch = (e & f) ^ ((~e) & g);
        const temp1 = h +% s1 +% ch +% SHA256_K[r] +% w[r];
        const s0 = jcrRotr32(a, 2) ^ jcrRotr32(a, 13) ^ jcrRotr32(a, 22);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = s0 +% maj;

        h = g;
        g = f;
        f = e;
        e = d +% temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 +% temp2;
        r += 1;
    }

    state.*[0] +%= a;
    state.*[1] +%= b;
    state.*[2] +%= c;
    state.*[3] +%= d;
    state.*[4] +%= e;
    state.*[5] +%= f;
    state.*[6] +%= g;
    state.*[7] +%= h;
}

/// Computes a FIPS 180-4 SHA-256 digest over `data` with zero heap allocation.
pub fn computeJsChunkSha256(data: []const u8): [32]u8 {
    let state: [8]u32 = [
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
    ];

    let off: usize = 0;
    while (off + 64 <= data.len) {
        jcrSha256Compress(&state, data[off..off + 64]);
        off += 64;
    }

    let tail: [128]u8 = [0; 128];
    const rem = data.len - off;
    let k: usize = 0;
    while (k < rem) {
        tail[k] = data[off + k];
        k += 1;
    }
    tail[rem] = 0x80;

    const total_blocks: usize = if (rem + 1 + 8 <= 64) 1 else 2;
    const bit_len: u64 = @as(u64, data.len) * 8;
    const len_pos = total_blocks * 64 - 8;
    tail[len_pos + 0] = @as(u8, @truncate(bit_len >> 56));
    tail[len_pos + 1] = @as(u8, @truncate(bit_len >> 48));
    tail[len_pos + 2] = @as(u8, @truncate(bit_len >> 40));
    tail[len_pos + 3] = @as(u8, @truncate(bit_len >> 32));
    tail[len_pos + 4] = @as(u8, @truncate(bit_len >> 24));
    tail[len_pos + 5] = @as(u8, @truncate(bit_len >> 16));
    tail[len_pos + 6] = @as(u8, @truncate(bit_len >> 8));
    tail[len_pos + 7] = @as(u8, @truncate(bit_len));

    jcrSha256Compress(&state, tail[0..64]);
    if (total_blocks == 2) {
        jcrSha256Compress(&state, tail[64..128]);
    }

    let digest: [32]u8 = [0; 32];
    let w_idx: usize = 0;
    while (w_idx < 8) {
        const v = state[w_idx];
        digest[w_idx * 4 + 0] = @as(u8, @truncate(v >> 24));
        digest[w_idx * 4 + 1] = @as(u8, @truncate(v >> 16));
        digest[w_idx * 4 + 2] = @as(u8, @truncate(v >> 8));
        digest[w_idx * 4 + 3] = @as(u8, @truncate(v));
        w_idx += 1;
    }
    return digest;
}

fn jcrBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn jcrLexLess(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn jcrAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn jcrAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn jcrAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        jcrAppendByte(out, pos, '0')?;
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
        jcrAppendByte(out, pos, tmp[n])?;
    }
}

fn jcrEncodeBase6432(src: [32]u8, dst: []u8) {
    let si: usize = 0;
    let di: usize = 0;
    // 10 full 3-byte triplets -> 40 chars
    while (si < 30) {
        const n: u32 = (@as(u32, src[si]) << 16) | (@as(u32, src[si + 1]) << 8) | @as(u32, src[si + 2]);
        dst[di + 0] = JCR_B64_TABLE[@as(usize, (n >> 18) & 0x3F)];
        dst[di + 1] = JCR_B64_TABLE[@as(usize, (n >> 12) & 0x3F)];
        dst[di + 2] = JCR_B64_TABLE[@as(usize, (n >> 6) & 0x3F)];
        dst[di + 3] = JCR_B64_TABLE[@as(usize, n & 0x3F)];
        si += 3;
        di += 4;
    }
    // Final 2 bytes (src[30], src[31]) -> 3 chars + '='
    const rem: u32 = (@as(u32, src[30]) << 16) | (@as(u32, src[31]) << 8);
    dst[di + 0] = JCR_B64_TABLE[@as(usize, (rem >> 18) & 0x3F)];
    dst[di + 1] = JCR_B64_TABLE[@as(usize, (rem >> 12) & 0x3F)];
    dst[di + 2] = JCR_B64_TABLE[@as(usize, (rem >> 6) & 0x3F)];
    dst[di + 3] = '=';
}

fn jcrFormatUuidFromDigest(digest: [32]u8, out: []u8) {
    let b: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        b[i] = digest[i];
        i += 1;
    }
    // Set RFC 4122 version 4 & variant 1 bits for standard debugId UUID shape
    b[6] = (b[6] & 0x0F) | 0x40;
    b[8] = (b[8] & 0x3F) | 0x80;

    let pos: usize = 0;
    let idx: usize = 0;
    while (idx < 16) {
        if (idx == 4 || idx == 6 || idx == 8 || idx == 10) {
            out[pos] = '-';
            pos += 1;
        }
        out[pos] = JCR_HEX_LOWER[@as(usize, b[idx] >> 4)];
        out[pos + 1] = JCR_HEX_LOWER[@as(usize, b[idx] & 0xF)];
        pos += 2;
        idx += 1;
    }
}

// ============================================================================
// Section 1: Content Fingerprint, SRI (`sha256-...`) & Deterministic Debug ID
// ============================================================================

/// Zero-heap content fingerprint, Subresource Integrity (`sha256-...`), and
/// deterministic RFC 4122 `debugId` derived from a finalized JS chunk.
pub struct JsChunkIntegrity {
    hash64: u64 = 0,
    sha256: [32]u8 = [0; 32],
    sri_buf: [51]u8 = [0; 51],
    debug_id_buf: [36]u8 = [0; 36],

    static fn empty(): JsChunkIntegrity {
        return JsChunkIntegrity{};
    }

    /// Returns the 51-byte `sha256-<base64>` SRI digest string.
    fn sri(): []const u8 {
        return _.sri_buf[0..JS_CHUNK_SRI_LEN];
    }

    /// Returns the 36-byte deterministic UUID string for `//# debugId=`.
    fn debugId(): []const u8 {
        return _.debug_id_buf[0..JS_CHUNK_UUID_LEN];
    }

    /// Formats the 64-bit content hash as 16 lowercase hexadecimal characters.
    fn formatShortHex(out: []u8): JsChunkResultError!usize {
        if (out.len < 16) return err.BufferTooSmall;
        let i: usize = 0;
        while (i < 16) {
            const shift: u6 = @as(u6, @intCast(60 - i * 4));
            out[i] = JCR_HEX_LOWER[@as(usize, @intCast((_.hash64 >> shift) & 0xF))];
            i += 1;
        }
        return 16;
    }
}

/// Computes deterministic 64-bit hash, 32-byte SHA-256, `sha256-...` SRI string,
/// and RFC 4122 `debugId` UUID over `code`.
pub fn computeJsChunkIntegrity(code: []const u8): JsChunkIntegrity {
    const digest = computeJsChunkSha256(code);
    const h64: u64 = (@as(u64, digest[0]) << 56) |
        (@as(u64, digest[1]) << 48) |
        (@as(u64, digest[2]) << 40) |
        (@as(u64, digest[3]) << 32) |
        (@as(u64, digest[4]) << 24) |
        (@as(u64, digest[5]) << 16) |
        (@as(u64, digest[6]) << 8) |
        @as(u64, digest[7]);

    let sri_buf: [51]u8 = [0; 51];
    let p: usize = 0;
    while (p < JS_CHUNK_SRI_PREFIX.len) {
        sri_buf[p] = JS_CHUNK_SRI_PREFIX[p];
        p += 1;
    }
    jcrEncodeBase6432(digest, sri_buf[7..51]);

    let uuid_buf: [36]u8 = [0; 36];
    jcrFormatUuidFromDigest(digest, uuid_buf[0..]);

    return JsChunkIntegrity{
        hash64: h64,
        sha256: digest,
        sri_buf: sri_buf,
        debug_id_buf: uuid_buf,
    };
}

// ============================================================================
// Section 2: SIMD Shebang Splitter & Cross-Chunk Import Header Emitter
// ============================================================================

/// Output module format for finalized JS chunks.
pub enum JsChunkFinalFormat {
    Esm,
    Cjs,
    Iife,
}

/// Returns the canonical ASCII name of a `JsChunkFinalFormat`.
pub fn jsChunkFinalFormatName(fmt: JsChunkFinalFormat): []const u8 {
    return match (fmt) {
        JsChunkFinalFormat.Esm => "esm",
        JsChunkFinalFormat.Cjs => "cjs",
        JsChunkFinalFormat.Iife => "iife",
    };
}

/// Result of splitting a leading `#!...` Shebang from a JS source buffer.
pub struct JsChunkShebangSplit {
    shebang: []const u8 = "",
    body: []const u8 = "",
}

/// Extracts a leading `#!...` Shebang line using `cap:simd` (`Simd.indexOfByte`),
/// stripping trailing `\r` / `\n` from the Shebang slice.
pub fn splitLeadingShebang(code: []const u8): JsChunkShebangSplit {
    if (code.len >= 2 && code[0] == '#' && code[1] == '!') {
        if (nl := Simd.indexOfByte(code, '\n')) {
            const line_end = if (nl > 0 && code[nl - 1] == '\r') nl - 1 else nl;
            return JsChunkShebangSplit{
                shebang: code[0..line_end],
                body: code[nl + 1..],
            };
        }
        return JsChunkShebangSplit{
            shebang: code,
            body: "",
        };
    }
    return JsChunkShebangSplit{
        shebang: "",
        body: code,
    };
}

/// Cross-chunk import binding emitted at the top of a finalized JS chunk.
/// - `imported_name == ""`: side-effect import (`import "./chunk.js";`)
/// - `imported_name == "*"`: namespace import (`import * as local from "./chunk.js";`)
/// - `imported_name == "default"`: default import (`import local from "./chunk.js";`)
/// - otherwise: named import (`import { imported as local } from "./chunk.js";`)
pub struct JsChunkImportBinding {
    from_path: []const u8 = "",
    imported_name: []const u8 = "",
    local_name: []const u8 = "",
}

/// Export binding emitted at the tail of a finalized JS chunk.
pub struct JsChunkExportBinding {
    local_name: []const u8 = "",
    exported_name: []const u8 = "",
}

fn jcrImportBefore(a: JsChunkImportBinding, b: JsChunkImportBinding): bool {
    if (!jcrBytesEql(a.from_path, b.from_path)) return jcrLexLess(a.from_path, b.from_path);
    if (!jcrBytesEql(a.imported_name, b.imported_name)) return jcrLexLess(a.imported_name, b.imported_name);
    return jcrLexLess(a.local_name, b.local_name);
}

fn jcrImportEql(a: JsChunkImportBinding, b: JsChunkImportBinding): bool {
    return jcrBytesEql(a.from_path, b.from_path) &&
        jcrBytesEql(a.imported_name, b.imported_name) &&
        jcrBytesEql(a.local_name, b.local_name);
}

fn jcrIsNamedImport(imp: JsChunkImportBinding): bool {
    return imp.imported_name.len > 0 &&
        !jcrBytesEql(imp.imported_name, "*") &&
        !jcrBytesEql(imp.imported_name, "default");
}

/// Sorts, deduplicates, and groups cross-chunk imports into canonical ESM `import`
/// or CJS `require` statements. `scratch_order` must have length >= `imports.len`.
pub fn emitSortedCrossChunkImports(
    imports: []const JsChunkImportBinding,
    format: JsChunkFinalFormat,
    scratch_order: []usize,
    out: []u8,
    emitted_count: *u32,
): JsChunkResultError!usize {
    emitted_count.* = 0;
    if (imports.len == 0 || format == JsChunkFinalFormat.Iife) return 0;
    if (scratch_order.len < imports.len) return err.BufferTooSmall;

    let idx: usize = 0;
    while (idx < imports.len) {
        if (imports[idx].from_path.len == 0 || Simd.indexOfAny2(imports[idx].from_path, '\n', '\r') != null) {
            return err.InvalidImportSpecifier;
        }
        scratch_order[idx] = idx;
        idx += 1;
    }

    // Deterministic insertion sort by (from_path, imported_name, local_name)
    let s: usize = 1;
    while (s < imports.len) {
        const key_i = scratch_order[s];
        const key_imp = imports[key_i];
        let j = s;
        while (j > 0 && jcrImportBefore(key_imp, imports[scratch_order[j - 1]])) {
            scratch_order[j] = scratch_order[j - 1];
            j -= 1;
        }
        scratch_order[j] = key_i;
        s += 1;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < imports.len) {
        const cur = imports[scratch_order[i]];
        // Skip exact duplicates
        if (i > 0 && jcrImportEql(cur, imports[scratch_order[i - 1]])) {
            i += 1;
            continue;
        }

        if (format == JsChunkFinalFormat.Esm) {
            if (cur.imported_name.len == 0) {
                jcrAppendBytes(out, &pos, "import \"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\";\n")?;
                emitted_count.* += 1;
                i += 1;
            } else if (jcrBytesEql(cur.imported_name, "*")) {
                jcrAppendBytes(out, &pos, "import * as ")?;
                jcrAppendBytes(out, &pos, cur.local_name)?;
                jcrAppendBytes(out, &pos, " from \"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\";\n")?;
                emitted_count.* += 1;
                i += 1;
            } else if (jcrBytesEql(cur.imported_name, "default")) {
                jcrAppendBytes(out, &pos, "import ")?;
                jcrAppendBytes(out, &pos, cur.local_name)?;
                jcrAppendBytes(out, &pos, " from \"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\";\n")?;
                emitted_count.* += 1;
                i += 1;
            } else {
                // Group consecutive named imports from the same `from_path`
                jcrAppendBytes(out, &pos, "import { ")?;
                let first = true;
                while (i < imports.len) {
                    const item = imports[scratch_order[i]];
                    if (!jcrBytesEql(item.from_path, cur.from_path) || !jcrIsNamedImport(item)) break;
                    if (i > 0 && jcrImportEql(item, imports[scratch_order[i - 1]])) {
                        i += 1;
                        continue;
                    }
                    if (!first) jcrAppendBytes(out, &pos, ", ")?;
                    jcrAppendBytes(out, &pos, item.imported_name)?;
                    if (item.local_name.len > 0 && !jcrBytesEql(item.local_name, item.imported_name)) {
                        jcrAppendBytes(out, &pos, " as ")?;
                        jcrAppendBytes(out, &pos, item.local_name)?;
                    }
                    first = false;
                    emitted_count.* += 1;
                    i += 1;
                }
                jcrAppendBytes(out, &pos, " } from \"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\";\n")?;
            }
        } else {
            // CommonJS require() emission
            if (cur.imported_name.len == 0) {
                jcrAppendBytes(out, &pos, "require(\"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\");\n")?;
                emitted_count.* += 1;
                i += 1;
            } else if (jcrBytesEql(cur.imported_name, "*") || jcrBytesEql(cur.imported_name, "default")) {
                jcrAppendBytes(out, &pos, "const ")?;
                jcrAppendBytes(out, &pos, cur.local_name)?;
                jcrAppendBytes(out, &pos, " = require(\"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\");\n")?;
                emitted_count.* += 1;
                i += 1;
            } else {
                jcrAppendBytes(out, &pos, "const { ")?;
                let first = true;
                while (i < imports.len) {
                    const item = imports[scratch_order[i]];
                    if (!jcrBytesEql(item.from_path, cur.from_path) || !jcrIsNamedImport(item)) break;
                    if (i > 0 && jcrImportEql(item, imports[scratch_order[i - 1]])) {
                        i += 1;
                        continue;
                    }
                    if (!first) jcrAppendBytes(out, &pos, ", ")?;
                    jcrAppendBytes(out, &pos, item.imported_name)?;
                    if (item.local_name.len > 0 && !jcrBytesEql(item.local_name, item.imported_name)) {
                        jcrAppendBytes(out, &pos, ": ")?;
                        jcrAppendBytes(out, &pos, item.local_name)?;
                    }
                    first = false;
                    emitted_count.* += 1;
                    i += 1;
                }
                jcrAppendBytes(out, &pos, " } = require(\"")?;
                jcrAppendBytes(out, &pos, cur.from_path)?;
                jcrAppendBytes(out, &pos, "\");\n")?;
            }
        }
    }

    return pos;
}

// ============================================================================
// Section 3: Final JS Chunk Packaging Engine (`finalizeJsChunkResult`)
// ============================================================================

/// Options governing `finalizeJsChunkResult`.
pub struct JsChunkFinalizeOptions {
    format: JsChunkFinalFormat = JsChunkFinalFormat.Esm,
    shebang: []const u8 = "",
    banner: []const u8 = "",
    footer: []const u8 = "",
    iife_global_name: []const u8 = "",
    emit_debug_id: bool = true,
    source_map_url: []const u8 = "",
    emit_use_strict: bool = false,

    static fn defaults(): JsChunkFinalizeOptions {
        return JsChunkFinalizeOptions{};
    }
}

/// Summary and integrity metadata returned by `finalizeJsChunkResult`.
pub struct JsChunkFinalSummary {
    format: JsChunkFinalFormat = JsChunkFinalFormat.Esm,
    output_bytes: usize = 0,
    total_lines: u32 = 0,
    imports_emitted: u32 = 0,
    exports_emitted: u32 = 0,
    has_shebang: bool = false,
    has_debug_id: bool = false,
    has_source_map_url: bool = false,
    integrity: JsChunkIntegrity = JsChunkIntegrity.empty(),
}

fn jcrEmitExports(
    exports: []const JsChunkExportBinding,
    format: JsChunkFinalFormat,
    out: []u8,
    pos: *usize,
    emitted_count: *u32,
)! {
    emitted_count.* = 0;
    if (exports.len == 0) return;

    if (format == JsChunkFinalFormat.Esm) {
        let has_named = false;
        let i: usize = 0;
        while (i < exports.len) {
            const ex = exports[i];
            if (ex.local_name.len == 0 || ex.exported_name.len == 0) return err.InvalidExportBinding;
            if (jcrBytesEql(ex.exported_name, "default")) {
                jcrAppendBytes(out, pos, "export default ")?;
                jcrAppendBytes(out, pos, ex.local_name)?;
                jcrAppendBytes(out, pos, ";\n")?;
                emitted_count.* += 1;
            } else {
                has_named = true;
            }
            i += 1;
        }
        if (has_named) {
            jcrAppendBytes(out, pos, "export { ")?;
            let first = true;
            let k: usize = 0;
            while (k < exports.len) {
                const ex = exports[k];
                if (!jcrBytesEql(ex.exported_name, "default")) {
                    if (!first) jcrAppendBytes(out, pos, ", ")?;
                    jcrAppendBytes(out, pos, ex.local_name)?;
                    if (!jcrBytesEql(ex.local_name, ex.exported_name)) {
                        jcrAppendBytes(out, pos, " as ")?;
                        jcrAppendBytes(out, pos, ex.exported_name)?;
                    }
                    first = false;
                    emitted_count.* += 1;
                }
                k += 1;
            }
            jcrAppendBytes(out, pos, " };\n")?;
        }
    } else if (format == JsChunkFinalFormat.Cjs) {
        jcrAppendBytes(out, pos, "module.exports = { ")?;
        let i: usize = 0;
        while (i < exports.len) {
            const ex = exports[i];
            if (ex.local_name.len == 0 || ex.exported_name.len == 0) return err.InvalidExportBinding;
            if (i > 0) jcrAppendBytes(out, pos, ", ")?;
            jcrAppendBytes(out, pos, ex.exported_name)?;
            if (!jcrBytesEql(ex.local_name, ex.exported_name)) {
                jcrAppendBytes(out, pos, ": ")?;
                jcrAppendBytes(out, pos, ex.local_name)?;
            }
            emitted_count.* += 1;
            i += 1;
        }
        jcrAppendBytes(out, pos, " };\n")?;
    } else {
        // IIFE return object
        jcrAppendBytes(out, pos, "return { ")?;
        let i: usize = 0;
        while (i < exports.len) {
            const ex = exports[i];
            if (ex.local_name.len == 0 || ex.exported_name.len == 0) return err.InvalidExportBinding;
            if (i > 0) jcrAppendBytes(out, pos, ", ")?;
            jcrAppendBytes(out, pos, ex.exported_name)?;
            if (!jcrBytesEql(ex.local_name, ex.exported_name)) {
                jcrAppendBytes(out, pos, ": ")?;
                jcrAppendBytes(out, pos, ex.local_name)?;
            }
            emitted_count.* += 1;
            i += 1;
        }
        jcrAppendBytes(out, pos, " };\n")?;
    }
}

/// Packages a finalized JS chunk into `out`, synthesizing Shebang, Banner,
/// sorted/deduplicated cross-chunk imports, body, export tail, IIFE wrapper,
/// Footer, deterministic `//# debugId=<uuid>`, `//# sourceMappingURL=...`,
/// and final SHA-256 SRI integrity metadata.
pub fn finalizeJsChunkResult(
    body_code: []const u8,
    imports: []const JsChunkImportBinding,
    exports: []const JsChunkExportBinding,
    opts: JsChunkFinalizeOptions,
    scratch_order: []usize,
    out: []u8,
): JsChunkResultError!JsChunkFinalSummary {
    let summary = JsChunkFinalSummary{ format: opts.format };
    let pos: usize = 0;

    // 1. Shebang (explicit option takes priority over body-embedded Shebang)
    const split = splitLeadingShebang(body_code);
    const chosen_shebang = if (opts.shebang.len > 0) opts.shebang else split.shebang;
    if (chosen_shebang.len > 0) {
        if (chosen_shebang.len < 2 || chosen_shebang[0] != '#' || chosen_shebang[1] != '!') {
            return err.InvalidShebang;
        }
        if (Simd.indexOfAny2(chosen_shebang, '\n', '\r') != null) {
            return err.InvalidShebang;
        }
        jcrAppendBytes(out, &pos, chosen_shebang)?;
        jcrAppendByte(out, &pos, '\n')?;
        summary.has_shebang = true;
    }

    // 2. Banner
    if (opts.banner.len > 0) {
        jcrAppendBytes(out, &pos, opts.banner)?;
        if (opts.banner[opts.banner.len - 1] != '\n') jcrAppendByte(out, &pos, '\n')?;
    }

    // 3. Optional "use strict";
    if (opts.emit_use_strict) {
        jcrAppendBytes(out, &pos, "\"use strict\";\n")?;
    }

    // 4. Optional IIFE opening wrapper
    if (opts.format == JsChunkFinalFormat.Iife) {
        if (opts.iife_global_name.len > 0) {
            jcrAppendBytes(out, &pos, "var ")?;
            jcrAppendBytes(out, &pos, opts.iife_global_name)?;
            jcrAppendBytes(out, &pos, " = (() => {\n")?;
        } else {
            jcrAppendBytes(out, &pos, "(() => {\n")?;
        }
    }

    // 5. Sorted & deduplicated cross-chunk imports
    let imp_count: u32 = 0;
    const imp_len = emitSortedCrossChunkImports(imports, opts.format, scratch_order, out[pos..], &imp_count)?;
    pos += imp_len;
    summary.imports_emitted = imp_count;

    // 6. Module body
    if (split.body.len > 0) {
        jcrAppendBytes(out, &pos, split.body)?;
        if (out[pos - 1] != '\n') jcrAppendByte(out, &pos, '\n')?;
    }

    // 7. Export tail
    let exp_count: u32 = 0;
    jcrEmitExports(exports, opts.format, out, &pos, &exp_count)?;
    summary.exports_emitted = exp_count;

    // 8. Optional IIFE closing wrapper
    if (opts.format == JsChunkFinalFormat.Iife) {
        jcrAppendBytes(out, &pos, "})();\n")?;
    }

    // 9. Footer
    if (opts.footer.len > 0) {
        jcrAppendBytes(out, &pos, opts.footer)?;
        if (opts.footer[opts.footer.len - 1] != '\n') jcrAppendByte(out, &pos, '\n')?;
    }

    // 10. Deterministic Debug ID derived from pre-trailer chunk bytes
    if (opts.emit_debug_id) {
        const pre_integrity = computeJsChunkIntegrity(out[0..pos]);
        jcrAppendBytes(out, &pos, JS_CHUNK_DEBUG_ID_PREFIX)?;
        jcrAppendBytes(out, &pos, pre_integrity.debugId())?;
        jcrAppendByte(out, &pos, '\n')?;
        summary.has_debug_id = true;
    }

    // 11. SourceMap trailer comment
    if (opts.source_map_url.len > 0) {
        jcrAppendBytes(out, &pos, JS_CHUNK_SOURCEMAP_PREFIX)?;
        jcrAppendBytes(out, &pos, opts.source_map_url)?;
        jcrAppendByte(out, &pos, '\n')?;
        summary.has_source_map_url = true;
    }

    // 12. Compute final SRI integrity & SIMD line count over exact emitted bytes
    summary.output_bytes = pos;
    summary.total_lines = @as(u32, @intCast(Simd.countByte(out[0..pos], '\n')));
    summary.integrity = computeJsChunkIntegrity(out[0..pos]);
    return summary;
}

// ============================================================================
// Section 4: Diagnostic Summary Report Formatter
// ============================================================================

/// Formats a concise diagnostic summary for finalized JS chunk telemetry.
pub fn formatJsChunkResultReport(summary: JsChunkFinalSummary, out: []u8): JsChunkResultError!usize {
    let pos: usize = 0;
    jcrAppendBytes(out, &pos, "JsChunkResult(fmt=")?;
    jcrAppendBytes(out, &pos, jsChunkFinalFormatName(summary.format))?;
    jcrAppendBytes(out, &pos, ", bytes=")?;
    jcrAppendU64(out, &pos, @as(u64, summary.output_bytes))?;
    jcrAppendBytes(out, &pos, ", lines=")?;
    jcrAppendU64(out, &pos, @as(u64, summary.total_lines))?;
    jcrAppendBytes(out, &pos, ", imports=")?;
    jcrAppendU64(out, &pos, @as(u64, summary.imports_emitted))?;
    jcrAppendBytes(out, &pos, ", exports=")?;
    jcrAppendU64(out, &pos, @as(u64, summary.exports_emitted))?;
    jcrAppendBytes(out, &pos, ", sri=")?;
    jcrAppendBytes(out, &pos, summary.integrity.sri())?;
    jcrAppendByte(out, &pos, ')')?;
    return pos;
}
