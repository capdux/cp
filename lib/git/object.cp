// Source: cp/lib/git/object.cp (Git Blob/Tree/Commit SHA-1 Hashing & Object Parser)
// Pure CP Git object model: OID hex codec, SHA-1 blob/object hashing,
// canonical Git tree binary codec, and commit header/message parser & encoder.

const std = @import("std");

/// Unified error set for `cap:git` object hashing, codec parsing, `.gitignore`, and `.git` inspection.
pub err GitError {
    InvalidObject,
    InvalidOid,
    IoError,
    OutOfMemory,
}

/// Git loose object type discriminator.
pub enum GitObjectKind {
    Blob,
    Tree,
    Commit,
    Tag,

    /// Returns the canonical lowercase Git header type name (`"blob"`, `"tree"`, `"commit"`, `"tag"`).
    pub fn asStr(): string {
        return switch (_) {
            .Blob => "blob",
            .Tree => "tree",
            .Commit => "commit",
            .Tag => "tag",
        };
    }

    /// Parses a canonical Git object type name into `GitObjectKind`.
    pub static fn fromString(s: string): GitObjectKind? {
        if (std.mem.eql(u8, s, "blob")) return GitObjectKind.Blob;
        if (std.mem.eql(u8, s, "tree")) return GitObjectKind.Tree;
        if (std.mem.eql(u8, s, "commit")) return GitObjectKind.Commit;
        if (std.mem.eql(u8, s, "tag")) return GitObjectKind.Tag;
        return null;
    }
}

const HEX_LOWER: []const u8 = "0123456789abcdef";

fn hexNibble(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return null;
}

/// 20-byte SHA-1 Git Object Identifier (OID).
pub struct GitOid {
    pub bytes: [20]u8 = [0; 20],

    /// Constructs a `GitOid` from a raw 20-byte SHA-1 digest.
    pub static fn fromBytes(bytes: [20]u8): GitOid {
        return GitOid{ bytes: bytes };
    }

    /// Returns the all-zero null OID (`0000...0000`).
    pub static fn zero(): GitOid {
        return GitOid{ bytes: [0; 20] };
    }

    /// Returns `true` iff `str` is exactly 40 hexadecimal ASCII characters.
    pub static fn isValidHex(str: string): bool {
        if (str.len != 40) return false;
        let i: usize = 0;
        while (i < 40) : (i += 1) {
            if (hexNibble(str[i]) == null) return false;
        }
        return true;
    }

    /// Parses a 40-character hex string into a 20-byte `GitOid`.
    pub static fn parseHex(str: string): GitOid ! GitError {
        if (str.len != 40) return GitError.InvalidOid;
        let out: [20]u8 = undefined;
        for (i of 0..20) {
            const hi_opt = hexNibble(str[i * 2]);
            if (hi_opt == null) return GitError.InvalidOid;
            const lo_opt = hexNibble(str[i * 2 + 1]);
            if (lo_opt == null) return GitError.InvalidOid;
            out[i] = (hi_opt.? << 4) | lo_opt.?;
        }
        return GitOid{ bytes: out };
    }

    static fn formatHexInto(bytes: [20]u8, buf: []u8) {
        if (buf.len < 40) {
            @panic("GitOid.toHex: buffer must be at least 40 bytes");
        }
        for (i of 0..20) {
            const b = bytes[i];
            buf[i * 2] = HEX_LOWER[usize((b >> 4) & 0x0F)];
            buf[i * 2 + 1] = HEX_LOWER[usize(b & 0x0F)];
        }
    }

    /// Formats this OID as 40 lowercase hex characters into `buf[0..40]` and returns the slice.
    pub fn toHex(buf: []u8): string {
        GitOid.formatHexInto(_.bytes, buf);
        return buf[0..40];
    }

    /// Allocates and returns a 40-character lowercase hex string for this OID.
    pub fn toHexAlloc(alloc: std.mem.Allocator): ([]u8) ! GitError {
        let out = alloc.alloc(u8, 40) catch return GitError.OutOfMemory;
        GitOid.formatHexInto(_.bytes, out);
        return out;
    }

