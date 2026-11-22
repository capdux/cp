// Source: Absorbed from /data/dev/bun/src/install/lifecycle_script_runner.cp | Synced: 2026-11-22T01:27:38Z
// ============================================================================
// lib/pkg/lifecycle.cp — Zero-Trust Lifecycle Script Runner & Postinstall Optimizer
// Absorbed from Bun src/install/{lifecycle_script_runner.cp,postinstall_optimizer.cp}
// ============================================================================

const std = @import("std");
import { JsonValue, JsonTag } from "cap:json";
import { Pipe, IoStatus } from "cap:io";
import {
    FD,
    PosixSpawn,
    SpawnOptions,
    PollFd,
    POLLIN,
    POLLERR,
    POLLHUP,
    waitpid,
    poll,
    monotonicNs,
} from "cap:sys";
import {
    PackageNameHash,
    TruncatedPackageNameHash,
    hashPackageName,
    truncatePackageNameHash,
} from "./types.cp";

pub err LifecycleError {
    UntrustedScriptBlocked,
    SpawnFailed,
    ScriptFailed,
    OutOfMemory,
}

pub enum LifecyclePhase: u8 {
    Preinstall = 0,
    Install = 1,
    Postinstall = 2,
    Prepare = 3,
    Preprepare = 4,
    Postprepare = 5,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .Preinstall => "preinstall",
            .Install => "install",
            .Postinstall => "postinstall",
            .Prepare => "prepare",
            .Preprepare => "preprepare",
            .Postprepare => "postprepare",
        };
    }

    pub static fn fromStr(s: []const u8): LifecyclePhase? {
        if (std.mem.eql(u8, s, "preinstall")) return .Preinstall;
        if (std.mem.eql(u8, s, "install")) return .Install;
        if (std.mem.eql(u8, s, "postinstall")) return .Postinstall;
        if (std.mem.eql(u8, s, "prepare")) return .Prepare;
        if (std.mem.eql(u8, s, "preprepare")) return .Preprepare;
        if (std.mem.eql(u8, s, "postprepare")) return .Postprepare;
        return null;
    }
}

pub struct PackageScripts {
    preinstall: []const u8 = "",
    install: []const u8 = "",
    postinstall: []const u8 = "",
    prepare: []const u8 = "",

    pub static fn fromPackageJson(root: JsonValue): PackageScripts {
        let out = PackageScripts{};
        if (root.tag != .Object) return out;
        const scripts_val = root.get("scripts") ?? return out;
        if (scripts_val.tag != .Object) return out;

        if (s := scripts_val.get("preinstall")?.asString()) out.preinstall = s;
        if (s := scripts_val.get("install")?.asString()) out.install = s;
        if (s := scripts_val.get("postinstall")?.asString()) out.postinstall = s;
        if (s := scripts_val.get("prepare")?.asString()) out.prepare = s;
        return out;
    }

    pub fn get(phase: LifecyclePhase): []const u8 {
        return switch (phase) {
            .Preinstall => _.preinstall,
            .Install => _.install,
            .Postinstall => _.postinstall,
            .Prepare => _.prepare,
            .Preprepare, .Postprepare => "",
        };
    }

    pub fn hasAny(): bool {
        return _.preinstall.len > 0 || _.install.len > 0 || _.postinstall.len > 0 || _.prepare.len > 0;
    }
}

pub enum PostinstallOptimizeKind: u8 {
    None = 0,
    NativeBinlink = 1,
    Ignore = 2,
}

