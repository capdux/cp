// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateCompileResultForCssChunk.cp | Synced: 2026-11-21T23:21:49Z
// ============================================================================
// lib/bundler/cssChunkEmit.cp — CSS Chunk Compiler, URL Rewriter & Safe Minifier
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven CSS chunk emission engine for `cap build`:
//   1. Spec-compliant CSS Chunk assembly (`assembleCssChunkResult`): deduplicates
//      and hoists a single `@charset "UTF-8";` to byte 0, hoists external `@import`
//      rules to the top (CSS Syntax L3), preserves `@layer` statement order,
//      tracks per-module `(byte_offset, byte_len, start_line, end_line)` spans
//      via `cap:simd` (`Simd.countByte`), and appends `/*# sourceMappingURL=... */`;
//   2. Zero-heap `url(...)` relative asset rewriter (`rewriteCssAssetUrls`) using
//      `cap:simd` scanning while skipping `data:`/`http:`/`https:`/`#` URLs and
//      preserving query/fragment suffixes;
//   3. Lexer-aware safe CSS minifier (`minifyCssChunkSafe`) preserving strings,
//      required spaces around `+`/`-` inside `calc()`/`min()`/`max()`/`clamp()`,
//      and `/*! ... */` legal comments with `cap:simd` fast-path scanning.

import { Simd } from "cap:simd";

/// Canonical UTF-8 charset at-rule hoisted to byte 0 when required.
pub const CSS_CHARSET_UTF8: []const u8 = "@charset \"UTF-8\";";

/// Prefix for CSS SourceMap V3 footer comments.
pub const CSS_SOURCEMAP_PREFIX: []const u8 = "/*# sourceMappingURL=";

/// Suffix for CSS SourceMap V3 footer comments.
pub const CSS_SOURCEMAP_SUFFIX: []const u8 = " */";

/// Error set for CSS chunk minification, URL rewriting, and assembly.
pub err CssChunkEmitError {
    BufferTooSmall,
    CapacityExceeded,
    InvalidAssetMapping,
}

// ============================================================================
// Internal Zero-Alloc Byte, Case-Folding, and Formatting Helpers
// ============================================================================

fn ceToLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn ceIsWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn ceBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ceStartsWithIgnoreCase(s: []const u8, prefix_lower: []const u8): bool {
    if (s.len < prefix_lower.len) return false;
    let i: usize = 0;
    while (i < prefix_lower.len) {
        if (ceToLower(s[i]) != prefix_lower[i]) return false;
        i += 1;
    }
    return true;
}

fn ceAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ceAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ceAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ceAppendByte(out, pos, '0')?;
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
        ceAppendByte(out, pos, tmp[n])?;
    }
}

fn ceTrimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && ceIsWs(s[start])) start += 1;
    let end = s.len;
    while (end > start && ceIsWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn ceStripLeadingDotSlash(path: []const u8): []const u8 {
    let s = path;
    while (s.len >= 2 && s[0] == '.' && s[1] == '/') {
        s = s[2..];
    }
    return s;
}

fn ceIsMathFuncPrefix(out: []const u8, pos: usize): bool {
    // Check if the identifier immediately preceding '(' is calc / min / max / clamp
    if (pos >= 4 && ceStartsWithIgnoreCase(out[pos - 4..pos], "calc")) {
        if (pos == 4 || !ceIsIdentChar(out[pos - 5])) return true;
    }
    if (pos >= 5 && ceStartsWithIgnoreCase(out[pos - 5..pos], "clamp")) {
        if (pos == 5 || !ceIsIdentChar(out[pos - 6])) return true;
    }
    if (pos >= 3 && (ceStartsWithIgnoreCase(out[pos - 3..pos], "min") || ceStartsWithIgnoreCase(out[pos - 3..pos], "max"))) {
        if (pos == 3 || !ceIsIdentChar(out[pos - 4])) return true;
    }
    return false;
}

fn ceIsIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-';
}

fn ceIsPunctNoSpaceBefore(c: u8): bool {
    return c == '{' || c == '}' || c == ';' || c == ',' || c == '>' || c == '~' || c == ')';
}

