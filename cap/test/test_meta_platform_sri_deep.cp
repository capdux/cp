// cap/test/test_meta_platform_sri_deep.cp
// ============================================================================
// Deep Regression Suite for cap:meta, cap:platform & cap:resolver SRI/HostedGit
// (Batch 6 - Subagent 6A)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    isPowerOfTwo,
    nextPowerOfTwo,
    alignUp,
    alignDown,
    isAligned,
    bitCount,
    leadingZeros,
    trailingZeros,
    reverseBits,
    rotateLeft,
    rotateRight,
    packU16,
    unpackU16,
    packU32,
    unpackU32,
    packU64,
    unpackU64,
    asInt,
    fromInt,
    bitfieldIntersects,
    bitfieldInsert,
    bitfieldRemove,
    bitfieldContains,
    isInt,
    isFloat,
    isNumber,
    isBool,
    isSlice,
    isPointer,
    isSingleItemPtr,
    isConstPtr,
    isOptional,
    isStruct,
    isEnum,
    isUnion,
    isArray,
    hasField,
    hasDecl,
    fieldCount,
    sizeOf,
    alignOf,
    typeName,
    ChildType,
    ItemType,
} from "cap:meta";

import {
    OsKind,
    ArchKind,
    Endianness,
    KernelVersion,
    PlatformInfo,
} from "cap:platform";

import {
    IntegrityTag,
    Integrity,
    GitHost,
    HostedGitInfo,
} from "cap:resolver";

@packed struct DemoFlags {
    read: bool = false,
    write: bool = false,
    exec: bool = false,
    pad: u5 = 0,
}

pub struct SampleRecord {
    id: u64 = 0;
    score: f64 = 0.0;
    tag: u16 = 0;

    pub fn isReady(): bool {
        return _.id > 0;
    }
}

pub enum SampleMode: u8 {
    Fast = 0,
    Safe = 1,
}

pub union SamplePayload {
    num: u32,
    flag: bool,
}

// ============================================================================
// 1. cap:meta — Bit Manipulation, Alignment & Integer Packing
// ============================================================================
fn testMetaBits(): ! {
    expect_eq(isPowerOfTwo(0), false);
    expect_eq(isPowerOfTwo(1), true);
    expect_eq(isPowerOfTwo(2), true);
    expect_eq(isPowerOfTwo(3), false);
    expect_eq(isPowerOfTwo(1024), true);
    expect_eq(isPowerOfTwo(1023), false);

    expect_eq(nextPowerOfTwo(0), 1);
    expect_eq(nextPowerOfTwo(1), 1);
    expect_eq(nextPowerOfTwo(5), 8);
    expect_eq(nextPowerOfTwo(16), 16);
    expect_eq(nextPowerOfTwo(1000), 1024);

    expect_eq(alignUp(0, 4096), 0);
    expect_eq(alignUp(1, 4096), 4096);
    expect_eq(alignUp(4096, 4096), 4096);
    expect_eq(alignUp(4097, 4096), 8192);
    expect_eq(alignDown(4097, 4096), 4096);
    expect_eq(alignDown(4095, 4096), 0);
    expect_eq(isAligned(8192, 4096), true);
    expect_eq(isAligned(8193, 4096), false);

    expect_eq(bitCount(0), 0);
    expect_eq(bitCount(0b10110110), 5);
    expect_eq(bitCount(0xFF as u8), 8);

    expect_eq(leadingZeros(0x00FF as u16), 8);
    expect_eq(leadingZeros(0x80 as u8), 0);
    expect_eq(trailingZeros(0b10110000 as u8), 4);
    expect_eq(trailingZeros(0 as u32), 32);

    const rev8 = reverseBits(0b10110000 as u8);
    expect_eq(rev8, 0b00001101 as u8);
    const rotl8 = rotateLeft(0b10000011 as u8, 1);
    expect_eq(rotl8, 0b00000111 as u8);
    const rotr8 = rotateRight(0b00000111 as u8, 1);
    expect_eq(rotr8, 0b10000011 as u8);

    const p16 = packU16(0xAB, 0xCD);
    expect_eq(p16, 0xABCD as u16);
    const [h16, l16] = unpackU16(p16);
    expect_eq(h16, 0xAB as u8);
    expect_eq(l16, 0xCD as u8);

    const p32 = packU32(0x1234, 0x5678);
    expect_eq(p32, 0x12345678 as u32);
    const [h32, l32] = unpackU32(p32);
    expect_eq(h32, 0x1234 as u16);
    expect_eq(l32, 0x5678 as u16);

    const p64 = packU64(0xCAFEBABE, 0xDEADBEEF);
    expect_eq(p64, 0xCAFEBABEDEADBEEF as u64);
    const [h64, l64] = unpackU64(p64);
    expect_eq(h64, 0xCAFEBABE as u32);
    expect_eq(l64, 0xDEADBEEF as u32);

    // Packed bitfield helpers
    let flags = DemoFlags{ read: true };
    const w_mask = DemoFlags{ write: true };
    expect_eq(bitfieldIntersects(DemoFlags, flags, w_mask), false);
    bitfieldInsert(DemoFlags, *flags, w_mask);
    expect_eq(bitfieldContains(DemoFlags, flags, DemoFlags{ read: true, write: true }), true);
    expect_eq(asInt(DemoFlags, flags), 0b011 as u8);
    bitfieldRemove(DemoFlags, *flags, DemoFlags{ read: true });
    expect_eq(flags.read, false);
    expect_eq(flags.write, true);
    const restored = fromInt(DemoFlags, 0b101);
    expect_eq(restored.read, true);
    expect_eq(restored.exec, true);
}

