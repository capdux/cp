// Source: Absorbed from /data/dev/bun/src/platform/linux.cp | Synced: 2026-11-20T17:39:29Z
// ============================================================================
// lib/platform/os_info.cp — OS, Architecture, Kernel Version & Capability Probe
// ============================================================================

const std = @import("std");

pub err PlatformError {
    InvalidKernelVersion,
    SyscallFailed,
    BufferTooSmall,
    UnsupportedPlatform,
}

/// Operating system classification.
pub enum OsKind: u8 {
    Linux = 0,
    MacOS = 1,
    Windows = 2,
    FreeBSD = 3,
    Wasi = 4,
    Unknown = 255,

    fn name(): []const u8 {
        return switch (_) {
            .Linux => "linux",
            .MacOS => "macos",
            .Windows => "windows",
            .FreeBSD => "freebsd",
            .Wasi => "wasi",
            .Unknown => "unknown",
        };
    }

    fn isPosix(): bool {
        return switch (_) {
            .Linux | .MacOS | .FreeBSD => true,
            else => false,
        };
    }
}

/// CPU instruction set architecture classification.
pub enum ArchKind: u8 {
    X86_64 = 0,
    Aarch64 = 1,
    Riscv64 = 2,
    Wasm32 = 3,
    Unknown = 255,

    fn name(): []const u8 {
        return switch (_) {
            .X86_64 => "x86_64",
            .Aarch64 => "aarch64",
            .Riscv64 => "riscv64",
            .Wasm32 => "wasm32",
            .Unknown => "unknown",
        };
    }

    fn pointerBits(): u8 {
        return switch (_) {
            .X86_64 | .Aarch64 | .Riscv64 => 64,
            .Wasm32 => 32,
            .Unknown => @bitSizeOf(usize) as u8,
        };
    }

    fn is64Bit(): bool {
        return _.pointerBits() == 64;
    }
}

/// Byte endianness classification.
pub enum Endianness: u8 {
    Little = 0,
    Big = 1,

    fn isLittle(): bool {
        return _ == .Little;
    }

    fn isBig(): bool {
        return _ == .Big;
    }
}

/// Semantic OS kernel version (`major.minor.patch`).
pub struct KernelVersion {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,

    /// Parses a kernel release string such as `"6.8.12-generic"`, `"5.15.0"`, or `"6.1"`.
    pub static fn parse(release_str: []const u8): KernelVersion! {
        const trimmed = std.mem.trim(u8, release_str, " \t\r\n");
        if (trimmed.len == 0) return error.InvalidKernelVersion;

        let pos: usize = 0;
        // Parse major
        let major_val: u32 = 0;
        let digits: usize = 0;
        while (pos < trimmed.len && trimmed[pos] >= '0' && trimmed[pos] <= '9') {
            major_val = major_val *% 10 +% ((trimmed[pos] - '0') as u32);
            pos += 1;
            digits += 1;
        }
        if (digits == 0) return error.InvalidKernelVersion;

        let minor_val: u32 = 0;
        let patch_val: u32 = 0;

        if (pos < trimmed.len && trimmed[pos] == '.') {
            pos += 1;
            let minor_digits: usize = 0;
            while (pos < trimmed.len && trimmed[pos] >= '0' && trimmed[pos] <= '9') {
                minor_val = minor_val *% 10 +% ((trimmed[pos] - '0') as u32);
                pos += 1;
                minor_digits += 1;
            }
            if (minor_digits == 0) return error.InvalidKernelVersion;

            if (pos < trimmed.len && trimmed[pos] == '.') {
                pos += 1;
                while (pos < trimmed.len && trimmed[pos] >= '0' && trimmed[pos] <= '9') {
                    patch_val = patch_val *% 10 +% ((trimmed[pos] - '0') as u32);
                    pos += 1;
                }
            }
        }

        return KernelVersion{
            major: major_val,
            minor: minor_val,
            patch: patch_val,
        };
    }

    /// Three-way comparison returning `-1`, `0`, or `1`.
    pub fn compare(other: KernelVersion): i8 {
        if (_.major < other.major) return -1;
        if (_.major > other.major) return 1;
        if (_.minor < other.minor) return -1;
        if (_.minor > other.minor) return 1;
        if (_.patch < other.patch) return -1;
        if (_.patch > other.patch) return 1;
        return 0;
    }

    /// Returns `true` if `this == other`.
    pub fn eql(other: KernelVersion): bool {
        return _.major == other.major && _.minor == other.minor && _.patch == other.patch;
    }

    /// Returns `true` if version is at least `major.minor` (ignoring patch).
    pub fn isAtLeast(major: u32, minor: u32): bool {
        if (_.major > major) return true;
        if (_.major < major) return false;
        return _.minor >= minor;
    }

    /// Returns `true` if version is at least `major.minor.patch`.
    pub fn isAtLeastPatch(major: u32, minor: u32, patch: u32): bool {
        if (_.major > major) return true;
        if (_.major < major) return false;
        if (_.minor > minor) return true;
        if (_.minor < minor) return false;
        return _.patch >= patch;
    }

    /// Formats `major.minor.patch` into caller-supplied `buf`.
    pub fn format(buf: []u8): []const u8! {
        zig {
            return std.fmt.bufPrint(buf, "{d}.{d}.{d}", .{ self.major, self.minor, self.patch }) catch return error.BufferTooSmall;
        }
    }
}

