// Type definitions for CP

import { foldAsciiByte } from "../../unicase/src/index.cp";
export fn wmByteEq(a: u8, b: u8, ignore_case: bool): bool;
export fn wmMatchCore(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool;
export class WildMatch {
    pattern: string,
    any_many: u8,
    any_one: u8,
    ignore_case: bool,
    allow_escape: bool,
    fn new(pattern: string): WildMatch;
    fn newWithWildcards(pattern: string, any_many: u8, any_one: u8): WildMatch;
    fn newIgnoreCase(pattern: string): WildMatch;
    fn newWithEscape(pattern: string, ignore_case: bool): WildMatch;
    fn matches(input: string): bool;
}

export fn wildmatch(pattern: string, input: string): bool;
export fn wildmatchIgnoreCase(pattern: string, input: string): bool;
export fn wildmatchCustom(pattern: string, input: string, any_many: u8, any_one: u8): bool;
export fn hasWildcards(pattern: string): bool;
