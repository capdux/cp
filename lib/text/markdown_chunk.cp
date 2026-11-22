// Origin: Pure-CP Stdlib (AI RAG Structure-Aware Markdown Chunker & Heading Breadcrumb Indexer) | Created: 2026-11-22
// Zero-heap-allocation structure-aware Markdown chunker with 6-level heading breadcrumb tracking,
// fenced code block & GFM table integrity preservation, and UTF-8 safe line/paragraph splitting.

/// Structural category of a chunked Markdown section.
pub enum MdSectionKind {
    Prose,
    CodeBlock,
    Table,
    List,
}

/// Configuration controlling Markdown structural chunking behavior.
pub struct MdChunkConfig {
    max_chars: usize = 512;
    min_chars: usize = 64;
    keep_code_blocks_intact: bool = true;
    keep_tables_intact: bool = true;

    /// Returns the default chunking configuration.
    static fn default(): MdChunkConfig {
        return MdChunkConfig{};
    }
}

/// Fixed 6-level ATX heading stack (`H1..H6`) storing byte offsets into the source document.
pub struct MdHeadingTrail {
    starts: [6]usize = [0; 6];
    ends: [6]usize = [0; 6];
    present: [6]bool = [false; 6];
    deepest_level: u8 = 0;

    /// Sets the active heading at `level` (`1..6`) and clears all deeper levels.
    fn setHeading(*_, level: u8, start: usize, end: usize) {
        if (level < 1 || level > 6) return;
        const idx = usize(level - 1);
        _.starts[idx] = start;
        _.ends[idx] = end;
        _.present[idx] = true;

        let j = idx + 1;
        while (j < 6) : (j += 1) {
            _.starts[j] = 0;
            _.ends[j] = 0;
            _.present[j] = false;
        }
        _.deepest_level = level;
    }

    /// Clears all heading levels.
    fn clear(*_) {
        let i: usize = 0;
        while (i < 6) : (i += 1) {
            _.starts[i] = 0;
            _.ends[i] = 0;
            _.present[i] = false;
        }
        _.deepest_level = 0;
    }

    /// Returns the number of active heading levels in the trail.
    fn depth(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < 6) : (i += 1) {
            if (_.present[i]) count += 1;
        }
        return count;
    }

    /// Returns the zero-copy slice of the heading at `level` (`1..6`), or `null` if absent.
    fn headingText(src: string, level: u8): string? {
        if (level < 1 || level > 6) return null;
        const idx = usize(level - 1);
        if (!_.present[idx]) return null;
        const s = _.starts[idx];
        const e = _.ends[idx];
        if (s > e || e > src.len) return null;
        return src[s..e];
    }

    /// Formats the active heading hierarchy into `out_buf` joined by `" > "` (e.g. `"Intro > Setup > Install"`).
    /// Returns the number of bytes written, or `null` if `out_buf` is too small.
    fn formatBreadcrumb(src: string, out_buf: *[]u8): usize? {
        const sep: string = " > ";
        let out_pos: usize = 0;
        let first = true;
        let i: usize = 0;
        while (i < 6) : (i += 1) {
            if (!_.present[i]) continue;
            const s = _.starts[i];
            const e = _.ends[i];
            if (s > e || e > src.len) continue;
            const seg = src[s..e];
            if (!first) {
                if (out_pos + sep.len > out_buf.len) return null;
                let k: usize = 0;
                while (k < sep.len) : (k += 1) {
                    out_buf[out_pos + k] = sep[k];
                }
                out_pos += sep.len;
            }
            if (out_pos + seg.len > out_buf.len) return null;
            let k: usize = 0;
            while (k < seg.len) : (k += 1) {
                out_buf[out_pos + k] = seg[k];
            }
            out_pos += seg.len;
            first = false;
        }
        return out_pos;
    }
}

