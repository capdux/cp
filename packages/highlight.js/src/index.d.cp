// Type definitions for CP

import { isSpace, isDigit, isHexDigit, isAlpha, strEq, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class Token {
    kind: u8,
    start: usize,
    len: usize,
}

export fn isIdentStart(c: u8): bool;
export fn isIdentCont(c: u8): bool;
export fn sliceEq(src: &[]u8, start: usize, length: usize, lit: string): bool;
export fn sliceEqIgnoreCase(src: &[]u8, start: usize, length: usize, upperLit: string): bool;
export fn isCodeKeyword(src: &[]u8, s: usize, n: usize): bool;
export fn isCodeType(src: &[]u8, s: usize, n: usize): bool;
export fn isJsonKeyword(src: &[]u8, s: usize, n: usize): bool;
export fn isSqlKeyword(src: &[]u8, s: usize, n: usize): bool;
export fn isSqlType(src: &[]u8, s: usize, n: usize): bool;
export fn isOperatorByte(c: u8): bool;
export fn isPunctByte(c: u8): bool;
export fn nextToken(src: &[]u8, pos: usize, lang: u8): Token;
export fn tokenize(src: &[]u8, lang: u8, out: []Token): usize;
export fn writeStrChecked(dst: []u8, pos: usize, s: string): i64;
export fn writeSliceChecked(dst: []u8, pos: usize, src: &[]u8, start: usize, length: usize): i64;
export fn writeHtmlEscaped(dst: []u8, pos_in: usize, src: &[]u8, start: usize, length: usize): i64;
export fn ansiCodeFor(kind: u8): string;
export fn htmlClassFor(kind: u8): string;
export fn highlightAnsi(src: &[]u8, lang: u8, dst: []u8): i64;
export fn highlightHtml(src: &[]u8, lang: u8, dst: []u8): i64;
