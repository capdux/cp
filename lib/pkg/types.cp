// Source: Absorbed from /data/dev/bun/src/install_types/ExternalString.cp | Synced: 2026-11-21T16:51:29Z
// ============================================================================
// lib/pkg/types.cp — Core Package Manager Types, Compact Strings, Slices & Behavior
// Absorbed from Bun src/install_types/{ExternalString,SlicedString,NodeLinker,SemverString}.cp
// and src/install/ExternalSlice.cp
// ============================================================================

const std = @import("std");
import { Hash } from "cap:hash";

pub type PackageID = u32;
pub type DependencyID = u32;
pub type ResolutionID = u32;
pub type PackageNameHash = u64;
pub type TruncatedPackageNameHash = u32;
pub type PackageNameAndVersionHash = u64;

pub const INVALID_PACKAGE_ID: PackageID = std.math.maxInt(u32);
pub const INVALID_DEPENDENCY_ID: DependencyID = std.math.maxInt(u32);
pub const INVALID_RESOLUTION_ID: ResolutionID = std.math.maxInt(u32);

pub err PkgError {
    InvalidDependency,
    InvalidVersion,
    InvalidResolution,
    InvalidPackument,
    InvalidLockfile,
    LockfileCorrupt,
    IntegrityCheckFailed,
    PathTraversalDetected,
    UnsupportedArchiveEntry,
    TarballDecompressFailed,
    PatchFailed,
    PackageNotFound,
    CyclicDependency,
    MissingVersion,
    InvalidPackageName,
    BufferTooSmall,
    IoError,
    OutOfMemory,
}

/// Package installation linking strategy (from Bun NodeLinker.cp).
pub enum NodeLinker: u8 {
    Auto = 0,
    Hoisted = 1,
    Isolated = 2,

    static fn fromStr(input: []const u8): NodeLinker? {
        if (std.mem.eql(u8, input, "hoisted")) return .Hoisted;
        if (std.mem.eql(u8, input, "isolated")) return .Isolated;
        if (std.mem.eql(u8, input, "auto")) return .Auto;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Auto => "auto",
            .Hoisted => "hoisted",
            .Isolated => "isolated",
        };
    }
}

/// Computes canonical 64-bit Wyhash for package names & cache keys.
pub fn hashPackageName(name: []const u8): PackageNameHash {
    return Hash.wyhash(name, 0);
}

/// Computes truncated 32-bit package name hash for compact sets (e.g. trustedDependencies).
pub fn truncatePackageNameHash(h: PackageNameHash): TruncatedPackageNameHash {
    return (h & 0xFFFFFFFF) as u32;
}

/// Compact 8-byte string pointer into a contiguous `StringBuf` byte buffer.
pub struct StringPointer {
    off: u32 = 0,
    len: u32 = 0,

    pub static fn init(off: u32, len: u32): StringPointer {
        return StringPointer{ off: off, len: len };
    }
}

