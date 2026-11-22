// Source: Absorbed from /data/dev/bun/src/install/lockfile.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pkg/lockfile.cp — Dependency Graph, Cycle/Topo Engine & Dual Lockfile Persistence
// Absorbed from Bun src/install/{lockfile.cp,lockfile/bun.lock.cp,lockfile/bun.lockb.cp}
// ============================================================================

const std = @import("std");
import { Hash } from "cap:hash";
import { JSON, JsonValue } from "cap:json";
import { Integrity } from "../resolver/integrity.cp";
import { readFile, writeFile } from "cap:sys";
import {
    PackageID,
    INVALID_PACKAGE_ID,
    Behavior,
    PkgError,
} from "./types.cp";
import { Resolution, ResolutionTag, RepositoryInfo, PackageMeta, Origin, BinLink, BinLinkKind, BinEntry } from "./resolution.cp";

pub const BINARY_LOCKFILE_MAGIC: []const u8 = "CPLK0001";
pub const BINARY_LOCKFILE_VERSION: u32 = 1;
pub const BINARY_LOCKFILE_HEADER_SIZE: usize = 68;

pub enum LockfileFormat: u8 {
    Text = 0,
    Binary = 1,
}

pub struct LockfileDepEdge {
    name: []const u8 = "",
    spec: []const u8 = "",
    behavior: Behavior = Behavior{},
    resolved_pkg_id: PackageID = INVALID_PACKAGE_ID,
}

pub struct LockfilePackage {
    id: PackageID = INVALID_PACKAGE_ID,
    name: []const u8 = "",
    version: []const u8 = "",
    resolution: Resolution = Resolution{},
    meta: PackageMeta = PackageMeta{},
    bin: BinLink = BinLink{},
    dependencies: std.ArrayListUnmanaged(LockfileDepEdge) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.bin.deinit(allocator);
        _.dependencies.deinit(allocator);
    }
}

pub struct WorkspaceEntry {
    name: []const u8 = "",
    path: []const u8 = "",
    version: []const u8 = "",
    dependencies: std.ArrayListUnmanaged(LockfileDepEdge) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.dependencies.deinit(allocator);
    }
}

pub struct OverrideEntry {
    name: []const u8 = "",
    spec: []const u8 = "",
}

pub struct PatchedDepEntry {
    name_and_version: []const u8 = "",
    patch_path: []const u8 = "",
    patchfile_hash: u64 = 0,
}

/// Industrial-grade Lockfile dependency graph with cycle detection, topological ordering,
/// deterministic text (`cap.lock`) JSON persistence, and compact binary (`CPLK0001`) serialization.
pub struct Lockfile {
    lockfile_version: u32 = 1,
    config_version: u32 = 1,
    packages: std.ArrayListUnmanaged(LockfilePackage) = .empty,
    workspaces: std.ArrayListUnmanaged(WorkspaceEntry) = .empty,
    overrides: std.ArrayListUnmanaged(OverrideEntry) = .empty,
    trusted_dependencies: std.ArrayListUnmanaged(string) = .empty,
    patched_dependencies: std.ArrayListUnmanaged(PatchedDepEntry) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,
    meta_hash: [32]u8 = [0; 32],

