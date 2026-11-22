// Source: Absorbed from /data/dev/bun/src/shell/builtin/cat.cp | Synced: 2026-11-22T09:30:29Z
// Ported from Bun: /data/dev/bun/src/shell/builtin/cat.cp & /data/dev/bun/docs/runtime/shell.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation POSIX/GNU `cat` option parser & streaming line formatter:
// - Parses bundled short flags (-b/-e/-E/-n/-s/-t/-T/-u/-v/-A), long flags, `--` sentinel & `-` stdin operands
// - Supports Bun strict compatibility mode (flagging unsupported options) as well as full POSIX/GNU mode
// - Streaming stateful CatLineFormatter handles cross-chunk numbering (-n / -b), blank squeezing (-s),
//   line-end markers (-E), tab visualization (-T), and non-printing caret/meta notation (-v) with zero heap allocation.

/// 单次 `cat` 命令支持的最大输入操作数数量
pub const SHELL_CAT_MAX_INPUTS: usize = 32;

/// `cat` 选项解析与格式化错误集
pub err ShellCatError {
    IllegalOption,
    UnsupportedOption,
    TooManyInputs,
    BufferTooSmall,
}

/// `cat` 单个输入源类型
pub enum CatInputKind: u8 {
    /// 标准输入（无参数或显式 `-` 操作数）
    Stdin = 0,
    /// 文件路径操作数
    File = 1,
}

/// `cat` 输入条目
pub struct CatInputEntry {
    kind: CatInputKind = .Stdin,
    path: []const u8 = "-",
}

/// `cat` 命令行选项集合（对齐 Bun `shell/builtin/cat.cp` `Opts` 与 POSIX/GNU 扩展）
pub struct CatOpts {
    number_nonblank: bool = false,
    show_ends: bool = false,
    number_all: bool = false,
    squeeze_blank: bool = false,
    show_tabs: bool = false,
    disable_output_buffering: bool = false,
    show_nonprinting: bool = false,

    /// 无任何内容改写标志时，可走零变换直通快速路径
    fn isPassthrough(): bool {
        return !_.number_nonblank &&
            !_.show_ends &&
            !_.number_all &&
            !_.squeeze_blank &&
            !_.show_tabs &&
            !_.show_nonprinting;
    }

    /// `-b` 仅对非空行编号
    fn effectiveNumberNonblank(): bool {
        return _.number_nonblank;
    }

