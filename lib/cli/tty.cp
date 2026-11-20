// Source: /data/dev/bun/src/bun_core/tty.cp
// lib/cli/tty.cp
// Terminal Mode, POSIX TTY Size Probing, Full ANSI 16/256/RGB Colors, Spinner & ProgressBar

const std = @import("std");

pub enum TtyMode: i32 {
    Normal = 0,
    Raw = 1,
    Io = 2,
}

pub struct TerminalSize {
    pub cols: u16 = 80;
    pub rows: u16 = 24;
}

pub struct Terminal {
    /// Returns `true` if `fd` refers to an interactive terminal device (`isatty(fd) != 0`).
    pub static fn isTty(fd: i32 = 1): bool {
        if (fd < 0) {
            return false;
        }
        return std.c.isatty(@intCast(fd)) != 0;
    }

    /// Queries terminal window dimensions via Linux `ioctl(TIOCGWINSZ)`.
    /// Safely falls back to `80x24` when `fd` is not a TTY or ioctl fails.
    pub static fn getSize(fd: i32 = 1): TerminalSize {
        if (fd < 0) {
            return TerminalSize{ cols: 80, rows: 24 };
        }
        zig {
            var wsz: std.posix.winsize = .{
                .row = 0,
                .col = 0,
                .xpixel = 0,
                .ypixel = 0,
            };
            const rc = std.os.linux.ioctl(@intCast(fd), std.os.linux.T.IOCGWINSZ, @intFromPtr(&wsz));
            if (rc == 0 and wsz.col > 0 and wsz.row > 0) {
                return TerminalSize{ .cols = wsz.col, .rows = wsz.row };
            }
        }
        return TerminalSize{ cols: 80, rows: 24 };
    }

    pub static fn columns(fd: i32 = 1): u16 {
        return Terminal.getSize(fd).cols;
    }

    pub static fn rows(fd: i32 = 1): u16 {
        return Terminal.getSize(fd).rows;
    }

    pub static fn supportsColor(fd: i32 = 1): bool {
        if (std.c.getenv("NO_COLOR") != null) {
            return false;
        }
        if (term_ptr := std.c.getenv("TERM")) {
            const term = std.mem.span(term_ptr);
            if (std.mem.eql(u8, term, "dumb")) {
                return false;
            }
        }
        return Terminal.isTty(fd);
    }

    pub static fn supportsTrueColor(): bool {
        if (ct_ptr := std.c.getenv("COLORTERM")) {
            const ct = std.mem.span(ct_ptr);
            return std.mem.eql(u8, ct, "truecolor") || std.mem.eql(u8, ct, "24bit");
        }
        return false;
    }
}

pub struct AnsiColor {
    // Text attributes / styles
    pub const RESET: []const u8 = "\x1b[0m";
    pub const BOLD: []const u8 = "\x1b[1m";
    pub const DIM: []const u8 = "\x1b[2m";
    pub const ITALIC: []const u8 = "\x1b[3m";
    pub const UNDERLINE: []const u8 = "\x1b[4m";
    pub const BLINK: []const u8 = "\x1b[5m";
    pub const INVERSE: []const u8 = "\x1b[7m";
    pub const HIDDEN: []const u8 = "\x1b[8m";
    pub const STRIKETHROUGH: []const u8 = "\x1b[9m";

    // 16 standard foreground colors
    pub const BLACK: []const u8 = "\x1b[30m";
    pub const RED: []const u8 = "\x1b[31m";
    pub const GREEN: []const u8 = "\x1b[32m";
    pub const YELLOW: []const u8 = "\x1b[33m";
    pub const BLUE: []const u8 = "\x1b[34m";
    pub const MAGENTA: []const u8 = "\x1b[35m";
    pub const CYAN: []const u8 = "\x1b[36m";
    pub const WHITE: []const u8 = "\x1b[37m";
    pub const GRAY: []const u8 = "\x1b[90m";
    pub const BRIGHT_BLACK: []const u8 = "\x1b[90m";
    pub const BRIGHT_RED: []const u8 = "\x1b[91m";
    pub const BRIGHT_GREEN: []const u8 = "\x1b[92m";
    pub const BRIGHT_YELLOW: []const u8 = "\x1b[93m";
    pub const BRIGHT_BLUE: []const u8 = "\x1b[94m";
    pub const BRIGHT_MAGENTA: []const u8 = "\x1b[95m";
    pub const BRIGHT_CYAN: []const u8 = "\x1b[96m";
    pub const BRIGHT_WHITE: []const u8 = "\x1b[97m";

