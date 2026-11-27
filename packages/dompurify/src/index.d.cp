// Type definitions for CP

import { toLowerByte, isSpace, isAlnum, eqIgnoreCase, toLower } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class SanitizeHtmlOptions {
    allow_links: bool,
    allow_images: bool,
    strip_comments: bool,
}

export fn defaultSanitizeOptions(): SanitizeHtmlOptions;
export fn dmpIsNameChar(c: u8): bool;
export fn dmpHasPrefixCI(s: string, prefix: string): bool;
export fn dmpWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn dmpIsBlockedContentTag(tag: string): bool;
export fn dmpIsBlockedVoidTag(tag: string): bool;
export fn dmpIsAllowedTag(tag: string, opts: SanitizeHtmlOptions): bool;
export fn dmpIsEventAttr(attr: string): bool;
export fn dmpIsSafeUrl(url: string): bool;
export fn dmpIsSafeAttr(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool;
export fn dmpFindTagClose(html: string, start: usize): usize;
export fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize;
export fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize;
export fn isSafeHtml(html: string): bool;
