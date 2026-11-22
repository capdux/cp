// Source: Absorbed from /data/dev/bun/src/md/links.cp | Synced: 2026-11-20T09:22:11Z
// lib/md/links.cp
// CommonMark Inline Links, Reference Links, Shortcut Links, Images, Wiki Links & Angle Autolinks

const std = @import("std");
import { isAlpha, isAlphaNum, isBlank, isWhitespace } from "./helpers.cp";
import { countBackticks, emitText, findCodeSpanEnd, findHtmlTag } from "./inlineCore.cp";
import { lookupRefDef } from "./refDefs.cp";
import { Md4cParserState, MdRefDef, SpanDetail } from "./types.cp";

pub struct BracketLinkResult {
    is_link: bool = false,
    label_end: usize = 0,
    link_end: usize = 0,
}

pub struct AutolinkResult {
    end_pos: usize = 0,
    is_email: bool = false,
}

fn invokeProcessInline(p: *Md4cParserState, content: []const u8, base_off: u32): ! {
    if (p.process_inline_ptr == 0) {
        emitText(p, .normal, content)?;
        return;
    }
    zig {
        const cb: fn (*Md4cParserState, []const u8, u32) anyerror!void = @ptrFromInt(p.process_inline_ptr);
        try cb(p, content, base_off);
    }
}

pub fn findAutolink(p: *const Md4cParserState, content: []const u8, start: usize): AutolinkResult? {
    * = p;
    if (start + 1 >= content.len) return null;

    const pos = start + 1;

    if (isAlpha(content[pos])) {
        let scheme_end = pos;
        while (scheme_end < content.len && (isAlphaNum(content[scheme_end]) ||
            content[scheme_end] == '+' || content[scheme_end] == '-' || content[scheme_end] == '.'))
        {
            scheme_end += 1;
        }
        const scheme_len = scheme_end - pos;
        if (scheme_len >= 2 && scheme_len <= 32 && scheme_end < content.len && content[scheme_end] == ':') {
            let uri_end = scheme_end + 1;
            while (uri_end < content.len && content[uri_end] != '>' && !isWhitespace(content[uri_end])) {
                uri_end += 1;
            }
            if (uri_end < content.len && content[uri_end] == '>') {
                return AutolinkResult{ end_pos: uri_end + 1, is_email: false };
            }
        }

        let email_pos = pos;
        while (email_pos < content.len && (isAlphaNum(content[email_pos]) ||
            content[email_pos] == '.' || content[email_pos] == '-' ||
            content[email_pos] == '_' || content[email_pos] == '+'))
        {
            email_pos += 1;
        }
        if (email_pos < content.len && content[email_pos] == '@' && email_pos > pos) {
            email_pos += 1;
            const domain_start = email_pos;
            let label_len: u32 = 0;
            let dot_count: u32 = 0;
            let valid_domain = true;
            while (email_pos < content.len && (isAlphaNum(content[email_pos]) ||
                content[email_pos] == '.' || content[email_pos] == '-'))
            {
                if (content[email_pos] == '.') {
                    if (label_len == 0) {
                        valid_domain = false;
                        break;
                    }
                    label_len = 0;
                    dot_count += 1;
                } else {
                    label_len += 1;
                    if (label_len > 63) {
                        valid_domain = false;
                        break;
                    }
                }
                email_pos += 1;
            }
            if (valid_domain && email_pos < content.len && content[email_pos] == '>' &&
                email_pos > domain_start && label_len > 0 && dot_count > 0 &&
                isAlphaNum(content[email_pos - 1]))
            {
                return AutolinkResult{ end_pos: email_pos + 1, is_email: true };
            }
        }
    }

    return null;
}

pub fn renderAutolink(p: *Md4cParserState, url: []const u8, is_email: bool): ! {
    p.renderer.enterSpan(.a, SpanDetail{ href: url, autolink: true, autolink_email: is_email })?;
    emitText(p, .normal, url)?;
    p.renderer.leaveSpan(.a)?;
}

