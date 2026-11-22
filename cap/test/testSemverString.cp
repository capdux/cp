// cap/test/testSemverString.cp
// Production-grade deep tests for cap:semver semverString.cp
// (8-byte inline/pooled SemverString, SemverExternalString, SemverStringPool)

import { test, assert, assertEq } from "cap:test";
import {
    SEMVER_STRING_MAX_INLINE_LEN,
    SEMVER_POOL_MAX_BYTES,
    SEMVER_POOL_MAX_ENTRIES,
    SemverStringError,
    SemverStringTag,
    SemverStringPointer,
    semverStringHash,
    SemverString,
    SemverExternalString,
    SemverStringPool,
} from "cap:semver";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SemverString: inline capacity boundary (0..8 bytes & bit-63 high-bit guard)", () => {
    assertEq(SEMVER_STRING_MAX_INLINE_LEN, @as(usize, 8));

    const empty = SemverString.empty();
    assert(empty.isInline());
    assert(empty.tag() == SemverStringTag.small);
    assert(empty.isEmpty());
    assertEq(empty.len(), @as(usize, 0));

    // 8-byte ASCII fits inline
    assert(SemverString.canInline("1.2.3-rc"));
    const s8 = SemverString.initInline("1.2.3-rc") catch unreachable;
    assert(s8.isInline());
    assert(!s8.isEmpty());
    assertEq(s8.len(), @as(usize, 8));

    let out: [32]u8 = [0; 32];
    const n8 = s8.copyTo("", out[0..]) catch unreachable;
    assert(eqStr(out[0..n8], "1.2.3-rc"));

    // 9-byte string cannot be inlined
    assert(!SemverString.canInline("1.2.3-rc1"));

    // 8-byte string with byte[7] >= 0x80 cannot be inlined (bit 63 reserved for big tag)
    let hi8: [8]u8 = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 0x80];
    assert(!SemverString.canInline(hi8[0..]));

    // Embedded NUL byte cannot be inlined
    let nul_str: [4]u8 = ['1', 0, '2', '3'];
    assert(!SemverString.canInline(nul_str[0..]));
});

test("SemverStringPool: zero-alloc inline bypass & external hash deduplication", () => {
    let pool = SemverStringPool.init();
    assertEq(pool.usedBytes(), @as(usize, 0));
    assertEq(pool.dedupCount(), @as(usize, 0));

    // Inline strings do not consume pool arena space
    const v1 = pool.intern("1.0.0") catch unreachable;
    assert(v1.isInline());
    assertEq(pool.usedBytes(), @as(usize, 0));
    assertEq(pool.dedupCount(), @as(usize, 0));

    // External strings are stored once and deduplicated on repeat intern
    const long1 = pool.intern("@scope/my-package@1.2.3-beta.1") catch unreachable;
    assert(!long1.isInline());
    assert(long1.tag() == SemverStringTag.big);
    assertEq(pool.dedupCount(), @as(usize, 1));
    const used_after_first = pool.usedBytes();
    assertEq(used_after_first, @as(usize, 30));

    const long1_dup = pool.intern("@scope/my-package@1.2.3-beta.1") catch unreachable;
    assertEq(pool.dedupCount(), @as(usize, 1));
    assertEq(pool.usedBytes(), used_after_first);
    assertEq(long1.ptr().off, long1_dup.ptr().off);
    assertEq(long1.ptr().len, long1_dup.ptr().len);

    // Distinct long string allocates new slot
    const long2 = pool.intern("@scope/other-pkg@2.0.0") catch unreachable;
    assertEq(pool.dedupCount(), @as(usize, 2));
    assert(long2.ptr().off == @as(u32, @intCast(used_after_first)));

    let out: [64]u8 = [0; 64];
    const n1 = pool.resolve(long1, out[0..]) catch unreachable;
    assert(eqStr(out[0..n1], "@scope/my-package@1.2.3-beta.1"));
});

test("SemverString: cross-pool equality & lexicographical ordering", () => {
    let pool_a = SemverStringPool.init();
    let pool_b = SemverStringPool.init();

    // Put padding in pool_b so offsets differ for the same external string
    * = pool_b.intern("padding-entry-for-different-offset") catch unreachable;

    const a_pkg = pool_a.intern("lodash-es@4.17.21") catch unreachable;
    const b_pkg = pool_b.intern("lodash-es@4.17.21") catch unreachable;
    assert(a_pkg.ptr().off != b_pkg.ptr().off);
    assert(a_pkg.eql(b_pkg, pool_a.bytes(), pool_b.bytes()));
    assertEq(a_pkg.order(b_pkg, pool_a.bytes(), pool_b.bytes()), 0);

    const s_alpha = pool_a.intern("alpha") catch unreachable;
    const s_beta = pool_b.intern("beta") catch unreachable;
    assert(!s_alpha.eql(s_beta, pool_a.bytes(), pool_b.bytes()));
    assertEq(s_alpha.order(s_beta, pool_a.bytes(), pool_b.bytes()), -1);
    assertEq(s_beta.order(s_alpha, pool_b.bytes(), pool_a.bytes()), 1);

    // Inline vs external comparison
    const s_short = pool_a.intern("lodash") catch unreachable;
    assert(!s_short.eql(a_pkg, pool_a.bytes(), pool_a.bytes()));
    assertEq(s_short.order(a_pkg, pool_a.bytes(), pool_a.bytes()), -1);
    assertEq(a_pkg.order(s_short, pool_a.bytes(), pool_a.bytes()), 1);
});