    pub static fn init(): Lockfile {
        return Lockfile{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (*pkg of _.packages.items) {
            pkg.deinit(allocator);
        }
        _.packages.deinit(allocator);
        for (*ws of _.workspaces.items) {
            ws.deinit(allocator);
        }
        _.workspaces.deinit(allocator);
        _.overrides.deinit(allocator);
        _.trusted_dependencies.deinit(allocator);
        _.patched_dependencies.deinit(allocator);
        for (s of _.owned_strings.items) {
            allocator.free(s);
        }
        _.owned_strings.deinit(allocator);
    }

    fn internStr(*_, allocator: std.mem.Allocator, s: []const u8): []const u8! {
        if (s.len == 0) return "";
        const copy = try allocator.dupe(u8, s);
        errdefer allocator.free(copy);
        try _.owned_strings.append(allocator, copy);
        return copy;
    }

    pub fn packageCount(): usize {
        return _.packages.items.len;
    }

    pub fn edgeCount(): usize {
        let total: usize = 0;
        for (pkg of _.packages.items) {
            total += pkg.dependencies.items.len;
        }
        return total;
    }

    pub fn addPackage(
        *_,
        allocator: std.mem.Allocator,
        name: []const u8,
        version: []const u8,
        resolution: Resolution,
        meta: PackageMeta,
    ): PackageID! {
        const id: PackageID = @intCast(_.packages.items.len);
        const owned_name = try _.internStr(allocator, name);
        const owned_ver = try _.internStr(allocator, version);

        const owned_res = Resolution{
            tag: resolution.tag,
            version: try _.internStr(allocator, resolution.version),
            tarball_url: try _.internStr(allocator, resolution.tarball_url),
            path: try _.internStr(allocator, resolution.path),
            repo: RepositoryInfo{
                owner: try _.internStr(allocator, resolution.repo.owner),
                repo: try _.internStr(allocator, resolution.repo.repo),
                committish: try _.internStr(allocator, resolution.repo.committish),
                resolved: try _.internStr(allocator, resolution.repo.resolved),
                package_name: try _.internStr(allocator, resolution.repo.package_name),
            },
        };

        const owned_meta = PackageMeta{
            id: id,
            origin: meta.origin,
            os: meta.os,
            cpu: meta.cpu,
            libc: meta.libc,
            has_install_script: meta.has_install_script,
            is_trusted: meta.is_trusted,
            integrity: meta.integrity,
            tarball_url: try _.internStr(allocator, meta.tarball_url),
            man_dir: try _.internStr(allocator, meta.man_dir),
        };

        try _.packages.append(allocator, LockfilePackage{
            id: id,
            name: owned_name,
            version: owned_ver,
            resolution: owned_res,
            meta: owned_meta,
        });
        return id;
    }

    pub fn setPackageBin(*_, allocator: std.mem.Allocator, pkg_id: PackageID, bin: BinLink): ! {
        if ((pkg_id as usize) >= _.packages.items.len) return error.InvalidPackageName;
        let pkg = &_.packages.items[pkg_id as usize];
        pkg.bin.deinit(allocator);
        pkg.bin = BinLink{
            kind: bin.kind,
            file_path: try _.internStr(allocator, bin.file_path),
            dir_path: try _.internStr(allocator, bin.dir_path),
        };
        for (e of bin.entries.items) {
            try pkg.bin.entries.append(allocator, BinEntry{
                name: try _.internStr(allocator, e.name),
                target: try _.internStr(allocator, e.target),
            });
        }
    }

    pub fn addDependency(
        *_,
        allocator: std.mem.Allocator,
        from_pkg_id: PackageID,
        dep_name: []const u8,
        spec: []const u8,
        behavior: Behavior,
        resolved_pkg_id: PackageID,
    ): ! {
        if ((from_pkg_id as usize) >= _.packages.items.len) return error.InvalidPackageName;
        const owned_name = try _.internStr(allocator, dep_name);
        const owned_spec = try _.internStr(allocator, spec);
        try _.packages.items[from_pkg_id as usize].dependencies.append(allocator, LockfileDepEdge{
            name: owned_name,
            spec: owned_spec,
            behavior: behavior,
            resolved_pkg_id: resolved_pkg_id,
        });
    }

    pub fn addWorkspace(*_, allocator: std.mem.Allocator, name: []const u8, path: []const u8, version: []const u8): ! {
        try _.workspaces.append(allocator, WorkspaceEntry{
            name: try _.internStr(allocator, name),
            path: try _.internStr(allocator, path),
            version: try _.internStr(allocator, version),
        });
    }

    pub fn addOverride(*_, allocator: std.mem.Allocator, name: []const u8, spec: []const u8): ! {
        try _.overrides.append(allocator, OverrideEntry{
            name: try _.internStr(allocator, name),
            spec: try _.internStr(allocator, spec),
        });
    }

    pub fn addTrustedDependency(*_, allocator: std.mem.Allocator, name: []const u8): ! {
        for (t of _.trusted_dependencies.items) {
            if (std.mem.eql(u8, t, name)) return;
        }
        try _.trusted_dependencies.append(allocator, try _.internStr(allocator, name));
    }

    pub fn addPatchedDependency(*_, allocator: std.mem.Allocator, name_and_version: []const u8, patch_path: []const u8, patchfile_hash: u64): ! {
        try _.patched_dependencies.append(allocator, PatchedDepEntry{
            name_and_version: try _.internStr(allocator, name_and_version),
            patch_path: try _.internStr(allocator, patch_path),
            patchfile_hash: patchfile_hash,
        });
    }

    pub fn findPackageByName(name: []const u8): PackageID? {
        for (pkg of _.packages.items) {
            if (std.mem.eql(u8, pkg.name, name)) {
                return pkg.id;
            }
        }
        return null;
    }

    pub fn findPackageByNameAndVersion(name: []const u8, version: []const u8): PackageID? {
        for (pkg of _.packages.items) {
            if (std.mem.eql(u8, pkg.name, name) && std.mem.eql(u8, pkg.version, version)) {
                return pkg.id;
            }
        }
        return null;
    }

    pub fn getPackage(id: PackageID): LockfilePackage? {
        if ((id as usize) >= _.packages.items.len) return null;
        return _.packages.items[id as usize];
    }

    /// Computes a deterministic 256-bit hash fingerprint over the lockfile graph.
    pub fn computeMetaHash(*_) {
        const k_mul: u64 = (@as(u64, 0x9e3779b9) << 32) | 0x7f4a7c15;
        let h1: u64 = (@as(u64, 0x517cc1b7) << 32) | 0x27220a95;
        let h2: u64 = k_mul;
        let h3: u64 = (@as(u64, 0xbf58476d) << 32) | 0x1ce4e5b9;
        let h4: u64 = (@as(u64, 0x94d049bb) << 32) | 0x133111eb;

        for (pkg of _.packages.items) {
            h1 = Hash.wyhash(pkg.name, h1);
            h2 = Hash.wyhash(pkg.version, h2);
            h3 ^= @as(u64, pkg.id) *% k_mul;
            for (edge of pkg.dependencies.items) {
                h4 = Hash.wyhash(edge.name, h4 ^ @as(u64, edge.resolved_pkg_id));
            }
        }
        std.mem.writeInt(u64, _.meta_hash[0..8], h1, .little);
        std.mem.writeInt(u64, _.meta_hash[8..16], h2, .little);
        std.mem.writeInt(u64, _.meta_hash[16..24], h3, .little);
        std.mem.writeInt(u64, _.meta_hash[24..32], h4, .little);
    }

    fn dfsCycleCheck(
        pkgs: []const LockfilePackage,
        node: usize,
        state: []u8,
    ): bool {
        state[node] = 1; // visiting
        for (edge of pkgs[node].dependencies.items) {
            if (edge.resolved_pkg_id == INVALID_PACKAGE_ID) continue;
            const target = edge.resolved_pkg_id as usize;
            if (target >= pkgs.len) continue;
            if (state[target] == 1) return true;
            if (state[target] == 0) {
                if (Lockfile.dfsCycleCheck(pkgs, target, state)) return true;
            }
        }
        state[node] = 2; // visited
        return false;
    }

    /// Detects whether the dependency graph contains any directed cycle.
    pub fn hasCycle(allocator: std.mem.Allocator): bool! {
        const n = _.packages.items.len;
        if (n == 0) return false;
        const state = try allocator.alloc(u8, n);
        defer allocator.free(state);
        @memset(state, 0);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (state[i] == 0) {
                if (Lockfile.dfsCycleCheck(_.packages.items, i, state)) return true;
            }
        }
        return false;
    }