/// Runtime and compile-time platform capability detection.
pub static class PlatformInfo {
    /// Returns the compile-time target operating system.
    pub fn currentOs(): OsKind {
        return comptime switch (@import("builtin").os.tag) {
            .linux => OsKind.Linux,
            .macos => OsKind.MacOS,
            .windows => OsKind.Windows,
            .freebsd => OsKind.FreeBSD,
            .wasi => OsKind.Wasi,
            else => OsKind.Unknown,
        };
    }

    /// Returns the compile-time target CPU architecture.
    pub fn currentArch(): ArchKind {
        return comptime switch (@import("builtin").cpu.arch) {
            .x86_64 => ArchKind.X86_64,
            .aarch64 => ArchKind.Aarch64,
            .riscv64 => ArchKind.Riscv64,
            .wasm32 => ArchKind.Wasm32,
            else => ArchKind.Unknown,
        };
    }

    /// Returns the host memory page size in bytes.
    pub fn pageSize(): usize {
        return std.heap.pageSize();
    }

    /// Returns the number of logical CPU cores available (fallback `1`).
    pub fn cpuCount(): usize {
        return std.Thread.getCpuCount() catch 1;
    }

    /// Returns the target CPU byte order.
    pub fn endianness(): Endianness {
        return comptime switch (@import("builtin").cpu.arch.endian()) {
            .little => Endianness.Little,
            .big => Endianness.Big,
        };
    }

    /// Queries the live OS kernel version (`std.os.linux.uname` on Linux).
    pub fn kernelVersion(): KernelVersion! {
        if (comptime @import("builtin").os.tag == .linux) {
            let rel_buf: [65]u8 = [0; 65];
            let rel_len: usize = 0;
            let ok = false;
            zig {
                var uts: std.os.linux.utsname = undefined;
                const rc = std.os.linux.uname(&uts);
                if (std.os.linux.errno(rc) == .SUCCESS) {
                    const rel = std.mem.sliceTo(&uts.release, 0);
                    const n = @min(rel.len, rel_buf.len);
                    @memcpy(rel_buf[0..n], rel[0..n]);
                    rel_len = n;
                    ok = true;
                }
            }
            if (!ok) return error.SyscallFailed;
            return KernelVersion.parse(rel_buf[0..rel_len]);
        }
        return error.UnsupportedPlatform;
    }

    /// Returns `true` if Linux kernel supports `io_uring` (>= 5.1).
    pub fn supportsIoUring(): bool {
        if (currentOs() != .Linux) return false;
        const kv = kernelVersion() catch return false;
        return kv.isAtLeast(5, 1);
    }

    /// Returns `true` if Linux kernel supports `copy_file_range` (>= 4.5).
    pub fn supportsCopyFileRange(): bool {
        if (currentOs() != .Linux) return false;
        const kv = kernelVersion() catch return false;
        return kv.isAtLeast(4, 5);
    }

    /// Returns `true` if Linux kernel supports `statx` (>= 4.11).
    pub fn supportsStatx(): bool {
        if (currentOs() != .Linux) return false;
        const kv = kernelVersion() catch return false;
        return kv.isAtLeast(4, 11);
    }

    /// Returns `true` if Linux kernel supports `pidfd` (>= 5.3).
    pub fn supportsPidfd(): bool {
        if (currentOs() != .Linux) return false;
        const kv = kernelVersion() catch return false;
        return kv.isAtLeast(5, 3);
    }

    /// Reads the current executable path (`/proc/self/exe` on Linux) into `buf`.
    pub fn executablePath(buf: []u8): []const u8! {
        if (buf.len == 0) return error.BufferTooSmall;
        if (comptime @import("builtin").os.tag == .linux) {
            let out_len: usize = 0;
            let ok = false;
            zig {
                const rc = std.os.linux.readlink("/proc/self/exe", buf.ptr, buf.len);
                if (std.os.linux.errno(rc) == .SUCCESS) {
                    out_len = rc;
                    ok = true;
                }
            }
            if (!ok) return error.SyscallFailed;
            return buf[0..out_len];
        }
        return error.UnsupportedPlatform;
    }

    /// Reads the system hostname from `uname` nodename into `buf`.
    pub fn hostname(buf: []u8): []const u8! {
        if (comptime @import("builtin").os.tag == .linux) {
            let host_buf: [65]u8 = [0; 65];
            let host_len: usize = 0;
            let ok = false;
            zig {
                var uts: std.os.linux.utsname = undefined;
                const rc = std.os.linux.uname(&uts);
                if (std.os.linux.errno(rc) == .SUCCESS) {
                    const node = std.mem.sliceTo(&uts.nodename, 0);
                    const n = @min(node.len, host_buf.len);
                    @memcpy(host_buf[0..n], node[0..n]);
                    host_len = n;
                    ok = true;
                }
            }
            if (!ok) return error.SyscallFailed;
            if (buf.len < host_len) return error.BufferTooSmall;
            @memcpy(buf[0..host_len], host_buf[0..host_len]);
            return buf[0..host_len];
        }
        return error.UnsupportedPlatform;
    }
}
