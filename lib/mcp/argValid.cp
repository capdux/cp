// Origin: Pure-CP Stdlib (Zero-Heap MCP Tool Argument Strict Constraint Validator & AI Error Hint Formatter) | Created: 2026-11-22
// Pure CP zero-heap-allocation MCP tool argument validator for production Tool Servers:
//   1. `McpArgType` (`Str | Int | Float | Bool | Array | Object`), `McpArgRule`,
//      `McpArgViolationKind`, `McpArgViolation`, `McpArgValidationReport`
//   2. `validateMcpToolArgs`: zero-copy top-level JSON object scanner (safely skips nested objects/arrays)
//      validating required presence, strict type matching, string length `[min_len, max_len]`,
//      integer range `[min_int, max_int]`, duplicate keys, and CSV enum whitelist (`enum_csv`)
//   3. `formatMcpArgErrorHint`: zero-heap actionable LLM self-healing error message renderer

const std = @import("std");

/// Expected JSON value type for an MCP tool argument.
pub enum McpArgType {
    Str,
    Int,
    Float,
    Bool,
    Array,
    Object,
}

/// Declarative validation constraint for a single top-level MCP tool argument.
pub struct McpArgRule {
    name: string = "";
    kind: McpArgType = McpArgType.Str;
    required: bool = false;
    min_len: usize = 0;
    max_len: usize = 0;
    min_int: i64 = 0;
    max_int: i64 = 0;
    has_int_bounds: bool = false;
    enum_csv: string = "";
}

/// Specific constraint violation category detected during argument validation.
pub enum McpArgViolationKind {
    MalformedJson,
    MissingRequired,
    TypeMismatch,
    StringTooShort,
    StringTooLong,
    IntOutOfRange,
    EnumMismatch,
    DuplicateKey,
}

/// Detailed violation record emitted into caller-provided `out_violations`.
pub struct McpArgViolation {
    rule_index: usize = 0;
    field_name: string = "";
    kind: McpArgViolationKind = McpArgViolationKind.MalformedJson;
    expected_type: McpArgType = McpArgType.Str;
    min_len: usize = 0;
    max_len: usize = 0;
    min_int: i64 = 0;
    max_int: i64 = 0;
    has_int_bounds: bool = false;
    enum_csv: string = "";
}

/// Aggregated validation outcome returned by `validateMcpToolArgs`.
pub struct McpArgValidationReport {
    valid: bool = false;
    violation_count: usize = 0;
    total_violations: usize = 0;
    matched_fields: usize = 0;
    malformed_json: bool = false;
}

// ============================================================================
// Internal Zero-Copy RFC 8259 JSON Scanner Helpers
// ============================================================================

type McpArgVec16 = @Vector(16, u8);

fn makeMcpArgWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\r'] = 1;
    lut['\n'] = 1;
    return lut;
}

fn makeMcpArgHexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = '0';
    while (i <= '9') : (i += 1) { lut[i] = 1; }
    i = 'a';
    while (i <= 'f') : (i += 1) { lut[i] = 1; }
    i = 'A';
    while (i <= 'F') : (i += 1) { lut[i] = 1; }
    return lut;
}

const MCP_ARG_WS_LUT: [u8; 256] = makeMcpArgWsLut();
const MCP_ARG_HEX_LUT: [u8; 256] = makeMcpArgHexLut();

fn isWs(b: u8): bool {
    return MCP_ARG_WS_LUT[@as(usize, b)] != 0;
}

