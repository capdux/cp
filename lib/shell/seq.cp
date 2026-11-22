// Source: Absorbed from /data/dev/bun/src/shell/builtin/seq.cp | Synced: 2026-11-22T05:15:00Z
// ============================================================================
// POSIX / GNU `seq` Numeric Sequence Parser, Iterator & Formatter
// Absorbed from Bun: src/shell/builtin/seq.cp
// ============================================================================

/// 默认最大步数上限（防畸形极小步长导致资源耗尽）
pub const DEFAULT_SEQ_MAX_STEPS: usize = 1000000;

/// `seq` 参数解析与序列生成错误集
pub err SeqError {
    MissingOperand,
    MissingOptionArg,
    InvalidNumber,
    ZeroIncrement,
    WrongDirection,
    TooManyOperands,
    StepLimitExceeded,
    OutOfMemory,
}

/// 解析后的 `seq` 命令行选项与范围参数
pub struct SeqOptions {
    start: f64 = 1.0,
    end: f64 = 1.0,
    increment: f64 = 1.0,
    separator: []const u8 = "\n",
    terminator: []const u8 = "",
    fixed_width: bool = false,
    max_steps: usize = DEFAULT_SEQ_MAX_STEPS,

    /// 解析 `seq` 命令行参数：
    /// - `-s SEP` / `-sSEP` / `--separator SEP` / `--separator=SEP`
    /// - `-t TERM` / `-tTERM` / `--terminator TERM` / `--terminator=TERM`
    /// - `-w` / `--fixed-width` / `--equal-width`
    /// - `--` 显式结束选项区
    /// - 位置参数：`LAST` | `FIRST LAST` | `FIRST INCREMENT LAST`
    static fn parse(args: []const []const u8): SeqOptions! {
        if (args.len == 0) {
            return err.MissingOperand;
        }

        let opts = SeqOptions{};
        let idx: usize = 0;

        while (idx < args.len) {
            const arg = args[idx];
            if (std.mem.eql(u8, arg, "--")) {
                idx += 1;
                break;
            }
            if (std.mem.eql(u8, arg, "-s") || std.mem.eql(u8, arg, "--separator")) {
                idx += 1;
                if (idx >= args.len) {
                    return err.MissingOptionArg;
                }
                opts.separator = args[idx];
                idx += 1;
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--separator=")) {
                opts.separator = arg[12..];
                idx += 1;
                continue;
            }
            if (std.mem.startsWith(u8, arg, "-s") && arg.len > 2) {
                opts.separator = arg[2..];
                idx += 1;
                continue;
            }
            if (std.mem.eql(u8, arg, "-t") || std.mem.eql(u8, arg, "--terminator")) {
                idx += 1;
                if (idx >= args.len) {
                    return err.MissingOptionArg;
                }
                opts.terminator = args[idx];
                idx += 1;
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--terminator=")) {
                opts.terminator = arg[13..];
                idx += 1;
                continue;
            }
            if (std.mem.startsWith(u8, arg, "-t") && arg.len > 2) {
                opts.terminator = arg[2..];
                idx += 1;
                continue;
            }
            if (std.mem.eql(u8, arg, "-w") ||
                std.mem.eql(u8, arg, "--fixed-width") ||
                std.mem.eql(u8, arg, "--equal-width"))
            {
                opts.fixed_width = true;
                idx += 1;
                continue;
            }
            break;
        }

        const rem = args.len - idx;
        if (rem == 0) {
            return err.MissingOperand;
        }
        if (rem > 3) {
            return err.TooManyOperands;
        }

        const n1 = parseFiniteF64(args[idx])?;
        if (rem == 1) {
            opts.start = 1.0;
            opts.end = n1;
            opts.increment = if (opts.start > opts.end) -1.0 else 1.0;
            return opts;
        }

        const n2 = parseFiniteF64(args[idx + 1])?;
        if (rem == 2) {
            opts.start = n1;
            opts.end = n2;
            opts.increment = if (opts.start > opts.end) -1.0 else 1.0;
            return opts;
        }

        const n3 = parseFiniteF64(args[idx + 2])?;
        opts.start = n1;
        opts.increment = n2;
        opts.end = n3;

        if (opts.increment == 0.0) {
            return err.ZeroIncrement;
        }
        if (opts.start > opts.end && opts.increment > 0.0) {
            return err.WrongDirection;
        }
        if (opts.start < opts.end && opts.increment < 0.0) {
            return err.WrongDirection;
        }
        return opts;
    }

    /// 预估序列生成的元素总数（用于容量预估与步数上限校验）
    fn estimatedCount(): usize {
        if (_.increment == 0.0) {
            return 0;
        }
        if (_.increment > 0.0 && _.start > _.end) {
            return 0;
        }
        if (_.increment < 0.0 && _.start < _.end) {
            return 0;
        }
        const span = (_.end - _.start) / _.increment;
        if (span < 0.0 || !std.math.isFinite(span)) {
            return 0;
        }
        const eps: f64 = 1.0e-9;
        const steps_f = @floor(span + eps);
        if (steps_f < 0.0) {
            return 0;
        }
        if (steps_f > 1.0e15) {
            return _.max_steps + 1;
        }
        return @as(usize, @intFromFloat(steps_f)) + 1;
    }
}

/// 零堆分配惰性数字序列迭代器（使用 `start + step * increment` 消除浮点累加漂移）
pub struct SeqIter {
    opts: SeqOptions,
    step: usize = 0,

    static fn init(opts: SeqOptions): SeqIter {
        return SeqIter{
            opts: opts,
            step: 0,
        };
    }

    fn reset(*_) {
        _.step = 0;
    }