/// Dual-mode 8-byte compact string:
/// - Inline mode (bit 63 == 0): up to 8 ASCII bytes stored directly inside `bytes[0..8]`
/// - External mode (bit 63 == 1): 31-bit offset + 32-bit length pointing into `StringBuf`
pub struct SemverString {
    bytes: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0],

    pub const MAX_INLINE_LEN: usize = 8;

    pub static fn canInline(s: []const u8): bool {
        if (s.len <= 7) return true;
        if (s.len == 8) return (s[7] & 0x80) == 0;
        return false;
    }

    pub static fn initInline(s: []const u8): SemverString {
        let out = SemverString{};
        const n = if (s.len > 8) 8 else s.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out.bytes[i] = s[i];
        }
        return out;
    }

    pub static fn initExternal(off: u32, length: u32): SemverString {
        let out = SemverString{};
        const packed_bits: u64 = ((off as u64) & 0x7FFFFFFF) | ((length as u64) << 32) | (1 << 63);
        zig {
            std.mem.writeInt(u64, &out.bytes, packed_bits, .little);
        }
        return out;
    }

    pub fn isInline(): bool {
        return (_.bytes[7] & 0x80) == 0;
    }

    pub fn isEmpty(): bool {
        if (_.isInline()) {
            return _.bytes[0] == 0;
        }
        return _.len() == 0;
    }

    pub fn len(): usize {
        if (_.isInline()) {
            let i: usize = 0;
            while (i < 8) : (i += 1) {
                if (_.bytes[i] == 0) return i;
            }
            return 8;
        }
        let packed_bits: u64 = 0;
        zig {
            packed_bits = std.mem.readInt(u64, &self.bytes, .little);
        }
        return ((packed_bits >> 32) & 0x7FFFFFFF) as usize;
    }

    pub fn pointer(): StringPointer {
        if (_.isInline()) {
            return StringPointer{ off: 0, len: _.len() as u32 };
        }
        let packed_bits: u64 = 0;
        zig {
            packed_bits = std.mem.readInt(u64, &self.bytes, .little);
        }
        const off: u32 = (packed_bits & 0x7FFFFFFF) as u32;
        const length: u32 = ((packed_bits >> 32) & 0x7FFFFFFF) as u32;
        return StringPointer{ off: off, len: length };
    }

    pub static fn sliceOf(s: &SemverString, buf: []const u8): []const u8 {
        if (s.isInline()) {
            const n = s.len();
            if (n == 0) return "";
            return s.bytes[0..n];
        }
        const p = s.pointer();
        const start = p.off as usize;
        const end = start + (p.len as usize);
        if (end > buf.len) return "";
        return buf[start..end];
    }

    pub fn asSlice(*_, buf: []const u8): []const u8 {
        return SemverString.sliceOf(_, buf);
    }

    pub fn eql(other: SemverString, this_buf: []const u8, other_buf: []const u8): bool {
        return std.mem.eql(u8, SemverString.sliceOf(&_, this_buf), SemverString.sliceOf(&other, other_buf));
    }

    pub fn order(other: SemverString, this_buf: []const u8, other_buf: []const u8): std.math.Order {
        return std.mem.order(u8, SemverString.sliceOf(&_, this_buf), SemverString.sliceOf(&other, other_buf));
    }
}

/// Dual-mode zero-copy external string pairing a `SemverString` with its 64-bit Wyhash.
pub struct ExternalString {
    value: SemverString = SemverString{},
    hash: u64 = 0,

    pub static fn from(in_str: []const u8): ExternalString {
        const h = Hash.wyhash(in_str, 0);
        if (SemverString.canInline(in_str)) {
            return ExternalString{
                value: SemverString.initInline(in_str),
                hash: h,
            };
        }
        return ExternalString{
            value: SemverString{},
            hash: h,
        };
    }

    pub static fn init(off: u32, length: u32, hash: u64): ExternalString {
        return ExternalString{
            value: SemverString.initExternal(off, length),
            hash: hash,
        };
    }

    pub fn isInline(): bool {
        return _.value.isInline();
    }

    pub fn isEmpty(): bool {
        return _.value.isEmpty();
    }

    pub fn len(): usize {
        return _.value.len();
    }

    pub fn asSlice(*_, buf: []const u8): []const u8 {
        return SemverString.sliceOf(&_.value, buf);
    }

    pub fn eql(other: ExternalString, this_buf: []const u8, other_buf: []const u8): bool {
        if (_.hash != 0 && other.hash != 0 && _.hash != other.hash) return false;
        return _.value.eql(other.value, this_buf, other_buf);
    }

    pub fn order(other: ExternalString, this_buf: []const u8, other_buf: []const u8): std.math.Order {
        return _.value.order(other.value, this_buf, other_buf);
    }
}

/// Sub-slice view anchored inside a backing `buf` slice (from Bun SlicedString.cp).
pub struct SlicedString {
    buf: []const u8 = "",
    str: []const u8 = "",

    pub static fn init(buf: []const u8, sub_str: []const u8): SlicedString {
        return SlicedString{ buf: buf, str: sub_str };
    }

    pub fn len(): usize {
        return _.str.len;
    }

    pub fn sub(input: []const u8): SlicedString {
        return SlicedString{ buf: _.buf, str: input };
    }

