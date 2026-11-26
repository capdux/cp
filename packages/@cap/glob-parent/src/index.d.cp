// Type definitions for CP

import { findExtglobStart } from "../../is-extglob/src/index.cp";
export class GlobSplit {
    base: string,
    glob: string,
    is_glob: bool,
}

export fn findMatchingBracket(s: string, openIdx: usize): usize?;
export fn findMatchingBrace(s: string, openIdx: usize): usize?;
export fn findFirstGlobPos(s: string): usize?;
export fn stripLeadingNegation(raw: string): string;
export fn trimTrailingSlashes(s: string): string;
export fn lastSlashBefore(s: string, limit: usize): usize?;
export fn isGlob(pattern: string): bool;
export fn globParent(pattern: string): string;
export fn splitGlobBase(pattern: string): GlobSplit;
export fn isGlobSpecial(c: u8): bool;
export fn escapeGlob(raw: string, out: []u8): string;
