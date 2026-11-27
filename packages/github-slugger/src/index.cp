// @cap/markdown-toc — Zero-allocation Markdown heading extractor, GitHub anchor slugger & TOC generator
// Clean-room MIT implementation inspired by npm `github-slugger` (35M/wk) and `markdown-toc`.
// Accelerated with comptime 256-entry slug LUT, 16-lane @Vector(16, u8) SIMD newline/slug scanning, and @memcpy.

import { strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

type GsVec16 = @Vector(16, u8);

const MAX_HEADINGS: usize = 128;
const MAX_SLUG_POOL: usize = 4096;

fn gsBuildSlugLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        let out: u8 = 0 as u8;
        if (i == 32 || i == 9) {
            out = '-' as u8;
        } else if (i >= 65 && i <= 90) {
            out = (i + 32) as u8;
        } else if ((i >= 97 && i <= 122) || (i >= 48 && i <= 57) || i == 45 || i == 95 || i >= 128) {
            out = i as u8;
        }
        tbl[i] = out;
        i += 1;
    }
    return tbl;
}

const GS_SLUG_LUT: [256]u8 = gsBuildSlugLut();

fn gsFindNewlineSimd(s: string, start: usize) -> usize {
    let i = start;
    const len = s.len;
    const v_nl: GsVec16 = @splat(10 as u8);
    while (i + 16 <= len) {
        const v: GsVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_nl)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != '\n') {
        i += 1;
    }
    return i;
}

pub struct Heading {
    pub level: u8,       // 1..6
    pub text: string,    // cleaned heading text slice
    pub slug_start: usize,
    pub slug_len: usize,
    pub line: usize,     // 1-based line number
}

pub struct TocResult {
    pub headings: [MAX_HEADINGS]Heading,
    pub count: usize,
    pub slug_pool: [MAX_SLUG_POOL]u8,
    pub slug_pool_len: usize,

    pub static fn empty() -> TocResult {
        let h: [MAX_HEADINGS]Heading = undefined;
        let p: [MAX_SLUG_POOL]u8 = undefined;
        return TocResult{
            headings: h,
            count: 0,
            slug_pool: p,
            slug_pool_len: 0,
        };
    }

    pub fn slugAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        const h = _.headings[idx];
        return _.slug_pool[h.slug_start..h.slug_start + h.slug_len];
    }

    pub fn levelAt(idx: usize) -> u8 {
        if (idx >= _.count) {
            return 0;
        }
        return _.headings[idx].level;
    }

    pub fn textAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        return _.headings[idx].text;
    }

    pub fn lineAt(idx: usize) -> usize {
        if (idx >= _.count) {
            return 0;
        }
        return _.headings[idx].line;
    }
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

