// Source: cp/lib/prompt/extractor.cp (LLM Output Structured Extractor & Delimiter Guard)
// Pure CP zero-copy structured extractor for LLM responses (fenced Markdown code blocks,
// balanced JSON object/array extraction, XML tags like <thinking>/<tool_call>/<answer>,
// and prompt-boundary delimiter escaping).

const std = @import("std");
import { PromptError } from "./messages.cp";

/// Extracted fenced Markdown code block (`lang` and `content` are zero-copy slices of the source text).
pub struct CodeBlock {
    lang: string = "",
    content: string = "",
}

/// Extracted XML tag span (`tag` and `content` are slices; `start_offset..end_offset` covers `<tag>...</tag>`).
pub struct XmlTagMatch {
    tag: string = "",
    content: string = "",
    start_offset: usize = 0,
    end_offset: usize = 0,
}

struct LineSpan {
    start: usize = 0,
    end: usize = 0,
    next_start: usize = 0,
}

fn nextLineSpan(text: string, pos: usize): LineSpan {
    let i = pos;
    while (i < text.len && text[i] != '\n') : (i += 1) {}
    let end = i;
    if (end > pos && text[end - 1] == '\r') {
        end -= 1;
    }
    const next_start: usize = if (i < text.len && text[i] == '\n') i + 1 else text.len;
    return LineSpan{
        start: pos,
        end: end,
        next_start: next_start,
    };
}

struct FenceOpen {
    matched: bool = false,
    fence_len: usize = 0,
    lang: string = "",
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn parseOpeningFence(line: string): FenceOpen {
    let i: usize = 0;
    let lead_spaces: usize = 0;
    while (i < line.len && line[i] == ' ' && lead_spaces < 3) {
        i += 1;
        lead_spaces += 1;
    }
    let ticks: usize = 0;
    while (i < line.len && line[i] == '`') {
        ticks += 1;
        i += 1;
    }
    if (ticks < 3) {
        return FenceOpen{ matched: false };
    }
    // CommonMark: backtick info string cannot contain backticks.
    let scan = i;
    while (scan < line.len) : (scan += 1) {
        if (line[scan] == '`') {
            return FenceOpen{ matched: false };
        }
    }
    while (i < line.len && isSpaceOrTab(line[i])) : (i += 1) {}
    const lang_start = i;
    while (i < line.len && !isSpaceOrTab(line[i])) : (i += 1) {}
    return FenceOpen{
        matched: true,
        fence_len: ticks,
        lang: line[lang_start..i],
    };
}

fn isClosingFence(line: string, min_ticks: usize): bool {
    let i: usize = 0;
    let lead_spaces: usize = 0;
    while (i < line.len && line[i] == ' ' && lead_spaces < 3) {
        i += 1;
        lead_spaces += 1;
    }
    let ticks: usize = 0;
    while (i < line.len && line[i] == '`') {
        ticks += 1;
        i += 1;
    }
    if (ticks < min_ticks) return false;
    while (i < line.len) : (i += 1) {
        if (!isSpaceOrTab(line[i])) return false;
    }
    return true;
}

fn asciiTrimSpace(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn findBalancedJsonSlice(text: string): string? {
    let i: usize = 0;
    while (i < text.len) {
        const ch = text[i];
        if (ch != '{' && ch != '[') {
            i += 1;
            continue;
        }
        const open_char = ch;
        const close_char: u8 = if (open_char == '{') '}' else ']';
        let depth: usize = 0;
        let in_string = false;
        let escaped = false;
        let j = i;
        while (j < text.len) : (j += 1) {
            const c = text[j];
            if (in_string) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_string = false;
                }
                continue;
            }
            if (c == '"') {
                in_string = true;
                continue;
            }
            if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                if (depth == 0) break;
                depth -= 1;
                if (depth == 0) {
                    if (c == close_char) {
                        return text[i..(j + 1)];
                    }
                    break;
                }
            }
        }
        i += 1;
    }
    return null;
}

fn isXmlWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

struct OpenTagSpan {
    tag_start: usize = 0,
    content_start: usize = 0,
}

