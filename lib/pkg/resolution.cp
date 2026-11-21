// Source: Absorbed from /data/dev/bun/src/install/resolution.cp | Synced: 2026-11-21T05:48:10Z
// ============================================================================
// lib/pkg/resolution.cp — Package Resolution, Platform Bitmasks, Meta & BinLink
// Absorbed from Bun src/install/{resolution,repository,bin,lockfile/Package/Meta}.cp
// ============================================================================

const std = @import("std");
import { Integrity } from "../resolver/integrity.cp";
import { JsonValue } from "cap:json";
import {
    PackageID,
    INVALID_PACKAGE_ID,
    PkgError,
} from "./types.cp";
import { Dependency, DependencyVersionTag } from "./dependency.cp";

pub enum ResolutionTag: u8 {
    Root = 0,
    Npm = 1,
    LocalTarball = 2,
    RemoteTarball = 3,
    Folder = 4,
    Git = 5,
    GitHub = 6,
    Workspace = 7,
    Symlink = 8,
    SingleFileModule = 100,
    Uninit = 255,

    fn isGit(): bool {
        return _ == .Git || _ == .GitHub;
    }

    fn canEnqueueInstallTask(): bool {
        return _ == .Npm || _ == .LocalTarball || _ == .RemoteTarball || _ == .Git || _ == .GitHub;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Root => "root",
            .Npm => "npm",
            .LocalTarball => "local_tarball",
            .RemoteTarball => "remote_tarball",
            .Folder => "folder",
            .Git => "git",
            .GitHub => "github",
            .Workspace => "workspace",
            .Symlink => "symlink",
            .SingleFileModule => "single_file_module",
            .Uninit => "uninitialized",
        };
    }

    static fn fromStr(s: []const u8): ResolutionTag? {
        if (std.mem.eql(u8, s, "root")) return .Root;
        if (std.mem.eql(u8, s, "npm")) return .Npm;
        if (std.mem.eql(u8, s, "local_tarball")) return .LocalTarball;
        if (std.mem.eql(u8, s, "remote_tarball")) return .RemoteTarball;
        if (std.mem.eql(u8, s, "folder")) return .Folder;
        if (std.mem.eql(u8, s, "git")) return .Git;
        if (std.mem.eql(u8, s, "github")) return .GitHub;
        if (std.mem.eql(u8, s, "workspace")) return .Workspace;
        if (std.mem.eql(u8, s, "symlink")) return .Symlink;
        if (std.mem.eql(u8, s, "single_file_module")) return .SingleFileModule;
        if (std.mem.eql(u8, s, "uninitialized")) return .Uninit;
        return null;
    }
}

pub struct RepositoryInfo {
    owner: []const u8 = "",
    repo: []const u8 = "",
    committish: []const u8 = "",
    resolved: []const u8 = "",
    package_name: []const u8 = "",

    pub static fn parseGit(input: []const u8): RepositoryInfo {
        let remain = input;
        if (std.mem.startsWith(u8, remain, "git+")) {
            remain = remain[4..];
        }
        if (hash_idx := std.mem.lastIndexOfScalar(u8, remain, '#')) {
            return RepositoryInfo{
                repo: remain[0..hash_idx],
                committish: remain[hash_idx + 1 ..],
            };
        }
        return RepositoryInfo{ repo: remain };
    }

    pub static fn parseGitHub(input: []const u8): RepositoryInfo {
        let remain = input;
        if (std.mem.startsWith(u8, remain, "github:")) {
            remain = remain[7..];
        }
        let slash_idx: usize? = null;
        let hash_idx: usize? = null;
        let i: usize = 0;
        while (i < remain.len) : (i += 1) {
            if (remain[i] == '/' && slash_idx == null) slash_idx = i;
            if (remain[i] == '#') hash_idx = i;
        }
        const s_idx = slash_idx ?? return RepositoryInfo{ repo: remain };
        const owner = remain[0..s_idx];
        if (h_idx := hash_idx) {
            return RepositoryInfo{
                owner: owner,
                repo: remain[s_idx + 1 .. h_idx],
                committish: remain[h_idx + 1 ..],
            };
        }
        return RepositoryInfo{
            owner: owner,
            repo: remain[s_idx + 1 ..],
        };
    }

