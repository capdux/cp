// Type definitions for CP

export class ValidationIssue {
    path: string,
    message: string,
    code: string,
}

export class FormatErrorOptions {
    prefix: string,
    prefix_sep: string,
    issue_sep: string,
    max_issues: usize,
    include_path: bool,
}

export fn defaultFormatErrorOptions(): FormatErrorOptions;
export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;
export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;
export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;