fn findNextOpeningTag(text: string, start_pos: usize, tag_name: string): OpenTagSpan? {
    if (tag_name.len == 0) return null;
    let pos = start_pos;
    while (pos + 1 + tag_name.len < text.len) {
        const lt_opt = std.mem.indexOfPos(u8, text, pos, "<");
        if (lt_opt == null) return null;
        const lt = lt_opt.?;
        const name_start = lt + 1;
        if (name_start + tag_name.len >= text.len) return null;
        if (std.mem.eql(u8, text[name_start..(name_start + tag_name.len)], tag_name)) {
            let after = name_start + tag_name.len;
            if (text[after] == '>') {
                return OpenTagSpan{
                    tag_start: lt,
                    content_start: after + 1,
                };
            }
            if (isXmlWs(text[after])) {
                // Scan until closing '>' respecting quoted attribute values.
                let in_quote: u8 = 0;
                while (after < text.len) : (after += 1) {
                    const c = text[after];
                    if (in_quote != 0) {
                        if (c == in_quote) in_quote = 0;
                    } else if (c == '"' || c == '\'') {
                        in_quote = c;
                    } else if (c == '>') {
                        return OpenTagSpan{
                            tag_start: lt,
                            content_start: after + 1,
                        };
                    }
                }
            }
        }
        pos = lt + 1;
    }
    return null;
}

struct CloseTagSpan {
    content_end: usize = 0,
    tag_end: usize = 0,
}

fn findNextClosingTag(text: string, start_pos: usize, tag_name: string): CloseTagSpan? {
    if (tag_name.len == 0) return null;
    let pos = start_pos;
    while (pos + 2 + tag_name.len < text.len) {
        const close_opt = std.mem.indexOfPos(u8, text, pos, "</");
        if (close_opt == null) return null;
        const lt = close_opt.?;
        const name_start = lt + 2;
        if (name_start + tag_name.len >= text.len) return null;
        if (std.mem.eql(u8, text[name_start..(name_start + tag_name.len)], tag_name)) {
            let after = name_start + tag_name.len;
            while (after < text.len && isXmlWs(text[after])) : (after += 1) {}
            if (after < text.len && text[after] == '>') {
                return CloseTagSpan{
                    content_end: lt,
                    tag_end: after + 1,
                };
            }
        }
        pos = lt + 2;
    }
    return null;
}

fn contentEndBeforeClosingLine(text: string, content_start: usize, closing_line_start: usize): usize {
    if (closing_line_start <= content_start) return content_start;
    let end = closing_line_start;
    if (end > content_start && text[end - 1] == '\n') {
        end -= 1;
        if (end > content_start && text[end - 1] == '\r') {
            end -= 1;
        }
    }
    return end;
}

/// Zero-copy LLM response extractor for Markdown code blocks, balanced JSON payloads,
/// XML reasoning/tool-call tags, and prompt-delimiter injection protection.
pub static struct PromptExtractor {
    /// Extracts all properly closed fenced Markdown code blocks (````lang\n...\n````) in `text`.
    /// Caller owns the returned slice and must free it with `alloc.free(blocks)`.
    static fn extractCodeBlocks(alloc: std.mem.Allocator, text: string): ([]CodeBlock) ! PromptError {
        let blocks: std.ArrayList(CodeBlock) = .empty;
        errdefer blocks.deinit(alloc);

        let pos: usize = 0;
        let in_fence = false;
        let fence_ticks: usize = 0;
        let fence_lang: string = "";
        let content_start: usize = 0;

        while (pos < text.len) {
            const span = nextLineSpan(text, pos);
            const line = text[span.start..span.end];

            if (!in_fence) {
                const open = parseOpeningFence(line);
                if (open.matched) {
                    in_fence = true;
                    fence_ticks = open.fence_len;
                    fence_lang = open.lang;
                    content_start = span.next_start;
                }
            } else {
                if (isClosingFence(line, fence_ticks)) {
                    const content_end = contentEndBeforeClosingLine(text, content_start, span.start);
                    blocks.append(alloc, CodeBlock{
                        lang: fence_lang,
                        content: text[content_start..content_end],
                    }) catch return PromptError.OutOfMemory;
                    in_fence = false;
                    fence_ticks = 0;
                    fence_lang = "";
                }
            }
            pos = span.next_start;
        }

        const slice = blocks.toOwnedSlice(alloc) catch return PromptError.OutOfMemory;
        return slice;
    }