    /// Compares two OIDs for byte-for-byte equality.
    pub fn eql(other: GitOid): bool {
        return std.mem.eql(u8, _.bytes[0..], other.bytes[0..]);
    }

    /// Returns `true` if all 20 bytes are zero.
    pub fn isZero(): bool {
        for (i of 0..20) {
            if (_.bytes[i] != 0) return false;
        }
        return true;
    }
}

/// Git object SHA-1 hashing utilities (`SHA1("<kind> <len>\0<payload>")`).
pub static struct GitObject {
    /// Computes the exact Git SHA-1 OID for an object of `kind` with `payload`.
    pub static fn hashObject(kind: GitObjectKind, payload: []const u8): GitOid {
        let hdr_buf: [64]u8 = undefined;
        const hdr = std.fmt.bufPrint(&hdr_buf, "{s} {d}\x00", .{ kind.asStr(), payload.len }) catch unreachable;

        let hasher = std.crypto.hash.Sha1.init(.{});
        hasher.update(hdr);
        hasher.update(payload);
        let digest: [20]u8 = undefined;
        hasher.final(&digest);
        return GitOid{ bytes: digest };
    }

    /// Computes the exact Git blob SHA-1 OID (`SHA1("blob <len>\0<data>")`), matching `git hash-object`.
    pub static fn hashBlob(data: []const u8): GitOid {
        return GitObject.hashObject(GitObjectKind.Blob, data);
    }
}

/// A single entry inside a Git tree object (`mode`, `name`, `oid`).
pub struct GitTreeEntry {
    pub mode: u32 = 0o100644,
    pub name: string = "",
    pub oid: GitOid = GitOid.zero(),

    /// Returns `true` if this entry represents a directory/subtree (`mode == 0o040000`).
    pub fn isDir(): bool {
        return (_.mode & 0o170000) == 0o040000;
    }
}

fn compareGitTreeEntries(a: GitTreeEntry, b: GitTreeEntry): i32 {
    const min_len = if (a.name.len < b.name.len) a.name.len else b.name.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a.name[i] < b.name[i]) return -1;
        if (a.name[i] > b.name[i]) return 1;
    }
    if (a.name.len == b.name.len) return 0;
    const a_next: u8 = if (a.name.len > min_len) a.name[min_len] else if (a.isDir()) '/' else 0;
    const b_next: u8 = if (b.name.len > min_len) b.name[min_len] else if (b.isDir()) '/' else 0;
    if (a_next < b_next) return -1;
    if (a_next > b_next) return 1;
    return 0;
}

