// @cap/dompurify — Zero-allocation HTML sanitizer in pure CP
// Clean-room Apache-2.0 implementation inspired by npm `dompurify`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUTs, and @memcpy.

import { toLowerByte, eqIgnoreCase, toLower } from "cap:ascii";

type DmpVec16 = @Vector(16, u8);

fn dmpBuildNameCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = 1;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = 1;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = 1;
        c += 1;
    }
    lut['-' as usize] = 1;
    lut['_' as usize] = 1;
    return lut;
}

fn dmpBuildSpaceLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x20] = 1;
    lut[0x09] = 1;
    lut[0x0A] = 1;
    lut[0x0B] = 1;
    lut[0x0C] = 1;
    lut[0x0D] = 1;
    return lut;
}

const DMP_NAME_CHAR_LUT: [256]u8 = dmpBuildNameCharLut();
const DMP_SPACE_LUT: [256]u8 = dmpBuildSpaceLut();

fn dmpIsSpace(c: u8): bool {
    return DMP_SPACE_LUT[c as usize] != 0;
}

fn dmpIsNameChar(c: u8): bool {
    return DMP_NAME_CHAR_LUT[c as usize] != 0;
}

fn dmpFindByte(s: string, needle: u8, start: usize, end: usize): usize {
    let i: usize = start;
    const v_n: DmpVec16 = @splat(needle);
    while (i + 16 <= end) {
        const v: DmpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_n)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == needle) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < end) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return end;
}

fn dmpFindTagDelims(s: string, start: usize): usize {
    let i: usize = start;
    const len = s.len;
    const v_gt: DmpVec16 = @splat('>' as u8);
    const v_dq: DmpVec16 = @splat('"' as u8);
    const v_sq: DmpVec16 = @splat('\'' as u8);
    while (i + 16 <= len) {
        const v: DmpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_gt) || @reduce(.Or, v == v_dq) || @reduce(.Or, v == v_sq)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == '>' || c == '"' || c == '\'') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == '>' || c == '"' || c == '\'') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn dmpWriteStr(src: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const rem: usize = out.len - pos;
    const n: usize = if (src.len < rem) src.len else rem;
    @memcpy(out[pos .. pos + n], src[0 .. n]);
    return pos + n;
}

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
    while (i < html.len) {
        if (quote != 0) {
            i = dmpFindByte(html, quote, i, html.len);
            if (i >= html.len) {
                return html.len;
            }
            quote = 0;
            i += 1;
        } else {
            i = dmpFindTagDelims(html, i);
            if (i >= html.len) {
                return html.len;
            }
            const c: u8 = html[i];
            if (c == '>') {
                return i;
            }
            quote = c;
            i += 1;
        }
    }
    return html.len;
}

fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize {
    let i: usize = start;
    while (i + 2 < html.len) {
        i = dmpFindByte(html, '<' as u8, i, html.len);
        if (i + 2 >= html.len) {
            break;
        }
        if (html[i + 1] == '/') {
            let j: usize = i + 2;
            while (j < html.len && dmpIsSpace(html[j])) : (j += 1) {}
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
        i += 1;
    }
    return html.len;
}

fn dmpFindCommentEnd(html: string, start: usize): usize {
    let cend: usize = start;
    while (cend + 2 < html.len) {
        cend = dmpFindByte(html, '-' as u8, cend, html.len);
        if (cend + 2 >= html.len) {
            return html.len;
        }
        if (html[cend + 1] == '-' && html[cend + 2] == '>') {
            return cend + 3;
        }
        cend += 1;
    }
    return html.len;
}

/// Sanitize HTML into `out` buffer, removing blocked tags, event attributes, and unsafe URIs.
/// Returns the number of bytes written to `out`.
pub fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize {
    let out_pos: usize = 0;
    let i: usize = 0;

    while (i < html.len) {
        const lt: usize = dmpFindByte(html, '<' as u8, i, html.len);
        if (lt > i) {
            out_pos = dmpWriteStr(html[i..lt], out, out_pos);
            i = lt;
            if (i >= html.len) {
                break;
            }
        }

        // HTML comment: <!-- ... -->
        if (i + 3 < html.len && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            const after_comment: usize = dmpFindCommentEnd(html, i + 4);
            if (!opts.strip_comments) {
                out_pos = dmpWriteStr(html[i..after_comment], out, out_pos);
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
        while (p < gt && dmpIsSpace(html[p])) : (p += 1) {}
        let is_closing: bool = false;
        if (p < gt && html[p] == '/') {
            is_closing = true;
            p += 1;
            while (p < gt && dmpIsSpace(html[p])) : (p += 1) {}
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
            out_pos = dmpWriteStr("</", out, out_pos);
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
            while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
            if (p >= attr_end_limit) {
                break;
            }
            const a_start: usize = p;
            while (p < attr_end_limit && !dmpIsSpace(html[p]) && html[p] != '=' && html[p] != '/') : (p += 1) {}
            const attr_name: string = html[a_start..p];
            if (attr_name.len == 0) {
                p += 1;
                continue;
            }

            while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
            let has_val: bool = false;
            let attr_val: string = "";
            if (p < attr_end_limit && html[p] == '=') {
                has_val = true;
                p += 1;
                while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
                if (p < attr_end_limit && (html[p] == '"' || html[p] == '\'')) {
                    const q: u8 = html[p];
                    p += 1;
                    const v_start: usize = p;
                    p = dmpFindByte(html, q, p, attr_end_limit);
                    attr_val = html[v_start..p];
                    if (p < attr_end_limit && html[p] == q) {
                        p += 1;
                    }
                } else {
                    const v_start: usize = p;
                    while (p < attr_end_limit && !dmpIsSpace(html[p])) : (p += 1) {}
                    attr_val = html[v_start..p];
                }
            }

            if (dmpIsSafeAttr(tag_name, attr_name, attr_val, opts)) {
                out_pos = dmpWriteByte(out, out_pos, ' ');
                out_pos += toLower(attr_name, out[out_pos..out.len]);
                if (has_val) {
                    out_pos = dmpWriteStr("=\"", out, out_pos);
                    out_pos = dmpWriteStr(attr_val, out, out_pos);
                    out_pos = dmpWriteByte(out, out_pos, '"');
                }
            }
        }

        if (self_closing) {
            out_pos = dmpWriteStr("/>", out, out_pos);
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
        i = dmpFindByte(html, '<' as u8, i, html.len);
        if (i >= html.len) {
            break;
        }
        if (i + 3 < html.len && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            i = dmpFindCommentEnd(html, i + 4);
            continue;
        }

        const gt: usize = dmpFindTagClose(html, i + 1);
        if (gt >= html.len) {
            i += 1;
            continue;
        }

        let p: usize = i + 1;
        while (p < gt && dmpIsSpace(html[p])) : (p += 1) {}
        let is_closing: bool = false;
        if (p < gt && html[p] == '/') {
            is_closing = true;
            p += 1;
            while (p < gt && dmpIsSpace(html[p])) : (p += 1) {}
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
                while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
                if (p >= attr_end_limit) {
                    break;
                }
                const a_start: usize = p;
                while (p < attr_end_limit && !dmpIsSpace(html[p]) && html[p] != '=' && html[p] != '/') : (p += 1) {}
                const attr_name: string = html[a_start..p];
                if (attr_name.len == 0) {
                    p += 1;
                    continue;
                }
                while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
                let attr_val: string = "";
                if (p < attr_end_limit && html[p] == '=') {
                    p += 1;
                    while (p < attr_end_limit && dmpIsSpace(html[p])) : (p += 1) {}
                    if (p < attr_end_limit && (html[p] == '"' || html[p] == '\'')) {
                        const q: u8 = html[p];
                        p += 1;
                        const v_start: usize = p;
                        p = dmpFindByte(html, q, p, attr_end_limit);
                        attr_val = html[v_start..p];
                        if (p < attr_end_limit && html[p] == q) {
                            p += 1;
                        }
                    } else {
                        const v_start: usize = p;
                        while (p < attr_end_limit && !dmpIsSpace(html[p])) : (p += 1) {}
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
