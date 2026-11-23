// cap/test/testBundlerDiskCache.cp — Deep production tests for cap:bundler diskCache
// Covers:
//   1. Deterministic 128-bit build cache key derivation, avalanche sensitivity & shard paths
//   2. SIMD-accelerated relative path normalization & traversal rejection
//   3. Zero-heap BuildDiskCacheIndex lookup validation (mtime, size, ABI, TTL, LRU touch)
//   4. Targeted invalidation (single key, source_hash, domain, compiler ABI) & upsert updates
//   5. Deterministic LRU + TTL + byte/entry quota eviction planning & pinned protection
//   6. IEEE 802.3 CRC32 verification & binary catalog snapshot round-trip encode/decode
//   7. 500-iteration deterministic malformed/truncated/bit-flipped binary index Fuzz gate
//   8. SIMD 256-shard distribution analysis & human-readable diagnostic report

import { expect, expectEqual, test } from "cap:test";
import {
    DISK_CACHE_INDEX_MAGIC,
    DISK_CACHE_INDEX_VERSION,
    DISK_CACHE_HEADER_BYTES,
    DISK_CACHE_RECORD_BYTES,
    DISK_CACHE_KEY_HEX_LEN,
    DiskCacheError,
    updateDiskCacheCrc32,
    computeDiskCacheCrc32,
    computeDiskCacheHash64,
    normalizeCacheRelPath,
    isValidCacheKeyHex,
    DiskCacheDomain,
    diskCacheDomainName,
    BuildCacheKeyInput,
    BuildCacheKey,
    computeBuildCacheKey,
    parseBuildCacheKeyHex,
    parseShardCachePath,
    DiskCacheEntry,
    DiskCacheLookupQuery,
    DiskCacheMissReason,
    DiskCacheEvictionPolicy,
    DiskCacheEvictionReason,
    DiskCacheEvictionItem,
    BuildDiskCacheIndex,
    encodedDiskCacheIndexSize,
    encodeDiskCacheIndex,
    decodeDiskCacheIndex,
    formatDiskCacheReport,
} from "cap:bundler";

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

test("diskCache: deterministic 128-bit key derivation, avalanche & two-level shard path", () => {
    const base_in = BuildCacheKeyInput{
        domain: DiskCacheDomain.TypeScript,
        source_path: "src/components/button.tsx",
        source_hash: 0x123456789ABCDEF0,
        dep_tree_hash: 0x0FEDCBA987654321,
        compiler_version: "cpc-1.2.0",
        compiler_abi: 14,
        target_name: "linux-x86_64",
        define_hash: 0xDEADBEEF,
        loader_flags: 0x03,
        minify: true,
        sourcemap: false,
    };

    const k1 = computeBuildCacheKey(base_in) catch unreachable;
    const k2 = computeBuildCacheKey(base_in) catch unreachable;
    expect(k1.eql(k2));
    expect(!k1.isZero());
    expectEqual(k1.shortHash64(), k2.shortHash64());

    // Changing any single field must produce a distinct 128-bit key
    let mod_dep = base_in;
    mod_dep.dep_tree_hash ^= 1;
    const k_dep = computeBuildCacheKey(mod_dep) catch unreachable;
    expect(!k1.eql(k_dep));

    let mod_target = base_in;
    mod_target.target_name = "macos-aarch64";
    const k_target = computeBuildCacheKey(mod_target) catch unreachable;
    expect(!k1.eql(k_target));

    let mod_define = base_in;
    mod_define.define_hash ^= 0x10;
    const k_define = computeBuildCacheKey(mod_define) catch unreachable;
    expect(!k1.eql(k_define));

    let mod_min = base_in;
    mod_min.minify = false;
    const k_min = computeBuildCacheKey(mod_min) catch unreachable;
    expect(!k1.eql(k_min));

    // Hex round-trip
    let hex_buf: [64]u8 = [0; 64];
    const hex_len = k1.formatHex(hex_buf[0..]) catch unreachable;
    expectEqual(hex_len, DISK_CACHE_KEY_HEX_LEN);
    expect(isValidCacheKeyHex(hex_buf[0..hex_len]));
    const parsed_hex = parseBuildCacheKeyHex(hex_buf[0..hex_len]) catch unreachable;
    expect(k1.eql(parsed_hex));

    // Two-level shard path round-trip: `ab/cd1234...bin`
    let shard_buf: [64]u8 = [0; 64];
    const shard_len = k1.formatShardPath(".bin", shard_buf[0..]) catch unreachable;
    expectEqual(shard_len, 39);
    expectEqual(shard_buf[2], '/');
    expectEqual(shard_buf[0], hex_buf[0]);
    expectEqual(shard_buf[1], hex_buf[1]);
    expect(strEql(shard_buf[35..39], ".bin"));

    const parsed_shard = parseShardCachePath(shard_buf[0..shard_len]) catch unreachable;
    expect(k1.eql(parsed_shard));

    // Tampered shard prefix must be rejected
    shard_buf[0] = if (shard_buf[0] == 'a') 'b' else 'a';
    let bad_shard = false;
    * = parseShardCachePath(shard_buf[0..shard_len]) catch {
        bad_shard = true;
    };
    expect(bad_shard);

    // Invalid empty inputs must error
    let empty_in = base_in;
    empty_in.compiler_version = "";
    let empty_err = false;
    * = computeBuildCacheKey(empty_in) catch {
        empty_err = true;
    };
    expect(empty_err);
});