    // 16 standard background colors
    pub const BG_BLACK: []const u8 = "\x1b[40m";
    pub const BG_RED: []const u8 = "\x1b[41m";
    pub const BG_GREEN: []const u8 = "\x1b[42m";
    pub const BG_YELLOW: []const u8 = "\x1b[43m";
    pub const BG_BLUE: []const u8 = "\x1b[44m";
    pub const BG_MAGENTA: []const u8 = "\x1b[45m";
    pub const BG_CYAN: []const u8 = "\x1b[46m";
    pub const BG_WHITE: []const u8 = "\x1b[47m";
    pub const BG_BRIGHT_BLACK: []const u8 = "\x1b[100m";
    pub const BG_BRIGHT_RED: []const u8 = "\x1b[101m";
    pub const BG_BRIGHT_GREEN: []const u8 = "\x1b[102m";
    pub const BG_BRIGHT_YELLOW: []const u8 = "\x1b[103m";
    pub const BG_BRIGHT_BLUE: []const u8 = "\x1b[104m";
    pub const BG_BRIGHT_MAGENTA: []const u8 = "\x1b[105m";
    pub const BG_BRIGHT_CYAN: []const u8 = "\x1b[106m";
    pub const BG_BRIGHT_WHITE: []const u8 = "\x1b[107m";

    /// Formats a 256-color foreground escape sequence (`\x1b[38;5;Nm`) into `out_buf`.
    pub static fn fg256(code: u8, out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(out_buf, "\x1b[38;5;{d}m", .{code}) catch "";
    }

    /// Formats a 256-color background escape sequence (`\x1b[48;5;Nm`) into `out_buf`.
    pub static fn bg256(code: u8, out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(out_buf, "\x1b[48;5;{d}m", .{code}) catch "";
    }

    /// Formats a 24-bit TrueColor RGB foreground sequence (`\x1b[38;2;R;G;Bm`) into `out_buf`.
    pub static fn fgRgb(r: u8, g: u8, b: u8, out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(out_buf, "\x1b[38;2;{d};{d};{d}m", .{ r, g, b }) catch "";
    }

    /// Formats a 24-bit TrueColor RGB background sequence (`\x1b[48;2;R;G;Bm`) into `out_buf`.
    pub static fn bgRgb(r: u8, g: u8, b: u8, out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(out_buf, "\x1b[48;2;{d};{d};{d}m", .{ r, g, b }) catch "";
    }

    /// Strips ANSI CSI (`\x1b[...`), OSC (`\x1b]...BEL/ST`), and 2-byte ESC sequences from `input` into `out_buf`.
    pub static fn stripAnsi(input: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_len: usize = 0;
        while (i < input.len && out_len < out_buf.len) {
            if (input[i] == 0x1b && i + 1 < input.len) {
                const next_b = input[i + 1];
                if (next_b == '[') {
                    // CSI sequence: ESC [ ... final byte (0x40..0x7E)
                    i += 2;
                    while (i < input.len) {
                        const c = input[i];
                        i += 1;
                        if (c >= 0x40 && c <= 0x7E) {
                            break;
                        }
                    }
                    continue;
                }
                if (next_b == ']') {
                    // OSC sequence: ESC ] ... BEL (0x07) or ST (ESC \)
                    i += 2;
                    while (i < input.len) {
                        if (input[i] == 0x07) {
                            i += 1;
                            break;
                        }
                        if (input[i] == 0x1b && i + 1 < input.len && input[i + 1] == '\\') {
                            i += 2;
                            break;
                        }
                        i += 1;
                    }
                    continue;
                }
                if (next_b >= 0x40 && next_b <= 0x7E) {
                    i += 2;
                    continue;
                }
            }
            out_buf[out_len] = input[i];
            out_len += 1;
            i += 1;
        }
        return out_buf[0..out_len];
    }
}

pub enum SpinnerStyle: u8 {
    Dots = 0,
    Line = 1,
    Arrow = 2,
}

pub enum SpinnerState: u8 {
    Running = 0,
    Succeeded = 1,
    Failed = 2,
}

pub struct Spinner {
    pub style: SpinnerStyle = SpinnerStyle.Dots;
    pub state: SpinnerState = SpinnerState.Running;
    pub frame_idx: usize = 0;
    pub message: []const u8 = "";

    pub static fn init(message: []const u8 = "", style: SpinnerStyle = .Dots): Spinner {
        return Spinner {
            style: style,
            state: SpinnerState.Running,
            frame_idx: 0,
            message: message,
        };
    }

    fn frameCount(): usize {
        if (_.style == SpinnerStyle.Dots) { return 10; }
        if (_.style == SpinnerStyle.Line) { return 4; }
        return 8;
    }

    fn frameAt(idx: usize): []const u8 {
        if (_.style == SpinnerStyle.Dots) {
            const m = idx % 10;
            if (m == 0) { return "⠋"; }
            if (m == 1) { return "⠙"; }
            if (m == 2) { return "⠹"; }
            if (m == 3) { return "⠸"; }
            if (m == 4) { return "⠼"; }
            if (m == 5) { return "⠴"; }
            if (m == 6) { return "⠦"; }
            if (m == 7) { return "⠧"; }
            if (m == 8) { return "⠇"; }
            return "⠏";
        }
        if (_.style == SpinnerStyle.Line) {
            const m = idx % 4;
            if (m == 0) { return "|"; }
            if (m == 1) { return "/"; }
            if (m == 2) { return "-"; }
            return "\\";
        }
        const m = idx % 8;
        if (m == 0) { return "←"; }
        if (m == 1) { return "↖"; }
        if (m == 2) { return "↑"; }
        if (m == 3) { return "↗"; }
        if (m == 4) { return "→"; }
        if (m == 5) { return "↘"; }
        if (m == 6) { return "↓"; }
        return "↙";
    }