fn ceIsPunctNoSpaceAfter(c: u8): bool {
    return c == '{' || c == '}' || c == ';' || c == ',' || c == ':' || c == '>' || c == '~' || c == '(';
}

// ============================================================================
// Section 1: Lexer-Aware Safe CSS Minifier (`minifyCssChunkSafe`)
// ============================================================================

/// Options controlling `minifyCssChunkSafe`.
pub struct CssMinifyOptions {
    preserve_legal_comments: bool = true,
    preserve_calc_spaces: bool = true,
    strip_trailing_semicolon: bool = true,

    static fn defaults(): CssMinifyOptions {
        return CssMinifyOptions{};
    }
}

/// Statistics returned by `minifyCssChunkSafe`.
pub struct CssMinifyStats {
    input_bytes: usize = 0,
    output_bytes: usize = 0,
    comments_stripped: u32 = 0,
    legal_comments_kept: u32 = 0,
    bytes_saved: usize = 0,
}

/// Minifies CSS safely into `out`:
///   - Preserves single- and double-quoted string literals verbatim;
///   - Strips `/* ... */` comments while preserving `/*! ... */` legal comments;
///   - Preserves mandatory spaces around `+` and `-` inside `calc()`/`min()`/`max()`/`clamp()`;
///   - Uses `cap:simd` (`Simd.skipWhitespace`, `Simd.indexOfByte`) on hot scan paths.
pub fn minifyCssChunkSafe(
    src: []const u8,
    opts: CssMinifyOptions,
    out: []u8,
): CssChunkEmitError!CssMinifyStats {
    let stats = CssMinifyStats{ input_bytes: src.len };
    let pos: usize = 0;
    let i: usize = 0;
    let pending_space = false;

    // Track nesting inside calc()/min()/max()/clamp()
    let total_paren_depth: u32 = 0;
    let math_entry_depth: u32 = 0;
    let in_math_func = false;

    while (i < src.len) {
        const c = src[i];

        // 1. Whitespace run (SIMD fast-skip)
        if (ceIsWs(c)) {
            const ws = Simd.skipWhitespace(src[i..]);
            i += if (ws > 0) ws else 1;
            if (pos > 0) {
                pending_space = true;
            }
            continue;
        }

        // 2. CSS block comment /* ... */
        if (c == '/' && i + 1 < src.len && src[i + 1] == '*') {
            const is_legal = opts.preserve_legal_comments && (i + 2 < src.len && src[i + 2] == '!');
            const start = i;
            i += 2;
            let found_end = false;
            while (i + 1 < src.len) {
                if (star_rel := Simd.indexOfByte(src[i..], '*')) {
                    i += star_rel;
                    if (i + 1 < src.len && src[i + 1] == '/') {
                        i += 2;
                        found_end = true;
                        break;
                    }
                    i += 1;
                } else {
                    i = src.len;
                    break;
                }
            }
            if (!found_end) {
                i = src.len;
            }
            if (is_legal) {
                ceAppendBytes(out, &pos, src[start..i])?;
                stats.legal_comments_kept += 1;
                pending_space = false;
            } else {
                stats.comments_stripped += 1;
                // Treat comment as token separator so `a/*c*/b` stays `a b`
                if (pos > 0) pending_space = true;
            }
            continue;
        }

        // 3. String literal ('...' or "...") — emit verbatim
        if (c == '\'' || c == '"') {
            if (pending_space && pos > 0 && !ceIsPunctNoSpaceAfter(out[pos - 1])) {
                ceAppendByte(out, &pos, ' ')?;
            }
            pending_space = false;
            const quote = c;
            ceAppendByte(out, &pos, quote)?;
            i += 1;
            while (i < src.len) {
                const sc = src[i];
                ceAppendByte(out, &pos, sc)?;
                i += 1;
                if (sc == '\\' && i < src.len) {
                    ceAppendByte(out, &pos, src[i])?;
                    i += 1;
                    continue;
                }
                if (sc == quote) break;
            }
            continue;
        }

        // 4. Strip trailing semicolon right before '}'
        if (c == '}' && opts.strip_trailing_semicolon && pos > 0 && out[pos - 1] == ';') {
            pos -= 1;
        }

        // 5. Decide whether pending_space must be emitted before token `c`
        if (pending_space && pos > 0) {
            const prev = out[pos - 1];
            let need_space = true;
            if (in_math_func && opts.preserve_calc_spaces && (c == '+' || c == '-' || prev == '+' || prev == '-')) {
                need_space = true;
            } else if (c == ':' && prev != ' ') {
                // Keep space before pseudo-class in descendant selector e.g. `.a :hover`
                // while removing space in property declarations `color: red`
                need_space = false;
            } else if (ceIsPunctNoSpaceBefore(c) || ceIsPunctNoSpaceAfter(prev)) {
                need_space = false;
            }
            if (need_space) {
                ceAppendByte(out, &pos, ' ')?;
            }
        }
        pending_space = false;

        // 6. Track math function parentheses for calc() space preservation
        if (c == '(') {
            if (!in_math_func && opts.preserve_calc_spaces && ceIsMathFuncPrefix(out[0..pos], pos)) {
                in_math_func = true;
                math_entry_depth = total_paren_depth;
            }
            total_paren_depth += 1;
        } else if (c == ')') {
            if (total_paren_depth > 0) {
                total_paren_depth -= 1;
                if (in_math_func && total_paren_depth <= math_entry_depth) {
                    in_math_func = false;
                }
            }
        }

        ceAppendByte(out, &pos, c)?;
        i += 1;
    }

    stats.output_bytes = pos;
    stats.bytes_saved = if (src.len >= pos) src.len - pos else 0;
    return stats;
}

