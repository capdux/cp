// Source: cp/lib/text/chunker.cp (UTF-8 Safe Recursive Text & Markdown RAG Chunker)
// Pure CP UTF-8 safe recursive text & Markdown RAG chunker with overlap windowing,
// exact byte offsets, and fenced-code-block-aware heading preservation.

const std = @import("std");

/// Error set shared across `cap:text` operations.
pub err TextErr {
    InvalidParameter,
    OutOfMemory,
}

/// A single zero-copy RAG chunk referencing a contiguous byte span in the source document.
pub struct TextChunk {
    text: string = "";
    start_byte: usize = 0;
    end_byte: usize = 0;
    chunk_index: usize = 0;
    heading: string = "";
}

/// Configuration for `TextChunker.splitRecursive`.
pub struct ChunkOptions {
    max_chars: usize = 512;
    overlap_chars: usize = 0;
}

/// Returns the byte length of the UTF-8 codepoint starting at `pos` (safe on invalid bytes).
fn utf8CharLenAt(s: string, pos: usize): usize {
    if (pos >= s.len) return 0;
    const b = s[pos];
    if (b < 0x80) return 1;
    if ((b & 0xE0) == 0xC0) {
        if (pos + 1 < s.len && (s[pos + 1] & 0xC0) == 0x80) return 2;
        return 1;
    }
    if ((b & 0xF0) == 0xE0) {
        if (pos + 2 < s.len && (s[pos + 1] & 0xC0) == 0x80 && (s[pos + 2] & 0xC0) == 0x80) return 3;
        return 1;
    }
    if ((b & 0xF8) == 0xF0) {
        if (pos + 3 < s.len && (s[pos + 1] & 0xC0) == 0x80 && (s[pos + 2] & 0xC0) == 0x80 && (s[pos + 3] & 0xC0) == 0x80) return 4;
        return 1;
    }
    return 1;
}

/// Advances up to `max_cp` UTF-8 codepoints starting from `start_byte`, clamped to `limit_end`.
fn advanceCodepoints(s: string, start_byte: usize, max_cp: usize, limit_end: usize): usize {
    let pos = start_byte;
    let count: usize = 0;
    while (pos < limit_end && count < max_cp) {
        const step = utf8CharLenAt(s, pos);
        if (step == 0 || pos + step > limit_end) break;
        pos += step;
        count += 1;
    }
    return pos;
}

/// Steps backward up to `back_cp` UTF-8 codepoints from `end_byte`, stopping at `>= min_byte`.
fn stepBackCodepoints(s: string, end_byte: usize, min_byte: usize, back_cp: usize): usize {
    let pos = end_byte;
    let count: usize = 0;
    while (pos > min_byte && count < back_cp) {
        pos -= 1;
        while (pos > min_byte && (s[pos] & 0xC0) == 0x80) {
            pos -= 1;
        }
        count += 1;
    }
    return pos;
}

fn startsWithAt(s: string, pos: usize, end_limit: usize, pat: string): bool {
    if (pos + pat.len > end_limit) return false;
    return std.mem.eql(u8, s[pos..(pos + pat.len)], pat);
}

struct SplitPoint {
    found: bool = false;
    split_end: usize = 0;
    next_start: usize = 0;
}

/// Searches for the last occurrence of `sep` within `[cur..limit_byte]` (or right at `limit_byte`)
/// such that the resulting `split_end` strictly advances beyond `prev_split_end`.
fn findSplitBySeparator(
    s: string,
    cur: usize,
    limit_byte: usize,
    range_end: usize,
    prev_split_end: usize,
    sep: string,
    keep_first_byte: bool,
): SplitPoint {
    // 1. Check if `sep` begins right at `limit_byte` (clean boundary without shrinking chunk)
    if (!keep_first_byte && limit_byte > cur && limit_byte > prev_split_end) {
        if (startsWithAt(s, limit_byte, range_end, sep)) {
            return SplitPoint{
                found: true,
                split_end: limit_byte,
                next_start: limit_byte + sep.len,
            };
        }
    }
    // For ". ", check if the chunk ended with '.' and limit_byte points at ' '
    if (keep_first_byte && limit_byte > cur + 1 && limit_byte > prev_split_end) {
        if (s[limit_byte - 1] == '.' && limit_byte < range_end && s[limit_byte] == ' ') {
            return SplitPoint{
                found: true,
                split_end: limit_byte,
                next_start: limit_byte + 1,
            };
        }
    }

    if (limit_byte <= cur + sep.len) {
        return SplitPoint{};
    }

    // 2. Scan backward from `limit_byte - sep.len` down to `cur`
    let pos: usize = limit_byte - sep.len;
    while (true) {
        if (std.mem.eql(u8, s[pos..(pos + sep.len)], sep)) {
            const cand_end = if (keep_first_byte) pos + 1 else pos;
            const cand_next = pos + sep.len;
            if (cand_end > cur && cand_end > prev_split_end) {
                return SplitPoint{
                    found: true,
                    split_end: cand_end,
                    next_start: cand_next,
                };
            }
        }
        if (pos <= cur) break;
        pos -= 1;
    }
    return SplitPoint{};
}

