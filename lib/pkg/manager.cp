// Source: Absorbed from /data/dev/bun/src/install/PackageManager.cp | Synced: 2026-11-21T22:49:22Z
// ============================================================================
// lib/pkg/manager.cp — End-to-End Package Manager Engine
// Absorbed from Bun src/install/PackageManager.cp
// ============================================================================

const std = @import("std");
import { JSON, JsonValue } from "cap:json";
import { PackageJson } from "../resolver/package_json.cp";
import { Integrity } from "../resolver/integrity.cp";
import { readFile, exists, mkdirAll } from "cap:sys";
import {
    PackageID,
    INVALID_PACKAGE_ID,
    Behavior,
    InstallFeatures,
    PkgError,
} from "./types.cp";
import { Dependency, DependencyList, DependencyVersionTag } from "./dependency.cp";
import { Resolution, PackageMeta, Origin, BinLink, BinEntry } from "./resolution.cp";
import { RegistryConfig, NpmPackageManifest, NpmVersionEntry } from "./npm.cp";
import { Lockfile } from "./lockfile.cp";
import { TarballExtractor, TarballExtractOptions } from "./tarball.cp";
import { InstallStrategy, CopyMethod, LinkStats, PackageLinker, BinLinker } from "./linker.cp";

pub struct PackageManagerConfig {
    project_dir: []const u8 = ".",
    cache_dir: []const u8 = ".cap/cache",
    modules_dir_name: []const u8 = "node_modules",
    lockfile_name: []const u8 = "cap.lock",
    binary_lockfile_name: []const u8 = "cap.lockb",
    strategy: InstallStrategy = .Hoisted,
    copy_method: CopyMethod = .Auto,
    verify_integrity: bool = true,
    save_text_lockfile: bool = true,
    save_binary_lockfile: bool = false,
    features: InstallFeatures = InstallFeatures{},
}

pub struct TarballSourceEntry {
    name: []const u8 = "",
    version: []const u8 = "",
    tgz_bytes: []const u8 = "",
    integrity: Integrity = Integrity{},
    bin: BinLink = BinLink{},
    dependencies: std.ArrayListUnmanaged(Dependency) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.bin.deinit(allocator);
        _.dependencies.deinit(allocator);
    }
}

pub struct PackumentEntry {
    name: []const u8 = "",
    manifest: NpmPackageManifest = NpmPackageManifest{},
}

pub struct PatchSourceEntry {
    name_and_version: []const u8 = "",
    patch_path: []const u8 = "",
    patch_text: []const u8 = "",
}

pub struct InstallPlanItem {
    name: []const u8 = "",
    version: []const u8 = "",
    requested_spec: []const u8 = "",
    resolution: Resolution = Resolution{},
    integrity: Integrity = Integrity{},
    cache_hit: bool = false,
    needs_extract: bool = false,
    is_direct: bool = false,
    behavior: Behavior = Behavior{},
}

pub struct InstallPlan {
    items: std.ArrayListUnmanaged(InstallPlanItem) = .empty,
    total_dependencies: usize = 0,
    cache_hits: usize = 0,
    extracts_needed: usize = 0,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.items.deinit(allocator);
    }
}

pub struct InstallReport {
    strategy: InstallStrategy = .Hoisted,
    installed_packages: usize = 0,
    cache_hits: usize = 0,
    tarballs_extracted: usize = 0,
    files_linked: usize = 0,
    bins_linked: usize = 0,
    patches_applied: usize = 0,
    lockfile_written: bool = false,
    lockfile_packages: usize = 0,
    has_cycle: bool = false,
}

/// Complete offline/local/cache-backed package manager orchestrating:
/// package.json -> DependencyList -> Registry/Packument/Semver -> Lockfile -> Tarball SRI Extract -> Linker & BinLinker -> Lockfile Save.
pub struct PackageManager {
    allocator: std.mem.Allocator,
    config: PackageManagerConfig,
    registry_config: RegistryConfig = RegistryConfig.init(),
    packuments: std.ArrayListUnmanaged(PackumentEntry) = .empty,
    tarball_sources: std.ArrayListUnmanaged(TarballSourceEntry) = .empty,
    patch_sources: std.ArrayListUnmanaged(PatchSourceEntry) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(allocator: std.mem.Allocator, config: PackageManagerConfig): PackageManager {
        return PackageManager{
            allocator: allocator,
            config: config,
        };
    }

