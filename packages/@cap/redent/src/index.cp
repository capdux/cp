// @cap/redent — Zero-allocation strip-and-reindent multiline string utility for CP
// Clean-room MIT implementation with parity to npm `redent`.
// Composes `@cap/strip-indent` and `@cap/indent-string` with zero duplication.

import { stripIndent } from "../../strip-indent/src/index.cp";
import { indentString, indentStringWith } from "../../indent-string/src/index.cp";

const RD_SCRATCH_CAP: usize = 16384;

/// Strip common leading indentation from `s`, then re-indent each line by `count`
/// repetitions of `indent` (controlling blank lines via `include_empty_lines`).
/// Writes into `out` and returns the number of bytes written.
pub fn redentWithIndent(
    s: string,
    count: usize,
    indent: string,
    include_empty_lines: bool,
    out: []u8,
): usize {
    if (s.len == 0) {
        return 0;
    }
    if (count == 0 || indent.len == 0) {
        return stripIndent(s, out);
    }
    let scratch: [RD_SCRATCH_CAP]u8 = undefined;
    const stripped_len = stripIndent(s, scratch[0..RD_SCRATCH_CAP]);
    return indentStringWith(
        scratch[0..stripped_len],
        count,
        indent,
        include_empty_lines,
        out,
    );
}

/// Strip common leading indentation from `s`, then re-indent each non-empty line
/// by `count` spaces. Writes into `out` and returns the number of bytes written.
pub fn redent(s: string, count: usize, out: []u8): usize {
    if (s.len == 0) {
        return 0;
    }
    if (count == 0) {
        return stripIndent(s, out);
    }
    let scratch: [RD_SCRATCH_CAP]u8 = undefined;
    const stripped_len = stripIndent(s, scratch[0..RD_SCRATCH_CAP]);
    return indentString(scratch[0..stripped_len], count, out);
}
