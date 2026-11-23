// Source: cp/lib/git/repo.cp (Read-Only .git HEAD & Ref Inspector)
// Pure CP read-only `.git` repository HEAD and reference resolver using `std.c` file I/O.
// Supports symbolic refs, loose refs, `packed-refs`, detached OIDs, and `.git` gitdir files (worktrees/submodules).

const std = @import("std");
import { GitError, GitOid } from "./object.cp";

fn trimAsciiSpaceAndCrLf(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn trimRightSlashes(s: string): string {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

fn readSmallFileAlloc(alloc: std.mem.Allocator, path: string, max_bytes: usize): ([]u8) ! GitError {
    if (path.len == 0 || path.len >= 4096) return GitError.IoError;
    let path_buf: [4096]u8 = undefined;
    @memcpy(path_buf[0..path.len], path);
    path_buf[path.len] = 0;
    const c_path: [*:0]const u8 = unsafe { @ptrCast(&path_buf[0]) };

    const flags = std.c.O{ .ACCMODE = .RDONLY, .CLOEXEC = true };
    const fd = std.c.open(c_path, flags, @as(std.c.mode_t, 0));
    if (fd < 0) return GitError.IoError;
    defer * = std.c.close(fd);

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let chunk: [4096]u8 = undefined;
    while (true) {
        const n = std.c.read(fd, unsafe { @ptrCast(&chunk[0]) }, chunk.len);
        if (n == 0) break;
        if (n < 0) {
            if (std.c._errno().* == @intFromEnum(std.c.E.INTR)) continue;
            return GitError.IoError;
        }
        const read_len = n as usize;
        if (out.items.len + read_len > max_bytes) return GitError.IoError;
        out.appendSlice(alloc, chunk[0..read_len]) catch return GitError.OutOfMemory;
    }
    return out.toOwnedSlice(alloc) catch return GitError.OutOfMemory;
}

fn readSmallFileOpt(alloc: std.mem.Allocator, path: string, max_bytes: usize): ([]u8)? {
    const res = readSmallFileAlloc(alloc, path, max_bytes) catch return null;
    return res;
}

fn lookupPackedRef(alloc: std.mem.Allocator, git_dir: string, ref_name: string): ([]u8) ! GitError {
    const packed_path = std.fmt.allocPrint(alloc, "{s}/packed-refs", .{git_dir}) catch return GitError.OutOfMemory;
    defer alloc.free(packed_path);

    const content = readSmallFileAlloc(alloc, packed_path, 4 * 1024 * 1024)?;
    defer alloc.free(content);

    let iter = std.mem.splitScalar(u8, content, '\n');
    while (raw_line := iter.next()) {
        const line = trimAsciiSpaceAndCrLf(raw_line);
        if (line.len == 0 || line[0] == '#' || line[0] == '^') continue;
        const sp = std.mem.indexOfScalar(u8, line, ' ') ?? continue;
        const oid_part = line[0..sp];
        const name_part = trimAsciiSpaceAndCrLf(line[sp + 1 ..]);
        if (std.mem.eql(u8, name_part, ref_name)) {
            if (!GitOid.isValidHex(oid_part)) return GitError.InvalidOid;
            return alloc.dupe(u8, oid_part) catch return GitError.OutOfMemory;
        }
    }
    return GitError.IoError;
}

fn resolveGitDir(alloc: std.mem.Allocator, repo_root: string): ([]u8) ! GitError {
    const clean_root = trimRightSlashes(repo_root);
    const dot_git = std.fmt.allocPrint(alloc, "{s}/.git", .{clean_root}) catch return GitError.OutOfMemory;

    // Check if `<repo_root>/.git/HEAD` is directly readable (standard repo)
    const head_probe = std.fmt.allocPrint(alloc, "{s}/HEAD", .{dot_git}) catch {
        alloc.free(dot_git);
        return GitError.OutOfMemory;
    };
    defer alloc.free(head_probe);

    if (head_probe.len < 4096) {
        let path_buf: [4096]u8 = undefined;
        @memcpy(path_buf[0..head_probe.len], head_probe);
        path_buf[head_probe.len] = 0;
        const c_path: [*:0]const u8 = unsafe { @ptrCast(&path_buf[0]) };
        const fd = std.c.open(c_path, .{ .ACCMODE = .RDONLY, .CLOEXEC = true }, @as(std.c.mode_t, 0));
        if (fd >= 0) {
            * = std.c.close(fd);
            return dot_git;
        }
    }

    // Fallback: `.git` may be a worktree/submodule file containing `gitdir: <path>`
    const dot_git_file = readSmallFileAlloc(alloc, dot_git, 8192) catch {
        alloc.free(dot_git);
        return GitError.IoError;
    };
    defer alloc.free(dot_git_file);
    alloc.free(dot_git);

    const trimmed = trimAsciiSpaceAndCrLf(dot_git_file);
    if (!std.mem.startsWith(u8, trimmed, "gitdir:")) {
        return GitError.IoError;
    }
    const raw_target = trimAsciiSpaceAndCrLf(trimmed[7..]);
    if (raw_target.len == 0) return GitError.IoError;

    if (raw_target[0] == '/') {
        return alloc.dupe(u8, trimRightSlashes(raw_target)) catch return GitError.OutOfMemory;
    }
    return std.fmt.allocPrint(alloc, "{s}/{s}", .{ clean_root, trimRightSlashes(raw_target) }) catch return GitError.OutOfMemory;
}

/// Owned `.git/HEAD` inspection result. Call `deinit()` to free owned strings.
pub struct GitHead {
    alloc: std.mem.Allocator = undefined,
    pub is_detached: bool = false,
    pub branch_name: string = "",
    pub oid_hex: string = "",

    /// Releases `branch_name` and `oid_hex` owned by this `GitHead`.
    pub fn deinit(*_) {
        if (_.branch_name.len > 0) {
            _.alloc.free(_.branch_name);
            _.branch_name = "";
        }
        if (_.oid_hex.len > 0) {
            _.alloc.free(_.oid_hex);
            _.oid_hex = "";
        }
    }
}

/// Read-only `.git` metadata inspector (`HEAD`, symbolic refs, loose refs, and `packed-refs`).
pub static struct GitRepo {
    /// Reads `<repo_root>/.git/HEAD`, resolving symbolic `ref: refs/heads/<branch>`
    /// (loose ref or `packed-refs`) or detached 40-char hex OIDs.
    pub static fn readHead(alloc: std.mem.Allocator, repo_root: string): GitHead ! GitError {
        const git_dir = resolveGitDir(alloc, repo_root)?;
        defer alloc.free(git_dir);

        const head_path = std.fmt.allocPrint(alloc, "{s}/HEAD", .{git_dir}) catch return GitError.OutOfMemory;
        defer alloc.free(head_path);

        const head_raw = readSmallFileAlloc(alloc, head_path, 4096)?;
        defer alloc.free(head_raw);

        const head_line = trimAsciiSpaceAndCrLf(head_raw);
        if (std.mem.startsWith(u8, head_line, "ref:")) {
            const ref_path = trimAsciiSpaceAndCrLf(head_line[4..]);
            if (ref_path.len == 0) return GitError.InvalidObject;

            const branch_slice = if (std.mem.startsWith(u8, ref_path, "refs/heads/"))
                ref_path["refs/heads/".len ..]
            else
                ref_path;

            const loose_path = std.fmt.allocPrint(alloc, "{s}/{s}", .{ git_dir, ref_path }) catch return GitError.OutOfMemory;
            defer alloc.free(loose_path);

            let owned_oid: []u8 = undefined;
            const loose_opt = readSmallFileOpt(alloc, loose_path, 256);
            if (loose_raw := loose_opt) {
                defer alloc.free(loose_raw);
                const oid_trimmed = trimAsciiSpaceAndCrLf(loose_raw);
                if (!GitOid.isValidHex(oid_trimmed)) return GitError.InvalidOid;
                owned_oid = alloc.dupe(u8, oid_trimmed) catch return GitError.OutOfMemory;
            } else {
                owned_oid = lookupPackedRef(alloc, git_dir, ref_path)?;
            }
            errdefer alloc.free(owned_oid);

            // Normalize hex to lowercase for consistent consumer comparisons
            for (i of 0..owned_oid.len) {
                if (owned_oid[i] >= 'A' && owned_oid[i] <= 'F') {
                    owned_oid[i] = owned_oid[i] - 'A' + 'a';
                }
            }

            const owned_branch = alloc.dupe(u8, branch_slice) catch return GitError.OutOfMemory;
            return GitHead{
                alloc: alloc,
                is_detached: false,
                branch_name: owned_branch,
                oid_hex: owned_oid,
            };
        }

        // Detached HEAD: expect 40-char SHA-1 hex OID
        if (!GitOid.isValidHex(head_line)) return GitError.InvalidOid;
        let owned_oid = alloc.dupe(u8, head_line) catch return GitError.OutOfMemory;
        for (i of 0..owned_oid.len) {
            if (owned_oid[i] >= 'A' && owned_oid[i] <= 'F') {
                owned_oid[i] = owned_oid[i] - 'A' + 'a';
            }
        }
        return GitHead{
            alloc: alloc,
            is_detached: true,
            branch_name: "",
            oid_hex: owned_oid,
        };
    }
}
