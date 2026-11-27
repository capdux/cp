// Source: Absorbed from /data/dev/bun/src/bun_core/output.cp | Synced: 2026-11-23T21:08:17Z (ANSI 16-Color Style & Visible Length)

const ESC: u8 = 0x1B;
const BEL: u8 = 0x07;

/// ANSI 标准 16 色终端颜色
export enum AnsiColor {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    bright_black,
    bright_red,
    bright_green,
    bright_yellow,
    bright_blue,
    bright_magenta,
    bright_cyan,
    bright_white,

    /// 返回前景色 SGR 代码（30..37 / 90..97）
    fn fgCode(): u8 {
        return switch (_) {
            .black => 30,
            .red => 31,
            .green => 32,
            .yellow => 33,
            .blue => 34,
            .magenta => 35,
            .cyan => 36,
            .white => 37,
            .bright_black => 90,
            .bright_red => 91,
            .bright_green => 92,
            .bright_yellow => 93,
            .bright_blue => 94,
            .bright_magenta => 95,
            .bright_cyan => 96,
            .bright_white => 97,
        };
    }

    /// 返回背景色 SGR 代码（40..47 / 100..107）
    fn bgCode(): u8 {
        return switch (_) {
            .black => 40,
            .red => 41,
            .green => 42,
            .yellow => 43,
            .blue => 44,
            .magenta => 45,
            .cyan => 46,
            .white => 47,
            .bright_black => 100,
            .bright_red => 101,
            .bright_green => 102,
            .bright_yellow => 103,
            .bright_blue => 104,
            .bright_magenta => 105,
            .bright_cyan => 106,
            .bright_white => 107,
        };
    }
}

struct BufWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) {
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn writeU8Dec(*_, val: u8): bool {
        if (val >= 100) {
            if (!_.writeByte('0' + val / 100)) return false;
            if (!_.writeByte('0' + (val / 10) % 10)) return false;
            return _.writeByte('0' + val % 10);
        }
        if (val >= 10) {
            if (!_.writeByte('0' + val / 10)) return false;
            return _.writeByte('0' + val % 10);
        }
        return _.writeByte('0' + val);
    }

    fn writeSgrParam(*_, has_param: *bool, code: u8): bool {
        if (has_param.*) {
            if (!_.writeByte(';')) return false;
        }
        has_param.* = true;
        return _.writeU8Dec(code);
    }
}

/// ANSI 文本样式描述器（纯 CP、零外部依赖、零堆分配）
export struct AnsiStyle {
    fg: AnsiColor? = null;
    bg: AnsiColor? = null;
    bold: bool = false;
    dim: bool = false;
    italic: bool = false;
    underline: bool = false;
    strikethrough: bool = false;

    /// 判断样式是否为空（无任何前景色/背景色/修饰）
    fn isEmpty(): bool {
        return _.fg == null
            && _.bg == null
            && !_.bold
            && !_.dim
            && !_.italic
            && !_.underline
            && !_.strikethrough;
    }

    /// 按当前样式格式化文本为 `\x1b[...m<text>\x1b[0m`；空样式直接返回原文本
    fn format(text: []const u8, out: []u8): ([]const u8)? {
        if (_.isEmpty()) {
            return text;
        }
        let w = BufWriter{ out: out, pos: 0 };
        let has_param = false;

        if (!w.writeByte(ESC)) return null;
        if (!w.writeByte('[')) return null;

        if (_.bold) {
            if (!w.writeSgrParam(&has_param, 1)) return null;
        }
        if (_.dim) {
            if (!w.writeSgrParam(&has_param, 2)) return null;
        }
        if (_.italic) {
            if (!w.writeSgrParam(&has_param, 3)) return null;
        }
        if (_.underline) {
            if (!w.writeSgrParam(&has_param, 4)) return null;
        }
        if (_.strikethrough) {
            if (!w.writeSgrParam(&has_param, 9)) return null;
        }
        if (fg_color := _.fg) {
            if (!w.writeSgrParam(&has_param, fg_color.fgCode())) return null;
        }
        if (bg_color := _.bg) {
            if (!w.writeSgrParam(&has_param, bg_color.bgCode())) return null;
        }

        if (!w.writeByte('m')) return null;
        if (!w.writeSlice(text)) return null;
        if (!w.writeSlice("\x1b[0m")) return null;
        return out[0..w.pos];
    }

    /// 24-bit TrueColor 前景色格式化：`\x1b[38;2;<r>;<g>;<b>m<text>\x1b[0m`
    static fn formatRgb(r: u8, g: u8, b: u8, text: []const u8, out: []u8): ([]const u8)? {
        let w = BufWriter{ out: out, pos: 0 };
        if (!w.writeSlice("\x1b[38;2;")) return null;
        if (!w.writeU8Dec(r)) return null;
        if (!w.writeByte(';')) return null;
        if (!w.writeU8Dec(g)) return null;
        if (!w.writeByte(';')) return null;
        if (!w.writeU8Dec(b)) return null;
        if (!w.writeByte('m')) return null;
        if (!w.writeSlice(text)) return null;
        if (!w.writeSlice("\x1b[0m")) return null;
        return out[0..w.pos];
    }
}

/// 若 input[start..] 起始于 ANSI 转义序列，返回该序列消耗的字节长度；否则返回 0
fn ansiSeqLen(input: []const u8, start: usize): usize {
    if (start >= input.len || input[start] != ESC) {
        return 0;
    }
    if (start + 1 >= input.len) {
        return 0;
    }
    const next = input[start + 1];

    if (next == '[') {
        let i = start + 2;
        while (i < input.len && input[i] >= 0x30 && input[i] <= 0x3F) {
            i += 1;
        }
        while (i < input.len && input[i] >= 0x20 && input[i] <= 0x2F) {
            i += 1;
        }
        if (i < input.len && input[i] >= 0x40 && input[i] <= 0x7E) {
            return (i + 1) - start;
        }
        return i - start;
    }

    if (next == ']') {
        let i = start + 2;
        while (i < input.len) {
            if (input[i] == BEL) {
                return (i + 1) - start;
            }
            if (input[i] == ESC && i + 1 < input.len && input[i + 1] == '\\') {
                return (i + 2) - start;
            }
            i += 1;
        }
        return input.len - start;
    }

    if (next >= 0x30 && next <= 0x7E) {
        return 2;
    }
    return 0;
}

/// 统计忽略所有 ANSI 转义序列后的纯可见字节/ASCII 字符长度
export fn visibleLen(input: []const u8): usize {
    let i: usize = 0;
    let count: usize = 0;
    while (i < input.len) {
        const seq_len = ansiSeqLen(input, i);
        if (seq_len > 0) {
            i += seq_len;
            continue;
        }
        count += 1;
        i += 1;
    }
    return count;
}
