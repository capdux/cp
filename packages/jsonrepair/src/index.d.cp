// Type definitions for CP

export fn isWs(c: u8): bool;
export fn isIdentStart(c: u8): bool;
export fn isIdentChar(c: u8): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn strEq(a: string, b: string): bool;
export fn stripMarkdownFence(raw: string): string;
export fn skipWsAndComments(s: string, start: usize): usize;
export fn repairJson(raw: string, out: []u8): string;