// ============================================================================
// 2. cap:meta — Compile-Time Type Traits & Introspection
// ============================================================================
type U32Slice = []u32;
type U8Slice = []u8;
type U32Ptr = *u32;
type ByteArr4 = [4]u8;
type ByteArr16 = [16]u8;
type PtrU16Arr8 = *[8]u16;
type OptU32 = u32?;
type OptU64 = u64?;

fn testMetaTraits(): ! {
    comptime {
        if (!isInt(i32) || !isInt(u64) || isInt(f64)) @compileError("isInt failed");
        if (!isFloat(f32) || !isFloat(f64) || isFloat(i32)) @compileError("isFloat failed");
        if (!isNumber(u8) || !isNumber(f32) || isNumber(bool)) @compileError("isNumber failed");
        if (!isBool(bool) || isBool(u8)) @compileError("isBool failed");
        if (!isSlice(string) || !isSlice(U32Slice) || isSlice(ByteArr4)) @compileError("isSlice failed");
        if (!isPointer(U32Ptr) || !isPointer(U8Slice) || isPointer(u32)) @compileError("isPointer failed");
        if (!isSingleItemPtr(U32Ptr) || isSingleItemPtr(U32Slice)) @compileError("isSingleItemPtr failed");
        if (!isConstPtr(string) || isConstPtr(U8Slice)) @compileError("isConstPtr failed");
        if (!isOptional(OptU32) || isOptional(u32)) @compileError("isOptional failed");
        if (!isStruct(SampleRecord) || isStruct(SampleMode)) @compileError("isStruct failed");
        if (!isEnum(SampleMode) || isEnum(SampleRecord)) @compileError("isEnum failed");
        if (!isUnion(SamplePayload) || isUnion(SampleRecord)) @compileError("isUnion failed");
        if (!isArray(ByteArr16) || isArray(U8Slice)) @compileError("isArray failed");

        if (!hasField(SampleRecord, "id") || hasField(SampleRecord, "missing")) @compileError("hasField failed");
        if (!hasDecl(SampleRecord, "isReady") || hasDecl(SampleRecord, "nope")) @compileError("hasDecl failed");
        if (fieldCount(SampleRecord) != 3) @compileError("fieldCount failed");
        if (ChildType(string) != u8) @compileError("ChildType slice failed");
        if (ChildType(OptU64) != u64) @compileError("ChildType optional failed");
        if (ItemType(PtrU16Arr8) != u16) @compileError("ItemType ptr-array failed");
    }

    expect_eq(sizeOf(u64), 8);
    expect_eq(alignOf(u64) >= 4, true);
    expect_eq(fieldCount(SampleRecord), 3);
    expect_eq(std.mem.eql(u8, typeName(SampleRecord), "SampleRecord"), true);
    expect_eq(std.mem.eql(u8, typeName(SampleMode), "SampleMode"), true);
}

