// Source: cp/lib/eval/metrics.cp (LLM Evaluation Scorers, Token F1, JSON Field Match & Pass@k)
// Pure CP, zero-external-dependency evaluation metrics for AI Agents and LLM outputs.

const std = @import("std");
import { JSON } from "cap:json";
import { JsonValue } from "../json/value.cp";

/// Error set shared across `cap:eval` scoring, aggregation, and regression gating.
pub err EvalError {
    InvalidInput,
    InvalidThreshold,
    EmptyDataset,
    DivisionByZero,
    OutOfMemory,
}

/// String matching strategy for `EvalScorer.exactMatch`.
pub enum MatchMode: u8 {
    Exact = 0,
    ExactTrimmed = 1,
    CaseInsensitive = 2,
    Contains = 3,
    Prefix = 4,
    Suffix = 5,
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: string): string {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn isTokenChar(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 0x80) return true;
    return false;
}

fn toLowerAsciiSlice(alloc: std.mem.Allocator, s: string): string ! EvalError {
    let buf = alloc.alloc(u8, s.len) catch return EvalError.OutOfMemory;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c >= 'A' && c <= 'Z') {
            buf[i] = c + 32;
        } else {
            buf[i] = c;
        }
    }
    return buf;
}

fn tokenizeLower(
    arena_alloc: std.mem.Allocator,
    text: string,
    out: *std.ArrayList(string),
): void ! EvalError {
    let i: usize = 0;
    while (i < text.len) {
        while (i < text.len && !isTokenChar(text[i])) : (i += 1) {}
        if (i >= text.len) break;
        const start = i;
        while (i < text.len && isTokenChar(text[i])) : (i += 1) {}
        const lower = toLowerAsciiSlice(arena_alloc, text[start..i])?;
        out.append(arena_alloc, lower) catch return EvalError.OutOfMemory;
    }
}

fn resolveJsonField(root: JsonValue, field_path: string): JsonValue? {
    if (field_path.len == 0) return root;
    if (field_path[0] == '/') {
        return root.pointer(field_path);
    }

    let cur = root;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= field_path.len) : (i += 1) {
        if (i == field_path.len || field_path[i] == '.') {
            const seg = field_path[start..i];
            if (seg.len == 0) return null;
            if (cur.isObject()) {
                if (next_val := cur.get(seg)) {
                    cur = next_val;
                } else {
                    return null;
                }
            } else if (cur.isArray()) {
                const idx = std.fmt.parseInt(usize, seg, 10) catch return null;
                if (next_val := cur.getIndex(idx)) {
                    cur = next_val;
                } else {
                    return null;
                }
            } else {
                return null;
            }
            start = i + 1;
        }
    }
    return cur;
}

/// Stateless evaluation scorer providing exact/fuzzy/JSON/Pass@k metrics.
pub static class EvalScorer {
    /// Scores `actual` against `expected` using `mode`, returning `1.0` on match and `0.0` otherwise.
    pub fn exactMatch(expected: string, actual: string, mode: MatchMode): f64 {
        switch (mode) {
            .Exact => {
                return if (std.mem.eql(u8, expected, actual)) 1.0 else 0.0;
            },
            .ExactTrimmed => {
                const e = trimAscii(expected);
                const a = trimAscii(actual);
                return if (std.mem.eql(u8, e, a)) 1.0 else 0.0;
            },
            .CaseInsensitive => {
                return if (std.ascii.eqlIgnoreCase(expected, actual)) 1.0 else 0.0;
            },
            .Contains => {
                return if (std.mem.indexOf(u8, actual, expected) != null) 1.0 else 0.0;
            },
            .Prefix => {
                return if (std.mem.startsWith(u8, actual, expected)) 1.0 else 0.0;
            },
            .Suffix => {
                return if (std.mem.endsWith(u8, actual, expected)) 1.0 else 0.0;
            },
        }
    }

    /// Returns the fraction of `keywords` present in `actual` (`0.0..1.0`). Returns `1.0` if `keywords` is empty.
    pub fn containsAll(actual: string, keywords: []const string): f64 {
        if (keywords.len == 0) return 1.0;
        let hits: usize = 0;
        let i: usize = 0;
        while (i < keywords.len) : (i += 1) {
            const kw = keywords[i];
            if (kw.len == 0 || std.mem.indexOf(u8, actual, kw) != null) {
                hits += 1;
            }
        }
        return (hits as f64) / (keywords.len as f64);
    }