    /// Zero-allocation extraction of the first fenced code block matching `lang_filter`
    /// (case-insensitive). Pass `""` to match the first code block of any language.
    static fn extractFirstCodeBlock(text: string, lang_filter: string): CodeBlock? {
        let pos: usize = 0;
        let in_fence = false;
        let fence_ticks: usize = 0;
        let fence_lang: string = "";
        let content_start: usize = 0;

        while (pos < text.len) {
            const span = nextLineSpan(text, pos);
            const line = text[span.start..span.end];

            if (!in_fence) {
                const open = parseOpeningFence(line);
                if (open.matched) {
                    in_fence = true;
                    fence_ticks = open.fence_len;
                    fence_lang = open.lang;
                    content_start = span.next_start;
                }
            } else {
                if (isClosingFence(line, fence_ticks)) {
                    const content_end = contentEndBeforeClosingLine(text, content_start, span.start);
                    const matches_lang = (lang_filter.len == 0) || std.ascii.eqlIgnoreCase(fence_lang, lang_filter);
                    if (matches_lang) {
                        return CodeBlock{
                            lang: fence_lang,
                            content: text[content_start..content_end],
                        };
                    }
                    in_fence = false;
                    fence_ticks = 0;
                    fence_lang = "";
                }
            }
            pos = span.next_start;
        }
        return null;
    }

    /// Extracts the first JSON object (`{...}`) or array (`[...]`) from `text`.
    /// Prioritizes ````json` fenced code blocks, falling back to a string-aware balanced
    /// bracket scan across raw prose. Returns `PromptError.JsonNotFound` if none exists.
    static fn extractFirstJson(text: string): string ! PromptError {
        if (blk := PromptExtractor.extractFirstCodeBlock(text, "json")) {
            const trimmed = asciiTrimSpace(blk.content);
            if (trimmed.len > 0) {
                if (balanced := findBalancedJsonSlice(trimmed)) {
                    return balanced;
                }
            }
        }

        if (raw := findBalancedJsonSlice(text)) {
            return raw;
        }
        return PromptError.JsonNotFound;
    }

    /// Extracts the first `<tag_name>...</tag_name>` block from `text`.
    /// Returns `PromptError.TagNotFound` if the opening or closing tag is absent.
    static fn extractXmlTag(text: string, tag_name: string): XmlTagMatch ! PromptError {
        const open_opt = findNextOpeningTag(text, 0, tag_name);
        if (open_opt == null) return PromptError.TagNotFound;
        const open = open_opt.?;
        const close_opt = findNextClosingTag(text, open.content_start, tag_name);
        if (close_opt == null) return PromptError.TagNotFound;
        const close = close_opt.?;
        return XmlTagMatch{
            tag: tag_name,
            content: text[open.content_start..close.content_end],
            start_offset: open.tag_start,
            end_offset: close.tag_end,
        };
    }

    /// Extracts all complete `<tag_name>...</tag_name>` blocks from `text` in document order.
    /// Caller owns the returned slice and must free it with `alloc.free(matches)`.
    static fn extractAllXmlTags(
        alloc: std.mem.Allocator,
        text: string,
        tag_name: string,
    ): ([]XmlTagMatch) ! PromptError {
        let list: std.ArrayList(XmlTagMatch) = .empty;
        errdefer list.deinit(alloc);

        let pos: usize = 0;
        while (pos < text.len) {
            const open = findNextOpeningTag(text, pos, tag_name) ?? break;
            const close = findNextClosingTag(text, open.content_start, tag_name) ?? break;
            list.append(alloc, XmlTagMatch{
                tag: tag_name,
                content: text[open.content_start..close.content_end],
                start_offset: open.tag_start,
                end_offset: close.tag_end,
            }) catch return PromptError.OutOfMemory;
            pos = close.tag_end;
        }

        const slice = list.toOwnedSlice(alloc) catch return PromptError.OutOfMemory;
        return slice;
    }