fn skipWs(s: string, start: usize): usize {
    let i: usize = start;
    const v_sp: McpArgVec16 = @splat(' ' as u8);
    const v_tab: McpArgVec16 = @splat('\t' as u8);
    const v_cr: McpArgVec16 = @splat('\r' as u8);
    const v_lf: McpArgVec16 = @splat('\n' as u8);
    while (i + 16 <= s.len) {
        const v: McpArgVec16 = s[i .. i + 16][0..16].*;
        const is_ws = (v == v_sp) | (v == v_tab) | (v == v_cr) | (v == v_lf);
        if (!@reduce(.And, is_ws)) break;
        i += 16;
    }
    while (i < s.len && isWs(s[i])) : (i += 1) {}
    return i;
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: McpArgVec16 = a[i .. i + 16][0..16].*;
        const vb: McpArgVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

struct ParsedStringSpan {
    end_pos: usize = 0;
    raw_content: string = "";
    decoded_len: usize = 0;
}

/// Parses a JSON string starting at `s[start] == '"'`.
/// Returns `null` on unterminated string, unescaped control char (< 0x20), or invalid escape.
fn parseJsonStringSpan(s: string, start: usize): ParsedStringSpan? {
    if (start >= s.len || s[start] != '"') return null;
    let i: usize = start + 1;
    let dlen: usize = 0;
    const v_dq: McpArgVec16 = @splat('"' as u8);
    const v_bs: McpArgVec16 = @splat('\\' as u8);
    const v_ctrl: McpArgVec16 = @splat(0x20 as u8);
    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: McpArgVec16 = s[i .. i + 16][0..16].*;
            const special = (v == v_dq) | (v == v_bs) | (v < v_ctrl);
            if (@reduce(.Or, special)) break;
            i += 16;
            dlen += 16;
        }
        if (i >= s.len) break;
        const c: u8 = s[i];
        if (c == '"') {
            return ParsedStringSpan{
                end_pos: i + 1,
                raw_content: s[start + 1 .. i],
                decoded_len: dlen,
            };
        }
        if (c < 0x20) return null;
        if (c == '\\') {
            if (i + 1 >= s.len) return null;
            const esc: u8 = s[i + 1];
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' ||
                esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                i += 2;
                dlen += 1;
            } else if (esc == 'u') {
                if (i + 6 > s.len) return null;
                if (MCP_ARG_HEX_LUT[@as(usize, s[i + 2])] == 0 ||
                    MCP_ARG_HEX_LUT[@as(usize, s[i + 3])] == 0 ||
                    MCP_ARG_HEX_LUT[@as(usize, s[i + 4])] == 0 ||
                    MCP_ARG_HEX_LUT[@as(usize, s[i + 5])] == 0) {
                    return null;
                }
                i += 6;
                dlen += 1;
            } else {
                return null;
            }
        } else {
            i += 1;
            dlen += 1;
        }
    }
    return null;
}

/// Skips a balanced nested JSON object `{...}` or array `[...]` starting at `start`.
fn skipComposite(s: string, start: usize): usize? {
    if (start >= s.len) return null;
    const open_ch: u8 = s[start];
    if (open_ch != '{' && open_ch != '[') return null;

    let depth: usize = 0;
    let i: usize = start;
    const v_dq: McpArgVec16 = @splat('"' as u8);
    const v_lb: McpArgVec16 = @splat('{' as u8);
    const v_rb: McpArgVec16 = @splat('}' as u8);
    const v_ls: McpArgVec16 = @splat('[' as u8);
    const v_rs: McpArgVec16 = @splat(']' as u8);
    while (i < s.len) {
        if (depth > 0) {
            while (i + 16 <= s.len) {
                const v: McpArgVec16 = s[i .. i + 16][0..16].*;
                const hit = (v == v_dq) | (v == v_lb) | (v == v_rb) | (v == v_ls) | (v == v_rs);
                if (@reduce(.Or, hit)) break;
                i += 16;
            }
            if (i >= s.len) break;
        }
        const c: u8 = s[i];
        if (c == '"') {
            const sp = parseJsonStringSpan(s, i) ?? return null;
            i = sp.end_pos;
            continue;
        }
        if (c == '{' || c == '[') {
            depth += 1;
        } else if (c == '}' || c == ']') {
            if (depth == 0) return null;
            depth -= 1;
            if (depth == 0) return i + 1;
        }
        i += 1;
    }
    return null;
}

struct ParsedNumberSpan {
    end_pos: usize = 0;
    is_int: bool = false;
    int_val: i64 = 0;
    int_overflow: bool = false;
}

