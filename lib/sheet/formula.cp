// Source: cp/lib/sheet/formula.cp (Cell Coordinates, Values & Formula Evaluator)
// Pure CP spreadsheet coordinate parser, cell value representation, dependency extractor,
// and recursive-descent formula evaluator with range & function support.

const std = @import("std");

/// Maximum allowed nesting depth for formula expressions.
pub const MAX_FORMULA_DEPTH: usize = 64;

/// Maximum supported row index (1-based 9999 -> 0-based 9998).
pub const MAX_ROW_COUNT: usize = 9999;

/// Maximum supported column index (ZZ -> 0-based 701, total 702 columns).
pub const MAX_COL_COUNT: usize = 702;

/// Error set for spreadsheet coordinate, formula, DAG, and evaluation operations.
pub err SheetError {
    InvalidCellRef,
    InvalidFormula,
    CycleDetected,
    DivisionByZero,
    TypeMismatch,
    UnknownFunction,
    OutOfMemory,
}

/// 0-based spreadsheet cell coordinate (`A1` -> `row = 0, col = 0`).
pub struct CellCoord {
    row: usize,
    col: usize,

    /// Pack `(row, col)` into a compact `u64` key for hash maps and dependency sets.
    fn key(): u64 {
        return (u64(_.row) << 32) | u64(_.col);
    }

    /// Unpack a `u64` key back into a `CellCoord`.
    static fn fromKey(k: u64): CellCoord {
        return CellCoord{
            row: usize(k >> 32),
            col: usize(k & 0xFFFF_FFFF),
        };
    }

    /// Compare two coordinates for equality.
    fn eql(other: CellCoord): bool {
        return _.row == other.row && _.col == other.col;
    }

    /// Parse an A1-style reference (`A1`..`ZZ9999`, case-insensitive column letters).
    static fn parseRef(ref_str: string): CellCoord ! SheetError {
        if (ref_str.len < 2) {
            return SheetError.InvalidCellRef;
        }

        let i: usize = 0;
        let col_val: usize = 0;
        let letter_count: usize = 0;

        while (i < ref_str.len) {
            const c = ref_str[i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                const upper: u8 = if (c >= 'a') (c - 'a' + 'A') else c;
                letter_count += 1;
                if (letter_count > 2) {
                    return SheetError.InvalidCellRef;
                }
                col_val = col_val * 26 + usize(upper - 'A' + 1);
                i += 1;
            } else {
                break;
            }
        }

        if (letter_count == 0 || i >= ref_str.len) {
            return SheetError.InvalidCellRef;
        }

        // Row must start with '1'..'9' (no leading zero) and be 1..=9999.
        if (ref_str[i] < '1' || ref_str[i] > '9') {
            return SheetError.InvalidCellRef;
        }

        let row_val: usize = 0;
        while (i < ref_str.len) : (i += 1) {
            const d = ref_str[i];
            if (d < '0' || d > '9') {
                return SheetError.InvalidCellRef;
            }
            row_val = row_val * 10 + usize(d - '0');
            if (row_val > MAX_ROW_COUNT) {
                return SheetError.InvalidCellRef;
            }
        }

        if (row_val == 0) {
            return SheetError.InvalidCellRef;
        }

        return CellCoord{
            row: row_val - 1,
            col: col_val - 1,
        };
    }

    /// Format this coordinate as an uppercase A1-style reference string.
    fn formatRef(alloc: std.mem.Allocator): ([]u8) ! SheetError {
        if (_.row >= MAX_ROW_COUNT || _.col >= MAX_COL_COUNT) {
            return SheetError.InvalidCellRef;
        }
        if (_.col < 26) {
            const ch: u8 = u8('A') + u8(_.col);
            return std.fmt.allocPrint(alloc, "{c}{d}", .{ ch, _.row + 1 });
        }
        const hi: u8 = u8('A') + u8(_.col / 26 - 1);
        const lo: u8 = u8('A') + u8(_.col % 26);
        return std.fmt.allocPrint(alloc, "{c}{c}{d}", .{ hi, lo, _.row + 1 });
    }
}

/// Evaluated or literal value stored in a spreadsheet cell.
pub enum CellValue {
    Empty,
    Number: f64,
    Text: string,

    static fn makeEmpty(): CellValue {
        return .Empty;
    }

    static fn makeNumber(v: f64): CellValue {
        return CellValue.Number(v);
    }

    static fn makeText(s: string): CellValue {
        return CellValue.Text(s);
    }

