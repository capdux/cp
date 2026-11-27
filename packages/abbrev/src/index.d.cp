// Type definitions for CP

export class AbbrevEntry {
    abbrev: string,
    full: string,
}

export fn abvStrEq(a: string, b: string): bool;
export fn abvStrLt(a: string, b: string): bool;
export fn abvHasPrefix(word: string, prefix: string): bool;
export fn abvLcp(a: string, b: string): usize;
export fn abbrev(words: &[]string, out: []AbbrevEntry): usize;
export fn shortestAbbrev(words: &[]string, target: string): string?;
export fn resolveAbbrev(words: &[]string, prefix: string): string?;