    fn dfsTopo(
        pkgs: []const LockfilePackage,
        node: usize,
        state: []u8,
        out: *std.ArrayListUnmanaged(PackageID),
        allocator: std.mem.Allocator,
    ): ! {
        state[node] = 1;
        for (edge of pkgs[node].dependencies.items) {
            if (edge.resolved_pkg_id == INVALID_PACKAGE_ID) continue;
            const target = edge.resolved_pkg_id as usize;
            if (target >= pkgs.len) continue;
            if (state[target] == 1) return error.CyclicDependency;
            if (state[target] == 0) {
                try Lockfile.dfsTopo(pkgs, target, state, out, allocator);
            }
        }
        state[node] = 2;
        try out.append(allocator, pkgs[node].id);
    }

    /// Computes post-order topological ordering (dependencies appear before dependents).
    pub fn topoOrder(allocator: std.mem.Allocator): ([]PackageID)! {
        const n = _.packages.items.len;
        let out: std.ArrayListUnmanaged(PackageID) = .empty;
        errdefer out.deinit(allocator);
        if (n == 0) return out.toOwnedSlice(allocator);

        const state = try allocator.alloc(u8, n);
        defer allocator.free(state);
        @memset(state, 0);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (state[i] == 0) {
                try Lockfile.dfsTopo(_.packages.items, i, state, &out, allocator);
            }
        }
        return out.toOwnedSlice(allocator);
    }

    // ------------------------------------------------------------------------
    // JSON Escaping & Text Lockfile (`cap.lock`) Serialization / Parsing
    // ------------------------------------------------------------------------

    fn writeJsonStr(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): ! {
        try out.append(allocator, '"');
        for (c of s) {
            if (c == '"') {
                try out.appendSlice(allocator, "\\\"");
            } else if (c == '\\') {
                try out.appendSlice(allocator, "\\\\");
            } else if (c == '\n') {
                try out.appendSlice(allocator, "\\n");
            } else if (c == '\r') {
                try out.appendSlice(allocator, "\\r");
            } else if (c == '\t') {
                try out.appendSlice(allocator, "\\t");
            } else {
                try out.append(allocator, c);
            }
        }
        try out.append(allocator, '"');
    }

    /// Serializes the lockfile to deterministic human-readable `cap.lock` JSON text.
    pub fn toTextJsonAlloc(allocator: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        let hdr_buf: [128]u8 = undefined;
        const hdr = std.fmt.bufPrint(
            &hdr_buf,
            "{{\n  \"lockfileVersion\": {d},\n  \"configVersion\": {d},\n",
            .{ _.lockfile_version, _.config_version },
        ) catch unreachable;
        try out.appendSlice(allocator, hdr);

        // Workspaces
        try out.appendSlice(allocator, "  \"workspaces\": [");
        let wi: usize = 0;
        while (wi < _.workspaces.items.len) : (wi += 1) {
            const ws = _.workspaces.items[wi];
            if (wi > 0) try out.appendSlice(allocator, ",");
            try out.appendSlice(allocator, "\n    { \"name\": ");
            try Lockfile.writeJsonStr(&out, allocator, ws.name);
            try out.appendSlice(allocator, ", \"path\": ");
            try Lockfile.writeJsonStr(&out, allocator, ws.path);
            try out.appendSlice(allocator, ", \"version\": ");
            try Lockfile.writeJsonStr(&out, allocator, ws.version);
            try out.appendSlice(allocator, " }");
        }
        if (_.workspaces.items.len > 0) try out.appendSlice(allocator, "\n  ");
        try out.appendSlice(allocator, "],\n");

        // Overrides
        try out.appendSlice(allocator, "  \"overrides\": {");
        let oi: usize = 0;
        while (oi < _.overrides.items.len) : (oi += 1) {
            const ov = _.overrides.items[oi];
            if (oi > 0) try out.appendSlice(allocator, ",");
            try out.appendSlice(allocator, "\n    ");
            try Lockfile.writeJsonStr(&out, allocator, ov.name);
            try out.appendSlice(allocator, ": ");
            try Lockfile.writeJsonStr(&out, allocator, ov.spec);
        }
        if (_.overrides.items.len > 0) try out.appendSlice(allocator, "\n  ");
        try out.appendSlice(allocator, "},\n");

        // Trusted Dependencies
        try out.appendSlice(allocator, "  \"trustedDependencies\": [");
        let ti: usize = 0;
        while (ti < _.trusted_dependencies.items.len) : (ti += 1) {
            if (ti > 0) try out.appendSlice(allocator, ", ");
            try Lockfile.writeJsonStr(&out, allocator, _.trusted_dependencies.items[ti]);
        }
        try out.appendSlice(allocator, "],\n");

        // Patched Dependencies
        try out.appendSlice(allocator, "  \"patchedDependencies\": {");
        let pi: usize = 0;
        while (pi < _.patched_dependencies.items.len) : (pi += 1) {
            const pd = _.patched_dependencies.items[pi];
            if (pi > 0) try out.appendSlice(allocator, ",");
            try out.appendSlice(allocator, "\n    ");
            try Lockfile.writeJsonStr(&out, allocator, pd.name_and_version);
            try out.appendSlice(allocator, ": { \"path\": ");
            try Lockfile.writeJsonStr(&out, allocator, pd.patch_path);
            let h_buf: [48]u8 = undefined;
            const h_str = std.fmt.bufPrint(&h_buf, ", \"hash\": {d} }}", .{pd.patchfile_hash}) catch unreachable;
            try out.appendSlice(allocator, h_str);
        }
        if (_.patched_dependencies.items.len > 0) try out.appendSlice(allocator, "\n  ");
        try out.appendSlice(allocator, "},\n");

        // Packages
        try out.appendSlice(allocator, "  \"packages\": [");
        let k: usize = 0;
        while (k < _.packages.items.len) : (k += 1) {
            const pkg = _.packages.items[k];
            if (k > 0) try out.appendSlice(allocator, ",");
            try out.appendSlice(allocator, "\n    {\n");

            let id_buf: [64]u8 = undefined;
            const id_s = std.fmt.bufPrint(&id_buf, "      \"id\": {d},\n", .{pkg.id}) catch unreachable;
            try out.appendSlice(allocator, id_s);

            try out.appendSlice(allocator, "      \"name\": ");
            try Lockfile.writeJsonStr(&out, allocator, pkg.name);
            try out.appendSlice(allocator, ",\n");

            try out.appendSlice(allocator, "      \"version\": ");
            try Lockfile.writeJsonStr(&out, allocator, pkg.version);
            try out.appendSlice(allocator, ",\n");

            let res_buf: [512]u8 = undefined;
            const res_str = pkg.resolution.format(&res_buf);
            try out.appendSlice(allocator, "      \"resolutionTag\": ");
            try Lockfile.writeJsonStr(&out, allocator, pkg.resolution.tag.asStr());
            try out.appendSlice(allocator, ",\n      \"resolution\": ");
            try Lockfile.writeJsonStr(&out, allocator, res_str);
            try out.appendSlice(allocator, ",\n");

            try out.appendSlice(allocator, "      \"tarballUrl\": ");
            try Lockfile.writeJsonStr(&out, allocator, pkg.meta.tarball_url);
            try out.appendSlice(allocator, ",\n");

            let sri_buf: [256]u8 = undefined;
            const sri_str = pkg.meta.integrity.format(&sri_buf) catch "";
            try out.appendSlice(allocator, "      \"integrity\": ");
            try Lockfile.writeJsonStr(&out, allocator, sri_str);
            try out.appendSlice(allocator, ",\n");

            let meta_buf: [160]u8 = undefined;
            const meta_s = std.fmt.bufPrint(
                &meta_buf,
                "      \"os\": {d}, \"cpu\": {d}, \"hasInstallScript\": {s},\n",
                .{ pkg.meta.os, pkg.meta.cpu, if (pkg.meta.has_install_script) "true" else "false" },
            ) catch unreachable;
            try out.appendSlice(allocator, meta_s);

            // Bin link
            try out.appendSlice(allocator, "      \"binKind\": ");
            try out.appendSlice(allocator, switch (pkg.bin.kind) {
                .None => "\"none\"",
                .File => "\"file\"",
                .NamedFile => "\"named_file\"",
                .Dir => "\"dir\"",
                .Map => "\"map\"",
            });
            if (pkg.bin.kind == .File) {
                try out.appendSlice(allocator, ", \"binPath\": ");
                try Lockfile.writeJsonStr(&out, allocator, pkg.bin.file_path);
            } else if (pkg.bin.kind == .Dir) {
                try out.appendSlice(allocator, ", \"binPath\": ");
                try Lockfile.writeJsonStr(&out, allocator, pkg.bin.dir_path);
            } else if (pkg.bin.kind == .NamedFile || pkg.bin.kind == .Map) {
                try out.appendSlice(allocator, ", \"binEntries\": {");
                let bi: usize = 0;
                while (bi < pkg.bin.entries.items.len) : (bi += 1) {
                    const be = pkg.bin.entries.items[bi];
                    if (bi > 0) try out.appendSlice(allocator, ", ");
                    try Lockfile.writeJsonStr(&out, allocator, be.name);
                    try out.appendSlice(allocator, ": ");
                    try Lockfile.writeJsonStr(&out, allocator, be.target);
                }
                try out.appendSlice(allocator, "}");
            }
            try out.appendSlice(allocator, ",\n");

            // Dependencies
            try out.appendSlice(allocator, "      \"dependencies\": [");
            let di: usize = 0;
            while (di < pkg.dependencies.items.len) : (di += 1) {
                const edge = pkg.dependencies.items[di];
                if (di > 0) try out.appendSlice(allocator, ",");
                try out.appendSlice(allocator, "\n        { \"name\": ");
                try Lockfile.writeJsonStr(&out, allocator, edge.name);
                try out.appendSlice(allocator, ", \"spec\": ");
                try Lockfile.writeJsonStr(&out, allocator, edge.spec);
                let edge_buf: [96]u8 = undefined;
                const edge_s = std.fmt.bufPrint(
                    &edge_buf,
                    ", \"behavior\": {d}, \"resolved\": {d} }}",
                    .{ edge.behavior.toByte(), edge.resolved_pkg_id },
                ) catch unreachable;
                try out.appendSlice(allocator, edge_s);
            }
            if (pkg.dependencies.items.len > 0) try out.appendSlice(allocator, "\n      ");
            try out.appendSlice(allocator, "]\n    }");
        }
        if (_.packages.items.len > 0) try out.appendSlice(allocator, "\n  ");
        try out.appendSlice(allocator, "]\n}\n");
        return out.toOwnedSlice(allocator);
    }

    /// Parses a `cap.lock` JSON string back into a `Lockfile` graph.
    pub static fn fromTextJson(allocator: std.mem.Allocator, text: []const u8): Lockfile! {
        const root = JSON.parse(allocator, text) catch return error.InvalidLockfile;
        if (root.tag != .Object) return error.InvalidLockfile;

        let lf = Lockfile.init();
        errdefer lf.deinit(allocator);

        if (v := root.get("lockfileVersion")) {
            if (n := v.asNumber()) {
                if (n >= 0.0) lf.lockfile_version = @intFromFloat(n);
            }
        }
        if (v := root.get("configVersion")) {
            if (n := v.asNumber()) {
                if (n >= 0.0) lf.config_version = @intFromFloat(n);
            }
        }
        if (ws_arr := root.get("workspaces")) {
            if (ws_arr.tag == .Array) {
                for (elem of ws_arr.arr_val) {
                    if (elem.tag != .Object) continue;
                    const w_name = (elem.get("name")?.asString()) ?? "";
                    const w_path = (elem.get("path")?.asString()) ?? "";
                    const w_ver = (elem.get("version")?.asString()) ?? "";
                    try lf.addWorkspace(allocator, w_name, w_path, w_ver);
                }
            }
        }
        if (ov_obj := root.get("overrides")) {
            if (ov_obj.tag == .Object) {
                for (f of ov_obj.obj_val) {
                    if (spec := f.value.asString()) {
                        try lf.addOverride(allocator, f.key, spec);
                    }
                }
            }
        }
        if (td_arr := root.get("trustedDependencies")) {
            if (td_arr.tag == .Array) {
                for (elem of td_arr.arr_val) {
                    if (s := elem.asString()) try lf.addTrustedDependency(allocator, s);
                }
            }
        }
        if (pd_obj := root.get("patchedDependencies")) {
            if (pd_obj.tag == .Object) {
                for (f of pd_obj.obj_val) {
                    if (f.value.tag == .Object) {
                        const p_path = (f.value.get("path")?.asString()) ?? "";
                        let p_hash: u64 = 0;
                        if (hv := f.value.get("hash")) {
                            if (hn := hv.asNumber()) {
                                if (hn >= 0.0) p_hash = @intFromFloat(hn);
                            }
                        }
                        try lf.addPatchedDependency(allocator, f.key, p_path, p_hash);
                    }
                }
            }
        }

        if (pkgs_arr := root.get("packages")) {
            if (pkgs_arr.tag == .Array) {
                // First pass: create all package nodes
                for (elem of pkgs_arr.arr_val) {
                    if (elem.tag != .Object) continue;
                    const p_name = (elem.get("name")?.asString()) ?? "";
                    const p_ver = (elem.get("version")?.asString()) ?? "";
                    const res_s = (elem.get("resolution")?.asString()) ?? "";
                    const res_tag_s = (elem.get("resolutionTag")?.asString()) ?? "";
                    const tb_url = (elem.get("tarballUrl")?.asString()) ?? "";
                    const integ_s = (elem.get("integrity")?.asString()) ?? "";

                    let res = Resolution.fromTextLockfile(res_s) catch Resolution{};
                    if (rt := ResolutionTag.fromStr(res_tag_s)) {
                        if (rt == .Npm) {
                            res = Resolution.initNpm(p_ver, tb_url);
                        }
                    }

                    let meta = PackageMeta{};
                    meta.tarball_url = tb_url;
                    if (integ_s.len > 0) {
                        meta.integrity = Integrity.parseMultiple(integ_s) catch Integrity{};
                    }
                    if (ov := elem.get("os")) {
                        if (n := ov.asNumber()) {
                            if (n >= 0.0) meta.os = @intFromFloat(n);
                        }
                    }
                    if (cv := elem.get("cpu")) {
                        if (n := cv.asNumber()) {
                            if (n >= 0.0) meta.cpu = @intFromFloat(n);
                        }
                    }
                    if (hv := elem.get("hasInstallScript")) {
                        if (b := hv.asBool()) {
                            meta.has_install_script = b;
                        }
                    }

                    const pkg_id = try lf.addPackage(allocator, p_name, p_ver, res, meta);

                    // Restore bin
                    const b_kind_s = (elem.get("binKind")?.asString()) ?? "none";
                    if (std.mem.eql(u8, b_kind_s, "file")) {
                        const bp = (elem.get("binPath")?.asString()) ?? "";
                        try lf.setPackageBin(allocator, pkg_id, BinLink.initFile(bp));
                    } else if (std.mem.eql(u8, b_kind_s, "dir")) {
                        const bp = (elem.get("binPath")?.asString()) ?? "";
                        try lf.setPackageBin(allocator, pkg_id, BinLink.initDir(bp));
                    } else if (std.mem.eql(u8, b_kind_s, "named_file") || std.mem.eql(u8, b_kind_s, "map")) {
                        let blink = BinLink.initMap();
                        if (std.mem.eql(u8, b_kind_s, "named_file")) blink.kind = .NamedFile;
                        defer blink.deinit(allocator);
                        if (be_obj := elem.get("binEntries")) {
                            if (be_obj.tag == .Object) {
                                for (bf of be_obj.obj_val) {
                                    if (bs := bf.value.asString()) {
                                        try blink.entries.append(allocator, BinEntry{ name: bf.key, target: bs });
                                    }
                                }
                            }
                        }
                        try lf.setPackageBin(allocator, pkg_id, blink);
                    }
                }

                // Second pass: add dependency edges
                let idx: usize = 0;
                for (elem of pkgs_arr.arr_val) {
                    if (elem.tag != .Object) continue;
                    if (idx >= lf.packages.items.len) break;
                    if (deps_arr := elem.get("dependencies")) {
                        if (deps_arr.tag == .Array) {
                            for (de of deps_arr.arr_val) {
                                if (de.tag != .Object) continue;
                                const d_name = (de.get("name")?.asString()) ?? "";
                                const d_spec = (de.get("spec")?.asString()) ?? "";
                                let beh_byte: u8 = Behavior.PROD_BIT;
                                if (bv := de.get("behavior")) {
                                    if (bn := bv.asNumber()) {
                                        if (bn >= 0.0) beh_byte = @intFromFloat(bn);
                                    }
                                }
                                let resolved_id: PackageID = INVALID_PACKAGE_ID;
                                if (rv := de.get("resolved")) {
                                    if (rn := rv.asNumber()) {
                                        if (rn >= 0.0) resolved_id = @intFromFloat(rn);
                                    }
                                }
                                const from_id: PackageID = @intCast(idx);
                                try lf.addDependency(allocator, from_id, d_name, d_spec, Behavior.fromByte(beh_byte), resolved_id);
                            }
                        }
                    }
                    idx += 1;
                }
            }
        }

        lf.computeMetaHash();
        return lf;
    }

    // ------------------------------------------------------------------------
    // Binary Lockfile (`CPLK0001`) Serialization / Parsing
    // ------------------------------------------------------------------------

    fn writeU16(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, val: u16): ! {
        let buf: [2]u8 = undefined;
        zig { std.mem.writeInt(u16, &buf, val, .little); }
        try out.appendSlice(allocator, buf[0..2]);
    }

    fn writeU32(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, val: u32): ! {
        let buf: [4]u8 = undefined;
        zig { std.mem.writeInt(u32, &buf, val, .little); }
        try out.appendSlice(allocator, buf[0..4]);
    }

    fn writeU64(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, val: u64): ! {
        let buf: [8]u8 = undefined;
        zig { std.mem.writeInt(u64, &buf, val, .little); }
        try out.appendSlice(allocator, buf[0..8]);
    }

    fn writeLenStr(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): ! {
        try Lockfile.writeU32(out, allocator, s.len as u32);
        if (s.len > 0) try out.appendSlice(allocator, s);
    }

    fn readU8(data: []const u8, pos: *usize): u8! {
        if (pos.* + 1 > data.len) return error.LockfileCorrupt;
        const v = data[pos.*];
        pos.* += 1;
        return v;
    }

    fn readU16(data: []const u8, pos: *usize): u16! {
        if (pos.* + 2 > data.len) return error.LockfileCorrupt;
        let v: u16 = 0;
        const p = pos.*;
        zig { v = std.mem.readInt(u16, data[p..][0..2], .little); }
        pos.* += 2;
        return v;
    }

    fn readU32(data: []const u8, pos: *usize): u32! {
        if (pos.* + 4 > data.len) return error.LockfileCorrupt;
        let v: u32 = 0;
        const p = pos.*;
        zig { v = std.mem.readInt(u32, data[p..][0..4], .little); }
        pos.* += 4;
        return v;
    }

    fn readU64(data: []const u8, pos: *usize): u64! {
        if (pos.* + 8 > data.len) return error.LockfileCorrupt;
        let v: u64 = 0;
        const p = pos.*;
        zig { v = std.mem.readInt(u64, data[p..][0..8], .little); }
        pos.* += 8;
        return v;
    }

    fn readLenStr(data: []const u8, pos: *usize): ([]const u8)! {
        const slen = try Lockfile.readU32(data, pos);
        const len_sz = slen as usize;
        if (pos.* + len_sz > data.len) return error.LockfileCorrupt;
        const s = data[pos.* .. pos.* + len_sz];
        pos.* += len_sz;
        return s;
    }

    /// Serializes the lockfile to compact binary `CPLK0001` format with CRC32 integrity checksum.
    pub fn toBinaryAlloc(*_, allocator: std.mem.Allocator): ([]u8)! {
        _.computeMetaHash();
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        // 68-byte header:
        // [0..8]   magic "CPLK0001"
        // [8..12]  format_version (u32)
        // [12..16] pkg_count (u32)
        // [16..20] workspace_count (u32)
        // [20..24] override_count (u32)
        // [24..28] trusted_count (u32)
        // [28..32] patched_count (u32)
        // [32..64] meta_hash ([32]u8)
        // [64..68] payload_crc32 (u32, backfilled)
        try out.appendSlice(allocator, BINARY_LOCKFILE_MAGIC);
        try Lockfile.writeU32(&out, allocator, BINARY_LOCKFILE_VERSION);
        try Lockfile.writeU32(&out, allocator, _.packages.items.len as u32);
        try Lockfile.writeU32(&out, allocator, _.workspaces.items.len as u32);
        try Lockfile.writeU32(&out, allocator, _.overrides.items.len as u32);
        try Lockfile.writeU32(&out, allocator, _.trusted_dependencies.items.len as u32);
        try Lockfile.writeU32(&out, allocator, _.patched_dependencies.items.len as u32);
        try out.appendSlice(allocator, _.meta_hash[0..32]);
        try Lockfile.writeU32(&out, allocator, 0);

        // Workspaces
        for (ws of _.workspaces.items) {
            try Lockfile.writeLenStr(&out, allocator, ws.name);
            try Lockfile.writeLenStr(&out, allocator, ws.path);
            try Lockfile.writeLenStr(&out, allocator, ws.version);
        }
        // Overrides
        for (ov of _.overrides.items) {
            try Lockfile.writeLenStr(&out, allocator, ov.name);
            try Lockfile.writeLenStr(&out, allocator, ov.spec);
        }
        // Trusted deps
        for (td of _.trusted_dependencies.items) {
            try Lockfile.writeLenStr(&out, allocator, td);
        }
        // Patched deps
        for (pd of _.patched_dependencies.items) {
            try Lockfile.writeLenStr(&out, allocator, pd.name_and_version);
            try Lockfile.writeLenStr(&out, allocator, pd.patch_path);
            try Lockfile.writeU64(&out, allocator, pd.patchfile_hash);
        }

        // Packages
        for (pkg of _.packages.items) {
            try Lockfile.writeU32(&out, allocator, pkg.id);
            try Lockfile.writeLenStr(&out, allocator, pkg.name);
            try Lockfile.writeLenStr(&out, allocator, pkg.version);
            try out.append(allocator, pkg.resolution.tag as u8);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.version);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.tarball_url);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.path);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.repo.owner);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.repo.repo);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.repo.committish);
            try Lockfile.writeLenStr(&out, allocator, pkg.resolution.repo.resolved);

            try out.append(allocator, pkg.meta.origin as u8);
            try Lockfile.writeU16(&out, allocator, pkg.meta.os);
            try Lockfile.writeU16(&out, allocator, pkg.meta.cpu);
            try out.append(allocator, pkg.meta.libc);
            try out.append(allocator, if (pkg.meta.has_install_script) 1 else 0);
            try out.append(allocator, pkg.meta.integrity.tag as u8);
            const d_len = pkg.meta.integrity.tag.digestLen();
            try out.appendSlice(allocator, pkg.meta.integrity.digest[0..d_len]);
            try Lockfile.writeLenStr(&out, allocator, pkg.meta.tarball_url);

            try out.append(allocator, pkg.bin.kind as u8);
            try Lockfile.writeLenStr(&out, allocator, pkg.bin.file_path);
            try Lockfile.writeLenStr(&out, allocator, pkg.bin.dir_path);
            try Lockfile.writeU32(&out, allocator, pkg.bin.entries.items.len as u32);
            for (be of pkg.bin.entries.items) {
                try Lockfile.writeLenStr(&out, allocator, be.name);
                try Lockfile.writeLenStr(&out, allocator, be.target);
            }

            try Lockfile.writeU32(&out, allocator, pkg.dependencies.items.len as u32);
            for (edge of pkg.dependencies.items) {
                try Lockfile.writeLenStr(&out, allocator, edge.name);
                try Lockfile.writeLenStr(&out, allocator, edge.spec);
                try out.append(allocator, edge.behavior.toByte());
                try Lockfile.writeU32(&out, allocator, edge.resolved_pkg_id);
            }
        }

        const payload_crc = Hash.crc32(out.items[BINARY_LOCKFILE_HEADER_SIZE..]);
        zig {
            std.mem.writeInt(u32, out.items[64..68], payload_crc, .little);
        }
        return out.toOwnedSlice(allocator);
    }

    /// Loads and verifies a `CPLK0001` binary lockfile payload.
    pub static fn fromBinary(allocator: std.mem.Allocator, data: []const u8): Lockfile! {
        if (data.len < BINARY_LOCKFILE_HEADER_SIZE) return error.LockfileCorrupt;
        if (!std.mem.eql(u8, data[0..8], BINARY_LOCKFILE_MAGIC)) return error.InvalidLockfile;

        let pos: usize = 8;
        const fmt_ver = try Lockfile.readU32(data, &pos);
        if (fmt_ver != BINARY_LOCKFILE_VERSION) return error.InvalidLockfile;

        const pkg_count = try Lockfile.readU32(data, &pos);
        const ws_count = try Lockfile.readU32(data, &pos);
        const ov_count = try Lockfile.readU32(data, &pos);
        const td_count = try Lockfile.readU32(data, &pos);
        const pd_count = try Lockfile.readU32(data, &pos);

        let lf = Lockfile.init();
        errdefer lf.deinit(allocator);
        @memcpy(lf.meta_hash[0..32], data[pos .. pos + 32]);
        pos += 32;

        const expected_crc = try Lockfile.readU32(data, &pos);
        const actual_crc = Hash.crc32(data[BINARY_LOCKFILE_HEADER_SIZE..]);
        if (expected_crc != actual_crc) return error.LockfileCorrupt;

        let wi: u32 = 0;
        while (wi < ws_count) : (wi += 1) {
            const w_name = try Lockfile.readLenStr(data, &pos);
            const w_path = try Lockfile.readLenStr(data, &pos);
            const w_ver = try Lockfile.readLenStr(data, &pos);
            try lf.addWorkspace(allocator, w_name, w_path, w_ver);
        }
        let oi: u32 = 0;
        while (oi < ov_count) : (oi += 1) {
            const o_name = try Lockfile.readLenStr(data, &pos);
            const o_spec = try Lockfile.readLenStr(data, &pos);
            try lf.addOverride(allocator, o_name, o_spec);
        }
        let ti: u32 = 0;
        while (ti < td_count) : (ti += 1) {
            const td = try Lockfile.readLenStr(data, &pos);
            try lf.addTrustedDependency(allocator, td);
        }
        let pi: u32 = 0;
        while (pi < pd_count) : (pi += 1) {
            const nv = try Lockfile.readLenStr(data, &pos);
            const pp = try Lockfile.readLenStr(data, &pos);
            const ph = try Lockfile.readU64(data, &pos);
            try lf.addPatchedDependency(allocator, nv, pp, ph);
        }

        let ki: u32 = 0;
        while (ki < pkg_count) : (ki += 1) {
            const raw_id = try Lockfile.readU32(data, &pos);
            const p_name = try Lockfile.readLenStr(data, &pos);
            const p_ver = try Lockfile.readLenStr(data, &pos);
            const res_tag_b = try Lockfile.readU8(data, &pos);
            const res_ver = try Lockfile.readLenStr(data, &pos);
            const res_tb = try Lockfile.readLenStr(data, &pos);
            const res_path = try Lockfile.readLenStr(data, &pos);
            const r_owner = try Lockfile.readLenStr(data, &pos);
            const r_repo = try Lockfile.readLenStr(data, &pos);
            const r_comm = try Lockfile.readLenStr(data, &pos);
            const r_res = try Lockfile.readLenStr(data, &pos);

            let res = Resolution{
                tag: res_tag_b as ResolutionTag,
                version: res_ver,
                tarball_url: res_tb,
                path: res_path,
            };
            res.repo.owner = r_owner;
            res.repo.repo = r_repo;
            res.repo.committish = r_comm;
            res.repo.resolved = r_res;

            const origin_b = try Lockfile.readU8(data, &pos);
            const os_m = try Lockfile.readU16(data, &pos);
            const cpu_m = try Lockfile.readU16(data, &pos);
            const libc_m = try Lockfile.readU8(data, &pos);
            const has_script = (try Lockfile.readU8(data, &pos)) != 0;
            const integ_tag_b = try Lockfile.readU8(data, &pos);

            let integ = Integrity{};
            zig {
                integ.tag = @enumFromInt(integ_tag_b);
            }
            const d_len = integ.tag.digestLen();
            if (pos + d_len > data.len) return error.LockfileCorrupt;
            if (d_len > 0) {
                @memcpy(integ.digest[0..d_len], data[pos .. pos + d_len]);
                pos += d_len;
            }
            const tb_url = try Lockfile.readLenStr(data, &pos);

            let meta = PackageMeta{
                id: raw_id,
                origin: origin_b as Origin,
                os: os_m,
                cpu: cpu_m,
                libc: libc_m,
                has_install_script: has_script,
                integrity: integ,
                tarball_url: tb_url,
            };

            const pkg_id = try lf.addPackage(allocator, p_name, p_ver, res, meta);

            const bin_kind_b = try Lockfile.readU8(data, &pos);
            const bin_fp = try Lockfile.readLenStr(data, &pos);
            const bin_dp = try Lockfile.readLenStr(data, &pos);
            const bin_cnt = try Lockfile.readU32(data, &pos);
            let blink = BinLink{
                kind: bin_kind_b as BinLinkKind,
                file_path: bin_fp,
                dir_path: bin_dp,
            };
            defer blink.deinit(allocator);
            let bi: u32 = 0;
            while (bi < bin_cnt) : (bi += 1) {
                const bn = try Lockfile.readLenStr(data, &pos);
                const bt = try Lockfile.readLenStr(data, &pos);
                try blink.entries.append(allocator, BinEntry{ name: bn, target: bt });
            }
            try lf.setPackageBin(allocator, pkg_id, blink);

            const dep_cnt = try Lockfile.readU32(data, &pos);
            let di: u32 = 0;
            while (di < dep_cnt) : (di += 1) {
                const dn = try Lockfile.readLenStr(data, &pos);
                const ds = try Lockfile.readLenStr(data, &pos);
                const db = try Lockfile.readU8(data, &pos);
                const dr = try Lockfile.readU32(data, &pos);
                try lf.addDependency(allocator, pkg_id, dn, ds, Behavior.fromByte(db), dr);
            }
        }

        return lf;
    }

    pub fn saveTextFile(*_, allocator: std.mem.Allocator, path: []const u8): ! {
        const bytes = try _.toTextJsonAlloc(allocator);
        defer allocator.free(bytes);
        try writeFile(path, bytes, 0o644);
    }

    pub static fn loadTextFile(allocator: std.mem.Allocator, path: []const u8): Lockfile! {
        const bytes = try readFile(allocator, path);
        defer allocator.free(bytes);
        return Lockfile.fromTextJson(allocator, bytes);
    }

    pub fn saveBinaryFile(*_, allocator: std.mem.Allocator, path: []const u8): ! {
        const bytes = try _.toBinaryAlloc(allocator);
        defer allocator.free(bytes);
        try writeFile(path, bytes, 0o644);
    }

    pub static fn loadBinaryFile(allocator: std.mem.Allocator, path: []const u8): Lockfile! {
        const bytes = try readFile(allocator, path);
        defer allocator.free(bytes);
        return Lockfile.fromBinary(allocator, bytes);
    }
}