    pub fn deinit(*_) {
        _.registry_config.deinit(_.allocator);
        for (*p of _.packuments.items) {
            p.manifest.deinit(_.allocator);
        }
        _.packuments.deinit(_.allocator);
        for (*tb of _.tarball_sources.items) {
            tb.deinit(_.allocator);
        }
        _.tarball_sources.deinit(_.allocator);
        _.patch_sources.deinit(_.allocator);
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
    }

    fn internStr(*_, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try _.allocator.dupe(u8, s);
        errdefer _.allocator.free(copy);
        try _.owned_strings.append(_.allocator, copy);
        return copy;
    }

    pub fn loadNpmrc(*_, npmrc_text: []const u8): ! {
        const owned = try _.internStr(npmrc_text);
        try _.registry_config.parseNpmrc(_.allocator, owned);
    }

    pub fn registerPackument(*_, pkg_name: []const u8, json_bytes: []const u8): ! {
        const owned_json = try _.internStr(json_bytes);
        const manifest = try NpmPackageManifest.parsePackument(_.allocator, owned_json);
        const owned_name = try _.internStr(pkg_name);
        try _.packuments.append(_.allocator, PackumentEntry{
            name: owned_name,
            manifest: manifest,
        });
    }

    pub fn registerTarballBytes(
        *_,
        name: []const u8,
        version: []const u8,
        tgz_bytes: []const u8,
        integrity: Integrity,
        bin: BinLink,
    ): ! {
        const owned_name = try _.internStr(name);
        const owned_ver = try _.internStr(version);
        const owned_bytes = try _.internStr(tgz_bytes);

        let entry = TarballSourceEntry{
            name: owned_name,
            version: owned_ver,
            tgz_bytes: owned_bytes,
            integrity: integrity,
            bin: BinLink{
                kind: bin.kind,
                file_path: try _.internStr(bin.file_path),
                dir_path: try _.internStr(bin.dir_path),
            },
        };
        errdefer entry.deinit(_.allocator);
        for (be of bin.entries.items) {
            try entry.bin.entries.append(_.allocator, BinEntry{
                name: try _.internStr(be.name),
                target: try _.internStr(be.target),
            });
        }
        try _.tarball_sources.append(_.allocator, entry);
    }

    pub fn registerPatch(*_, name_and_version: []const u8, patch_path: []const u8, patch_text: []const u8): ! {
        try _.patch_sources.append(_.allocator, PatchSourceEntry{
            name_and_version: try _.internStr(name_and_version),
            patch_path: try _.internStr(patch_path),
            patch_text: try _.internStr(patch_text),
        });
    }

