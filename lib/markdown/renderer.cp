// Source: cp/lib/markdown/renderer.cp (CommonMark 0.31.2 + GFM Linear-Time Parser & Safe HTML Renderer)
// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)
// Pure-CP CommonMark 0.31.2 & GFM block/inline parser + HTML renderer with:
// - cap:simd line-end & HTML escape scanning + 16-lane @Vector(16, u8) inline trigger scanning
// - Hard nesting depth cap (MD_MAX_NESTING_DEPTH = 64) for blockquotes & lists
// - Linear-time anti-quadratic-backtracking defenses (no-close bitmasks & scan budgets)
// - Safe URL filtering (blocks javascript:, vbscript:, data:text/html) & HTML escaping

const std = @import("std");
import { Simd } from "cap:simd";
import {
    TableAlign,
    isAtxHeaderLine,
    isHrLine,
    isOpeningCodeFence,
    isClosingCodeFence,
    isTableUnderline,
} from "./line.cp";
import {
    parseAutolink,
    parseInlineLinkOrImage,
    isSafeUrl,
} from "./links.cp";

type MdVec16 = @Vector(16, u8);

/// Maximum nesting depth for recursive blockquotes, lists, and inline emphasis (matches pulldown-cmark safety cap).
pub const MD_MAX_NESTING_DEPTH: usize = 64;

pub struct RenderOptions {
    safe_urls: bool = true,
    gfm_tables: bool = true,
    gfm_tasklist: bool = true,
    gfm_strikethrough: bool = true,
    max_depth: usize = MD_MAX_NESTING_DEPTH,
}

/// SIMD-accelerated line-end finder using `cap:simd` `Simd.indexOfByte('\n')`.
/// Returns index of `\n` (or `text.len` if no newline exists).
pub fn simdFindLineEnd(text: []const u8): usize {
    const hit_opt = Simd.indexOfByte(text, '\n');
    if (h := hit_opt) return h;
    return text.len;
}

/// 16-lane `@Vector(16, u8)` SIMD scan for inline Markdown trigger characters:
/// `*`, `_`, `[`, `` ` ``, `\`, `<`, `&`, `~`, `!`, `>`, `"`.
pub fn simdIndexOfInlineSpecial(text: []const u8): usize? {
    let i: usize = 0;
    const n = text.len;
    const v_star: MdVec16 = @splat('*');
    const v_under: MdVec16 = @splat('_');
    const v_lbrack: MdVec16 = @splat('[');
    const v_tick: MdVec16 = @splat('`');
    const v_bs: MdVec16 = @splat('\\');
    const v_lt: MdVec16 = @splat('<');
    const v_gt: MdVec16 = @splat('>');
    const v_amp: MdVec16 = @splat('&');
    const v_tilde: MdVec16 = @splat('~');
    const v_bang: MdVec16 = @splat('!');
    const v_quot: MdVec16 = @splat('"');

    while (i + 16 <= n) {
        const chunk: MdVec16 = text[i..][0..16].*;
        const m0: u16 = @bitCast(chunk == v_star);
        const m1: u16 = @bitCast(chunk == v_under);
        const m2: u16 = @bitCast(chunk == v_lbrack);
        const m3: u16 = @bitCast(chunk == v_tick);
        const m4: u16 = @bitCast(chunk == v_bs);
        const m5: u16 = @bitCast(chunk == v_lt);
        const m6: u16 = @bitCast(chunk == v_gt);
        const m7: u16 = @bitCast(chunk == v_amp);
        const m8: u16 = @bitCast(chunk == v_tilde);
        const m9: u16 = @bitCast(chunk == v_bang);
        const m10: u16 = @bitCast(chunk == v_quot);
        const mask: u16 = m0 | m1 | m2 | m3 | m4 | m5 | m6 | m7 | m8 | m9 | m10;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }

    while (i < n) : (i += 1) {
        const c = text[i];
        if (c == '*' || c == '_' || c == '[' || c == '`' || c == '\\' ||
            c == '<' || c == '>' || c == '&' || c == '~' || c == '!' || c == '"')
        {
            return i;
        }
    }
    return null;
}

