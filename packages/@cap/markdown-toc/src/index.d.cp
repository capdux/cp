// Type definitions for CP

export class Heading {
    level: u8,
    text: string,
    slug_start: usize,
    slug_len: usize,
    line: usize,
}

export class TocResult {
    headings: [MAX_HEADINGS]Heading,
    count: usize,
    slug_pool: [MAX_SLUG_POOL]u8,
    slug_pool_len: usize,
    fn empty(): TocResult;
    fn slugAt(idx: usize): string;
    fn levelAt(idx: usize): u8;
    fn textAt(idx: usize): string;
    fn lineAt(idx: usize): usize;
}

export fn toLower(c: u8): u8;
export fn isAlphaNum(c: u8): bool;
export fn strEq(a: string, b: string): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendUsize(out: []u8, pos: usize, v: usize): usize;
export fn githubSlug(heading: string, out: []u8): string;
export fn trimHeadingText(raw: string): string;
export fn isFenceLine(line: string): bool;
export fn slugExists(res: &mut TocResult, cand: string): bool;
export fn extractToc(markdown: string): TocResult;
export fn renderTocMarkdown(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string;
