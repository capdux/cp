// Origin: Pure-CP Stdlib Test Suite (lib/db/sstableIndex.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RocksDB/LevelDB Prefix-Compressed Data Block & Sparse Block Index:
//   1. Prefix-compressed block encoding, periodic Restart Points & compression ratio telemetry
//   2. O(log R) Restart Point binary search + linear point lookup (`get`), miss boundaries & buffer_too_small
//   3. Zero-heap forward cursor iteration (`seekToFirst`, `seek(lower_key)`, `next`) across restart intervals
//   4. 64-block `SstSparseIndex` O(log B) `locateBlock`, `locateRange` pruning, out-of-order & saturation guards
//   5. Builder write guards (empty/non-ascending/post-finish) & malformed/truncated/corrupt block rejection on open
//   6. End-to-end 2-level SSTable lookup (SstSparseIndex -> PrefixBlockReader) & cap.db facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL,
    PREFIX_BLOCK_MAX_BYTES,
    SST_INDEX_MAX_BLOCKS,
    PrefixBlockStatus,
    PrefixBlockLookupStatus,
    PrefixBlockStats,
    SstIndexRange,
    compareLexBytes,
    PrefixBlockBuilder,
    PrefixBlockReader,
    SstSparseIndex,
} from "cap:db";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn formatOrderKey(idx: usize, out: *[]u8): usize {
    // "tenant:prod:order:00" .. "tenant:prod:order:99" (20 bytes)
    const prefix = "tenant:prod:order:";
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        out[i] = prefix[i];
    }
    out[18] = @as(u8, @intCast('0' + (idx / 10)));
    out[19] = @as(u8, @intCast('0' + (idx % 10)));
    return 20;
}

fn formatVal(idx: usize, out: *[]u8): usize {
    out[0] = 'v';
    out[1] = '_';
    out[2] = @as(u8, @intCast('0' + (idx / 10)));
    out[3] = @as(u8, @intCast('0' + (idx % 10)));
    return 4;
}

test("SstableIndex: prefix-compressed block encoding, restart intervals & compression stats") {
    let builder = PrefixBlockBuilder.init(4);
    let kbuf: [32]u8 = [0; 32];
    let vbuf: [16]u8 = [0; 16];
    let ks: []u8 = kbuf[0..];
    let vs: []u8 = vbuf[0..];

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const kl = formatOrderKey(i, *ks);
        const vl = formatVal(i, *vs);
        expect_eq(builder.add(kbuf[0..kl], vbuf[0..vl]), PrefixBlockStatus.ok);
    }

    const block = builder.finish();
    const st: PrefixBlockStats = builder.stats();
    expect_true(st.finished);
    expect_eq(st.entry_count, @as(u32, 32));
    expect_eq(st.restart_count, @as(u32, 8)); // 32 / 4 = 8 restart points
    expect_eq(st.raw_key_bytes, @as(u64, 640)); // 32 * 20
    expect_eq(st.raw_val_bytes, @as(u64, 128)); // 32 * 4
    expect_true(st.shared_prefix_saved_bytes > 400);
    expect_true(st.key_compression_permille >= 650);
    expect_true(block.len < 450); // Much smaller than 768 raw bytes
    expect_eq(st.encoded_bytes, block.len);

    // Calling finish() a second time is idempotent
    expect_eq(builder.finish().len, block.len);
}

