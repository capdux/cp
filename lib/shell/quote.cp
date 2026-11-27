// Source: Absorbed from /data/dev/bun/src/shell/shell.cp | Synced: 2026-11-24T04:51:05Z (POSIX Shell Quoting & Tokenizer)

type ShQuoteVec16 = @Vector(16, u8);

fn makeShSafeLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
            || c == '_' || c == '@' || c == '%' || c == '+' || c == '='
            || c == ':' || c == ',' || c == '.' || c == '/' || c == '-') {
            lut[i] = 1;
        }
    }
    return lut;
}

const SH_SAFE_LUT: [u8; 256] = makeShSafeLut();

fn shSimdFindByte(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_n: ShQuoteVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ShQuoteVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_n);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn shSimdFindAny2(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i = start;
    const va: ShQuoteVec16 = @splat(a);
    const vb: ShQuoteVec16 = @splat(b);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ShQuoteVec16 = s[i .. i + 16][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == a || s[i] == b) return i;
    }
    return s.len;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isSafeChar(c: u8): bool {
    return SH_SAFE_LUT[usize(c)] != 0;
}

/// 判断字符串在 POSIX Shell 命令行中是否需要引号包裹
/// 空串或含任何非 `[A-Za-z0-9_@%+=:,./-]` 字符时返回 true
export fn needsQuoting(s: []const u8): bool {
    if (s.len == 0) {
        return true;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (SH_SAFE_LUT[usize(s[i])] == 0) {
            return true;
        }
    }
    return false;
}

/// 按 POSIX Shell 单引号规则安全转义字符串（零堆分配）
/// 安全字符串原样写入；空串写入 `''`；含特殊字符用单引号包裹，内部 `'` 转义为 `'\''`
export fn quotePosix(s: []const u8, out: []u8): ([]const u8)? {
    if (s.len == 0) {
        if (out.len < 2) {
            return null;
        }
        out[0] = '\'';
        out[1] = '\'';
        return out[0..2];
    }

    if (!needsQuoting(s)) {
        if (out.len < s.len) {
            return null;
        }
        @memcpy(out[0..s.len], s);
        return out[0..s.len];
    }

    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '\'';
    pos += 1;

    let i: usize = 0;
    while (i < s.len) {
        const next_sq = shSimdFindByte(s, i, '\'');
        const run = next_sq - i;
        if (run > 0) {
            if (pos + run > out.len) return null;
            @memcpy(out[pos .. pos + run], s[i..next_sq]);
            pos += run;
            i = next_sq;
        }
        if (i < s.len) {
            if (pos + 4 > out.len) return null;
            out[pos] = '\'';
            out[pos + 1] = '\\';
            out[pos + 2] = '\'';
            out[pos + 3] = '\'';
            pos += 4;
            i += 1;
        }
    }

    if (pos >= out.len) return null;
    out[pos] = '\'';
    pos += 1;
    return out[0..pos];
}

/// 将多个命令行参数按 POSIX 规则安全转义并以空格拼接（零堆分配）
export fn joinArgs(args: []const []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            if (pos >= out.len) return null;
            out[pos] = ' ';
            pos += 1;
        }
        const written = quotePosix(args[i], out[pos..]) ?? return null;
        pos += written.len;
        i += 1;
    }
    return out[0..pos];
}

/// POSIX Shell 命令行参数流式拆分器（零堆分配）
/// 支持单引号字面量、双引号内 `\"`/`\\`/`\$`/`\`` 转义、非引号反斜杠转义、相邻拼接及未闭合引号检测
export struct ShellArgIter {
    src: []const u8;
    pos: usize;
    had_error: bool;

    /// 初始化命令行参数迭代器
    static fn init(cmdline: []const u8): ShellArgIter {
        return ShellArgIter{
            src: cmdline,
            pos: 0,
            had_error: false,
        };
    }

    /// 解析过程中是否遇到词法错误（如未闭合引号、孤立尾部反斜杠或缓冲区不足）
    fn hasError(): bool {
        return _.had_error;
    }

    /// 读取下一个展开后的命令行参数写入 out；到达末尾或出错返回 null
    fn next(*_, out: []u8): ([]const u8)? {
        if (_.had_error) {
            return null;
        }

        while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        let out_pos: usize = 0;

        while (_.pos < _.src.len && !isSpace(_.src[_.pos])) {
            const c = _.src[_.pos];

            // 1. 单引号字面量：内部全部原样保留直至闭合单引号
            if (c == '\'') {
                _.pos += 1;
                const sq_end = shSimdFindByte(_.src, _.pos, '\'');
                if (sq_end >= _.src.len) {
                    _.had_error = true;
                    return null;
                }
                const run = sq_end - _.pos;
                if (out_pos + run > out.len) {
                    _.had_error = true;
                    return null;
                }
                if (run > 0) {
                    @memcpy(out[out_pos .. out_pos + run], _.src[_.pos..sq_end]);
                    out_pos += run;
                }
                _.pos = sq_end + 1;
                continue;
            }

            // 2. 双引号字符串：仅对 \" \\ \$ \` 与续行 \n 生效转义
            if (c == '"') {
                _.pos += 1;
                let closed = false;
                while (_.pos < _.src.len) {
                    const next_sp = shSimdFindAny2(_.src, _.pos, '"', '\\');
                    const run = next_sp - _.pos;
                    if (run > 0) {
                        if (out_pos + run > out.len) {
                            _.had_error = true;
                            return null;
                        }
                        @memcpy(out[out_pos .. out_pos + run], _.src[_.pos..next_sp]);
                        out_pos += run;
                        _.pos = next_sp;
                    }
                    if (_.pos >= _.src.len) {
                        break;
                    }
                    const ch = _.src[_.pos];
                    if (ch == '"') {
                        _.pos += 1;
                        closed = true;
                        break;
                    }
                    if (ch == '\\' && _.pos + 1 < _.src.len) {
                        const nxt = _.src[_.pos + 1];
                        if (nxt == '\n') {
                            _.pos += 2;
                            continue;
                        }
                        if (nxt == '"' || nxt == '\\' || nxt == '$' || nxt == '`') {
                            if (out_pos >= out.len) {
                                _.had_error = true;
                                return null;
                            }
                            out[out_pos] = nxt;
                            out_pos += 1;
                            _.pos += 2;
                            continue;
                        }
                    }
                    if (out_pos >= out.len) {
                        _.had_error = true;
                        return null;
                    }
                    out[out_pos] = ch;
                    out_pos += 1;
                    _.pos += 1;
                }
                if (!closed) {
                    _.had_error = true;
                    return null;
                }
                continue;
            }

            // 3. 非引号反斜杠转义
            if (c == '\\') {
                if (_.pos + 1 >= _.src.len) {
                    _.had_error = true;
                    return null;
                }
                const nxt = _.src[_.pos + 1];
                if (nxt == '\n') {
                    _.pos += 2;
                    continue;
                }
                if (out_pos >= out.len) {
                    _.had_error = true;
                    return null;
                }
                out[out_pos] = nxt;
                out_pos += 1;
                _.pos += 2;
                continue;
            }

            // 4. 普通非空白字符
            if (out_pos >= out.len) {
                _.had_error = true;
                return null;
            }
            out[out_pos] = c;
            out_pos += 1;
            _.pos += 1;
        }

        return out[0..out_pos];
    }
}
