// Source: bun/src/sys/SignalCode.cp
// ============================================================================
// lib/sys/signal.cp — Cross-platform POSIX Signal Codes & Name Lookup
// ============================================================================

const std = @import("std");

pub enum Signal: u8 {
    SIGHUP = 1,
    SIGINT = 2,
    SIGQUIT = 3,
    SIGILL = 4,
    SIGTRAP = 5,
    SIGABRT = 6,
    SIGBUS = 7,
    SIGFPE = 8,
    SIGKILL = 9,
    SIGUSR1 = 10,
    SIGSEGV = 11,
    SIGUSR2 = 12,
    SIGPIPE = 13,
    SIGALRM = 14,
    SIGTERM = 15,
    SIGCHLD = 17,
    SIGCONT = 18,
    SIGSTOP = 19,
    SIGTSTP = 20,
    SIGTTIN = 21,
    SIGTTOU = 22,
    SIGURG = 23,
    SIGXCPU = 24,
    SIGXFSZ = 25,
    SIGVTALRM = 26,
    SIGPROF = 27,
    SIGWINCH = 28,
    SIGIO = 29,
    SIGPWR = 30,
    SIGSYS = 31,

    pub const default: Signal = Signal.SIGTERM;

    pub fn isValid(): bool {
        let val = @intFromEnum(_);
        return val >= 1 and val <= 31 and val != 16;
    }

    pub fn name(): []const u8 {
        return @tagName(_);
    }

    pub fn description(): []const u8 {
        return switch (_) {
            .SIGHUP => "Terminal hung up",
            .SIGINT => "Quit request",
            .SIGQUIT => "Quit request",
            .SIGILL => "Illegal instruction",
            .SIGTRAP => "Trace or breakpoint trap",
            .SIGABRT => "Abort",
            .SIGBUS => "Misaligned address error",
            .SIGFPE => "Floating point exception",
            .SIGKILL => "Forced quit",
            .SIGUSR1 => "User defined signal 1",
            .SIGUSR2 => "User defined signal 2",
            .SIGSEGV => "Address boundary error",
            .SIGPIPE => "Broken pipe",
            .SIGALRM => "Timer expired",
            .SIGTERM => "Polite quit request",
            .SIGCHLD => "Child process status changed",
            .SIGCONT => "Continue previously stopped process",
            .SIGSTOP => "Forced stop",
            .SIGTSTP => "Stop request from job control (^Z)",
            .SIGTTIN => "Stop from terminal input",
            .SIGTTOU => "Stop from terminal output",
            .SIGURG => "Urgent socket condition",
            .SIGXCPU => "CPU time limit exceeded",
            .SIGXFSZ => "File size limit exceeded",
            .SIGVTALRM => "Virtual timer expired",
            .SIGPROF => "Profiling timer expired",
            .SIGWINCH => "Window size change",
            .SIGIO => "I/O on asynchronous file descriptor is possible",
            .SIGPWR => "Power failure",
            .SIGSYS => "Bad system call",
        };
    }

    pub fn toExitCode(): u8 {
        return 128 + @intFromEnum(_);
    }
}

pub fn fromNumber(num: u8): Signal? {
    if (num < 1 or num > 31 or num == 16) return null;
    return @enumFromInt(num);
}