/// Zero-copy span describing a single structure-aware Markdown chunk.
pub struct MdChunkSpan {
    start: usize = 0;
    end: usize = 0;
    start_line: u32 = 1;
    end_line: u32 = 1;
    kind: MdSectionKind = MdSectionKind.Prose;
    heading_level: u8 = 0;
    trail: MdHeadingTrail = MdHeadingTrail{};

    /// Returns the zero-copy substring slice of this chunk inside `src`.
    fn slice(src: string): string {
        if (_.start > _.end || _.end > src.len) return "";
        return src[_.start.._.end];
    }
}

struct LineInfo {
    start: usize = 0;
    content_end: usize = 0; // excludes trailing \r and \n
    next_pos: usize = 0;    // byte offset of next line start
    line_no: u32 = 1;
}

fn readLine(src: string, pos: usize, line_no: u32): LineInfo {
    let i = pos;
    while (i < src.len && src[i] != '\n') : (i += 1) {}
    let content_end = i;
    if (content_end > pos && src[content_end - 1] == '\r') {
        content_end -= 1;
    }
    const next_pos = if (i < src.len && src[i] == '\n') i + 1 else i;
    return LineInfo{
        start: pos,
        content_end: content_end,
        next_pos: next_pos,
        line_no: line_no,
    };
}

fn isBlankLine(src: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end) : (i += 1) {
        const c = src[i];
        if (c != ' ' && c != '\t' && c != '\r') return false;
    }
    return true;
}

struct HeadingParse {
    level: u8 = 0;
    text_start: usize = 0;
    text_end: usize = 0;
}

/// Parses an ATX Markdown heading (`#`..`######`) from `[start, end)`.
fn parseAtxHeading(src: string, start: usize, end: usize): HeadingParse? {
    let i = start;
    let spaces: usize = 0;
    while (i < end && src[i] == ' ' && spaces < 3) : (i += 1) {
        spaces += 1;
    }
    if (i >= end || src[i] != '#') return null;

    let hashes: u8 = 0;
    while (i < end && src[i] == '#') : (i += 1) {
        if (hashes >= 6) return null;
        hashes += 1;
    }
    if (hashes == 0) return null;
    if (i < end && src[i] != ' ' && src[i] != '\t') return null;

    // Trim leading whitespace after '#'
    while (i < end && (src[i] == ' ' || src[i] == '\t')) : (i += 1) {}
    let t_end = end;
    // Trim trailing whitespace
    while (t_end > i && (src[t_end - 1] == ' ' || src[t_end - 1] == '\t')) : (t_end -= 1) {}
    // Trim optional closing sequence of '#' preceded by space/tab
    if (t_end > i && src[t_end - 1] == '#') {
        let probe = t_end;
        while (probe > i && src[probe - 1] == '#') : (probe -= 1) {}
        if (probe == i || src[probe - 1] == ' ' || src[probe - 1] == '\t') {
            t_end = probe;
            while (t_end > i && (src[t_end - 1] == ' ' || src[t_end - 1] == '\t')) : (t_end -= 1) {}
        }
    }

    return HeadingParse{
        level: hashes,
        text_start: i,
        text_end: t_end,
    };
}

struct FenceOpen {
    ch: u8 = 0;
    len: usize = 0;
}

fn parseFenceOpen(src: string, start: usize, end: usize): FenceOpen? {
    let i = start;
    let spaces: usize = 0;
    while (i < end && src[i] == ' ' && spaces < 3) : (i += 1) {
        spaces += 1;
    }
    if (i >= end) return null;
    const ch = src[i];
    if (ch != '`' && ch != '~') return null;

    let count: usize = 0;
    while (i < end && src[i] == ch) : (i += 1) {
        count += 1;
    }
    if (count < 3) return null;

    // Backtick info strings cannot contain additional backticks
    if (ch == '`') {
        let k = i;
        while (k < end) : (k += 1) {
            if (src[k] == '`') return null;
        }
    }
    return FenceOpen{ ch: ch, len: count };
}