pub fn tryMatchBracketLink(p: *Md4cParserState, content: []const u8, start: usize): BracketLinkResult {
    let pos = start + 1;
    let depth: u32 = 1;
    while (pos < content.len && depth > 0) {
        if (content[pos] == '\\' && pos + 1 < content.len) {
            pos += 2;
            continue;
        }
        if (content[pos] == '`') {
            const count = countBackticks(content, pos);
            if (end_pos := findCodeSpanEnd(content, pos + count, count)) {
                pos = end_pos + count;
                continue;
            }
        }
        if (content[pos] == '<' && !p.flags.no_html_spans) {
            if (tag_end := findHtmlTag(content, pos)) {
                pos = tag_end;
                continue;
            }
            if (al := findAutolink(p, content, pos)) {
                pos = al.end_pos;
                continue;
            }
        }
        if (content[pos] == '[') depth += 1;
        if (content[pos] == ']') depth -= 1;
        if (depth > 0) pos += 1;
    }
    if (depth != 0) return BracketLinkResult{};

    const label_end = pos;
    pos += 1;

    if (pos >= content.len) {
        const inner_label = content[start + 1 .. label_end];
        const is_ref = lookupRefDef(p, inner_label) != null;
        return BracketLinkResult{ is_link: is_ref, label_end: label_end, link_end: label_end + 1 };
    }

    if (content[pos] == '(') {
        let cur = pos + 1;
        while (cur < content.len && (isBlank(content[cur]) || content[cur] == '\n' || content[cur] == '\r')) cur += 1;
        if (cur < content.len && content[cur] == '<') {
            cur += 1;
            while (cur < content.len && content[cur] != '>' && content[cur] != '\n') {
                if (content[cur] == '\\' && cur + 1 < content.len) {
                    cur += 2;
                } else {
                    cur += 1;
                }
            }
            if (cur < content.len && content[cur] == '>') {
                cur += 1;
            } else {
                return BracketLinkResult{ is_link: false, label_end: label_end, link_end: label_end + 1 };
            }
        } else {
            let paren_depth: u32 = 0;
            while (cur < content.len && !isWhitespace(content[cur])) {
                if (content[cur] == '(') {
                    paren_depth += 1;
                } else if (content[cur] == ')') {
                    if (paren_depth == 0) break;
                    paren_depth -= 1;
                }
                if (content[cur] == '\\' && cur + 1 < content.len) {
                    cur += 2;
                } else {
                    cur += 1;
                }
            }
        }
        while (cur < content.len && (isBlank(content[cur]) || content[cur] == '\n' || content[cur] == '\r')) cur += 1;
        if (cur < content.len && (content[cur] == '"' || content[cur] == '\'' || content[cur] == '(')) {
            const close_ch: u8 = if (content[cur] == '(') ')' else content[cur];
            cur += 1;
            while (cur < content.len && content[cur] != close_ch) {
                if (content[cur] == '\\' && cur + 1 < content.len) {
                    cur += 2;
                } else {
                    cur += 1;
                }
            }
            if (cur < content.len) cur += 1;
        }
        while (cur < content.len && (isBlank(content[cur]) || content[cur] == '\n' || content[cur] == '\r')) cur += 1;
        if (cur < content.len && content[cur] == ')') {
            return BracketLinkResult{ is_link: true, label_end: label_end, link_end: cur + 1 };
        }
    }

    if (content[pos] == '[') {
        let cur = pos + 1;
        while (cur < content.len && content[cur] != ']') {
            if (content[cur] == '[') break;
            if (content[cur] == '\\' && cur + 1 < content.len) {
                cur += 2;
            } else {
                cur += 1;
            }
        }
        if (cur < content.len && content[cur] == ']') {
            const ref_label = if (cur > pos + 1) content[pos + 1 .. cur] else content[start + 1 .. label_end];
            if (lookupRefDef(p, ref_label) != null) {
                return BracketLinkResult{ is_link: true, label_end: label_end, link_end: cur + 1 };
            }
        }
    }

    const inner_label = content[start + 1 .. label_end];
    if (lookupRefDef(p, inner_label) != null) {
        return BracketLinkResult{ is_link: true, label_end: label_end, link_end: label_end + 1 };
    }

    return BracketLinkResult{ is_link: false, label_end: label_end, link_end: label_end + 1 };
}