// ============================================================================
// 3. cap:platform — KernelVersion & PlatformInfo Live Syscalls
// ============================================================================
fn testPlatform(): ! {
    const kv1 = KernelVersion.parse("6.8.12-generic") catch unreachable;
    expect_eq(kv1.major, 6);
    expect_eq(kv1.minor, 8);
    expect_eq(kv1.patch, 12);
    expect_eq(kv1.isAtLeast(6, 8), true);
    expect_eq(kv1.isAtLeast(5, 1), true);
    expect_eq(kv1.isAtLeast(6, 9), false);
    expect_eq(kv1.isAtLeastPatch(6, 8, 12), true);
    expect_eq(kv1.isAtLeastPatch(6, 8, 13), false);

    const kv2 = KernelVersion.parse("5.15") catch unreachable;
    expect_eq(kv2.major, 5);
    expect_eq(kv2.minor, 15);
    expect_eq(kv2.patch, 0);
    expect_eq(kv1.compare(kv2), 1);
    expect_eq(kv2.compare(kv1), -1);
    expect_eq(kv1.compare(kv1), 0);

    let fmt_buf: [32]u8 = undefined;
    const kv_str = kv1.format(&fmt_buf) catch unreachable;
    expect_eq(std.mem.eql(u8, kv_str, "6.8.12"), true);

    let bad_kv = false;
    * = KernelVersion.parse("not-a-version") catch {
        bad_kv = true;
    };
    expect_eq(bad_kv, true);

    expect_eq(PlatformInfo.pageSize() >= 4096, true);
    expect_eq(PlatformInfo.cpuCount() >= 1, true);
    expect_eq(PlatformInfo.endianness(), Endianness.Little);

    const cur_os = PlatformInfo.currentOs();
    const cur_arch = PlatformInfo.currentArch();
    expect_eq(cur_os != OsKind.Unknown, true);
    expect_eq(cur_arch != ArchKind.Unknown, true);

    if (cur_os == OsKind.Linux) {
        const live_kv = PlatformInfo.kernelVersion() catch unreachable;
        expect_eq(live_kv.major >= 3, true);
        expect_eq(PlatformInfo.supportsCopyFileRange(), live_kv.isAtLeast(4, 5));
        expect_eq(PlatformInfo.supportsStatx(), live_kv.isAtLeast(4, 11));
        expect_eq(PlatformInfo.supportsIoUring(), live_kv.isAtLeast(5, 1));
        expect_eq(PlatformInfo.supportsPidfd(), live_kv.isAtLeast(5, 3));

        let exe_buf: [1024]u8 = undefined;
        const exe_path = PlatformInfo.executablePath(&exe_buf) catch unreachable;
        expect_eq(exe_path.len > 0, true);
        expect_eq(exe_path[0] == '/', true);

        let host_buf: [128]u8 = undefined;
        const host = PlatformInfo.hostname(&host_buf) catch unreachable;
        expect_eq(host.len > 0, true);
    }
}

// ============================================================================
// 4. cap:resolver — W3C SRI Integrity Verification & Multi-Algorithm Parsing
// ============================================================================
fn testIntegrity(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    const payload = "console.log('hello cp integrity');";
    const s256 = Integrity.computeSha256(payload);
    const s512 = Integrity.computeSha512(payload);
    const s384 = Integrity.computeSha384(payload);
    const s1 = Integrity.computeSha1(payload);

    expect_eq(s256.tag, IntegrityTag.Sha256);
    expect_eq(s512.tag, IntegrityTag.Sha512);
    expect_eq(s256.verify(payload), true);
    expect_eq(s512.verify(payload), true);
    expect_eq(s384.verify(payload), true);
    expect_eq(s1.verify(payload), true);
    expect_eq(s256.verify("tampered!"), false);
    expect_eq(s512.verify("tampered!"), false);

    // Format & Parse roundtrip
    let sri_buf: [256]u8 = undefined;
    const s256_str = s256.format(&sri_buf) catch unreachable;
    expect_eq(std.mem.startsWith(u8, s256_str, "sha256-"), true);
    const s512_alloc = s512.formatAlloc(alloc) catch unreachable;
    expect_eq(std.mem.startsWith(u8, s512_alloc, "sha512-"), true);

    const parsed_256 = Integrity.parse(s256_str) catch unreachable;
    expect_eq(parsed_256.eql(s256), true);
    expect_eq(parsed_256.verify(payload), true);

    // parseMultiple: space-separated with unknown algo + weaker sha256 + stronger sha512
    let combined: []const u8 = "";
    zig {
        combined = std.fmt.allocPrint(alloc, "sha999-unknown {s} {s}", .{ s256_str, s512_alloc }) catch unreachable;
    }
    const strongest = Integrity.parseMultiple(combined) catch unreachable;
    expect_eq(strongest.tag, IntegrityTag.Sha512);
    expect_eq(strongest.eql(s512), true);
    expect_eq(strongest.verify(payload), true);

    // Invalid inputs
    let bad_sri = false;
    * = Integrity.parse("sha256-not_valid_length") catch {
        bad_sri = true;
    };
    expect_eq(bad_sri, true);

    let unknown_only = false;
    * = Integrity.parseMultiple("sha3-abcdef") catch {
        unknown_only = true;
    };
    expect_eq(unknown_only, true);
}