    pub fn eql(other: RepositoryInfo): bool {
        if (!std.mem.eql(u8, _.owner, other.owner)) return false;
        if (!std.mem.eql(u8, _.repo, other.repo)) return false;
        if (_.resolved.len > 0 && other.resolved.len > 0) {
            return std.mem.eql(u8, _.resolved, other.resolved);
        }
        return std.mem.eql(u8, _.committish, other.committish);
    }

    pub fn formatAs(label: []const u8, buf: []u8): []const u8 {
        const suffix_ref = if (_.resolved.len > 0) _.resolved else _.committish;
        if (_.owner.len > 0) {
            if (suffix_ref.len > 0) {
                return std.fmt.bufPrint(buf, "{s}{s}/{s}#{s}", .{ label, _.owner, _.repo, suffix_ref }) catch "";
            }
            return std.fmt.bufPrint(buf, "{s}{s}/{s}", .{ label, _.owner, _.repo }) catch "";
        }
        if (suffix_ref.len > 0) {
            return std.fmt.bufPrint(buf, "{s}{s}#{s}", .{ label, _.repo, suffix_ref }) catch "";
        }
        return std.fmt.bufPrint(buf, "{s}{s}", .{ label, _.repo }) catch "";
    }
}

pub struct Resolution {
    tag: ResolutionTag = .Uninit,
    version: []const u8 = "",
    tarball_url: []const u8 = "",
    path: []const u8 = "",
    repo: RepositoryInfo = RepositoryInfo{},

    pub static fn initRoot(): Resolution {
        return Resolution{ tag: .Root };
    }

    pub static fn initNpm(version: []const u8, tarball_url: []const u8): Resolution {
        return Resolution{
            tag: .Npm,
            version: version,
            tarball_url: tarball_url,
        };
    }

    pub static fn initLocalTarball(path: []const u8): Resolution {
        return Resolution{ tag: .LocalTarball, path: path };
    }

    pub static fn initRemoteTarball(url: []const u8): Resolution {
        return Resolution{ tag: .RemoteTarball, tarball_url: url };
    }

    pub static fn initFolder(path: []const u8): Resolution {
        return Resolution{ tag: .Folder, path: path };
    }

    pub static fn initSymlink(path: []const u8): Resolution {
        return Resolution{ tag: .Symlink, path: path };
    }

    pub static fn initWorkspace(path: []const u8): Resolution {
        return Resolution{ tag: .Workspace, path: path };
    }

    pub static fn initGit(repo: RepositoryInfo): Resolution {
        return Resolution{ tag: .Git, repo: repo };
    }

    pub static fn initGitHub(repo: RepositoryInfo): Resolution {
        return Resolution{ tag: .GitHub, repo: repo };
    }

    pub static fn initSingleFile(path: []const u8): Resolution {
        return Resolution{ tag: .SingleFileModule, path: path };
    }

    pub static fn fromTextLockfile(res_str: []const u8): Resolution! {
        if (std.mem.eql(u8, res_str, "root:") || std.mem.eql(u8, res_str, "root")) {
            return Resolution.initRoot();
        }
        if (std.mem.startsWith(u8, res_str, "link:")) {
            return Resolution.initSymlink(res_str[5..]);
        }
        if (std.mem.startsWith(u8, res_str, "workspace:")) {
            return Resolution.initWorkspace(res_str[10..]);
        }
        if (std.mem.startsWith(u8, res_str, "file:")) {
            const p = res_str[5..];
            if (Dependency.isTarball(p)) return Resolution.initLocalTarball(p);
            return Resolution.initFolder(p);
        }
        if (std.mem.startsWith(u8, res_str, "module:")) {
            return Resolution.initSingleFile(res_str[7..]);
        }
        if (std.mem.startsWith(u8, res_str, "github:")) {
            return Resolution.initGitHub(RepositoryInfo.parseGitHub(res_str));
        }
        if (std.mem.startsWith(u8, res_str, "git+")) {
            return Resolution.initGit(RepositoryInfo.parseGit(res_str));
        }

        const inferred = DependencyVersionTag.infer(res_str);
        return switch (inferred) {
            .Git => Resolution.initGit(RepositoryInfo.parseGit(res_str)),
            .GitHub => Resolution.initGitHub(RepositoryInfo.parseGitHub(res_str)),
            .Tarball => if (Dependency.isRemoteTarball(res_str)) Resolution.initRemoteTarball(res_str) else Resolution.initLocalTarball(res_str),
            .Npm => Resolution.initNpm(res_str, ""),
            else => error.InvalidResolution,
        };
    }

