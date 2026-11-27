// Source: bun/src/crash_handler/crash_handler.cp -> cp/lib/crash_handler/types.cp

const std = @import("std");

pub enum SignalKind {
    Segv,
    Abrt,
    Bus,
    Ill,
    Fpe,
    Trap,
    Panic,
    Unknown,
}

pub fn signalName(sig: SignalKind): []const u8 {
    switch (sig) {
        .Segv => return "SIGSEGV",
        .Abrt => return "SIGABRT",
        .Bus => return "SIGBUS",
        .Ill => return "SIGILL",
        .Fpe => return "SIGFPE",
        .Trap => return "SIGTRAP",
        .Panic => return "PANIC",
        .Unknown => return "UNKNOWN",
    }
}

pub fn signalDescription(sig: SignalKind): []const u8 {
    switch (sig) {
        .Segv => return "Segmentation fault (invalid memory reference)",
        .Abrt => return "Process aborted",
        .Bus => return "Bus error (bad memory access)",
        .Ill => return "Illegal instruction",
        .Fpe => return "Floating-point exception",
        .Trap => return "Trace/breakpoint trap",
        .Panic => return "Runtime panic",
        .Unknown => return "Unknown signal",
    }
}

pub fn signalFromCode(code: i32): SignalKind {
    switch (code) {
        4 => return SignalKind.Ill,
        5 => return SignalKind.Trap,
        6 => return SignalKind.Abrt,
        7 => return SignalKind.Bus,
        8 => return SignalKind.Fpe,
        11 => return SignalKind.Segv,
        else => return SignalKind.Unknown,
    }
}

pub enum CrashReason {
    Panic: []const u8,
    Unreachable,
    SegmentationFault: usize,
    IllegalInstruction: usize,
    BusError: usize,
    FloatingPointError: usize,
    Trap: usize,
    Abort,
    OutOfMemory,
    AssertionFailed: []const u8,
    ErrorCode: i32,
}

pub fn reasonName(reason: CrashReason): []const u8 {
    switch (reason) {
        .Panic _ => return "panic",
        .Unreachable => return "unreachable",
        .SegmentationFault _ => return "segmentation_fault",
        .IllegalInstruction _ => return "illegal_instruction",
        .BusError _ => return "bus_error",
        .FloatingPointError _ => return "floating_point_error",
        .Trap _ => return "trap",
        .Abort => return "abort",
        .OutOfMemory => return "out_of_memory",
        .AssertionFailed _ => return "assertion_failed",
        .ErrorCode _ => return "error_code",
    }
}

pub fn reasonSignal(reason: CrashReason): SignalKind {
    switch (reason) {
        .Panic _ => return SignalKind.Panic,
        .Unreachable => return SignalKind.Panic,
        .SegmentationFault _ => return SignalKind.Segv,
        .IllegalInstruction _ => return SignalKind.Ill,
        .BusError _ => return SignalKind.Bus,
        .FloatingPointError _ => return SignalKind.Fpe,
        .Trap _ => return SignalKind.Trap,
        .Abort => return SignalKind.Abrt,
        .OutOfMemory => return SignalKind.Abrt,
        .AssertionFailed _ => return SignalKind.Panic,
        .ErrorCode code => return signalFromCode(code),
    }
}

pub fn reasonAddress(reason: CrashReason): usize? {
    switch (reason) {
        .SegmentationFault addr => return addr,
        .IllegalInstruction addr => return addr,
        .BusError addr => return addr,
        .FloatingPointError addr => return addr,
        .Trap addr => return addr,
        else => return null,
    }
}

pub struct StackFrame {
    addr: usize,
    symbol: []const u8,
    file: []const u8,
    line: u32,
    col: u32,
    offset: usize,
}

pub struct Breadcrumb {
    timestamp_ms: u64,
    category: []const u8,
    message: []const u8,
    level: u8,
}

pub struct MetadataEntry {
    key: []const u8,
    value: []const u8,
}

pub struct CrashReport {
    reason: CrashReason,
    signal: SignalKind,
    fault_addr: usize,
    thread_id: u64,
    timestamp_ms: u64,
    frames: []const StackFrame,
    breadcrumbs: []const Breadcrumb,
    metadata: []const MetadataEntry,
    owned: bool,

    pub static fn init(
        reason: CrashReason,
        fault_addr: usize,
        thread_id: u64,
        timestamp_ms: u64,
        frames: []const StackFrame,
        breadcrumbs: []const Breadcrumb,
        metadata: []const MetadataEntry,
    ): CrashReport {
        return CrashReport{
            .reason = reason,
            .signal = reasonSignal(reason),
            .fault_addr = fault_addr,
            .thread_id = thread_id,
            .timestamp_ms = timestamp_ms,
            .frames = frames,
            .breadcrumbs = breadcrumbs,
            .metadata = metadata,
            .owned = false,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.owned) {
            if (_.frames.len > 0) {
                alloc.free(_.frames);
            }
            if (_.breadcrumbs.len > 0) {
                alloc.free(_.breadcrumbs);
            }
            if (_.metadata.len > 0) {
                alloc.free(_.metadata);
            }
            _.frames = [];
            _.breadcrumbs = [];
            _.metadata = [];
            _.owned = false;
        }
    }
}

pub struct CrashReportBuilder {
    reason: CrashReason,
    signal: SignalKind,
    fault_addr: usize,
    thread_id: u64,
    timestamp_ms: u64,
    frames: std.ArrayList(StackFrame),
    breadcrumbs: std.ArrayList(Breadcrumb),
    metadata: std.ArrayList(MetadataEntry),

    pub static fn init(reason: CrashReason, fault_addr: usize, thread_id: u64, timestamp_ms: u64): CrashReportBuilder {
        return CrashReportBuilder{
            .reason = reason,
            .signal = reasonSignal(reason),
            .fault_addr = fault_addr,
            .thread_id = thread_id,
            .timestamp_ms = timestamp_ms,
            .frames = std.ArrayList(StackFrame).empty,
            .breadcrumbs = std.ArrayList(Breadcrumb).empty,
            .metadata = std.ArrayList(MetadataEntry).empty,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.frames.deinit(alloc);
        _.breadcrumbs.deinit(alloc);
        _.metadata.deinit(alloc);
    }

    pub fn addFrame(*_, alloc: std.mem.Allocator, frame: StackFrame): ! {
        _.frames.append(alloc, frame)?;
    }

    pub fn addBreadcrumb(*_, alloc: std.mem.Allocator, crumb: Breadcrumb): ! {
        _.breadcrumbs.append(alloc, crumb)?;
    }

    pub fn addMetadata(*_, alloc: std.mem.Allocator, key: []const u8, value: []const u8): ! {
        _.metadata.append(alloc, MetadataEntry{ .key = key, .value = value })?;
    }

    pub fn build(*_, alloc: std.mem.Allocator): !CrashReport {
        let frames_slice = _.frames.toOwnedSlice(alloc)?;
        errdefer alloc.free(frames_slice);
        let crumbs_slice = _.breadcrumbs.toOwnedSlice(alloc)?;
        errdefer alloc.free(crumbs_slice);
        let meta_slice = _.metadata.toOwnedSlice(alloc)?;
        return CrashReport{
            .reason = _.reason,
            .signal = _.signal,
            .fault_addr = _.fault_addr,
            .thread_id = _.thread_id,
            .timestamp_ms = _.timestamp_ms,
            .frames = frames_slice,
            .breadcrumbs = crumbs_slice,
            .metadata = meta_slice,
            .owned = true,
        };
    }
}
