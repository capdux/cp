// Type definitions for CP

export enum BrowserOp {
    Ge,
    Gt,
    Le,
    Lt,
    Eq,
    LastN,
    Defaults,
    Dead,
    Unknown
}

export class BrowserClause {
    negated: bool,
    is_and: bool,
    browser: string,
    op: BrowserOp,
    version_major: u32,
    version_minor: u32,
    count: u32,
}

export fn blqIsSpace(c: u8): bool;
export fn blqIsDigit(c: u8): bool;
export fn blqToLower(c: u8): u8;
export fn blqTrim(s: string): string;
export fn blqEqIgnoreCase(a: string, b: string): bool;
export fn blqStartsWithWord(s: string, word: string): bool;
export fn blqParseU32(s: string): u32;
export fn blqIsDeadBrowser(browser: string): bool;
export fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32;
export fn parseBrowserClause(clause: string): BrowserClause;
export fn matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool;
