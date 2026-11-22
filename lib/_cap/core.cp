// Source: Absorbed from /data/dev/bun/src/bun.cp | Synced: 2026-11-22T10:13:39Z
// ============================================================================
// lib/_cap/core.cp — Zero-Dependency Micro-Core for `import cap from "cap"`
// ============================================================================
// Only the ultra-lightweight core primitives live here so that importing
// `cap` has near-zero parse/transpile/compile cost. All 50+ subsystems
// (`cap.bits`, `cap.hash`, `cap.errno`, `cap.sys`, `cap.strings`, etc.) are
// lazily discovered and mounted by `cpc` at compile time only when referenced.
// ============================================================================

const std = @import("std");
const builtin = @import("builtin");

// ----------------------------------------------------------------------------
// 1. Memory & Out-of-Memory Handling
// ----------------------------------------------------------------------------

pub type OOM = std.mem.Allocator.Error;

pub const default_allocator: std.mem.Allocator = std.heap.page_allocator;
pub const page_allocator: std.mem.Allocator = std.heap.page_allocator;

pub const DefaultAllocator = struct {
    pub static fn allocator(): std.mem.Allocator {
        return default_allocator;
    }
};

pub fn outOfMemory(): noreturn {
    @branchHint(.cold);
    @panic("cap: out of memory");
}

fn isOomOnlyError(comptime ErrorUnionOrSet: type): bool {
    zig {
        @setEvalBranchQuota(10000);
        const info = @typeInfo(ErrorUnionOrSet);
        const ErrorSet = switch (info) {
            .error_union => |u| u.error_set,
            .error_set => ErrorUnionOrSet,
            else => return false,
        };
        const names = @typeInfo(ErrorSet).error_set.error_names orelse return false;
        if (names.len == 0) return false;
        for (names) |name| {
            if (!std.mem.eql(u8, name, "OutOfMemory")) return false;
        }
        return true;
    }
}

pub fn HandleOomReturn(comptime ArgType: type): type {
    zig {
        const arg_info = @typeInfo(ArgType);
        if (arg_info == .@"union" and @hasDecl(ArgType, "__IS_CP_RESULT")) {
            const ok_field = @typeInfo(ArgType).@"union".fields[0];
            return ok_field.type;
        }
        if (isOomOnlyError(ArgType)) {
            return switch (arg_info) {
                .error_union => |u| u.payload,
                .error_set => noreturn,
                else => unreachable,
            };
        }
        return ArgType;
    }
}

/// Unwrap an error union / error set / CpResult, panicking on OutOfMemory.
pub fn handleOom(error_union_or_set: auto): HandleOomReturn(@TypeOf(error_union_or_set)) {
    zig {
        const ArgType = @TypeOf(error_union_or_set);
        const info = @typeInfo(ArgType);
        if (info == .@"union" and @hasDecl(ArgType, "__IS_CP_RESULT")) {
            switch (error_union_or_set) {
                .ok => |v| return v,
                .err => outOfMemory(),
            }
        }
        const e = switch (info) {
            .error_union => error_union_or_set catch |err_val| err_val,
            .error_set => error_union_or_set,
            else => return error_union_or_set,
        };
        if (info == .error_union) {
            if (error_union_or_set) |val| {
                return val;
            } else |_| {}
        }
        if (comptime isOomOnlyError(ArgType)) {
            return outOfMemory();
        }
        if (e == error.OutOfMemory) {
            return outOfMemory();
        }
        return e;
    }
}

/// Allocate a single `T` with `allocator` and initialize it to `t`.
pub fn create(allocator: std.mem.Allocator, comptime T: type, t: T): *T {
    zig {
        const pointer = allocator.create(T) catch outOfMemory();
        pointer.* = t;
        return pointer;
    }
}

/// Destroy a pointer allocated from `default_allocator`.
@inline pub fn destroy(pointer: auto) {
    zig {
        default_allocator.destroy(pointer);
    }
}

/// Destroy a pointer allocated from a specific `allocator`.
@inline pub fn destroyWith(allocator: std.mem.Allocator, pointer: auto) {
    zig {
        allocator.destroy(pointer);
    }
}

fn TakeReturnType(comptime PtrType: type): type {
    zig {
        const child = @typeInfo(PtrType).pointer.child;
        return ?@typeInfo(child).optional.child;
    }
}

/// Take the value out of `*?T`, setting it to `null`.
@inline pub fn take(val: auto): TakeReturnType(@TypeOf(val)) {
    zig {
        if (val.*) |v| {
            val.* = null;
            return v;
        }
        return null;
    }
}

/// Copy elements from `src` into `dest` using `@memcpy`/`memmove` byte semantics.
pub fn copy(comptime T: type, dest: []T, src: []const T) {
    zig {
        const input: []const u8 = std.mem.sliceAsBytes(src);
        const output: []u8 = std.mem.sliceAsBytes(dest);
        if (input.len == 0) return;
        if (@intFromPtr(output.ptr) < @intFromPtr(input.ptr)) {
            std.mem.copyForwards(u8, output[0..input.len], input);
        } else {
            std.mem.copyBackwards(u8, output[0..input.len], input);
        }
    }
}