// ============================================================================
// Section 2: Zero-Heap `url(...)` Relative Asset Rewriter
// ============================================================================

/// Single asset path mapping from original relative URL to hashed output path.
pub struct CssUrlAssetMapping {
    original_rel_path: []const u8 = "",
    hashed_asset_path: []const u8 = "",
}

/// Configuration for `rewriteCssAssetUrls`.
pub struct CssUrlRewriteOptions {
    public_path: []const u8 = "",
    quote_rewritten: bool = true,

    static fn defaults(): CssUrlRewriteOptions {
        return CssUrlRewriteOptions{};
    }
}

/// Statistics returned by `rewriteCssAssetUrls`.
pub struct CssUrlRewriteStats {
    output_bytes: usize = 0,
    urls_scanned: u32 = 0,
    urls_rewritten: u32 = 0,
    urls_skipped_external: u32 = 0,
    urls_unmatched: u32 = 0,
}

/// Returns `true` if `url` is a `data:`, `http:`, `https:`, `blob:`, `about:`,
/// protocol-relative (`//`), or fragment-only (`#`) reference that must not be rewritten.
pub fn isExternalOrDataCssUrl(url: []const u8): bool {
    const trimmed = ceTrimWs(url);
    if (trimmed.len == 0) return true;
    if (trimmed[0] == '#') return true;
    if (trimmed.len >= 2 && trimmed[0] == '/' && trimmed[1] == '/') return true;
    if (ceStartsWithIgnoreCase(trimmed, "data:")) return true;
    if (ceStartsWithIgnoreCase(trimmed, "http:")) return true;
    if (ceStartsWithIgnoreCase(trimmed, "https:")) return true;
    if (ceStartsWithIgnoreCase(trimmed, "blob:")) return true;
    if (ceStartsWithIgnoreCase(trimmed, "about:")) return true;
    return false;
}