    pub fn isGit(): bool {
        return _.tag.isGit();
    }

    pub fn canEnqueueInstallTask(): bool {
        return _.tag.canEnqueueInstallTask();
    }

    pub fn format(buf: []u8): []const u8 {
        return switch (_.tag) {
            .Root => std.fmt.bufPrint(buf, "root:", .{}) catch "",
            .Npm => std.fmt.bufPrint(buf, "{s}", .{_.version}) catch "",
            .LocalTarball => std.fmt.bufPrint(buf, "{s}", .{_.path}) catch "",
            .RemoteTarball => std.fmt.bufPrint(buf, "{s}", .{_.tarball_url}) catch "",
            .Folder => std.fmt.bufPrint(buf, "file:{s}", .{_.path}) catch "",
            .Workspace => std.fmt.bufPrint(buf, "workspace:{s}", .{_.path}) catch "",
            .Symlink => std.fmt.bufPrint(buf, "link:{s}", .{_.path}) catch "",
            .SingleFileModule => std.fmt.bufPrint(buf, "module:{s}", .{_.path}) catch "",
            .Git => _.repo.formatAs("git+", buf),
            .GitHub => _.repo.formatAs("github:", buf),
            .Uninit => "",
        };
    }

    pub fn eql(other: Resolution): bool {
        if (_.tag != other.tag) return false;
        return switch (_.tag) {
            .Root, .Uninit => true,
            .Npm => std.mem.eql(u8, _.version, other.version),
            .LocalTarball, .Folder, .Workspace, .Symlink, .SingleFileModule => std.mem.eql(u8, _.path, other.path),
            .RemoteTarball => std.mem.eql(u8, _.tarball_url, other.tarball_url),
            .Git, .GitHub => _.repo.eql(other.repo),
        };
    }
}

/// Platform OS bitmask (from Bun Npm.OperatingSystem).
pub struct OperatingSystem {
    pub const NONE: u16 = 0;
    pub const AIX: u16 = 1 << 1;
    pub const DARWIN: u16 = 1 << 2;
    pub const FREEBSD: u16 = 1 << 3;
    pub const LINUX: u16 = 1 << 4;
    pub const OPENBSD: u16 = 1 << 5;
    pub const SUNOS: u16 = 1 << 6;
    pub const WIN32: u16 = 1 << 7;
    pub const ANDROID: u16 = 1 << 8;
    pub const ALL: u16 = AIX | DARWIN | FREEBSD | LINUX | OPENBSD | SUNOS | WIN32 | ANDROID;

    pub static fn current(): u16 {
        let res: u16 = ALL;
        zig {
            const os_tag = @import("builtin").target.os.tag;
            if (os_tag == .linux) res = LINUX;
            if (os_tag == .macos) res = DARWIN;
            if (os_tag == .windows) res = WIN32;
            if (os_tag == .freebsd) res = FREEBSD;
        }
        return res;
    }

    pub static fn fromName(name: []const u8): u16 {
        if (std.mem.eql(u8, name, "aix")) return AIX;
        if (std.mem.eql(u8, name, "darwin")) return DARWIN;
        if (std.mem.eql(u8, name, "freebsd")) return FREEBSD;
        if (std.mem.eql(u8, name, "linux")) return LINUX;
        if (std.mem.eql(u8, name, "openbsd")) return OPENBSD;
        if (std.mem.eql(u8, name, "sunos")) return SUNOS;
        if (std.mem.eql(u8, name, "win32")) return WIN32;
        if (std.mem.eql(u8, name, "android")) return ANDROID;
        return 0;
    }

    pub static fn isMatch(mask: u16, target: u16): bool {
        return (mask & target) != 0;
    }

    pub static fn parseJsonArray(val: JsonValue): u16 {
        if (val.tag != .Array || val.arr_val.len == 0) return ALL;
        let added: u16 = NONE;
        let removed: u16 = NONE;
        for (elem of val.arr_val) {
            if (raw := elem.asString()) {
                if (raw.len > 0 && raw[0] == '!') {
                    removed |= OperatingSystem.fromName(raw[1..]);
                } else {
                    added |= OperatingSystem.fromName(raw);
                }
            }
        }
        const base = if (added == NONE) ALL else added;
        return base & ~removed;
    }
}