/// Zero-trust lifecycle security policy:
/// By default (`allow_all == false`), ONLY packages explicitly listed in `trustedDependencies`
/// (and not in `ignored_packages`) are permitted to execute lifecycle scripts.
pub struct TrustedPolicy {
    allow_all: bool = false,
    trusted_names: std.ArrayListUnmanaged(string) = .empty,
    trusted_hashes: std.ArrayListUnmanaged(TruncatedPackageNameHash) = .empty,
    ignored_names: std.ArrayListUnmanaged(string) = .empty,
    native_binlink_names: std.ArrayListUnmanaged(string) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(): TrustedPolicy {
        return TrustedPolicy{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.trusted_names.deinit(allocator);
        _.trusted_hashes.deinit(allocator);
        _.ignored_names.deinit(allocator);
        _.native_binlink_names.deinit(allocator);
        for (s of _.owned_strings.items) allocator.free(s);
        _.owned_strings.deinit(allocator);
    }

    fn internStr(*_, allocator: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try allocator.dupe(u8, s);
        errdefer allocator.free(copy);
        try _.owned_strings.append(allocator, copy);
        return copy;
    }

    pub fn addTrusted(*_, allocator: std.mem.Allocator, pkg_name: []const u8): ! {
        if (pkg_name.len == 0) return;
        for (t of _.trusted_names.items) {
            if (std.mem.eql(u8, t, pkg_name)) return;
        }
        const owned = try _.internStr(allocator, pkg_name);
        try _.trusted_names.append(allocator, owned);
        const h = truncatePackageNameHash(hashPackageName(owned));
        try _.trusted_hashes.append(allocator, h);
    }

    pub fn addIgnored(*_, allocator: std.mem.Allocator, pkg_name: []const u8): ! {
        if (pkg_name.len == 0) return;
        for (ig of _.ignored_names.items) {
            if (std.mem.eql(u8, ig, pkg_name)) return;
        }
        try _.ignored_names.append(allocator, try _.internStr(allocator, pkg_name));
    }

    pub fn addNativeBinlink(*_, allocator: std.mem.Allocator, pkg_name: []const u8): ! {
        if (pkg_name.len == 0) return;
        for (nb of _.native_binlink_names.items) {
            if (std.mem.eql(u8, nb, pkg_name)) return;
        }
        try _.native_binlink_names.append(allocator, try _.internStr(allocator, pkg_name));
    }

    pub fn loadFromPackageJson(*_, allocator: std.mem.Allocator, root: JsonValue): ! {
        if (root.tag != .Object) return;
        if (td := root.get("trustedDependencies")) {
            if (td.tag == .Array) {
                for (elem of td.arr_val) {
                    if (s := elem.asString()) try _.addTrusted(allocator, s);
                }
            }
        }
        if (ig := root.get("ignoreScripts")) {
            if (ig.tag == .Array) {
                for (elem of ig.arr_val) {
                    if (s := elem.asString()) try _.addIgnored(allocator, s);
                }
            }
        }
        if (nd := root.get("nativeDependencies")) {
            if (nd.tag == .Array) {
                for (elem of nd.arr_val) {
                    if (s := elem.asString()) try _.addNativeBinlink(allocator, s);
                }
            }
        }
    }

    pub fn isIgnored(pkg_name: []const u8): bool {
        for (ig of _.ignored_names.items) {
            if (std.mem.eql(u8, ig, pkg_name)) return true;
        }
        return false;
    }

    pub fn optimizeKind(pkg_name: []const u8): PostinstallOptimizeKind {
        if (_.isIgnored(pkg_name)) return .Ignore;
        for (nb of _.native_binlink_names.items) {
            if (std.mem.eql(u8, nb, pkg_name)) return .NativeBinlink;
        }
        return .None;
    }

    /// Returns `true` only if `pkg_name` is explicitly trusted (or `allow_all` is set) and not ignored.
    pub fn isTrusted(pkg_name: []const u8): bool {
        if (_.isIgnored(pkg_name)) return false;
        if (_.allow_all) return true;
        const h = truncatePackageNameHash(hashPackageName(pkg_name));
        let i: usize = 0;
        while (i < _.trusted_hashes.items.len) : (i += 1) {
            if (_.trusted_hashes.items[i] == h && std.mem.eql(u8, _.trusted_names.items[i], pkg_name)) {
                return true;
            }
        }
        return false;
    }
}

pub struct LifecycleExecutionResult {
    pkg_name: []const u8 = "",
    pkg_version: []const u8 = "",
    phase: LifecyclePhase = .Postinstall,
    script: []const u8 = "",
    allowed: bool = false,
    executed: bool = false,
    exit_code: i32 = 0,
    stdout: []u8 = [],
    stderr: []u8 = [],
    duration_ns: u64 = 0,
    duration_ms: u64 = 0,

