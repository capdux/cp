// @cap/dompurify — Zero-allocation HTML sanitizer in pure CP
// Clean-room Apache-2.0 implementation inspired by npm `dompurify`.

import { toLowerByte, isSpace, isAlnum, eqIgnoreCase, toLower } from "cap:ascii";
import { writeStr } from "cap:fmt";

/// Configuration options for `sanitizeHtml`.
pub struct SanitizeHtmlOptions {
    allow_links: bool,
    allow_images: bool,
    strip_comments: bool,
}

/// Default sanitization options (`allow_links: true`, `allow_images: true`, `strip_comments: true`).
pub fn defaultSanitizeOptions(): SanitizeHtmlOptions {
    return SanitizeHtmlOptions {
        allow_links: true,
        allow_images: true,
        strip_comments: true,
    };
}

fn dmpIsNameChar(c: u8): bool {
    return isAlnum(c) || c == '-' || c == '_';
}

fn dmpHasPrefixCI(s: string, prefix: string): bool {
    if (prefix.len > s.len) {
        return false;
    }
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn dmpWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn dmpIsBlockedContentTag(tag: string): bool {
    return eqIgnoreCase(tag, "script") ||
        eqIgnoreCase(tag, "style") ||
        eqIgnoreCase(tag, "iframe") ||
        eqIgnoreCase(tag, "object") ||
        eqIgnoreCase(tag, "form");
}

fn dmpIsBlockedVoidTag(tag: string): bool {
    return eqIgnoreCase(tag, "embed");
}

fn dmpIsAllowedTag(tag: string, opts: SanitizeHtmlOptions): bool {
    if (eqIgnoreCase(tag, "a")) {
        return opts.allow_links;
    }
    if (eqIgnoreCase(tag, "img")) {
        return opts.allow_images;
    }
    return eqIgnoreCase(tag, "p") ||
        eqIgnoreCase(tag, "b") ||
        eqIgnoreCase(tag, "i") ||
        eqIgnoreCase(tag, "em") ||
        eqIgnoreCase(tag, "strong") ||
        eqIgnoreCase(tag, "ul") ||
        eqIgnoreCase(tag, "ol") ||
        eqIgnoreCase(tag, "li") ||
        eqIgnoreCase(tag, "code") ||
        eqIgnoreCase(tag, "pre") ||
        eqIgnoreCase(tag, "div") ||
        eqIgnoreCase(tag, "span") ||
        eqIgnoreCase(tag, "h1") ||
        eqIgnoreCase(tag, "h2") ||
        eqIgnoreCase(tag, "h3") ||
        eqIgnoreCase(tag, "h4") ||
        eqIgnoreCase(tag, "h5") ||
        eqIgnoreCase(tag, "h6") ||
        eqIgnoreCase(tag, "br") ||
        eqIgnoreCase(tag, "hr");
}

fn dmpIsEventAttr(attr: string): bool {
    return attr.len >= 3 && toLowerByte(attr[0]) == 'o' && toLowerByte(attr[1]) == 'n';
}

fn dmpIsSafeUrl(url: string): bool {
    // Skip leading whitespace and ASCII control chars
    let start: usize = 0;
    while (start < url.len && url[start] <= 0x20) : (start += 1) {}
    const trimmed: string = url[start..url.len];
    if (dmpHasPrefixCI(trimmed, "javascript:")) {
        return false;
    }
    if (dmpHasPrefixCI(trimmed, "vbscript:")) {
        return false;
    }
    if (dmpHasPrefixCI(trimmed, "data:")) {
        return false;
    }
    return true;
}

fn dmpIsSafeAttr(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool {
    if (dmpIsEventAttr(attr)) {
        return false;
    }
    if (eqIgnoreCase(attr, "href")) {
        return opts.allow_links && eqIgnoreCase(tag, "a") && dmpIsSafeUrl(val);
    }
    if (eqIgnoreCase(attr, "src")) {
        return opts.allow_images && eqIgnoreCase(tag, "img") && dmpIsSafeUrl(val);
    }
    return eqIgnoreCase(attr, "alt") ||
        eqIgnoreCase(attr, "title") ||
        eqIgnoreCase(attr, "class") ||
        eqIgnoreCase(attr, "id") ||
        eqIgnoreCase(attr, "width") ||
        eqIgnoreCase(attr, "height") ||
        eqIgnoreCase(attr, "target") ||
        eqIgnoreCase(attr, "rel") ||
        eqIgnoreCase(attr, "dir") ||
        eqIgnoreCase(attr, "lang");
}

fn dmpFindTagClose(html: string, start: usize): usize {
    let i: usize = start;
    let quote: u8 = 0;
    while (i < html.len) : (i += 1) {
        const c: u8 = html[i];
        if (quote != 0) {
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                return i;
            }
        }
    }
    return html.len;
}

fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize {
    let i: usize = start;
    while (i + 2 < html.len) : (i += 1) {
        if (html[i] == '<' && html[i + 1] == '/') {
            let j: usize = i + 2;
            while (j < html.len && isSpace(html[j])) : (j += 1) {}
            const name_start: usize = j;
            while (j < html.len && dmpIsNameChar(html[j])) : (j += 1) {}
            if (eqIgnoreCase(html[name_start..j], tag)) {
                const close_gt: usize = dmpFindTagClose(html, j);
                if (close_gt < html.len) {
                    return close_gt + 1;
                }
                return html.len;
            }
        }
    }
    return html.len;
}

/// Sanitize HTML into `out` buffer, removing blocked tags, event attributes, and unsafe URIs.
/// Returns the number of bytes written to `out`.
pub fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize {
    let out_pos: usize = 0;
    let i: usize = 0;

    while (i < html.len) {
        if (html[i] != '<') {
            out_pos = dmpWriteByte(out, out_pos, html[i]);
            i += 1;
            continue;
        }

        // HTML comment: <!-- ... -->
        if (i + 3 < html.len && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            let cend: usize = i + 4;
            while (cend + 2 < html.len && !(html[cend] == '-' && html[cend + 1] == '-' && html[cend + 2] == '>')) : (cend += 1) {}
            const after_comment: usize = if (cend + 2 < html.len) (cend + 3) else html.len;
            if (!opts.strip_comments) {
                out_pos = writeStr(html[i..after_comment], out, out_pos);
            }
            i = after_comment;
            continue;
        }

        const gt: usize = dmpFindTagClose(html, i + 1);
        if (gt >= html.len) {
            // Unclosed '<' treated as literal text
            out_pos = dmpWriteByte(out, out_pos, html[i]);
            i += 1;
            continue;
        }

        let p: usize = i + 1;
        while (p < gt && isSpace(html[p])) : (p += 1) {}
        let is_closing: bool = false;
        if (p < gt && html[p] == '/') {
            is_closing = true;
            p += 1;
            while (p < gt && isSpace(html[p])) : (p += 1) {}
        }

        const name_start: usize = p;
        while (p < gt && dmpIsNameChar(html[p])) : (p += 1) {}
        const tag_name: string = html[name_start..p];

        if (tag_name.len == 0) {
            i = gt + 1;
            continue;
        }

        let self_closing: bool = false;
        if (!is_closing && gt > i + 1 && html[gt - 1] == '/') {
            self_closing = true;
        }

        // Strip blocked tags and their inner content
        if (!is_closing && dmpIsBlockedContentTag(tag_name)) {
            if (self_closing) {
                i = gt + 1;
            } else {
                i = dmpFindClosingTagEnd(html, gt + 1, tag_name);
            }
            continue;
        }
        if (dmpIsBlockedContentTag(tag_name) || dmpIsBlockedVoidTag(tag_name)) {
            i = gt + 1;
            continue;
        }

        if (!dmpIsAllowedTag(tag_name, opts)) {
            i = gt + 1;
            continue;
        }

        if (is_closing) {
            out_pos = writeStr("</", out, out_pos);
            out_pos += toLower(tag_name, out[out_pos..out.len]);
            out_pos = dmpWriteByte(out, out_pos, '>');
            i = gt + 1;
            continue;
        }

        // Emit opening tag with filtered safe attributes
        out_pos = dmpWriteByte(out, out_pos, '<');
        out_pos += toLower(tag_name, out[out_pos..out.len]);

        const attr_end_limit: usize = if (self_closing) (gt - 1) else gt;
        while (p < attr_end_limit) {
            while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
            if (p >= attr_end_limit) {
                break;
            }
            const a_start: usize = p;
            while (p < attr_end_limit && !isSpace(html[p]) && html[p] != '=' && html[p] != '/') : (p += 1) {}
            const attr_name: string = html[a_start..p];
            if (attr_name.len == 0) {
                p += 1;
                continue;
            }

            while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
            let has_val: bool = false;
            let attr_val: string = "";
            if (p < attr_end_limit && html[p] == '=') {
                has_val = true;
                p += 1;
                while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
                if (p < attr_end_limit && (html[p] == '"' || html[p] == '\'')) {
                    const q: u8 = html[p];
                    p += 1;
                    const v_start: usize = p;
                    while (p < attr_end_limit && html[p] != q) : (p += 1) {}
                    attr_val = html[v_start..p];
                    if (p < attr_end_limit && html[p] == q) {
                        p += 1;
                    }
                } else {
                    const v_start: usize = p;
                    while (p < attr_end_limit && !isSpace(html[p])) : (p += 1) {}
                    attr_val = html[v_start..p];
                }
            }

            if (dmpIsSafeAttr(tag_name, attr_name, attr_val, opts)) {
                out_pos = dmpWriteByte(out, out_pos, ' ');
                out_pos += toLower(attr_name, out[out_pos..out.len]);
                if (has_val) {
                    out_pos = writeStr("=\"", out, out_pos);
                    out_pos = writeStr(attr_val, out, out_pos);
                    out_pos = dmpWriteByte(out, out_pos, '"');
                }
            }
        }

        if (self_closing) {
            out_pos = writeStr("/>", out, out_pos);
        } else {
            out_pos = dmpWriteByte(out, out_pos, '>');
        }
        i = gt + 1;
    }

    return out_pos;
}

/// Returns `true` if `html` contains only allowed tags, safe attributes, and safe URI schemes.
pub fn isSafeHtml(html: string): bool {
    const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
    let i: usize = 0;
    while (i < html.len) {
        if (html[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 3 < html.len && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            let cend: usize = i + 4;
            while (cend + 2 < html.len && !(html[cend] == '-' && html[cend + 1] == '-' && html[cend + 2] == '>')) : (cend += 1) {}
            i = if (cend + 2 < html.len) (cend + 3) else html.len;
            continue;
        }

        const gt: usize = dmpFindTagClose(html, i + 1);
        if (gt >= html.len) {
            i += 1;
            continue;
        }

        let p: usize = i + 1;
        while (p < gt && isSpace(html[p])) : (p += 1) {}
        let is_closing: bool = false;
        if (p < gt && html[p] == '/') {
            is_closing = true;
            p += 1;
            while (p < gt && isSpace(html[p])) : (p += 1) {}
        }

        const name_start: usize = p;
        while (p < gt && dmpIsNameChar(html[p])) : (p += 1) {}
        const tag_name: string = html[name_start..p];

        if (tag_name.len == 0 || dmpIsBlockedContentTag(tag_name) || dmpIsBlockedVoidTag(tag_name) || !dmpIsAllowedTag(tag_name, opts)) {
            return false;
        }

        if (!is_closing) {
            const self_closing: bool = gt > i + 1 && html[gt - 1] == '/';
            const attr_end_limit: usize = if (self_closing) (gt - 1) else gt;
            while (p < attr_end_limit) {
                while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
                if (p >= attr_end_limit) {
                    break;
                }
                const a_start: usize = p;
                while (p < attr_end_limit && !isSpace(html[p]) && html[p] != '=' && html[p] != '/') : (p += 1) {}
                const attr_name: string = html[a_start..p];
                if (attr_name.len == 0) {
                    p += 1;
                    continue;
                }
                while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
                let attr_val: string = "";
                if (p < attr_end_limit && html[p] == '=') {
                    p += 1;
                    while (p < attr_end_limit && isSpace(html[p])) : (p += 1) {}
                    if (p < attr_end_limit && (html[p] == '"' || html[p] == '\'')) {
                        const q: u8 = html[p];
                        p += 1;
                        const v_start: usize = p;
                        while (p < attr_end_limit && html[p] != q) : (p += 1) {}
                        attr_val = html[v_start..p];
                        if (p < attr_end_limit && html[p] == q) {
                            p += 1;
                        }
                    } else {
                        const v_start: usize = p;
                        while (p < attr_end_limit && !isSpace(html[p])) : (p += 1) {}
                        attr_val = html[v_start..p];
                    }
                }
                if (!dmpIsSafeAttr(tag_name, attr_name, attr_val, opts)) {
                    return false;
                }
            }
        }
        i = gt + 1;
    }
    return true;
}