fn isFenceClose(src: string, start: usize, end: usize, open: FenceOpen): bool {
    let i = start;
    let spaces: usize = 0;
    while (i < end && src[i] == ' ' && spaces < 3) : (i += 1) {
        spaces += 1;
    }
    let count: usize = 0;
    while (i < end && src[i] == open.ch) : (i += 1) {
        count += 1;
    }
    if (count < open.len) return false;
    while (i < end) : (i += 1) {
        if (src[i] != ' ' && src[i] != '\t') return false;
    }
    return true;
}

fn lineContainsPipe(src: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end) : (i += 1) {
        if (src[i] == '|') return true;
    }
    return false;
}

fn isTableSeparatorLine(src: string, start: usize, end: usize): bool {
    if (isBlankLine(src, start, end)) return false;
    let has_dash = false;
    let has_pipe = false;
    let dash_count: usize = 0;
    let i = start;
    while (i < end) : (i += 1) {
        const c = src[i];
        if (c == '-') {
            has_dash = true;
            dash_count += 1;
        } else if (c == '|') {
            has_pipe = true;
        } else if (c == ':' || c == ' ' || c == '\t') {
            // valid separator character
        } else {
            return false;
        }
    }
    return has_dash && (has_pipe || dash_count >= 3);
}

fn isTableStart(src: string, line: LineInfo): bool {
    if (!lineContainsPipe(src, line.start, line.content_end)) return false;
    if (line.next_pos >= src.len) return false;
    const next_line = readLine(src, line.next_pos, line.line_no + 1);
    return isTableSeparatorLine(src, next_line.start, next_line.content_end);
}

fn isListLine(src: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end && (src[i] == ' ' || src[i] == '\t')) : (i += 1) {}
    if (i >= end) return false;
    const c = src[i];
    if (c == '-' || c == '*' || c == '+') {
        return i + 1 < end && (src[i + 1] == ' ' || src[i + 1] == '\t');
    }
    if (c >= '0' && c <= '9') {
        let j = i;
        while (j < end && src[j] >= '0' && src[j] <= '9') : (j += 1) {}
        if (j < end && (src[j] == '.' || src[j] == ')')) {
            return j + 1 < end && (src[j + 1] == ' ' || src[j + 1] == '\t');
        }
    }
    return false;
}

fn trailsEqual(a: MdHeadingTrail, b: MdHeadingTrail): bool {
    if (a.deepest_level != b.deepest_level) return false;
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        if (a.present[i] != b.present[i]) return false;
        if (a.present[i] && (a.starts[i] != b.starts[i] || a.ends[i] != b.ends[i])) {
            return false;
        }
    }
    return true;
}

fn utf8SafeCutEnd(src: string, start: usize, limit: usize): usize {
    if (limit >= src.len) return src.len;
    let cut = limit;
    // Step back over UTF-8 continuation bytes (10xxxxxx)
    while (cut > start && (src[cut] & 0xC0) == 0x80) : (cut -= 1) {}
    if (cut == start) {
        // Advance past at least one full UTF-8 codepoint to guarantee progress
        cut = start + 1;
        while (cut < src.len && (src[cut] & 0xC0) == 0x80) : (cut += 1) {}
        return cut;
    }
    return cut;
}

fn countNewlines(src: string, start: usize, end: usize): u32 {
    let n: u32 = 0;
    let i = start;
    while (i < end) : (i += 1) {
        if (src[i] == '\n') n += 1;
    }
    return n;
}

struct ChunkEmitter {
    out_chunks: []MdChunkSpan;
    total: usize = 0;
    has_pending: bool = false;
    pending: MdChunkSpan = MdChunkSpan{};
    max_chars: usize = 512;
    min_chars: usize = 64;

    fn flush(*_) {
        if (!_.has_pending) return;
        if (_.total < _.out_chunks.len) {
            _.out_chunks[_.total] = _.pending;
        }
        _.total += 1;
        _.has_pending = false;
    }