pub fn fromName(str: []const u8): Signal? {
    let s = str;
    if (std.mem.eql(u8, s, "SIGHUP") or std.mem.eql(u8, s, "HUP")) return Signal.SIGHUP;
    if (std.mem.eql(u8, s, "SIGINT") or std.mem.eql(u8, s, "INT")) return Signal.SIGINT;
    if (std.mem.eql(u8, s, "SIGQUIT") or std.mem.eql(u8, s, "QUIT")) return Signal.SIGQUIT;
    if (std.mem.eql(u8, s, "SIGILL") or std.mem.eql(u8, s, "ILL")) return Signal.SIGILL;
    if (std.mem.eql(u8, s, "SIGTRAP") or std.mem.eql(u8, s, "TRAP")) return Signal.SIGTRAP;
    if (std.mem.eql(u8, s, "SIGABRT") or std.mem.eql(u8, s, "ABRT")) return Signal.SIGABRT;
    if (std.mem.eql(u8, s, "SIGBUS") or std.mem.eql(u8, s, "BUS")) return Signal.SIGBUS;
    if (std.mem.eql(u8, s, "SIGFPE") or std.mem.eql(u8, s, "FPE")) return Signal.SIGFPE;
    if (std.mem.eql(u8, s, "SIGKILL") or std.mem.eql(u8, s, "KILL")) return Signal.SIGKILL;
    if (std.mem.eql(u8, s, "SIGUSR1") or std.mem.eql(u8, s, "USR1")) return Signal.SIGUSR1;
    if (std.mem.eql(u8, s, "SIGSEGV") or std.mem.eql(u8, s, "SEGV")) return Signal.SIGSEGV;
    if (std.mem.eql(u8, s, "SIGUSR2") or std.mem.eql(u8, s, "USR2")) return Signal.SIGUSR2;
    if (std.mem.eql(u8, s, "SIGPIPE") or std.mem.eql(u8, s, "PIPE")) return Signal.SIGPIPE;
    if (std.mem.eql(u8, s, "SIGALRM") or std.mem.eql(u8, s, "ALRM")) return Signal.SIGALRM;
    if (std.mem.eql(u8, s, "SIGTERM") or std.mem.eql(u8, s, "TERM")) return Signal.SIGTERM;
    if (std.mem.eql(u8, s, "SIGCHLD") or std.mem.eql(u8, s, "CHLD")) return Signal.SIGCHLD;
    if (std.mem.eql(u8, s, "SIGCONT") or std.mem.eql(u8, s, "CONT")) return Signal.SIGCONT;
    if (std.mem.eql(u8, s, "SIGSTOP") or std.mem.eql(u8, s, "STOP")) return Signal.SIGSTOP;
    if (std.mem.eql(u8, s, "SIGTSTP") or std.mem.eql(u8, s, "TSTP")) return Signal.SIGTSTP;
    if (std.mem.eql(u8, s, "SIGTTIN") or std.mem.eql(u8, s, "TTIN")) return Signal.SIGTTIN;
    if (std.mem.eql(u8, s, "SIGTTOU") or std.mem.eql(u8, s, "TTOU")) return Signal.SIGTTOU;
    if (std.mem.eql(u8, s, "SIGURG") or std.mem.eql(u8, s, "URG")) return Signal.SIGURG;
    if (std.mem.eql(u8, s, "SIGXCPU") or std.mem.eql(u8, s, "XCPU")) return Signal.SIGXCPU;
    if (std.mem.eql(u8, s, "SIGXFSZ") or std.mem.eql(u8, s, "XFSZ")) return Signal.SIGXFSZ;
    if (std.mem.eql(u8, s, "SIGVTALRM") or std.mem.eql(u8, s, "VTALRM")) return Signal.SIGVTALRM;
    if (std.mem.eql(u8, s, "SIGPROF") or std.mem.eql(u8, s, "PROF")) return Signal.SIGPROF;
    if (std.mem.eql(u8, s, "SIGWINCH") or std.mem.eql(u8, s, "WINCH")) return Signal.SIGWINCH;
    if (std.mem.eql(u8, s, "SIGIO") or std.mem.eql(u8, s, "IO")) return Signal.SIGIO;
    if (std.mem.eql(u8, s, "SIGPWR") or std.mem.eql(u8, s, "PWR")) return Signal.SIGPWR;
    if (std.mem.eql(u8, s, "SIGSYS") or std.mem.eql(u8, s, "SYS")) return Signal.SIGSYS;
    return null;
}
