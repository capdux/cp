// Source: Absorbed from /data/dev/bun/src/md/inlines.cp | Synced: 2026-11-20T09:22:20Z
// lib/md/inlines.cp
// CommonMark & GFM Inline Delimiter Stack Resolution, Code Spans, Entities, Autolinks & Links

const std = @import("std");
import { findPermissiveAutolink, isEmphBoundaryResolved } from "./autolinks.cp";
import { findEntity, isAsciiPunctuation } from "./helpers.cp";
import {
    canCloseEmphasis,
    canOpenEmphasis,
    countBackticks,
    emitEmphCloseTags,
    emitEmphOpenTags,
    emitText,
    enterSpan,
    findCodeSpanEnd,
    findHtmlTag,
    leaveSpan,
    normalizeCodeSpanContent,
} from "./inline_core.cp";
import {
    findAutolink,
    labelContainsLink,
    processLink,
    processWikiLink,
    renderAutolink,
    tryMatchBracketLink,
} from "./links.cp";
import { EmphDelim, MAX_EMPH_MATCHES, Md4cParserState, SpanDetail, VerbatimLine } from "./types.cp";

pub fn processLeafBlock(p: *Md4cParserState, block_lines: []const VerbatimLine, trim_trailing: bool): ! {
    if (block_lines.len == 0) return;

    p.buffer.clearRetainingCapacity();

    for (vline of block_lines) {
        if (vline.beg > vline.end || vline.end > p.size) continue;
        if (p.buffer.items.len > 0) {
            p.buffer.append(p.allocator, '\n') catch {};
        }
        p.buffer.appendSlice(p.allocator, p.text[vline.beg..vline.end]) catch {};
    }

    let merged = p.buffer.items;
    if (trim_trailing) {
        while (merged.len > 0 && (merged[merged.len - 1] == ' ' || merged[merged.len - 1] == '\t')) {
            merged = merged[0 .. merged.len - 1];
        }
    }
    processInlineContent(p, merged, block_lines[0].beg)?;
}

pub fn collectEmphasisDelimiters(p: *Md4cParserState, content: []const u8) {
    p.emph_delims.clearRetainingCapacity();
    let i: usize = 0;
    while (i < content.len) {
        const c = content[i];
        if (c == '\\' && i + 1 < content.len && isAsciiPunctuation(content[i + 1])) {
            i += 2;
            continue;
        }
        if (c == '`') {
            const count = countBackticks(content, i);
            if (end_pos := findCodeSpanEnd(content, i + count, count)) {
                i = end_pos + count;
            } else {
                i += count;
            }
            continue;
        }
        if (c == '<') {
            if (!p.flags.no_html_spans) {
                if (tag_end := findHtmlTag(content, i)) {
                    i = tag_end;
                    continue;
                }
                if (al := findAutolink(p, content, i)) {
                    i = al.end_pos;
                    continue;
                }
            }
        }
        if (c == '[' || (c == '!' && i + 1 < content.len && content[i + 1] == '[')) {
            const is_img = c == '!';
            const bracket_start = if (is_img) i + 1 else i;
            const link_result = tryMatchBracketLink(p, content, bracket_start);
            if (link_result.is_link) {
                if (!is_img) {
                    const label = content[bracket_start + 1 .. link_result.label_end];
                    if (labelContainsLink(p, label)) {
                        i += 1;
                        continue;
                    }
                }
                i = link_result.link_end;
                continue;
            }
        }
        if (c == '*' || c == '_') {
            const run_start = i;
            while (i < content.len && content[i] == c) i += 1;
            const count = i - run_start;
            p.emph_delims.append(p.allocator, EmphDelim{
                pos: run_start,
                count: count,
                emph_char: c,
                can_open: canOpenEmphasis(c, content, run_start, i),
                can_close: canCloseEmphasis(c, content, run_start, i),
                remaining: count,
            }) catch {};
            continue;
        }
        if (c == '~' && p.flags.strikethrough) {
            const run_start = i;
            while (i < content.len && content[i] == '~') i += 1;
            const count = i - run_start;
            if (count == 1 || count == 2) {
                p.emph_delims.append(p.allocator, EmphDelim{
                    pos: run_start,
                    count: count,
                    emph_char: '~',
                    can_open: canOpenEmphasis('~', content, run_start, i),
                    can_close: canCloseEmphasis('~', content, run_start, i),
                    remaining: count,
                }) catch {};
            }
            continue;
        }
        i += 1;
    }
}