test("SstableIndex: O(log R) Restart Point binary lookup, miss boundaries & buffer_too_small") {
    let builder = PrefixBlockBuilder.init(4);
    let kbuf: [32]u8 = [0; 32];
    let vbuf: [16]u8 = [0; 16];
    let ks: []u8 = kbuf[0..];
    let vs: []u8 = vbuf[0..];

    // Insert even indices 0, 2, 4, ..., 62 (32 keys, 8 restart points)
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const id = i * 2;
        const kl = formatOrderKey(id, *ks);
        const vl = formatVal(id, *vs);
        expect_eq(builder.add(kbuf[0..kl], vbuf[0..vl]), PrefixBlockStatus.ok);
    }

    const block = builder.finish();
    const reader_opt = PrefixBlockReader.open(block);
    expect_true(reader_opt != null);
    if (reader := reader_opt) {
        expect_eq(reader.restartCount(), @as(usize, 8));

        let out_k: [64]u8 = [0; 64];
        let out_v: [32]u8 = [0; 32];
        let ok_s: []u8 = out_k[0..];
        let ov_s: []u8 = out_v[0..];

        // Verify all 32 present keys (both restart points and delta-compressed interior keys)
        let j: usize = 0;
        while (j < 32) : (j += 1) {
            const id = j * 2;
            const kl = formatOrderKey(id, *ks);
            const vl = formatVal(id, *vs);
            const res = reader.get(kbuf[0..kl], *ok_s, *ov_s);
            expect_eq(res.status, PrefixBlockLookupStatus.found);
            expect_true(res.found);
            expect_true(bytesEq(out_k[0..res.key_len], kbuf[0..kl]));
            expect_true(bytesEq(out_v[0..res.val_len], vbuf[0..vl]));
            // Binary probes over 8 restarts + initial bound check <= 5; linear steps <= restart_interval (4)
            expect_true(res.binary_probes <= 5);
            expect_true(res.linear_steps >= 1 and res.linear_steps <= 4);
        }

        // Missing keys: before first, between two keys (odd id), and after last
        const miss_low = reader.get("aaa", *ok_s, *ov_s);
        expect_eq(miss_low.status, PrefixBlockLookupStatus.not_found);
        expect_eq(miss_low.linear_steps, @as(u32, 0));

        const kl_odd = formatOrderKey(17, *ks);
        const miss_mid = reader.get(kbuf[0..kl_odd], *ok_s, *ov_s);
        expect_eq(miss_mid.status, PrefixBlockLookupStatus.not_found);

        const miss_high = reader.get("zzz", *ok_s, *ov_s);
        expect_eq(miss_high.status, PrefixBlockLookupStatus.not_found);

        // Output buffer too small is reported cleanly without panic
        let tiny_k: [4]u8 = [0; 4];
        let tiny_ks: []u8 = tiny_k[0..];
        const kl0 = formatOrderKey(0, *ks);
        const small_res = reader.get(kbuf[0..kl0], *tiny_ks, *ov_s);
        expect_eq(small_res.status, PrefixBlockLookupStatus.buffer_too_small);
        expect_eq(small_res.key_len, @as(usize, 20));
    }
}

test("SstableIndex: zero-heap seek(lower_key) & forward cursor iteration") {
    let builder = PrefixBlockBuilder.init(3);
    * = builder.add("k:01", "v1");
    * = builder.add("k:03", "v3");
    * = builder.add("k:05", "v5");
    * = builder.add("k:07", "v7");
    * = builder.add("k:09", "v9");
    * = builder.add("k:11", "v11");
    * = builder.add("k:13", "v13");
    const block = builder.finish();

    const reader_opt = PrefixBlockReader.open(block);
    expect_true(reader_opt != null);
    if (reader := reader_opt) {
        // Full forward scan from seekToFirst()
        let cur = reader.seekToFirst();
        let count: usize = 0;
        while (cur.isValid()) {
            count += 1;
            if (!cur.next()) break;
        }
        expect_eq(count, @as(usize, 7));

        // Seek to non-existent key "k:06" lands on "k:07" and iterates to end
        let cur2 = reader.seek("k:06");
        expect_true(cur2.isValid());
        expect_true(bytesEq(cur2.keySlice(), "k:07"));
        expect_true(bytesEq(cur2.valueSlice(), "v7"));
        expect_true(cur2.next());
        expect_true(bytesEq(cur2.keySlice(), "k:09"));
        expect_true(cur2.next());
        expect_true(bytesEq(cur2.keySlice(), "k:11"));
        expect_true(cur2.next());
        expect_true(bytesEq(cur2.keySlice(), "k:13"));
        expect_true(!cur2.next());
        expect_true(!cur2.isValid());

        // Seek beyond largest key yields invalid cursor
        const cur3 = reader.seek("k:99");
        expect_true(!cur3.isValid());

        // Seek smaller than smallest key lands on "k:01"
        const cur4 = reader.seek("k:00");
        expect_true(cur4.isValid());
        expect_true(bytesEq(cur4.keySlice(), "k:01"));
    }
}

