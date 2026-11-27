// Type definitions for CP

import { isSpace, trim, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn parseSpaceTokens(input: string, out: []string): usize;
export fn hasSpaceToken(input: string, target: string): bool;
export fn stringifySpaceTokens(tokens: &[]string, out: []u8): usize;
export fn parseCommaTokens(input: string, out: []string): usize;
export fn stringifyCommaTokens(tokens: &[]string, out: []u8): usize;