    pub fn currentFrame(): []const u8 {
        if (_.state == SpinnerState.Succeeded) { return "✔"; }
        if (_.state == SpinnerState.Failed) { return "✖"; }
        return _.frameAt(_.frame_idx);
    }

    /// Advances the animation frame by one step and returns the new frame glyph.
    pub fn tick(*_): []const u8 {
        const glyph = _.currentFrame();
        if (_.state == SpinnerState.Running) {
            _.frame_idx = (_.frame_idx + 1) % _.frameCount();
        }
        return glyph;
    }

    pub fn setMessage(*_, msg: []const u8) {
        _.message = msg;
    }

    pub fn succeed(*_, msg: []const u8 = "") {
        _.state = SpinnerState.Succeeded;
        if (msg.len > 0) {
            _.message = msg;
        }
    }

    pub fn fail(*_, msg: []const u8 = "") {
        _.state = SpinnerState.Failed;
        if (msg.len > 0) {
            _.message = msg;
        }
    }

    pub fn render(out_buf: []u8): []const u8 {
        const glyph = _.currentFrame();
        if (_.message.len == 0) {
            return std.fmt.bufPrint(out_buf, "{s}", .{glyph}) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s} {s}", .{ glyph, _.message }) catch "";
    }
}

fn monotonicNowNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    return ((ts.sec as u64) * 1000000000) + (ts.nsec as u64);
}

pub struct ProgressBar {
    pub total: u64 = 100;
    pub current: u64 = 0;
    pub width: usize = 20;
    pub start_ns: u64 = 0;
    pub last_ns: u64 = 0;
    pub label: []const u8 = "";

    pub static fn init(total: u64, width: usize): ProgressBar {
        const now = monotonicNowNs();
        return ProgressBar {
            total: if (total == 0) 1 else total,
            current: 0,
            width: if (width == 0) 20 else width,
            start_ns: now,
            last_ns: now,
            label: "",
        };
    }

    pub static fn initWithTime(total: u64, width: usize, start_ns: u64, label: []const u8 = ""): ProgressBar {
        return ProgressBar {
            total: if (total == 0) 1 else total,
            current: 0,
            width: if (width == 0) 20 else width,
            start_ns: start_ns,
            last_ns: start_ns,
            label: label,
        };
    }

    pub fn tick(*_, delta: u64) {
        _.tickAt(delta, monotonicNowNs());
    }

    pub fn tickAt(*_, delta: u64, now_ns: u64) {
        if (_.current + delta >= _.total) {
            _.current = _.total;
        } else {
            _.current += delta;
        }
        if (now_ns >= _.last_ns) {
            _.last_ns = now_ns;
        }
    }

    pub fn set(*_, value: u64) {
        _.setAt(value, monotonicNowNs());
    }

    pub fn setAt(*_, value: u64, now_ns: u64) {
        _.current = if (value > _.total) _.total else value;
        if (now_ns >= _.last_ns) {
            _.last_ns = now_ns;
        }
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

    pub fn elapsedNs(): u64 {
        return if (_.last_ns >= _.start_ns) (_.last_ns - _.start_ns) else 0;
    }

    /// Computes throughput in items per second based on elapsed nanoseconds.
    pub fn itemsPerSec(): u64 {
        const el = _.elapsedNs();
        if (el == 0 || _.current == 0) {
            return 0;
        }
        const scaled = (_.current as u128) * 1000000000;
        return (scaled / (el as u128)) as u64;
    }

    /// Estimates remaining seconds (ETA) at current throughput.
    pub fn etaSeconds(): u64 {
        if (_.current >= _.total) {
            return 0;
        }
        const rate = _.itemsPerSec();
        if (rate == 0) {
            return 0;
        }
        const rem = _.total - _.current;
        return (rem + rate - 1) / rate;
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

    /// Renders bar + percentage + throughput + ETA into `out_buf`.
    pub fn renderLine(out_buf: []u8): []const u8 {
        const bar_len = _.width + 2;
        if (out_buf.len < bar_len) {
            return "";
        }
        let bar_tmp: [128]u8 = undefined;
        const bar = if (bar_len <= 128) _.renderBar(&bar_tmp) else "";
        const pct = _.percent();
        const rate = _.itemsPerSec();
        const eta = _.etaSeconds();
        if (_.label.len > 0) {
            return std.fmt.bufPrint(
                out_buf,
                "{s} {s} {d}% ({d}/{d}, {d}/s, ETA {d}s)",
                .{ _.label, bar, pct, _.current, _.total, rate, eta },
            ) catch "";
        }
        return std.fmt.bufPrint(
            out_buf,
            "{s} {d}% ({d}/{d}, {d}/s, ETA {d}s)",
            .{ bar, pct, _.current, _.total, rate, eta },
        ) catch "";
    }
}

pub const Progress = ProgressBar;
