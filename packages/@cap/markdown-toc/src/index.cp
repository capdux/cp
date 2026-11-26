// @cap/markdown-toc — Zero-allocation Markdown heading extractor, GitHub anchor slugger & TOC generator
// Clean-room MIT implementation inspired by npm `github-slugger` (35M/wk) and `markdown-toc`.

const MAX_HEADINGS: usize = 128;
const MAX_SLUG_POOL: usize = 4096;

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

fn toLower(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isAlphaNum(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendUsize(out: []u8, pos: usize, v: usize) -> usize {
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let x = v;
    if (x == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (x > 0 && n < 24) {
            tmp[n] = ((x % 10) as u8) + '0';
            x = x / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = tmp[k];
        p += 1;
    }
    return p;
}

pub fn githubSlug(heading: string, out: []u8) -> string {
    // GitHub heading anchor algorithm: lowercase ASCII, keep alnum/non-ASCII UTF-8/'-'/'_', spaces -> '-'
    let pos: usize = 0;
    let i: usize = 0;
    while (i < heading.len && pos < out.len) {
        const c = heading[i];
        if (c == ' ' || c == '\t') {
            pos = appendByte(out, pos, '-');
        } else if (isAlphaNum(c) || c == '-' || c == '_') {
            pos = appendByte(out, pos, toLower(c));
        } else if (c >= 128) {
            // Preserve UTF-8 non-ASCII characters (e.g. CJK headings)
            pos = appendByte(out, pos, c);
        }
        // Strip punctuation like '.', ':', '(', ')', '/', '`', '*', '?'
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
        while (i < markdown.len && markdown[i] != '\n') {
            i += 1;
        }
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
                cp = appendStr(cand_buf[0..280], cp, base);
                cp = appendByte(cand_buf[0..280], cp, '-');
                cp = appendUsize(cand_buf[0..280], cp, suffix);
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
        let k: usize = 0;
        while (k < final_slug.len) {
            res.slug_pool[res.slug_pool_len] = final_slug[k];
            res.slug_pool_len += 1;
            k += 1;
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
                pos = appendStr(out, pos, "  ");
                ind += 1;
            }
            pos = appendStr(out, pos, "- [");
            pos = appendStr(out, pos, toc.textAt(i));
            pos = appendStr(out, pos, "](#");
            pos = appendStr(out, pos, toc.slugAt(i));
            pos = appendStr(out, pos, ")");
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