/// Binary codec for Git tree objects (`<octal_mode> <name>\0<20-byte-oid>...`).
pub static struct GitTreeCodec {
    /// Encodes `entries` in canonical Git tree sort order (`/` appended virtually for tree entries).
    pub static fn encode(alloc: std.mem.Allocator, entries: []const GitTreeEntry): ([]u8) ! GitError {
        let sorted = alloc.alloc(GitTreeEntry, entries.len) catch return GitError.OutOfMemory;
        defer alloc.free(sorted);
        for (i of 0..entries.len) {
            const e = entries[i];
            if (e.name.len == 0 || std.mem.indexOfScalar(u8, e.name, '/') != null || std.mem.indexOfScalar(u8, e.name, 0) != null) {
                return GitError.InvalidObject;
            }
            if (std.mem.eql(u8, e.name, ".") || std.mem.eql(u8, e.name, "..")) {
                return GitError.InvalidObject;
            }
            sorted[i] = e;
        }

        // Insertion sort in canonical Git tree order (stable & zero extra allocation)
        let idx: usize = 1;
        while (idx < sorted.len) : (idx += 1) {
            const key = sorted[idx];
            let j: usize = idx;
            while (j > 0 && compareGitTreeEntries(sorted[j - 1], key) > 0) : (j -= 1) {
                sorted[j] = sorted[j - 1];
            }
            sorted[j] = key;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let mode_buf: [16]u8 = undefined;
        for (i of 0..sorted.len) {
            const entry = sorted[i];
            const mode_str = std.fmt.bufPrint(&mode_buf, "{o}", .{entry.mode}) catch return GitError.InvalidObject;
            out.appendSlice(alloc, mode_str) catch return GitError.OutOfMemory;
            out.append(alloc, ' ') catch return GitError.OutOfMemory;
            out.appendSlice(alloc, entry.name) catch return GitError.OutOfMemory;
            out.append(alloc, 0) catch return GitError.OutOfMemory;
            out.appendSlice(alloc, &entry.oid.bytes) catch return GitError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return GitError.OutOfMemory;
    }

    /// Decodes a raw Git tree payload into a slice of `GitTreeEntry`.
    /// Each `entry.name` borrows from `raw_tree`; caller owns the returned slice (`alloc.free`).
    pub static fn decode(alloc: std.mem.Allocator, raw_tree: []const u8): ([]GitTreeEntry) ! GitError {
        let entries: std.ArrayList(GitTreeEntry) = .empty;
        errdefer entries.deinit(alloc);

        let pos: usize = 0;
        while (pos < raw_tree.len) {
            const space_rel_opt = std.mem.indexOfScalar(u8, raw_tree[pos..], ' ');
            if (space_rel_opt == null) return GitError.InvalidObject;
            const space_rel = space_rel_opt.?;
            if (space_rel == 0) return GitError.InvalidObject;
            const mode_slice = raw_tree[pos .. pos + space_rel];
            for (k of 0..mode_slice.len) {
                if (mode_slice[k] < '0' || mode_slice[k] > '7') return GitError.InvalidObject;
            }
            const mode = std.fmt.parseInt(u32, mode_slice, 8) catch return GitError.InvalidObject;

            const name_start = pos + space_rel + 1;
            const nul_rel_opt = std.mem.indexOfScalar(u8, raw_tree[name_start..], 0);
            if (nul_rel_opt == null) return GitError.InvalidObject;
            const nul_rel = nul_rel_opt.?;
            if (nul_rel == 0) return GitError.InvalidObject;
            const name = raw_tree[name_start .. name_start + nul_rel];
            if (std.mem.indexOfScalar(u8, name, '/') != null || std.mem.eql(u8, name, ".") || std.mem.eql(u8, name, "..")) {
                return GitError.InvalidObject;
            }

            const oid_start = name_start + nul_rel + 1;
            if (oid_start + 20 > raw_tree.len) return GitError.InvalidObject;
            let oid_bytes: [20]u8 = undefined;
            @memcpy(&oid_bytes, raw_tree[oid_start .. oid_start + 20]);

            entries.append(alloc, GitTreeEntry{
                mode: mode,
                name: name,
                oid: GitOid{ bytes: oid_bytes },
            }) catch return GitError.OutOfMemory;

            pos = oid_start + 20;
        }

        return entries.toOwnedSlice(alloc) catch return GitError.OutOfMemory;
    }
}

/// Parsed Git commit object (`tree`, `parents`, `author`, `committer`, `message`).
pub struct GitCommit {
    alloc: std.mem.Allocator = undefined,
    pub tree: GitOid = GitOid.zero(),
    pub parents: []const GitOid = &.{},
    pub author: string = "",
    pub committer: string = "",
    pub message: string = "",

    /// Releases the `parents` slice allocated during `GitCommitCodec.parse`.
    pub fn deinit(*_) {
        if (_.parents.len > 0) {
            _.alloc.free(_.parents);
            _.parents = &.{};
        }
    }
}

/// Codec for parsing and encoding Git commit object payloads.
pub static struct GitCommitCodec {
    /// Parses a raw Git commit payload. `author`, `committer`, and `message` borrow from `raw_commit`;
    /// call `commit.deinit()` to release the allocated `parents` slice.
    pub static fn parse(alloc: std.mem.Allocator, raw_commit: []const u8): GitCommit ! GitError {
        let parents: std.ArrayList(GitOid) = .empty;
        errdefer parents.deinit(alloc);

        let has_tree = false;
        let has_author = false;
        let has_committer = false;
        let tree_oid = GitOid.zero();
        let author_val: string = "";
        let committer_val: string = "";
        let message_val: string = "";

        let pos: usize = 0;
        let found_blank = false;
        while (pos < raw_commit.len) {
            const nl_opt = std.mem.indexOfScalar(u8, raw_commit[pos..], '\n');
            const line_end = if (nl_opt != null) pos + nl_opt.? else raw_commit.len;
            const line = raw_commit[pos..line_end];

            if (line.len == 0) {
                if (nl_opt == null) return GitError.InvalidObject;
                message_val = raw_commit[line_end + 1 ..];
                found_blank = true;
                break;
            }

            // Skip continuation lines of multi-line headers (e.g., gpgsig)
            if (line[0] == ' ') {
                if (nl_opt == null) break;
                pos = line_end + 1;
                continue;
            }

            if (std.mem.startsWith(u8, line, "tree ")) {
                if (has_tree) return GitError.InvalidObject;
                tree_oid = GitOid.parseHex(line[5..])?;
                has_tree = true;
            } else if (std.mem.startsWith(u8, line, "parent ")) {
                const p_oid = GitOid.parseHex(line[7..])?;
                parents.append(alloc, p_oid) catch return GitError.OutOfMemory;
            } else if (std.mem.startsWith(u8, line, "author ")) {
                if (has_author) return GitError.InvalidObject;
                author_val = line[7..];
                has_author = true;
            } else if (std.mem.startsWith(u8, line, "committer ")) {
                if (has_committer) return GitError.InvalidObject;
                committer_val = line[10..];
                has_committer = true;
            } else {
                // Unknown/optional headers (e.g., encoding, gpgsig, mergetag) must contain a space
                if (std.mem.indexOfScalar(u8, line, ' ') == null) {
                    return GitError.InvalidObject;
                }
            }

            if (nl_opt == null) break;
            pos = line_end + 1;
        }

        if (!found_blank || !has_tree || !has_author || !has_committer) {
            return GitError.InvalidObject;
        }

        const owned_parents = parents.toOwnedSlice(alloc) catch return GitError.OutOfMemory;
        return GitCommit{
            alloc: alloc,
            tree: tree_oid,
            parents: owned_parents,
            author: author_val,
            committer: committer_val,
            message: message_val,
        };
    }

    /// Encodes a `GitCommit` into its canonical Git commit payload (`tree`, `parent`*, `author`, `committer`, `\n`, `message`).
    pub static fn encode(alloc: std.mem.Allocator, commit: GitCommit): ([]u8) ! GitError {
        if (commit.author.len == 0 || commit.committer.len == 0) {
            return GitError.InvalidObject;
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let hex_buf: [40]u8 = undefined;

        out.appendSlice(alloc, "tree ") catch return GitError.OutOfMemory;
        out.appendSlice(alloc, commit.tree.toHex(&hex_buf)) catch return GitError.OutOfMemory;
        out.append(alloc, '\n') catch return GitError.OutOfMemory;

        for (i of 0..commit.parents.len) {
            out.appendSlice(alloc, "parent ") catch return GitError.OutOfMemory;
            out.appendSlice(alloc, commit.parents[i].toHex(&hex_buf)) catch return GitError.OutOfMemory;
            out.append(alloc, '\n') catch return GitError.OutOfMemory;
        }

        out.appendSlice(alloc, "author ") catch return GitError.OutOfMemory;
        out.appendSlice(alloc, commit.author) catch return GitError.OutOfMemory;
        out.append(alloc, '\n') catch return GitError.OutOfMemory;

        out.appendSlice(alloc, "committer ") catch return GitError.OutOfMemory;
        out.appendSlice(alloc, commit.committer) catch return GitError.OutOfMemory;
        out.appendSlice(alloc, "\n\n") catch return GitError.OutOfMemory;

        out.appendSlice(alloc, commit.message) catch return GitError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return GitError.OutOfMemory;
    }
}