test("SstableIndex: 64-block SstSparseIndex binary locateBlock, locateRange & guards") {
    let idx = SstSparseIndex.init();
    expect_true(idx.locateBlock("any") == null);
    expect_true(!idx.locateRange("a", "z").found);

    // Register 64 blocks with fence keys "b_00" .. "b_63"
    let fbuf: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < SST_INDEX_MAX_BLOCKS) : (i += 1) {
        fbuf[0] = 'b';
        fbuf[1] = '_';
        fbuf[2] = @as(u8, @intCast('0' + (i / 10)));
        fbuf[3] = @as(u8, @intCast('0' + (i % 10)));
        expect_eq(
            idx.addBlock(fbuf[0..4], @as(u64, @intCast(i * 4096)), 4096, 16),
            PrefixBlockStatus.ok,
        );
    }

    // Saturation rejection at 64 blocks
    expect_eq(idx.addBlock("b_99", 999999, 4096, 1), PrefixBlockStatus.index_full);
    expect_eq(idx.count(), SST_INDEX_MAX_BLOCKS);
    expect_eq(idx.stats().total_entries, @as(u64, 64 * 16));

    // O(log B) binary locateBlock checks
    expect_eq(idx.locateBlock("a_00") ?? 999, @as(usize, 0));
    expect_eq(idx.locateBlock("b_00") ?? 999, @as(usize, 0));
    expect_eq(idx.locateBlock("b_00a") ?? 999, @as(usize, 1));
    expect_eq(idx.locateBlock("b_31") ?? 999, @as(usize, 31));
    expect_eq(idx.locateBlock("b_63") ?? 999, @as(usize, 63));
    expect_true(idx.locateBlock("b_64") == null);

    // Range pruning tests
    const r1: SstIndexRange = idx.locateRange("b_10", "b_20");
    expect_true(r1.found);
    expect_eq(r1.first_block, @as(usize, 10));
    expect_eq(r1.last_block, @as(usize, 20));
    expect_eq(r1.block_count, @as(usize, 11));

    // Open-ended range [b_60, +inf)
    const r2 = idx.locateRange("b_60", "");
    expect_true(r2.found);
    expect_eq(r2.first_block, @as(usize, 60));
    expect_eq(r2.last_block, @as(usize, 63));

    // Inverted or out-of-range queries return found=false
    expect_true(!idx.locateRange("b_30", "b_10").found);
    expect_true(!idx.locateRange("z_00", "z_99").found);

    // Out-of-order fence key rejection
    idx.clear();
    expect_eq(idx.addBlock("m_key", 0, 100, 5), PrefixBlockStatus.ok);
    expect_eq(idx.addBlock("m_key", 100, 100, 5), PrefixBlockStatus.out_of_order_key);
    expect_eq(idx.addBlock("a_key", 100, 100, 5), PrefixBlockStatus.out_of_order_key);
    expect_eq(idx.addBlock("", 100, 100, 5), PrefixBlockStatus.empty_key);
}

