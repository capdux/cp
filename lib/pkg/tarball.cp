// Source: /data/dev/bun/src/install/extract_tarball.cp
// ============================================================================
// lib/pkg/tarball.cp — Safe Tarball (.tgz / .tar.gz) Builder, Verifier & Extractor
// Absorbed from Bun src/install/{extract_tarball.cp,TarballStream.cp}
// ============================================================================

const std = @import("std");
import { gzip, gunzip, TarHeader, TarReader, TAR_BLOCK_SIZE } from "cap:pack";
import { Integrity } from "../resolver/integrity.cp";
import { mkdirAll, writeFile, readFile, symlink } from "cap:sys";
import { PkgError } from "./types.cp";

pub struct TarballExtractOptions {
    strip_components: usize = 1,
    file_mode: u32 = 0o644,
    dir_mode: u32 = 0o755,
    exec_mode: u32 = 0o755,
    verify_integrity: bool = true,
    expected_integrity: Integrity = Integrity{},
    allow_symlinks: bool = false,
}

pub struct ExtractStats {
    files_extracted: usize = 0,
    dirs_created: usize = 0,
    symlinks_created: usize = 0,
    bytes_written: u64 = 0,
    skipped_entries: usize = 0,
    computed_integrity: Integrity = Integrity{},
}

pub struct TarEntryInput {
    name: []const u8 = "",
    data: []const u8 = "",
    mode: u32 = 0o644,
    is_dir: bool = false,
    symlink_target: []const u8 = "",
}

/// Builds USTAR `.tar` and `.tar.gz` archives in memory for packaging & testing.
pub struct TarballBuilder {
    pub static fn buildTarAlloc(allocator: std.mem.Allocator, entries: []const TarEntryInput): ([]u8)! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        let hdr: [TAR_BLOCK_SIZE]u8 = undefined;
        let zeros: [TAR_BLOCK_SIZE]u8 = [0; TAR_BLOCK_SIZE];

        for (entry of entries) {
            const typeflag: u8 = if (entry.symlink_target.len > 0)
                '2'
            else if (entry.is_dir)
                '5'
            else
                '0';

            const payload_size: u64 = if (entry.is_dir || entry.symlink_target.len > 0) 0 else (entry.data.len as u64);
            if (!TarHeader.encodeHeader(&hdr, entry.name, payload_size, entry.mode, 0, typeflag)) {
                return error.UnsupportedArchiveEntry;
            }
            if (entry.symlink_target.len > 0) {
                if (entry.symlink_target.len > 100) return error.UnsupportedArchiveEntry;
                @memcpy(hdr[157 .. 157 + entry.symlink_target.len], entry.symlink_target);
                const chksum = TarHeader.computeChecksum(hdr[0..TAR_BLOCK_SIZE]);
                TarHeader.writeOctal(hdr[148..155], chksum as u64);
                hdr[155] = ' ';
            }

            try out.appendSlice(allocator, hdr[0..TAR_BLOCK_SIZE]);

            if (payload_size > 0) {
                try out.appendSlice(allocator, entry.data);
                const rem = entry.data.len % TAR_BLOCK_SIZE;
                if (rem != 0) {
                    const pad = TAR_BLOCK_SIZE - rem;
                    try out.appendSlice(allocator, zeros[0..pad]);
                }
            }
        }

        // Standard 1024-byte (2 x 512) EOF marker
        try out.appendSlice(allocator, zeros[0..TAR_BLOCK_SIZE]);
        try out.appendSlice(allocator, zeros[0..TAR_BLOCK_SIZE]);
        return out.toOwnedSlice(allocator);
    }

    pub static fn buildTgzAlloc(allocator: std.mem.Allocator, entries: []const TarEntryInput): ([]u8)! {
        const tar_bytes = try TarballBuilder.buildTarAlloc(allocator, entries);
        defer allocator.free(tar_bytes);
        const gz = gzip(tar_bytes, 6);
        if (gz.len == 0) return error.TarballDecompressFailed;
        return try allocator.dupe(u8, gz);
    }

    pub static fn computeSha512(tgz_bytes: []const u8): Integrity {
        return Integrity.computeSha512(tgz_bytes);
    }
}

/// Production tarball extractor with strict path traversal defense and W3C SRI verification.
pub struct TarballExtractor {
    /// Validates that an archive member path contains no traversal (`..`), absolute root,
    /// Windows drive prefixes, backslashes, or NUL bytes.
    pub static fn isSafeArchivePath(raw_path: []const u8): bool {
        if (raw_path.len == 0) return false;
        if (raw_path[0] == '/' || raw_path[0] == '\\') return false;
        if (raw_path.len >= 2 && raw_path[1] == ':') return false;

        for (c of raw_path) {
            if (c == 0 || c == '\\') return false;
        }

        let iter = std.mem.splitScalar(u8, raw_path, '/');
        while (seg := iter.next()) {
            if (std.mem.eql(u8, seg, "..")) return false;
        }
        return true;
    }

    /// Strips `count` leading path components (e.g. `package/index.js` -> `index.js`).
    pub static fn stripComponents(raw_path: []const u8, count: usize): ([]const u8)? {
        let trimmed = std.mem.trimEnd(u8, raw_path, "/");
        if (trimmed.len == 0) return null;

        let skipped: usize = 0;
        let pos: usize = 0;
        while (skipped < count && pos < trimmed.len) {
            if (slash := std.mem.indexOfScalarPos(u8, trimmed, pos, '/')) {
                pos = slash + 1;
                skipped += 1;
            } else {
                return null;
            }
        }
        if (pos >= trimmed.len) return null;
        let out = trimmed[pos..];
        while (std.mem.startsWith(u8, out, "./")) {
            out = out[2..];
        }
        if (out.len == 0 || std.mem.eql(u8, out, ".")) return null;
        return out;
    }