    /// Returns `0.0` if any non-empty term in `forbidden` appears in `actual`, otherwise `1.0`.
    pub fn forbiddenNone(actual: string, forbidden: []const string): f64 {
        let i: usize = 0;
        while (i < forbidden.len) : (i += 1) {
            const term = forbidden[i];
            if (term.len > 0 && std.mem.indexOf(u8, actual, term) != null) {
                return 0.0;
            }
        }
        return 1.0;
    }

    /// Computes SQuAD-style bag-of-tokens F1 score (`0.0..1.0`) splitting on ASCII whitespace and punctuation.
    pub fn tokenF1(alloc: std.mem.Allocator, expected: string, actual: string): f64 ! EvalError {
        let arena_state = std.heap.ArenaAllocator.init(alloc);
        defer arena_state.deinit();
        const aa = arena_state.allocator();

        let exp_tokens: std.ArrayList(string) = .empty;
        let act_tokens: std.ArrayList(string) = .empty;
        tokenizeLower(aa, expected, *exp_tokens)?;
        tokenizeLower(aa, actual, *act_tokens)?;

        if (exp_tokens.items.len == 0 && act_tokens.items.len == 0) {
            return 1.0;
        }
        if (exp_tokens.items.len == 0 || act_tokens.items.len == 0) {
            return 0.0;
        }

        let counts = std.StringHashMap(usize).init(aa);
        let i: usize = 0;
        while (i < exp_tokens.items.len) : (i += 1) {
            const tok = exp_tokens.items[i];
            const gop = counts.getOrPut(tok) catch return EvalError.OutOfMemory;
            if (gop.found_existing) {
                gop.value_ptr.* += 1;
            } else {
                gop.value_ptr.* = 1;
            }
        }

        let common: usize = 0;
        let j: usize = 0;
        while (j < act_tokens.items.len) : (j += 1) {
            const tok = act_tokens.items[j];
            if (ptr := counts.getPtr(tok)) {
                if (ptr.* > 0) {
                    common += 1;
                    ptr.* -= 1;
                }
            }
        }

        if (common == 0) return 0.0;

        const prec = (common as f64) / (act_tokens.items.len as f64);
        const rec = (common as f64) / (exp_tokens.items.len as f64);
        return (2.0 * prec * rec) / (prec + rec);
    }

    /// Parses `expected_json` and `actual_json`, returning `1.0` if `field_path` exists in both and is structurally equal, else `0.0`.
    pub fn jsonFieldMatch(
        alloc: std.mem.Allocator,
        expected_json: string,
        actual_json: string,
        field_path: string,
    ): f64 ! EvalError {
        let arena_state = std.heap.ArenaAllocator.init(alloc);
        defer arena_state.deinit();
        const aa = arena_state.allocator();

        const exp_root = JSON.parse(aa, expected_json) catch return EvalError.InvalidInput;
        const act_root = JSON.parse(aa, actual_json) catch return EvalError.InvalidInput;

        const exp_val = resolveJsonField(exp_root, field_path) ?? return 0.0;
        const act_val = resolveJsonField(act_root, field_path) ?? return 0.0;

        return if (exp_val.equals(act_val)) 1.0 else 0.0;
    }

    /// Numerically stable unbiased Pass@k estimator: `1 - C(n - c, k) / C(n, k)` (requires `n >= k >= 1` and `0 <= c <= n`).
    pub fn passAtK(n: usize, c: usize, k: usize): f64 ! EvalError {
        if (k == 0 || n == 0 || k > n || c > n) {
            return EvalError.InvalidInput;
        }
        if (c == 0) return 0.0;
        if (n - c < k) return 1.0;

        let prod: f64 = 1.0;
        let i: usize = 0;
        while (i < k) : (i += 1) {
            const num = (n - c - i) as f64;
            const den = (n - i) as f64;
            prod *= num / den;
        }
        const res = 1.0 - prod;
        if (res < 0.0) return 0.0;
        if (res > 1.0) return 1.0;
        return res;
    }
}
