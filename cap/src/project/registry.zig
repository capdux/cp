const std = @import("std");
const main_mod = @import("../main.zig");
const semver = @import("semver.zig");
const archive = @import("archive.zig");
const native = @import("native.zig");

pub const DEFAULT_REGISTRY = "https://registry.capdux.dev";

pub const ResolvedArtifact = struct {
    tarball: []const u8,
    integrity: ?[]const u8 = null,

    pub fn deinit(self: *ResolvedArtifact, allocator: std.mem.Allocator) void {
        allocator.free(self.tarball);
        if (self.integrity) |i| allocator.free(i);
    }
};

pub const ResolvedPackageVersion = struct {
    version: []const u8,
    main_pkg: ResolvedArtifact,
    binary_pkg: ?ResolvedArtifact = null,

    pub fn deinit(self: *ResolvedPackageVersion, allocator: std.mem.Allocator) void {
        allocator.free(self.version);
        self.main_pkg.deinit(allocator);
        if (self.binary_pkg) |*b| b.deinit(allocator);
    }
};

pub const FetchInstallResult = struct {
    version: []const u8,
    resolved_url: []const u8,
    integrity: []const u8,
    has_prebuilt_binary: bool,

    pub fn deinit(self: *FetchInstallResult, allocator: std.mem.Allocator) void {
        allocator.free(self.version);
        allocator.free(self.resolved_url);
        allocator.free(self.integrity);
    }
};

/// Returns true if an explicit remote registry is configured via CAP_REGISTRY env or cap.json "registry".
pub fn hasExplicitRemoteRegistry(manifest_registry: ?[]const u8) bool {
    if (main_mod.global_env.get("CAP_REGISTRY")) |env_reg| {
        if (env_reg.len > 0) return true;
    }
    if (manifest_registry) |mr| {
        if (mr.len > 0) return true;
    }
    return false;
}

/// Resolves registry base URL with precedence:
/// CAP_REGISTRY env > cap.json "registry" > DEFAULT_REGISTRY ("https://registry.capdux.dev").
pub fn resolveRegistryUrl(allocator: std.mem.Allocator, manifest_registry: ?[]const u8) ![]const u8 {
    if (main_mod.global_env.get("CAP_REGISTRY")) |env_reg| {
        if (env_reg.len > 0) {
            return try allocator.dupe(u8, std.mem.trimEnd(u8, env_reg, "/"));
        }
    }
    if (manifest_registry) |mr| {
        if (mr.len > 0) {
            return try allocator.dupe(u8, std.mem.trimEnd(u8, mr, "/"));
        }
    }
    return try allocator.dupe(u8, DEFAULT_REGISTRY);
}

/// Percent-encodes a package name for registry URLs (e.g. "@cap/picohttp" -> "@cap%2Fpicohttp").
pub fn encodePkgName(allocator: std.mem.Allocator, pkg_name: []const u8) ![]const u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    for (pkg_name) |c| {
        if (c == '/') {
            try out.appendSlice(allocator, "%2F");
        } else if (c == ' ') {
            try out.appendSlice(allocator, "%20");
        } else {
            try out.append(allocator, c);
        }
    }
    return try out.toOwnedSlice(allocator);
}

fn resolveTarballUrl(allocator: std.mem.Allocator, registry_url: []const u8, raw_url: []const u8) ![]const u8 {
    if (std.mem.startsWith(u8, raw_url, "http://") or std.mem.startsWith(u8, raw_url, "https://")) {
        return try allocator.dupe(u8, raw_url);
    }
    const base = std.mem.trimEnd(u8, registry_url, "/");
    const rel = std.mem.trimStart(u8, raw_url, "/");
    return try std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, rel });
}

/// Performs an HTTP(S) GET request via Zig 0.17 std.http.Client and returns owned response bytes.
pub fn httpGetBytes(allocator: std.mem.Allocator, url: []const u8) ![]u8 {
    var client: std.http.Client = .{
        .allocator = allocator,
        .io = main_mod.global_io,
    };
    defer client.deinit();

    var body_writer: std.Io.Writer.Allocating = .init(allocator);
    defer body_writer.deinit();

    const res = client.fetch(.{
        .location = .{ .url = url },
        .method = .GET,
        .response_writer = &body_writer.writer,
    }) catch |err| {
        std.debug.print("Error: HTTP GET failed for {s}: {s}\n", .{ url, @errorName(err) });
        return error.HttpRequestFailed;
    };

    if (res.status == .not_found) {
        return error.PackageNotFound;
    }
    if (res.status != .ok) {
        std.debug.print("Error: HTTP {d} for {s}\n", .{ @backingInt(res.status), url });
        return error.HttpRequestFailed;
    }

    return try body_writer.toOwnedSlice();
}

