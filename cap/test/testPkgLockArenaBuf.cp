// cap/test/testPkgLockArenaBuf.cp — Deep production tests for cap:pkg lockArenaBuf
// Covers:
//   1. Open-addressing FNV-1a string interning deduplication & savings telemetry
//   2. Five-segment LockArenaBuffers (dependencies, resolutions, trees) & SIMD backslash guard
//   3. Legacy package_id -> dependency_id mapping with visited bitflags
//   4. Fragmentation compaction (`compactLockArenaBuffers`) reclaiming orphan string bytes
//   5. 8-byte aligned CRC32 binary image encode + zero-copy LockArenaImageView & hydration
//   6. 500-iteration deterministic malformed/truncated/misaligned/OOB-offset binary image Fuzz gate
//   7. SIMD string pool scan (`findInLockStringPool`) & diagnostic report formatting

import { expect, expectEqual, test } from "cap:test";
import {
    LOCK_ARENA_MAGIC,
    LOCK_ARENA_VERSION,
    LOCK_ARENA_HEADER_BYTES,
    LOCK_ARENA_ALIGNMENT,
    LOCK_ARENA_INVALID_ID,
    LOCK_ARENA_DEAD_SENTINEL,
    computeLockArenaCrc32,
    updateLockArenaCrc32,
    alignLockArenaOffset,
    isLockArenaAligned,
    hasWindowsPathSeparator,
    findInLockStringPool,
    LockExternString,
    LockArenaDepKind,
    LockArenaDep,
    LockArenaTreeNode,
    LockArenaBuffers,
    compactLockArenaBuffers,
    encodedLockArenaImageSize,
    encodeLockArenaImage,
    LockArenaImageView,
    decodeLockArenaImageInto,
    formatLockArenaReport,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("lockArenaBuf: open-addressing FNV-1a string interning deduplication & savings", () => {
    let str_pool: [512]u8 = [0; 512];
    let buckets: [32]u32 = [0; 32];
    let exts: [16]LockExternString = [LockExternString.empty(); 16];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];

    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);

    const r1 = arena.internString("https://registry.npmjs.org/typescript/-/typescript-5.6.2.tgz") catch unreachable;
    const r2 = arena.internString("MIT") catch unreachable;
    const r3 = arena.internString("https://registry.npmjs.org/typescript/-/typescript-5.6.2.tgz") catch unreachable;
    const r4 = arena.internString("MIT") catch unreachable;
    const r_empty = arena.internString("") catch unreachable;

    expect(r1.eql(r3));
    expect(r2.eql(r4));
    expect(r_empty.isEmpty());

    const s1 = arena.resolveString(r1) catch unreachable;
    const s2 = arena.resolveString(r2) catch unreachable;
    expect(strEql(s1, "https://registry.npmjs.org/typescript/-/typescript-5.6.2.tgz"));
    expect(strEql(s2, "MIT"));

    const st = arena.stats();
    expectEqual(st.extern_strings_count, @as(usize, 2));
    expectEqual(st.string_bytes_used, @as(usize, 60 + 3));
    expectEqual(st.intern_requests, @as(u64, 5));
    expectEqual(st.intern_hits, @as(u64, 3)); // r3, r4, empty
    expectEqual(st.dedup_saved_bytes, @as(u64, 60 + 3));
});

