// Origin: Pure-CP Stdlib (RAG Recursive Markdown-Aware Document Chunker & Citation Spans) | Created: 2026-11-22
// Pure CP zero-allocation, zero-copy recursive Markdown document chunker with code-fence
// awareness, heading/paragraph/sentence/word/UTF-8 boundary splitting, overlap windows,
// CJK-aware approximate token estimation, and 1-based line range citation locators.

/// Configuration for `chunkDocumentRag`.
pub struct RagChunkConfig {
    max_bytes: usize = 512,
    overlap_bytes: usize = 64,
    respect_code_fences: bool = true,
    split_on_headings: bool = true,

    pub static fn default(): RagChunkConfig {
        return RagChunkConfig{
            max_bytes: 512,
            overlap_bytes: 64,
            respect_code_fences: true,
            split_on_headings: true,
        };
    }
}

/// Zero-copy citation span for a single RAG document chunk.
pub struct RagChunkSpan {
    start: usize = 0,
    end: usize = 0,
    index: u32 = 0,
    est_tokens: u32 = 0,
    line_start: u32 = 1,
    line_end: u32 = 1,

    /// Returns the zero-copy substring view into `doc` for this chunk span.
    pub fn slice(doc: string): string {
        if (_.start > doc.len || _.end > doc.len || _.start > _.end) {
            return "";
        }
        return doc[_.start.._.end];
    }
}

// ============================================================================
// Internal UTF-8 & Markdown Fence Helpers
// ============================================================================

fn isUtf8Cont(b: u8): bool {
    return (b & 0xC0) == 0x80;
}

