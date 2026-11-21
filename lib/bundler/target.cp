// Source: Absorbed from /data/dev/bun/src/options_types/CompileTarget.zig | Synced: 2026-11-21T01:08:38Z
// ============================================================================
// lib/bundler/target.cp — Cross-Compilation Target Triple Parser & Formatter
// ============================================================================

const std = @import("std");

pub enum TargetOs: u8 {
    Linux = 0,
    Darwin = 1,
    Windows = 2,
    FreeBSD = 3,
    Wasi = 4,

    pub fn npmName(): []const u8 {
        return switch (_) {
            .Linux => "linux",
            .Darwin => "darwin",
            .Windows => "windows",
            .FreeBSD => "freebsd",
            .Wasi => "wasi",
        };
    }

    pub fn isUnix(): bool {
        return switch (_) {
            .Linux, .Darwin, .FreeBSD => true,
            .Windows, .Wasi => false,
        };
    }

    pub fn exeExt(): []const u8 {
        return switch (_) {
            .Windows => ".exe",
            .Wasi => ".wasm",
            else => "",
        };
    }

    pub fn sharedLibExt(): []const u8 {
        return switch (_) {
            .Windows => ".dll",
            .Darwin => ".dylib",
            .Wasi => ".wasm",
            else => ".so",
        };
    }
}

pub enum TargetArch: u8 {
    X64 = 0,
    Arm64 = 1,
    Riscv64 = 2,
    Wasm32 = 3,

    pub fn npmName(): []const u8 {
        return switch (_) {
            .X64 => "x64",
            .Arm64 => "arm64",
            .Riscv64 => "riscv64",
            .Wasm32 => "wasm32",
        };
    }
}

pub enum TargetAbi: u8 {
    Gnu = 0,
    Musl = 1,
    Msvc = 2,
    None = 3,

    pub fn npmName(): []const u8 {
        return switch (_) {
            .Gnu => "gnu",
            .Musl => "musl",
            .Msvc => "msvc",
            .None => "none",
        };
    }
}

pub err TargetParseError {
    InvalidTarget,
    UnsupportedTarget,
}

fn parseOsToken(tok: []const u8): TargetOs? {
    if (std.ascii.eqlIgnoreCase(tok, "linux")) return .Linux;
    if (std.ascii.eqlIgnoreCase(tok, "darwin") || std.ascii.eqlIgnoreCase(tok, "macos")) return .Darwin;
    if (std.ascii.eqlIgnoreCase(tok, "windows") || std.ascii.eqlIgnoreCase(tok, "win32") || std.ascii.eqlIgnoreCase(tok, "win")) return .Windows;
    if (std.ascii.eqlIgnoreCase(tok, "freebsd")) return .FreeBSD;
    if (std.ascii.eqlIgnoreCase(tok, "wasi")) return .Wasi;
    return null;
}

fn parseArchToken(tok: []const u8): TargetArch? {
    if (std.ascii.eqlIgnoreCase(tok, "x64") || std.ascii.eqlIgnoreCase(tok, "x86_64") || std.ascii.eqlIgnoreCase(tok, "amd64")) return .X64;
    if (std.ascii.eqlIgnoreCase(tok, "arm64") || std.ascii.eqlIgnoreCase(tok, "aarch64")) return .Arm64;
    if (std.ascii.eqlIgnoreCase(tok, "riscv64")) return .Riscv64;
    if (std.ascii.eqlIgnoreCase(tok, "wasm32") || std.ascii.eqlIgnoreCase(tok, "wasm")) return .Wasm32;
    return null;
}

fn parseAbiToken(tok: []const u8): TargetAbi? {
    if (std.ascii.eqlIgnoreCase(tok, "gnu") || std.ascii.eqlIgnoreCase(tok, "glibc")) return .Gnu;
    if (std.ascii.eqlIgnoreCase(tok, "musl")) return .Musl;
    if (std.ascii.eqlIgnoreCase(tok, "msvc")) return .Msvc;
    if (std.ascii.eqlIgnoreCase(tok, "none")) return .None;
    return null;
}

/// Cross-compilation target triple (`os`, `arch`, `abi`, `baseline`) absorbed from Bun `CompileTarget`.
pub struct CompileTarget {
    os: TargetOs = .Linux,
    arch: TargetArch = .X64,
    abi: TargetAbi = .Gnu,
    baseline: bool = false,

    /// Detects the host OS, architecture, and ABI at compile time.
    pub static fn host(): CompileTarget {
        const os: TargetOs = switch (@import("builtin").os.tag) {
            .linux => .Linux,
            .macos => .Darwin,
            .windows => .Windows,
            .freebsd => .FreeBSD,
            .wasi => .Wasi,
            else => .Linux,
        };
        const arch: TargetArch = switch (@import("builtin").cpu.arch) {
            .x86_64 => .X64,
            .aarch64 => .Arm64,
            .riscv64 => .Riscv64,
            .wasm32 => .Wasm32,
            else => .X64,
        };
        const abi: TargetAbi = switch (@import("builtin").abi) {
            .musl, .musleabi, .musleabihf => .Musl,
            .msvc => .Msvc,
            .none => .None,
            else => if (os == .Linux) .Gnu else .None,
        };
        return CompileTarget{
            os: os,
            arch: arch,
            abi: abi,
            baseline: false,
        };
    }

