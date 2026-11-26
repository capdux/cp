// Type definitions for CP

export enum SemverBump {
    None = 0,
    Patch = 1,
    Minor = 2,
    Major = 3
}

export class CommitFooter {
    token: string,
    value: string,
    is_breaking: bool,
    fn empty(): CommitFooter;
}

export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export class ParsedCommit {
    valid: bool,
    commit_type: string,
    scope: string,
    description: string,
    body: string,
    is_breaking: bool,
    breaking_note: string,
    footers: [16]CommitFooter,
    footer_count: usize,
    fn invalid(): ParsedCommit;
    fn footerAt(i: usize): CommitFooter?;
    fn getFooter(token: string): string?;
}

export fn isAlpha(c: u8): bool;
export fn isAlphaNum(c: u8): bool;
export fn isTypeChar(c: u8): bool;
export fn isFooterTokenChar(c: u8): bool;
export fn isSpace(c: u8): bool;
export fn isLineBlank(s: string, start: usize, end: usize): bool;
export fn trimLineEnd(s: string, start: usize, end: usize): usize;
export fn trimBlockEdges(s: string, start: usize, end: usize): string;
export class FooterLineMatch {
    matched: bool,
    token: string,
    val_start: usize,
    is_breaking: bool,
}

export fn matchFooterLine(msg: string, line_start: usize, line_end: usize): FooterLineMatch;
export fn parseCommit(msg: string): ParsedCommit;
export fn isValidHeader(header: string): bool;
export fn recommendedBump(commit: ParsedCommit): SemverBump;
export fn bumpRank(b: SemverBump): u8;
export fn highestBump(a: SemverBump, b: SemverBump): SemverBump;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn formatCommitHeader(commit: ParsedCommit, out: []u8): string;