test("lockArenaBuf: five-segment arena operations & SIMD Windows path guard", () => {
    let str_pool: [512]u8 = [0; 512];
    let buckets: [32]u32 = [0; 32];
    let exts: [16]LockExternString = [LockExternString.empty(); 16];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];

    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);

    const d0 = arena.addDependency("react", "^19.0.0", LockArenaDepKind.Npm, 0x01, 0, 10) catch unreachable;
    const d1 = arena.addDependency("react", "^19.0.0", LockArenaDepKind.Npm, 0x02, 1, 10) catch unreachable;
    const d2 = arena.addDependency("pkg-a", "packages/pkg-a", LockArenaDepKind.Workspace, 0x04, 0, 11) catch unreachable;
    expectEqual(d0, @as(u32, 0));
    expectEqual(d1, @as(u32, 1));
    expectEqual(d2, @as(u32, 2));

    // Reject Windows backslash separator on workspace/folder/symlink deps
    expect(hasWindowsPathSeparator("packages\\pkg-a"));
    expect(!hasWindowsPathSeparator("packages/pkg-a"));
    let win_err = false;
    * = arena.addDependency("bad", "packages\\bad", LockArenaDepKind.Folder, 0, 0, 12) catch {
        win_err = true;
    };
    expect(win_err);

    // Add hoisted tree nodes
    const t0 = arena.addTreeNode(LOCK_ARENA_INVALID_ID, 0, 0, 3) catch unreachable;
    const t1 = arena.addTreeNode(t0, 2, 1, 1) catch unreachable;
    expectEqual(t0, @as(u32, 0));
    expectEqual(t1, @as(u32, 1));

    // Update resolution in place
    arena.setResolution(d1, 15) catch unreachable;
    expectEqual(arena.resolutions[1], @as(u32, 15));

    const st = arena.stats();
    expectEqual(st.dependencies_count, @as(usize, 3));
    expectEqual(st.resolutions_count, @as(usize, 3));
    expectEqual(st.trees_count, @as(usize, 2));
    // "react" and "^19.0.0" were deduplicated on d1
    expectEqual(st.extern_strings_count, @as(usize, 4));
});

test("lockArenaBuf: legacyPackageToDependencyId with visited tracking", () => {
    let str_pool: [256]u8 = [0; 256];
    let buckets: [16]u32 = [0; 16];
    let exts: [8]LockExternString = [LockExternString.empty(); 8];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];

    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);
    * = arena.addDependency("a", "1.0.0", LockArenaDepKind.Npm, 0, 0, 42) catch unreachable;
    * = arena.addDependency("b", "1.0.0", LockArenaDepKind.Npm, 0, 0, 99) catch unreachable;
    * = arena.addDependency("a-dup", "1.0.0", LockArenaDepKind.Npm, 0, 1, 42) catch unreachable;

    let visited: [8]u8 = [0; 8];
    const first = arena.legacyPackageToDependencyId(42, visited[0..]) catch unreachable;
    const second = arena.legacyPackageToDependencyId(42, visited[0..]) catch unreachable;
    expectEqual(first, @as(u32, 0));
    expectEqual(second, @as(u32, 2));

    // Third lookup for pkg 42 has no unvisited dep slots -> MissingResolution
    let missing = false;
    * = arena.legacyPackageToDependencyId(42, visited[0..]) catch {
        missing = true;
    };
    expect(missing);
    expectEqual(arena.legacyPackageToDependencyId(0, visited[0..]) catch unreachable, @as(u32, 0));
    expectEqual(arena.legacyPackageToDependencyId(LOCK_ARENA_INVALID_ID, visited[0..]) catch unreachable, LOCK_ARENA_INVALID_ID);
});

test("lockArenaBuf: fragmentation compaction reclaims orphan interned strings", () => {
    let src_str: [512]u8 = [0; 512];
    let src_bkt: [32]u32 = [0; 32];
    let src_ext: [16]LockExternString = [LockExternString.empty(); 16];
    let src_dep: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let src_res: [8]u32 = [0; 8];
    let src_tr: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let src = LockArenaBuffers.init(src_str[0..], src_bkt[0..], src_ext[0..], src_dep[0..], src_res[0..], src_tr[0..]);

    // Intern orphan/dead strings not referenced by any dependency
    * = src.internString("orphan-unused-tarball-url-that-wastes-space.tgz") catch unreachable;
    * = src.internString("another-dead-license-metadata-string") catch unreachable;
    // Add live dependencies
    * = src.addDependency("zod", "^3.23.8", LockArenaDepKind.Npm, 1, 0, 1) catch unreachable;
    * = src.addDependency("zod", "^3.23.8", LockArenaDepKind.Npm, 2, 1, 1) catch unreachable;
    * = src.addTreeNode(LOCK_ARENA_INVALID_ID, 0, 0, 2) catch unreachable;

    let dst_str: [512]u8 = [0; 512];
    let dst_bkt: [32]u32 = [0; 32];
    let dst_ext: [16]LockExternString = [LockExternString.empty(); 16];
    let dst_dep: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let dst_res: [8]u32 = [0; 8];
    let dst_tr: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let dst = LockArenaBuffers.init(dst_str[0..], dst_bkt[0..], dst_ext[0..], dst_dep[0..], dst_res[0..], dst_tr[0..]);

    const cstat = compactLockArenaBuffers(&src, &dst) catch unreachable;
    expectEqual(cstat.before_extern_strings, @as(usize, 4));
    expectEqual(cstat.after_extern_strings, @as(usize, 2));
    expectEqual(cstat.reclaimed_string_bytes, @as(usize, 47 + 36));
    expectEqual(cstat.live_dependencies, @as(usize, 2));
    expectEqual(cstat.live_trees, @as(usize, 1));
    expectEqual(cstat.dedup_saved_bytes, @as(u64, 3 + 7)); // "zod" + "^3.23.8"

    const d0_name = dst.resolveString(dst.dependencies[0].name_ref) catch unreachable;
    const d1_spec = dst.resolveString(dst.dependencies[1].spec_ref) catch unreachable;
    expect(strEql(d0_name, "zod"));
    expect(strEql(d1_spec, "^3.23.8"));
});