    /// Ensures a symlink target inside `link_rel_path` never escapes the destination root.
    pub static fn isSafeSymlinkTarget(link_rel_path: []const u8, target: []const u8): bool {
        if (target.len == 0) return false;
        if (target[0] == '/' || target[0] == '\\') return false;
        for (c of target) {
            if (c == 0 || c == '\\') return false;
        }

        // Compute depth of link's parent directory
        let depth: isize = 0;
        if (last_slash := std.mem.lastIndexOfScalar(u8, link_rel_path, '/')) {
            const parent = link_rel_path[0..last_slash];
            let p_iter = std.mem.splitScalar(u8, parent, '/');
            while (seg := p_iter.next()) {
                if (seg.len > 0 && !std.mem.eql(u8, seg, ".")) {
                    depth += 1;
                }
            }
        }

        let t_iter = std.mem.splitScalar(u8, target, '/');
        while (seg := t_iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
            if (std.mem.eql(u8, seg, "..")) {
                depth -= 1;
                if (depth < 0) return false;
            } else {
                depth += 1;
            }
        }
        return true;
    }

    fn extractLinkname(tar_buf: []const u8, entry_start: usize): []const u8 {
        if (entry_start + 257 > tar_buf.len) return "";
        const field = tar_buf[entry_start + 157 .. entry_start + 257];
        let end: usize = 0;
        while (end < field.len && field[end] != 0) : (end += 1) {}
        return field[0..end];
    }

    /// Extracts uncompressed TAR bytes to `dest_dir` with full path traversal protection.
    pub static fn extractTarBytesToDir(
        allocator: std.mem.Allocator,
        tar_bytes: []const u8,
        dest_dir: []const u8,
        opts: TarballExtractOptions,
    ): ExtractStats! {
        try mkdirAll(dest_dir, opts.dir_mode);
        let stats = ExtractStats{};

        let reader = TarReader.init(tar_bytes);
        while (true) {
            const entry_hdr_pos = reader.pos;
            const entry = reader.next() ?? break;

            if (!TarballExtractor.isSafeArchivePath(entry.name)) {
                return error.PathTraversalDetected;
            }

            const rel_opt = TarballExtractor.stripComponents(entry.name, opts.strip_components);
            if (rel_opt == null) {
                stats.skipped_entries += 1;
                continue;
            }
            const rel_path = rel_opt.?;

            if (!TarballExtractor.isSafeArchivePath(rel_path)) {
                return error.PathTraversalDetected;
            }

            const full_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{
                std.mem.trimEnd(u8, dest_dir, "/"),
                rel_path,
            });
            defer allocator.free(full_path);

            if (entry.isDirectory()) {
                try mkdirAll(full_path, opts.dir_mode);
                stats.dirs_created += 1;
                continue;
            }

            if (entry.isSymlink()) {
                const target = TarballExtractor.extractLinkname(tar_bytes, entry_hdr_pos);
                if (!TarballExtractor.isSafeSymlinkTarget(rel_path, target)) {
                    return error.PathTraversalDetected;
                }
                if (!opts.allow_symlinks) {
                    stats.skipped_entries += 1;
                    continue;
                }
                if (last_slash := std.mem.lastIndexOfScalar(u8, full_path, '/')) {
                    try mkdirAll(full_path[0..last_slash], opts.dir_mode);
                }
                try symlink(target, full_path);
                stats.symlinks_created += 1;
                continue;
            }

            if (entry.isRegularFile()) {
                if (last_slash := std.mem.lastIndexOfScalar(u8, full_path, '/')) {
                    try mkdirAll(full_path[0..last_slash], opts.dir_mode);
                }
                const mode: u32 = if ((entry.mode & 0o111) != 0) opts.exec_mode else opts.file_mode;
                try writeFile(full_path, entry.data, mode);
                stats.files_extracted += 1;
                stats.bytes_written += entry.data.len as u64;
                continue;
            }

            stats.skipped_entries += 1;
        }

        return stats;
    }

    /// Verifies optional SRI integrity, gunzips `.tar.gz`/`.tgz`, and safely extracts to `dest_dir`.
    pub static fn extractBytesToDir(
        allocator: std.mem.Allocator,
        tgz_bytes: []const u8,
        dest_dir: []const u8,
        opts: TarballExtractOptions,
    ): ExtractStats! {
        if (opts.verify_integrity && opts.expected_integrity.tag.isSupported()) {
            if (!opts.expected_integrity.verify(tgz_bytes)) {
                return error.IntegrityCheckFailed;
            }
        }

        const tar_bytes = gunzip(tgz_bytes) ?? return error.TarballDecompressFailed;
        let stats = try TarballExtractor.extractTarBytesToDir(allocator, tar_bytes, dest_dir, opts);
        stats.computed_integrity = if (opts.expected_integrity.tag.isSupported())
            opts.expected_integrity
        else
            Integrity.computeSha512(tgz_bytes);
        return stats;
    }

    /// Reads a `.tgz` / `.tar.gz` file from disk, verifies SRI, and extracts to `dest_dir`.
    pub static fn extractFileToDir(
        allocator: std.mem.Allocator,
        tgz_path: []const u8,
        dest_dir: []const u8,
        opts: TarballExtractOptions,
    ): ExtractStats! {
        const bytes = try readFile(allocator, tgz_path);
        defer allocator.free(bytes);
        return TarballExtractor.extractBytesToDir(allocator, bytes, dest_dir, opts);
    }
}
