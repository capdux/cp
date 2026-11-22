// Production Deep Tests for cap:shell CondExprEvaluator (lib/shell/cond_expr.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_COND_DEPTH,
    CondExprError,
    CondOp,
    CondFileStat,
    CondStatFn,
    condToExitCode,
    CondExprEvaluator,
} from "cap:shell";

fn mockStat(path: string): CondFileStat? {
    if (std.mem.eql(u8, path, "/app/main.cp")) {
        return CondFileStat.regularFile(1024, 0o644);
    }
    if (std.mem.eql(u8, path, "/app/run.sh")) {
        return CondFileStat.regularFile(128, 0o755);
    }
    if (std.mem.eql(u8, path, "/app/empty.txt")) {
        return CondFileStat.regularFile(0, 0o400);
    }
    if (std.mem.eql(u8, path, "/app/src")) {
        return CondFileStat.directory(0o755);
    }
    if (std.mem.eql(u8, path, "/dev/null")) {
        return CondFileStat.charDevice(0o666);
    }
    if (std.mem.eql(u8, path, "/app/latest")) {
        return CondFileStat.symlink(0o777);
    }
    return CondFileStat.missing();
}

test("shell cond_expr: unary string tests (-z/-n/bare) & binary string comparisons", fn() {
    expect_eq(CondExprEvaluator.evalUnary(.str_empty, "", null) catch unreachable, true);
    expect_eq(CondExprEvaluator.evalUnary(.str_empty, "abc", null) catch unreachable, false);
    expect_eq(CondExprEvaluator.evalUnary(.str_nonempty, "abc", null) catch unreachable, true);
    expect_eq(CondExprEvaluator.evalUnary(.str_nonempty, "", null) catch unreachable, false);

    expect_eq(CondExprEvaluator.evalBinary(.str_eq, "cp", "cp") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.str_eq, "cp", "js") catch unreachable, false);
    expect_eq(CondExprEvaluator.evalBinary(.str_neq, "cp", "js") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.str_lt, "alpha", "beta") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.str_gt, "zeta", "beta") catch unreachable, true);

    const bare_true = ["nonempty"];
    expect_eq(CondExprEvaluator.evalTokens(bare_true[0..], null) catch unreachable, true);
    const bare_false = [""];
    expect_eq(CondExprEvaluator.evalTokens(bare_false[0..], null) catch unreachable, false);
    expect_eq(condToExitCode(true), 0);
    expect_eq(condToExitCode(false), 1);
});

test("shell cond_expr: file tests (-e/-f/-d/-c/-L/-s/-r/-w/-x) & empty-path safety guard", fn() {
    const stat_cb = mockStat;

    const t_reg = ["-f", "/app/main.cp"];
    expect_eq(CondExprEvaluator.evalTokens(t_reg[0..], stat_cb) catch unreachable, true);

    const t_dir = ["-d", "/app/src"];
    expect_eq(CondExprEvaluator.evalTokens(t_dir[0..], stat_cb) catch unreachable, true);

    const t_not_dir = ["-d", "/app/main.cp"];
    expect_eq(CondExprEvaluator.evalTokens(t_not_dir[0..], stat_cb) catch unreachable, false);

    const t_chr = ["-c", "/dev/null"];
    expect_eq(CondExprEvaluator.evalTokens(t_chr[0..], stat_cb) catch unreachable, true);

    const t_lnk = ["-L", "/app/latest"];
    expect_eq(CondExprEvaluator.evalTokens(t_lnk[0..], stat_cb) catch unreachable, true);

    const t_nonempty = ["-s", "/app/main.cp"];
    expect_eq(CondExprEvaluator.evalTokens(t_nonempty[0..], stat_cb) catch unreachable, true);

    const t_zero_size = ["-s", "/app/empty.txt"];
    expect_eq(CondExprEvaluator.evalTokens(t_zero_size[0..], stat_cb) catch unreachable, false);

    const t_exec = ["-x", "/app/run.sh"];
    expect_eq(CondExprEvaluator.evalTokens(t_exec[0..], stat_cb) catch unreachable, true);

    const t_ro_w = ["-w", "/app/empty.txt"];
    expect_eq(CondExprEvaluator.evalTokens(t_ro_w[0..], stat_cb) catch unreachable, false);

    // Bun CondExpr.cp contract: empty path MUST return false even for file tests
    const t_empty_path = ["-f", ""];
    expect_eq(CondExprEvaluator.evalTokens(t_empty_path[0..], stat_cb) catch unreachable, false);

    const t_missing = ["-e", "/does/not/exist"];
    expect_eq(CondExprEvaluator.evalTokens(t_missing[0..], stat_cb) catch unreachable, false);
});

