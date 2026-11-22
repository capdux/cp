// cap/test/test_shell_cat_opts_production_deep.cp
// 深度生产级回归测试：cap:shell POSIX/GNU `cat` 选项解析与流式格式化器 (lib/shell/cat_opts.cp)

import { test } from "cap:test";
import {
    SHELL_CAT_MAX_INPUTS,
    CatInputKind,
    CatOpts,
    CatPlan,
    CatLineFormatter,
    formatCatBytes,
} from "cap:shell";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("cat_opts: default stdin, explicit '-', '--' sentinel & bundled/long flags") {
    // 1. 无参数默认读取 stdin 且走 passthrough 快速路径
    const empty_args: [0]([]const u8) = [];
    const p_empty = CatPlan.parse(empty_args[0..])?;
    if (!p_empty.opts.isPassthrough()) return err.ExpectedPassthrough;
    if (!p_empty.readsStdin()) return err.ExpectedStdin;
    if (p_empty.inputsSlice().len != 1 || p_empty.inputsSlice()[0].kind != .Stdin) {
        return err.ExpectedSingleStdin;
    }

    // 2. 短选项聚合 + 长选项 + 显式 '-' + '--' 后置以 '-' 开头文件名
    const args: [6]([]const u8) = ["-nsE", "--show-tabs", "a.txt", "-", "--", "-dash.txt"];
    const plan = CatPlan.parse(args[0..])?;
    if (!plan.opts.number_all || !plan.opts.squeeze_blank || !plan.opts.show_ends || !plan.opts.show_tabs) {
        return err.FlagsNotParsed;
    }
    const ins = plan.inputsSlice();
    if (ins.len != 3) return err.UnexpectedInputCount;
    if (ins[0].kind != .File || !bytesEq(ins[0].path, "a.txt")) return err.Input0Mismatch;
    if (ins[1].kind != .Stdin || !bytesEq(ins[1].path, "-")) return err.Input1Mismatch;
    if (ins[2].kind != .File || !bytesEq(ins[2].path, "-dash.txt")) return err.Input2Mismatch;
    if (!plan.readsStdin()) return err.ExpectedReadsStdin;
}

test("cat_opts: -b overrides -n precedence & composite flags -e, -t, -A") {
    const args_bn: [2]([]const u8) = ["-n", "-b"];
    const p_bn = CatPlan.parse(args_bn[0..])?;
    if (!p_bn.opts.effectiveNumberNonblank()) return err.ExpectedNumberNonblank;
    if (p_bn.opts.effectiveNumberAll()) return err.NumberAllShouldBeSuppressed;

    const args_a: [1]([]const u8) = ["-A"];
    const p_a = CatPlan.parse(args_a[0..])?;
    if (!p_a.opts.show_nonprinting || !p_a.opts.show_ends || !p_a.opts.show_tabs) {
        return err.ShowAllCompositeFailed;
    }

    const args_e: [1]([]const u8) = ["-e"];
    const p_e = CatPlan.parse(args_e[0..])?;
    if (!p_e.opts.show_nonprinting || !p_e.opts.show_ends || p_e.opts.show_tabs) {
        return err.ShortECompositeFailed;
    }

    const args_t: [1]([]const u8) = ["-t"];
    const p_t = CatPlan.parse(args_t[0..])?;
    if (!p_t.opts.show_nonprinting || !p_t.opts.show_tabs || p_t.opts.show_ends) {
        return err.ShortTCompositeFailed;
    }
}

test("cat_opts: strict Bun compatibility mode flags unsupported vs illegal options") {
    // Bun 当前 builtin/cat.cp 对 -b/-e/-n/-s/-t/-u/-v 返回 UnsupportedOption，其余返回 IllegalOption
    const args_unsup: [1]([]const u8) = ["-n"];
    let caught_unsup = false;
    * = CatPlan.parseWithMode(args_unsup[0..], true) catch {
        caught_unsup = true;
    };
    if (!caught_unsup) return err.ExpectedUnsupportedInStrictMode;

    const args_illegal: [1]([]const u8) = ["-z"];
    let caught_illegal = false;
    * = CatPlan.parseWithMode(args_illegal[0..], true) catch {
        caught_illegal = true;
    };
    if (!caught_illegal) return err.ExpectedIllegalInStrictMode;

    // 普通文件名在 strict 模式下正常解析
    const args_ok: [2]([]const u8) = ["foo.txt", "bar.txt"];
    const p_ok = CatPlan.parseWithMode(args_ok[0..], true)?;
    if (p_ok.inputsSlice().len != 2 || p_ok.readsStdin()) return err.StrictFilesParseFailed;
}