    pub fn hash(): u64 {
        return Hash.wyhash(_.str, 0);
    }

    pub fn value(): SemverString {
        if (SemverString.canInline(_.str)) {
            return SemverString.initInline(_.str);
        }
        const buf_addr = unsafe { @intFromPtr(_.buf.ptr) };
        const sub_addr = unsafe { @intFromPtr(_.str.ptr) };
        if (sub_addr >= buf_addr && (sub_addr + _.str.len) <= (buf_addr + _.buf.len)) {
            const off = (sub_addr - buf_addr) as u32;
            return SemverString.initExternal(off, _.str.len as u32);
        }
        return SemverString{};
    }

    pub fn external(): ExternalString {
        return ExternalString{
            value: _.value(),
            hash: _.hash(),
        };
    }
}

/// Contiguous byte arena & string deduplication pool for compact lockfile/manifest storage.
pub struct StringBuf {
    bytes: std.ArrayListUnmanaged(u8) = .empty,
    pool: std.AutoHashMapUnmanaged(u64, SemverString) = .empty,

    pub static fn init(): StringBuf {
        return StringBuf{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.bytes.deinit(allocator);
        _.pool.deinit(allocator);
    }

    pub fn items(): []const u8 {
        return _.bytes.items;
    }

    pub fn slice(s: SemverString): []const u8 {
        if (s.isInline()) {
            const n = s.len();
            if (n == 0) return "";
            // Inline bytes only live in caller if passed by ref; external slices live in _.bytes
            return "";
        }
        const p = s.pointer();
        const start = p.off as usize;
        const end = start + (p.len as usize);
        if (end > _.bytes.items.len) return "";
        return _.bytes.items[start..end];
    }

    pub fn append(*_, allocator: std.mem.Allocator, s: []const u8): SemverString! {
        if (SemverString.canInline(s)) {
            return SemverString.initInline(s);
        }
        const h = Hash.wyhash(s, 0);
        if (existing := _.pool.get(h)) {
            if (std.mem.eql(u8, SemverString.sliceOf(&existing, _.bytes.items), s)) {
                return existing;
            }
        }
        const off = _.bytes.items.len as u32;
        try _.bytes.appendSlice(allocator, s);
        const str_ref = SemverString.initExternal(off, s.len as u32);
        try _.pool.put(allocator, h, str_ref);
        return str_ref;
    }

    pub fn appendExternal(*_, allocator: std.mem.Allocator, s: []const u8): ExternalString! {
        const h = Hash.wyhash(s, 0);
        const val = try _.append(allocator, s);
        return ExternalString{ value: val, hash: h };
    }
}

/// Offset+length typed slice referencing into an external backing slice (from Bun ExternalSlice.cp).
pub fn ExternalSlice(comptime T: type): type {
    return struct {
        off: u32 = 0,
        len: u32 = 0,

        pub const Slice = @This();

        pub static fn invalid(): Slice {
            return Slice{
                off: std.math.maxInt(u32),
                len: std.math.maxInt(u32),
            };
        }

        pub fn isInvalid(): bool {
            return _.off == std.math.maxInt(u32) && _.len == std.math.maxInt(u32);
        }

        pub fn contains(id: u32): bool {
            return id >= _.off && id < (_.off + _.len);
        }

        pub fn begin(): u32 {
            return _.off;
        }

        pub fn end(): u32 {
            return _.off + _.len;
        }

        pub fn get(backing: []const T): []const T {
            if (_.len == 0) return [];
            const start = _.off as usize;
            if (start >= backing.len) return [];
            const stop = @min(backing.len, start + (_.len as usize));
            return backing[start..stop];
        }

        pub fn mut(backing: []T): []T {
            if (_.len == 0) return [];
            const start = _.off as usize;
            if (start >= backing.len) return [];
            const stop = @min(backing.len, start + (_.len as usize));
            return backing[start..stop];
        }

        pub static fn fromOffsets(off: u32, len: u32): Slice {
            return Slice{ off: off, len: len };
        }
    };
}

/// Feature flags controlling which dependency groups are installed.
pub struct InstallFeatures {
    prod_dependencies: bool = true,
    dev_dependencies: bool = true,
    optional_dependencies: bool = true,
    peer_dependencies: bool = true,
    workspaces: bool = true,
}

/// Dependency behavior bitfield (normal/prod, optional, dev, peer, workspace, bundled).
pub struct Behavior {
    prod: bool = false,
    optional: bool = false,
    dev: bool = false,
    peer: bool = false,
    workspace: bool = false,
    bundled: bool = false,