// ============================================================================
// 5. cap:resolver — HostedGitInfo Parsing & URL Formatting
// ============================================================================
fn testHostedGit(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 1. "github:user/repo#v1.0"
    const g1 = HostedGitInfo.parse("github:user/repo#v1.0") catch unreachable;
    expect_eq(g1.host, GitHost.GitHub);
    expect_eq(std.mem.eql(u8, g1.user, "user"), true);
    expect_eq(std.mem.eql(u8, g1.repo, "repo"), true);
    expect_eq(std.mem.eql(u8, g1.committish.?, "v1.0"), true);
    expect_eq(std.mem.eql(u8, g1.httpsUrlAlloc(alloc) catch unreachable, "https://github.com/user/repo.git#v1.0"), true);
    expect_eq(std.mem.eql(u8, g1.tarballUrlAlloc(alloc) catch unreachable, "https://codeload.github.com/user/repo/tar.gz/v1.0"), true);
    expect_eq(std.mem.eql(u8, g1.sshUrlAlloc(alloc) catch unreachable, "git@github.com:user/repo.git#v1.0"), true);

    // 2. "user/repo#main"
    const g2 = HostedGitInfo.parse("user/repo#main") catch unreachable;
    expect_eq(g2.host, GitHost.GitHub);
    expect_eq(std.mem.eql(u8, g2.user, "user"), true);
    expect_eq(std.mem.eql(u8, g2.repo, "repo"), true);
    expect_eq(std.mem.eql(u8, g2.committish.?, "main"), true);

    // 3. "https://github.com/user/repo.git#commit"
    const g3 = HostedGitInfo.parse("https://github.com/user/repo.git#commit") catch unreachable;
    expect_eq(g3.host, GitHost.GitHub);
    expect_eq(std.mem.eql(u8, g3.user, "user"), true);
    expect_eq(std.mem.eql(u8, g3.repo, "repo"), true);
    expect_eq(std.mem.eql(u8, g3.committish.?, "commit"), true);

    // 4. "git+ssh://git@gitlab.com/org/proj.git"
    const g4 = HostedGitInfo.parse("git+ssh://git@gitlab.com/org/proj.git") catch unreachable;
    expect_eq(g4.host, GitHost.GitLab);
    expect_eq(std.mem.eql(u8, g4.user, "org"), true);
    expect_eq(std.mem.eql(u8, g4.repo, "proj"), true);
    expect_eq(g4.committish == null, true);
    expect_eq(std.mem.eql(u8, g4.httpsUrlAlloc(alloc) catch unreachable, "https://gitlab.com/org/proj.git"), true);
    expect_eq(std.mem.eql(u8, g4.tarballUrlAlloc(alloc) catch unreachable, "https://gitlab.com/org/proj/-/archive/HEAD/proj-HEAD.tar.gz"), true);
    expect_eq(std.mem.eql(u8, g4.sshUrlAlloc(alloc) catch unreachable, "git@gitlab.com:org/proj.git"), true);

    // 5. SCP-style, Bitbucket, Gist, SourceHut
    const g5 = HostedGitInfo.parse("git@bitbucket.org:acme/tool.git#v2") catch unreachable;
    expect_eq(g5.host, GitHost.Bitbucket);
    expect_eq(std.mem.eql(u8, g5.user, "acme"), true);
    expect_eq(std.mem.eql(u8, g5.repo, "tool"), true);
    expect_eq(std.mem.eql(u8, g5.tarballUrlAlloc(alloc) catch unreachable, "https://bitbucket.org/acme/tool/get/v2.tar.gz"), true);

    const g6 = HostedGitInfo.parse("gist:deadbeef1234#rev1") catch unreachable;
    expect_eq(g6.host, GitHost.Gist);
    expect_eq(std.mem.eql(u8, g6.repo, "deadbeef1234"), true);
    expect_eq(std.mem.eql(u8, g6.tarballUrlAlloc(alloc) catch unreachable, "https://codeload.github.com/gist/deadbeef1234/tar.gz/rev1"), true);

    const g7 = HostedGitInfo.parse("sourcehut:~alice/core#v0.9") catch unreachable;
    expect_eq(g7.host, GitHost.SourceHut);
    expect_eq(std.mem.eql(u8, g7.user, "~alice"), true);
    expect_eq(std.mem.eql(u8, g7.repo, "core"), true);
    expect_eq(std.mem.eql(u8, g7.tarballUrlAlloc(alloc) catch unreachable, "https://git.sr.ht/~alice/core/archive/v0.9.tar.gz"), true);

    // Invalid URLs fail cleanly
    let bad_git = false;
    * = HostedGitInfo.parse("./local/path") catch {
        bad_git = true;
    };
    expect_eq(bad_git, true);
}

pub fn main() {
    test("1. cap:meta Bit Manipulation, Alignment & Integer Packing", () => {
        try testMetaBits();
    });
    test("2. cap:meta Compile-Time Type Traits & Introspection", () => {
        try testMetaTraits();
    });
    test("3. cap:platform KernelVersion & PlatformInfo Live Syscalls", () => {
        try testPlatform();
    });
    test("4. cap:resolver W3C SRI Integrity Verification & Multi-Algorithm Parsing", () => {
        try testIntegrity();
    });
    test("5. cap:resolver HostedGitInfo Parsing & URL Formatting", () => {
        try testHostedGit();
    });
}