/// SIMD-accelerated HTML escape scan using `cap:simd` `Simd.indexOfAny4('<', '>', '&', '"')`.
pub fn simdIndexOfHtmlEscape(text: []const u8): usize? {
    return Simd.indexOfAny4(text, '<', '>', '&', '"');
}

/// Appends HTML-escaped `text` into `out` using SIMD block-skipping (`<`, `>`, `&`, `"`).
pub fn escapeHtmlSimd(out: *std.ArrayList(u8), allocator: std.mem.Allocator, text: []const u8): ! {
    let i: usize = 0;
    while (i < text.len) {
        const rem = text[i..];
        const hit_opt = simdIndexOfHtmlEscape(rem);
        if (hit_opt == null) {
            out.appendSlice(allocator, rem)?;
            break;
        }
        const hit = hit_opt.?;
        if (hit > 0) {
            out.appendSlice(allocator, rem[0..hit])?;
            i += hit;
        }
        const c = text[i];
        if (c == '<') {
            out.appendSlice(allocator, "&lt;")?;
        } else if (c == '>') {
            out.appendSlice(allocator, "&gt;")?;
        } else if (c == '&') {
            out.appendSlice(allocator, "&amp;")?;
        } else {
            out.appendSlice(allocator, "&quot;")?;
        }
        i += 1;
    }
}

fn mdRendTrimCr(line: []const u8): []const u8 {
    if (line.len > 0 && line[line.len - 1] == '\r') {
        return line[0 .. line.len - 1];
    }
    return line;
}

fn mdRendSkipLeadingSpaces(line: []const u8, max_spaces: usize): usize {
    let i: usize = 0;
    while (i < line.len && i < max_spaces && line[i] == ' ') : (i += 1) {}
    return i;
}

fn mdRendIsBlankLine(line: []const u8): bool {
    let i: usize = 0;
    while (i < line.len) : (i += 1) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') return false;
    }
    return true;
}