fn splitRangeInto(
    alloc: std.mem.Allocator,
    full_text: string,
    range_start: usize,
    range_end: usize,
    opts: ChunkOptions,
    heading: string,
    out_list: *std.ArrayList(TextChunk),
): ! {
    if (range_start >= range_end) return;

    const eff_overlap: usize = if (opts.overlap_chars >= opts.max_chars) opts.max_chars - 1 else opts.overlap_chars;
    const base_count = out_list.items.len;
    let cur: usize = range_start;
    let prev_split_end: usize = range_start;

    while (cur < range_end) {
        const limit_byte = advanceCodepoints(full_text, cur, opts.max_chars, range_end);
        if (limit_byte >= range_end) {
            if (range_end > prev_split_end || out_list.items.len == base_count) {
                out_list.append(alloc, TextChunk{
                    text: full_text[cur..range_end],
                    start_byte: cur,
                    end_byte: range_end,
                    chunk_index: out_list.items.len,
                    heading: heading,
                }) catch return TextErr.OutOfMemory;
            }
            break;
        }

        // Priority: "\n\n" -> "\n" -> ". " -> " " -> UTF-8 codepoint boundary
        let sp = findSplitBySeparator(full_text, cur, limit_byte, range_end, prev_split_end, "\n\n", false);
        if (!sp.found) {
            sp = findSplitBySeparator(full_text, cur, limit_byte, range_end, prev_split_end, "\n", false);
        }
        if (!sp.found) {
            sp = findSplitBySeparator(full_text, cur, limit_byte, range_end, prev_split_end, ". ", true);
        }
        if (!sp.found) {
            sp = findSplitBySeparator(full_text, cur, limit_byte, range_end, prev_split_end, " ", false);
        }
        if (!sp.found) {
            sp = SplitPoint{
                found: true,
                split_end: limit_byte,
                next_start: limit_byte,
            };
        }

        out_list.append(alloc, TextChunk{
            text: full_text[cur..sp.split_end],
            start_byte: cur,
            end_byte: sp.split_end,
            chunk_index: out_list.items.len,
            heading: heading,
        }) catch return TextErr.OutOfMemory;

        prev_split_end = sp.split_end;
        if (sp.next_start >= range_end) break;

        if (eff_overlap == 0) {
            cur = sp.next_start;
        } else {
            let backed = stepBackCodepoints(full_text, sp.split_end, cur, eff_overlap);
            if (backed <= cur) {
                backed = advanceCodepoints(full_text, cur, 1, range_end);
            }
            cur = backed;
        }
    }
}

fn isWhitespaceByte(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn hasNonWhitespace(s: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end) : (i += 1) {
        if (!isWhitespaceByte(s[i])) return true;
    }
    return false;
}

/// Checks if `line` is a Markdown ATX heading (`#`, `##`, `###`, up to `######` + space/tab).
/// Returns the trimmed heading text slice, or `null` if the line is not a heading.
fn parseMarkdownHeading(line: string): string? {
    let end = line.len;
    if (end > 0 && line[end - 1] == '\r') end -= 1;
    let hashes: usize = 0;
    while (hashes < end && hashes < 6 && line[hashes] == '#') {
        hashes += 1;
    }
    if (hashes == 0 || hashes >= end) return null;
    if (line[hashes] != ' ' && line[hashes] != '\t') return null;

    let start = hashes + 1;
    while (start < end && (line[start] == ' ' || line[start] == '\t')) {
        start += 1;
    }
    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t')) {
        end -= 1;
    }
    return line[start..end];
}

