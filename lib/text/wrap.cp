// Source: Absorbed from /data/dev/bun/src/cli.zig | Synced: 2026-11-21T16:45:27Z (CLI Word-Wrap, Dedent & Truncate Formatter)
// Pure-CP zero-allocation greedy word-wrapping, common-indent dedent, prefix indent, and ellipsis truncation.

type Vec16u8 = @Vector(16, u8);

// bit 0: horizontal whitespace (' ', '\t', '\r'), bit 1: any whitespace including '\n'
fn makeWrapWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 3;
    lut['\t'] = 3;
    lut['\r'] = 3;
    lut['\n'] = 2;
    return lut;
}

const WRAP_WS_LUT: [u8; 256] = makeWrapWsLut();

fn findNewlineSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_nl: Vec16u8 = @splat('\n');
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == v_nl);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && s[i] != '\n') {
        i += 1;
    }
    return i;
}

fn findWordEndSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_sp: Vec16u8 = @splat(' ');
    const v_tb: Vec16u8 = @splat('\t');
    const v_cr: Vec16u8 = @splat('\r');
    const v_nl: Vec16u8 = @splat('\n');
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const hit = (v == v_sp) | (v == v_tb) | (v == v_cr) | (v == v_nl);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && (WRAP_WS_LUT[@as(usize, s[i])] & 2) == 0) {
        i += 1;
    }
    return i;
}

fn countLeadingSpacesSimd(s: []const u8, limit: usize): usize {
    let i: usize = 0;
    const v_sp: Vec16u8 = @splat(' ');
    while (i + 16 <= limit) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v != v_sp);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < limit && s[i] == ' ') {
        i += 1;
    }
    return i;
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
    return true;
}

/// Greedy word-wrap `text` to `max_width` columns (preserving explicit `\n` paragraph breaks).
/// Long words exceeding `max_width` are placed intact on their own line without splitting mid-word.
export fn wrapText(text: []const u8, max_width: usize, out: []u8): ([]const u8)? {
    if (max_width == 0) return null;
    let pos: usize = 0;
    let col: usize = 0;
    let i: usize = 0;

    while (i < text.len) {
        if (text[i] == '\n') {
            if (!writeByte(out, &pos, '\n')) return null;
            col = 0;
            i += 1;
            continue;
        }
        // Skip horizontal whitespace between words
        while (i < text.len && (WRAP_WS_LUT[@as(usize, text[i])] & 1) != 0) {
            i += 1;
        }
        if (i >= text.len || text[i] == '\n') continue;

        const wstart = i;
        i = findWordEndSimd(text, i);
        const word = text[wstart..i];

        if (col == 0) {
            if (!writeSlice(out, &pos, word)) return null;
            col = word.len;
        } else if (col + 1 + word.len <= max_width) {
            if (!writeByte(out, &pos, ' ')) return null;
            if (!writeSlice(out, &pos, word)) return null;
            col += 1 + word.len;
        } else {
            if (!writeByte(out, &pos, '\n')) return null;
            if (!writeSlice(out, &pos, word)) return null;
            col = word.len;
        }
    }
    return out[0..pos];
}

/// Prefix every non-empty line of `text` with `prefix` into `out`.
export fn indentNonEmptyLines(text: []const u8, prefix: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        const line_start = i;
        i = findNewlineSimd(text, i);
        const line = text[line_start..i];
        const has_nl = i < text.len && text[i] == '\n';
        if (has_nl) i += 1;

        if (line.len > 0) {
            if (!writeSlice(out, &pos, prefix)) return null;
            if (!writeSlice(out, &pos, line)) return null;
        }
        if (has_nl) {
            if (!writeByte(out, &pos, '\n')) return null;
        }
    }
    return out[0..pos];
}

/// Compute minimum common leading ASCII space count across all non-blank lines.
export fn commonIndent(text: []const u8): usize {
    let min_indent: usize? = null;
    let i: usize = 0;
    while (i < text.len) {
        const line_start = i;
        i = findNewlineSimd(text, i);
        const line = text[line_start..i];
        if (i < text.len && text[i] == '\n') i += 1;

        const spaces = countLeadingSpacesSimd(line, line.len);
        // Ignore blank / whitespace-only lines
        if (spaces == line.len || (spaces + 1 == line.len && line[spaces] == '\r')) {
            continue;
        }
        if (cur := min_indent) {
            if (spaces < cur) min_indent = spaces;
        } else {
            min_indent = spaces;
        }
    }
    return min_indent ?? 0;
}

/// Remove common leading indentation from every line of `text` into `out`.
export fn dedentLines(text: []const u8, out: []u8): ([]const u8)? {
    const strip = commonIndent(text);
    let pos: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        const line_start = i;
        i = findNewlineSimd(text, i);
        const line = text[line_start..i];
        const has_nl = i < text.len && text[i] == '\n';
        if (has_nl) i += 1;

        const skip = if (strip < line.len) strip else line.len;
        const actual_skip = countLeadingSpacesSimd(line, skip);
        if (!writeSlice(out, &pos, line[actual_skip..])) return null;
        if (has_nl) {
            if (!writeByte(out, &pos, '\n')) return null;
        }
    }
    return out[0..pos];
}

/// Truncate `text` to at most `max_len` bytes, appending `ellipsis` (e.g. `"..."`) when truncated.
/// Ensures truncation never splits a multi-byte UTF-8 sequence.
export fn truncateEllipsis(text: []const u8, max_len: usize, ellipsis: []const u8, out: []u8): ([]const u8)? {
    if (text.len <= max_len) {
        if (text.len > out.len) return null;
        if (text.len > 0) {
            @memcpy(out[0..text.len], text);
        }
        return out[0..text.len];
    }
    if (max_len <= ellipsis.len) {
        if (max_len > out.len) return null;
        if (max_len > 0) {
            @memcpy(out[0..max_len], ellipsis[0..max_len]);
        }
        return out[0..max_len];
    }
    let cut = max_len - ellipsis.len;
    // Walk back over UTF-8 continuation bytes (10xxxxxx) so we never split a codepoint
    while (cut > 0 && (text[cut] & 0xC0) == 0x80) {
        cut -= 1;
    }
    let pos: usize = 0;
    if (!writeSlice(out, &pos, text[0..cut])) return null;
    if (!writeSlice(out, &pos, ellipsis)) return null;
    return out[0..pos];
}
