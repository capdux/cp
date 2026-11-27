// Type definitions for CP

import { isSpace, trim, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";

export fn parseSpaceTokens(input: string, out: []string): usize;
export fn parse(input: string, out: []string): usize;
export fn countTokens(input: string): usize;
export fn tokenAt(input: string, index: usize): string?;
export fn hasSpaceToken(input: string, target: string): bool;
export fn containsToken(input: string, target: string): bool;
export fn stringifySpaceTokens(tokens: []const string, out: []u8): usize;
export fn stringify(tokens: []const string, out: []u8): usize;
export fn normalizeSpaceTokens(input: string, out: []u8): usize;
export fn parseCommaTokens(input: string, out: []string): usize;
export fn stringifyCommaTokens(tokens: []const string, out: []u8): usize;