/// Parses and validates an RFC 8259 JSON number starting at `start`.
fn parseJsonNumberSpan(s: string, start: usize): ParsedNumberSpan? {
    if (start >= s.len) return null;
    let i: usize = start;
    let neg: bool = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
        if (i >= s.len) return null;
    }

    const int_start: usize = i;
    if (s[i] == '0') {
        i += 1;
        if (i < s.len && s[i] >= '0' && s[i] <= '9') return null; // leading zero disallowed
    } else if (s[i] >= '1' && s[i] <= '9') {
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    } else {
        return null;
    }
    const int_end: usize = i;

    let has_frac_or_exp: bool = false;
    if (i < s.len && s[i] == '.') {
        has_frac_or_exp = true;
        i += 1;
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        has_frac_or_exp = true;
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) i += 1;
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    }

    if (has_frac_or_exp) {
        return ParsedNumberSpan{
            end_pos: i,
            is_int: false,
            int_val: 0,
            int_overflow: false,
        };
    }

    // Parse signed 64-bit integer with overflow check
    let acc: u64 = 0;
    let overflow: bool = false;
    const limit: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    let k: usize = int_start;
    while (k < int_end) : (k += 1) {
        const digit: u64 = (s[k] - '0') as u64;
        if (acc > (limit - digit) / 10) {
            overflow = true;
            break;
        }
        acc = acc * 10 + digit;
    }

    let val: i64 = 0;
    if (!overflow) {
        if (neg) {
            if (acc == 9223372036854775808) {
                val = std.math.minInt(i64);
            } else {
                val = -(acc as i64);
            }
        } else {
            val = acc as i64;
        }
    }

    return ParsedNumberSpan{
        end_pos: i,
        is_int: true,
        int_val: val,
        int_overflow: overflow,
    };
}

/// Checks whether `val` matches any comma-separated item in `enum_csv`.
fn matchesEnumCsv(val: string, enum_csv: string): bool {
    if (enum_csv.len == 0) return true;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= enum_csv.len) : (i += 1) {
        if (i == enum_csv.len || enum_csv[i] == ',') {
            if (strEq(val, enum_csv[start..i])) return true;
            start = i + 1;
        }
    }
    return false;
}

fn pushViolation(
    report: *McpArgValidationReport,
    out_violations: *[]McpArgViolation,
    v: McpArgViolation,
) {
    if (report.violation_count < out_violations.len) {
        out_violations[report.violation_count] = v;
        report.violation_count += 1;
    }
    report.total_violations += 1;
}

fn failMalformed(
    report: *McpArgValidationReport,
    out_violations: *[]McpArgViolation,
): McpArgValidationReport {
    report.malformed_json = true;
    pushViolation(report, out_violations, McpArgViolation{
        kind: McpArgViolationKind.MalformedJson,
    });
    return report.*;
}

fn markSeen(seen: *[4]u64, idx: usize): bool {
    if (idx >= 256) return false;
    const word: usize = idx / 64;
    const bit: u6 = (idx % 64) as u6;
    const mask: u64 = (1 as u64) << bit;
    const was_seen: bool = (seen[word] & mask) != 0;
    seen[word] = seen[word] | mask;
    return was_seen;
}

fn isSeen(seen: [4]u64, idx: usize): bool {
    if (idx >= 256) return false;
    const word: usize = idx / 64;
    const bit: u6 = (idx % 64) as u6;
    return (seen[word] & ((1 as u64) << bit)) != 0;
}