/// CPU Architecture bitmask (from Bun Npm.Architecture).
pub struct Architecture {
    pub const NONE: u16 = 0;
    pub const ARM: u16 = 1 << 1;
    pub const ARM64: u16 = 1 << 2;
    pub const IA32: u16 = 1 << 3;
    pub const MIPS: u16 = 1 << 4;
    pub const MIPSEL: u16 = 1 << 5;
    pub const PPC: u16 = 1 << 6;
    pub const PPC64: u16 = 1 << 7;
    pub const S390: u16 = 1 << 8;
    pub const S390X: u16 = 1 << 9;
    pub const X32: u16 = 1 << 10;
    pub const X64: u16 = 1 << 11;
    pub const ALL: u16 = ARM | ARM64 | IA32 | MIPS | MIPSEL | PPC | PPC64 | S390 | S390X | X32 | X64;

    pub static fn current(): u16 {
        let res: u16 = ALL;
        zig {
            const arch = @import("builtin").cpu.arch;
            if (arch == .x86_64) res = X64;
            if (arch.isAARCH64()) res = ARM64;
            if (arch.isX86()) res = IA32;
        }
        return res;
    }

    pub static fn fromName(name: []const u8): u16 {
        if (std.mem.eql(u8, name, "arm")) return ARM;
        if (std.mem.eql(u8, name, "arm64")) return ARM64;
        if (std.mem.eql(u8, name, "ia32")) return IA32;
        if (std.mem.eql(u8, name, "mips")) return MIPS;
        if (std.mem.eql(u8, name, "mipsel")) return MIPSEL;
        if (std.mem.eql(u8, name, "ppc")) return PPC;
        if (std.mem.eql(u8, name, "ppc64")) return PPC64;
        if (std.mem.eql(u8, name, "s390")) return S390;
        if (std.mem.eql(u8, name, "s390x")) return S390X;
        if (std.mem.eql(u8, name, "x32")) return X32;
        if (std.mem.eql(u8, name, "x64")) return X64;
        return 0;
    }

    pub static fn isMatch(mask: u16, target: u16): bool {
        return (mask & target) != 0;
    }

    pub static fn parseJsonArray(val: JsonValue): u16 {
        if (val.tag != .Array || val.arr_val.len == 0) return ALL;
        let added: u16 = NONE;
        let removed: u16 = NONE;
        for (elem of val.arr_val) {
            if (raw := elem.asString()) {
                if (raw.len > 0 && raw[0] == '!') {
                    removed |= Architecture.fromName(raw[1..]);
                } else {
                    added |= Architecture.fromName(raw);
                }
            }
        }
        const base = if (added == NONE) ALL else added;
        return base & ~removed;
    }
}

/// C library bitmask (from Bun Npm.Libc).
pub struct Libc {
    pub const NONE: u8 = 0;
    pub const GLIBC: u8 = 1 << 1;
    pub const MUSL: u8 = 1 << 2;
    pub const ALL: u8 = GLIBC | MUSL;

    pub static fn current(): u8 {
        return GLIBC;
    }

    pub static fn fromName(name: []const u8): u8 {
        if (std.mem.eql(u8, name, "glibc")) return GLIBC;
        if (std.mem.eql(u8, name, "musl")) return MUSL;
        return 0;
    }

    pub static fn isMatch(mask: u8, target: u8): bool {
        if (mask == NONE) return true;
        return (mask & target) != 0;
    }

    pub static fn parseJsonArray(val: JsonValue): u8 {
        if (val.tag != .Array || val.arr_val.len == 0) return NONE;
        let added: u8 = NONE;
        let removed: u8 = NONE;
        for (elem of val.arr_val) {
            if (raw := elem.asString()) {
                if (raw.len > 0 && raw[0] == '!') {
                    removed |= Libc.fromName(raw[1..]);
                } else {
                    added |= Libc.fromName(raw);
                }
            }
        }
        const base = if (added == NONE) ALL else added;
        return base & ~removed;
    }
}

pub enum Origin: u8 {
    Npm = 0,
    Local = 1,
    Git = 2,
    Tarball = 3,
    Workspace = 4,
}