fn utf8SeqLen(lead: u8): usize {
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

fn alignUtf8Backward(doc: string, pos: usize): usize {
    if (pos >= doc.len) {
        return doc.len;
    }
    let p: usize = pos;
    while (p > 0 && isUtf8Cont(doc[p])) {
        p -= 1;
    }
    return p;
}

fn alignUtf8Forward(doc: string, pos: usize): usize {
    if (pos >= doc.len) {
        return doc.len;
    }
    let p: usize = pos;
    while (p < doc.len && isUtf8Cont(doc[p])) {
        p += 1;
    }
    return p;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isAsciiAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

struct FenceSpan {
    open_start: usize = 0,
    close_end: usize = 0,
}

fn matchFenceMarkerAtLine(doc: string, line_start: usize, line_end: usize): u8 {
    let i: usize = line_start;
    let spaces: usize = 0;
    while (i < line_end && doc[i] == ' ' && spaces < 3) {
        i += 1;
        spaces += 1;
    }
    if (i + 3 > line_end) {
        return 0;
    }
    const ch: u8 = doc[i];
    if (ch != '`' && ch != '~') {
        return 0;
    }
    if (doc[i + 1] == ch && doc[i + 2] == ch) {
        return ch;
    }
    return 0;
}

fn isClosingFenceLine(doc: string, line_start: usize, line_end: usize, marker: u8): bool {
    let i: usize = line_start;
    let spaces: usize = 0;
    while (i < line_end && doc[i] == ' ' && spaces < 3) {
        i += 1;
        spaces += 1;
    }
    let count: usize = 0;
    while (i < line_end && doc[i] == marker) {
        count += 1;
        i += 1;
    }
    if (count < 3) {
        return false;
    }
    while (i < line_end) : (i += 1) {
        if (doc[i] != ' ' && doc[i] != '\t' && doc[i] != '\r') {
            return false;
        }
    }
    return true;
}

/// Scans `doc` to determine whether byte offset `target_pos` lies strictly inside a closed
/// Markdown fenced code block (` ``` ` or ` ~~~ `). Returns its `[open_start, close_end]` span.
fn findEnclosingCodeFence(doc: string, target_pos: usize): FenceSpan? {
    let pos: usize = 0;
    let in_fence: bool = false;
    let fence_marker: u8 = 0;
    let fence_start: usize = 0;

    while (pos < doc.len) {
        const line_start: usize = pos;
        let line_end: usize = pos;
        while (line_end < doc.len && doc[line_end] != '\n') : (line_end += 1) {}
        const next_pos: usize = if (line_end < doc.len && doc[line_end] == '\n') line_end + 1 else doc.len;

        if (!in_fence) {
            if (line_start > target_pos) {
                return null;
            }
            const m: u8 = matchFenceMarkerAtLine(doc, line_start, line_end);
            if (m != 0) {
                in_fence = true;
                fence_marker = m;
                fence_start = line_start;
            }
        } else {
            if (isClosingFenceLine(doc, line_start, line_end, fence_marker)) {
                const close_end: usize = next_pos;
                if (target_pos > fence_start && target_pos < close_end) {
                    return FenceSpan{
                        open_start: fence_start,
                        close_end: close_end,
                    };
                }
                in_fence = false;
                fence_marker = 0;
                if (close_end > target_pos) {
                    return null;
                }
            }
        }
        pos = next_pos;
    }

    return null;
}

fn isPositionInsideFence(doc: string, pos: usize): bool {
    return findEnclosingCodeFence(doc, pos) != null;
}

// ============================================================================
// 1. Citation Line Range Locator (1-Based)
// ============================================================================

/// Computes the 1-based `[line_start, line_end]` range for byte span `doc[start..end]`.
pub fn locateLineRange(doc: string, start: usize, end: usize): [2]u32 {
    const s: usize = if (start <= doc.len) start else doc.len;
    let e: usize = if (end <= doc.len) end else doc.len;
    if (e < s) {
        e = s;
    }

    let line_start: u32 = 1;
    let i: usize = 0;
    while (i < s) : (i += 1) {
        if (doc[i] == '\n') {
            line_start += 1;
        }
    }

    let line_end: u32 = line_start;
    const scan_end: usize = if (e > s && doc[e - 1] == '\n') e - 1 else e;
    i = s;
    while (i < scan_end) : (i += 1) {
        if (doc[i] == '\n') {
            line_end += 1;
        }
    }

    const res: [2]u32 = [line_start, line_end];
    return res;
}

// ============================================================================
// 2. CJK-Aware Approximate RAG Token Estimator
// ============================================================================

/// Estimates approximate LLM token count for `text` with zero heap allocation.
/// Distinguishes ASCII words (~4 chars/token), punctuation, and CJK / 3-4 byte UTF-8
/// characters (~1 token/char), producing significantly better estimates than `len / 4`.
pub fn estimateRagTokens(text: string): u32 {
    if (text.len == 0) {
        return 0;
    }

    let tokens: usize = 0;
    let two_byte_count: usize = 0;
    let has_non_space: bool = false;
    let i: usize = 0;

    while (i < text.len) {
        const b: u8 = text[i];
        if (b < 0x80) {
            if (isAsciiSpace(b)) {
                i += 1;
                continue;
            }
            has_non_space = true;
            if (isAsciiAlnum(b)) {
                const word_start: usize = i;
                while (i < text.len && text[i] < 0x80 && isAsciiAlnum(text[i])) : (i += 1) {}
                const wlen: usize = i - word_start;
                if (wlen <= 6) {
                    tokens += 1;
                } else {
                    tokens += (wlen + 3) / 4;
                }
                continue;
            }
            // ASCII punctuation run
            const pstart: usize = i;
            while (i < text.len && text[i] < 0x80 && !isAsciiSpace(text[i]) && !isAsciiAlnum(text[i])) : (i += 1) {}
            const plen: usize = i - pstart;
            tokens += (plen + 1) / 2;
            continue;
        }

        // Non-ASCII UTF-8 codepoint
        has_non_space = true;
        const seq_len: usize = utf8SeqLen(b);
        if (seq_len >= 3) {
            // CJK ideographs (3-byte) and Emoji/supplementary plane (4-byte): ~1 token per codepoint
            tokens += 1;
        } else {
            two_byte_count += 1;
        }
        i += if (i + seq_len <= text.len) seq_len else (text.len - i);
    }

    if (two_byte_count > 0) {
        tokens += (two_byte_count + 1) / 2;
    }

    if (tokens == 0 && has_non_space) {
        return 1;
    }
    if (tokens > 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }
    return tokens as u32;
}

// ============================================================================
// 3. Recursive Markdown-Aware Document Chunker with Overlap
// ============================================================================

fn findLastHeadingSplit(doc: string, lo: usize, hi: usize, respect_fences: bool): usize? {
    if (hi <= lo + 1) {
        return null;
    }
    let i: usize = hi - 1;
    while (i > lo) : (i -= 1) {
        if (doc[i - 1] == '\n' && doc[i] == '#') {
            if (!respect_fences || !isPositionInsideFence(doc, i)) {
                return i;
            }
        }
    }
    return null;
}

fn findLastParagraphSplit(doc: string, lo: usize, hi: usize, respect_fences: bool): usize? {
    if (hi <= lo + 2) {
        return null;
    }
    let i: usize = hi;
    while (i >= lo + 2) : (i -= 1) {
        if (doc[i - 2] == '\n' && doc[i - 1] == '\n') {
            if (!respect_fences || !isPositionInsideFence(doc, i - 1)) {
                return i;
            }
        }
    }
    return null;
}

fn findLastNewlineSplit(doc: string, lo: usize, hi: usize): usize? {
    if (hi <= lo) {
        return null;
    }
    let i: usize = hi;
    while (i > lo) : (i -= 1) {
        if (doc[i - 1] == '\n') {
            return i;
        }
    }
    return null;
}

fn findLastSentenceSplit(doc: string, lo: usize, hi: usize): usize? {
    if (hi <= lo + 2) {
        return null;
    }
    let i: usize = hi;
    while (i >= lo + 2) : (i -= 1) {
        // ASCII sentence end: ". ", "! ", "? "
        const prev2: u8 = doc[i - 2];
        const prev1: u8 = doc[i - 1];
        if ((prev2 == '.' || prev2 == '!' || prev2 == '?') && (prev1 == ' ' || prev1 == '\t')) {
            return i;
        }
        // UTF-8 CJK full-stop "。" (E3 80 82), "！" (EF BC 81), "？" (EF BC 9F)
        if (i >= lo + 3) {
            const b0: u8 = doc[i - 3];
            const b1: u8 = doc[i - 2];
            const b2: u8 = doc[i - 1];
            if ((b0 == 0xE3 && b1 == 0x80 && b2 == 0x82) ||
                (b0 == 0xEF && b1 == 0xBC && (b2 == 0x81 || b2 == 0x9F)))
            {
                return i;
            }
        }
    }
    return null;
}

fn findLastSpaceSplit(doc: string, lo: usize, hi: usize): usize? {
    if (hi <= lo) {
        return null;
    }
    let i: usize = hi;
    while (i > lo) : (i -= 1) {
        if (doc[i - 1] == ' ' || doc[i - 1] == '\t') {
            return i;
        }
    }
    return null;
}

fn chooseChunkEnd(doc: string, cursor: usize, cfg: RagChunkConfig): usize {
    const remaining: usize = doc.len - cursor;
    if (remaining <= cfg.max_bytes) {
        return doc.len;
    }

    let search_end: usize = alignUtf8Backward(doc, cursor + cfg.max_bytes);
    if (search_end <= cursor) {
        // Extreme case: max_bytes smaller than a single multi-byte UTF-8 codepoint.
        return alignUtf8Forward(doc, cursor + 1);
    }

    // If respecting code fences and search_end cuts inside a fenced block:
    // prefer splitting right BEFORE the fence if there is already content before it.
    if (cfg.respect_code_fences) {
        if (fence := findEnclosingCodeFence(doc, search_end)) {
            if (fence.open_start > cursor) {
                return fence.open_start;
            }
            // The chunk starts inside or at the beginning of a fence that exceeds max_bytes;
            // fall through to split cleanly on line boundary inside the fence.
        }
    }

    const min_split: usize = cursor + (cfg.max_bytes / 4);
    const lo: usize = if (min_split > cursor) min_split else cursor + 1;

    // 1. Markdown heading boundary ("\n#")
    if (cfg.split_on_headings) {
        if (pos := findLastHeadingSplit(doc, cursor + 1, search_end, cfg.respect_code_fences)) {
            if (pos > cursor) {
                return pos;
            }
        }
    }

    // 2. Paragraph boundary ("\n\n")
    if (pos := findLastParagraphSplit(doc, lo, search_end, cfg.respect_code_fences)) {
        return pos;
    }

    // 3. Single newline ("\n")
    if (pos := findLastNewlineSplit(doc, lo, search_end)) {
        return pos;
    }

    // 4. Sentence boundary (". ", "! ", "? ", "。", "！", "？")
    if (pos := findLastSentenceSplit(doc, lo, search_end)) {
        return pos;
    }

    // 5. Space/tab boundary
    if (pos := findLastSpaceSplit(doc, lo, search_end)) {
        return pos;
    }

    // 6. Guaranteed UTF-8 codepoint boundary fallback
    return search_end;
}

/// Recursively chunks `doc` into caller-provided `out_spans` with zero heap allocation.
/// Respects Markdown headings, paragraphs, code fences, sentence/word boundaries, and
/// guarantees every span `[start, end)` lies on valid UTF-8 codepoint boundaries.
/// Returns the number of chunks written, or `null` if config is invalid (`max_bytes == 0`
/// or `overlap_bytes >= max_bytes`) or `out_spans` capacity is insufficient.
pub fn chunkDocumentRag(doc: string, cfg: RagChunkConfig, out_spans: *[]RagChunkSpan): usize? {
    if (cfg.max_bytes == 0 || cfg.overlap_bytes >= cfg.max_bytes) {
        return null;
    }
    if (doc.len == 0) {
        return 0;
    }

    let count: usize = 0;
    let cursor: usize = 0;

    while (cursor < doc.len) {
        if (count >= out_spans.len) {
            return null;
        }

        const chunk_end: usize = chooseChunkEnd(doc, cursor, cfg);
        const lines: [2]u32 = locateLineRange(doc, cursor, chunk_end);
        const est_tok: u32 = estimateRagTokens(doc[cursor..chunk_end]);

        out_spans[count] = RagChunkSpan{
            start: cursor,
            end: chunk_end,
            index: count as u32,
            est_tokens: est_tok,
            line_start: lines[0],
            line_end: lines[1],
        };
        count += 1;

        if (chunk_end >= doc.len) {
            break;
        }

        if (cfg.overlap_bytes == 0) {
            cursor = chunk_end;
        } else {
            let next_start: usize = if (chunk_end > cfg.overlap_bytes) chunk_end - cfg.overlap_bytes else 0;
            next_start = alignUtf8Forward(doc, next_start);
            if (next_start <= cursor || next_start >= chunk_end) {
                next_start = chunk_end;
            }
            cursor = next_start;
        }
    }

    return count;
}