/// Validates a raw JSON object string of MCP tool arguments against declarative `rules`
/// with zero heap allocation. Safely skips nested objects/arrays so inner keys never collide
/// with top-level argument names.
pub fn validateMcpToolArgs(
    args_json: string,
    rules: []const McpArgRule,
    out_violations: *[]McpArgViolation,
): McpArgValidationReport {
    let report = McpArgValidationReport{};

    let pos: usize = skipWs(args_json, 0);
    if (pos >= args_json.len || args_json[pos] != '{') {
        return failMalformed(&report, out_violations);
    }
    pos += 1;
    pos = skipWs(args_json, pos);

    let seen: [4]u64 = [0; 4];

    if (pos < args_json.len && args_json[pos] == '}') {
        pos += 1;
    } else {
        while (true) {
            pos = skipWs(args_json, pos);
            const key_span = parseJsonStringSpan(args_json, pos) ?? return failMalformed(&report, out_violations);
            pos = skipWs(args_json, key_span.end_pos);
            if (pos >= args_json.len || args_json[pos] != ':') {
                return failMalformed(&report, out_violations);
            }
            pos = skipWs(args_json, pos + 1);
            if (pos >= args_json.len) {
                return failMalformed(&report, out_violations);
            }

            // Find matching rule (if any)
            let matched_idx: usize? = null;
            let r_i: usize = 0;
            while (r_i < rules.len) : (r_i += 1) {
                if (strEq(key_span.raw_content, rules[r_i].name)) {
                    matched_idx = r_i;
                    break;
                }
            }

            // Parse value token & classify type
            const ch: u8 = args_json[pos];
            let is_str: bool = false;
            let is_int: bool = false;
            let is_float: bool = false;
            let is_bool: bool = false;
            let is_arr: bool = false;
            let is_obj: bool = false;
            let str_val: string = "";
            let str_dlen: usize = 0;
            let num_int_val: i64 = 0;
            let num_int_overflow: bool = false;

            if (ch == '"') {
                const v_span = parseJsonStringSpan(args_json, pos) ?? return failMalformed(&report, out_violations);
                is_str = true;
                str_val = v_span.raw_content;
                str_dlen = v_span.decoded_len;
                pos = v_span.end_pos;
            } else if (ch == '{') {
                const end_p = skipComposite(args_json, pos) ?? return failMalformed(&report, out_violations);
                is_obj = true;
                pos = end_p;
            } else if (ch == '[') {
                const end_p = skipComposite(args_json, pos) ?? return failMalformed(&report, out_violations);
                is_arr = true;
                pos = end_p;
            } else if (ch == 't' && pos + 4 <= args_json.len && strEq(args_json[pos .. pos + 4], "true")) {
                is_bool = true;
                pos += 4;
            } else if (ch == 'f' && pos + 5 <= args_json.len && strEq(args_json[pos .. pos + 5], "false")) {
                is_bool = true;
                pos += 5;
            } else if (ch == 'n' && pos + 4 <= args_json.len && strEq(args_json[pos .. pos + 4], "null")) {
                // Valid JSON null, matches none of Str/Int/Float/Bool/Array/Object
                pos += 4;
            } else if (ch == '-' || (ch >= '0' && ch <= '9')) {
                const n_span = parseJsonNumberSpan(args_json, pos) ?? return failMalformed(&report, out_violations);
                if (n_span.is_int) {
                    is_int = true;
                    num_int_val = n_span.int_val;
                    num_int_overflow = n_span.int_overflow;
                } else {
                    is_float = true;
                }
                pos = n_span.end_pos;
            } else {
                return failMalformed(&report, out_violations);
            }

            // Apply rule constraints if key matched a declared rule
            if (r_idx := matched_idx) {
                const rule: McpArgRule = rules[r_idx];
                const dup: bool = markSeen(&seen, r_idx);
                if (dup) {
                    pushViolation(&report, out_violations, McpArgViolation{
                        rule_index: r_idx,
                        field_name: rule.name,
                        kind: McpArgViolationKind.DuplicateKey,
                        expected_type: rule.kind,
                    });
                } else {
                    report.matched_fields += 1;
                }

                let type_ok: bool = false;
                if (rule.kind == McpArgType.Str) {
                    type_ok = is_str;
                } else if (rule.kind == McpArgType.Int) {
                    type_ok = is_int;
                } else if (rule.kind == McpArgType.Float) {
                    type_ok = is_float || is_int;
                } else if (rule.kind == McpArgType.Bool) {
                    type_ok = is_bool;
                } else if (rule.kind == McpArgType.Array) {
                    type_ok = is_arr;
                } else if (rule.kind == McpArgType.Object) {
                    type_ok = is_obj;
                }

                if (!type_ok) {
                    pushViolation(&report, out_violations, McpArgViolation{
                        rule_index: r_idx,
                        field_name: rule.name,
                        kind: McpArgViolationKind.TypeMismatch,
                        expected_type: rule.kind,
                    });
                } else if (rule.kind == McpArgType.Str) {
                    if (rule.min_len > 0 && str_dlen < rule.min_len) {
                        pushViolation(&report, out_violations, McpArgViolation{
                            rule_index: r_idx,
                            field_name: rule.name,
                            kind: McpArgViolationKind.StringTooShort,
                            expected_type: rule.kind,
                            min_len: rule.min_len,
                            max_len: rule.max_len,
                        });
                    }
                    if (rule.max_len > 0 && str_dlen > rule.max_len) {
                        pushViolation(&report, out_violations, McpArgViolation{
                            rule_index: r_idx,
                            field_name: rule.name,
                            kind: McpArgViolationKind.StringTooLong,
                            expected_type: rule.kind,
                            min_len: rule.min_len,
                            max_len: rule.max_len,
                        });
                    }
                    if (rule.enum_csv.len > 0 && !matchesEnumCsv(str_val, rule.enum_csv)) {
                        pushViolation(&report, out_violations, McpArgViolation{
                            rule_index: r_idx,
                            field_name: rule.name,
                            kind: McpArgViolationKind.EnumMismatch,
                            expected_type: rule.kind,
                            enum_csv: rule.enum_csv,
                        });
                    }
                } else if (rule.kind == McpArgType.Int) {
                    if (num_int_overflow ||
                        (rule.has_int_bounds && (num_int_val < rule.min_int || num_int_val > rule.max_int))) {
                        pushViolation(&report, out_violations, McpArgViolation{
                            rule_index: r_idx,
                            field_name: rule.name,
                            kind: McpArgViolationKind.IntOutOfRange,
                            expected_type: rule.kind,
                            min_int: rule.min_int,
                            max_int: rule.max_int,
                            has_int_bounds: rule.has_int_bounds,
                        });
                    }
                }
            }

            pos = skipWs(args_json, pos);
            if (pos >= args_json.len) {
                report.malformed_json = true;
                pushViolation(&report, out_violations, McpArgViolation{
                    kind: McpArgViolationKind.MalformedJson,
                });
                return report;
            }
            if (args_json[pos] == ',') {
                pos += 1;
                continue;
            }
            if (args_json[pos] == '}') {
                pos += 1;
                break;
            }
            report.malformed_json = true;
            pushViolation(&report, out_violations, McpArgViolation{
                kind: McpArgViolationKind.MalformedJson,
            });
            return report;
        }
    }

    // Reject trailing non-whitespace bytes after closing '}'
    pos = skipWs(args_json, pos);
    if (pos != args_json.len) {
        report.malformed_json = true;
        pushViolation(&report, out_violations, McpArgViolation{
            kind: McpArgViolationKind.MalformedJson,
        });
        return report;
    }

    // Check required fields that were missing from the top-level object
    let r_i: usize = 0;
    while (r_i < rules.len) : (r_i += 1) {
        if (rules[r_i].required && !isSeen(seen, r_i)) {
            pushViolation(&report, out_violations, McpArgViolation{
                rule_index: r_i,
                field_name: rules[r_i].name,
                kind: McpArgViolationKind.MissingRequired,
                expected_type: rules[r_i].kind,
            });
        }
    }

    report.valid = (report.total_violations == 0);
    return report;
}

