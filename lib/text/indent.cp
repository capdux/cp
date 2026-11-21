// Source: Absorbed from /data/dev/bun/src/string.zig (Multi-line Text Dedent & Indent Formatter)
// Pure-CP zero-allocation multi-line indentation detector, dedenter, indenter, and prefix stripper.

/// Byte bounds of a single line split into content and trailing EOL (`\n` or `\r\n`).
struct LineSpan {
    content_start: usize;
    content_end: usize;
    eol_end: usize;
}

fn isIndentSpace(b: u8): bool {
    return b == ' ' || b == '\t';
}

/// Read the next line span starting at `pos` (`pos < text.len`).
fn nextLineSpan(text: []const u8, pos: usize): LineSpan {
    let i = pos;
    while (i < text.len && text[i] != '\n') {
        i += 1;
    }
    if (i < text.len) {
        const content_end = if (i > pos && text[i - 1] == '\r') (i - 1) else i;
        return LineSpan {
            content_start: pos,
            content_end: content_end,
            eol_end: i + 1,
        };
    }
    return LineSpan {
        content_start: pos,
        content_end: text.len,
        eol_end: text.len,
    };
}

/// Check if a line (excluding EOL) contains only spaces/tabs (or is empty).
fn isBlankLine(line: []const u8): bool {
    let i: usize = 0;
    while (i < line.len) {
        if (!isIndentSpace(line[i])) return false;
        i += 1;
    }
    return true;
}

/// Count leading space/tab bytes on a line.
fn leadingIndentLen(line: []const u8): usize {
    let i: usize = 0;
    while (i < line.len && isIndentSpace(line[i])) {
        i += 1;
    }
    return i;
}

/// Check if `src` starts with `prefix`.
fn startsWith(src: []const u8, prefix: []const u8): bool {
    if (src.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (src[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

/// Append `src` into `out` at `offset`, returning new offset or `null` if `out` is too small.
fn pushBytes(out: []u8, offset: usize, src: []const u8): usize? {
    if (offset + src.len > out.len) return null;
    let i: usize = 0;
    while (i < src.len) {
        out[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Compute the minimum leading space/tab indentation (in bytes) across all non-blank lines.
/// Returns `0` when `text` is empty or contains only blank lines.
pub fn minCommonIndent(text: []const u8): usize {
    let found = false;
    let min_indent: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        const sp = nextLineSpan(text, pos);
        const line = text[sp.content_start..sp.content_end];
        if (!isBlankLine(line)) {
            const lead = leadingIndentLen(line);
            if (!found || lead < min_indent) {
                min_indent = lead;
                found = true;
                if (min_indent == 0) return 0;
            }
        }
        pos = sp.eol_end;
    }

    return if (found) min_indent else 0;
}

/// Remove `minCommonIndent(text)` leading bytes from every non-blank line,
/// normalize whitespace-only lines to empty lines, and preserve `\n` / `\r\n` endings.
/// Returns `null` if `out` is too small.
pub fn dedent(text: []const u8, out: []u8): ([]const u8)? {
    if (text.len == 0) return out[0..0];

    const common = minCommonIndent(text);
    let w: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        const sp = nextLineSpan(text, pos);
        const line = text[sp.content_start..sp.content_end];
        const eol = text[sp.content_end..sp.eol_end];

        if (!isBlankLine(line)) {
            const skip = if (common < line.len) common else line.len;
            w = pushBytes(out, w, line[skip..line.len]) orelse return null;
        }
        w = pushBytes(out, w, eol) orelse return null;
        pos = sp.eol_end;
    }

    return out[0..w];
}

/// Prepend `prefix` to each line of `text`, preserving `\n` / `\r\n` endings.
/// When `indent_empty == false`, empty and whitespace-only lines are left unindented.
/// Returns `null` if `out` is too small.
pub fn indentLines(text: []const u8, prefix: []const u8, indent_empty: bool, out: []u8): ([]const u8)? {
    if (text.len == 0) return out[0..0];

    let w: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        const sp = nextLineSpan(text, pos);
        const line = text[sp.content_start..sp.content_end];
        const eol = text[sp.content_end..sp.eol_end];

        if (indent_empty || !isBlankLine(line)) {
            w = pushBytes(out, w, prefix) orelse return null;
        }
        w = pushBytes(out, w, line) orelse return null;
        w = pushBytes(out, w, eol) orelse return null;
        pos = sp.eol_end;
    }

    return out[0..w];
}

/// Strip a fixed `prefix` (such as `"> "` or `"// "`) from the start of each line that begins with it.
/// Lines that do not start with `prefix` are copied unchanged; `\n` / `\r\n` endings are preserved.
/// Returns `null` if `out` is too small.
pub fn stripCommonPrefix(text: []const u8, prefix: []const u8, out: []u8): ([]const u8)? {
    if (text.len == 0) return out[0..0];

    let w: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        const sp = nextLineSpan(text, pos);
        const line = text[sp.content_start..sp.content_end];
        const eol = text[sp.content_end..sp.eol_end];

        if (prefix.len > 0 && startsWith(line, prefix)) {
            w = pushBytes(out, w, line[prefix.len..line.len]) orelse return null;
        } else {
            w = pushBytes(out, w, line) orelse return null;
        }
        w = pushBytes(out, w, eol) orelse return null;
        pos = sp.eol_end;
    }

    return out[0..w];
}