test("shell cond_expr: signed 64-bit integer comparisons (-eq/-ne/-lt/-le/-gt/-ge)", fn() {
    expect_eq(CondExprEvaluator.evalBinary(.int_eq, "42", "+42") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_ne, "-10", "10") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_lt, "-100", "-5") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_le, "7", "7") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_gt, "9223372036854775807", "0") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_lt, "-9223372036854775808", "0") catch unreachable, true);
    expect_eq(CondExprEvaluator.evalBinary(.int_ge, " 15 ", "15") catch unreachable, true);

    // Non-numeric operand must return InvalidInteger
    let nan_err = false;
    * = CondExprEvaluator.evalBinary(.int_eq, "abc", "1") catch { nan_err = true; };
    expect_eq(nan_err, true);

    // Overflow beyond i64 must return InvalidInteger
    let ovf_err = false;
    * = CondExprEvaluator.evalBinary(.int_eq, "9223372036854775808", "0") catch { ovf_err = true; };
    expect_eq(ovf_err, true);
});

test("shell cond_expr: logical precedence (|| < && < !) and parenthesized grouping", fn() {
    const stat_cb = mockStat;

    // `false && false || true` => `(false && false) || true` => true
    const prec1 = ["a", "==", "b", "&&", "1", "-eq", "2", "||", "3", "-eq", "3"];
    expect_eq(CondExprEvaluator.evalTokens(prec1[0..], stat_cb) catch unreachable, true);

    // `true || false && false` => `true || (false && false)` => true
    const prec2 = ["1", "-eq", "1", "||", "2", "-eq", "3", "&&", "4", "-eq", "5"];
    expect_eq(CondExprEvaluator.evalTokens(prec2[0..], stat_cb) catch unreachable, true);

    // Parentheses override precedence: `(true || false) && false` => false
    const grouped = ["(", "1", "-eq", "1", "||", "2", "-eq", "3", ")", "&&", "!", "-f", "/app/main.cp"];
    expect_eq(CondExprEvaluator.evalTokens(grouped[0..], stat_cb) catch unreachable, false);

    // Double negation: `! ! -d /app/src` => true
    const dneg = ["!", "!", "-d", "/app/src"];
    expect_eq(CondExprEvaluator.evalTokens(dneg[0..], stat_cb) catch unreachable, true);
});

test("shell cond_expr: operator table parsing & roundtrip formatting", fn() {
    expect_eq(CondOp.parse("=") == .str_eq, true);
    expect_eq(CondOp.parse("==") == .str_eq, true);
    expect_eq(CondOp.parse("-h") == .is_symlink, true);
    expect_eq(CondOp.parse("-L") == .is_symlink, true);
    expect_eq(CondOp.parse("-unknown") == null, true);

    if (op := CondOp.parse("-ge")) {
        expect_eq(op.asStr(), "-ge");
        expect_eq(op.isBinary(), true);
        expect_eq(op.isUnary(), false);
    } else {
        unreachable;
    }
});

test("shell cond_expr: malformed expressions, unbalanced parens & recursion depth guard", fn() {
    expect_eq(MAX_COND_DEPTH, 32);

    const empty: [0]string = [];
    let empty_err = false;
    * = CondExprEvaluator.evalTokens(empty[0..], null) catch { empty_err = true; };
    expect_eq(empty_err, true);

    // Missing operand after unary `-z`
    const miss_unary = ["-z"];
    let miss_u_err = false;
    * = CondExprEvaluator.evalTokens(miss_unary[0..], null) catch { miss_u_err = true; };
    expect_eq(miss_u_err, true);

    // Missing RHS after binary `==`
    const miss_bin = ["foo", "=="];
    let miss_b_err = false;
    * = CondExprEvaluator.evalTokens(miss_bin[0..], null) catch { miss_b_err = true; };
    expect_eq(miss_b_err, true);

    // Unbalanced parentheses
    const unbal = ["(", "a", "==", "a"];
    let unbal_err = false;
    * = CondExprEvaluator.evalTokens(unbal[0..], null) catch { unbal_err = true; };
    expect_eq(unbal_err, true);

    // Trailing unexpected token
    const trail = ["a", "==", "a", "extra"];
    let trail_err = false;
    * = CondExprEvaluator.evalTokens(trail[0..], null) catch { trail_err = true; };
    expect_eq(trail_err, true);

    // Excessively nested `!` triggers ExpressionTooDeep cleanly
    const deep_nots = [
        "!", "!", "!", "!", "!", "!", "!", "!", "!", "!",
        "!", "!", "!", "!", "!", "!", "!", "!", "!", "!",
        "!", "!", "!", "!", "!", "!", "!", "!", "!", "!",
        "!", "!", "!", "!", "x",
    ];
    let depth_err = false;
    * = CondExprEvaluator.evalTokens(deep_nots[0..], null) catch { depth_err = true; };
    expect_eq(depth_err, true);
});
