// Source: Absorbed from /data/dev/bun/src/crash_handler/crash_handler.zig | Synced: 2026-11-22T10:58:52Z
// Also absorbs: /data/dev/bun/src/crash_handler/handle_oom.zig
// lib/safety/crash.cp
// OOM Policy Guard & Bun-Style Signal Crash Diagnostic Report Formatter

const std = @import("std");
import { CpuFeatures } from "./cpu_features.cp";

pub enum OomPolicy: u8 {
    Abort = 0,
    Panic = 1,
    SoftFail = 2,
    Callback = 3,
}

pub type OomHookFn = fn (requested_bytes: usize, context: []const u8) void;

pub struct OomGuard {
    policy: OomPolicy = OomPolicy.SoftFail,
    oom_count: u64 = 0,
    last_requested_bytes: usize = 0,
    max_single_alloc: usize = 0,
    hook: OomHookFn? = null,

    pub static fn init(policy: OomPolicy = .SoftFail): OomGuard {
        return OomGuard {
            policy: policy,
            oom_count: 0,
            last_requested_bytes: 0,
            max_single_alloc: 0,
            hook: null,
        };
    }

    pub fn setHook(*_, hook: OomHookFn?) {
        _.hook = hook;
    }

    pub fn setMaxSingleAlloc(*_, limit: usize) {
        _.max_single_alloc = limit;
    }

    pub fn count(): u64 {
        return _.oom_count;
    }

    pub fn lastRequestedBytes(): usize {
        return _.last_requested_bytes;
    }

    pub fn recordOom(*_, requested_bytes: usize, context: []const u8 = "") {
        _.oom_count += 1;
        _.last_requested_bytes = requested_bytes;
        if (cb := _.hook) {
            cb(requested_bytes, context);
        }
        switch (_.policy) {
            OomPolicy.Abort => {
                std.c.abort();
            },
            OomPolicy.Panic => {
                @panic("OomGuard: out of memory");
            },
            OomPolicy.SoftFail | OomPolicy.Callback => {},
        }
    }

    /// Attempts a guarded byte-slice allocation, enforcing `max_single_alloc` and recording
    /// any `OutOfMemory` failure under the active `OomPolicy`.
    pub fn checkAlloc(*_, allocator: std.mem.Allocator, len: usize, context: []const u8 = ""): ([]u8)? {
        if (_.max_single_alloc > 0 && len > _.max_single_alloc) {
            _.recordOom(len, context);
            return null;
        }
        const mem = allocator.alloc(u8, len) catch {
            _.recordOom(len, context);
            return null;
        };
        return mem;
    }

    pub fn reset(*_) {
        _.oom_count = 0;
        _.last_requested_bytes = 0;
    }
}

pub struct CrashReport {
    signal_num: i32 = 0,
    signal_name: []const u8 = "UNKNOWN",
    pid: i32 = 0,
    fault_addr: usize = 0,
    message: []const u8 = "",
    action_context: []const u8 = "",
    cpu_summary: [256]u8 = [0; 256],
    cpu_summary_len: usize = 0,
    oom_count: u64 = 0,

    pub static fn signalName(sig: i32): []const u8 {
        if (comptime @import("builtin").os.tag == .linux) {
            if (sig == @intFromEnum(std.os.linux.SIG.SEGV)) return "SIGSEGV";
            if (sig == @intFromEnum(std.os.linux.SIG.ABRT)) return "SIGABRT";
            if (sig == @intFromEnum(std.os.linux.SIG.BUS)) return "SIGBUS";
            if (sig == @intFromEnum(std.os.linux.SIG.FPE)) return "SIGFPE";
            if (sig == @intFromEnum(std.os.linux.SIG.ILL)) return "SIGILL";
            if (sig == @intFromEnum(std.os.linux.SIG.TERM)) return "SIGTERM";
            if (sig == @intFromEnum(std.os.linux.SIG.KILL)) return "SIGKILL";
            if (sig == @intFromEnum(std.os.linux.SIG.HUP)) return "SIGHUP";
            if (sig == @intFromEnum(std.os.linux.SIG.INT)) return "SIGINT";
            if (sig == @intFromEnum(std.os.linux.SIG.TRAP)) return "SIGTRAP";
            if (sig == @intFromEnum(std.os.linux.SIG.PIPE)) return "SIGPIPE";
        }
        return switch (sig) {
            1 => "SIGHUP",
            2 => "SIGINT",
            4 => "SIGILL",
            5 => "SIGTRAP",
            6 => "SIGABRT",
            7 => "SIGBUS",
            8 => "SIGFPE",
            9 => "SIGKILL",
            11 => "SIGSEGV",
            13 => "SIGPIPE",
            15 => "SIGTERM",
            else => "SIGUNKNOWN",
        };
    }

    /// Captures process PID, signal metadata, CPU ISA feature summary, and custom crash context.
    pub static fn capture(
        sig: i32,
        message: []const u8 = "",
        action_context: []const u8 = "",
        fault_addr: usize = 0,
        oom_count: u64 = 0,
    ): CrashReport {
        let rep = CrashReport {
            signal_num: sig,
            signal_name: CrashReport.signalName(sig),
            pid: @intCast(std.c.getpid()),
            fault_addr: fault_addr,
            message: message,
            action_context: action_context,
            cpu_summary: [0; 256],
            cpu_summary_len: 0,
            oom_count: oom_count,
        };
        const features = CpuFeatures.detect();
        const summary = features.formatSummary(&rep.cpu_summary);
        rep.cpu_summary_len = summary.len;
        return rep;
    }

    pub fn cpuSummary(): []const u8 {
        return _.cpu_summary[0.._.cpu_summary_len];
    }

    /// Formats a Bun-style crash diagnostic report into a stack/caller buffer (zero heap allocation).
    pub fn formatReport(buf: []u8): []const u8 {
        const cpu = _.cpu_summary[0.._.cpu_summary_len];
        const reason = if (_.message.len > 0) _.message else "fatal signal received";
        const ctx = if (_.action_context.len > 0) _.action_context else "none";
        const res = std.fmt.bufPrint(
            buf,
            "============================================================\n" ++
                "CP Runtime Crash Report\n" ++
                "PID: {d} | OS: {s} | Arch: {s}\n" ++
                "Signal: {s} ({d}) at address 0x{x}\n" ++
                "Reason: {s}\n" ++
                "Context: {s}\n" ++
                "CPU: {s}\n" ++
                "OOM Count: {d}\n" ++
                "============================================================\n",
            .{
                _.pid,
                @tagName(@import("builtin").os.tag),
                @tagName(@import("builtin").cpu.arch),
                _.signal_name,
                _.signal_num,
                _.fault_addr,
                reason,
                ctx,
                cpu,
                _.oom_count,
            },
        ) catch return buf[0..0];
        return res;
    }

    /// Allocates and returns a formatted Bun-style crash diagnostic report.
    pub fn formatReportAlloc(allocator: std.mem.Allocator): []u8! {
        let tmp: [2048]u8 = undefined;
        const formatted = _.formatReport(&tmp);
        const out = try allocator.alloc(u8, formatted.len);
        @memcpy(out, formatted);
        return out;
    }
}