/// Package metadata stored per resolved lockfile package (from Bun lockfile/Package/Meta.cp).
pub struct PackageMeta {
    id: PackageID = INVALID_PACKAGE_ID,
    origin: Origin = .Npm,
    os: u16 = OperatingSystem.ALL,
    cpu: u16 = Architecture.ALL,
    libc: u8 = Libc.NONE,
    has_install_script: bool = false,
    is_trusted: bool = false,
    integrity: Integrity = Integrity{},
    tarball_url: []const u8 = "",
    man_dir: []const u8 = "",

    pub fn isDisabled(target_cpu: u16, target_os: u16): bool {
        return !Architecture.isMatch(_.cpu, target_cpu) || !OperatingSystem.isMatch(_.os, target_os);
    }

    pub fn isDisabledForCurrentPlatform(): bool {
        return _.isDisabled(Architecture.current(), OperatingSystem.current());
    }

    pub fn verifyBytes(data: []const u8): bool {
        if (!_.integrity.tag.isSupported()) return true;
        return _.integrity.verify(data);
    }
}

pub enum BinLinkKind: u8 {
    None = 0,
    File = 1,
    NamedFile = 2,
    Dir = 3,
    Map = 4,
}

pub struct BinEntry {
    name: []const u8 = "",
    target: []const u8 = "",
}

/// Normalized `bin` field in package.json (from Bun bin.cp).
pub struct BinLink {
    kind: BinLinkKind = .None,
    file_path: []const u8 = "",
    dir_path: []const u8 = "",
    entries: std.ArrayListUnmanaged(BinEntry) = .empty,

    pub static fn initNone(): BinLink {
        return BinLink{};
    }

    pub static fn initFile(path: []const u8): BinLink {
        return BinLink{ kind: .File, file_path: path };
    }

    pub static fn initNamedFile(allocator: std.mem.Allocator, name: []const u8, path: []const u8): BinLink! {
        let b = BinLink{ kind: .NamedFile };
        try b.entries.append(allocator, BinEntry{ name: name, target: path });
        return b;
    }

    pub static fn initDir(dir_path: []const u8): BinLink {
        return BinLink{ kind: .Dir, dir_path: dir_path };
    }

    pub static fn initMap(): BinLink {
        return BinLink{ kind: .Map };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.entries.deinit(allocator);
    }

    pub fn addEntry(*_, allocator: std.mem.Allocator, name: []const u8, target: []const u8): ! {
        if (_.kind == .None) {
            _.kind = .NamedFile;
        } else if (_.kind == .NamedFile && _.entries.items.len >= 1) {
            _.kind = .Map;
        }
        try _.entries.append(allocator, BinEntry{ name: name, target: target });
    }

    pub fn isEmpty(): bool {
        return _.kind == .None;
    }

    pub static fn normalizedBinName(name: []const u8): []const u8 {
        if (idx := std.mem.lastIndexOfAny(u8, name, "/\\:")) {
            return name[idx + 1 ..];
        }
        return name;
    }

    pub static fn fromJson(allocator: std.mem.Allocator, bin_val: JsonValue?, dir_bin_val: JsonValue?): BinLink! {
        if (bv := bin_val) {
            if (bv.tag == .String && bv.str_val.len > 0) {
                return BinLink.initFile(bv.str_val);
            }
            if (bv.tag == .Object) {
                if (bv.obj_val.len == 1) {
                    const f = bv.obj_val[0];
                    if (val_s := f.value.asString()) {
                        if (val_s.len > 0) {
                            return BinLink.initNamedFile(allocator, f.key, val_s);
                        }
                    }
                } else if (bv.obj_val.len > 1) {
                    let b = BinLink.initMap();
                    errdefer b.deinit(allocator);
                    for (f of bv.obj_val) {
                        if (val_s := f.value.asString()) {
                            if (val_s.len > 0) {
                                try b.entries.append(allocator, BinEntry{ name: f.key, target: val_s });
                            }
                        }
                    }
                    if (b.entries.items.len > 0) return b;
                }
            }
        }
        if (dv := dir_bin_val) {
            if (s := dv.asString()) {
                if (s.len > 0) return BinLink.initDir(s);
            }
        }
        return BinLink.initNone();
    }
}