fn mdRendIsAsciiPunct(c: u8): bool {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') ||
        (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

fn mdRendParseUlMarker(line: []const u8): usize? {
    const lead = mdRendSkipLeadingSpaces(line, 3);
    if (lead >= line.len) return null;
    const c = line[lead];
    if (c != '-' && c != '*' && c != '+') return null;
    if (lead + 1 >= line.len) return lead + 1;
    if (line[lead + 1] == ' ' || line[lead + 1] == '\t') {
        return lead + 2;
    }
    return null;
}

fn mdRendParseOlMarker(line: []const u8): usize? {
    const lead = mdRendSkipLeadingSpaces(line, 3);
    let p = lead;
    let digits: usize = 0;
    while (p < line.len && line[p] >= '0' && line[p] <= '9' && digits < 9) {
        p += 1;
        digits += 1;
    }
    if (digits == 0 || p >= line.len) return null;
    if (line[p] != '.' && line[p] != ')') return null;
    p += 1;
    if (p >= line.len) return p;
    if (line[p] == ' ' || line[p] == '\t') return p + 1;
    return null;
}

struct InlineGuard {
    no_close_tick_mask: u64 = 0,
    no_close_star1: bool = false,
    no_close_star2: bool = false,
    no_close_under1: bool = false,
    no_close_under2: bool = false,
    no_close_tilde2: bool = false,
    no_close_bracket: bool = false,
    no_close_angle: bool = false,
    scan_budget: usize = 0,
}

fn mdRendFindClosingTicks(text: []const u8, start: usize, ticks: usize, guard: *InlineGuard): usize? {
    if (ticks <= 63) {
        const bit: u64 = (@as(u64, 1)) << @as(u6, @intCast(ticks));
        if ((guard.no_close_tick_mask & bit) != 0) return null;
    }
    let i = start;
    while (i < text.len) {
        if (guard.scan_budget == 0) break;
        guard.scan_budget -= 1;
        if (text[i] == '`') {
            let run: usize = 0;
            while (i + run < text.len && text[i + run] == '`') : (run += 1) {}
            if (run == ticks) return i;
            i += run;
            continue;
        }
        i += 1;
    }
    if (ticks <= 63) {
        const bit: u64 = (@as(u64, 1)) << @as(u6, @intCast(ticks));
        guard.no_close_tick_mask |= bit;
    }
    return null;
}

fn mdRendFindClosingDelim(text: []const u8, start: usize, ch: u8, count: usize, guard: *InlineGuard): usize? {
    if (ch == '*' && count == 1 && guard.no_close_star1) return null;
    if (ch == '*' && count == 2 && guard.no_close_star2) return null;
    if (ch == '_' && count == 1 && guard.no_close_under1) return null;
    if (ch == '_' && count == 2 && guard.no_close_under2) return null;
    if (ch == '~' && count == 2 && guard.no_close_tilde2) return null;

    let i = start;
    while (i + count <= text.len) {
        if (guard.scan_budget == 0) break;
        guard.scan_budget -= 1;
        if (text[i] == '\\' && i + 1 < text.len) {
            i += 2;
            continue;
        }
        if (text[i] == ch) {
            if (count == 1) {
                if (i > start) return i;
            } else if (text[i + 1] == ch) {
                if (i > start) return i;
            }
        }
        i += 1;
    }

    if (ch == '*' && count == 1) guard.no_close_star1 = true;
    if (ch == '*' && count == 2) guard.no_close_star2 = true;
    if (ch == '_' && count == 1) guard.no_close_under1 = true;
    if (ch == '_' && count == 2) guard.no_close_under2 = true;
    if (ch == '~' && count == 2) guard.no_close_tilde2 = true;
    return null;
}

fn mdRendRenderInline(
    out: *std.ArrayList(u8),
    allocator: std.mem.Allocator,
    text: []const u8,
    opts: RenderOptions,
    depth: usize,
    guard: *InlineGuard,
): ! {
    if (depth >= opts.max_depth) {
        escapeHtmlSimd(out, allocator, text)?;
        return;
    }

    let i: usize = 0;
    while (i < text.len) {
        const rem = text[i..];
        const hit_opt = simdIndexOfInlineSpecial(rem);
        if (hit_opt == null) {
            out.appendSlice(allocator, rem)?;
            break;
        }
        const hit = hit_opt.?;
        if (hit > 0) {
            out.appendSlice(allocator, rem[0..hit])?;
            i += hit;
        }

        const c = text[i];

        // 1. Backslash escape
        if (c == '\\') {
            if (i + 1 < text.len && mdRendIsAsciiPunct(text[i + 1])) {
                escapeHtmlSimd(out, allocator, text[i + 1 .. i + 2])?;
                i += 2;
                continue;
            }
            out.append(allocator, '\\')?;
            i += 1;
            continue;
        }

        // 2. Inline code span (`...` or ``...``) with O(N) anti-quadratic cache
        if (c == '`') {
            let ticks: usize = 0;
            while (i + ticks < text.len && text[i + ticks] == '`') : (ticks += 1) {}
            const close_opt = mdRendFindClosingTicks(text, i + ticks, ticks, guard);
            if (close_pos := close_opt) {
                let inner = text[i + ticks .. close_pos];
                if (inner.len >= 2 && inner[0] == ' ' && inner[inner.len - 1] == ' ') {
                    let all_spaces = true;
                    for (k of 0..inner.len) {
                        if (inner[k] != ' ') {
                            all_spaces = false;
                            break;
                        }
                    }
                    if (!all_spaces) {
                        inner = inner[1 .. inner.len - 1];
                    }
                }
                out.appendSlice(allocator, "<code>")?;
                escapeHtmlSimd(out, allocator, inner)?;
                out.appendSlice(allocator, "</code>")?;
                i = close_pos + ticks;
                continue;
            }
            out.appendSlice(allocator, text[i .. i + ticks])?;
            i += ticks;
            continue;
        }

        // 3. Autolink (<https://...> / <user@domain>) with O(N) guard
        if (c == '<') {
            if (!guard.no_close_angle && guard.scan_budget > 0) {
                guard.scan_budget -= 1;
                if (link := parseAutolink(text, i)) {
                    const safe = !opts.safe_urls || isSafeUrl(link.dest);
                    if (link.kind == .autolink_email) {
                        out.appendSlice(allocator, "<a href=\"mailto:")?;
                        escapeHtmlSimd(out, allocator, link.dest)?;
                        out.appendSlice(allocator, "\">")?;
                        escapeHtmlSimd(out, allocator, link.text)?;
                        out.appendSlice(allocator, "</a>")?;
                    } else {
                        out.appendSlice(allocator, "<a href=\"")?;
                        if (safe) {
                            escapeHtmlSimd(out, allocator, link.dest)?;
                        }
                        out.appendSlice(allocator, "\">")?;
                        escapeHtmlSimd(out, allocator, link.text)?;
                        out.appendSlice(allocator, "</a>")?;
                    }
                    i = link.end_pos;
                    continue;
                } else {
                    // Check if any '>' remains; if not, latch no_close_angle for O(1) skip
                    if (Simd.indexOfByte(text[i + 1 ..], '>') == null) {
                        guard.no_close_angle = true;
                    }
                }
            }
            out.appendSlice(allocator, "&lt;")?;
            i += 1;
            continue;
        }

        // 4. Inline link or image: [text](url "title") / ![alt](url "title")
        if (c == '[' || (c == '!' && i + 1 < text.len && text[i + 1] == '[')) {
            if (!guard.no_close_bracket && guard.scan_budget > 0) {
                guard.scan_budget -= 1;
                if (link := parseInlineLinkOrImage(text, i)) {
                    if (link.kind == .inline_link || link.kind == .image) {
                        const safe = !opts.safe_urls || isSafeUrl(link.dest);
                        if (link.kind == .image) {
                            out.appendSlice(allocator, "<img src=\"")?;
                            if (safe) escapeHtmlSimd(out, allocator, link.dest)?;
                            out.appendSlice(allocator, "\" alt=\"")?;
                            escapeHtmlSimd(out, allocator, link.text)?;
                            if (link.title.len > 0) {
                                out.appendSlice(allocator, "\" title=\"")?;
                                escapeHtmlSimd(out, allocator, link.title)?;
                            }
                            out.appendSlice(allocator, "\" />")?;
                        } else {
                            out.appendSlice(allocator, "<a href=\"")?;
                            if (safe) escapeHtmlSimd(out, allocator, link.dest)?;
                            if (link.title.len > 0) {
                                out.appendSlice(allocator, "\" title=\"")?;
                                escapeHtmlSimd(out, allocator, link.title)?;
                            }
                            out.appendSlice(allocator, "\">")?;
                            mdRendRenderInline(out, allocator, link.text, opts, depth + 1, guard)?;
                            out.appendSlice(allocator, "</a>")?;
                        }
                        i = link.end_pos;
                        continue;
                    }
                } else {
                    if (Simd.indexOfByte(text[i + 1 ..], ']') == null) {
                        guard.no_close_bracket = true;
                    }
                }
            }
            out.append(allocator, c)?;
            i += 1;
            continue;
        }

        // 5. GFM Strikethrough: ~~text~~
        if (opts.gfm_strikethrough && c == '~' && i + 1 < text.len && text[i + 1] == '~') {
            if (close_pos := mdRendFindClosingDelim(text, i + 2, '~', 2, guard)) {
                out.appendSlice(allocator, "<del>")?;
                mdRendRenderInline(out, allocator, text[i + 2 .. close_pos], opts, depth + 1, guard)?;
                out.appendSlice(allocator, "</del>")?;
                i = close_pos + 2;
                continue;
            }
            out.appendSlice(allocator, "~~")?;
            i += 2;
            continue;
        }

        // 6. Emphasis / Strong: **...**, __...__, *...*, _..._
        if (c == '*' || c == '_') {
            if (i + 1 < text.len && text[i + 1] == c) {
                if (close_pos := mdRendFindClosingDelim(text, i + 2, c, 2, guard)) {
                    out.appendSlice(allocator, "<strong>")?;
                    mdRendRenderInline(out, allocator, text[i + 2 .. close_pos], opts, depth + 1, guard)?;
                    out.appendSlice(allocator, "</strong>")?;
                    i = close_pos + 2;
                    continue;
                }
                out.append(allocator, c)?;
                out.append(allocator, c)?;
                i += 2;
                continue;
            }
            if (close_pos := mdRendFindClosingDelim(text, i + 1, c, 1, guard)) {
                out.appendSlice(allocator, "<em>")?;
                mdRendRenderInline(out, allocator, text[i + 1 .. close_pos], opts, depth + 1, guard)?;
                out.appendSlice(allocator, "</em>")?;
                i = close_pos + 1;
                continue;
            }
            out.append(allocator, c)?;
            i += 1;
            continue;
        }

        // 7. HTML escape for raw special chars
        if (c == '>') {
            out.appendSlice(allocator, "&gt;")?;
        } else if (c == '&') {
            out.appendSlice(allocator, "&amp;")?;
        } else if (c == '"') {
            out.appendSlice(allocator, "&quot;")?;
        } else {
            out.append(allocator, c)?;
        }
        i += 1;
    }
}

fn mdRendRenderTableCells(
    out: *std.ArrayList(u8),
    allocator: std.mem.Allocator,
    line: []const u8,
    tag: []const u8,
    alignments: *const [32]TableAlign,
    col_count: usize,
    opts: RenderOptions,
    depth: usize,
    guard: *InlineGuard,
): ! {
    let start: usize = 0;
    let end: usize = line.len;
    while (start < end && (line[start] == ' ' || line[start] == '\t')) : (start += 1) {}
    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t')) : (end -= 1) {}
    if (start < end && line[start] == '|') start += 1;
    if (end > start && line[end - 1] == '|') end -= 1;

    out.appendSlice(allocator, "<tr>")?;
    let p = start;
    let col: usize = 0;
    while (col < col_count) : (col += 1) {
        let cell_beg = p;
        while (p < end && line[p] != '|') : (p += 1) {}
        let cell_end = p;
        if (p < end && line[p] == '|') p += 1;

        while (cell_beg < cell_end && (line[cell_beg] == ' ' || line[cell_beg] == '\t')) : (cell_beg += 1) {}
        while (cell_end > cell_beg && (line[cell_end - 1] == ' ' || line[cell_end - 1] == '\t')) : (cell_end -= 1) {}

        out.append(allocator, '<')?;
        out.appendSlice(allocator, tag)?;
        if (col < 32) {
            const al = alignments[col];
            if (al == .left) {
                out.appendSlice(allocator, " align=\"left\"")?;
            } else if (al == .center) {
                out.appendSlice(allocator, " align=\"center\"")?;
            } else if (al == .right) {
                out.appendSlice(allocator, " align=\"right\"")?;
            }
        }
        out.append(allocator, '>')?;
        if (cell_end > cell_beg) {
            mdRendRenderInline(out, allocator, line[cell_beg..cell_end], opts, depth + 1, guard)?;
        }
        out.appendSlice(allocator, "</")?;
        out.appendSlice(allocator, tag)?;
        out.append(allocator, '>')?;
    }
    out.appendSlice(allocator, "</tr>\n")?;
}

fn mdRendRenderBlocks(
    out: *std.ArrayList(u8),
    allocator: std.mem.Allocator,
    md: []const u8,
    opts: RenderOptions,
    depth: usize,
    guard: *InlineGuard,
): ! {
    if (depth >= opts.max_depth) {
        out.appendSlice(allocator, "<p>")?;
        escapeHtmlSimd(out, allocator, md)?;
        out.appendSlice(allocator, "</p>\n")?;
        return;
    }

    let pos: usize = 0;
    while (pos < md.len) {
        const rel_nl = simdFindLineEnd(md[pos..]);
        const line_raw = md[pos .. pos + rel_nl];
        const next_pos = if (pos + rel_nl < md.len) (pos + rel_nl + 1) else md.len;
        const line = mdRendTrimCr(line_raw);

        if (mdRendIsBlankLine(line)) {
            pos = next_pos;
            continue;
        }

        const lead = mdRendSkipLeadingSpaces(line, 3);

        // 1. Fenced code block (``` or ~~~)
        const fence = isOpeningCodeFence(line, lead);
        if (fence.is_fence) {
            out.appendSlice(allocator, "<pre><code")?;
            if (fence.info.len > 0) {
                let lang_end: usize = 0;
                while (lang_end < fence.info.len && fence.info[lang_end] != ' ' && fence.info[lang_end] != '\t') : (lang_end += 1) {}
                if (lang_end > 0) {
                    out.appendSlice(allocator, " class=\"language-")?;
                    escapeHtmlSimd(out, allocator, fence.info[0..lang_end])?;
                    out.append(allocator, '"')?;
                }
            }
            out.append(allocator, '>')?;
            pos = next_pos;
            while (pos < md.len) {
                const c_nl = simdFindLineEnd(md[pos..]);
                const c_raw = md[pos .. pos + c_nl];
                const c_next = if (pos + c_nl < md.len) (pos + c_nl + 1) else md.len;
                const c_line = mdRendTrimCr(c_raw);
                const c_lead = mdRendSkipLeadingSpaces(c_line, 3);
                if (isClosingCodeFence(c_line, c_lead, fence.fence_char, fence.fence_count)) {
                    pos = c_next;
                    break;
                }
                escapeHtmlSimd(out, allocator, c_line)?;
                out.append(allocator, '\n')?;
                pos = c_next;
            }
            out.appendSlice(allocator, "</code></pre>\n")?;
            continue;
        }

        // 2. ATX Heading (# .. ######)
        const atx = isAtxHeaderLine(line, lead, false);
        if (atx.is_atx) {
            let open_tag: [4]u8 = ['<', 'h', ('0' as u8) + atx.level, '>'];
            let close_tag: [6]u8 = ['<', '/', 'h', ('0' as u8) + atx.level, '>', '\n'];
            out.appendSlice(allocator, &open_tag)?;
            mdRendRenderInline(out, allocator, line[atx.content_beg..atx.content_end], opts, depth + 1, guard)?;
            out.appendSlice(allocator, &close_tag)?;
            pos = next_pos;
            continue;
        }

        // 3. Thematic break (--- / *** / ___)
        if (isHrLine(line, lead)) {
            out.appendSlice(allocator, "<hr />\n")?;
            pos = next_pos;
            continue;
        }

        // 4. Blockquote (> ...)
        if (lead < line.len && line[lead] == '>') {
            let bq_buf: std.ArrayList(u8) = .empty;
            defer bq_buf.deinit(allocator);
            while (pos < md.len) {
                const q_nl = simdFindLineEnd(md[pos..]);
                const q_line = mdRendTrimCr(md[pos .. pos + q_nl]);
                const q_next = if (pos + q_nl < md.len) (pos + q_nl + 1) else md.len;
                const q_lead = mdRendSkipLeadingSpaces(q_line, 3);
                if (q_lead >= q_line.len || q_line[q_lead] != '>') break;
                let inner_start = q_lead + 1;
                if (inner_start < q_line.len && q_line[inner_start] == ' ') inner_start += 1;
                bq_buf.appendSlice(allocator, q_line[inner_start..])?;
                bq_buf.append(allocator, '\n')?;
                pos = q_next;
            }
            out.appendSlice(allocator, "<blockquote>\n")?;
            mdRendRenderBlocks(out, allocator, bq_buf.items, opts, depth + 1, guard)?;
            out.appendSlice(allocator, "</blockquote>\n")?;
            continue;
        }

        // 5. Unordered or Ordered List (+ GFM Task List [x] / [ ])
        const ul_opt = mdRendParseUlMarker(line);
        const ol_opt = if (ul_opt == null) mdRendParseOlMarker(line) else null;
        if (ul_opt != null || ol_opt != null) {
            const is_ol = ol_opt != null;
            out.appendSlice(allocator, if (is_ol) "<ol>\n" else "<ul>\n")?;
            while (pos < md.len) {
                const l_nl = simdFindLineEnd(md[pos..]);
                const l_line = mdRendTrimCr(md[pos .. pos + l_nl]);
                const l_next = if (pos + l_nl < md.len) (pos + l_nl + 1) else md.len;
                const m_opt = if (is_ol) mdRendParseOlMarker(l_line) else mdRendParseUlMarker(l_line);
                if (m_opt == null) break;
                let item_body = l_line[m_opt.?..];
                out.appendSlice(allocator, "<li>")?;
                if (opts.gfm_tasklist && item_body.len >= 3 && item_body[0] == '[' && item_body[2] == ']' &&
                    (item_body[1] == ' ' || item_body[1] == 'x' || item_body[1] == 'X') &&
                    (item_body.len == 3 || item_body[3] == ' ' || item_body[3] == '\t'))
                {
                    if (item_body[1] == ' ') {
                        out.appendSlice(allocator, "<input type=\"checkbox\" disabled /> ")?;
                    } else {
                        out.appendSlice(allocator, "<input type=\"checkbox\" checked disabled /> ")?;
                    }
                    item_body = if (item_body.len > 3) item_body[4..] else "";
                }
                mdRendRenderInline(out, allocator, item_body, opts, depth + 1, guard)?;
                out.appendSlice(allocator, "</li>\n")?;
                pos = l_next;
            }
            out.appendSlice(allocator, if (is_ol) "</ol>\n" else "</ul>\n")?;
            continue;
        }

        // 6. GFM Table (header row followed by table delimiter row)
        if (opts.gfm_tables && next_pos < md.len) {
            const sep_nl = simdFindLineEnd(md[next_pos..]);
            const sep_line = mdRendTrimCr(md[next_pos .. next_pos + sep_nl]);
            const sep_next = if (next_pos + sep_nl < md.len) (next_pos + sep_nl + 1) else md.len;
            const tbl = isTableUnderline(sep_line, 0);
            if (tbl.is_underline && tbl.col_count > 0) {
                out.appendSlice(allocator, "<table>\n<thead>\n")?;
                mdRendRenderTableCells(out, allocator, line, "th", &tbl.alignments, tbl.col_count, opts, depth, guard)?;
                out.appendSlice(allocator, "</thead>\n<tbody>\n")?;
                pos = sep_next;
                while (pos < md.len) {
                    const r_nl = simdFindLineEnd(md[pos..]);
                    const r_line = mdRendTrimCr(md[pos .. pos + r_nl]);
                    const r_next = if (pos + r_nl < md.len) (pos + r_nl + 1) else md.len;
                    if (mdRendIsBlankLine(r_line) || Simd.indexOfByte(r_line, '|') == null) break;
                    mdRendRenderTableCells(out, allocator, r_line, "td", &tbl.alignments, tbl.col_count, opts, depth, guard)?;
                    pos = r_next;
                }
                out.appendSlice(allocator, "</tbody>\n</table>\n")?;
                continue;
            }
        }

        // 7. Paragraph
        out.appendSlice(allocator, "<p>")?;
        let first_line = true;
        while (pos < md.len) {
            const p_nl = simdFindLineEnd(md[pos..]);
            const p_line = mdRendTrimCr(md[pos .. pos + p_nl]);
            const p_next = if (pos + p_nl < md.len) (pos + p_nl + 1) else md.len;
            if (mdRendIsBlankLine(p_line)) break;
            if (!first_line) {
                const p_lead = mdRendSkipLeadingSpaces(p_line, 3);
                if (isAtxHeaderLine(p_line, p_lead, false).is_atx ||
                    isOpeningCodeFence(p_line, p_lead).is_fence ||
                    isHrLine(p_line, p_lead) ||
                    (p_lead < p_line.len && p_line[p_lead] == '>') ||
                    mdRendParseUlMarker(p_line) != null ||
                    mdRendParseOlMarker(p_line) != null)
                {
                    break;
                }
                out.append(allocator, '\n')?;
            }
            mdRendRenderInline(out, allocator, p_line, opts, depth + 1, guard)?;
            first_line = false;
            pos = p_next;
        }
        out.appendSlice(allocator, "</p>\n")?;
    }
}

/// Renders CommonMark 0.31.2 + GFM Markdown into safe HTML using configurable `RenderOptions`.
/// Enforces linear-time anti-quadratic-backtracking budgets and recursion depth limit (`max_depth`).
pub fn renderToHtmlWithOptions(
    allocator: std.mem.Allocator,
    md: []const u8,
    opts: RenderOptions,
): ([]u8)! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    let guard = InlineGuard{
        .scan_budget = if (md.len * 8 + 1024 > 65536) (md.len * 8 + 1024) else 65536,
    };
    mdRendRenderBlocks(&out, allocator, md, opts, 0, &guard)?;
    return out.toOwnedSlice(allocator);
}

/// Renders CommonMark 0.31.2 + GFM Markdown into safe HTML with default hardened options
/// (`safe_urls = true`, `gfm_tables = true`, `gfm_tasklist = true`, `gfm_strikethrough = true`, `max_depth = 64`).
pub fn renderToHtml(allocator: std.mem.Allocator, md: []const u8): ([]u8)! {
    return renderToHtmlWithOptions(allocator, md, RenderOptions{});
}