    fn emit(*_, span: MdChunkSpan) {
        if (span.start >= span.end) return;

        // Merge consecutive Prose chunks under the exact same heading trail when either is below min_chars
        if (_.has_pending &&
            _.pending.kind == MdSectionKind.Prose &&
            span.kind == MdSectionKind.Prose &&
            trailsEqual(_.pending.trail, span.trail))
        {
            const combined_len = span.end - _.pending.start;
            const prev_len = _.pending.end - _.pending.start;
            const curr_len = span.end - span.start;
            if (combined_len <= _.max_chars && (prev_len < _.min_chars || curr_len < _.min_chars)) {
                _.pending.end = span.end;
                _.pending.end_line = span.end_line;
                return;
            }
        }

        _.flush();
        _.pending = span;
        _.has_pending = true;
    }
}

/// Splits a block `[blk_start, blk_end)` into pieces of at most `max_chars` bytes,
/// preferring newline and space boundaries while preserving UTF-8 codepoint integrity.
fn emitSplitRange(
    emitter: *ChunkEmitter,
    src: string,
    blk_start: usize,
    blk_end: usize,
    blk_start_line: u32,
    kind: MdSectionKind,
    trail: MdHeadingTrail,
    max_chars: usize,
) {
    let pos = blk_start;
    let cur_line = blk_start_line;

    while (pos < blk_end) {
        // Trim leading spaces/newlines on continuation splits
        if (pos > blk_start) {
            while (pos < blk_end && (src[pos] == '\n' || src[pos] == '\r')) {
                if (src[pos] == '\n') cur_line += 1;
                pos += 1;
            }
            if (pos >= blk_end) break;
        }

        if (blk_end - pos <= max_chars) {
            const nl = countNewlines(src, pos, blk_end);
            emitter.emit(MdChunkSpan{
                start: pos,
                end: blk_end,
                start_line: cur_line,
                end_line: cur_line + nl,
                kind: kind,
                heading_level: trail.deepest_level,
                trail: trail,
            });
            break;
        }

        const raw_limit = pos + max_chars;
        const safe_limit = utf8SafeCutEnd(src, pos, raw_limit);

        // Prefer breaking at last newline, then last space within the second half of the window
        let cut = safe_limit;
        let found_nl: usize? = null;
        let scan = safe_limit;
        while (scan > pos) : (scan -= 1) {
            if (src[scan - 1] == '\n') {
                found_nl = scan - 1;
                break;
            }
        }
        if (nl_pos := found_nl) {
            if (nl_pos > pos) {
                cut = nl_pos;
            }
        } else {
            let sp = safe_limit;
            while (sp > pos) : (sp -= 1) {
                if (src[sp - 1] == ' ' || src[sp - 1] == '\t') {
                    cut = sp - 1;
                    break;
                }
            }
            if (cut == pos) {
                cut = safe_limit;
            }
        }

        // Trim trailing \r from cut end
        let piece_end = cut;
        if (piece_end > pos && src[piece_end - 1] == '\r') {
            piece_end -= 1;
        }
        if (piece_end <= pos) {
            piece_end = safe_limit;
            cut = safe_limit;
        }

        const nl_inside = countNewlines(src, pos, piece_end);
        emitter.emit(MdChunkSpan{
            start: pos,
            end: piece_end,
            start_line: cur_line,
            end_line: cur_line + nl_inside,
            kind: kind,
            heading_level: trail.deepest_level,
            trail: trail,
        });

        let next_pos = cut;
        if (next_pos < blk_end && (src[next_pos] == ' ' || src[next_pos] == '\t' || src[next_pos] == '\n')) {
            if (src[next_pos] == '\n') {
                cur_line += nl_inside + 1;
            } else {
                cur_line += nl_inside;
            }
            next_pos += 1;
        } else {
            cur_line += nl_inside;
        }
        pos = next_pos;
    }
}

