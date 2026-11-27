// Type definitions for CP

import { strEq, isSpace, isAlpha, isDigit, trim } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn isIdentStart(c: u8): bool;
export fn isIdentChar(c: u8): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn stripMarkdownFence(raw: string): string;
export fn skipWsAndComments(s: string, start: usize): usize;
export fn repairJson(raw: string, out: []u8): string;
