const std = @import("std");
const main_mod = @import("../main.zig");
const semver = @import("semver.zig");
const archive = @import("archive.zig");
const native = @import("native.zig");

pub const DEFAULT_REGISTRY = "https://registry.capdux.org";

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
/// CAP_REGISTRY env > cap.json "registry" > DEFAULT_REGISTRY ("https://registry.capdux.org").
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
    // npm-compatible format: { "dist": { "tarball": "...", "integrity": "sha512-...", "shasum": "<40hex>" } }
    if (ver_obj.get("dist")) |dist_val| {
        if (dist_val == .object) {
            if (dist_val.object.get("tarball")) |tb| {
                if (tb == .string and tb.string.len > 0) {
                    var integ: ?[]const u8 = null;
                    if (dist_val.object.get("integrity")) |iv| {
                        if (iv == .string and iv.string.len > 0) {
                            integ = try allocator.dupe(u8, iv.string);
                        }
                    } else if (dist_val.object.get("shasum")) |sv| {
                        if (sv == .string and sv.string.len > 0) {
                            integ = try allocator.dupe(u8, sv.string);
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

    // Verify integrity before creating/writing destination directory (supports sha256/sha512 SRI & sha1)
    if (resolved.main_pkg.integrity) |expected| {
        if (expected.len > 0) {
            try archive.verifyIntegrity(allocator, main_bytes, expected);
        }
    }
    const actual_integrity = if (resolved.main_pkg.integrity) |exp|
        (if (exp.len > 0) try allocator.dupe(u8, exp) else try archive.computeSha256Hex(allocator, main_bytes))
    else
        try archive.computeSha256Hex(allocator, main_bytes);
    errdefer allocator.free(actual_integrity);

    // Extract main package (auto-detects .tar.zst vs .tgz by magic bytes); remove cache_pkg_dir on failure
    errdefer cleanupDir(cache_pkg_dir);
    try native.ensureDirPath(cache_pkg_dir);
    try archive.extractArchiveAuto(allocator, main_bytes, cache_pkg_dir, resolved.main_pkg.integrity);

    var has_prebuilt = false;
    if (resolved.binary_pkg) |bin_art| {
        const bin_url = try resolveTarballUrl(allocator, registry_url, bin_art.tarball);
        defer allocator.free(bin_url);

        std.debug.print("  ↓ Downloading prebuilt [{s}] from {s}\n", .{ target_key, bin_url });
        const bin_bytes = try httpGetBytes(allocator, bin_url);
        defer allocator.free(bin_bytes);

        try archive.extractArchiveAuto(allocator, bin_bytes, cache_pkg_dir, bin_art.integrity);
        has_prebuilt = true;
    }

    return FetchInstallResult{
        .version = try allocator.dupe(u8, resolved.version),
        .resolved_url = main_url,
        .integrity = actual_integrity,
        .has_prebuilt_binary = has_prebuilt,
    };
}

/// One-shot remote registry fetch + integrity verify + magic-byte auto-extract into `cache_pkg_dir`.
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

pub const NPM_DEFAULT_REGISTRY = "https://registry.npmjs.org";

pub const PublishOptions = struct {
    registry: ?[]const u8 = null,
    access: []const u8 = "public",
    tag: []const u8 = "latest",
    otp: ?[]const u8 = null,
    dry_run: bool = false,
};

/// Resolves publish auth token with precedence:
/// `CAP_TOKEN` > `NPM_TOKEN` > `~/.npmrc` (`//<host>/:_authToken=...` or `_authToken=...`).
pub fn resolveAuthToken(allocator: std.mem.Allocator, registry_url: []const u8) !?[]const u8 {
    if (main_mod.global_env.get("CAP_TOKEN")) |tok| {
        if (tok.len > 0) return try allocator.dupe(u8, tok);
    }
    if (main_mod.global_env.get("NPM_TOKEN")) |tok| {
        if (tok.len > 0) return try allocator.dupe(u8, tok);
    }

    const home = main_mod.global_env.get("HOME") orelse main_mod.global_env.get("USERPROFILE") orelse return null;
    const npmrc_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ home, ".npmrc" });
    defer allocator.free(npmrc_path);

    const content = blk: {
        if (npmrc_path.len > 0 and npmrc_path[0] == '/') {
            var root_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{}) catch return null;
            defer root_dir.close(main_mod.global_io);
            break :blk root_dir.readFileAlloc(main_mod.global_io, npmrc_path[1..], allocator, @fromBackingInt(@intCast(1024 * 1024))) catch return null;
        }
        break :blk std.Io.Dir.readFileAlloc(.cwd(), main_mod.global_io, npmrc_path, allocator, @fromBackingInt(@intCast(1024 * 1024))) catch return null;
    };
    defer allocator.free(content);

    // Strip scheme from registry_url to match `//registry.npmjs.org/:_authToken=...`
    var host_part = registry_url;
    if (std.mem.startsWith(u8, host_part, "https://")) {
        host_part = host_part["https://".len..];
    } else if (std.mem.startsWith(u8, host_part, "http://")) {
        host_part = host_part["http://".len..];
    }
    host_part = std.mem.trimEnd(u8, host_part, "/");

    var fallback_token: ?[]const u8 = null;
    var lines = std.mem.splitScalar(u8, content, '\n');
    while (lines.next()) |raw_line| {
        const line = std.mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0 or line[0] == '#' or line[0] == ';') continue;
        if (std.mem.indexOf(u8, line, "_authToken=")) |pos| {
            const val = std.mem.trim(u8, line[pos + "_authToken=".len ..], " \t\"'");
            if (val.len == 0) continue;
            if (std.mem.indexOf(u8, line, host_part) != null) {
                if (fallback_token) |ft| allocator.free(ft);
                return try allocator.dupe(u8, val);
            }
            if (fallback_token == null and std.mem.startsWith(u8, line, "_authToken=")) {
                fallback_token = try allocator.dupe(u8, val);
            }
        }
    }
    return fallback_token;
}

/// Synthesizes a minimal npm-compatible `package.json` from a parsed `cap.json` manifest
/// so that official `registry.npmjs.org` / Verdaccio accepts the `.tgz` tarball.
pub fn synthesizePackageJson(
    allocator: std.mem.Allocator,
    name: []const u8,
    version: []const u8,
    main_entry: ?[]const u8,
    license: ?[]const u8,
    description: ?[]const u8,
    deps: *const std.StringHashMap([]const u8),
    dev_deps: *const std.StringHashMap([]const u8),
) ![]u8 {
    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const aa = arena.allocator();

    var root = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try root.put(aa, "name", .{ .string = name });
    try root.put(aa, "version", .{ .string = version });
    if (description) |d| {
        if (d.len > 0) try root.put(aa, "description", .{ .string = d });
    }
    if (main_entry) |m| {
        if (m.len > 0) try root.put(aa, "main", .{ .string = m });
    }
    try root.put(aa, "license", .{ .string = license orelse "MIT" });

    if (deps.count() > 0) {
        var dobj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
        var it = deps.iterator();
        while (it.next()) |e| {
            try dobj.put(aa, e.key_ptr.*, .{ .string = e.value_ptr.* });
        }
        try root.put(aa, "dependencies", .{ .object = dobj });
    }
    if (dev_deps.count() > 0) {
        var dobj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
        var it = dev_deps.iterator();
        while (it.next()) |e| {
            try dobj.put(aa, e.key_ptr.*, .{ .string = e.value_ptr.* });
        }
        try root.put(aa, "devDependencies", .{ .object = dobj });
    }

    var out: std.Io.Writer.Allocating = .init(allocator);
    errdefer out.deinit();
    try std.json.Stringify.value(std.json.Value{ .object = root }, .{ .whitespace = .indent_2 }, &out.writer);
    try out.writer.writeAll("\n");
    return try out.toOwnedSlice();
}

/// Builds an npm-compatible `PUT /<encoded_pkg>` JSON publish payload with base64 `_attachments`
/// and uploads it to the target registry.
pub fn publishToRegistry(
    allocator: std.mem.Allocator,
    registry_url: []const u8,
    pkg_name: []const u8,
    version: []const u8,
    pkg_json_bytes: []const u8,
    tgz_res: *const archive.TgzPackResult,
    opts: PublishOptions,
    auth_token: []const u8,
) !void {
    const base = std.mem.trimEnd(u8, registry_url, "/");
    const encoded_name = try encodePkgName(allocator, pkg_name);
    defer allocator.free(encoded_name);

    // npm attachment filename convention: "<name>-<version>.tgz"
    const att_name = try std.fmt.allocPrint(allocator, "{s}-{s}.tgz", .{ pkg_name, version });
    defer allocator.free(att_name);
    const tarball_url = try std.fmt.allocPrint(allocator, "{s}/{s}/-/{s}-{s}.tgz", .{
        base,
        encoded_name,
        blk: {
            if (std.mem.lastIndexOfScalar(u8, pkg_name, '/')) |idx| break :blk pkg_name[idx + 1 ..];
            break :blk pkg_name;
        },
        version,
    });
    defer allocator.free(tarball_url);

    // Base64-encode the .tgz bytes for `_attachments`
    const b64_len = std.base64.standard.Encoder.calcSize(tgz_res.tgz_bytes.len);
    const b64_buf = try allocator.alloc(u8, b64_len);
    defer allocator.free(b64_buf);
    _ = std.base64.standard.Encoder.encode(b64_buf, tgz_res.tgz_bytes);

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const aa = arena.allocator();

    // Parse synthetic package.json to embed inside versions[version]
    const parsed_pkg = try std.json.parseFromSlice(std.json.Value, aa, pkg_json_bytes, .{});
    if (parsed_pkg.value != .object) return error.InvalidRegistryMetadata;
    var ver_obj = parsed_pkg.value.object;

    const ver_id = try std.fmt.allocPrint(aa, "{s}@{s}", .{ pkg_name, version });
    try ver_obj.put(aa, "_id", .{ .string = ver_id });

    var dist_obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try dist_obj.put(aa, "integrity", .{ .string = tgz_res.integrity });
    try dist_obj.put(aa, "shasum", .{ .string = tgz_res.shasum });
    try dist_obj.put(aa, "tarball", .{ .string = tarball_url });
    try dist_obj.put(aa, "fileCount", .{ .integer = @intCast(tgz_res.file_count) });
    try dist_obj.put(aa, "unpackedSize", .{ .integer = @intCast(tgz_res.raw_size) });
    try ver_obj.put(aa, "dist", .{ .object = dist_obj });

    var versions_obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try versions_obj.put(aa, version, .{ .object = ver_obj });

    var dist_tags = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try dist_tags.put(aa, opts.tag, .{ .string = version });

    var att_entry = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try att_entry.put(aa, "content_type", .{ .string = "application/octet-stream" });
    try att_entry.put(aa, "data", .{ .string = b64_buf });
    try att_entry.put(aa, "length", .{ .integer = @intCast(tgz_res.tgz_bytes.len) });

    var attachments = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try attachments.put(aa, att_name, .{ .object = att_entry });

    var payload_root = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try payload_root.put(aa, "_id", .{ .string = pkg_name });
    try payload_root.put(aa, "name", .{ .string = pkg_name });
    try payload_root.put(aa, "access", .{ .string = opts.access });
    try payload_root.put(aa, "dist-tags", .{ .object = dist_tags });
    try payload_root.put(aa, "versions", .{ .object = versions_obj });
    try payload_root.put(aa, "_attachments", .{ .object = attachments });

    var body_out: std.Io.Writer.Allocating = .init(allocator);
    defer body_out.deinit();
    try std.json.Stringify.value(std.json.Value{ .object = payload_root }, .{}, &body_out.writer);
    const payload_bytes = try body_out.toOwnedSlice();
    defer allocator.free(payload_bytes);

    const put_url = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, encoded_name });
    defer allocator.free(put_url);

    const auth_hdr = try std.fmt.allocPrint(allocator, "Bearer {s}", .{auth_token});
    defer allocator.free(auth_hdr);

    var hdrs_list: std.ArrayList(std.http.Header) = .empty;
    defer hdrs_list.deinit(allocator);
    try hdrs_list.append(allocator, .{ .name = "authorization", .value = auth_hdr });
    try hdrs_list.append(allocator, .{ .name = "content-type", .value = "application/json" });
    try hdrs_list.append(allocator, .{ .name = "accept", .value = "application/json" });
    try hdrs_list.append(allocator, .{ .name = "npm-command", .value = "publish" });
    if (opts.otp) |otp_code| {
        if (otp_code.len > 0) {
            try hdrs_list.append(allocator, .{ .name = "npm-otp", .value = otp_code });
        }
    }

    var client: std.http.Client = .{
        .allocator = allocator,
        .io = main_mod.global_io,
    };
    defer client.deinit();

    var resp_writer: std.Io.Writer.Allocating = .init(allocator);
    defer resp_writer.deinit();

    std.debug.print("  ↑ Publishing {s}@{s} -> {s} (tag={s}, access={s}, {d} B .tgz)\n", .{
        pkg_name,
        version,
        put_url,
        opts.tag,
        opts.access,
        tgz_res.compressed_size,
    });

    const res = client.fetch(.{
        .location = .{ .url = put_url },
        .method = .PUT,
        .payload = payload_bytes,
        .extra_headers = hdrs_list.items,
        .response_writer = &resp_writer.writer,
    }) catch |err| {
        std.debug.print("Error: HTTP PUT publish failed for {s}: {s}\n", .{ put_url, @errorName(err) });
        return error.PublishFailed;
    };

    const status_code = @backingInt(res.status);
    if (status_code < 200 or status_code >= 300) {
        const resp_body = resp_writer.written();
        std.debug.print("Error: Registry rejected publish with HTTP {d}: {s}\n", .{ status_code, resp_body });
        return error.PublishRejected;
    }
}
