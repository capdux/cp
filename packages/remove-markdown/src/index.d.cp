// Type definitions for CP

import { isSpace, isAlpha, isAlnum, isDigit, trim, findByte } from "cap:ascii";
export class StripOptions {
    keep_img_alt: bool,
    keep_code_block_text: bool,
    strip_html: bool,
    collapse_whitespace: bool,
    fn defaults(): StripOptions;
}

export fn isAsciiPunct(c: u8): bool;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushTextByte(out: []u8, pos: usize, b: u8, collapse_ws: bool): usize;
export fn isRuleLine(trimmed: string): bool;
export fn isRefLinkDef(trimmed: string): bool;
export class FenceInfo {
    is_fence: bool,
    ch: u8,
    len: usize,
}

export fn detectFence(line: string): FenceInfo;
export fn isClosingFence(line: string, fence_char: u8, min_len: usize): bool;
export fn stripBlockPrefixes(raw_line: string): string;
export fn findMatchingBracket(s: string, open_idx: usize): usize;
export fn findMatchingParen(s: string, open_idx: usize): usize;
export fn stripInline(s: string, opts: StripOptions, out: []u8, start_pos: usize, depth: u8): usize;
export fn stripMarkdown(md: string, out: []u8): string;
export fn stripMarkdownWith(md: string, opts: StripOptions, out: []u8): string;
export fn stripToSingleLine(md: string, out: []u8): string;
export fn excerpt(md: string, max_bytes: usize, out: []u8): string;