    fn isEmpty(): bool {
        return switch (_) {
            .Empty => true,
            else => false,
        };
    }

    fn asNumber(): f64? {
        return switch (_) {
            .Number v => v,
            else => null,
        };
    }

    fn asText(): string? {
        return switch (_) {
            .Text s => s,
            else => null,
        };
    }

    fn eql(other: CellValue): bool {
        return switch (_) {
            .Empty => switch (other) {
                .Empty => true,
                else => false,
            },
            .Number a => switch (other) {
                .Number b => a == b,
                else => false,
            },
            .Text a => switch (other) {
                .Text b => std.mem.eql(u8, a, b),
                else => false,
            },
        };
    }
}

/// Callback context for looking up cell values during formula evaluation.
pub struct CellLookup {
    ctx: *const anyopaque,
    get_fn: fn (ctx: *const anyopaque, coord_key: u64) CellValue,
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlnum(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '_';
}

fn eqIgnoreCase(a: string, b: string): bool {
    return std.ascii.eqlIgnoreCase(a, b);
}

fn skipWs(src: string, pos: *usize) {
    while (pos < src.len) {
        const c = src[pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            pos += 1;
        } else {
            break;
        }
    }
}

/// Strip optional leading whitespace and optional `'='` prefix from a formula string.
pub fn normalizeFormulaExpr(raw: string): string ! SheetError {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return SheetError.InvalidFormula;
    }
    if (trimmed[0] == '=') {
        const rest = std.mem.trim(u8, trimmed[1..], " \t\r\n");
        if (rest.len == 0) {
            return SheetError.InvalidFormula;
        }
        return rest;
    }
    return trimmed;
}

enum BuiltinFunc {
    Sum,
    Avg,
    Min,
    Max,
    Count,
    If,
    Round,
}

fn resolveFunction(name: string): BuiltinFunc ! SheetError {
    if (eqIgnoreCase(name, "SUM")) return .Sum;
    if (eqIgnoreCase(name, "AVG") || eqIgnoreCase(name, "AVERAGE")) return .Avg;
    if (eqIgnoreCase(name, "MIN")) return .Min;
    if (eqIgnoreCase(name, "MAX")) return .Max;
    if (eqIgnoreCase(name, "COUNT")) return .Count;
    if (eqIgnoreCase(name, "IF")) return .If;
    if (eqIgnoreCase(name, "ROUND")) return .Round;
    return SheetError.UnknownFunction;
}

fn valueToNumber(v: CellValue): f64 ! SheetError {
    return switch (v) {
        .Empty => 0.0,
        .Number n => n,
        .Text => SheetError.TypeMismatch,
    };
}

fn valueIsTruthy(v: CellValue): bool ! SheetError {
    return switch (v) {
        .Empty => false,
        .Number n => n != 0.0,
        .Text s => s.len > 0,
    };
}

fn valuesEqual(a: CellValue, b: CellValue): bool {
    return switch (a) {
        .Empty => switch (b) {
            .Empty => true,
            .Number nb => nb == 0.0,
            .Text sb => sb.len == 0,
        },
        .Number na => switch (b) {
            .Empty => na == 0.0,
            .Number nb => na == nb,
            .Text => false,
        },
        .Text sa => switch (b) {
            .Empty => sa.len == 0,
            .Number => false,
            .Text sb => std.mem.eql(u8, sa, sb),
        },
    };
}

struct FormulaParser {
    src: string,
    pos: usize = 0,
    depth: usize = 0,
    alloc: std.mem.Allocator,
    lookup: CellLookup?,
    deps: *std.ArrayList(u64)?,

    fn peek(*_): u8? {
        skipWs(_.src, *_.pos);
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn matchChar(*_, ch: u8): bool {
        skipWs(_.src, *_.pos);
        if (_.pos < _.src.len && _.src[_.pos] == ch) {
            _.pos += 1;
            return true;
        }
        return false;
    }

    fn recordDep(*_, coord_key: u64): ! {
        if (dep_list := _.deps) {
            for (existing of dep_list.items) {
                if (existing == coord_key) return;
            }
            dep_list.append(_.alloc, coord_key)?;
        }
    }

    fn parseNumber(*_): CellValue ! SheetError {
        skipWs(_.src, *_.pos);
        const start = _.pos;
        let has_digit = false;
        let has_dot = false;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isDigit(c)) {
                has_digit = true;
                _.pos += 1;
            } else if (c == '.' && !has_dot) {
                has_dot = true;
                _.pos += 1;
            } else {
                break;
            }
        }

        if (!has_digit) {
            return SheetError.InvalidFormula;
        }