pub fn labelContainsLink(p: *Md4cParserState, label: []const u8): bool {
    let pos: usize = 0;
    while (pos < label.len) {
        if (label[pos] == '\\' && pos + 1 < label.len) {
            pos += 2;
            continue;
        }
        if (label[pos] == '`') {
            const count = countBackticks(label, pos);
            if (end_pos := findCodeSpanEnd(label, pos + count, count)) {
                pos = end_pos + count;
                continue;
            }
        }
        if (label[pos] == '<' && !p.flags.no_html_spans) {
            if (tag_end := findHtmlTag(label, pos)) {
                pos = tag_end;
                continue;
            }
            if (al := findAutolink(p, label, pos)) {
                pos = al.end_pos;
                continue;
            }
        }
        if (label[pos] == '[') {
            const is_inner_image = pos > 0 && label[pos - 1] == '!';
            const inner = tryMatchBracketLink(p, label, pos);
            if (inner.is_link && !is_inner_image) return true;
            if (inner.link_end > pos) {
                pos = inner.link_end;
                continue;
            }
        }
        pos += 1;
    }
    return false;
}

pub fn renderRefLink(p: *Md4cParserState, label_content: []const u8, ref_def: MdRefDef, is_image: bool): ! {
    if (p.image_nesting_level > 0) {
        invokeProcessInline(p, label_content, 0)?;
    } else if (is_image) {
        p.renderer.enterSpan(.img, SpanDetail{ href: ref_def.dest, title: ref_def.title })?;
        p.image_nesting_level += 1;
        invokeProcessInline(p, label_content, 0)?;
        p.image_nesting_level -= 1;
        p.renderer.leaveSpan(.img)?;
    } else {
        p.renderer.enterSpan(.a, SpanDetail{ href: ref_def.dest, title: ref_def.title })?;
        p.link_nesting_level += 1;
        invokeProcessInline(p, label_content, 0)?;
        p.link_nesting_level -= 1;
        p.renderer.leaveSpan(.a)?;
    }
}

pub fn processWikiLink(p: *Md4cParserState, content: []const u8, start: usize): usize?! {
    let pos = start + 2;
    const inner_start = pos;
    let pipe_pos: usize? = null;
    let bracket_depth: u32 = 0;

    while (pos < content.len) {
        if (content[pos] == '\n' || content[pos] == '\r') return null;
        if (content[pos] == '[') {
            bracket_depth += 1;
        } else if (content[pos] == ']') {
            if (bracket_depth > 0) {
                bracket_depth -= 1;
            } else if (pos + 1 < content.len && content[pos + 1] == ']') {
                break;
            } else {
                return null;
            }
        } else if (content[pos] == '|' && pipe_pos == null && bracket_depth == 0) {
            pipe_pos = pos;
        }
        pos += 1;
    }

    if (pos >= content.len || content[pos] != ']') return null;
    const inner_end = pos;

    let target = content[inner_start..inner_end];
    let label = target;
    if (pp := pipe_pos) {
        target = content[inner_start..pp];
        label = content[pp + 1 .. inner_end];
    }

    if (target.len > 100) return null;

    p.renderer.enterSpan(.wikilink, SpanDetail{ href: target })?;
    invokeProcessInline(p, label, 0)?;
    p.renderer.leaveSpan(.wikilink)?;
    return pos + 2;
}