test("diskCache: SIMD path normalization & traversal protection", () => {
    let out: [128]u8 = [0; 128];
    const n1 = normalizeCacheRelPath("src\\utils//./hash\\\\index.ts", out[0..]) catch unreachable;
    expect(strEql(out[0..n1], "src/utils/hash/index.ts"));

    const n2 = normalizeCacheRelPath("/leading/slash/file.cp", out[0..]) catch unreachable;
    expect(strEql(out[0..n2], "leading/slash/file.cp"));

    // Reject parent traversal `..`
    let rejected_trav = false;
    * = normalizeCacheRelPath("src/../secrets.env", out[0..]) catch {
        rejected_trav = true;
    };
    expect(rejected_trav);

    // Reject empty or dot-only path
    let rejected_dot = false;
    * = normalizeCacheRelPath("././", out[0..]) catch {
        rejected_dot = true;
    };
    expect(rejected_dot);

    // Reject non-ASCII hex keys via SIMD fast-path
    expect(!isValidCacheKeyHex("0123456789abcdef0123456789abcde\xff"));
    expect(!isValidCacheKeyHex("short"));
    expect(strEql(diskCacheDomainName(DiskCacheDomain.Macro), "macro"));
});

test("diskCache: BuildDiskCacheIndex lookup validation (ABI, mtime, size, TTL, LRU touch)", () => {
    let slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let idx = BuildDiskCacheIndex.init(slots[0..], 20);

    const key = computeBuildCacheKey(BuildCacheKeyInput{
        domain: DiskCacheDomain.JavaScript,
        source_path: "src/entry.js",
        source_hash: 0xAAA1,
        dep_tree_hash: 0xBBB1,
        compiler_version: "cpc-1.2.0",
        compiler_abi: 20,
        target_name: "linux-x86_64",
    }) catch unreachable;

    const slot = idx.upsert(DiskCacheEntry{
        key: key,
        domain: DiskCacheDomain.JavaScript,
        source_hash: 0xAAA1,
        dep_tree_hash: 0xBBB1,
        mtime_ns: 1700000000123,
        source_size: 4096,
        artifact_size: 8192,
        created_at_sec: 1000,
        last_access_sec: 1000,
        compiler_abi: 20,
        artifact_crc32: 0x11223344,
    }) catch unreachable;
    expectEqual(slot, @as(usize, 0));
    expectEqual(idx.count(), @as(usize, 1));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 8192));

    // Valid hit refreshes last_access_sec and hit_count
    const hit = idx.lookup(DiskCacheLookupQuery{
        key: key,
        expected_mtime_ns: 1700000000123,
        expected_source_size: 4096,
        expected_compiler_abi: 20,
        now_sec: 1200,
        ttl_secs: 600,
    });
    expect(hit.isHit());
    expectEqual(hit.entry.last_access_sec, @as(u64, 1200));
    expectEqual(hit.entry.hit_count, @as(u32, 1));

    // Compiler ABI mismatch
    const miss_abi = idx.lookup(DiskCacheLookupQuery{
        key: key,
        expected_mtime_ns: 1700000000123,
        expected_source_size: 4096,
        expected_compiler_abi: 21,
        now_sec: 1210,
        ttl_secs: 600,
    });
    expect(!miss_abi.isHit());
    expect(miss_abi.status == DiskCacheMissReason.CompilerAbiMismatch);

    // Mtime mismatch
    const miss_mtime = idx.lookup(DiskCacheLookupQuery{
        key: key,
        expected_mtime_ns: 1700000000999,
        expected_source_size: 4096,
        expected_compiler_abi: 20,
        now_sec: 1210,
        ttl_secs: 600,
    });
    expect(miss_mtime.status == DiskCacheMissReason.MtimeMismatch);

    // Source size mismatch
    const miss_size = idx.lookup(DiskCacheLookupQuery{
        key: key,
        expected_mtime_ns: 1700000000123,
        expected_source_size: 4097,
        expected_compiler_abi: 20,
        now_sec: 1210,
        ttl_secs: 600,
    });
    expect(miss_size.status == DiskCacheMissReason.SizeMismatch);

    // TTL expiration (last_access was 1200, ttl 600, now 1801 -> expired)
    const miss_ttl = idx.lookup(DiskCacheLookupQuery{
        key: key,
        expected_mtime_ns: 1700000000123,
        expected_source_size: 4096,
        expected_compiler_abi: 20,
        now_sec: 1801,
        ttl_secs: 600,
    });
    expect(miss_ttl.status == DiskCacheMissReason.ExpiredTtl);

    const st = idx.stats();
    expectEqual(st.lookup_hits, @as(u64, 1));
    expectEqual(st.lookup_misses, @as(u64, 4));
    expectEqual(st.abi_mismatches, @as(u64, 1));
    expectEqual(st.mtime_mismatches, @as(u64, 1));
    expectEqual(st.size_mismatches, @as(u64, 1));
    expectEqual(st.ttl_expirations, @as(u64, 1));
});