fn isFenceToggle(line: string): bool {
    let i: usize = 0;
    while (i < line.len && (line[i] == ' ' || line[i] == '\t')) : (i += 1) {}
    if (i + 3 <= line.len) {
        if (std.mem.eql(u8, line[i..(i + 3)], "```")) return true;
        if (std.mem.eql(u8, line[i..(i + 3)], "~~~")) return true;
    }
    return false;
}

/// UTF-8 safe recursive text and Markdown RAG chunker.
pub static struct TextChunker {
    /// Recursively splits `text` into zero-copy `TextChunk` slices of at most `opts.max_chars`
    /// UTF-8 codepoints, preferring `\n\n` -> `\n` -> `. ` -> ` ` -> UTF-8 codepoint boundaries,
    /// with optional `opts.overlap_chars` sliding overlap.
    /// Caller owns the returned slice and must free it with `alloc.free(chunks)`.
    static fn splitRecursive(
        alloc: std.mem.Allocator,
        text: string,
        opts: ChunkOptions = ChunkOptions{},
    ): ([]TextChunk)! {
        if (opts.max_chars == 0) return TextErr.InvalidParameter;
        if (text.len == 0) {
            return alloc.alloc(TextChunk, 0) catch return TextErr.OutOfMemory;
        }

        let chunks: std.ArrayList(TextChunk) = .empty;
        errdefer chunks.deinit(alloc);

        splitRangeInto(alloc, text, 0, text.len, opts, "", *chunks)?;
        return chunks.toOwnedSlice(alloc) catch return TextErr.OutOfMemory;
    }

    /// Splits Markdown document `md` by `#` / `##` / `###` headings (ignoring headings inside
    /// fenced code blocks), preserving the enclosing section title in `TextChunk.heading`
    /// and subdividing large sections up to `max_chars` UTF-8 codepoints.
    /// Caller owns the returned slice and must free it with `alloc.free(chunks)`.
    static fn splitMarkdown(
        alloc: std.mem.Allocator,
        md: string,
        max_chars: usize = 512,
    ): ([]TextChunk)! {
        if (max_chars == 0) return TextErr.InvalidParameter;
        if (md.len == 0) {
            return alloc.alloc(TextChunk, 0) catch return TextErr.OutOfMemory;
        }

        let chunks: std.ArrayList(TextChunk) = .empty;
        errdefer chunks.deinit(alloc);

        const opts = ChunkOptions{ max_chars: max_chars, overlap_chars: 0 };
        let cur_heading: string = "";
        let sec_start: usize = 0;
        let in_fence = false;

        let line_start: usize = 0;
        while (line_start < md.len) {
            let line_end = line_start;
            while (line_end < md.len && md[line_end] != '\n') {
                line_end += 1;
            }
            const next_line_start = if (line_end < md.len) line_end + 1 else md.len;
            const line = md[line_start..line_end];

            if (isFenceToggle(line)) {
                in_fence = !in_fence;
            } else if (!in_fence) {
                if (new_heading := parseMarkdownHeading(line)) {
                    if (hasNonWhitespace(md, sec_start, line_start)) {
                        let trim_end = line_start;
                        while (trim_end > sec_start && (md[trim_end - 1] == '\n' || md[trim_end - 1] == '\r')) {
                            trim_end -= 1;
                        }
                        splitRangeInto(alloc, md, sec_start, trim_end, opts, cur_heading, *chunks)?;
                    }
                    cur_heading = new_heading;
                    sec_start = line_start;
                }
            }
            line_start = next_line_start;
        }

        if (hasNonWhitespace(md, sec_start, md.len)) {
            let trim_end = md.len;
            while (trim_end > sec_start && (md[trim_end - 1] == '\n' || md[trim_end - 1] == '\r')) {
                trim_end -= 1;
            }
            splitRangeInto(alloc, md, sec_start, trim_end, opts, cur_heading, *chunks)?;
        }

        return chunks.toOwnedSlice(alloc) catch return TextErr.OutOfMemory;
    }
}