        const num_str = _.src[start.._.pos];
        const val = std.fmt.parseFloat(f64, num_str) catch return SheetError.InvalidFormula;
        return CellValue.Number(val);
    }

    fn parseString(*_, quote: u8): CellValue ! SheetError {
        _.pos += 1; // skip opening quote
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == quote) {
                const owned = buf.toOwnedSlice(_.alloc)?;
                return CellValue.Text(owned);
            }
            if (c == '\\') {
                if (_.pos >= _.src.len) return SheetError.InvalidFormula;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    'n' => buf.append(_.alloc, '\n')?,
                    't' => buf.append(_.alloc, '\t')?,
                    'r' => buf.append(_.alloc, '\r')?,
                    '\\' => buf.append(_.alloc, '\\')?,
                    '"' => buf.append(_.alloc, '"')?,
                    '\'' => buf.append(_.alloc, '\'')?,
                    else => buf.append(_.alloc, esc)?,
                }
            } else {
                buf.append(_.alloc, c)?;
            }
        }
        return SheetError.InvalidFormula;
    }

    fn readIdent(*_): string {
        const start = _.pos;
        while (_.pos < _.src.len && isAlnum(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }

    fn parseExpr(*_, evaluate: bool): CellValue ! SheetError {
        if (_.depth >= MAX_FORMULA_DEPTH) {
            return SheetError.InvalidFormula;
        }
        _.depth += 1;
        defer _.depth -= 1;
        return _.parseComparison(evaluate);
    }

    fn parseComparison(*_, evaluate: bool): CellValue ! SheetError {
        let left = _.parseAddSub(evaluate)?;
        while (true) {
            skipWs(_.src, *_.pos);
            if (_.pos >= _.src.len) break;

            let op: u8 = 0; // 1:==, 2:!=, 3:<, 4:<=, 5:>, 6:>=
            const c = _.src[_.pos];
            if (c == '=' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                op = 1;
                _.pos += 2;
            } else if (c == '=') {
                op = 1;
                _.pos += 1;
            } else if (c == '!' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                op = 2;
                _.pos += 2;
            } else if (c == '<' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '>') {
                op = 2;
                _.pos += 2;
            } else if (c == '<' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                op = 4;
                _.pos += 2;
            } else if (c == '<') {
                op = 3;
                _.pos += 1;
            } else if (c == '>' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                op = 6;
                _.pos += 2;
            } else if (c == '>') {
                op = 5;
                _.pos += 1;
            } else {
                break;
            }

            const right = _.parseAddSub(evaluate)?;
            if (evaluate) {
                let res_bool = false;
                if (op == 1) {
                    res_bool = valuesEqual(left, right);
                } else if (op == 2) {
                    res_bool = !valuesEqual(left, right);
                } else {
                    const a = valueToNumber(left)?;
                    const b = valueToNumber(right)?;
                    if (op == 3) res_bool = a < b;
                    else if (op == 4) res_bool = a <= b;
                    else if (op == 5) res_bool = a > b;
                    else res_bool = a >= b;
                }
                left = CellValue.Number(if (res_bool) 1.0 else 0.0);
            } else {
                left = .Empty;
            }
        }
        return left;
    }

    fn parseAddSub(*_, evaluate: bool): CellValue ! SheetError {
        let left = _.parseMulDiv(evaluate)?;
        while (true) {
            skipWs(_.src, *_.pos);
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c != '+' && c != '-') break;
            _.pos += 1;

            const right = _.parseMulDiv(evaluate)?;
            if (evaluate) {
                const a = valueToNumber(left)?;
                const b = valueToNumber(right)?;
                left = CellValue.Number(if (c == '+') (a + b) else (a - b));
            } else {
                left = .Empty;
            }
        }
        return left;
    }

    fn parseMulDiv(*_, evaluate: bool): CellValue ! SheetError {
        let left = _.parseUnary(evaluate)?;
        while (true) {
            skipWs(_.src, *_.pos);
            if (_.pos >= _.src.len) break;
            const c = _.src[_.pos];
            if (c != '*' && c != '/') break;
            _.pos += 1;

            const right = _.parseUnary(evaluate)?;
            if (evaluate) {
                const a = valueToNumber(left)?;
                const b = valueToNumber(right)?;
                if (c == '*') {
                    left = CellValue.Number(a * b);
                } else {
                    if (b == 0.0) {
                        return SheetError.DivisionByZero;
                    }
                    left = CellValue.Number(a / b);
                }
            } else {
                left = .Empty;
            }
        }
        return left;
    }

    fn parseUnary(*_, evaluate: bool): CellValue ! SheetError {
        skipWs(_.src, *_.pos);
        if (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '+') {
                _.pos += 1;
                const v = _.parseUnary(evaluate)?;
                if (!evaluate) return .Empty;
                return CellValue.Number(valueToNumber(v)?);
            }
            if (c == '-') {
                _.pos += 1;
                const v = _.parseUnary(evaluate)?;
                if (!evaluate) return .Empty;
                return CellValue.Number(-valueToNumber(v)?);
            }
        }
        return _.parsePrimary(evaluate);
    }

    fn parsePrimary(*_, evaluate: bool): CellValue ! SheetError {
        skipWs(_.src, *_.pos);
        if (_.pos >= _.src.len) {
            return SheetError.InvalidFormula;
        }

        const c = _.src[_.pos];
        if (c == '(') {
            _.pos += 1;
            const inner = _.parseExpr(evaluate)?;
            if (!_.matchChar(')')) {
                return SheetError.InvalidFormula;
            }
            return inner;
        }

        if (c == '"' || c == '\'') {
            return _.parseString(c);
        }

        if (isDigit(c)) {
            return _.parseNumber();
        }

        if (isAlpha(c)) {
            const ident = _.readIdent();
            skipWs(_.src, *_.pos);

            // Function call: FUNC(...)
            if (_.pos < _.src.len && _.src[_.pos] == '(') {
                return _.parseFuncCall(ident, evaluate);
            }

            // Boolean literals
            if (eqIgnoreCase(ident, "TRUE")) {
                return CellValue.Number(1.0);
            }
            if (eqIgnoreCase(ident, "FALSE")) {
                return CellValue.Number(0.0);
            }

            // Otherwise must be a single cell reference (e.g. A1, ZZ9999)
            const coord = CellCoord.parseRef(ident)?;
            const k = coord.key();
            _.recordDep(k)?;
            if (evaluate) {
                if (lk := _.lookup) {
                    return lk.get_fn(lk.ctx, k);
                }
            }
            return .Empty;
        }

        return SheetError.InvalidFormula;
    }

    fn evalRangeOrArg(*_, func: BuiltinFunc, sum: *f64, count: *usize, min_v: *f64, max_v: *f64, has_min_max: *bool, evaluate: bool): ! {
        skipWs(_.src, *_.pos);
        const saved_pos = _.pos;

        // Check if this argument is a cell range `A1:B3`
        if (_.pos < _.src.len && isAlpha(_.src[_.pos])) {
            const first_ident = _.readIdent();
            skipWs(_.src, *_.pos);
            if (_.pos < _.src.len && _.src[_.pos] == ':') {
                _.pos += 1;
                skipWs(_.src, *_.pos);
                if (_.pos >= _.src.len || !isAlpha(_.src[_.pos])) {
                    return SheetError.InvalidFormula;
                }
                const second_ident = _.readIdent();
                skipWs(_.src, *_.pos);
                // Ensure the range ends at ',' or ')'
                if (_.pos >= _.src.len || (_.src[_.pos] != ',' && _.src[_.pos] != ')')) {
                    return SheetError.InvalidFormula;
                }

                const c1 = CellCoord.parseRef(first_ident)?;
                const c2 = CellCoord.parseRef(second_ident)?;
                const r_min = if (c1.row <= c2.row) c1.row else c2.row;
                const r_max = if (c1.row >= c2.row) c1.row else c2.row;
                const c_min = if (c1.col <= c2.col) c1.col else c2.col;
                const c_max = if (c1.col >= c2.col) c1.col else c2.col;

                let r = r_min;
                while (r <= r_max) : (r += 1) {
                    let col = c_min;
                    while (col <= c_max) : (col += 1) {
                        const coord = CellCoord{ row: r, col: col };
                        const k = coord.key();
                        _.recordDep(k)?;
                        if (evaluate) {
                            if (lk := _.lookup) {
                                const cv = lk.get_fn(lk.ctx, k);
                                if (num := cv.asNumber()) {
                                    sum += num;
                                    count += 1;
                                    if (!has_min_max) {
                                        min_v = num;
                                        max_v = num;
                                        has_min_max = true;
                                    } else {
                                        if (num < min_v) min_v = num;
                                        if (num > max_v) max_v = num;
                                    }
                                }
                            }
                        }
                    }
                }
                return;
            }
            // Not a range; rewind and parse as normal expression
            _.pos = saved_pos;
        }

        const val = _.parseExpr(evaluate)?;
        if (evaluate) {
            switch (val) {
                .Number n => {
                    sum += n;
                    count += 1;
                    if (!has_min_max) {
                        min_v = n;
                        max_v = n;
                        has_min_max = true;
                    } else {
                        if (n < min_v) min_v = n;
                        if (n > max_v) max_v = n;
                    }
                },
                .Empty => {
                    // Empty cell argument is skipped in aggregate functions
                },
                .Text => {
                    if (func != .Count) {
                        return SheetError.TypeMismatch;
                    }
                },
            }
        }
    }

    fn parseFuncCall(*_, name: string, evaluate: bool): CellValue ! SheetError {
        const func = resolveFunction(name)?;
        if (!_.matchChar('(')) {
            return SheetError.InvalidFormula;
        }

        if (func == .If) {
            const cond_val = _.parseExpr(evaluate)?;
            if (!_.matchChar(',')) return SheetError.InvalidFormula;

            const take_then = if (evaluate) (valueIsTruthy(cond_val)?) else false;
            const then_val = _.parseExpr(evaluate && take_then)?;
            if (!_.matchChar(',')) return SheetError.InvalidFormula;

            const else_val = _.parseExpr(evaluate && !take_then)?;
            if (!_.matchChar(')')) return SheetError.InvalidFormula;

            if (!evaluate) return .Empty;
            return if (take_then) then_val else else_val;
        }

        if (func == .Round) {
            const val = _.parseExpr(evaluate)?;
            let digits: f64 = 0.0;
            if (_.matchChar(',')) {
                const d_val = _.parseExpr(evaluate)?;
                if (evaluate) {
                    digits = valueToNumber(d_val)?;
                }
            }
            if (!_.matchChar(')')) return SheetError.InvalidFormula;
            if (!evaluate) return .Empty;

            const num = valueToNumber(val)?;
            const factor = std.math.pow(f64, 10.0, digits);
            if (factor == 0.0 || std.math.isNan(factor) || std.math.isInf(factor)) {
                return SheetError.TypeMismatch;
            }
            return CellValue.Number(@round(num * factor) / factor);
        }

        // Aggregate functions: SUM, AVG, MIN, MAX, COUNT
        if (_.matchChar(')')) {
            return SheetError.InvalidFormula;
        }

        let sum: f64 = 0.0;
        let count: usize = 0;
        let min_v: f64 = 0.0;
        let max_v: f64 = 0.0;
        let has_min_max = false;

        while (true) {
            _.evalRangeOrArg(func, *sum, *count, *min_v, *max_v, *has_min_max, evaluate)?;
            if (_.matchChar(',')) {
                continue;
            }
            if (_.matchChar(')')) {
                break;
            }
            return SheetError.InvalidFormula;
        }

        if (!evaluate) return .Empty;

        return switch (func) {
            .Sum => CellValue.Number(sum),
            .Count => CellValue.Number(f64(count)),
            .Avg => {
                if (count == 0) return SheetError.DivisionByZero;
                return CellValue.Number(sum / f64(count));
            },
            .Min => CellValue.Number(if (has_min_max) min_v else 0.0),
            .Max => CellValue.Number(if (has_min_max) max_v else 0.0),
            else => unreachable,
        };
    }
}

/// Validate formula syntax and extract all unique referenced cell coordinates (`u64` keys).
pub fn parseAndExtractDeps(alloc: std.mem.Allocator, raw_expr: string, out_deps: *std.ArrayList(u64)): string ! SheetError {
    const expr = normalizeFormulaExpr(raw_expr)?;
    let parser = FormulaParser{
        src: expr,
        pos: 0,
        depth: 0,
        alloc: alloc,
        lookup: null,
        deps: out_deps,
    };
    * = parser.parseExpr(false)?;
    skipWs(parser.src, *parser.pos);
    if (parser.pos != parser.src.len) {
        return SheetError.InvalidFormula;
    }
    return expr;
}

/// Evaluate a normalized formula expression using `lookup` for cell references.
pub fn evaluateFormula(alloc: std.mem.Allocator, expr: string, lookup: CellLookup): CellValue ! SheetError {
    let parser = FormulaParser{
        src: expr,
        pos: 0,
        depth: 0,
        alloc: alloc,
        lookup: lookup,
        deps: null,
    };
    const val = parser.parseExpr(true)?;
    skipWs(parser.src, *parser.pos);
    if (parser.pos != parser.src.len) {
        return SheetError.InvalidFormula;
    }
    return val;
}
