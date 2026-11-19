// Source: /data/dev/bun/src/bun_core/tty.cp
// lib/cli/tty.cp
// Terminal Mode, ANSI Escape Color Formatting & Progress Bar Renderer

pub enum TtyMode: i32 {
    Normal = 0,
    Raw = 1,
    Io = 2,
}

pub struct AnsiColor {
    pub const RESET: []const u8 = "\x1b[0m";
    pub const BOLD: []const u8 = "\x1b[1m";
    pub const DIM: []const u8 = "\x1b[2m";
    pub const RED: []const u8 = "\x1b[31m";
    pub const GREEN: []const u8 = "\x1b[32m";
    pub const YELLOW: []const u8 = "\x1b[33m";
    pub const BLUE: []const u8 = "\x1b[34m";
    pub const MAGENTA: []const u8 = "\x1b[35m";
    pub const CYAN: []const u8 = "\x1b[36m";
    pub const GRAY: []const u8 = "\x1b[90m";

    /// Strips CSI ANSI escape sequences (`\x1b[...m`) from `input` into `out_buf`.
    pub static fn stripAnsi(input: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_len: usize = 0;
        while (i < input.len && out_len < out_buf.len) {
            if (input[i] == 0x1b && i + 1 < input.len && input[i + 1] == '[') {
                i += 2;
                while (i < input.len) {
                    const c = input[i];
                    i += 1;
                    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                        break;
                    }
                }
                continue;
            }
            out_buf[out_len] = input[i];
            out_len += 1;
            i += 1;
        }
        return out_buf[0..out_len];
    }
}

pub struct ProgressBar {
    total: u64 = 100;
    current: u64 = 0;
    width: usize = 20;

    pub static fn init(total: u64, width: usize): ProgressBar {
        return ProgressBar {
            total: if (total == 0) 1 else total,
            current: 0,
            width: if (width == 0) 20 else width,
        };
    }

    pub fn tick(*_, delta: u64) {
        if (_.current + delta >= _.total) {
            _.current = _.total;
        } else {
            _.current += delta;
        }
    }

    pub fn set(*_, value: u64) {
        _.current = if (value > _.total) _.total else value;
    }

    pub fn isComplete(): bool {
        return _.current >= _.total;
    }

    pub fn percent(): u8 {
        if (_.total == 0) {
            return 100;
        }
        const pct = (_.current * 100) / _.total;
        return (if (pct > 100) 100 else pct) as u8;
    }

    /// Renders an ASCII progress bar like `[#####-----]` into `out_buf`.
    pub fn renderBar(out_buf: []u8): []const u8 {
        const needed = _.width + 2;
        if (out_buf.len < needed) {
            return "";
        }
        const filled = ((_.current * (_.width as u64)) / _.total) as usize;
        out_buf[0] = '[';
        let i: usize = 0;
        while (i < _.width) {
            out_buf[1 + i] = if (i < filled) ('#' as u8) else ('-' as u8);
            i += 1;
        }
        out_buf[1 + _.width] = ']';
        return out_buf[0..needed];
    }
}

pub const Progress = ProgressBar;