    /// Escapes `<tag_name...>` and `</tag_name...>` occurrences (case-insensitive) into
    /// `&lt;...&gt;` entities so untrusted text cannot forge prompt delimiter boundaries.
    /// Caller owns the returned slice and must free it with `alloc.free(escaped)`.
    static fn escapeXmlDelimiter(
        alloc: std.mem.Allocator,
        text: string,
        tag_name: string,
    ): ([]u8) ! PromptError {
        if (tag_name.len == 0) {
            const copy = alloc.dupe(u8, text) catch return PromptError.OutOfMemory;
            return copy;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < text.len) {
            if (text[i] == '<') {
                let name_start = i + 1;
                let is_close = false;
                if (name_start < text.len && text[name_start] == '/') {
                    is_close = true;
                    name_start += 1;
                }
                if (name_start + tag_name.len <= text.len &&
                    std.ascii.eqlIgnoreCase(text[name_start..(name_start + tag_name.len)], tag_name))
                {
                    const after = name_start + tag_name.len;
                    const boundary_ok = (after == text.len) || text[after] == '>' || isXmlWs(text[after]);
                    if (boundary_ok) {
                        if (i > flush_start) {
                            out.appendSlice(alloc, text[flush_start..i]) catch return PromptError.OutOfMemory;
                        }
                        out.appendSlice(alloc, "&lt;") catch return PromptError.OutOfMemory;
                        if (is_close) {
                            out.append(alloc, '/') catch return PromptError.OutOfMemory;
                        }
                        out.appendSlice(alloc, text[name_start..after]) catch return PromptError.OutOfMemory;

                        // If a matching '>' follows on this tag, escape it as '&gt;' too.
                        let scan = after;
                        let found_gt = false;
                        while (scan < text.len) : (scan += 1) {
                            if (text[scan] == '>') {
                                if (scan > after) {
                                    out.appendSlice(alloc, text[after..scan]) catch return PromptError.OutOfMemory;
                                }
                                out.appendSlice(alloc, "&gt;") catch return PromptError.OutOfMemory;
                                i = scan + 1;
                                flush_start = i;
                                found_gt = true;
                                break;
                            }
                            if (text[scan] == '<') break;
                        }
                        if (!found_gt) {
                            i = after;
                            flush_start = after;
                        }
                        continue;
                    }
                }
            }
            i += 1;
        }
        if (i > flush_start) {
            out.appendSlice(alloc, text[flush_start..i]) catch return PromptError.OutOfMemory;
        }

        const slice = out.toOwnedSlice(alloc) catch return PromptError.OutOfMemory;
        return slice;
    }

    /// Escapes any embedded `<tag_name>` / `</tag_name>` boundaries inside `untrusted_text`
    /// and wraps the sanitized payload in `<tag_name>...</tag_name>`.
    /// Caller owns the returned slice and must free it with `alloc.free(wrapped)`.
    static fn wrapDelimited(
        alloc: std.mem.Allocator,
        tag_name: string,
        untrusted_text: string,
    ): ([]u8) ! PromptError {
        const escaped = PromptExtractor.escapeXmlDelimiter(alloc, untrusted_text, tag_name)?;
        defer alloc.free(escaped);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.append(alloc, '<') catch return PromptError.OutOfMemory;
        out.appendSlice(alloc, tag_name) catch return PromptError.OutOfMemory;
        out.append(alloc, '>') catch return PromptError.OutOfMemory;
        out.appendSlice(alloc, escaped) catch return PromptError.OutOfMemory;
        out.appendSlice(alloc, "</") catch return PromptError.OutOfMemory;
        out.appendSlice(alloc, tag_name) catch return PromptError.OutOfMemory;
        out.append(alloc, '>') catch return PromptError.OutOfMemory;

        const slice = out.toOwnedSlice(alloc) catch return PromptError.OutOfMemory;
        return slice;
    }
}
