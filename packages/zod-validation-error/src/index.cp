// @cap/zod-validation-error — Zero-heap human-readable validation error formatter
// Clean-room CP implementation inspired by npm:zod-validation-error (causaly/zod-validation-error)

pub struct ValidationIssue {
    path: string,
    message: string,
    code: string,
}

pub struct FormatErrorOptions {
    prefix: string,
    prefix_sep: string,
    issue_sep: string,
    max_issues: usize,
    include_path: bool,
}

pub fn defaultFormatErrorOptions(): FormatErrorOptions {
    return FormatErrorOptions{
        prefix: "Validation error",
        prefix_sep: ": ",
        issue_sep: "; ",
        max_issues: 99,
        include_path: true,
    };
}

fn zverWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize {
    if (pos < out.len) {
        out[pos] = ch;
        return pos + 1;
    }
    return pos;
}

/// Format a list of validation issues into a human-readable string in `out`.
/// Returns the number of bytes written.
pub fn formatValidationIssues(
    issues: []const ValidationIssue,
    opts: FormatErrorOptions,
    out: []u8,
): usize {
    if (out.len == 0) {
        return 0;
    }

    let pos: usize = 0;

    if (issues.len == 0) {
        if (opts.prefix.len > 0) {
            pos = zverWriteStr(out, pos, opts.prefix);
        }
        return pos;
    }

    if (opts.prefix.len > 0) {
        pos = zverWriteStr(out, pos, opts.prefix);
        pos = zverWriteStr(out, pos, opts.prefix_sep);
    }

    let limit = issues.len;
    if (opts.max_issues > 0 && opts.max_issues < limit) {
        limit = opts.max_issues;
    }

    let i: usize = 0;
    while (i < limit) : (i += 1) {
        if (i > 0) {
            pos = zverWriteStr(out, pos, opts.issue_sep);
        }
        const issue = issues[i];
        pos = zverWriteStr(out, pos, issue.message);
        if (opts.include_path && issue.path.len > 0) {
            pos = zverWriteStr(out, pos, " at \"");
            pos = zverWriteStr(out, pos, issue.path);
            pos = zverWriteByte(out, pos, '"');
        }
    }

    return pos;
}