/// Performs zero-heap-allocation, structure-aware Markdown chunking with 6-level heading breadcrumbs.
/// - Recognizes `#`..`######` ATX headings outside fenced code blocks (` ``` ` / `~~~`).
/// - Preserves fenced code blocks and GFM tables intact when configured.
/// - Splits oversized prose/list blocks on newline, whitespace, and UTF-8 codepoint boundaries.
/// - Writes up to `out_chunks.len` spans into `out_chunks` and returns the total chunk count.
pub fn chunkMarkdownStructured(
    src: string,
    cfg: MdChunkConfig,
    out_chunks: *[]MdChunkSpan,
): usize {
    if (src.len == 0 || cfg.max_chars == 0) {
        return 0;
    }

    let emitter = ChunkEmitter{
        out_chunks: out_chunks,
        total: 0,
        has_pending: false,
        pending: MdChunkSpan{},
        max_chars: cfg.max_chars,
        min_chars: cfg.min_chars,
    };

    let trail = MdHeadingTrail{};
    let pos: usize = 0;
    let line_no: u32 = 1;

    while (pos < src.len) {
        const line = readLine(src, pos, line_no);

        // 1. Skip blank lines
        if (isBlankLine(src, line.start, line.content_end)) {
            pos = line.next_pos;
            line_no += 1;
            continue;
        }

        // 2. ATX heading (#..######): updates breadcrumb trail and flushes pending block
        if (hp := parseAtxHeading(src, line.start, line.content_end)) {
            emitter.flush();
            trail.setHeading(hp.level, hp.text_start, hp.text_end);
            pos = line.next_pos;
            line_no += 1;
            continue;
        }

        // 3. Fenced code block (``` or ~~~): '#' inside is never parsed as heading
        if (fence := parseFenceOpen(src, line.start, line.content_end)) {
            const blk_start = line.start;
            const blk_start_line = line.line_no;
            let blk_end = line.content_end;
            let blk_end_line = line.line_no;
            pos = line.next_pos;
            line_no += 1;

            while (pos < src.len) {
                const cur = readLine(src, pos, line_no);
                blk_end = cur.content_end;
                blk_end_line = cur.line_no;
                pos = cur.next_pos;
                line_no += 1;
                if (isFenceClose(src, cur.start, cur.content_end, fence)) {
                    break;
                }
            }

            if (cfg.keep_code_blocks_intact || (blk_end - blk_start <= cfg.max_chars)) {
                emitter.emit(MdChunkSpan{
                    start: blk_start,
                    end: blk_end,
                    start_line: blk_start_line,
                    end_line: blk_end_line,
                    kind: MdSectionKind.CodeBlock,
                    heading_level: trail.deepest_level,
                    trail: trail,
                });
            } else {
                emitSplitRange(
                    *emitter,
                    src,
                    blk_start,
                    blk_end,
                    blk_start_line,
                    MdSectionKind.CodeBlock,
                    trail,
                    cfg.max_chars,
                );
            }
            continue;
        }

        // 4. GFM Markdown table block
        if (isTableStart(src, line)) {
            const blk_start = line.start;
            const blk_start_line = line.line_no;
            let blk_end = line.content_end;
            let blk_end_line = line.line_no;
            pos = line.next_pos;
            line_no += 1;

            // Consume separator line + subsequent table rows
            let consumed_sep = false;
            while (pos < src.len) {
                const cur = readLine(src, pos, line_no);
                if (!consumed_sep) {
                    consumed_sep = true;
                    blk_end = cur.content_end;
                    blk_end_line = cur.line_no;
                    pos = cur.next_pos;
                    line_no += 1;
                    continue;
                }
                if (isBlankLine(src, cur.start, cur.content_end)) break;
                if (parseAtxHeading(src, cur.start, cur.content_end) != null) break;
                if (parseFenceOpen(src, cur.start, cur.content_end) != null) break;
                if (!lineContainsPipe(src, cur.start, cur.content_end)) break;
                blk_end = cur.content_end;
                blk_end_line = cur.line_no;
                pos = cur.next_pos;
                line_no += 1;
            }

            if (cfg.keep_tables_intact || (blk_end - blk_start <= cfg.max_chars)) {
                emitter.emit(MdChunkSpan{
                    start: blk_start,
                    end: blk_end,
                    start_line: blk_start_line,
                    end_line: blk_end_line,
                    kind: MdSectionKind.Table,
                    heading_level: trail.deepest_level,
                    trail: trail,
                });
            } else {
                emitSplitRange(
                    *emitter,
                    src,
                    blk_start,
                    blk_end,
                    blk_start_line,
                    MdSectionKind.Table,
                    trail,
                    cfg.max_chars,
                );
            }
            continue;
        }

        // 5. Markdown list block
        if (isListLine(src, line.start, line.content_end)) {
            const blk_start = line.start;
            const blk_start_line = line.line_no;
            let blk_end = line.content_end;
            let blk_end_line = line.line_no;
            pos = line.next_pos;
            line_no += 1;

            while (pos < src.len) {
                const cur = readLine(src, pos, line_no);
                if (isBlankLine(src, cur.start, cur.content_end)) break;
                if (parseAtxHeading(src, cur.start, cur.content_end) != null) break;
                if (parseFenceOpen(src, cur.start, cur.content_end) != null) break;
                if (isTableStart(src, cur)) break;
                const indented = cur.start < cur.content_end && (src[cur.start] == ' ' || src[cur.start] == '\t');
                if (!isListLine(src, cur.start, cur.content_end) && !indented) break;
                blk_end = cur.content_end;
                blk_end_line = cur.line_no;
                pos = cur.next_pos;
                line_no += 1;
            }

            if (blk_end - blk_start <= cfg.max_chars) {
                emitter.emit(MdChunkSpan{
                    start: blk_start,
                    end: blk_end,
                    start_line: blk_start_line,
                    end_line: blk_end_line,
                    kind: MdSectionKind.List,
                    heading_level: trail.deepest_level,
                    trail: trail,
                });
            } else {
                emitSplitRange(
                    *emitter,
                    src,
                    blk_start,
                    blk_end,
                    blk_start_line,
                    MdSectionKind.List,
                    trail,
                    cfg.max_chars,
                );
            }
            continue;
        }

        // 6. Prose paragraph block
        const blk_start = line.start;
        const blk_start_line = line.line_no;
        let blk_end = line.content_end;
        let blk_end_line = line.line_no;
        pos = line.next_pos;
        line_no += 1;

        while (pos < src.len) {
            const cur = readLine(src, pos, line_no);
            if (isBlankLine(src, cur.start, cur.content_end)) break;
            if (parseAtxHeading(src, cur.start, cur.content_end) != null) break;
            if (parseFenceOpen(src, cur.start, cur.content_end) != null) break;
            if (isTableStart(src, cur)) break;
            if (isListLine(src, cur.start, cur.content_end)) break;
            blk_end = cur.content_end;
            blk_end_line = cur.line_no;
            pos = cur.next_pos;
            line_no += 1;
        }

        if (blk_end - blk_start <= cfg.max_chars) {
            emitter.emit(MdChunkSpan{
                start: blk_start,
                end: blk_end,
                start_line: blk_start_line,
                end_line: blk_end_line,
                kind: MdSectionKind.Prose,
                heading_level: trail.deepest_level,
                trail: trail,
            });
        } else {
            emitSplitRange(
                *emitter,
                src,
                blk_start,
                blk_end,
                blk_start_line,
                MdSectionKind.Prose,
                trail,
                cfg.max_chars,
            );
        }
    }

    emitter.flush();
    return emitter.total;
}