    /// POSIX/GNU 规则：`-b` 优先级高于 `-n`，同时指定时 `-n` 被抑制
    fn effectiveNumberAll(): bool {
        return _.number_all && !_.number_nonblank;
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// `cat` 解析后的执行计划（零动态堆分配）
pub struct CatPlan {
    opts: CatOpts = CatOpts{},
    inputs: [SHELL_CAT_MAX_INPUTS]CatInputEntry = [CatInputEntry{}; SHELL_CAT_MAX_INPUTS],
    input_count: usize = 0,

    /// 以完整 POSIX/GNU 模式解析 `cat` 参数
    static fn parse(args: []const ([]const u8)): CatPlan! {
        return CatPlan.parseWithMode(args, false);
    }

    /// 解析 `cat` 参数；当 `strict_bun_compat = true` 时，对齐 Bun `shell/builtin/cat.cp` 对未实现标志返回 `UnsupportedOption`
    static fn parseWithMode(args: []const ([]const u8), strict_bun_compat: bool): CatPlan! {
        let plan = CatPlan{};
        let parsing_flags = true;
        let idx: usize = 0;

        while (idx < args.len) {
            const arg = args[idx];
            if (parsing_flags && arg.len >= 2 && arg[0] == '-') {
                if (strEq(arg, "--")) {
                    parsing_flags = false;
                    idx += 1;
                    continue;
                }
                if (arg[1] == '-') {
                    if (strict_bun_compat) {
                        return err.IllegalOption;
                    }
                    if (strEq(arg, "--number-nonblank")) {
                        plan.opts.number_nonblank = true;
                    } else if (strEq(arg, "--number")) {
                        plan.opts.number_all = true;
                    } else if (strEq(arg, "--squeeze-blank")) {
                        plan.opts.squeeze_blank = true;
                    } else if (strEq(arg, "--show-ends")) {
                        plan.opts.show_ends = true;
                    } else if (strEq(arg, "--show-tabs")) {
                        plan.opts.show_tabs = true;
                    } else if (strEq(arg, "--show-nonprinting")) {
                        plan.opts.show_nonprinting = true;
                    } else if (strEq(arg, "--show-all")) {
                        plan.opts.show_nonprinting = true;
                        plan.opts.show_ends = true;
                        plan.opts.show_tabs = true;
                    } else {
                        return err.IllegalOption;
                    }
                    idx += 1;
                    continue;
                }

                // 短选项聚合解析（如 `-nsET`, `-b`, `-e`, `-t`, `-A`）
                let c_idx: usize = 1;
                while (c_idx < arg.len) {
                    const ch = arg[c_idx];
                    if (strict_bun_compat) {
                        if (ch == 'b' || ch == 'e' || ch == 'n' || ch == 's' || ch == 't' || ch == 'u' || ch == 'v') {
                            return err.UnsupportedOption;
                        }
                        return err.IllegalOption;
                    }
                    if (ch == 'b') {
                        plan.opts.number_nonblank = true;
                    } else if (ch == 'e') {
                        plan.opts.show_nonprinting = true;
                        plan.opts.show_ends = true;
                    } else if (ch == 'E') {
                        plan.opts.show_ends = true;
                    } else if (ch == 'n') {
                        plan.opts.number_all = true;
                    } else if (ch == 's') {
                        plan.opts.squeeze_blank = true;
                    } else if (ch == 't') {
                        plan.opts.show_nonprinting = true;
                        plan.opts.show_tabs = true;
                    } else if (ch == 'T') {
                        plan.opts.show_tabs = true;
                    } else if (ch == 'u') {
                        plan.opts.disable_output_buffering = true;
                    } else if (ch == 'v') {
                        plan.opts.show_nonprinting = true;
                    } else if (ch == 'A') {
                        plan.opts.show_nonprinting = true;
                        plan.opts.show_ends = true;
                        plan.opts.show_tabs = true;
                    } else {
                        return err.IllegalOption;
                    }
                    c_idx += 1;
                }
                idx += 1;
                continue;
            }

            // 操作数解析：单独 `-` 在 `--` 前表示 stdin，在 `--` 后按普通文件名 `-` 处理
            if (plan.input_count >= SHELL_CAT_MAX_INPUTS) {
                return err.TooManyInputs;
            }
            if (parsing_flags && strEq(arg, "-")) {
                plan.inputs[plan.input_count] = CatInputEntry{
                    kind: .Stdin,
                    path: "-",
                };
            } else {
                plan.inputs[plan.input_count] = CatInputEntry{
                    kind: .File,
                    path: arg,
                };
            }
            plan.input_count += 1;
            idx += 1;
        }

        // 无任何操作数时，按 POSIX 与 Bun 规范默认读取标准输入
        if (plan.input_count == 0) {
            plan.inputs[0] = CatInputEntry{
                kind: .Stdin,
                path: "-",
            };
            plan.input_count = 1;
        }

        return plan;
    }

    fn inputsSlice(): []const CatInputEntry {
        return _.inputs[0.._.input_count];
    }

    fn readsStdin(): bool {
        let i: usize = 0;
        while (i < _.input_count) {
            if (_.inputs[i].kind == .Stdin) return true;
            i += 1;
        }
        return false;
    }
}

fn pushByte(out: []u8, pos: *usize, b: u8): ! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn pushLineNumber(out: []u8, pos: *usize, num: u64): ! {
    // POSIX/GNU `cat` 标准格式：至少 6 位右对齐十进制行号 + '\t'（如 "     1\t"）
    let digits: [20]u8 = [0; 20];
    let d_len: usize = 0;
    let n = num;
    if (n == 0) {
        digits[0] = '0';
        d_len = 1;
    } else {
        while (n > 0) {
            const rem: u8 = @intCast(n % 10);
            digits[d_len] = (48 as u8) + rem;
            d_len += 1;
            n = n / 10;
        }
    }

    let pad: usize = 0;
    if (d_len < 6) {
        pad = (6 as usize) - d_len;
    }
    let p: usize = 0;
    while (p < pad) {
        pushByte(out, pos, ' ')?;
        p += 1;
    }

    let k: usize = d_len;
    while (k > 0) {
        k -= 1;
        pushByte(out, pos, digits[k])?;
    }
    pushByte(out, pos, '\t')?;
}

fn pushFormattedChar(opts: CatOpts, out: []u8, pos: *usize, ch: u8): ! {
    if (ch == '\t') {
        if (opts.show_tabs) {
            pushByte(out, pos, '^')?;
            pushByte(out, pos, 'I')?;
        } else {
            pushByte(out, pos, '\t')?;
        }
        return;
    }

    if (!opts.show_nonprinting) {
        pushByte(out, pos, ch)?;
        return;
    }

    // GNU/POSIX `-v` 不可打印字符表示法：
    // 0x00..0x1F (非 \n/\t) -> ^@..^_
    // 0x20..0x7E -> 原样
    // 0x7F -> ^?
    // 0x80..0xFF -> M- + 低 7 位规则
    if (ch < 0x20) {
        pushByte(out, pos, '^')?;
        pushByte(out, pos, ch + 64)?;
    } else if (ch < 0x7F) {
        pushByte(out, pos, ch)?;
    } else if (ch == 0x7F) {
        pushByte(out, pos, '^')?;
        pushByte(out, pos, '?')?;
    } else {
        pushByte(out, pos, 'M')?;
        pushByte(out, pos, '-')?;
        const low = ch - 0x80;
        if (low < 0x20) {
            pushByte(out, pos, '^')?;
            pushByte(out, pos, low + 64)?;
        } else if (low < 0x7F) {
            pushByte(out, pos, low)?;
        } else {
            pushByte(out, pos, '^')?;
            pushByte(out, pos, '?')?;
        }
    }
}

/// 有状态流式 `cat` 文本格式化器（支持跨 chunk 与多文件连续编号、空行压缩与不可见字符可视化）
pub struct CatLineFormatter {
    opts: CatOpts = CatOpts{},
    line_number: u64 = 1,
    at_line_start: bool = true,
    prev_blank: bool = false,

    static fn init(opts: CatOpts): CatLineFormatter {
        return CatLineFormatter{
            opts: opts,
            line_number: 1,
            at_line_start: true,
            prev_blank: false,
        };
    }

    fn reset(*_) {
        _.line_number = 1;
        _.at_line_start = true;
        _.prev_blank = false;
    }

    /// 将单块输入字节流按 `CatOpts` 格式化写入 `out`，返回写入字节数
    fn formatChunk(*_, input: []const u8, out: []u8): usize! {
        if (_.opts.isPassthrough()) {
            if (out.len < input.len) {
                return err.BufferTooSmall;
            }
            let i: usize = 0;
            while (i < input.len) {
                out[i] = input[i];
                i += 1;
            }
            return input.len;
        }

        let pos: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            const ch = input[i];
            if (_.at_line_start) {
                if (ch == '\n') {
                    if (_.opts.squeeze_blank && _.prev_blank) {
                        i += 1;
                        continue;
                    }
                    _.prev_blank = true;
                    if (_.opts.effectiveNumberAll()) {
                        pushLineNumber(out, &pos, _.line_number)?;
                        _.line_number += 1;
                    }
                    if (_.opts.show_ends) {
                        pushByte(out, &pos, '$')?;
                    }
                    pushByte(out, &pos, '\n')?;
                    _.at_line_start = true;
                    i += 1;
                    continue;
                }

                _.prev_blank = false;
                if (_.opts.effectiveNumberNonblank() || _.opts.effectiveNumberAll()) {
                    pushLineNumber(out, &pos, _.line_number)?;
                    _.line_number += 1;
                }
                _.at_line_start = false;
                pushFormattedChar(_.opts, out, &pos, ch)?;
                i += 1;
                continue;
            }

            if (ch == '\n') {
                if (_.opts.show_ends) {
                    pushByte(out, &pos, '$')?;
                }
                pushByte(out, &pos, '\n')?;
                _.at_line_start = true;
            } else {
                pushFormattedChar(_.opts, out, &pos, ch)?;
            }
            i += 1;
        }

        return pos;
    }
}

/// 一次性格式化单段字节流的便捷封装
pub fn formatCatBytes(opts: CatOpts, input: []const u8, out: []u8): usize! {
    let fmt = CatLineFormatter.init(opts);
    return fmt.formatChunk(input, out);
}