    fn next(*_): f64? {
        if (_.opts.increment == 0.0 || _.step >= _.opts.max_steps) {
            return null;
        }
        const step_f: f64 = @floatFromInt(_.step);
        let val = _.opts.start + step_f * _.opts.increment;
        // 微小浮点误差容差（如 0.1 + 0.2 = 0.30000000000000004）
        const tol = @abs(_.opts.increment) * 1.0e-9;
        if (_.opts.increment > 0.0) {
            if (val > _.opts.end + tol) {
                return null;
            }
            if (val > _.opts.end) {
                val = _.opts.end;
            }
        } else {
            if (val < _.opts.end - tol) {
                return null;
            }
            if (val < _.opts.end) {
                val = _.opts.end;
            }
        }
        if (val == 0.0) {
            val = 0.0; // 规范化 -0.0 为 +0.0
        }
        _.step += 1;
        return val;
    }
}

/// 按 `SeqOptions` 渲染完整序列文本（支持 `-w` 等宽前导补零、`-s` 分隔符与 `-t` 终止符）
pub fn formatSeq(allocator: std.mem.Allocator, opts: SeqOptions): []u8! {
    if (opts.increment == 0.0) {
        return err.ZeroIncrement;
    }
    if (opts.start > opts.end && opts.increment > 0.0) {
        return err.WrongDirection;
    }
    if (opts.start < opts.end && opts.increment < 0.0) {
        return err.WrongDirection;
    }
    const est = opts.estimatedCount();
    if (est > opts.max_steps) {
        return err.StepLimitExceeded;
    }

    let out: std.ArrayListUnmanaged(u8) = .{};
    errdefer out.deinit(allocator);

    let pad_width: usize = 0;
    if (opts.fixed_width) {
        const w_start = integerDisplayWidth(opts.start);
        const w_end = integerDisplayWidth(opts.end);
        pad_width = if (w_start > w_end) w_start else w_end;
    }

    let it = SeqIter.init(opts);
    let num_buf: [64]u8 = [0; 64];
    let emitted: usize = 0;

    while (val := it.next()) {
        const raw_str = formatSeqNumber(val, num_buf[0..]);
        if (opts.fixed_width && pad_width > 0) {
            appendPaddedNumber(allocator, &out, raw_str, pad_width)?;
        } else {
            out.appendSlice(allocator, raw_str) catch {
                return err.OutOfMemory;
            };
        }
        out.appendSlice(allocator, opts.separator) catch {
            return err.OutOfMemory;
        };
        emitted += 1;
    }

    if (emitted > 0 && opts.terminator.len > 0) {
        out.appendSlice(allocator, opts.terminator) catch {
            return err.OutOfMemory;
        };
    }

    return out.toOwnedSlice(allocator) catch {
        return err.OutOfMemory;
    };
}

/// 便捷入口：直接解析参数切片并生成完整 `seq` 输出文本
pub fn formatSeqArgs(allocator: std.mem.Allocator, args: []const []const u8): []u8! {
    const opts = SeqOptions.parse(args)?;
    return formatSeq(allocator, opts);
}

// ============================================================================
// 内部辅助函数
// ============================================================================

fn parseFiniteF64(raw: []const u8): f64! {
    if (raw.len == 0) {
        return err.InvalidNumber;
    }
    const v = std.fmt.parseFloat(f64, raw) catch {
        return err.InvalidNumber;
    };
    if (!std.math.isFinite(v)) {
        return err.InvalidNumber;
    }
    return v;
}

fn formatSeqNumber(val: f64, buf: []u8): []const u8 {
    // 接近整数时按整数格式输出，避免 0.30000000000000004 或 2.0 噪声
    const rounded = @round(val);
    if (@abs(val - rounded) <= 1.0e-9 && @abs(rounded) < 9.0e15) {
        const iv: i64 = @intFromFloat(rounded);
        return std.fmt.bufPrint(buf, "{d}", .{iv}) catch "0";
    }
    // 非整数：保留最多 6 位有效小数并裁剪尾随零
    const full = std.fmt.bufPrint(buf, "{d:.6}", .{val}) catch "0";
    if (std.mem.indexOfScalar(u8, full, '.') != null) {
        let end = full.len;
        while (end > 0 && full[end - 1] == '0') {
            end -= 1;
        }
        if (end > 0 && full[end - 1] == '.') {
            end -= 1;
        }
        return full[0..end];
    }
    return full;
}

fn integerDisplayWidth(val: f64): usize {
    let buf: [64]u8 = [0; 64];
    const s = formatSeqNumber(val, buf[0..]);
    if (dot := std.mem.indexOfScalar(u8, s, '.')) {
        return dot;
    }
    return s.len;
}

fn appendPaddedNumber(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    num_str: []const u8,
    target_width: usize,
): ! {
    const int_len = std.mem.indexOfScalar(u8, num_str, '.') ?? num_str.len;
    if (int_len >= target_width) {
        out.appendSlice(allocator, num_str) catch {
            return err.OutOfMemory;
        };
        return;
    }
    const pad_count = target_width - int_len;
    if (num_str.len > 0 && num_str[0] == '-') {
        out.append(allocator, '-') catch {
            return err.OutOfMemory;
        };
        let p: usize = 0;
        while (p < pad_count) {
            out.append(allocator, '0') catch {
                return err.OutOfMemory;
            };
            p += 1;
        }
        out.appendSlice(allocator, num_str[1..]) catch {
            return err.OutOfMemory;
        };
    } else {
        let p: usize = 0;
        while (p < pad_count) {
            out.append(allocator, '0') catch {
                return err.OutOfMemory;
            };
            p += 1;
        }
        out.appendSlice(allocator, num_str) catch {
            return err.OutOfMemory;
        };
    }
}