test("cat_opts: line numbering (-n vs -b) & blank squeezing (-s) formatting") {
    let out: [256]u8 = [0; 256];

    // 1. -n -s：压缩连续空行为 1 行，全部行编号
    const opts_ns = CatOpts{ number_all: true, squeeze_blank: true };
    const n1 = formatCatBytes(opts_ns, "alpha\n\n\n\nbeta\n", out[0..])?;
    const expected_ns = "     1\talpha\n     2\t\n     3\tbeta\n";
    if (!bytesEq(out[0..n1], expected_ns)) return err.FormatNsMismatch;

    // 2. -b -s -E：仅非空行编号，空行保留 '$' 但不编号，连续空行压缩为 1 行
    const opts_bse = CatOpts{ number_nonblank: true, number_all: true, squeeze_blank: true, show_ends: true };
    const n2 = formatCatBytes(opts_bse, "first\n\n\nsecond\n", out[0..])?;
    const expected_bse = "     1\tfirst$\n$\n     2\tsecond$\n";
    if (!bytesEq(out[0..n2], expected_bse)) return err.FormatBseMismatch;
}

test("cat_opts: -v -T -E non-printing caret/meta notation & passthrough round-trip") {
    let out: [128]u8 = [0; 128];

    // 1. 构造含 NUL(0x00), SOH(0x01), TAB(0x09), DEL(0x7F), 0x80(M-^@), 0xE1(M-a), 0xFF(M-^?) 的输入
    const raw: [9]u8 = [0x00, 0x01, '\t', 'A', 0x7F, 0x80, 0xE1, 0xFF, '\n'];
    const opts_all = CatOpts{ show_nonprinting: true, show_tabs: true, show_ends: true };
    const n = formatCatBytes(opts_all, raw[0..], out[0..])?;
    const expected = "^@^A^IA^?M-^@M-aM-^?$\n";
    if (!bytesEq(out[0..n], expected)) return err.NonPrintingFormatMismatch;

    // 2. 无选项 passthrough 精确字节 round-trip
    const opts_raw = CatOpts{};
    const n_raw = formatCatBytes(opts_raw, raw[0..], out[0..])?;
    if (!bytesEq(out[0..n_raw], raw[0..])) return err.PassthroughRoundTripFailed;
}

test("cat_opts: streaming cross-chunk state continuity & reset") {
    const opts = CatOpts{ number_all: true, squeeze_blank: true, show_ends: true };
    let fmt = CatLineFormatter.init(opts);
    let out: [256]u8 = [0; 256];

    // 将 "hello world\n\n\nnext\n" 拆分为 4 个任意边界 chunk 流式输入
    const n1 = fmt.formatChunk("hello ", out[0..])?;
    const n2 = fmt.formatChunk("world\n\n", out[n1..])?;
    const n3 = fmt.formatChunk("\n\nne", out[n1 + n2..])?;
    const n4 = fmt.formatChunk("xt\n", out[n1 + n2 + n3..])?;
    const total = n1 + n2 + n3 + n4;

    const expected = "     1\thello world$\n     2\t$\n     3\tnext$\n";
    if (!bytesEq(out[0..total], expected)) return err.StreamingContinuityFailed;

    // reset() 后行号归 1、空行状态重置
    fmt.reset();
    const n_reset = fmt.formatChunk("\nok\n", out[0..])?;
    const expected_reset = "     1\t$\n     2\tok$\n";
    if (!bytesEq(out[0..n_reset], expected_reset)) return err.ResetStateFailed;
}

test("cat_opts: malformed options, TooManyInputs & BufferTooSmall boundary guards") {
    // 1. 非法短选项与长选项拒绝
    const bad_short: [1]([]const u8) = ["-q"];
    let caught_short = false;
    * = CatPlan.parse(bad_short[0..]) catch {
        caught_short = true;
    };
    if (!caught_short) return err.ExpectedIllegalShort;

    const bad_long: [1]([]const u8) = ["--unknown-flag"];
    let caught_long = false;
    * = CatPlan.parse(bad_long[0..]) catch {
        caught_long = true;
    };
    if (!caught_long) return err.ExpectedIllegalLong;

    // 2. 超过 SHELL_CAT_MAX_INPUTS (32) 个输入拒绝
    if (SHELL_CAT_MAX_INPUTS != 32) return err.UnexpectedMaxInputs;
    let many: [33]([]const u8) = ["f.txt"; 33];
    let caught_many = false;
    * = CatPlan.parse(many[0..]) catch {
        caught_many = true;
    };
    if (!caught_many) return err.ExpectedTooManyInputs;

    // 3. 输出缓冲过小拒绝（passthrough 与格式化路径均受保护）
    let tiny: [3]u8 = [0; 3];
    let caught_small_pass = false;
    * = formatCatBytes(CatOpts{}, "abcdef", tiny[0..]) catch {
        caught_small_pass = true;
    };
    if (!caught_small_pass) return err.ExpectedSmallBufPassthrough;

    let caught_small_fmt = false;
    * = formatCatBytes(CatOpts{ number_all: true }, "a\n", tiny[0..]) catch {
        caught_small_fmt = true;
    };
    if (!caught_small_fmt) return err.ExpectedSmallBufFormatted;
}