test("diskCache: upsert overwrite & targeted invalidation (key, source_hash, domain, ABI)", () => {
    let slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let idx = BuildDiskCacheIndex.init(slots[0..], 10);

    const k1 = BuildCacheKey{ hi: 0x0100, lo: 1 };
    const k2 = BuildCacheKey{ hi: 0x0200, lo: 2 };
    const k3 = BuildCacheKey{ hi: 0x0300, lo: 3 };
    const k4 = BuildCacheKey{ hi: 0x0400, lo: 4 };

    * = idx.upsert(DiskCacheEntry{ key: k1, domain: DiskCacheDomain.JavaScript, source_hash: 0x5555, artifact_size: 1000, compiler_abi: 10 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: k2, domain: DiskCacheDomain.JavaScript, source_hash: 0x5555, artifact_size: 2000, compiler_abi: 10 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: k3, domain: DiskCacheDomain.Css, source_hash: 0x6666, artifact_size: 3000, compiler_abi: 9 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: k4, domain: DiskCacheDomain.Json, source_hash: 0x7777, artifact_size: 4000, compiler_abi: 10 }) catch unreachable;
    expectEqual(idx.totalArtifactBytes(), @as(u64, 10000));

    // Upsert existing key updates bytes accurately without duplicating slot
    * = idx.upsert(DiskCacheEntry{ key: k1, domain: DiskCacheDomain.JavaScript, source_hash: 0x5555, artifact_size: 1500, compiler_abi: 10 }) catch unreachable;
    expectEqual(idx.count(), @as(usize, 4));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 10500));

    // Invalidate by source_hash removes k1 + k2
    const rem_src = idx.invalidateBySourceHash(0x5555);
    expectEqual(rem_src, @as(usize, 2));
    expectEqual(idx.count(), @as(usize, 2));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 7000));

    // Invalidate mismatched ABI removes k3 (abi=9)
    const rem_abi = idx.invalidateMismatchedAbi(10);
    expectEqual(rem_abi, @as(usize, 1));
    expectEqual(idx.count(), @as(usize, 1));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 4000));

    // Invalidate by domain removes k4 (Json)
    const rem_dom = idx.invalidateByDomain(DiskCacheDomain.Json);
    expectEqual(rem_dom, @as(usize, 1));
    expectEqual(idx.count(), @as(usize, 0));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 0));
});