    /// Parses target triples such as `"linux-x64-musl"`, `"bun-darwin-arm64"`,
    /// `"cp-windows-x64-msvc"`, `"linux-arm64-baseline"`, or `"wasi-wasm32"`.
    pub static fn parse(input_raw: []const u8): CompileTarget! {
        const input = std.mem.trim(u8, input_raw, " \t\r\n");
        if (input.len == 0) {
            return error.InvalidTarget;
        }

        let target = CompileTarget.host();
        let found_os = false;
        let found_arch = false;
        let found_abi = false;

        let pos: usize = 0;
        while (pos < input.len) {
            const next_dash = std.mem.indexOfScalarPos(u8, input, pos, '-');
            let tok = input[pos..];
            if (idx := next_dash) {
                tok = input[pos..idx];
                pos = idx + 1;
            } else {
                pos = input.len;
            }

            if (tok.len == 0) continue;

            // Optional vendor/runtime prefix tokens
            if (std.ascii.eqlIgnoreCase(tok, "bun") || std.ascii.eqlIgnoreCase(tok, "cp")) {
                continue;
            }
            if (std.ascii.eqlIgnoreCase(tok, "baseline")) {
                target.baseline = true;
                continue;
            }
            if (std.ascii.eqlIgnoreCase(tok, "modern")) {
                target.baseline = false;
                continue;
            }
            if (os_val := parseOsToken(tok)) {
                target.os = os_val;
                found_os = true;
                continue;
            }
            if (arch_val := parseArchToken(tok)) {
                target.arch = arch_val;
                found_arch = true;
                continue;
            }
            if (abi_val := parseAbiToken(tok)) {
                target.abi = abi_val;
                found_abi = true;
                continue;
            }
            // Ignore optional version suffix like v1.2.3
            if (tok.len >= 2 && (tok[0] == 'v' || tok[0] == 'V') && tok[1] >= '0' && tok[1] <= '9') {
                continue;
            }
            return error.UnsupportedTarget;
        }

        if (!found_os && !found_arch && !found_abi) {
            return error.InvalidTarget;
        }

        if (found_os && !found_arch) {
            target.arch = if (target.os == .Wasi) .Wasm32 else .X64;
        }
        if (!found_os && target.arch == .Wasm32) {
            target.os = .Wasi;
        }

        if (!found_abi) {
            target.abi = switch (target.os) {
                .Linux => .Gnu,
                .Windows => .Msvc,
                .Darwin, .FreeBSD, .Wasi => .None,
            };
        }

        // Cross-check invalid OS/ABI combinations
        if (target.abi == .Musl && target.os != .Linux) {
            return error.InvalidTarget;
        }
        if (target.abi == .Msvc && target.os != .Windows) {
            return error.InvalidTarget;
        }
        if (target.os == .Wasi && target.arch != .Wasm32) {
            return error.InvalidTarget;
        }
        if (target.arch == .Arm64) {
            target.baseline = false;
        }

        return target;
    }

    /// Formats the canonical target triple into `buf` (e.g. `"linux-x64-musl"` or `"darwin-arm64"`).
    pub fn format(buf: []u8): []const u8 {
        const show_abi = (_.os == .Linux && _.abi != .None) || (_.os == .Windows && _.abi != .None);
        if (show_abi) {
            if (_.baseline) {
                return std.fmt.bufPrint(buf, "{s}-{s}-{s}-baseline", .{
                    _.os.npmName(),
                    _.arch.npmName(),
                    _.abi.npmName(),
                }) catch "";
            }
            return std.fmt.bufPrint(buf, "{s}-{s}-{s}", .{
                _.os.npmName(),
                _.arch.npmName(),
                _.abi.npmName(),
            }) catch "";
        }
        if (_.baseline) {
            return std.fmt.bufPrint(buf, "{s}-{s}-baseline", .{
                _.os.npmName(),
                _.arch.npmName(),
            }) catch "";
        }
        return std.fmt.bufPrint(buf, "{s}-{s}", .{
            _.os.npmName(),
            _.arch.npmName(),
        }) catch "";
    }

    pub fn formatAlloc(allocator: std.mem.Allocator): ([]u8)! {
        let tmp: [128]u8 = undefined;
        const formatted = _.format(&tmp);
        return allocator.dupe(u8, formatted);
    }

    pub fn exeExt(): []const u8 {
        return _.os.exeExt();
    }

    pub fn sharedLibExt(): []const u8 {
        return _.os.sharedLibExt();
    }

    pub fn isUnix(): bool {
        return _.os.isUnix();
    }

    pub fn eql(other: CompileTarget): bool {
        return _.os == other.os &&
            _.arch == other.arch &&
            _.abi == other.abi &&
            _.baseline == other.baseline;
    }
}