test("lockArenaBuf: 8-byte aligned CRC32 binary image encode, zero-copy view & hydration", () => {
    // IEEE 802.3 CRC32 standard vector check
    expectEqual(computeLockArenaCrc32("123456789"), @as(u32, 0xCBF43926));
    expectEqual(updateLockArenaCrc32(updateLockArenaCrc32(0, "123"), "456789"), @as(u32, 0xCBF43926));
    expectEqual(alignLockArenaOffset(105, 8), @as(usize, 112));
    expect(isLockArenaAligned(LOCK_ARENA_HEADER_BYTES, LOCK_ARENA_ALIGNMENT));

    let str_pool: [512]u8 = [0; 512];
    let buckets: [32]u32 = [0; 32];
    let exts: [16]LockExternString = [LockExternString.empty(); 16];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);

    * = arena.addDependency("esbuild", "0.24.0", LockArenaDepKind.Npm, 0x05, 0, 100) catch unreachable;
    * = arena.addDependency("cap-core", "libs/core", LockArenaDepKind.Workspace, 0x01, 0, 101) catch unreachable;
    * = arena.addTreeNode(LOCK_ARENA_INVALID_ID, 0, 0, 2) catch unreachable;
    * = arena.addTreeNode(0, 1, 2, 0) catch unreachable;

    let img: [1024]u8 = [0; 1024];
    const expected_sz = encodedLockArenaImageSize(&arena);
    expect(isLockArenaAligned(expected_sz, LOCK_ARENA_ALIGNMENT));
    const written = encodeLockArenaImage(&arena, img[0..]) catch unreachable;
    expectEqual(written, expected_sz);
    expectEqual(img[0], LOCK_ARENA_MAGIC[0]);
    expectEqual(img[3], LOCK_ARENA_MAGIC[3]);

    // Open zero-copy read-only view directly over serialized bytes
    const view = LockArenaImageView.open(img[0..written]) catch unreachable;
    expectEqual(view.version, LOCK_ARENA_VERSION);
    expectEqual(view.externStringCount(), @as(usize, 4));
    expectEqual(view.dependencyCount(), @as(usize, 2));
    expectEqual(view.resolutionCount(), @as(usize, 2));
    expectEqual(view.treeCount(), @as(usize, 2));

    const d0 = view.getDependency(0) catch unreachable;
    const d1 = view.getDependency(1) catch unreachable;
    expect(strEql(view.resolveString(d0.name_ref) catch unreachable, "esbuild"));
    expect(strEql(view.resolveString(d1.spec_ref) catch unreachable, "libs/core"));
    expect(d1.kind == LockArenaDepKind.Workspace);
    expectEqual(view.getResolution(0) catch unreachable, @as(u32, 100));
    expectEqual(view.getResolution(1) catch unreachable, @as(u32, 101));
    const tn1 = view.getTreeNode(1) catch unreachable;
    expectEqual(tn1.parent_tree_id, @as(u32, 0));
    expectEqual(tn1.dep_id, @as(u32, 1));
    expect(view.containsString("esbuild"));
    expect(!view.containsString("nonexistent-pkg"));

    // Hydrate into a fresh mutable LockArenaBuffers
    let h_str: [512]u8 = [0; 512];
    let h_bkt: [32]u32 = [0; 32];
    let h_ext: [16]LockExternString = [LockExternString.empty(); 16];
    let h_dep: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let h_res: [8]u32 = [0; 8];
    let h_tr: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let hydrated = LockArenaBuffers.init(h_str[0..], h_bkt[0..], h_ext[0..], h_dep[0..], h_res[0..], h_tr[0..]);

    const summary = decodeLockArenaImageInto(img[0..written], &hydrated) catch unreachable;
    expectEqual(summary.dependencies_count, @as(usize, 2));
    expectEqual(summary.trees_count, @as(usize, 2));
    expectEqual(hydrated.dependencies_len, @as(usize, 2));
    expectEqual(hydrated.resolutions[0], @as(u32, 100));
});