/// Zero-initialize every byte of `Type` (ignoring default struct values).
pub fn zero(comptime Type: type): Type {
    return std.mem.zeroes(Type);
}

// ----------------------------------------------------------------------------
// 2. Environment & Platform Compile-Time Constants
// ----------------------------------------------------------------------------

pub struct Environment {
    pub const isLinux: bool = builtin.target.os.tag == .linux;
    pub const isMac: bool = builtin.target.os.tag == .macos;
    pub const isDarwin: bool = builtin.target.os.tag == .macos;
    pub const isWindows: bool = builtin.target.os.tag == .windows;
    pub const isFreeBSD: bool = builtin.target.os.tag == .freebsd;
    pub const isWasm: bool = builtin.cpu.arch.isWasm();
    pub const isPosix: bool = !isWindows && !isWasm;

    pub const isDebug: bool = builtin.mode == .debug;
    pub const isRelease: bool = builtin.mode != .debug;
    pub const isTest: bool = builtin.is_test;
    pub const allow_assert: bool = isDebug || isTest || (builtin.mode == .safe);

    pub const isX64: bool = builtin.cpu.arch == .x86_64;
    pub const isX86: bool = builtin.cpu.arch.isX86();
    pub const isAarch64: bool = builtin.cpu.arch.isAARCH64();
    pub const is64Bit: bool = @sizeOf(usize) == 8;

    pub const os_tag = builtin.target.os.tag;
    pub const arch = builtin.cpu.arch;
    pub const mode = builtin.mode;
}

// ----------------------------------------------------------------------------
// 3. Assertions & Global Process Control
// ----------------------------------------------------------------------------

@inline pub fn debugAssert(ok: bool) {
    if (comptime !Environment.isDebug) {
        return;
    }
    if (!ok) {
        unreachable;
    }
}

@inline pub fn assert(ok: bool) {
    if (comptime !Environment.allow_assert) {
        return;
    }
    if (!ok) {
        if (comptime Environment.isDebug) {
            unreachable;
        }
        @panic("assertion failure");
    }
}

@inline pub fn assertWithLocation(ok: bool, src: std.builtin.SourceLocation) {
    if (comptime !Environment.allow_assert) {
        return;
    }
    if (!ok) {
        if (comptime Environment.isDebug) {
            unreachable;
        }
        std.debug.panic("assertion failure at {s}:{d}:{d}", .{ src.file, src.line, src.column });
    }
}

pub struct Global {
    pub static fn exit(code: u32): noreturn {
        zig {
            std.process.exit(@truncate(code));
        }
    }

    pub static fn panic(comptime fmt: []const u8, args: auto): noreturn {
        @branchHint(.cold);
        zig {
            std.debug.panic(fmt, args);
        }
    }
}

// ----------------------------------------------------------------------------
// 4. Numeric & Slice Primitives
// ----------------------------------------------------------------------------

@inline pub fn clamp(value: auto, min: @TypeOf(value), max: @TypeOf(value)): @TypeOf(value) {
    debugAssert(min <= max);
    zig {
        const T = @TypeOf(value);
        if (comptime (T == f32 or T == f64)) {
            var v = value;
            if (v < min) v = min;
            if (v > max) v = max;
            return v;
        }
        return std.math.clamp(value, min, max);
    }
}

@inline pub fn cast(comptime To: type, value: auto): To {
    zig {
        if (@typeInfo(@TypeOf(value)) == .int or @typeInfo(@TypeOf(value)) == .comptime_int) {
            return @ptrFromInt(@as(usize, value));
        }
        return @ptrCast(@alignCast(value));
    }
}

fn RangeArray(comptime min: usize, comptime max: usize): type {
    const n = max - min;
    return [n]usize;
}

@inline pub fn range(comptime min: usize, comptime max: usize): RangeArray(min, max) {
    zig {
        return comptime brk: {
            const n = max - min;
            var arr: [n]usize = undefined;
            var i: usize = min;
            while (i < max) : (i += 1) {
                arr[i - min] = i;
            }
            break :brk arr;
        };
    }
}

pub fn asByteSlice(buffer: auto): []const u8 {
    zig {
        const T = @TypeOf(buffer);
        return switch (T) {
            [*:0]u8, [*:0]const u8 => std.mem.span(buffer),
            [*c]const u8, [*c]u8 => std.mem.span(buffer),
            else => buffer,
        };
    }
}

pub fn constStrToU8(s: auto): []const u8 {
    return asByteSlice(s);
}

pub const sliceTo = std.mem.sliceTo;

// ----------------------------------------------------------------------------
// 5. Low-Level Path & File Descriptor Primitives
// ----------------------------------------------------------------------------