pub fn resolveEmphasisDelimiters(p: *Md4cParserState) {
    let delims = p.emph_delims.items;
    if (delims.len == 0) return;

    let closer_idx: usize = 0;
    while (closer_idx < delims.len) : (closer_idx += 1) {
        if (!delims[closer_idx].can_close || delims[closer_idx].remaining == 0) continue;

        let found_match = false;
        let reprocess_closer = false;
        if (closer_idx > 0) {
            let oi: usize = closer_idx;
            while (oi > 0) {
                oi -= 1;
                let opener = &delims[oi];
                if (opener.emph_char != delims[closer_idx].emph_char) continue;
                if (!opener.can_open || opener.remaining == 0 || !opener.active) continue;

                if (opener.emph_char == '~') {
                    if (opener.count != delims[closer_idx].count) continue;
                }

                if (opener.emph_char != '~' &&
                    (opener.can_close || delims[closer_idx].can_open) &&
                    (opener.count + delims[closer_idx].count) % 3 == 0 &&
                    opener.count % 3 != 0 && delims[closer_idx].count % 3 != 0)
                {
                    continue;
                }

                const use: usize = if (opener.emph_char == '~')
                    opener.remaining
                else if (opener.remaining >= 2 && delims[closer_idx].remaining >= 2)
                    2
                else
                    1;

                opener.remaining -= use;
                opener.open_count += use;
                if ((opener.open_num as usize) < MAX_EMPH_MATCHES) {
                    opener.open_sizes[opener.open_num] = @intCast(use);
                    opener.open_num += 1;
                }
                delims[closer_idx].remaining -= use;
                delims[closer_idx].close_count += use;
                if ((delims[closer_idx].close_num as usize) < MAX_EMPH_MATCHES) {
                    delims[closer_idx].close_sizes[delims[closer_idx].close_num] = @intCast(use);
                    delims[closer_idx].close_num += 1;
                }

                let k = oi + 1;
                while (k < closer_idx) : (k += 1) {
                    delims[k].active = false;
                }

                found_match = true;
                if (delims[closer_idx].remaining > 0 && delims[closer_idx].can_close) {
                    reprocess_closer = true;
                }
                break;
            }
        }

        if (!found_match && !delims[closer_idx].can_open) {
            delims[closer_idx].active = false;
        }
        if (reprocess_closer) {
            closer_idx = closer_idx -% 1;
        }
    }
}