test("lockArenaBuf: 500-iteration deterministic malformed/OOB-offset binary image Fuzz gate", () => {
    // Build a valid baseline image
    let str_pool: [512]u8 = [0; 512];
    let buckets: [32]u32 = [0; 32];
    let exts: [16]LockExternString = [LockExternString.empty(); 16];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);

    * = arena.addDependency("lodash", "^4.17.21", LockArenaDepKind.Npm, 1, 0, 10) catch unreachable;
    * = arena.addDependency("zod", "^3.23.8", LockArenaDepKind.Npm, 1, 0, 11) catch unreachable;
    * = arena.addTreeNode(LOCK_ARENA_INVALID_ID, 0, 0, 2) catch unreachable;

    let valid_img: [1024]u8 = [0; 1024];
    const valid_len = encodeLockArenaImage(&arena, valid_img[0..]) catch unreachable;

    let scratch: [1024]u8 = [0; 1024];
    let rng: u64 = 0xCAFE12349876BEEF;
    let rejected: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        let c: usize = 0;
        while (c < valid_len) {
            scratch[c] = valid_img[c];
            c += 1;
        }

        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const r = rng *% 0x2545F4914F6CDD1D;
        const mode = iter % 6;
        let cur_len = valid_len;

        if (mode == 0) {
            // Truncated image
            cur_len = @as(usize, @intCast(r % @as(u64, valid_len)));
        } else if (mode == 1) {
            // Single bit flip anywhere
            const idx = @as(usize, @intCast(r % @as(u64, valid_len)));
            scratch[idx] ^= @as(u8, 1) << @as(u3, @truncate(r >> 8));
        } else if (mode == 2) {
            // Corrupt section offset to 0xDEADBEEF or misaligned offset and recompute CRC32
            const sec_idx = @as(usize, @intCast(r % 5));
            const off_pos = 24 + sec_idx * 16;
            const bad_off: u64 = if ((r & 1) == 0) LOCK_ARENA_DEAD_SENTINEL else @as(u64, LOCK_ARENA_HEADER_BYTES + 3);
            scratch[off_pos + 0] = @as(u8, @truncate(bad_off));
            scratch[off_pos + 1] = @as(u8, @truncate(bad_off >> 8));
            scratch[off_pos + 2] = @as(u8, @truncate(bad_off >> 16));
            scratch[off_pos + 3] = @as(u8, @truncate(bad_off >> 24));
            scratch[off_pos + 4] = 0;
            scratch[off_pos + 5] = 0;
            scratch[off_pos + 6] = 0;
            scratch[off_pos + 7] = 0;
            let ncrc = updateLockArenaCrc32(0, scratch[0..12]);
            ncrc = updateLockArenaCrc32(ncrc, scratch[16..cur_len]);
            scratch[12] = @as(u8, @truncate(ncrc));
            scratch[13] = @as(u8, @truncate(ncrc >> 8));
            scratch[14] = @as(u8, @truncate(ncrc >> 16));
            scratch[15] = @as(u8, @truncate(ncrc >> 24));
        } else if (mode == 3) {
            // Out-of-bounds extern_string handle length inside section 1, with valid CRC32
            const s1_off = @as(usize, @intCast(
                @as(u64, scratch[24 + 16]) | (@as(u64, scratch[24 + 17]) << 8),
            ));
            scratch[s1_off + 4] = 0xFF;
            scratch[s1_off + 5] = 0xFF;
            let ncrc = updateLockArenaCrc32(0, scratch[0..12]);
            ncrc = updateLockArenaCrc32(ncrc, scratch[16..cur_len]);
            scratch[12] = @as(u8, @truncate(ncrc));
            scratch[13] = @as(u8, @truncate(ncrc >> 8));
            scratch[14] = @as(u8, @truncate(ncrc >> 16));
            scratch[15] = @as(u8, @truncate(ncrc >> 24));
        } else if (mode == 4) {
            // Corrupt dep_kind enum byte with valid CRC32
            const s2_off = @as(usize, @intCast(
                @as(u64, scratch[24 + 32]) | (@as(u64, scratch[24 + 33]) << 8),
            ));
            scratch[s2_off + 18] = 200;
            let ncrc = updateLockArenaCrc32(0, scratch[0..12]);
            ncrc = updateLockArenaCrc32(ncrc, scratch[16..cur_len]);
            scratch[12] = @as(u8, @truncate(ncrc));
            scratch[13] = @as(u8, @truncate(ncrc >> 8));
            scratch[14] = @as(u8, @truncate(ncrc >> 16));
            scratch[15] = @as(u8, @truncate(ncrc >> 24));
        } else {
            // Break parallel length invariant (deps.elem_count != resolutions.elem_count) with valid CRC32
            scratch[24 + 48 + 12] = 99;
            let ncrc = updateLockArenaCrc32(0, scratch[0..12]);
            ncrc = updateLockArenaCrc32(ncrc, scratch[16..cur_len]);
            scratch[12] = @as(u8, @truncate(ncrc));
            scratch[13] = @as(u8, @truncate(ncrc >> 8));
            scratch[14] = @as(u8, @truncate(ncrc >> 16));
            scratch[15] = @as(u8, @truncate(ncrc >> 24));
        }

        let failed = false;
        * = LockArenaImageView.open(scratch[0..cur_len]) catch {
            failed = true;
        };
        expect(failed);
        if (failed) rejected += 1;
        iter += 1;
    }

    expectEqual(rejected, @as(usize, 500));
    // Original valid image still opens cleanly
    const ok_view = LockArenaImageView.open(valid_img[0..valid_len]) catch unreachable;
    expectEqual(ok_view.dependencyCount(), @as(usize, 2));
});