    fn findPackumentIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.packuments.items.len) : (i += 1) {
            const p = _.packuments.items[i];
            if (std.mem.eql(u8, p.name, name) || std.mem.eql(u8, p.manifest.name, name)) {
                return i;
            }
        }
        return null;
    }

    fn findTarballSource(name: []const u8, version: []const u8): TarballSourceEntry? {
        for (tb of _.tarball_sources.items) {
            if (std.mem.eql(u8, tb.name, name) && (version.len == 0 || std.mem.eql(u8, tb.version, version))) {
                return tb;
            }
        }
        return null;
    }

    fn findPatch(name: []const u8, version: []const u8): PatchSourceEntry? {
        for (ps of _.patch_sources.items) {
            if (ps.name_and_version.len > name.len + 1 &&
                std.mem.startsWith(u8, ps.name_and_version, name) &&
                ps.name_and_version[name.len] == '@' &&
                std.mem.eql(u8, ps.name_and_version[name.len + 1 ..], version))
            {
                return ps;
            }
        }
        return null;
    }

    fn cachePkgPathAlloc(pkg_name: []const u8, version: []const u8): ([]u8)! {
        const escaped = try PackageLinker.escapeStoreNameAlloc(_.allocator, pkg_name);
        defer _.allocator.free(escaped);
        return std.fmt.allocPrint(_.allocator, "{s}/{s}@{s}", .{
            std.mem.trimEnd(u8, _.config.cache_dir, "/"),
            escaped,
            version,
        });
    }

    fn isPackageCached(pkg_name: []const u8, version: []const u8): bool! {
        const cache_path = try _.cachePkgPathAlloc(pkg_name, version);
        defer _.allocator.free(cache_path);
        const marker = try std.fmt.allocPrint(_.allocator, "{s}/package.json", .{cache_path});
        defer _.allocator.free(marker);
        return exists(cache_path) && exists(marker);
    }

    /// Reads `<project_dir>/package.json` using `cap:sys` + `cap:resolver` `PackageJson.parse`.
    pub fn readRootPackageJson(*_): PackageJson! {
        const path = try std.fmt.allocPrint(_.allocator, "{s}/package.json", .{
            std.mem.trimEnd(u8, _.config.project_dir, "/"),
        });
        defer _.allocator.free(path);
        const bytes = try readFile(_.allocator, path);
        const owned = try _.internStr(bytes);
        _.allocator.free(bytes);
        return PackageJson.parse(_.allocator, owned);
    }

    fn resolveDepToVersion(
        *_,
        dep: Dependency,
        existing_lf: (*Lockfile)?,
    ): NpmVersionEntry! {
        const target_name = dep.realName();
        if (p_idx := _.findPackumentIndex(target_name)) {
            const manifest = _.packuments.items[p_idx].manifest;
            if (dep.version.tag == .Npm) {
                if (v := manifest.resolveVersion(dep.version.npm_range, false)) return v;
            } else if (dep.version.tag == .DistTag) {
                if (v := manifest.findByDistTag(dep.version.dist_tag)) return v;
            }
        }

        // Fallback: check registered tarball source
        for (tb of _.tarball_sources.items) {
            if (std.mem.eql(u8, tb.name, target_name)) {
                if (dep.version.tag != .Npm || dep.version.satisfiesVersion(tb.version)) {
                    return NpmVersionEntry{
                        version_str: tb.version,
                        integrity: tb.integrity,
                        bin: tb.bin,
                    };
                }
            }
        }

        // Fallback: check existing lockfile incremental entry
        if (lf := existing_lf) {
            if (pid := lf.findPackageByName(dep.name)) {
                if (pkg := lf.getPackage(pid)) {
                    if (dep.version.tag != .Npm || dep.version.satisfiesVersion(pkg.version)) {
                        return NpmVersionEntry{
                            version_str: pkg.version,
                            tarball_url: pkg.meta.tarball_url,
                            integrity: pkg.meta.integrity,
                            bin: pkg.bin,
                        };
                    }
                }
            }
        }

        return error.PackageNotFound;
    }

    static fn tryLoadLockfile(allocator: std.mem.Allocator, path: []const u8): Lockfile? {
        const lf = Lockfile.loadTextFile(allocator, path) catch return null;
        return lf;
    }

    static fn tryParseJson(allocator: std.mem.Allocator, bytes: []const u8): JsonValue? {
        const v = JSON.parse(allocator, bytes) catch return null;
        return v;
    }

    /// Plans an installation by reading `package.json`, checking `cap.lock` & cache hits.
    pub fn planInstall(*_): InstallPlan! {
        const pkg_json = try _.readRootPackageJson();
        let deps = DependencyList.init();
        defer deps.deinit(_.allocator);
        try deps.addFromPackageJsonRoot(_.allocator, pkg_json.root, _.config.features);
        deps.sort();

        const lock_path = try std.fmt.allocPrint(_.allocator, "{s}/{s}", .{
            std.mem.trimEnd(u8, _.config.project_dir, "/"),
            _.config.lockfile_name,
        });
        defer _.allocator.free(lock_path);

        let existing_lf: Lockfile? = null;
        if (exists(lock_path)) {
            existing_lf = PackageManager.tryLoadLockfile(_.allocator, lock_path);
        }
        defer {
            if (*lf := existing_lf) lf.deinit(_.allocator);
        }

        let plan = InstallPlan{};
        errdefer plan.deinit(_.allocator);

        for (dep of deps.slice()) {
            const ver_entry = try _.resolveDepToVersion(dep, if (existing_lf != null) &existing_lf.? else null);
            const cached = try _.isPackageCached(dep.name, ver_entry.version_str);
            try plan.items.append(_.allocator, InstallPlanItem{
                name: dep.name,
                version: ver_entry.version_str,
                requested_spec: dep.version.literal,
                resolution: Resolution.initNpm(ver_entry.version_str, ver_entry.tarball_url),
                integrity: ver_entry.integrity,
                cache_hit: cached,
                needs_extract: !cached,
                is_direct: true,
                behavior: dep.behavior,
            });
            plan.total_dependencies += 1;
            if (cached) {
                plan.cache_hits += 1;
            } else {
                plan.extracts_needed += 1;
            }
        }
        return plan;
    }

    fn resolveRecursive(
        *_,
        lf: *Lockfile,
        parent_id: PackageID,
        dep: Dependency,
        existing_lf: (*Lockfile)?,
        is_direct: bool,
        direct_flags: *std.ArrayListUnmanaged(bool),
    ): ! {
        const target_name = dep.realName();
        const ver_entry = try _.resolveDepToVersion(dep, existing_lf);

        if (existing_id := lf.findPackageByNameAndVersion(dep.name, ver_entry.version_str)) {
            try lf.addDependency(_.allocator, parent_id, dep.name, dep.version.literal, dep.behavior, existing_id);
            if (is_direct && (existing_id as usize) < direct_flags.items.len) {
                direct_flags.items[existing_id as usize] = true;
            }
            return;
        }

        const scope = _.registry_config.resolveForPackage(target_name);
        let tb_url = ver_entry.tarball_url;
        if (tb_url.len == 0) {
            const built_url = try scope.tarballUrlAlloc(_.allocator, target_name, ver_entry.version_str);
            tb_url = try _.internStr(built_url);
            _.allocator.free(built_url);
        }

        let meta = PackageMeta{
            origin: .Npm,
            os: ver_entry.os_mask,
            cpu: ver_entry.cpu_mask,
            libc: ver_entry.libc_mask,
            has_install_script: ver_entry.has_install_script,
            integrity: ver_entry.integrity,
            tarball_url: tb_url,
        };
        const res = Resolution.initNpm(ver_entry.version_str, tb_url);
        const pkg_id = try lf.addPackage(_.allocator, dep.name, ver_entry.version_str, res, meta);
        try direct_flags.append(_.allocator, is_direct);

        // Also inspect extracted package.json bin if manifest didn't provide one
        try lf.setPackageBin(_.allocator, pkg_id, ver_entry.bin);
        try lf.addDependency(_.allocator, parent_id, dep.name, dep.version.literal, dep.behavior, pkg_id);

        // Resolve transitive dependencies from packument entry
        for (sub of ver_entry.dependencies.items) {
            const sub_dep = try Dependency.init(sub.name, sub.spec, Behavior.normal());
            try _.resolveRecursive(lf, pkg_id, sub_dep, existing_lf, false, direct_flags);
        }
        if (_.config.features.optional_dependencies) {
            for (sub of ver_entry.optional_dependencies.items) {
                const sub_dep = try Dependency.init(sub.name, sub.spec, Behavior.optionalOnly());
                _.resolveRecursive(lf, pkg_id, sub_dep, existing_lf, false, direct_flags) catch {};
            }
        }
    }

    /// Executes the full end-to-end install pipeline:
    /// 1. Parse root `package.json` & optional existing `cap.lock`
    /// 2. Resolve direct & transitive dependency graph + cycle check
    /// 3. Extract missing tarballs into `cache_dir` with SRI integrity verification
    /// 4. Materialize `node_modules` (`Hoisted` or `Isolated` strategy) + transitive isolated links
    /// 5. Apply `patchedDependencies` via `cap:patch`
    /// 6. Link `.bin` executables with `0o755` permissions
    /// 7. Write `cap.lock` (and optional `cap.lockb`)
    pub fn install(*_): InstallReport! {
        let report = InstallReport{ strategy: _.config.strategy };
        const clean_proj = std.mem.trimEnd(u8, _.config.project_dir, "/");
        const clean_cache = std.mem.trimEnd(u8, _.config.cache_dir, "/");
        const modules_dir = try std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ clean_proj, _.config.modules_dir_name });
        defer _.allocator.free(modules_dir);
        const bin_dir = try std.fmt.allocPrint(_.allocator, "{s}/.bin", .{modules_dir});
        defer _.allocator.free(bin_dir);

        try mkdirAll(clean_cache, 0o755);
        try mkdirAll(modules_dir, 0o755);

        const root_pkg = try _.readRootPackageJson();
        let root_deps = DependencyList.init();
        defer root_deps.deinit(_.allocator);
        try root_deps.addFromPackageJsonRoot(_.allocator, root_pkg.root, _.config.features);
        root_deps.sort();

        // Load optional existing lockfile
        const lock_path = try std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ clean_proj, _.config.lockfile_name });
        defer _.allocator.free(lock_path);
        let existing_lf: Lockfile? = null;
        if (exists(lock_path)) {
            existing_lf = PackageManager.tryLoadLockfile(_.allocator, lock_path);
        }
        defer {
            if (*elf := existing_lf) elf.deinit(_.allocator);
        }

        let lf = Lockfile.init();
        defer lf.deinit(_.allocator);

        let direct_flags: std.ArrayListUnmanaged(bool) = .empty;
        defer direct_flags.deinit(_.allocator);

        const root_name = if (root_pkg.name.len > 0) root_pkg.name else "root";
        const root_ver = if (root_pkg.version.len > 0) root_pkg.version else "0.0.0";
        const root_id = try lf.addPackage(_.allocator, root_name, root_ver, Resolution.initRoot(), PackageMeta{ origin: .Workspace });
        try direct_flags.append(_.allocator, false);

        if (td_val := root_pkg.root.get("trustedDependencies")) {
            if (td_val.tag == .Array) {
                for (elem of td_val.arr_val) {
                    if (s := elem.asString()) try lf.addTrustedDependency(_.allocator, s);
                }
            }
        }

        const existing_ptr: (*Lockfile)? = if (existing_lf != null) &existing_lf.? else null;
        for (dep of root_deps.slice()) {
            try _.resolveRecursive(&lf, root_id, dep, existing_ptr, true, &direct_flags);
        }

        report.has_cycle = try lf.hasCycle(_.allocator);

        let link_stats = LinkStats{};
        let installed_dirs: std.ArrayListUnmanaged(string) = .empty;
        defer installed_dirs.deinit(_.allocator);
        try installed_dirs.append(_.allocator, "");

        // Materialize & link each resolved non-root package (id >= 1)
        let idx: usize = 1;
        while (idx < lf.packages.items.len) : (idx += 1) {
            const pkg = lf.packages.items[idx];
            if (pkg.meta.isDisabledForCurrentPlatform()) {
                try installed_dirs.append(_.allocator, "");
                continue;
            }

            const cache_pkg_dir = try _.cachePkgPathAlloc(pkg.name, pkg.version);
            defer _.allocator.free(cache_pkg_dir);

            const cached = try _.isPackageCached(pkg.name, pkg.version);
            if (cached) {
                report.cache_hits += 1;
            } else {
                const tb_src = _.findTarballSource(pkg.name, pkg.version) ?? return error.PackageNotFound;
                const extract_opts = TarballExtractOptions{
                    strip_components: 1,
                    verify_integrity: _.config.verify_integrity,
                    expected_integrity: if (pkg.meta.integrity.tag.isSupported()) pkg.meta.integrity else tb_src.integrity,
                };
                const estats = try TarballExtractor.extractBytesToDir(_.allocator, tb_src.tgz_bytes, cache_pkg_dir, extract_opts);
                if (!lf.packages.items[idx].meta.integrity.tag.isSupported()) {
                    lf.packages.items[idx].meta.integrity = estats.computed_integrity;
                }
                report.tarballs_extracted += 1;
            }

            const is_direct = if (idx < direct_flags.items.len) direct_flags.items[idx] else false;
            const inst_dir = try PackageLinker.linkPackage(
                _.allocator,
                modules_dir,
                cache_pkg_dir,
                pkg.name,
                pkg.version,
                _.config.strategy,
                _.config.copy_method,
                is_direct,
                &link_stats,
            );
            const owned_inst_dir = try _.internStr(inst_dir);
            _.allocator.free(inst_dir);
            try installed_dirs.append(_.allocator, owned_inst_dir);

            // Apply registered patch if any
            if (ps := _.findPatch(pkg.name, pkg.version)) {
                try PackageLinker.applyPatchToDir(_.allocator, owned_inst_dir, ps.patch_text, &link_stats);
                const nv = try std.fmt.allocPrint(_.allocator, "{s}@{s}", .{ pkg.name, pkg.version });
                defer _.allocator.free(nv);
                try lf.addPatchedDependency(_.allocator, nv, ps.patch_path, 0);
            }

            // Link binaries from manifest bin or extracted package.json
            if (!pkg.bin.isEmpty()) {
                try BinLinker.linkPackageBins(_.allocator, bin_dir, owned_inst_dir, pkg.name, pkg.bin, &link_stats);
            } else {
                const pkg_json_path = try std.fmt.allocPrint(_.allocator, "{s}/package.json", .{owned_inst_dir});
                defer _.allocator.free(pkg_json_path);
                if (exists(pkg_json_path)) {
                    const pj_bytes = try readFile(_.allocator, pkg_json_path);
                    defer _.allocator.free(pj_bytes);
                    if (pj_val := PackageManager.tryParseJson(_.allocator, pj_bytes)) {
                        if (pj_val.tag == .Object) {
                            let dir_bin: JsonValue? = null;
                            if (dirs := pj_val.get("directories")) {
                                if (dirs.tag == .Object) dir_bin = dirs.get("bin");
                            }
                            let extracted_bin = try BinLink.fromJson(_.allocator, pj_val.get("bin"), dir_bin);
                            defer extracted_bin.deinit(_.allocator);
                            if (!extracted_bin.isEmpty()) {
                                try lf.setPackageBin(_.allocator, pkg.id, extracted_bin);
                                try BinLinker.linkPackageBins(_.allocator, bin_dir, owned_inst_dir, pkg.name, extracted_bin, &link_stats);
                            }
                        }
                    }
                }
            }
        }

        // For Isolated strategy: wire transitive dependency symlinks inside each `.cap/<pkg>@<ver>/node_modules/<dep>`
        if (_.config.strategy == .Isolated) {
            let pi: usize = 1;
            while (pi < lf.packages.items.len) : (pi += 1) {
                const pkg = lf.packages.items[pi];
                if (pkg.dependencies.items.len == 0) continue;
                const store_mods = try PackageLinker.isolatedStoreModulesDirAlloc(_.allocator, modules_dir, pkg.name, pkg.version);
                defer _.allocator.free(store_mods);
                for (edge of pkg.dependencies.items) {
                    if (edge.resolved_pkg_id == INVALID_PACKAGE_ID) continue;
                    const tid = edge.resolved_pkg_id as usize;
                    if (tid >= installed_dirs.items.len) continue;
                    const target_dir = installed_dirs.items[tid];
                    if (target_dir.len == 0) continue;
                    const link_dest = try std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ store_mods, edge.name });
                    defer _.allocator.free(link_dest);
                    try PackageLinker.linkPackageSymlink(target_dir, link_dest);
                }
            }
        }

        lf.computeMetaHash();
        if (_.config.save_text_lockfile) {
            try lf.saveTextFile(_.allocator, lock_path);
            report.lockfile_written = true;
        }
        if (_.config.save_binary_lockfile) {
            const bin_lock_path = try std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ clean_proj, _.config.binary_lockfile_name });
            defer _.allocator.free(bin_lock_path);
            try lf.saveBinaryFile(_.allocator, bin_lock_path);
            report.lockfile_written = true;
        }

        report.installed_packages = link_stats.packages_linked;
        report.files_linked = link_stats.files_linked;
        report.bins_linked = link_stats.bins_linked;
        report.patches_applied = link_stats.patches_applied;
        report.lockfile_packages = lf.packageCount();
        return report;
    }
}