test("diskCache: deterministic LRU + TTL + byte/entry quota eviction with pinned entries", () => {
    let slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let idx = BuildDiskCacheIndex.init(slots[0..], 1);

    // 5 entries:
    // e0: expired (last_access=100, now=1000, ttl=500) -> 1000 bytes
    // e1: pinned oldest (last_access=600, pinned=true) -> 2000 bytes (must be spared)
    // e2: active LRU oldest (last_access=650) -> 3000 bytes
    // e3: active middle (last_access=800) -> 2500 bytes
    // e4: active newest (last_access=950) -> 1500 bytes
    // Total = 10000 bytes, 5 entries.
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0x10, lo: 1 }, artifact_size: 1000, created_at_sec: 100, last_access_sec: 100 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0x20, lo: 2 }, artifact_size: 2000, created_at_sec: 600, last_access_sec: 600, pinned: true }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0x30, lo: 3 }, artifact_size: 3000, created_at_sec: 650, last_access_sec: 650 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0x40, lo: 4 }, artifact_size: 2500, created_at_sec: 800, last_access_sec: 800 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0x50, lo: 5 }, artifact_size: 1500, created_at_sec: 950, last_access_sec: 950 }) catch unreachable;

    let plan_items: [8]DiskCacheEvictionItem = [DiskCacheEvictionItem.empty(); 8];
    let scratch: [8]usize = [0; 8];

    // Policy: ttl=500, max_bytes=4000, max_entries=3, respect_pinned=true
    // Expected:
    //   1) e0 expired -> remaining 4 entries, 9000 bytes
    //   2) e2 (oldest unpinned, 3000B) -> remaining 3 entries, 6000 bytes (still > 4000B)
    //   3) e3 (next oldest unpinned, 2500B) -> remaining 2 entries (e1 pinned + e4 newest), 3500 bytes <= 4000B!
    const plan = idx.planEviction(
        DiskCacheEvictionPolicy{
            max_bytes: 4000,
            max_entries: 3,
            ttl_secs: 500,
            now_sec: 1000,
            respect_pinned: true,
        },
        plan_items[0..],
        scratch[0..],
    ) catch unreachable;

    expectEqual(plan.evicted_count, @as(usize, 3));
    expectEqual(plan.expired_count, @as(usize, 1));
    expectEqual(plan.quota_evicted_count, @as(usize, 2));
    expectEqual(plan.reclaimed_bytes, @as(u64, 6500));
    expectEqual(plan.remaining_entries, @as(usize, 2));
    expectEqual(plan.remaining_bytes, @as(u64, 3500));

    expect(plan_items[0].reason == DiskCacheEvictionReason.ExpiredTtl);
    expectEqual(plan_items[0].key.lo, @as(u64, 1));
    expect(plan_items[1].reason == DiskCacheEvictionReason.OverByteQuota);
    expectEqual(plan_items[1].key.lo, @as(u64, 3));
    expectEqual(plan_items[2].key.lo, @as(u64, 4));

    // Apply plan and verify surviving entries (pinned e1 + newest e4)
    const applied = idx.applyEvictionPlan(plan_items[0..plan.evicted_count]);
    expectEqual(applied, @as(usize, 3));
    expectEqual(idx.count(), @as(usize, 2));
    expectEqual(idx.totalArtifactBytes(), @as(u64, 3500));
    expect(idx.findSlot(BuildCacheKey{ hi: 0x20, lo: 2 }) != null);
    expect(idx.findSlot(BuildCacheKey{ hi: 0x50, lo: 5 }) != null);
    expect(idx.findSlot(BuildCacheKey{ hi: 0x30, lo: 3 }) == null);
});

