// Source: /data/dev/bun/src/md/ref_defs.cp
// lib/md/ref_defs.cp
// CommonMark Link Reference Definition Parser, Unicode Case-Folding Normalizer & Hashtable Builder

const std = @import("std");
import { decodeUtf8, encodeUtf8, isWhitespace } from "./helpers.cp";
import { caseFold } from "./unicode.cp";
import {
    OFF,
    BLOCK_CONTAINER_CLOSER,
    BLOCK_CONTAINER_OPENER,
    BLOCK_REF_DEF_ONLY,
    BlockHeader,
    Md4cParserState,
    MdRefDef,
    VerbatimLine,
} from "./types.cp";

pub struct ParseRefDefResult {
    end_pos: usize = 0,
    label: []const u8 = "",
    dest: []const u8 = "",
    title: []const u8 = "",
}

pub struct ParseDestResult {
    dest: []const u8 = "",
    end_pos: usize = 0,
}

pub struct ParseTitleResult {
    title: []const u8 = "",
    end_pos: usize = 0,
}

pub fn normalizeLabel(p: *Md4cParserState, raw: []const u8): []const u8 {
    let result: std.ArrayListUnmanaged(u8) = .empty;
    let in_ws = true;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!in_ws && result.items.len > 0) {
                result.append(p.allocator, ' ') catch return raw;
                in_ws = true;
            }
            i += 1;
        } else if (c >= 0x80) {
            const decoded = decodeUtf8(raw, i);
            const fold = caseFold(decoded.codepoint);
            let j: usize = 0;
            while (j < (fold.n_codepoints as usize)) : (j += 1) {
                let buf: [4]u8 = undefined;
                const len = encodeUtf8(fold.codepoints[j], &buf) as usize;
                if (len > 0) {
                    result.appendSlice(p.allocator, buf[0..len]) catch return raw;
                }
            }
            in_ws = false;
            i += decoded.len as usize;
        } else {
            result.append(p.allocator, std.ascii.toLower(c)) catch return raw;
            in_ws = false;
            i += 1;
        }
    }
    if (result.items.len > 0 && result.items[result.items.len - 1] == ' ') {
        result.items.len -= 1;
    }
    return result.items;
}

pub fn lookupRefDef(p: *Md4cParserState, raw_label: []const u8): MdRefDef? {
    if (raw_label.len == 0) return null;
    const normalized = normalizeLabel(p, raw_label);
    if (normalized.len == 0) return null;
    defer p.allocator.free(normalized);
    for (rd of p.ref_defs.items) {
        if (std.mem.eql(u8, rd.label, normalized)) return rd;
    }
    return null;
}

pub fn skipRefDefWhitespace(text: []const u8, start: usize): usize {
    let p = start;
    while (p < text.len && (text[p] == ' ' || text[p] == '\t')) p += 1;
    if (p < text.len && text[p] == '\n') {
        p += 1;
        while (p < text.len && (text[p] == ' ' || text[p] == '\t')) p += 1;
    }
    return p;
}

pub fn parseRefDefDest(text: []const u8, start: usize): ParseDestResult? {
    let p = start;
    if (p >= text.len) return null;

    if (text[p] == '<') {
        p += 1;
        const dest_start = p;
        while (p < text.len && text[p] != '>' && text[p] != '\n') {
            if (text[p] == '\\' && p + 1 < text.len) {
                p += 2;
            } else {
                p += 1;
            }
        }
        if (p >= text.len || text[p] != '>') return null;
        const dest = text[dest_start..p];
        p += 1;
        return ParseDestResult{ dest: dest, end_pos: p };
    } else {
        const dest_start = p;
        let paren_depth: u32 = 0;
        while (p < text.len && !isWhitespace(text[p])) {
            if (text[p] == '(') {
                paren_depth += 1;
            } else if (text[p] == ')') {
                if (paren_depth == 0) break;
                paren_depth -= 1;
            }
            if (text[p] == '\\' && p + 1 < text.len) {
                p += 2;
            } else {
                p += 1;
            }
        }
        if (p == dest_start) return null;
        return ParseDestResult{ dest: text[dest_start..p], end_pos: p };
    }
}

pub fn parseRefDefTitle(text: []const u8, start: usize): ParseTitleResult? {
    let p = start;
    if (p >= text.len) return null;

    const open_char = text[p];
    let close_char: u8 = '"';
    if (open_char == '"' || open_char == '\'') {
        close_char = open_char;
    } else if (open_char == '(') {
        close_char = ')';
    } else {
        return null;
    }
    p += 1;
    const title_start = p;

    while (p < text.len && text[p] != close_char) {
        if (text[p] == '\\' && p + 1 < text.len) {
            p += 2;
        } else {
            if (open_char == '(' && text[p] == '(') return null;
            p += 1;
        }
    }
    if (p >= text.len) return null;
    const title = text[title_start..p];
    p += 1;
    return ParseTitleResult{ title: title, end_pos: p };
}