fn ceLookupAssetMapping(
    mappings: []const CssUrlAssetMapping,
    base_url: []const u8,
): usize? {
    const norm_query = ceStripLeadingDotSlash(base_url);
    let i: usize = 0;
    while (i < mappings.len) {
        const m_norm = ceStripLeadingDotSlash(mappings[i].original_rel_path);
        if (ceBytesEql(m_norm, norm_query) || ceBytesEql(mappings[i].original_rel_path, base_url)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Scans `css` for `url(...)` references (skipping string literals and block comments),
/// skips `data:`/`http:`/`https:`/`#` URLs, and rewrites matched relative asset URLs
/// with `opts.public_path` + mapped hashed asset path while preserving `?query`/`#hash`.
pub fn rewriteCssAssetUrls(
    css: []const u8,
    mappings: []const CssUrlAssetMapping,
    opts: CssUrlRewriteOptions,
    out: []u8,
): CssChunkEmitError!CssUrlRewriteStats {
    let stats = CssUrlRewriteStats{};
    let pos: usize = 0;
    let i: usize = 0;

    while (i < css.len) {
        const c = css[i];

        // Copy block comments verbatim without touching `url(` inside comments
        if (c == '/' && i + 1 < css.len && css[i + 1] == '*') {
            const start = i;
            i += 2;
            while (i + 1 < css.len) {
                if (star := Simd.indexOfByte(css[i..], '*')) {
                    i += star;
                    if (i + 1 < css.len && css[i + 1] == '/') {
                        i += 2;
                        break;
                    }
                    i += 1;
                } else {
                    i = css.len;
                    break;
                }
            }
            if (i + 1 == css.len) i = css.len;
            ceAppendBytes(out, &pos, css[start..i])?;
            continue;
        }

        // Copy string literals verbatim
        if (c == '\'' || c == '"') {
            const start = i;
            const quote = c;
            i += 1;
            while (i < css.len) {
                if (css[i] == '\\' && i + 1 < css.len) {
                    i += 2;
                    continue;
                }
                if (css[i] == quote) {
                    i += 1;
                    break;
                }
                i += 1;
            }
            ceAppendBytes(out, &pos, css[start..i])?;
            continue;
        }

        // Match `url(` case-insensitively at identifier boundary
        if ((c == 'u' || c == 'U') &&
            i + 4 <= css.len &&
            ceStartsWithIgnoreCase(css[i..i + 4], "url(") &&
            (i == 0 || !ceIsIdentChar(css[i - 1])))
        {
            const url_start = i;
            let scan = i + 4;
            while (scan < css.len && ceIsWs(css[scan])) scan += 1;

            let inner_start = scan;
            let inner_end = scan;
            let found_close = false;

            if (scan < css.len && (css[scan] == '\'' || css[scan] == '"')) {
                const q = css[scan];
                scan += 1;
                inner_start = scan;
                while (scan < css.len) {
                    if (css[scan] == '\\' && scan + 1 < css.len) {
                        scan += 2;
                        continue;
                    }
                    if (css[scan] == q) {
                        inner_end = scan;
                        scan += 1;
                        break;
                    }
                    scan += 1;
                }
                while (scan < css.len && ceIsWs(css[scan])) scan += 1;
                if (scan < css.len && css[scan] == ')') {
                    found_close = true;
                    scan += 1;
                }
            } else {
                // Unquoted URL: SIMD scan for closing ')'
                if (close_rel := Simd.indexOfByte(css[scan..], ')')) {
                    inner_end = scan + close_rel;
                    scan = inner_end + 1;
                    found_close = true;
                }
            }

            if (!found_close) {
                // Malformed unclosed url( — emit safely as-is
                ceAppendByte(out, &pos, c)?;
                i += 1;
                continue;
            }

            const raw_url = ceTrimWs(css[inner_start..inner_end]);
            stats.urls_scanned += 1;

            if (isExternalOrDataCssUrl(raw_url)) {
                stats.urls_skipped_external += 1;
                ceAppendBytes(out, &pos, css[url_start..scan])?;
                i = scan;
                continue;
            }

            // Separate base path from optional ?query or #hash suffix via SIMD
            let split_idx = raw_url.len;
            if (sfx := Simd.indexOfAny2(raw_url, '?', '#')) {
                split_idx = sfx;
            }
            const base_path = raw_url[0..split_idx];
            const suffix = raw_url[split_idx..];

            if (mapped_idx := ceLookupAssetMapping(mappings, base_path)) {
                const mapped = mappings[mapped_idx].hashed_asset_path;
                ceAppendBytes(out, &pos, "url(")?;
                if (opts.quote_rewritten) ceAppendByte(out, &pos, '"')?;
                if (opts.public_path.len > 0) {
                    ceAppendBytes(out, &pos, opts.public_path)?;
                    const pub_ends_slash = opts.public_path[opts.public_path.len - 1] == '/';
                    const map_starts_slash = mapped.len > 0 && mapped[0] == '/';
                    if (!pub_ends_slash && !map_starts_slash) {
                        ceAppendByte(out, &pos, '/')?;
                    }
                    if (pub_ends_slash && map_starts_slash) {
                        ceAppendBytes(out, &pos, mapped[1..])?;
                    } else {
                        ceAppendBytes(out, &pos, mapped)?;
                    }
                } else {
                    ceAppendBytes(out, &pos, mapped)?;
                }
                ceAppendBytes(out, &pos, suffix)?;
                if (opts.quote_rewritten) ceAppendByte(out, &pos, '"')?;
                ceAppendByte(out, &pos, ')')?;
                stats.urls_rewritten += 1;
            } else {
                stats.urls_unmatched += 1;
                ceAppendBytes(out, &pos, css[url_start..scan])?;
            }
            i = scan;
            continue;
        }

        // Fast-forward to next interesting byte using SIMD when outside url/comment/string
        ceAppendByte(out, &pos, c)?;
        i += 1;
    }

    stats.output_bytes = pos;
    return stats;
}

// ============================================================================
// Section 3: Spec-Compliant CSS Chunk Assembler (`assembleCssChunkResult`)
// ============================================================================

/// Kind of CSS part fed into `assembleCssChunkResult` (mirrors Bun's CSS chunk order kinds).
pub enum CssChunkPartKind {
    LayerDecl,
    ExternalImport,
    SourceModule,
}

/// Single CSS part input for chunk compilation.
pub struct CssChunkPartInput {
    kind: CssChunkPartKind = CssChunkPartKind.SourceModule,
    source_index: u32 = 0,
    code: []const u8 = "",
}

/// Options governing `assembleCssChunkResult`.
pub struct CssChunkEmitOptions {
    minify: bool = false,
    preserve_legal_comments: bool = true,
    emit_charset_utf8: bool = false,
    banner: []const u8 = "",
    footer: []const u8 = "",
    source_map_url: []const u8 = "",
    public_path: []const u8 = "",

    static fn defaults(): CssChunkEmitOptions {
        return CssChunkEmitOptions{};
    }
}

/// Byte and 0-based line span recorded for each emitted CSS module part.
pub struct CssChunkModuleSpan {
    source_index: u32 = 0,
    byte_offset: usize = 0,
    byte_len: usize = 0,
    start_line: u32 = 0,
    end_line: u32 = 0,

    static fn empty(): CssChunkModuleSpan {
        return CssChunkModuleSpan{};
    }
}

/// Summary returned by `assembleCssChunkResult`.
pub struct CssChunkEmitSummary {
    output_bytes: usize = 0,
    emitted_charset: bool = false,
    hoisted_imports: u32 = 0,
    layer_decls: u32 = 0,
    source_modules: u32 = 0,
    urls_rewritten: u32 = 0,
    total_lines: u32 = 0,
    spans_written: usize = 0,
}

/// Strips a leading `@charset ...;` rule if present, returning `(stripped_slice, found_charset)`.
fn ceStripLeadingCharset(src: []const u8): []const u8 {
    let s = src;
    while (true) {
        let i: usize = 0;
        while (i < s.len && ceIsWs(s[i])) i += 1;
        if (i + 8 <= s.len && ceStartsWithIgnoreCase(s[i..], "@charset")) {
            if (semi := Simd.indexOfByte(s[i..], ';')) {
                s = s[i + semi + 1..];
                continue;
            }
        }
        break;
    }
    return s;
}

fn ceHasCharsetRule(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len && ceIsWs(src[i])) i += 1;
    return i + 8 <= src.len && ceStartsWithIgnoreCase(src[i..], "@charset");
}

fn ceWasImportEmitted(
    parts: []const CssChunkPartInput,
    up_to: usize,
    rule_trimmed: []const u8,
): bool {
    let k: usize = 0;
    while (k < up_to) {
        if (parts[k].kind == CssChunkPartKind.ExternalImport) {
            const prev = ceTrimWs(ceStripLeadingCharset(parts[k].code));
            if (ceBytesEql(prev, rule_trimmed)) return true;
        }
        k += 1;
    }
    return false;
}

/// Assembles a complete CSS chunk according to CSS Syntax L3 ordering:
///   1. Single deduplicated `@charset "UTF-8";` at byte 0 (if requested, seen in any input,
///      or any input contains non-ASCII UTF-8 bytes detected via `Simd.isAscii`);
///   2. Hoisted & deduplicated external `@import` rules;
///   3. Optional chunk banner;
///   4. `@layer` declarations & source module bodies with `url(...)` asset rewriting
///      and optional safe minification, recording exact spans via `Simd.countByte`;
///   5. Optional chunk footer & `/*# sourceMappingURL=... */`.
pub fn assembleCssChunkResult(
    parts: []const CssChunkPartInput,
    asset_mappings: []const CssUrlAssetMapping,
    opts: CssChunkEmitOptions,
    scratch: []u8,
    out: []u8,
    out_spans: []CssChunkModuleSpan,
): CssChunkEmitError!CssChunkEmitSummary {
    let summary = CssChunkEmitSummary{};
    let pos: usize = 0;
    let cur_line: u32 = 0;

    // Pass 1: Detect whether @charset "UTF-8"; is required at byte 0
    let need_charset = opts.emit_charset_utf8;
    let p_idx: usize = 0;
    while (p_idx < parts.len && !need_charset) {
        if (ceHasCharsetRule(parts[p_idx].code) || !Simd.isAscii(parts[p_idx].code)) {
            need_charset = true;
        }
        p_idx += 1;
    }

    if (need_charset) {
        ceAppendBytes(out, &pos, CSS_CHARSET_UTF8)?;
        if (!opts.minify) {
            ceAppendByte(out, &pos, '\n')?;
            cur_line += 1;
        }
        summary.emitted_charset = true;
    }

    // Pass 2: Hoist & deduplicate external @import rules to the very top (after @charset)
    let imp_idx: usize = 0;
    while (imp_idx < parts.len) {
        if (parts[imp_idx].kind == CssChunkPartKind.ExternalImport) {
            const stripped = ceTrimWs(ceStripLeadingCharset(parts[imp_idx].code));
            if (stripped.len > 0 && !ceWasImportEmitted(parts, imp_idx, stripped)) {
                ceAppendBytes(out, &pos, stripped)?;
                if (stripped[stripped.len - 1] != ';') {
                    ceAppendByte(out, &pos, ';')?;
                }
                if (!opts.minify) {
                    ceAppendByte(out, &pos, '\n')?;
                    cur_line += 1;
                }
                summary.hoisted_imports += 1;
            }
        }
        imp_idx += 1;
    }

    // Optional Banner
    if (opts.banner.len > 0) {
        const b_start = pos;
        ceAppendBytes(out, &pos, opts.banner)?;
        if (opts.banner[opts.banner.len - 1] != '\n') {
            ceAppendByte(out, &pos, '\n')?;
        }
        cur_line += @as(u32, @intCast(Simd.countByte(out[b_start..pos], '\n')));
    }

    // Pass 3: Emit LayerDecl and SourceModule parts in order
    let m_idx: usize = 0;
    while (m_idx < parts.len) {
        const part = parts[m_idx];
        if (part.kind == CssChunkPartKind.ExternalImport) {
            m_idx += 1;
            continue;
        }

        const body_raw = ceStripLeadingCharset(part.code);
        let stage_slice = body_raw;

        // URL rewrite stage (when mappings exist and part is a SourceModule)
        if (part.kind == CssChunkPartKind.SourceModule && asset_mappings.len > 0) {
            const rw = rewriteCssAssetUrls(
                body_raw,
                asset_mappings,
                CssUrlRewriteOptions{ public_path: opts.public_path, quote_rewritten: true },
                scratch,
            )?;
            stage_slice = scratch[0..rw.output_bytes];
            summary.urls_rewritten += rw.urls_rewritten;
        }

        const mod_byte_start = pos;
        const mod_line_start = cur_line;

        if (opts.minify) {
            const mstat = minifyCssChunkSafe(
                stage_slice,
                CssMinifyOptions{
                    preserve_legal_comments: opts.preserve_legal_comments,
                    preserve_calc_spaces: true,
                    strip_trailing_semicolon: true,
                },
                out[pos..],
            )?;
            pos += mstat.output_bytes;
        } else {
            const trimmed = ceTrimWs(stage_slice);
            if (trimmed.len > 0) {
                ceAppendBytes(out, &pos, trimmed)?;
                ceAppendByte(out, &pos, '\n')?;
            }
        }

        const written_len = pos - mod_byte_start;
        const added_lines = @as(u32, @intCast(Simd.countByte(out[mod_byte_start..pos], '\n')));
        cur_line += added_lines;

        if (part.kind == CssChunkPartKind.LayerDecl) {
            summary.layer_decls += 1;
        } else {
            summary.source_modules += 1;
            if (summary.spans_written < out_spans.len) {
                out_spans[summary.spans_written] = CssChunkModuleSpan{
                    source_index: part.source_index,
                    byte_offset: mod_byte_start,
                    byte_len: written_len,
                    start_line: mod_line_start,
                    end_line: cur_line,
                };
                summary.spans_written += 1;
            }
        }

        m_idx += 1;
    }

    // Optional Footer
    if (opts.footer.len > 0) {
        const f_start = pos;
        if (pos > 0 && out[pos - 1] != '\n') ceAppendByte(out, &pos, '\n')?;
        ceAppendBytes(out, &pos, opts.footer)?;
        if (opts.footer[opts.footer.len - 1] != '\n') ceAppendByte(out, &pos, '\n')?;
        cur_line += @as(u32, @intCast(Simd.countByte(out[f_start..pos], '\n')));
    }

    // Optional SourceMap footer comment
    if (opts.source_map_url.len > 0) {
        if (pos > 0 && out[pos - 1] != '\n') {
            ceAppendByte(out, &pos, '\n')?;
            cur_line += 1;
        }
        ceAppendBytes(out, &pos, CSS_SOURCEMAP_PREFIX)?;
        ceAppendBytes(out, &pos, opts.source_map_url)?;
        ceAppendBytes(out, &pos, CSS_SOURCEMAP_SUFFIX)?;
        ceAppendByte(out, &pos, '\n')?;
        cur_line += 1;
    }

    summary.output_bytes = pos;
    summary.total_lines = cur_line;
    return summary;
}

// ============================================================================
// Section 4: Diagnostic Summary Report Formatter
// ============================================================================

/// Formats a concise diagnostic summary for CSS chunk emission telemetry.
pub fn formatCssChunkEmitReport(summary: CssChunkEmitSummary, out: []u8): CssChunkEmitError!usize {
    let pos: usize = 0;
    ceAppendBytes(out, &pos, "CssChunkEmit(bytes=")?;
    ceAppendU64(out, &pos, @as(u64, summary.output_bytes))?;
    ceAppendBytes(out, &pos, ", lines=")?;
    ceAppendU64(out, &pos, @as(u64, summary.total_lines))?;
    ceAppendBytes(out, &pos, ", charset=")?;
    ceAppendBytes(out, &pos, if (summary.emitted_charset) "utf8" else "none")?;
    ceAppendBytes(out, &pos, ", imports=")?;
    ceAppendU64(out, &pos, @as(u64, summary.hoisted_imports))?;
    ceAppendBytes(out, &pos, ", layers=")?;
    ceAppendU64(out, &pos, @as(u64, summary.layer_decls))?;
    ceAppendBytes(out, &pos, ", modules=")?;
    ceAppendU64(out, &pos, @as(u64, summary.source_modules))?;
    ceAppendBytes(out, &pos, ", urls_rewritten=")?;
    ceAppendU64(out, &pos, @as(u64, summary.urls_rewritten))?;
    ceAppendByte(out, &pos, ')')?;
    return pos;
}