test("diskCache: IEEE 802.3 CRC32 standard vector & binary snapshot round-trip", () => {
    // Standard IEEE 802.3 CRC32 check value for "123456789" is 0xCBF43926
    const crc = computeDiskCacheCrc32("123456789");
    expectEqual(crc, @as(u32, 0xCBF43926));
    // Incremental update must match single-shot
    const crc_inc = updateDiskCacheCrc32(updateDiskCacheCrc32(0, "1234"), "56789");
    expectEqual(crc_inc, @as(u32, 0xCBF43926));

    let slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let idx = BuildDiskCacheIndex.init(slots[0..], 42);

    const k1 = computeBuildCacheKey(BuildCacheKeyInput{
        domain: DiskCacheDomain.TypeScript,
        source_path: "src/a.ts",
        source_hash: computeDiskCacheHash64(1, "export const a = 1;"),
        dep_tree_hash: 0x1111,
        compiler_version: "cpc-1.2.0",
        compiler_abi: 42,
        target_name: "linux-x86_64",
    }) catch unreachable;

    const k2 = computeBuildCacheKey(BuildCacheKeyInput{
        domain: DiskCacheDomain.Css,
        source_path: "src/b.css",
        source_hash: computeDiskCacheHash64(1, "body { color: red; }"),
        dep_tree_hash: 0x2222,
        compiler_version: "cpc-1.2.0",
        compiler_abi: 42,
        target_name: "linux-x86_64",
    }) catch unreachable;

    * = idx.upsert(DiskCacheEntry{
        key: k1,
        domain: DiskCacheDomain.TypeScript,
        source_hash: 0xAAAA,
        dep_tree_hash: 0x1111,
        path_hash: 0x9991,
        mtime_ns: -123456789,
        source_size: 512,
        artifact_size: 1024,
        created_at_sec: 100,
        last_access_sec: 250,
        compiler_abi: 42,
        hit_count: 7,
        artifact_crc32: 0x12345678,
        flags: 0x03,
        pinned: true,
    }) catch unreachable;

    * = idx.upsert(DiskCacheEntry{
        key: k2,
        domain: DiskCacheDomain.Css,
        source_hash: 0xBBBB,
        dep_tree_hash: 0x2222,
        path_hash: 0x9992,
        mtime_ns: 1700000000000,
        source_size: 256,
        artifact_size: 512,
        created_at_sec: 110,
        last_access_sec: 300,
        compiler_abi: 42,
        hit_count: 2,
        artifact_crc32: 0x87654321,
        flags: 0x01,
        pinned: false,
    }) catch unreachable;

    let buf: [512]u8 = [0; 512];
    const expected_size = encodedDiskCacheIndexSize(2);
    expectEqual(expected_size, DISK_CACHE_HEADER_BYTES + 2 * DISK_CACHE_RECORD_BYTES);

    const enc_len = encodeDiskCacheIndex(&idx, buf[0..]) catch unreachable;
    expectEqual(enc_len, expected_size);
    expectEqual(buf[0], DISK_CACHE_INDEX_MAGIC[0]);
    expectEqual(buf[3], DISK_CACHE_INDEX_MAGIC[3]);

    // Decode into fresh index and verify exact field fidelity
    let dst_slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let dst = BuildDiskCacheIndex.init(dst_slots[0..], 1);
    const summary = decodeDiskCacheIndex(buf[0..enc_len], &dst) catch unreachable;
    expectEqual(summary.version, DISK_CACHE_INDEX_VERSION);
    expectEqual(summary.compiler_abi, @as(u32, 42));
    expectEqual(summary.entry_count, @as(usize, 2));
    expectEqual(summary.total_payload_bytes, @as(u64, 1536));
    expectEqual(dst.count(), @as(usize, 2));
    expectEqual(dst.totalArtifactBytes(), @as(u64, 1536));

    const s1 = dst.findSlot(k1) ?? 999;
    expect(s1 < 8);
    expectEqual(dst.entries[s1].mtime_ns, @as(i64, -123456789));
    expectEqual(dst.entries[s1].hit_count, @as(u32, 7));
    expect(dst.entries[s1].pinned);
    expect(dst.entries[s1].domain == DiskCacheDomain.TypeScript);
});

