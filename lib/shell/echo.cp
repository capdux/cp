// Source: Absorbed from /data/dev/bun/src/shell/builtin/echo.cp | Synced: 2026-11-22T02:15:38Z
// Pure-CP zero-allocation GNU/Bash `echo` flag parser (`-n`, `-e`, `-E` and
// combined forms), backslash escape expander (`\a`, `\b`, `\c`, `\e`/`\E`,
// `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\0NNN`, `\xHH`), and formatter.

/// `echo` 格式化错误集
pub err EchoError {
    BufferTooSmall,
}

/// 解析后的 `echo` 选项标志与正文起始参数索引
pub struct EchoOptions {
    no_newline: bool = false,
    escape_sequences: bool = false,
    args_start: usize = 0,

    /// 按 GNU/Bash/Bun `echo` 语义解析前导组合标志（`-n`, `-e`, `-E`）
    /// - 仅包含 `n`/`e`/`E` 的 `-...` 参数视为有效标志；`-e` 与 `-E` 后者覆盖前者
    /// - 遇到 `-`、空串、非 `-` 开头或含其他字符（如 `-z` / `-ne1` / `--`）即停止标志扫描
    static fn parseFlags(args: []const string): EchoOptions {
        let opts = EchoOptions{
            no_newline: false,
            escape_sequences: false,
            args_start: 0,
        };
        let idx: usize = 0;
        while (idx < args.len) {
            const flag = args[idx];
            if (flag.len < 2 || flag[0] != '-') {
                break;
            }
            let valid = true;
            let k: usize = 1;
            while (k < flag.len) {
                const c = flag[k];
                if (c != 'n' && c != 'e' && c != 'E') {
                    valid = false;
                    break;
                }
                k += 1;
            }
            if (!valid) {
                break;
            }
            k = 1;
            while (k < flag.len) {
                const c = flag[k];
                if (c == 'n') {
                    opts.no_newline = true;
                } else if (c == 'e') {
                    opts.escape_sequences = true;
                } else if (c == 'E') {
                    opts.escape_sequences = false;
                }
                k += 1;
            }
            opts.args_start += 1;
            idx += 1;
        }
        return opts;
    }
}

/// 单段字符串反斜杠转义展开结果
pub struct EchoEscapeResult {
    /// 写入输出缓冲区的字节数
    written: usize,
    /// 是否遇到 `\c`（遇到 `\c` 时立即终止所有后续输出并抑制尾换行）
    stopped: bool,
}

fn hexDigitValue(c: u8): u8? {
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return null;
}

fn appendByte(buf: []u8, pos: usize, b: u8): usize! {
    if (pos >= buf.len) {
        return err.BufferTooSmall;
    }
    buf[pos] = b;
    return pos + 1;
}

fn appendSlice(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

/// 零堆分配展开 `echo -e` 反斜杠转义序列并写入 `buf`
pub fn expandEchoEscapes(input: string, buf: []u8): EchoEscapeResult! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '\\' && i + 1 < input.len) {
            const next = input[i + 1];
            if (next == '\\') {
                pos = appendByte(buf, pos, '\\')?;
                i += 2;
            } else if (next == 'a') {
                pos = appendByte(buf, pos, 0x07)?;
                i += 2;
            } else if (next == 'b') {
                pos = appendByte(buf, pos, 0x08)?;
                i += 2;
            } else if (next == 'c') {
                return EchoEscapeResult{ written: pos, stopped: true };
            } else if (next == 'e' || next == 'E') {
                pos = appendByte(buf, pos, 0x1b)?;
                i += 2;
            } else if (next == 'f') {
                pos = appendByte(buf, pos, 0x0c)?;
                i += 2;
            } else if (next == 'n') {
                pos = appendByte(buf, pos, '\n')?;
                i += 2;
            } else if (next == 'r') {
                pos = appendByte(buf, pos, '\r')?;
                i += 2;
            } else if (next == 't') {
                pos = appendByte(buf, pos, '\t')?;
                i += 2;
            } else if (next == 'v') {
                pos = appendByte(buf, pos, 0x0b)?;
                i += 2;
            } else if (next == '0') {
                i += 2;
                let val: u16 = 0;
                let digits: usize = 0;
                while (digits < 3 && i < input.len && input[i] >= '0' && input[i] <= '7') {
                    val = (val * 8 + ((input[i] - '0') as u16)) & 0xff;
                    i += 1;
                    digits += 1;
                }
                pos = appendByte(buf, pos, val as u8)?;
            } else if (next == 'x') {
                i += 2;
                let val: u16 = 0;
                let digits: usize = 0;
                while (digits < 2 && i < input.len) {
                    if (hv := hexDigitValue(input[i])) {
                        val = (val * 16 + (hv as u16)) & 0xff;
                        i += 1;
                        digits += 1;
                    } else {
                        break;
                    }
                }
                if (digits > 0) {
                    pos = appendByte(buf, pos, val as u8)?;
                } else {
                    pos = appendSlice(buf, pos, "\\x")?;
                }
            } else {
                pos = appendByte(buf, pos, '\\')?;
                pos = appendByte(buf, pos, next)?;
                i += 2;
            }
        } else {
            pos = appendByte(buf, pos, input[i])?;
            i += 1;
        }
    }
    return EchoEscapeResult{ written: pos, stopped: false };
}

/// 按 GNU/Bash/Bun `echo` 完整语义格式化参数切片到调用方缓冲区 `buf`
pub fn formatEchoArgs(args: []const string, buf: []u8): string! {
    const opts = EchoOptions.parseFlags(args);
    const body = args[opts.args_start..];

    let pos: usize = 0;
    let stop_output = false;
    let i: usize = 0;
    while (i < body.len) {
        if (stop_output) {
            break;
        }
        if (i > 0) {
            pos = appendByte(buf, pos, ' ')?;
        }
        if (opts.escape_sequences) {
            const res = expandEchoEscapes(body[i], buf[pos..])?;
            pos += res.written;
            if (res.stopped) {
                stop_output = true;
                break;
            }
        } else {
            pos = appendSlice(buf, pos, body[i])?;
        }
        i += 1;
    }

    if (!opts.no_newline && !stop_output) {
        pos = appendByte(buf, pos, '\n')?;
    }
    return buf[0..pos];
}