test("SemverExternalString: deterministic FNV-1a hash & O(1) eqlFast", () => {
    let pool = SemverStringPool.init();

    const ext1 = pool.internExternal("typescript@5.8.0-dev.20250101") catch unreachable;
    const ext2 = pool.internExternal("typescript@5.8.0-dev.20250101") catch unreachable;
    const ext3 = pool.internExternal("typescript@5.8.0-dev.20250102") catch unreachable;

    assertEq(ext1.hash, semverStringHash("typescript@5.8.0-dev.20250101"));
    assertEq(ext1.hash, ext2.hash);
    assert(ext1.hash != ext3.hash);
    assert(ext1.eqlFast(ext2, pool.bytes(), pool.bytes()));
    assert(!ext1.eqlFast(ext3, pool.bytes(), pool.bytes()));

    const inl = SemverExternalString.initInline("1.2.3") catch unreachable;
    assert(inl.value.isInline());
    assertEq(inl.hash, semverStringHash("1.2.3"));
});

test("SemverString: formatStorePath escaping (/, \\, :, # -> +)", () => {
    let pool = SemverStringPool.init();
    let out: [64]u8 = [0; 64];

    const scoped = pool.intern("@types/node@https://example.com/a\\b#v1") catch unreachable;
    const n1 = scoped.formatStorePath(pool.bytes(), out[0..]) catch unreachable;
    assert(eqStr(out[0..n1], "@types+node@https+++example.com+a+b+v1"));

    // Inline string with special chars
    const short_sp = pool.intern("a/b:c#d") catch unreachable;
    assert(short_sp.isInline());
    const n2 = short_sp.formatStorePath(pool.bytes(), out[0..]) catch unreachable;
    assert(eqStr(out[0..n2], "a+b+c+d"));
});

test("SemverStringError: NotInlinable, InvalidPointer, BufferTooSmall & PoolBytesFull guards", () => {
    let not_inline_err = false;
    const r1 = SemverString.initInline("too-long-for-inline") catch |e| {
        if (e == SemverStringError.NotInlinable) not_inline_err = true;
        SemverString.empty()
    };
    * = r1;
    assert(not_inline_err);

    // Out-of-bounds external pointer fails safely
    const bad_ptr = SemverString.initPointer(100, 20) catch unreachable;
    let invalid_ptr_err = false;
    const r2 = bad_ptr.sliceExternal("short") catch |e| {
        if (e == SemverStringError.InvalidPointer) invalid_ptr_err = true;
        ""
    };
    * = r2;
    assert(invalid_ptr_err);

    // Length with bit 31 set rejected by initPointer
    let high_len_err = false;
    const r3 = SemverString.initPointer(0, 0x80000001) catch |e| {
        if (e == SemverStringError.InvalidPointer) high_len_err = true;
        SemverString.empty()
    };
    * = r3;
    assert(high_len_err);

    // BufferTooSmall on copyTo
    const s = SemverString.initInline("1.2.3") catch unreachable;
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    const r4 = s.copyTo("", tiny[0..]) catch |e| {
        if (e == SemverStringError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r4;
    assert(buf_err);

    // PoolBytesFull guard
    assertEq(SEMVER_POOL_MAX_BYTES, @as(usize, 4096));
    assertEq(SEMVER_POOL_MAX_ENTRIES, @as(usize, 128));
    let pool = SemverStringPool.init();
    let chunk: [512]u8 = ['x'; 512];
    let k: usize = 0;
    while (k < 8) {
        chunk[0] = @as(u8, @intCast('A' + k));
        * = pool.intern(chunk[0..]) catch unreachable;
        k += 1;
    }
    assertEq(pool.usedBytes(), @as(usize, 4096));
    chunk[0] = 'Z';
    let full_err = false;
    const r5 = pool.intern(chunk[0..]) catch |e| {
        if (e == SemverStringError.PoolBytesFull) full_err = true;
        SemverString.empty()
    };
    * = r5;
    assert(full_err);
});