// ============================================================================
// Zero-Heap AI-Friendly Error Hint Renderer
// ============================================================================

fn appendBytes(out_buf: *[]u8, pos: *usize, text: string): bool {
    if (pos.* > out_buf.len || text.len > out_buf.len - pos.*) return false;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        out_buf[pos.* + i] = text[i];
    }
    pos.* += text.len;
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (v = v / 10) {
            tmp[n] = ('0' as u8) + ((v % 10) as u8);
            n += 1;
        }
    }
    if (pos.* > out_buf.len || n > out_buf.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

fn appendI64(out_buf: *[]u8, pos: *usize, val: i64): bool {
    if (val < 0) {
        if (!appendBytes(out_buf, pos, "-")) return false;
        if (val == std.math.minInt(i64)) {
            return appendU64(out_buf, pos, 9223372036854775808);
        }
        return appendU64(out_buf, pos, (-val) as u64);
    }
    return appendU64(out_buf, pos, val as u64);
}

fn argTypeName(kind: McpArgType): string {
    if (kind == McpArgType.Str) return "string";
    if (kind == McpArgType.Int) return "integer";
    if (kind == McpArgType.Float) return "number";
    if (kind == McpArgType.Bool) return "boolean";
    if (kind == McpArgType.Array) return "array";
    return "object";
}

/// Renders an actionable, human- and LLM-readable self-healing error hint into `out_buf`
/// with zero heap allocation. Returns bytes written, or `null` if `out_buf` is too small.
pub fn formatMcpArgErrorHint(
    report: McpArgValidationReport,
    violations: []const McpArgViolation,
    out_buf: *[]u8,
): usize? {
    let pos: usize = 0;
    if (report.valid) {
        if (!appendBytes(out_buf, &pos, "OK")) return null;
        return pos;
    }

    if (report.malformed_json) {
        if (!appendBytes(out_buf, &pos, "Invalid tool arguments JSON: expected a valid JSON object")) return null;
        return pos;
    }

    if (!appendBytes(out_buf, &pos, "Invalid tool arguments: ")) return null;
    const count: usize = if (report.violation_count < violations.len) report.violation_count else violations.len;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (i > 0) {
            if (!appendBytes(out_buf, &pos, "; ")) return null;
        }
        const v: McpArgViolation = violations[i];
        if (v.kind == McpArgViolationKind.MissingRequired) {
            if (!appendBytes(out_buf, &pos, "missing required field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' (expected ")) return null;
            if (!appendBytes(out_buf, &pos, argTypeName(v.expected_type))) return null;
            if (!appendBytes(out_buf, &pos, ")")) return null;
        } else if (v.kind == McpArgViolationKind.TypeMismatch) {
            if (!appendBytes(out_buf, &pos, "field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' wrong type (expected ")) return null;
            if (!appendBytes(out_buf, &pos, argTypeName(v.expected_type))) return null;
            if (!appendBytes(out_buf, &pos, ")")) return null;
        } else if (v.kind == McpArgViolationKind.StringTooShort) {
            if (!appendBytes(out_buf, &pos, "field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' too short (min_len=")) return null;
            if (!appendU64(out_buf, &pos, v.min_len as u64)) return null;
            if (!appendBytes(out_buf, &pos, ")")) return null;
        } else if (v.kind == McpArgViolationKind.StringTooLong) {
            if (!appendBytes(out_buf, &pos, "field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' too long (max_len=")) return null;
            if (!appendU64(out_buf, &pos, v.max_len as u64)) return null;
            if (!appendBytes(out_buf, &pos, ")")) return null;
        } else if (v.kind == McpArgViolationKind.IntOutOfRange) {
            if (!appendBytes(out_buf, &pos, "field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' out of range (allowed [")) return null;
            if (!appendI64(out_buf, &pos, v.min_int)) return null;
            if (!appendBytes(out_buf, &pos, ", ")) return null;
            if (!appendI64(out_buf, &pos, v.max_int)) return null;
            if (!appendBytes(out_buf, &pos, "])")) return null;
        } else if (v.kind == McpArgViolationKind.EnumMismatch) {
            if (!appendBytes(out_buf, &pos, "field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "' invalid value (allowed: ")) return null;
            if (!appendBytes(out_buf, &pos, v.enum_csv)) return null;
            if (!appendBytes(out_buf, &pos, ")")) return null;
        } else if (v.kind == McpArgViolationKind.DuplicateKey) {
            if (!appendBytes(out_buf, &pos, "duplicate field '")) return null;
            if (!appendBytes(out_buf, &pos, v.field_name)) return null;
            if (!appendBytes(out_buf, &pos, "'")) return null;
        } else {
            if (!appendBytes(out_buf, &pos, "malformed JSON")) return null;
        }
    }
    return pos;
}
