// Source: Absorbed from /data/dev/bun/src/install/hoisted_install.cp | Synced: 2026-11-21T16:52:15Z
// ============================================================================
// lib/pkg/linker.cp — Hoisted/Isolated Package Linker, BinLinker & Patch Applicator
// Absorbed from Bun src/install/{hoisted_install,isolated_install,bin,patch_install}.cp
// ============================================================================

const std = @import("std");
import { Hash } from "cap:hash";
import { UnifiedDiff, PatchApplyOptions } from "cap:patch";
import {
    DirEntry,
    DirIterator,
    mkdirAll,
    copyFile,
    link,
    symlink,
    unlink,
    chmod,
    readFile,
    writeFile,
    exists,
} from "cap:sys";
import { PkgError } from "./types.cp";
import { Dependency } from "./dependency.cp";
import { BinLink, BinLinkKind } from "./resolution.cp";
import { TarballExtractor } from "./tarball.cp";

pub enum InstallStrategy: u8 {
    Hoisted = 0,
    Isolated = 1,
}

pub enum CopyMethod: u8 {
    Auto = 0,
    Hardlink = 1,
    Copy = 2,
    Symlink = 3,
}

pub struct LinkStats {
    packages_linked: usize = 0,
    files_linked: usize = 0,
    dirs_created: usize = 0,
    bins_linked: usize = 0,
    patches_applied: usize = 0,
}

/// Links package executables into `<modules_dir>/.bin/<cmd>` and ensures `0o755` permissions.
pub struct BinLinker {
    pub static fn linkSingleBin(
        allocator: std.mem.Allocator,
        bin_dir: []const u8,
        bin_name: []const u8,
        abs_target: []const u8,
        stats: *LinkStats,
    ): ! {
        const clean_name = BinLink.normalizedBinName(bin_name);
        if (clean_name.len == 0 || !exists(abs_target)) return;

        try mkdirAll(bin_dir, 0o755);
        try chmod(abs_target, 0o755);

        const dest_link = try std.fmt.allocPrint(allocator, "{s}/{s}", .{
            std.mem.trimEnd(u8, bin_dir, "/"),
            clean_name,
        });
        defer allocator.free(dest_link);

        unlink(dest_link) catch {};
        try symlink(abs_target, dest_link);
        stats.bins_linked += 1;
    }

    pub static fn linkPackageBins(
        allocator: std.mem.Allocator,
        bin_dir: []const u8,
        pkg_dir: []const u8,
        pkg_name: []const u8,
        bin: BinLink,
        stats: *LinkStats,
    ): ! {
        if (bin.kind == .None) return;
        const clean_pkg_dir = std.mem.trimEnd(u8, pkg_dir, "/");

        switch (bin.kind) {
            .None => {},
            .File => {
                if (bin.file_path.len == 0) return;
                const cmd = Dependency.unscopedPackageName(pkg_name);
                const clean_rel = std.mem.trimStart(u8, bin.file_path, "./");
                const target = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_pkg_dir, clean_rel });
                defer allocator.free(target);
                try BinLinker.linkSingleBin(allocator, bin_dir, cmd, target, stats);
            },
            .NamedFile, .Map => {
                for (entry of bin.entries.items) {
                    const clean_rel = std.mem.trimStart(u8, entry.target, "./");
                    const target = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_pkg_dir, clean_rel });
                    defer allocator.free(target);
                    try BinLinker.linkSingleBin(allocator, bin_dir, entry.name, target, stats);
                }
            },
            .Dir => {
                if (bin.dir_path.len == 0) return;
                const clean_rel = std.mem.trimStart(u8, bin.dir_path, "./");
                const src_bin_dir = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_pkg_dir, clean_rel });
                defer allocator.free(src_bin_dir);
                if (!exists(src_bin_dir)) return;

                let iter = DirIterator.open(src_bin_dir) catch return;
                defer iter.close();
                while (entry := iter.next() catch null) {
                    if (entry.kind == .file || entry.kind == .sym_link) {
                        const target = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ src_bin_dir, entry.name });
                        defer allocator.free(target);
                        try BinLinker.linkSingleBin(allocator, bin_dir, entry.name, target, stats);
                    }
                }
            },
        }
    }
}