pub type FD = i32;
pub const invalid_fd: FD = -1;
pub type Mode = u32;
pub const MAX_PATH_BYTES: usize = 4096;
pub type PathBuffer = [4096]u8;

/// File open flags (Linux / POSIX compatible constants, Zig 0.17 std.os.linux.O backed).
pub struct O {
    pub const RDONLY: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .ACCMODE = .RDONLY }) else 0;
    pub const WRONLY: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .ACCMODE = .WRONLY }) else 1;
    pub const RDWR: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .ACCMODE = .RDWR }) else 2;
    pub const CREAT: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .CREAT = true }) else 0o100;
    pub const EXCL: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .EXCL = true }) else 0o200;
    pub const TRUNC: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .TRUNC = true }) else 0o1000;
    pub const APPEND: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .APPEND = true }) else 0o2000;
    pub const NONBLOCK: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .NONBLOCK = true }) else 0o4000;
    pub const DIRECTORY: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .DIRECTORY = true }) else 0o200000;
    pub const CLOEXEC: u32 = if (Environment.isLinux) @bitCast(std.os.linux.O{ .CLOEXEC = true }) else 0o2000000;
}

/// POSIX file mode & permission bit constants.
pub struct S {
    pub const IFMT: u32 = 0o170000;
    pub const IFIFO: u32 = 0o010000;
    pub const IFCHR: u32 = 0o020000;
    pub const IFDIR: u32 = 0o040000;
    pub const IFBLK: u32 = 0o060000;
    pub const IFREG: u32 = 0o100000;
    pub const IFLNK: u32 = 0o120000;
    pub const IFSOCK: u32 = 0o140000;

    pub const IRWXU: u32 = 0o0700;
    pub const IRUSR: u32 = 0o0400;
    pub const IWUSR: u32 = 0o0200;
    pub const IXUSR: u32 = 0o0100;

    pub const IRWXG: u32 = 0o0070;
    pub const IRGRP: u32 = 0o0040;
    pub const IWGRP: u32 = 0o0020;
    pub const IXGRP: u32 = 0o0010;

    pub const IRWXO: u32 = 0o0007;
    pub const IROTH: u32 = 0o0004;
    pub const IWOTH: u32 = 0o0002;
    pub const IXOTH: u32 = 0o0001;
}

// ----------------------------------------------------------------------------
// 6. Lightweight Output, ComptimeStringMap & Formatting
// ----------------------------------------------------------------------------

pub struct Output {
    pub static fn print(comptime fmt: []const u8, args: auto) {
        zig {
            std.debug.print(fmt, args);
        }
    }

    pub static fn println(comptime fmt: []const u8, args: auto) {
        zig {
            std.debug.print(fmt ++ "\n", args);
        }
    }

    pub static fn errorln(comptime fmt: []const u8, args: auto) {
        zig {
            std.debug.print("error: " ++ fmt ++ "\n", args);
        }
    }

    pub static fn debug(comptime fmt: []const u8, args: auto) {
        if (comptime Environment.isDebug) {
            zig {
                std.debug.print("[debug] " ++ fmt ++ "\n", args);
            }
        }
    }

    pub static fn flush() {
        // std.debug.print writes unbuffered to stderr; kept as a zero-cost hook.
    }
}

/// Compile-time string lookup map optimized by key length partitioning.
pub fn ComptimeStringMap(comptime V: type, comptime kvs_list: auto): type {
    zig {
        const KV = struct {
            key: []const u8,
            value: V,
        };
        const len_table = comptime blk: {
            @setEvalBranchQuota(100000);
            var arr: [kvs_list.len]KV = undefined;
            for (kvs_list, 0..) |kv, idx| {
                arr[idx] = .{
                    .key = @field(kv, "0"),
                    .value = if (V != void) @field(kv, "1") else {},
                };
            }
            break :blk arr;
        };
        return struct {
            pub const kvs = len_table;
            pub const Value = V;

            pub fn get(str: []const u8) ?V {
                inline for (kvs) |entry| {
                    if (entry.key.len == str.len and std.mem.eql(u8, entry.key, str)) {
                        return entry.value;
                    }
                }
                return null;
            }

            pub fn has(str: []const u8) bool {
                return get(str) != null;
            }
        };
    }
}

/// Format an integer byte count into a human-readable IEC/decimal size string.
pub fn fmtIntSize(bytes: u64, buf: []u8): []const u8 {
    zig {
        const units = [_][]const u8{ "B", "KB", "MB", "GB", "TB", "PB" };
        var val: f64 = @floatFromInt(bytes);
        var idx: usize = 0;
        while (val >= 1024.0 and idx + 1 < units.len) {
            val /= 1024.0;
            idx += 1;
        }
        if (idx == 0) {
            return std.fmt.bufPrint(buf, "{d} B", .{bytes}) catch buf[0..0];
        }
        return std.fmt.bufPrint(buf, "{d:.2} {s}", .{ val, units[idx] }) catch buf[0..0];
    }
}
