// @cap/balanced-match — Zero-allocation nested balanced delimiter matcher in pure CP
// Clean-room MIT implementation inspired by npm `balanced-match` v3.

import { strEq } from "cap:ascii";
import { indexOfBytes } from "cap:bin";

pub struct BalancedResult {
    found: bool,
    start: usize,
    end: usize,
    pre: string,
    body: string,
    post: string,
}

const BM_STACK_CAP: usize = 64;

fn bmMakeRange(s: usize, e: usize): [usize, usize] {
    let pair: [usize, usize] = undefined;
    @field(pair, "0") = s;
    @field(pair, "1") = e;
    return pair;
}

fn bmEmptyResult(): BalancedResult {
    return BalancedResult {
        found: false,
        start: 0,
        end: 0,
        pre: "",
        body: "",
        post: "",
    };
}

/// Find the byte offsets `[start, end]` of the first balanced pair `(a, b)` in `input`.
/// Supports nested delimiters, identical open/close delimiters, multi-byte strings,
/// and backtracking when an outer opener is unclosed (matching npm `balanced-match` v3).
pub fn balancedRange(a: string, b: string, input: string): [usize, usize]? {
    if (a.len == 0 || b.len == 0) {
        return null;
    }
    let ai_opt: usize? = indexOfBytes(input, a, 0);
    if (ai_opt == null) {
        return null;
    }
    let ai: usize = ai_opt.?;

    let bi_opt: usize? = indexOfBytes(input, b, ai + a.len);
    if (bi_opt == null) {
        return null;
    }
    let bi: usize = bi_opt.?;

    // Identical open and close delimiters (e.g. `"` and `"`)
    if (strEq(a, b)) {
        return bmMakeRange(ai, bi);
    }

    let begs: [64]usize = undefined;
    let begs_len: usize = 0;
    let left: usize = input.len;
    let right: usize = 0;
    let has_fallback: bool = false;
    let i: usize = ai;
    let has_ai: bool = true;
    let has_bi: bool = true;

    while (i < input.len) {
        if (has_ai && i == ai) {
            if (begs_len < BM_STACK_CAP) {
                begs[begs_len] = i;
                begs_len += 1;
            }
            if (next_a := indexOfBytes(input, a, i + a.len)) {
                ai = next_a;
                has_ai = true;
            } else {
                has_ai = false;
            }
        } else if (begs_len == 1) {
            return bmMakeRange(begs[0], bi);
        } else {
            if (begs_len > 0) {
                begs_len -= 1;
                const beg: usize = begs[begs_len];
                if (beg < left) {
                    left = beg;
                    right = bi;
                    has_fallback = true;
                }
            }
            if (next_b := indexOfBytes(input, b, i + b.len)) {
                bi = next_b;
                has_bi = true;
            } else {
                has_bi = false;
            }
            if (begs_len == 0) {
                if (!has_ai) {
                    break;
                }
                if (next_b2 := indexOfBytes(input, b, ai + a.len)) {
                    bi = next_b2;
                    has_bi = true;
                } else {
                    has_bi = false;
                }
            }
        }

        if (has_ai && has_bi) {
            if (ai < bi) {
                i = ai;
            } else {
                i = bi;
            }
        } else if (has_bi) {
            i = bi;
        } else {
            break;
        }
    }

    if (has_fallback) {
        return bmMakeRange(left, right);
    }
    return null;
}

/// Match the first balanced pair `(a, b)` in `input`, returning slices `pre`, `body`, `post`.
pub fn balanced(a: string, b: string, input: string): BalancedResult {
    if (rng := balancedRange(a, b, input)) {
        const s: usize = @field(rng, "0");
        const e: usize = @field(rng, "1");
        return BalancedResult {
            found: true,
            start: s,
            end: e,
            pre: input[0..s],
            body: input[(s + a.len)..e],
            post: input[(e + b.len)..input.len],
        };
    }
    return bmEmptyResult();
}

/// Find all sequential top-level balanced matches `(a, b)` in `input`, writing up to `out.len`
/// results with absolute `start`/`end` offsets into `out`. Returns the number of matches written.
pub fn balancedAll(a: string, b: string, input: string, out: []BalancedResult): usize {
    let count: usize = 0;
    let offset: usize = 0;
    while (offset < input.len && count < out.len) {
        const slice: string = input[offset..input.len];
        const res: BalancedResult = balanced(a, b, slice);
        if (!res.found) {
            break;
        }
        const abs_start: usize = offset + res.start;
        const abs_end: usize = offset + res.end;
        out[count] = BalancedResult {
            found: true,
            start: abs_start,
            end: abs_end,
            pre: res.pre,
            body: res.body,
            post: res.post,
        };
        count += 1;
        offset = abs_end + b.len;
    }
    return count;
}

export {
    BalancedResult,
    balancedRange,
    balanced,
    balancedAll,
};