/// Industrial-grade module tree linker supporting Hoisted (`node_modules/<pkg>`),
/// Isolated (`.cap/<name>@<ver>/node_modules/<pkg>`), hardlink/copy/symlink modes, and `patch_install`.
pub struct PackageLinker {
    /// Escapes scoped package names for isolated store keys (`@scope/pkg` -> `@scope+pkg`).
    pub static fn escapeStoreNameAlloc(allocator: std.mem.Allocator, pkg_name: []const u8): ([]u8)! {
        let out = try allocator.dupe(u8, pkg_name);
        let i: usize = 0;
        while (i < out.len) : (i += 1) {
            if (out[i] == '/') out[i] = '+';
        }
        return out;
    }

    pub static fn hoistedPathAlloc(allocator: std.mem.Allocator, modules_dir: []const u8, pkg_name: []const u8): ([]u8)! {
        return std.fmt.allocPrint(allocator, "{s}/{s}", .{
            std.mem.trimEnd(u8, modules_dir, "/"),
            pkg_name,
        });
    }

    pub static fn isolatedStorePkgPathAlloc(
        allocator: std.mem.Allocator,
        modules_dir: []const u8,
        pkg_name: []const u8,
        version: []const u8,
    ): ([]u8)! {
        const escaped = try PackageLinker.escapeStoreNameAlloc(allocator, pkg_name);
        defer allocator.free(escaped);
        return std.fmt.allocPrint(allocator, "{s}/.cap/{s}@{s}/node_modules/{s}", .{
            std.mem.trimEnd(u8, modules_dir, "/"),
            escaped,
            version,
            pkg_name,
        });
    }

    pub static fn isolatedStoreModulesDirAlloc(
        allocator: std.mem.Allocator,
        modules_dir: []const u8,
        pkg_name: []const u8,
        version: []const u8,
    ): ([]u8)! {
        const escaped = try PackageLinker.escapeStoreNameAlloc(allocator, pkg_name);
        defer allocator.free(escaped);
        return std.fmt.allocPrint(allocator, "{s}/.cap/{s}@{s}/node_modules", .{
            std.mem.trimEnd(u8, modules_dir, "/"),
            escaped,
            version,
        });
    }