test("SstableIndex: write guards & malformed/truncated/corrupt block rejection") {
    let builder = PrefixBlockBuilder.init(0); // 0 clamped to 1
    expect_eq(builder.add("", "v"), PrefixBlockStatus.empty_key);
    expect_eq(builder.add("b", "1"), PrefixBlockStatus.ok);
    expect_eq(builder.add("b", "2"), PrefixBlockStatus.out_of_order_key);
    expect_eq(builder.add("a", "0"), PrefixBlockStatus.out_of_order_key);
    expect_eq(builder.add("c", "3"), PrefixBlockStatus.ok);
    const valid_block = builder.finish();
    expect_eq(builder.add("d", "4"), PrefixBlockStatus.already_finished);

    // 1. Truncated blocks (< 4 bytes or cut in half)
    expect_true(PrefixBlockReader.open("") == null);
    expect_true(PrefixBlockReader.open("abc") == null);
    expect_true(PrefixBlockReader.open(valid_block[0 .. valid_block.len - 2]) == null);

    // 2. Corrupt restart count claiming 999 restarts
    let bad_count: [16]u8 = [0; 16];
    bad_count[12] = 0xE7;
    bad_count[13] = 0x03; // 999 LE
    expect_true(PrefixBlockReader.open(bad_count[0..]) == null);

    // 3. Corrupt restart offset pointing past data_end
    let bad_off: [12]u8 = [0; 12];
    bad_off[4] = 0xFF; // restart[0] = 255 > data_end(4)
    bad_off[8] = 1;    // num_restarts = 1
    expect_true(PrefixBlockReader.open(bad_off[0..]) == null);

    // 4. Restart entry with non-zero shared_len is rejected
    let bad_shared: [16]u8 = [0; 16];
    bad_shared[0] = 2; // shared_len = 2 (illegal at restart point 0!)
    bad_shared[1] = 1; // unshared_len = 1
    bad_shared[2] = 1; // val_len = 1
    bad_shared[3] = 'x';
    bad_shared[4] = 'y';
    // restarts_offset = 8, restart[0] = 0, num_restarts = 1
    bad_shared[12] = 1;
    expect_true(PrefixBlockReader.open(bad_shared[0..]) == null);

    // 5. Empty sealed block (4 zero bytes) opens cleanly and returns not_found
    let empty_b = PrefixBlockBuilder.init(PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL);
    const empty_bytes = empty_b.finish();
    expect_eq(empty_bytes.len, @as(usize, 4));
    const empty_r = PrefixBlockReader.open(empty_bytes);
    expect_true(empty_r != null);
    if (er := empty_r) {
        let ok_b: [8]u8 = [0; 8];
        let ov_b: [8]u8 = [0; 8];
        let oks: []u8 = ok_b[0..];
        let ovs: []u8 = ov_b[0..];
        expect_eq(er.get("k", *oks, *ovs).status, PrefixBlockLookupStatus.not_found);
    }
}

test("SstableIndex: 2-level SSTable lookup pipeline & cap.db facade parity") {
    // Build 2 blocks into a contiguous SSTable byte buffer & index them via cap.db facade
    let b0 = cap.db.PrefixBlockBuilder.init(2);
    * = b0.add("apple:01", "red");
    * = b0.add("apple:02", "green");
    * = b0.add("banana:01", "yellow");
    const blk0 = b0.finish();

    let b1 = cap.db.PrefixBlockBuilder.init(2);
    * = b1.add("cherry:01", "dark");
    * = b1.add("cherry:02", "sweet");
    * = b1.add("date:01", "brown");
    const blk1 = b1.finish();

    let sst_buf: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < blk0.len) : (i += 1) {
        sst_buf[i] = blk0[i];
    }
    const off1 = blk0.len;
    i = 0;
    while (i < blk1.len) : (i += 1) {
        sst_buf[off1 + i] = blk1[i];
    }

    let sparse = cap.db.SstSparseIndex.init();
    expect_eq(
        sparse.addBlock(b0.lastKeySlice(), 0, @as(u32, @intCast(blk0.len)), b0.stats().entry_count),
        PrefixBlockStatus.ok,
    );
    expect_eq(
        sparse.addBlock(b1.lastKeySlice(), @as(u64, @intCast(off1)), @as(u32, @intCast(blk1.len)), b1.stats().entry_count),
        PrefixBlockStatus.ok,
    );

    // 2-level lookup: sparse.locateBlock("cherry:02") -> block 1 -> PrefixBlockReader.get
    const blk_idx = sparse.locateBlock("cherry:02") ?? 99;
    expect_eq(blk_idx, @as(usize, 1));
    if (blk_meta := sparse.getBlock(blk_idx)) {
        const start = @as(usize, @intCast(blk_meta.block_offset));
        const end = start + @as(usize, @intCast(blk_meta.block_size));
        if (reader := cap.db.PrefixBlockReader.open(sst_buf[start..end])) {
            let kout: [32]u8 = [0; 32];
            let vout: [32]u8 = [0; 32];
            let ks: []u8 = kout[0..];
            let vs: []u8 = vout[0..];
            const res = reader.get("cherry:02", *ks, *vs);
            expect_true(res.found);
            expect_true(bytesEq(vout[0..res.val_len], "sweet"));
        } else {
            expect_true(false);
        }
    }

    expect_eq(compareLexBytes("a", "b"), @as(i32, -1));
    expect_eq(PREFIX_BLOCK_MAX_BYTES, @as(usize, 4096));
}