    pub fn ok(): bool {
        return _.allowed && _.executed && _.exit_code == 0;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.stdout.len > 0) allocator.free(_.stdout);
        if (_.stderr.len > 0) allocator.free(_.stderr);
        _.stdout = [];
        _.stderr = [];
    }
}

pub struct LifecycleStats {
    total_scripts_seen: usize = 0,
    executed_scripts: usize = 0,
    succeeded_scripts: usize = 0,
    failed_scripts: usize = 0,
    blocked_scripts: usize = 0,
    total_duration_ns: u64 = 0,
    blocked_packages: std.ArrayListUnmanaged(string) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.blocked_packages.deinit(allocator);
    }
}

/// Secure lifecycle script runner powered by `cap:sys` `PosixSpawn` & `cap:shell`.
pub struct LifecycleRunner {
    allocator: std.mem.Allocator,
    policy: TrustedPolicy = TrustedPolicy.init(),
    init_cwd: []const u8 = ".",
    node_modules_bin_dir: []const u8 = "",
    shell_bin: []const u8 = "/bin/sh",
    stats: LifecycleStats = LifecycleStats{},
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(allocator: std.mem.Allocator, init_cwd: []const u8, node_modules_bin_dir: []const u8): LifecycleRunner {
        return LifecycleRunner{
            allocator: allocator,
            init_cwd: init_cwd,
            node_modules_bin_dir: node_modules_bin_dir,
        };
    }

    pub fn deinit(*_) {
        _.policy.deinit(_.allocator);
        _.stats.deinit(_.allocator);
        for (s of _.owned_strings.items) _.allocator.free(s);
        _.owned_strings.deinit(_.allocator);
    }

    fn internStr(*_, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try _.allocator.dupe(u8, s);
        errdefer _.allocator.free(copy);
        try _.owned_strings.append(_.allocator, copy);
        return copy;
    }

    pub fn addTrusted(*_, pkg_name: []const u8): ! {
        try _.policy.addTrusted(_.allocator, pkg_name);
    }

    pub fn loadPolicyFromPackageJson(*_, root: JsonValue): ! {
        try _.policy.loadFromPackageJson(_.allocator, root);
    }