test("lockArenaBuf: SIMD string pool scan & diagnostic report formatting", () => {
    const pool = "react^19.0.0typescript^5.6.2MIT";
    const pos_ts = findInLockStringPool(pool, "typescript") ?? 999;
    expectEqual(pos_ts, @as(usize, 12));
    expect(findInLockStringPool(pool, "svelte") == null);

    let str_pool: [256]u8 = [0; 256];
    let buckets: [16]u32 = [0; 16];
    let exts: [8]LockExternString = [LockExternString.empty(); 8];
    let deps: [8]LockArenaDep = [LockArenaDep.empty(); 8];
    let res: [8]u32 = [0; 8];
    let trees: [4]LockArenaTreeNode = [LockArenaTreeNode.empty(); 4];
    let arena = LockArenaBuffers.init(str_pool[0..], buckets[0..], exts[0..], deps[0..], res[0..], trees[0..]);

    * = arena.addDependency("zod", "^3.23.8", LockArenaDepKind.Npm, 1, 0, 1) catch unreachable;
    * = arena.addDependency("zod", "^3.23.8", LockArenaDepKind.Npm, 1, 1, 1) catch unreachable;
    * = arena.addTreeNode(LOCK_ARENA_INVALID_ID, 0, 0, 2) catch unreachable;

    let rep: [256]u8 = [0; 256];
    const n = formatLockArenaReport(&arena, rep[0..]) catch unreachable;
    const text = rep[0..n];
    expect(containsSubstr(text, "LockArena(strings=2"));
    expect(containsSubstr(text, "saved=10B(50%)"));
    expect(containsSubstr(text, "deps=2"));
    expect(containsSubstr(text, "trees=1"));
});