test("diskCache: 500-iteration deterministic malformed/truncated binary index Fuzz gate", () => {
    // Build a valid baseline snapshot with 3 entries
    let base_slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let base_idx = BuildDiskCacheIndex.init(base_slots[0..], 77);
    let e_i: u64 = 1;
    while (e_i <= 3) {
        * = base_idx.upsert(DiskCacheEntry{
            key: BuildCacheKey{ hi: e_i *% 0x0101010101010101, lo: e_i *% 0x9E3779B97F4A7C15 },
            domain: DiskCacheDomain.JavaScript,
            source_hash: e_i * 101,
            artifact_size: e_i * 512,
            created_at_sec: 100 + e_i,
            last_access_sec: 200 + e_i,
            compiler_abi: 77,
        }) catch unreachable;
        e_i += 1;
    }

    let valid_snap: [512]u8 = [0; 512];
    const valid_len = encodeDiskCacheIndex(&base_idx, valid_snap[0..]) catch unreachable;

    let scratch_buf: [512]u8 = [0; 512];
    let fuzz_slots: [8]DiskCacheEntry = [DiskCacheEntry.empty(); 8];
    let fuzz_idx = BuildDiskCacheIndex.init(fuzz_slots[0..], 77);

    // Deterministic XorShift64* PRNG for 500 fuzz mutations
    let rng: u64 = 0x5EED1234CAFEBABE;
    let rejected_count: usize = 0;
    let iter: usize = 0;
    while (iter < 500) {
        // Copy baseline snapshot
        let c: usize = 0;
        while (c < valid_len) {
            scratch_buf[c] = valid_snap[c];
            c += 1;
        }

        // Advance PRNG
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const r = rng *% 0x2545F4914F6CDD1D;
        const mode = iter % 5;

        let slice_len = valid_len;
        if (mode == 0) {
            // Truncation mutation (0 .. valid_len - 1)
            slice_len = @as(usize, @intCast(r % @as(u64, valid_len)));
        } else if (mode == 1) {
            // Single bit flip anywhere in header or payload
            const byte_idx = @as(usize, @intCast(r % @as(u64, valid_len)));
            const bit_mask: u8 = @as(u8, 1) << @as(u3, @truncate(r >> 16));
            scratch_buf[byte_idx] ^= bit_mask;
        } else if (mode == 2) {
            // Corrupt header entry_count or version or magic
            const field = r % 3;
            if (field == 0) {
                scratch_buf[0] ^= 0xFF; // bad magic
            } else if (field == 1) {
                scratch_buf[4] = 99; // bad version
            } else {
                scratch_buf[12] = 0xFF;
                scratch_buf[13] = 0xFF; // inflated entry_count
            }
        } else if (mode == 3) {
            // Trailing garbage bytes appended
            slice_len = valid_len + 1 + @as(usize, @intCast(r % 16));
            let t = valid_len;
            while (t < slice_len) {
                scratch_buf[t] = @as(u8, @truncate(r ^ @as(u64, t)));
                t += 1;
            }
        } else {
            // Corrupt domain tag or pinned flag and recompute CRC32 to test semantic record validation
            const rec_off = DISK_CACHE_HEADER_BYTES + (@as(usize, @intCast(r % 3)) * DISK_CACHE_RECORD_BYTES);
            if ((r & 1) == 0) {
                scratch_buf[rec_off + 92] = 250; // invalid domain enum
            } else {
                scratch_buf[rec_off + 94] = 7; // invalid pinned bool byte
            }
            let new_crc = updateDiskCacheCrc32(0, scratch_buf[0..28]);
            new_crc = updateDiskCacheCrc32(new_crc, scratch_buf[DISK_CACHE_HEADER_BYTES..valid_len]);
            scratch_buf[28] = @as(u8, @truncate(new_crc));
            scratch_buf[29] = @as(u8, @truncate(new_crc >> 8));
            scratch_buf[30] = @as(u8, @truncate(new_crc >> 16));
            scratch_buf[31] = @as(u8, @truncate(new_crc >> 24));
        }

        let failed = false;
        * = decodeDiskCacheIndex(scratch_buf[0..slice_len], &fuzz_idx) catch {
            failed = true;
        };
        expect(failed);
        if (failed) rejected_count += 1;
        iter += 1;
    }

    expectEqual(rejected_count, @as(usize, 500));
    // Valid snapshot still decodes cleanly after 500 malformed attempts
    const ok_sum = decodeDiskCacheIndex(valid_snap[0..valid_len], &fuzz_idx) catch unreachable;
    expectEqual(ok_sum.entry_count, @as(usize, 3));
});

test("diskCache: SIMD 256-shard distribution analysis & diagnostic report", () => {
    let slots: [16]DiskCacheEntry = [DiskCacheEntry.empty(); 16];
    let idx = BuildDiskCacheIndex.init(slots[0..], 9);

    // Place 3 entries in shard 0xAB and 2 entries in shard 0xCD
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0xAB00000000000001, lo: 1 }, artifact_size: 1000 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0xAB00000000000002, lo: 2 }, artifact_size: 2000 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0xAB00000000000003, lo: 3 }, artifact_size: 3000 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0xCD00000000000001, lo: 4 }, artifact_size: 4000 }) catch unreachable;
    * = idx.upsert(DiskCacheEntry{ key: BuildCacheKey{ hi: 0xCD00000000000002, lo: 5 }, artifact_size: 5000 }) catch unreachable;

    const dist = idx.analyzeShardDistribution();
    expectEqual(dist.total_entries, @as(i32, 5));
    expectEqual(dist.non_empty_shards, @as(u32, 2));
    expectEqual(dist.max_per_shard, @as(i32, 3));
    expectEqual(dist.min_per_shard, @as(i32, 0));

    let rep: [256]u8 = [0; 256];
    const rep_len = formatDiskCacheReport(&idx, rep[0..]) catch unreachable;
    const rep_str = rep[0..rep_len];
    expect(containsSubstr(rep_str, "BuildDiskCache(abi=9"));
    expect(containsSubstr(rep_str, "entries=5/16"));
    expect(containsSubstr(rep_str, "bytes=15000"));
    expect(containsSubstr(rep_str, "shards=2"));
    expect(containsSubstr(rep_str, "max_shard=3"));
});