    /// Recursively materializes `src_dir` into `dest_dir` via hardlinks (with copy fallback), copy, or symlink.
    pub static fn linkDirectory(
        allocator: std.mem.Allocator,
        src_dir: []const u8,
        dest_dir: []const u8,
        method: CopyMethod,
        stats: *LinkStats,
    ): ! {
        const clean_src = std.mem.trimEnd(u8, src_dir, "/");
        const clean_dest = std.mem.trimEnd(u8, dest_dir, "/");

        if (method == .Symlink) {
            if (last_slash := std.mem.lastIndexOfScalar(u8, clean_dest, '/')) {
                try mkdirAll(clean_dest[0..last_slash], 0o755);
            }
            unlink(clean_dest) catch {};
            try symlink(clean_src, clean_dest);
            stats.files_linked += 1;
            return;
        }

        try mkdirAll(clean_dest, 0o755);
        stats.dirs_created += 1;

        let iter = try DirIterator.open(clean_src);
        defer iter.close();

        while (entry := try iter.next()) {
            const src_item = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_src, entry.name });
            defer allocator.free(src_item);
            const dst_item = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_dest, entry.name });
            defer allocator.free(dst_item);

            if (entry.kind == .directory) {
                try PackageLinker.linkDirectory(allocator, src_item, dst_item, method, stats);
            } else if (entry.kind == .file) {
                if (method == .Hardlink || method == .Auto) {
                    unlink(dst_item) catch {};
                    link(src_item, dst_item) catch {
                        try copyFile(src_item, dst_item);
                    };
                } else {
                    try copyFile(src_item, dst_item);
                }
                stats.files_linked += 1;
            }
        }
    }

    /// Creates a symlink from `link_path` -> `target_path` (creating parent dirs for `@scope/pkg`).
    pub static fn linkPackageSymlink(target_path: []const u8, link_path: []const u8): ! {
        if (last_slash := std.mem.lastIndexOfScalar(u8, link_path, '/')) {
            try mkdirAll(link_path[0..last_slash], 0o755);
        }
        unlink(link_path) catch {};
        try symlink(target_path, link_path);
    }

    /// Links a cached package directory into `modules_dir` using `Hoisted` or `Isolated` strategy.
    /// Returns the installed package directory path (caller owns memory).
    pub static fn linkPackage(
        allocator: std.mem.Allocator,
        modules_dir: []const u8,
        cache_pkg_dir: []const u8,
        pkg_name: []const u8,
        pkg_version: []const u8,
        strategy: InstallStrategy,
        method: CopyMethod,
        is_direct_dep: bool,
        stats: *LinkStats,
    ): ([]u8)! {
        if (strategy == .Hoisted) {
            const dest = try PackageLinker.hoistedPathAlloc(allocator, modules_dir, pkg_name);
            errdefer allocator.free(dest);
            try PackageLinker.linkDirectory(allocator, cache_pkg_dir, dest, method, stats);
            stats.packages_linked += 1;
            return dest;
        }

        // Isolated strategy (pnpm / Bun isolated):
        // 1. Materialize into `<modules_dir>/.cap/<escaped>@<ver>/node_modules/<pkg_name>`
        // 2. If direct root dependency, symlink `<modules_dir>/<pkg_name>` -> store package dir
        const store_pkg = try PackageLinker.isolatedStorePkgPathAlloc(allocator, modules_dir, pkg_name, pkg_version);
        errdefer allocator.free(store_pkg);
        try PackageLinker.linkDirectory(allocator, cache_pkg_dir, store_pkg, method, stats);

        if (is_direct_dep) {
            const top_link = try PackageLinker.hoistedPathAlloc(allocator, modules_dir, pkg_name);
            defer allocator.free(top_link);
            try PackageLinker.linkPackageSymlink(store_pkg, top_link);
        }

        stats.packages_linked += 1;
        return store_pkg;
    }

    /// Applies a unified diff patch string onto `pkg_dir` using `cap:patch` and writes a patch hash tag.
    pub static fn applyPatchToDir(
        allocator: std.mem.Allocator,
        pkg_dir: []const u8,
        patch_text: []const u8,
        stats: *LinkStats,
    ): ! {
        let doc = UnifiedDiff.parse(allocator, patch_text) catch return error.PatchFailed;
        defer doc.deinit(allocator);
        if (doc.files.items.len == 0) return error.PatchFailed;

        const clean_dir = std.mem.trimEnd(u8, pkg_dir, "/");
        const patch_hash = Hash.wyhash(patch_text, 0);
        const tag_path = try std.fmt.allocPrint(allocator, "{s}/.cap-patch-{x}", .{ clean_dir, patch_hash });
        defer allocator.free(tag_path);
        if (exists(tag_path)) {
            return;
        }

        for (file_entry of doc.files.items) {
            const rel_target = file_entry.targetPath();
            if (!TarballExtractor.isSafeArchivePath(rel_target)) {
                return error.PathTraversalDetected;
            }
            const abs_file = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ clean_dir, rel_target });
            defer allocator.free(abs_file);

            if (file_entry.is_deleted_file) {
                unlink(abs_file) catch {};
                continue;
            }

            let orig_text: []const u8 = "";
            let owned_orig: ([]u8)? = null;
            if (!file_entry.is_new_file && exists(abs_file)) {
                const raw = try readFile(allocator, abs_file);
                owned_orig = raw;
                orig_text = raw;
            }
            defer {
                if (o := owned_orig) allocator.free(o);
            }

            const apply_res = UnifiedDiff.applyFilePatch(allocator, orig_text, file_entry, PatchApplyOptions{}) catch return error.PatchFailed;
            defer allocator.free(apply_res.text);
            if (!apply_res.ok) return error.PatchFailed;

            if (last_slash := std.mem.lastIndexOfScalar(u8, abs_file, '/')) {
                try mkdirAll(abs_file[0..last_slash], 0o755);
            }
            // Break hardlink before writing so the shared package cache is never mutated in place.
            unlink(abs_file) catch {};
            try writeFile(abs_file, apply_res.text, 0o644);
        }

        const hash_txt = try std.fmt.allocPrint(allocator, "{x}", .{patch_hash});
        defer allocator.free(hash_txt);
        try writeFile(tag_path, hash_txt, 0o644);

        stats.patches_applied += 1;
    }
}