pub fn processInlineContent(p: *Md4cParserState, content: []const u8, base_off: u32): ! {
    if (p.recursion_depth >= p.max_recursion_depth) {
        return error.StackOverflow;
    }
    p.recursion_depth += 1;
    defer {
        p.recursion_depth -= 1;
    }

    if (p.process_inline_ptr == 0) {
        zig { p.process_inline_ptr = @intFromPtr(&processInlineContent); }
    }

    collectEmphasisDelimiters(p, content);
    resolveEmphasisDelimiters(p);

    const resolved = p.allocator.dupe(EmphDelim, p.emph_delims.items) catch {
        emitText(p, .normal, content)?;
        return;
    };
    defer p.allocator.free(resolved);

    let i: usize = 0;
    let text_start: usize = 0;
    let delim_cursor: usize = 0;

    while (i < content.len) {
        const c = content[i];

        if (!p.mark_char_map.isSet(c as usize)) {
            i += 1;
            continue;
        }

        if (c == '\n') {
            let emit_end = i;
            let is_hard = false;
            if (emit_end > text_start && content[emit_end - 1] == '\\') {
                emit_end -= 1;
                is_hard = true;
            } else {
                let sp = emit_end;
                while (sp > text_start && content[sp - 1] == ' ') sp -= 1;
                if (emit_end - sp >= 2) {
                    while (sp > text_start && (content[sp - 1] == ' ' || content[sp - 1] == '\t')) sp -= 1;
                    emit_end = sp;
                    is_hard = true;
                }
            }
            if (emit_end > text_start) emitText(p, .normal, content[text_start..emit_end])?;
            if (is_hard) {
                emitText(p, .br, "")?;
            } else {
                emitText(p, .softbr, "")?;
            }
            i += 1;
            text_start = i;
            continue;
        }

        if (c == '\\' && i + 1 < content.len && isAsciiPunctuation(content[i + 1])) {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            i += 1;
            emitText(p, .normal, content[i .. i + 1])?;
            i += 1;
            text_start = i;
            continue;
        }

        if (c == '`') {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            const count = countBackticks(content, i);
            if (end_pos := findCodeSpanEnd(content, i + count, count)) {
                enterSpan(p, .code)?;
                const code_content = normalizeCodeSpanContent(content[i + count .. end_pos]);
                emitText(p, .code, code_content)?;
                leaveSpan(p, .code)?;
                i = end_pos + count;
            } else {
                emitText(p, .normal, content[i .. i + count])?;
                i += count;
            }
            text_start = i;
            continue;
        }

        if (c == '*' || c == '_' || (c == '~' && p.flags.strikethrough)) {
            while (delim_cursor < resolved.len && resolved[delim_cursor].pos < i) delim_cursor += 1;

            if (delim_cursor < resolved.len && resolved[delim_cursor].pos == i) {
                if (i > text_start) emitText(p, .normal, content[text_start..i])?;

                const d = &resolved[delim_cursor];
                const run_end = d.pos + d.count;

                if (d.emph_char == '~') {
                    if (d.close_count > 0) leaveSpan(p, .del)?;
                } else {
                    emitEmphCloseTags(p, d.close_sizes[0..(d.close_num as usize)])?;
                }

                const text_chars = d.count -| (d.open_count + d.close_count);
                if (text_chars > 0) {
                    emitText(p, .normal, content[i .. i + text_chars])?;
                }

                if (d.emph_char == '~') {
                    if (d.open_count > 0) enterSpan(p, .del)?;
                } else {
                    emitEmphOpenTags(p, d.open_sizes[0..(d.open_num as usize)])?;
                }

                delim_cursor += 1;
                i = run_end;
                text_start = i;
                continue;
            }
            i += 1;
            continue;
        }

        if (c == '&') {
            if (end_pos := findEntity(content, i)) {
                if (i > text_start) emitText(p, .normal, content[text_start..i])?;
                emitText(p, .entity, content[i..end_pos])?;
                i = end_pos;
                text_start = i;
                continue;
            }
        }

        if (c == '<' && !p.flags.no_html_spans) {
            if (tag_end := findHtmlTag(content, i)) {
                if (i > text_start) emitText(p, .normal, content[text_start..i])?;
                emitText(p, .html, content[i..tag_end])?;
                i = tag_end;
                text_start = i;
                continue;
            }
            if (autolink := findAutolink(p, content, i)) {
                if (i > text_start) emitText(p, .normal, content[text_start..i])?;
                renderAutolink(p, content[i + 1 .. autolink.end_pos - 1], autolink.is_email)?;
                i = autolink.end_pos;
                text_start = i;
                continue;
            }
        }

        if (c == '[' && p.flags.wiki_links && i + 1 < content.len && content[i + 1] == '[') {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            if (end_pos := processWikiLink(p, content, i)?) {
                i = end_pos;
                text_start = i;
                continue;
            }
            text_start = i;
        }

        if (c == '[') {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            if (end_pos := processLink(p, content, i, base_off, false)?) {
                i = end_pos;
            } else {
                emitText(p, .normal, "[")?;
                i += 1;
            }
            text_start = i;
            continue;
        }

        if (c == '!' && i + 1 < content.len && content[i + 1] == '[') {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            if (end_pos := processLink(p, content, i + 1, base_off, true)?) {
                i = end_pos;
            } else {
                emitText(p, .normal, "!")?;
                i += 1;
            }
            text_start = i;
            continue;
        }

        if (p.link_nesting_level == 0 &&
            ((c == ':' && p.flags.permissive_url_autolinks) ||
                (c == '@' && p.flags.permissive_email_autolinks) ||
                (c == '.' && p.flags.permissive_www_autolinks)))
        {
            let al = findPermissiveAutolink(content, i, false);
            if (al == null) {
                al = findPermissiveAutolink(content, i, true);
                if (a := al) {
                    if (!isEmphBoundaryResolved(content, a, resolved)) {
                        al = null;
                    }
                }
            }
            if (a := al) {
                if (a.beg > text_start) emitText(p, .normal, content[text_start..a.beg])?;

                const link_text = content[a.beg..a.end];
                if (c == '@') {
                    p.renderer.enterSpan(.a, SpanDetail{ href: link_text, permissive_autolink: true, autolink_email: true })?;
                    emitText(p, .normal, link_text)?;
                    p.renderer.leaveSpan(.a)?;
                } else if (c == '.') {
                    p.renderer.enterSpan(.a, SpanDetail{ href: link_text, permissive_autolink: true, autolink_www: true })?;
                    emitText(p, .normal, link_text)?;
                    p.renderer.leaveSpan(.a)?;
                } else {
                    p.renderer.enterSpan(.a, SpanDetail{ href: link_text, permissive_autolink: true })?;
                    emitText(p, .normal, link_text)?;
                    p.renderer.leaveSpan(.a)?;
                }
                i = a.end;
                text_start = i;
                continue;
            }
        }

        if (c == 0) {
            if (i > text_start) emitText(p, .normal, content[text_start..i])?;
            emitText(p, .null_char, "")?;
            i += 1;
            text_start = i;
            continue;
        }

        i += 1;
    }

    if (text_start < content.len) {
        emitText(p, .normal, content[text_start..])?;
    }
}