    pub const PROD_BIT: u8 = 1 << 1;
    pub const OPTIONAL_BIT: u8 = 1 << 2;
    pub const DEV_BIT: u8 = 1 << 3;
    pub const PEER_BIT: u8 = 1 << 4;
    pub const WORKSPACE_BIT: u8 = 1 << 5;
    pub const BUNDLED_BIT: u8 = 1 << 6;

    pub static fn normal(): Behavior {
        return Behavior{ prod: true };
    }

    pub static fn devOnly(): Behavior {
        return Behavior{ dev: true };
    }

    pub static fn optionalOnly(): Behavior {
        return Behavior{ optional: true };
    }

    pub static fn peerOnly(): Behavior {
        return Behavior{ peer: true };
    }

    pub static fn optionalPeer(): Behavior {
        return Behavior{ optional: true, peer: true };
    }

    pub static fn workspaceDep(): Behavior {
        return Behavior{ workspace: true };
    }

    pub fn toByte(): u8 {
        let b: u8 = 0;
        if (_.prod) b |= PROD_BIT;
        if (_.optional) b |= OPTIONAL_BIT;
        if (_.dev) b |= DEV_BIT;
        if (_.peer) b |= PEER_BIT;
        if (_.workspace) b |= WORKSPACE_BIT;
        if (_.bundled) b |= BUNDLED_BIT;
        return b;
    }

    pub static fn fromByte(b: u8): Behavior {
        return Behavior{
            prod: (b & PROD_BIT) != 0,
            optional: (b & OPTIONAL_BIT) != 0,
            dev: (b & DEV_BIT) != 0,
            peer: (b & PEER_BIT) != 0,
            workspace: (b & WORKSPACE_BIT) != 0,
            bundled: (b & BUNDLED_BIT) != 0,
        };
    }

    pub fn isProd(): bool { return _.prod; }
    pub fn isOptional(): bool { return _.optional && !_.peer; }
    pub fn isOptionalPeer(): bool { return _.optional && _.peer; }
    pub fn isDev(): bool { return _.dev; }
    pub fn isPeer(): bool { return _.peer; }
    pub fn isWorkspace(): bool { return _.workspace; }
    pub fn isBundled(): bool { return _.bundled; }
    pub fn isRequired(): bool { return !_.optional; }

    pub fn eq(other: Behavior): bool {
        return _.toByte() == other.toByte();
    }

    pub fn includes(other: Behavior): bool {
        return (_.toByte() & other.toByte()) != 0;
    }

    pub fn merge(other: Behavior): Behavior {
        return Behavior.fromByte(_.toByte() | other.toByte());
    }

    pub fn isEnabled(features: InstallFeatures): bool {
        return _.prod ||
            (features.optional_dependencies && _.optional) ||
            (features.dev_dependencies && _.dev) ||
            (features.peer_dependencies && _.peer) ||
            (features.workspaces && _.workspace);
    }

    /// Canonical sort order: workspace -> dev -> optional -> prod -> peer.
    pub fn cmp(other: Behavior): std.math.Order {
        if (_.eq(other)) return .eq;
        if (_.workspace != other.workspace) return if (_.workspace) .lt else .gt;
        if (_.dev != other.dev) return if (_.dev) .lt else .gt;
        if (_.isOptional() != other.isOptional()) return if (_.isOptional()) .lt else .gt;
        if (_.prod != other.prod) return if (_.prod) .lt else .gt;
        if (_.peer != other.peer) return if (_.peer) .lt else .gt;
        return .eq;
    }
}