fn parseArtifactFromVersionObj(
    allocator: std.mem.Allocator,
    ver_obj: std.json.ObjectMap,
) !ResolvedArtifact {
    // CP native format: { "tarball": "...", "integrity": "sha256-..." }
    if (ver_obj.get("tarball")) |tb| {
        if (tb == .string and tb.string.len > 0) {
            var integ: ?[]const u8 = null;
            if (ver_obj.get("integrity")) |iv| {
                if (iv == .string and iv.string.len > 0) {
                    integ = try allocator.dupe(u8, iv.string);
                }
            }
            return ResolvedArtifact{
                .tarball = try allocator.dupe(u8, tb.string),
                .integrity = integ,
            };
        }
    }
    // npm-compatible format: { "dist": { "tarball": "...", "integrity": "..." } }
    if (ver_obj.get("dist")) |dist_val| {
        if (dist_val == .object) {
            if (dist_val.object.get("tarball")) |tb| {
                if (tb == .string and tb.string.len > 0) {
                    var integ: ?[]const u8 = null;
                    if (dist_val.object.get("integrity")) |iv| {
                        if (iv == .string and iv.string.len > 0) {
                            integ = try allocator.dupe(u8, iv.string);
                        }
                    }
                    return ResolvedArtifact{
                        .tarball = try allocator.dupe(u8, tb.string),
                        .integrity = integ,
                    };
                }
            }
        }
    }
    return error.InvalidRegistryMetadata;
}

/// Fetches package metadata JSON from <registry>/<encoded_pkg> and resolves the best matching version.
pub fn resolvePackageMeta(
    allocator: std.mem.Allocator,
    registry_url: []const u8,
    pkg_name: []const u8,
    ver_spec: []const u8,
    target_key: []const u8,
) !ResolvedPackageVersion {
    const encoded = try encodePkgName(allocator, pkg_name);
    defer allocator.free(encoded);

    const base = std.mem.trimEnd(u8, registry_url, "/");
    const meta_url = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, encoded });
    defer allocator.free(meta_url);

    const meta_bytes = try httpGetBytes(allocator, meta_url);
    defer allocator.free(meta_bytes);

    var parsed = std.json.parseFromSlice(std.json.Value, allocator, meta_bytes, .{}) catch {
        return error.InvalidRegistryMetadata;
    };
    defer parsed.deinit();

    if (parsed.value != .object) return error.InvalidRegistryMetadata;
    const root = parsed.value.object;

    // Support direct single-version manifest (e.g. pack-manifest.json) as fallback if "versions" absent
    const versions_val = root.get("versions") orelse {
        const ver_str = blk: {
            if (root.get("ver") orelse root.get("version")) |v| {
                if (v == .string and v.string.len > 0) break :blk v.string;
            }
            return error.InvalidRegistryMetadata;
        };
        const main_art = try parseArtifactFromVersionObj(allocator, root);
        errdefer {
            var m = main_art;
            m.deinit(allocator);
        }
        var bin_art: ?ResolvedArtifact = null;
        if (root.get("binaries")) |bins| {
            if (bins == .object) {
                if (bins.object.get(target_key)) |bval| {
                    if (bval == .object) {
                        bin_art = parseArtifactFromVersionObj(allocator, bval.object) catch null;
                    }
                }
            }
        }
        return ResolvedPackageVersion{
            .version = try allocator.dupe(u8, ver_str),
            .main_pkg = main_art,
            .binary_pkg = bin_art,
        };
    };

    if (versions_val != .object) return error.InvalidRegistryMetadata;
    const versions_map = versions_val.object;

    var chosen_ver_str: ?[]const u8 = null;

    // 1. "latest" or "*" -> check dist-tags.latest first, else highest semver
    if (std.mem.eql(u8, ver_spec, "latest") or std.mem.eql(u8, ver_spec, "*")) {
        if (root.get("dist-tags")) |dt| {
            if (dt == .object) {
                if (dt.object.get("latest")) |lv| {
                    if (lv == .string and versions_map.contains(lv.string)) {
                        chosen_ver_str = lv.string;
                    }
                }
            }
        }
        if (chosen_ver_str == null) {
            var best_v: ?semver.Version = null;
            var it = versions_map.iterator();
            while (it.next()) |e| {
                const v = semver.Version.parse(e.key_ptr.*) catch continue;
                if (best_v == null or v.compare(best_v.?) > 0) {
                    best_v = v;
                    chosen_ver_str = e.key_ptr.*;
                }
            }
        }
    } else if (versions_map.contains(ver_spec)) {
        // 2. Exact version key match
        chosen_ver_str = ver_spec;
    } else if (root.get("dist-tags")) |dt| {
        if (dt == .object) {
            if (dt.object.get(ver_spec)) |tv| {
                if (tv == .string and versions_map.contains(tv.string)) {
                    chosen_ver_str = tv.string;
                }
            }
        }
    }

    // 3. Semver range matching (e.g. ^1.0.0, ~1.2.0)
    if (chosen_ver_str == null) {
        const range = semver.VersionRange.parse(ver_spec) catch return error.PackageNotFound;
        var best_v: ?semver.Version = null;
        var it = versions_map.iterator();
        while (it.next()) |e| {
            const v = semver.Version.parse(e.key_ptr.*) catch continue;
            if (range.satisfies(v)) {
                if (best_v == null or v.compare(best_v.?) > 0) {
                    best_v = v;
                    chosen_ver_str = e.key_ptr.*;
                }
            }
        }
    }

    const ver_key = chosen_ver_str orelse return error.PackageNotFound;
    const ver_entry = versions_map.get(ver_key) orelse return error.PackageNotFound;
    if (ver_entry != .object) return error.InvalidRegistryMetadata;

    const main_art = try parseArtifactFromVersionObj(allocator, ver_entry.object);
    errdefer {
        var m = main_art;
        m.deinit(allocator);
    }

    var bin_art: ?ResolvedArtifact = null;
    if (ver_entry.object.get("binaries")) |bins| {
        if (bins == .object) {
            if (bins.object.get(target_key)) |bval| {
                if (bval == .object) {
                    bin_art = parseArtifactFromVersionObj(allocator, bval.object) catch null;
                }
            }
        }
    }

    return ResolvedPackageVersion{
        .version = try allocator.dupe(u8, ver_key),
        .main_pkg = main_art,
        .binary_pkg = bin_art,
    };
}

