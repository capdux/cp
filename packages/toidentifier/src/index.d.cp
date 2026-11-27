// Type definitions for CP

import { isAlpha, isAlnum, isDigit, isSpace, toUpperByte, toLowerByte } from "cap:ascii";

export fn toIdentifierLen(input: string): usize;
export fn toIdentifier(input: string, out: []u8): usize;
export fn toIdentifierChecked(input: string, out: []u8): usize?;
export fn toSafeIdentifier(input: string, out: []u8): usize;
export fn toCamelIdentifier(input: string, out: []u8): usize;
export fn toConstIdentifier(input: string, out: []u8): usize;
export fn toSnakeIdentifier(input: string, out: []u8): usize;
export fn isValidIdentifier(s: string): bool;
export fn isValidJsIdentifier(s: string): bool;