    /// Builds `envp` with prepended `node_modules/.bin` in `PATH` and standard npm lifecycle env vars:
    /// `npm_package_name`, `npm_package_version`, `npm_lifecycle_event`, `npm_lifecycle_script`, `INIT_CWD`.
    pub fn buildEnvpAlloc(
        pkg_name: []const u8,
        pkg_version: []const u8,
        pkg_dir: []const u8,
        phase: LifecyclePhase,
        script_cmd: []const u8,
    ): ([][]u8)! {
        let env_list: std.ArrayListUnmanaged([]u8) = .empty;
        errdefer {
            for (item of env_list.items) _.allocator.free(item);
            env_list.deinit(_.allocator);
        }

        let existing_path: []const u8 = "/usr/local/bin:/usr/bin:/bin";
        zig {
            var idx: usize = 0;
            while (std.c.environ[idx]) |c_entry| : (idx += 1) {
                const entry = std.mem.span(c_entry);
                if (std.mem.startsWith(u8, entry, "PATH=")) {
                    existing_path = entry[5..];
                    continue;
                }
                if (std.mem.startsWith(u8, entry, "npm_package_name=") or
                    std.mem.startsWith(u8, entry, "npm_package_version=") or
                    std.mem.startsWith(u8, entry, "npm_lifecycle_event=") or
                    std.mem.startsWith(u8, entry, "npm_lifecycle_script=") or
                    std.mem.startsWith(u8, entry, "npm_package_json=") or
                    std.mem.startsWith(u8, entry, "INIT_CWD="))
                {
                    continue;
                }
                const dup_e = try self.allocator.dupe(u8, entry);
                try env_list.append(self.allocator, dup_e);
            }
        }

        const path_entry = if (_.node_modules_bin_dir.len > 0)
            try std.fmt.allocPrint(_.allocator, "PATH={s}:{s}", .{ _.node_modules_bin_dir, existing_path })
        else
            try std.fmt.allocPrint(_.allocator, "PATH={s}", .{existing_path});
        try env_list.append(_.allocator, path_entry);

        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "npm_package_name={s}", .{pkg_name}));
        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "npm_package_version={s}", .{pkg_version}));
        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "npm_lifecycle_event={s}", .{phase.asStr()}));
        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "npm_lifecycle_script={s}", .{script_cmd}));
        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "INIT_CWD={s}", .{_.init_cwd}));

        const clean_pkg_dir = std.mem.trimEnd(u8, pkg_dir, "/");
        try env_list.append(_.allocator, try std.fmt.allocPrint(_.allocator, "npm_package_json={s}/package.json", .{clean_pkg_dir}));

        return env_list.toOwnedSlice(_.allocator);
    }

    fn freeEnvp(envp: [][]u8) {
        for (item of envp) _.allocator.free(item);
        _.allocator.free(envp);
    }

    /// Executes a single lifecycle script inside `pkg_dir` if allowed by `TrustedPolicy`.
    /// Untrusted packages are blocked and recorded in `stats.blocked_scripts`.
    pub fn runScript(
        *_,
        pkg_name: []const u8,
        pkg_version: []const u8,
        pkg_dir: []const u8,
        phase: LifecyclePhase,
        script_cmd: []const u8,
    ): LifecycleExecutionResult! {
        _.stats.total_scripts_seen += 1;

        if (!_.policy.isTrusted(pkg_name)) {
            _.stats.blocked_scripts += 1;
            const owned_name = try _.internStr(pkg_name);
            let already_recorded = false;
            for (b of _.stats.blocked_packages.items) {
                if (std.mem.eql(u8, b, owned_name)) {
                    already_recorded = true;
                    break;
                }
            }
            if (!already_recorded) {
                try _.stats.blocked_packages.append(_.allocator, owned_name);
            }
            return LifecycleExecutionResult{
                pkg_name: pkg_name,
                pkg_version: pkg_version,
                phase: phase,
                script: script_cmd,
                allowed: false,
                executed: false,
                exit_code: 0,
            };
        }

        let out_pipe = try Pipe.create();
        let err_pipe = Pipe.create() catch |e| {
            out_pipe.close();
            return e;
        };

        const envp_owned = _.buildEnvpAlloc(pkg_name, pkg_version, pkg_dir, phase, script_cmd) catch |e| {
            out_pipe.close();
            err_pipe.close();
            return e;
        };
        defer _.freeEnvp(envp_owned);

        let envp_const = try _.allocator.alloc(string, envp_owned.len);
        defer _.allocator.free(envp_const);
        let ei: usize = 0;
        while (ei < envp_owned.len) : (ei += 1) {
            envp_const[ei] = envp_owned[ei];
        }

        const argv: [3]string = [_.shell_bin, "-c", script_cmd];
        const spawn_opts = SpawnOptions{
            cwd: pkg_dir,
            envp: envp_const,
            stdout_fd: FD.fromNative(out_pipe.writer.getFd()),
            stderr_fd: FD.fromNative(err_pipe.writer.getFd()),
            search_path: true,
        };

        const t_start = monotonicNs();
        const pid = PosixSpawn.spawn(_.allocator, argv[0..3], spawn_opts) catch {
            out_pipe.close();
            err_pipe.close();
            return error.SpawnFailed;
        };

        out_pipe.writer.close();
        err_pipe.writer.close();

        * = out_pipe.reader.setNonblocking(true) catch {};
        * = err_pipe.reader.setNonblocking(true) catch {};

        let stdout_buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer stdout_buf.deinit(_.allocator);
        let stderr_buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer stderr_buf.deinit(_.allocator);

        let out_open = true;
        let err_open = true;
        let rbuf: [4096]u8 = undefined;

        while (out_open || err_open) {
            let pfds: [2]PollFd = undefined;
            let nfds: usize = 0;
            let out_slot: usize = 99;
            let err_slot: usize = 99;

            if (out_open) {
                out_slot = nfds;
                pfds[nfds] = PollFd{ .fd = out_pipe.reader.getFd(), .events = POLLIN, .revents = 0 };
                nfds += 1;
            }
            if (err_open) {
                err_slot = nfds;
                pfds[nfds] = PollFd{ .fd = err_pipe.reader.getFd(), .events = POLLIN, .revents = 0 };
                nfds += 1;
            }
            if (nfds == 0) break;

            * = poll(pfds[0..nfds], 5000) catch break;

            if (out_open && out_slot < nfds) {
                const rev = pfds[out_slot].revents;
                if ((rev & (POLLIN | POLLERR | POLLHUP)) != 0) {
                    const rr = out_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
                    if (rr.bytes > 0) {
                        try stdout_buf.appendSlice(_.allocator, rbuf[0..rr.bytes]);
                    }
                    switch (rr.status) {
                        .EndOfFile, .Error, .Closed => {
                            out_pipe.reader.close();
                            out_open = false;
                        },
                        else => {},
                    }
                }
            }
            if (err_open && err_slot < nfds) {
                const rev = pfds[err_slot].revents;
                if ((rev & (POLLIN | POLLERR | POLLHUP)) != 0) {
                    const rr = err_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
                    if (rr.bytes > 0) {
                        try stderr_buf.appendSlice(_.allocator, rbuf[0..rr.bytes]);
                    }
                    switch (rr.status) {
                        .EndOfFile, .Error, .Closed => {
                            err_pipe.reader.close();
                            err_open = false;
                        },
                        else => {},
                    }
                }
            }
        }

        if (out_open) out_pipe.reader.close();
        if (err_open) err_pipe.reader.close();

        const wres = try waitpid(pid, 0);
        const elapsed_ns = monotonicNs() - t_start;
        const exit_code: i32 = if (wres.exited())
            (wres.exitCode() as i32)
        else if (wres.signaled())
            (128 + (wres.termSig() as i32))
        else
            1;

        _.stats.executed_scripts += 1;
        _.stats.total_duration_ns += elapsed_ns;
        if (exit_code == 0) {
            _.stats.succeeded_scripts += 1;
        } else {
            _.stats.failed_scripts += 1;
        }

        return LifecycleExecutionResult{
            pkg_name: pkg_name,
            pkg_version: pkg_version,
            phase: phase,
            script: script_cmd,
            allowed: true,
            executed: true,
            exit_code: exit_code,
            stdout: try stdout_buf.toOwnedSlice(_.allocator),
            stderr: try stderr_buf.toOwnedSlice(_.allocator),
            duration_ns: elapsed_ns,
            duration_ms: elapsed_ns / 1000000,
        };
    }

    /// Runs `preinstall -> install -> postinstall` (and optional `prepare`) for a package.
    pub fn runPackageLifecycle(
        *_,
        pkg_name: []const u8,
        pkg_version: []const u8,
        pkg_dir: []const u8,
        scripts: PackageScripts,
        run_prepare: bool = false,
    ): (std.ArrayListUnmanaged(LifecycleExecutionResult))! {
        let results: std.ArrayListUnmanaged(LifecycleExecutionResult) = .empty;
        errdefer {
            for (*r of results.items) r.deinit(_.allocator);
            results.deinit(_.allocator);
        }

        const phases: [4]LifecyclePhase = [.Preinstall, .Install, .Postinstall, .Prepare];
        const limit: usize = if (run_prepare) 4 else 3;
        let i: usize = 0;
        while (i < limit) : (i += 1) {
            const ph = phases[i];
            const cmd = scripts.get(ph);
            if (cmd.len == 0) continue;
            const res = try _.runScript(pkg_name, pkg_version, pkg_dir, ph, cmd);
            try results.append(_.allocator, res);
            if (res.allowed && res.exit_code != 0) {
                break;
            }
        }
        return results;
    }
}