fn cleanupDir(path: []const u8) void {
    if (path.len == 0) return;
    if (path[0] == '/') {
        var root_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{}) catch return;
        defer root_dir.close(main_mod.global_io);
        root_dir.deleteTree(main_mod.global_io, path[1..]) catch {};
    } else {
        std.Io.Dir.deleteTree(.cwd(), main_mod.global_io, path) catch {};
    }
}

/// Downloads, SHA-256 verifies, and extracts a resolved package (main + optional platform binary)
/// into `cache_pkg_dir`. Cleans up `cache_pkg_dir` on any verification or extraction failure.
pub fn downloadAndExtractResolved(
    allocator: std.mem.Allocator,
    registry_url: []const u8,
    pkg_name: []const u8,
    resolved: *const ResolvedPackageVersion,
    cache_pkg_dir: []const u8,
    target_key: []const u8,
) !FetchInstallResult {
    const main_url = try resolveTarballUrl(allocator, registry_url, resolved.main_pkg.tarball);
    errdefer allocator.free(main_url);

    std.debug.print("  ↓ Downloading {s}@{s} from {s}\n", .{ pkg_name, resolved.version, main_url });
    const main_bytes = try httpGetBytes(allocator, main_url);
    defer allocator.free(main_bytes);

    // Verify integrity before creating/writing destination directory
    const actual_integrity = try archive.computeSha256Hex(allocator, main_bytes);
    errdefer allocator.free(actual_integrity);

    if (resolved.main_pkg.integrity) |expected| {
        if (expected.len > 0 and !std.mem.eql(u8, actual_integrity, expected)) {
            std.debug.print("Error: IntegrityMismatch for {s}@{s} (expected {s}, got {s})\n", .{
                pkg_name,
                resolved.version,
                expected,
                actual_integrity,
            });
            return error.IntegrityMismatch;
        }
    }

    // Extract main package; remove cache_pkg_dir if anything fails afterwards
    errdefer cleanupDir(cache_pkg_dir);
    try native.ensureDirPath(cache_pkg_dir);
    try archive.extractTarZstWithVerify(allocator, main_bytes, cache_pkg_dir, resolved.main_pkg.integrity);

    var has_prebuilt = false;
    if (resolved.binary_pkg) |bin_art| {
        const bin_url = try resolveTarballUrl(allocator, registry_url, bin_art.tarball);
        defer allocator.free(bin_url);

        std.debug.print("  ↓ Downloading prebuilt [{s}] from {s}\n", .{ target_key, bin_url });
        const bin_bytes = try httpGetBytes(allocator, bin_url);
        defer allocator.free(bin_bytes);

        try archive.extractTarZstWithVerify(allocator, bin_bytes, cache_pkg_dir, bin_art.integrity);
        has_prebuilt = true;
    }

    return FetchInstallResult{
        .version = try allocator.dupe(u8, resolved.version),
        .resolved_url = main_url,
        .integrity = actual_integrity,
        .has_prebuilt_binary = has_prebuilt,
    };
}

/// One-shot remote registry fetch + SHA-256 verify + extract into `cache_pkg_dir`.
pub fn fetchAndInstallFromRegistry(
    allocator: std.mem.Allocator,
    registry_url: []const u8,
    pkg_name: []const u8,
    ver_spec: []const u8,
    cache_pkg_dir: []const u8,
    target_key: []const u8,
) !FetchInstallResult {
    var resolved = try resolvePackageMeta(allocator, registry_url, pkg_name, ver_spec, target_key);
    defer resolved.deinit(allocator);
    return try downloadAndExtractResolved(allocator, registry_url, pkg_name, &resolved, cache_pkg_dir, target_key);
}