pub fn githubSlug(heading: string, out: []u8) -> string {
    // GitHub heading anchor algorithm: lowercase ASCII, keep alnum/non-ASCII UTF-8/'-'/'_', spaces -> '-'
    let pos: usize = 0;
    let i: usize = 0;

    // 16-lane SIMD fast path for pure ASCII alnum/space/'-'/'_' chunks
    const v_A: GsVec16 = @splat(65 as u8);
    const v_Z: GsVec16 = @splat(90 as u8);
    const v_a: GsVec16 = @splat(97 as u8);
    const v_z: GsVec16 = @splat(122 as u8);
    const v_0: GsVec16 = @splat(48 as u8);
    const v_9: GsVec16 = @splat(57 as u8);
    const v_sp: GsVec16 = @splat(32 as u8);
    const v_dash: GsVec16 = @splat(45 as u8);
    const v_under: GsVec16 = @splat(95 as u8);
    const v_32: GsVec16 = @splat(32 as u8);
    const v_one: GsVec16 = @splat(1 as u8);
    const v_zero: GsVec16 = @splat(0 as u8);

    while (i + 16 <= heading.len && pos + 16 <= out.len) {
        const v: GsVec16 = heading[i .. i + 16][0..16].*;
        const is_up = @select(u8, v >= v_A, v_one, v_zero) & @select(u8, v <= v_Z, v_one, v_zero);
        const is_lo = @select(u8, v >= v_a, v_one, v_zero) & @select(u8, v <= v_z, v_one, v_zero);
        const is_dg = @select(u8, v >= v_0, v_one, v_zero) & @select(u8, v <= v_9, v_one, v_zero);
        const is_sp = @select(u8, v == v_sp, v_one, v_zero);
        const is_ok = is_up | is_lo | is_dg | is_sp
            | @select(u8, v == v_dash, v_one, v_zero)
            | @select(u8, v == v_under, v_one, v_zero);
        if (@reduce(.Min, is_ok) == (0 as u8)) {
            // Contains punctuation, tab, or UTF-8 -> fall back to O(1) LUT for this chunk
            const blk_end = i + 16;
            while (i < blk_end && pos < out.len) {
                const mapped = GS_SLUG_LUT[heading[i] as usize];
                if (mapped != (0 as u8)) {
                    out[pos] = mapped;
                    pos += 1;
                }
                i += 1;
            }
            continue;
        }
        const lowered = v + @select(u8, is_up == v_one, v_32, v_zero);
        const slugged = @select(u8, is_sp == v_one, v_dash, lowered);
        const arr: [16]u8 = slugged;
        @memcpy(out[pos .. pos + 16], arr[0..16]);
        pos += 16;
        i += 16;
    }

    while (i < heading.len && pos < out.len) {
        const mapped = GS_SLUG_LUT[heading[i] as usize];
        if (mapped != (0 as u8)) {
            out[pos] = mapped;
            pos += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

fn trimHeadingText(raw: string) -> string {
    let start: usize = 0;
    let end = raw.len;
    while (start < end && (raw[start] == ' ' || raw[start] == '\t')) {
        start += 1;
    }
    while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t' || raw[end - 1] == '\r')) {
        end -= 1;
    }
    // Strip optional closing ATX hashes " ###"
    let hash_end = end;
    while (hash_end > start && raw[hash_end - 1] == '#') {
        hash_end -= 1;
    }
    if (hash_end < end && (hash_end == start || raw[hash_end - 1] == ' ' || raw[hash_end - 1] == '\t')) {
        end = hash_end;
        while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t')) {
            end -= 1;
        }
    }
    return raw[start..end];
}

fn isFenceLine(line: string) -> bool {
    let i: usize = 0;
    while (i < line.len && line[i] == ' ' && i < 3) {
        i += 1;
    }
    if (i + 3 <= line.len) {
        if (line[i] == '`' && line[i + 1] == '`' && line[i + 2] == '`') {
            return true;
        }
        if (line[i] == '~' && line[i + 1] == '~' && line[i + 2] == '~') {
            return true;
        }
    }
    return false;
}

fn slugExists(res: *TocResult, cand: string) -> bool {
    let i: usize = 0;
    while (i < res.count) {
        if (strEq(res.slugAt(i), cand)) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn extractToc(markdown: string) -> TocResult {
    let res = TocResult.empty();
    let in_fence = false;
    let line_num: usize = 0;
    let i: usize = 0;
    let base_buf: [256]u8 = undefined;
    let cand_buf: [280]u8 = undefined;

    while (i < markdown.len) {
        const line_start = i;
        i = gsFindNewlineSimd(markdown, i);
        const line_end = i;
        if (i < markdown.len && markdown[i] == '\n') {
            i += 1;
        }
        line_num += 1;

        const line = markdown[line_start..line_end];
        if (isFenceLine(line)) {
            in_fence = !in_fence;
            continue;
        }
        if (in_fence) {
            continue;
        }

        // Check ATX heading: 1..6 '#' followed by space/tab
        let hashes: usize = 0;
        while (hashes < line.len && hashes < 6 && line[hashes] == '#') {
            hashes += 1;
        }
        if (hashes == 0 || hashes >= line.len) {
            continue;
        }
        if (line[hashes] != ' ' && line[hashes] != '\t') {
            continue;
        }

        const clean = trimHeadingText(line[hashes + 1..line.len]);
        if (clean.len == 0 || res.count >= MAX_HEADINGS) {
            continue;
        }

        const base = githubSlug(clean, base_buf[0..256]);
        let final_slug = base;
        if (slugExists(&res, base)) {
            let suffix: usize = 1;
            while (suffix < 1000) {
                let cp: usize = 0;
                cp = writeStr(base, cand_buf[0..280], cp);
                cp = appendByte(cand_buf[0..280], cp, '-');
                cp = writeU64(suffix as u64, cand_buf[0..280], cp);
                const cand = cand_buf[0..cp];
                if (!slugExists(&res, cand)) {
                    final_slug = cand;
                    break;
                }
                suffix += 1;
            }
        }

        if (res.slug_pool_len + final_slug.len > MAX_SLUG_POOL) {
            break;
        }
        const st = res.slug_pool_len;
        if (final_slug.len > 0) {
            @memcpy(res.slug_pool[st .. st + final_slug.len], final_slug[0..final_slug.len]);
            res.slug_pool_len += final_slug.len;
        }
        res.headings[res.count] = Heading{
            level: hashes as u8,
            text: clean,
            slug_start: st,
            slug_len: final_slug.len,
            line: line_num,
        };
        res.count += 1;
    }

    return res;
}

pub fn renderTocMarkdown(toc: TocResult, min_level: u8, max_level: u8, out: []u8) -> string {
    let pos: usize = 0;
    let first = true;
    let i: usize = 0;
    while (i < toc.count) {
        const lvl = toc.levelAt(i);
        if (lvl >= min_level && lvl <= max_level) {
            if (!first) {
                pos = appendByte(out, pos, '\n');
            }
            first = false;
            let ind: u8 = min_level;
            while (ind < lvl) {
                pos = writeStr("  ", out, pos);
                ind += 1;
            }
            pos = writeStr("- [", out, pos);
            pos = writeStr(toc.textAt(i), out, pos);
            pos = writeStr("](#", out, pos);
            pos = writeStr(toc.slugAt(i), out, pos);
            pos = writeStr(")", out, pos);
        }
        i += 1;
    }
    return out[0..pos];
}

export {
    Heading,
    TocResult,
    githubSlug,
    extractToc,
    renderTocMarkdown,
};