pub fn parseRefDef(text: []const u8, pos: usize): ParseRefDefResult? {
    let p = pos;
    if (p >= text.len || text[p] != '[') return null;
    p += 1;

    const label_start = p;
    let label_len: usize = 0;
    while (p < text.len && text[p] != ']') {
        if (text[p] == '[') return null;
        if (text[p] == '\\' && p + 1 < text.len) {
            p += 2;
            label_len += 2;
        } else {
            p += 1;
            label_len += 1;
        }
        if (label_len > 999) return null;
    }
    if (p >= text.len) return null;
    const label = text[label_start..p];
    if (label.len == 0) return null;
    p += 1;

    if (p >= text.len || text[p] != ':') return null;
    p += 1;

    p = skipRefDefWhitespace(text, p);
    const dest_result = parseRefDefDest(text, p) ?? return null;
    p = dest_result.end_pos;
    const dest = dest_result.dest;

    const pos_after_dest = p;
    const p_before_title_ws = p;
    p = skipRefDefWhitespace(text, p);
    let had_newline_before_title = false;
    let wi = p_before_title_ws;
    while (wi < p) : (wi += 1) {
        if (text[wi] == '\n') {
            had_newline_before_title = true;
            break;
        }
    }

    let title: []const u8 = "";
    if (p < text.len && (text[p] == '"' || text[p] == '\'' || text[p] == '(')) {
        const had_whitespace_before_title = p > pos_after_dest;
        if (had_whitespace_before_title) {
            if (title_result := parseRefDefTitle(text, p)) {
                let after_title = title_result.end_pos;
                while (after_title < text.len && (text[after_title] == ' ' || text[after_title] == '\t')) after_title += 1;
                if (after_title >= text.len || text[after_title] == '\n') {
                    title = title_result.title;
                    p = after_title;
                    if (p < text.len && text[p] == '\n') p += 1;
                    return ParseRefDefResult{ end_pos: p, label: label, dest: dest, title: title };
                }
                if (!had_newline_before_title) return null;
            } else {
                if (!had_newline_before_title) return null;
            }
        }
    }

    p = pos_after_dest;
    while (p < text.len && (text[p] == ' ' || text[p] == '\t')) p += 1;
    if (p < text.len && text[p] != '\n') return null;
    if (p < text.len && text[p] == '\n') p += 1;

    return ParseRefDefResult{ end_pos: p, label: label, dest: dest, title: title };
}

pub fn buildRefDefHashtable(p: *Md4cParserState): ! {
    let off: usize = 0;
    const bytes = p.block_bytes.items;

    while (off < bytes.len) {
        const align_mask: usize = @alignOf(BlockHeader) - 1;
        off = (off + align_mask) & ~align_mask;
        if (off + @sizeOf(BlockHeader) > bytes.len) break;

        const hdr: *BlockHeader = unsafe { @ptrCast(@alignCast(bytes.ptr + off)) };
        const hdr_off = off;
        off += @sizeOf(BlockHeader);

        const n_lines = hdr.n_lines;
        const lines_size = (n_lines as usize) * @sizeOf(VerbatimLine);
        if (off + lines_size > bytes.len) break;

        const line_ptr: [*]VerbatimLine = unsafe { @ptrCast(@alignCast(bytes.ptr + off)) };
        const block_lines = unsafe { line_ptr[0..(n_lines as usize)] };
        off += lines_size;

        if (hdr.block_type != .p || (hdr.flags & BLOCK_CONTAINER_OPENER) != 0 || (hdr.flags & BLOCK_CONTAINER_CLOSER) != 0) {
            continue;
        }
        if (n_lines == 0) continue;

        p.buffer.clearRetainingCapacity();
        for (vline of block_lines) {
            if (vline.beg > vline.end || vline.end > p.size) continue;
            if (p.buffer.items.len > 0) {
                p.buffer.append(p.allocator, '\n') catch {};
            }
            p.buffer.appendSlice(p.allocator, p.text[vline.beg..vline.end]) catch {};
        }

        const merged = p.buffer.items;
        let pos: usize = 0;
        let lines_consumed: u32 = 0;

        while (pos < merged.len) {
            const result = parseRefDef(merged, pos) ?? break;
            const norm_label = normalizeLabel(p, result.label);
            if (norm_label.len == 0) break;

            let already_exists = false;
            for (existing of p.ref_defs.items) {
                if (std.mem.eql(u8, existing.label, norm_label)) {
                    already_exists = true;
                    break;
                }
            }
            if (!already_exists) {
                const dest_dupe = p.allocator.dupe(u8, result.dest)?;
                const title_dupe = p.allocator.dupe(u8, result.title)?;
                p.ref_defs.append(p.allocator, MdRefDef{
                    label: norm_label,
                    dest: dest_dupe,
                    title: title_dupe,
                    owns_label: true,
                    owns_dest: true,
                    owns_title: true,
                })?;
            } else {
                p.allocator.free(norm_label);
            }

            let newlines: u32 = 0;
            for (mc of merged[pos..result.end_pos]) {
                if (mc == '\n') newlines += 1;
            }
            if (result.end_pos >= merged.len && (result.end_pos == pos || merged[result.end_pos - 1] != '\n')) {
                newlines += 1;
            }
            lines_consumed += newlines;
            pos = result.end_pos;
        }

        if (lines_consumed > 0) {
            if (lines_consumed >= n_lines) {
                hdr.flags |= BLOCK_REF_DEF_ONLY;
            } else {
                const line_base: [*]VerbatimLine = unsafe { @ptrCast(@alignCast(bytes.ptr + hdr_off + @sizeOf(BlockHeader))) };
                let i: u32 = 0;
                while (i < lines_consumed) : (i += 1) {
                    unsafe {
                        line_base[i].beg = @as(OFF, 1);
                        line_base[i].end = @as(OFF, 0);
                    }
                }
            }
        }
    }
}