pub fn processLink(p: *Md4cParserState, content: []const u8, start: usize, base_off: u32, is_image: bool): usize?! {
    * = base_off;
    let pos = start + 1;
    let bracket_depth: u32 = 1;
    let has_inner_bracket = false;

    while (pos < content.len && bracket_depth > 0) {
        if (content[pos] == '\\' && pos + 1 < content.len) {
            pos += 2;
            continue;
        }
        if (content[pos] == '`') {
            const count = countBackticks(content, pos);
            if (end_pos := findCodeSpanEnd(content, pos + count, count)) {
                pos = end_pos + count;
                continue;
            }
        }
        if (content[pos] == '<' && !p.flags.no_html_spans) {
            if (tag_end := findHtmlTag(content, pos)) {
                pos = tag_end;
                continue;
            }
            if (autolink := findAutolink(p, content, pos)) {
                pos = autolink.end_pos;
                continue;
            }
        }
        if (content[pos] == '[') {
            bracket_depth += 1;
            has_inner_bracket = true;
        }
        if (content[pos] == ']') bracket_depth -= 1;
        if (bracket_depth > 0) pos += 1;
    }

    if (bracket_depth != 0) return null;

    const label_end = pos;
    const label = content[start + 1 .. label_end];
    pos += 1;

    if (pos < content.len && content[pos] == '(') {
        pos += 1;
        while (pos < content.len && (isBlank(content[pos]) || content[pos] == '\n' || content[pos] == '\r')) pos += 1;

        let dest_start = pos;
        let dest_end = pos;

        if (pos < content.len && content[pos] == '<') {
            dest_start = pos + 1;
            pos += 1;
            let angle_valid = true;
            while (pos < content.len && content[pos] != '>') {
                if (content[pos] == '\n' || content[pos] == '\r') {
                    angle_valid = false;
                    break;
                }
                if (content[pos] == '\\' && pos + 1 < content.len) {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            if (!angle_valid) return null;
            dest_end = pos;
            if (pos < content.len) pos += 1;
        } else {
            let paren_depth: u32 = 0;
            while (pos < content.len && !isWhitespace(content[pos])) {
                if (content[pos] == '(') {
                    paren_depth += 1;
                } else if (content[pos] == ')') {
                    if (paren_depth == 0) break;
                    paren_depth -= 1;
                }
                if (content[pos] == '\\' && pos + 1 < content.len) {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            dest_end = pos;
        }

        while (pos < content.len && (isBlank(content[pos]) || content[pos] == '\n' || content[pos] == '\r')) pos += 1;

        let title: []const u8 = "";
        if (pos < content.len && (content[pos] == '"' || content[pos] == '\'' || content[pos] == '(')) {
            const close_char: u8 = if (content[pos] == '(') ')' else content[pos];
            pos += 1;
            const title_start = pos;
            while (pos < content.len && content[pos] != close_char) {
                if (content[pos] == '\\' && pos + 1 < content.len) {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            title = content[title_start..pos];
            if (pos < content.len) pos += 1;
        }

        while (pos < content.len && (isBlank(content[pos]) || content[pos] == '\n' || content[pos] == '\r')) pos += 1;

        if (pos < content.len && content[pos] == ')') {
            pos += 1;
            const dest = content[dest_start..dest_end];

            if (!is_image && has_inner_bracket && labelContainsLink(p, label)) {
                return null;
            }

            if (p.image_nesting_level > 0) {
                invokeProcessInline(p, label, 0)?;
            } else if (is_image) {
                p.renderer.enterSpan(.img, SpanDetail{ href: dest, title: title })?;
                p.image_nesting_level += 1;
                invokeProcessInline(p, label, 0)?;
                p.image_nesting_level -= 1;
                p.renderer.leaveSpan(.img)?;
            } else {
                p.renderer.enterSpan(.a, SpanDetail{ href: dest, title: title })?;
                p.link_nesting_level += 1;
                invokeProcessInline(p, label, 0)?;
                p.link_nesting_level -= 1;
                p.renderer.leaveSpan(.a)?;
            }

            return pos;
        }
    }

    if (pos < content.len && content[pos] == '[') {
        const bracket_pos = pos;
        pos += 1;
        const ref_start = pos;
        while (pos < content.len && content[pos] != ']') {
            if (content[pos] == '[') break;
            if (content[pos] == '\\' && pos + 1 < content.len) {
                pos += 2;
            } else {
                pos += 1;
            }
        }
        if (pos < content.len && content[pos] == ']') {
            const ref_label = if (pos > ref_start) content[ref_start..pos] else label;
            pos += 1;
            if (ref_def := lookupRefDef(p, ref_label)) {
                if (!is_image && has_inner_bracket && labelContainsLink(p, label)) {
                    return null;
                }
                renderRefLink(p, label, ref_def, is_image)?;
                return pos;
            }
        } else {
            pos = bracket_pos;
        }
    }

    const char_after_label: u8 = if (label_end + 1 < content.len) content[label_end + 1] else 0;
    if (char_after_label != '[') {
        if (ref_def := lookupRefDef(p, label)) {
            if (!is_image && has_inner_bracket && labelContainsLink(p, label)) {
                return null;
            }
            renderRefLink(p, label, ref_def, is_image)?;
            return label_end + 1;
        }
    }

    return null;
}
