// Type definitions for CP

import { isSpace, isDigit, trim, eqIgnoreCase, findByte } from "cap:ascii";
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

export fn blqStartsWithWord(s: string, word: string): bool;
export fn blqParseU32(s: string): u32;
export fn blqIsDeadBrowser(browser: string): bool;
export fn blqCmpVersion(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32;
export fn parseBrowserClause(clause: string): BrowserClause;
export fn matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool;
