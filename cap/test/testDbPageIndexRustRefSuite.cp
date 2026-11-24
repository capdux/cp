import { test, expect_eq } from "cap:test";
import {
    PREFIX_BLOCK_MAX_BYTES,
    PREFIX_BLOCK_MAX_KEY_LEN,
    PrefixBlockStatus,
    PrefixBlockLookupStatus,
    scalarPageCommonPrefixLen,
    simdPageCommonPrefixLen,
    scalarPageCompareLex,
    simdPageCompareLex,
    scalarPageCopyBytes,
    simdPageCopyBytes,
    PrefixBlockBuilder,
    PrefixBlockReader,
    validatePrefixBlockStrict,
    SstSparseIndex,
} from "../../lib/db/sstableIndex.cp";
import {
    SLOTTED_PAGE_MAGIC,
    SLOTTED_PAGE_HEADER_SIZE,
    SLOTTED_SLOT_SIZE,
    SLOT_FLAG_ALIVE,
    SLOT_FLAG_DELETED,
    SlottedPageStatus,
    slottedPageCrc32,
    initPage,
    readPageHeader,
    readSlotMeta,
    livePayloadBytes,
    freeSpaceBytes,
    compactPage,
    insertRecord,
    getRecord,
    updateRecordInPlace,
    deleteRecord,
    setPageLsn,
    sealChecksum,
    verifyPageIntegrity,
} from "../../lib/db/slottedPage.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

fn formatKeyLen(buf: *[]u8, prefix: []const u8, idx: usize): usize {
    simdPageCopyBytes(buf[0..prefix.len], prefix);
    const d0 = ((idx / 1000) % 10) as u8;
    const d1 = ((idx / 100) % 10) as u8;
    const d2 = ((idx / 10) % 10) as u8;
    const d3 = (idx % 10) as u8;
    buf[prefix.len] = (48 as u8) + d0;
    buf[prefix.len + 1] = (48 as u8) + d1;
    buf[prefix.len + 2] = (48 as u8) + d2;
    buf[prefix.len + 3] = (48 as u8) + d3;
    return prefix.len + 4;
}

// ---------------------------------------------------------------------------
// Test 1: PrefixBlockBuilder -> PrefixBlockReader & SstSparseIndex Roundtrip
// ---------------------------------------------------------------------------

fn runTestPrefixBlockAndSparseIndex(): void! {
    // 1. Empty block roundtrip
    let empty_b = PrefixBlockBuilder.init(4);
    const empty_bytes = empty_b.finish();
    expect_eq(empty_bytes.len, 4 as usize);
    expect_true(validatePrefixBlockStrict(empty_bytes))?;
    const empty_r = PrefixBlockReader.open(empty_bytes).?;
    expect_eq(empty_r.restartCount(), 0 as usize);
    expect_false(empty_r.seekToFirst().isValid())?;
    expect_false(empty_r.seek("a").isValid())?;
    let ktmp: [64]u8 = [0; 64];
    let vtmp: [64]u8 = [0; 64];
    let ks: []u8 = ktmp[0..];
    let vs: []u8 = vtmp[0..];
    const miss0 = empty_r.get("a", ks, vs);
    expect_eq(miss0.status, PrefixBlockLookupStatus.not_found);

    // 2. Single-restart and multi-restart blocks (interval = 1, 4, 16)
    const intervals: [3]u32 = [1, 4, 16];
    let ii: usize = 0;
    while (ii < intervals.len) : (ii += 1) {
        let b = PrefixBlockBuilder.init(intervals[ii]);
        let key_arr: [48]u8 = [0; 48];
        let val_arr: [48]u8 = [0; 48];
        let key_s: []u8 = key_arr[0..];
        let val_s: []u8 = val_arr[0..];

        let n: usize = 0;
        while (n < 45) : (n += 1) {
            const klen = formatKeyLen(key_s, "tenant:acme:long_prefix_key:", n * 2);
            const vlen = formatKeyLen(val_s, "payload_value_record_", n * 2);
            const st = b.add(key_arr[0..klen], val_arr[0..vlen]);
            expect_eq(st, PrefixBlockStatus.ok);
        }
        // Reject out-of-order / duplicate / empty key
        expect_eq(b.add("", "v"), PrefixBlockStatus.empty_key);
        expect_eq(b.add("tenant:acme:long_prefix_key:0000", "v"), PrefixBlockStatus.out_of_order_key);

        const block = b.finish();
        expect_eq(b.add("zzzz", "v"), PrefixBlockStatus.already_finished);
        expect_true(validatePrefixBlockStrict(block))?;

        const reader = PrefixBlockReader.open(block).?;
        expect_true(reader.validateAllEntries())?;

        // Point get every present key and missing between/outside keys
        let q: usize = 0;
        while (q < 45) : (q += 1) {
            const klen = formatKeyLen(key_s, "tenant:acme:long_prefix_key:", q * 2);
            const vlen = formatKeyLen(val_s, "payload_value_record_", q * 2);
            const res = reader.get(key_arr[0..klen], ks, vs);
            expect_eq(res.status, PrefixBlockLookupStatus.found);
            expect_true(res.found)?;
            expect_eq(simdPageCompareLex(ktmp[0..res.key_len], key_arr[0..klen]), 0);
            expect_eq(simdPageCompareLex(vtmp[0..res.val_len], val_arr[0..vlen]), 0);

            // Odd key between even keys must be not_found
            const mlen = formatKeyLen(key_s, "tenant:acme:long_prefix_key:", q * 2 + 1);
            const res_m = reader.get(key_arr[0..mlen], ks, vs);
            expect_eq(res_m.status, PrefixBlockLookupStatus.not_found);
        }
        expect_eq(reader.get("aaa", ks, vs).status, PrefixBlockLookupStatus.not_found);
        expect_eq(reader.get("zzz", ks, vs).status, PrefixBlockLookupStatus.not_found);

        // Full cursor scan matches all 45 entries in strict ascending order
        let cur = reader.seekToFirst();
        let seen: usize = 0;
        while (cur.isValid()) {
            const klen = formatKeyLen(key_s, "tenant:acme:long_prefix_key:", seen * 2);
            const vlen = formatKeyLen(val_s, "payload_value_record_", seen * 2);
            expect_eq(simdPageCompareLex(cur.keySlice(), key_arr[0..klen]), 0);
            expect_eq(simdPageCompareLex(cur.valueSlice(), val_arr[0..vlen]), 0);
            seen += 1;
            if (!cur.next()) break;
        }
        expect_eq(seen, 45 as usize);

        // Seek to middle odd key lands on next even key
        const st_len = formatKeyLen(key_s, "tenant:acme:long_prefix_key:", 33);
        const se_len = formatKeyLen(val_s, "tenant:acme:long_prefix_key:", 34);
        const scur = reader.seek(key_arr[0..st_len]);
        expect_true(scur.isValid())?;
        expect_eq(simdPageCompareLex(scur.keySlice(), val_arr[0..se_len]), 0);
    }

    // 3. SstSparseIndex binary locate & range pruning
    let idx = SstSparseIndex.init();
    expect_eq(idx.addBlock("blk:0010", 0, 512, 10), PrefixBlockStatus.ok);
    expect_eq(idx.addBlock("blk:0020", 512, 512, 10), PrefixBlockStatus.ok);
    expect_eq(idx.addBlock("blk:0030", 1024, 512, 10), PrefixBlockStatus.ok);
    expect_eq(idx.addBlock("blk:0020", 1536, 512, 10), PrefixBlockStatus.out_of_order_key);
    expect_eq(idx.locateBlock("blk:0001").?, 0 as usize);
    expect_eq(idx.locateBlock("blk:0010").?, 0 as usize);
    expect_eq(idx.locateBlock("blk:0011").?, 1 as usize);
    expect_eq(idx.locateBlock("blk:0030").?, 2 as usize);
    expect_true(idx.locateBlock("blk:0031") == null)?;

    const rng = idx.locateRange("blk:0011", "blk:0025");
    expect_true(rng.found)?;
    expect_eq(rng.first_block, 1 as usize);
    expect_eq(rng.last_block, 2 as usize);
    expect_eq(rng.block_count, 2 as usize);
}

// ---------------------------------------------------------------------------
// Test 2: PrefixBlock Malformed / Overflow / Non-Monotonic Restart Rejection
// ---------------------------------------------------------------------------

fn runTestPrefixBlockMalformed(): void! {
    // 1. Truncated (< 4 bytes) and non-zero data with count=0
    expect_true(PrefixBlockReader.open("") == null)?;
    expect_true(PrefixBlockReader.open("\x00\x00\x00") == null)?;
    expect_true(PrefixBlockReader.open("\x01\x00\x00\x00\x00") == null)?;

    // 2. Overflow restart count (0xFFFFFFFF)
    const huge_restarts: []const u8 = "\x00\x01\x01a1\x00\x00\x00\x00\xFF\xFF\xFF\xFF";
    expect_true(PrefixBlockReader.open(huge_restarts) == null)?;
    expect_false(validatePrefixBlockStrict(huge_restarts))?;

    // 3. Non-monotonic restart offsets & out-of-bounds restart offset
    let b = PrefixBlockBuilder.init(2);
    expect_eq(b.add("k01", "v1"), PrefixBlockStatus.ok);
    expect_eq(b.add("k02", "v2"), PrefixBlockStatus.ok);
    expect_eq(b.add("k03", "v3"), PrefixBlockStatus.ok);
    const valid = b.finish();
    expect_true(validatePrefixBlockStrict(valid))?;

    let mut_buf: [256]u8 = [0; 256];
    simdPageCopyBytes(mut_buf[0..valid.len], valid);
    // Corrupt second restart offset to 0 (non-monotonic)
    const r1_off = valid.len - 8;
    mut_buf[r1_off] = 0;
    mut_buf[r1_off + 1] = 0;
    mut_buf[r1_off + 2] = 0;
    mut_buf[r1_off + 3] = 0;
    expect_true(PrefixBlockReader.open(mut_buf[0..valid.len]) == null)?;
    expect_false(validatePrefixBlockStrict(mut_buf[0..valid.len]))?;

    // Corrupt second restart offset out of bounds (0x7FFF)
    simdPageCopyBytes(mut_buf[0..valid.len], valid);
    mut_buf[r1_off] = 0xFF;
    mut_buf[r1_off + 1] = 0x7F;
    expect_true(PrefixBlockReader.open(mut_buf[0..valid.len]) == null)?;

    // Corrupt restart entry shared_len != 0
    simdPageCopyBytes(mut_buf[0..valid.len], valid);
    mut_buf[0] = 1; // shared_len = 1 at offset 0
    expect_true(PrefixBlockReader.open(mut_buf[0..valid.len]) == null)?;

    // Corrupt second restart key to be smaller than first restart key ("k03" -> "k00")
    simdPageCopyBytes(mut_buf[0..valid.len], valid);
    let p: usize = 0;
    while (p + 3 <= valid.len) : (p += 1) {
        if (mut_buf[p] == 'k' and mut_buf[p + 1] == '0' and mut_buf[p + 2] == '3') {
            mut_buf[p + 2] = '0';
            break;
        }
    }
    expect_true(PrefixBlockReader.open(mut_buf[0..valid.len]) == null)?;
    expect_false(validatePrefixBlockStrict(mut_buf[0..valid.len]))?;
}

// ---------------------------------------------------------------------------
// Test 3: SlottedPage 4KB/8KB CRUD, Space Conservation, Compaction & CRC32
// ---------------------------------------------------------------------------

fn runTestSlottedPageStrict(): void! {
    // 1. 4KB page lifecycle: insert, update (shrink/same/grow), delete, compact, space conservation
    let page4k: [4096]u8 = [0; 4096];
    let p4: []u8 = page4k[0..];
    expect_true(initPage(p4, 7, 100, 1))?;
    expect_eq(verifyPageIntegrity(p4), SlottedPageStatus.ok);

    const s0 = insertRecord(p4, "alpha_record_payload_00000001").?;
    const s1 = insertRecord(p4, "beta_record_payload_000000002").?;
    const s2 = insertRecord(p4, "gamma_record_payload_00000003").?;
    expect_eq(s0, 0 as u16);
    expect_eq(s1, 1 as u16);
    expect_eq(s2, 2 as u16);

    expect_eq(simdPageCompareLex(getRecord(p4, s0).?, "alpha_record_payload_00000001"), 0);
    expect_eq(simdPageCompareLex(getRecord(p4, s1).?, "beta_record_payload_000000002"), 0);
    expect_eq(simdPageCompareLex(getRecord(p4, s2).?, "gamma_record_payload_00000003"), 0);

    // Shrink update on s0 + delete s1 + grow update on s2
    expect_true(updateRecordInPlace(p4, s0, "alpha_short"))?;
    expect_true(deleteRecord(p4, s1))?;
    expect_true(getRecord(p4, s1) == null)?;
    expect_true(updateRecordInPlace(p4, s2, "gamma_record_grown_payload_exceeding_32_bytes_for_simd_copy"))?;

    // Space conservation before compaction: free_start + contig + live + frag == page_len
    const h_before = readPageHeader(p4).?;
    const fs_before = freeSpaceBytes(p4).?;
    const live_before = livePayloadBytes(p4).?;
    expect_eq(
        (h_before.free_start as usize) + fs_before.contiguous_bytes + live_before + fs_before.fragmented_bytes,
        4096 as usize,
    );

    // Compact page and verify all live records are byte-identical & fragmented_bytes == 0
    expect_true(compactPage(p4))?;
    expect_true(setPageLsn(p4, 200))?;
    * = sealChecksum(p4).?;
    expect_eq(verifyPageIntegrity(p4), SlottedPageStatus.ok);

    const h_after = readPageHeader(p4).?;
    expect_eq(h_after.fragmented_bytes, 0 as u16);
    expect_eq(livePayloadBytes(p4).?, live_before);
    expect_eq((h_after.free_end as usize) + live_before, 4096 as usize);
    expect_eq(simdPageCompareLex(getRecord(p4, s0).?, "alpha_short"), 0);
    expect_eq(simdPageCompareLex(getRecord(p4, s2).?, "gamma_record_grown_payload_exceeding_32_bytes_for_simd_copy"), 0);

    // Tombstone slot reuse: next insert reuses slot 1
    const s_reuse = insertRecord(p4, "reused_slot_1_data").?;
    expect_eq(s_reuse, 1 as u16);
    * = sealChecksum(p4).?;
    expect_eq(verifyPageIntegrity(p4), SlottedPageStatus.ok);

    // 2. Single-bit flip detection across header, slot dir, free gap, payload, and CRC32
    const probe_offsets: [8]usize = [0, 5, 18, 28, 32, 40, 2048, 4095];
    let pi: usize = 0;
    while (pi < probe_offsets.len) : (pi += 1) {
        const off = probe_offsets[pi];
        p4[off] ^= 0x01;
        const st = verifyPageIntegrity(p4);
        expect_true(st != SlottedPageStatus.ok)?;
        p4[off] ^= 0x01;
        expect_eq(verifyPageIntegrity(p4), SlottedPageStatus.ok);
    }

    // 3. 8KB page check + corrupt header/fragmented_bytes/slot collision rejection
    let page8k: [8192]u8 = [0; 8192];
    let p8: []u8 = page8k[0..];
    expect_true(initPage(p8, 99, 555, 0))?;
    const r8 = insertRecord(p8, "8kb_page_record_payload_0123456789ABCDEF").?;
    expect_eq(r8, 0 as u16);
    * = sealChecksum(p8).?;
    expect_eq(verifyPageIntegrity(p8), SlottedPageStatus.ok);

    // Forged fragmented_bytes > payload_area with recomputed CRC must be caught as corrupt_header
    p8[24] = 0xFF;
    p8[25] = 0x7F;
    const forged_crc = slottedPageCrc32(p8);
    p8[28] = (forged_crc & 0xFF) as u8;
    p8[29] = ((forged_crc >> 8) & 0xFF) as u8;
    p8[30] = ((forged_crc >> 16) & 0xFF) as u8;
    p8[31] = ((forged_crc >> 24) & 0xFF) as u8;
    expect_eq(verifyPageIntegrity(p8), SlottedPageStatus.corrupt_header);
}

// ---------------------------------------------------------------------------
// Test 4: 16-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let a: [96]u8 = [0; 96];
    let b: [96]u8 = [0; 96];
    let dst_simd: [96]u8 = [0; 96];
    let dst_scalar: [96]u8 = [0; 96];

    let i: usize = 0;
    while (i < 96) : (i += 1) {
        a[i] = ((i * 29 + 7) & 0xFF) as u8;
        b[i] = a[i];
    }

    let len: usize = 0;
    while (len <= 96) : (len += 1) {
        // Equal slices
        expect_eq(simdPageCommonPrefixLen(a[0..len], b[0..len]), scalarPageCommonPrefixLen(a[0..len], b[0..len]));
        expect_eq(simdPageCommonPrefixLen(a[0..len], b[0..len]), len);
        expect_eq(simdPageCompareLex(a[0..len], b[0..len]), scalarPageCompareLex(a[0..len], b[0..len]));
        expect_eq(simdPageCompareLex(a[0..len], b[0..len]), 0);

        // Copy parity
        let k: usize = 0;
        while (k < 96) : (k += 1) {
            dst_simd[k] = 0x55;
            dst_scalar[k] = 0x55;
        }
        simdPageCopyBytes(dst_simd[0..len], a[0..len]);
        scalarPageCopyBytes(dst_scalar[0..len], a[0..len]);
        expect_eq(scalarPageCompareLex(dst_simd[0..96], dst_scalar[0..96]), 0);

        // Single-byte mismatch at every position for representative lengths
        if (len > 0 and (len <= 34 or len == 48 or len == 64 or len == 96)) {
            let pos: usize = 0;
            while (pos < len) : (pos += 1) {
                const orig = b[pos];
                b[pos] = orig +% 1;
                expect_eq(simdPageCommonPrefixLen(a[0..len], b[0..len]), scalarPageCommonPrefixLen(a[0..len], b[0..len]));
                expect_eq(simdPageCommonPrefixLen(a[0..len], b[0..len]), pos);
                expect_eq(simdPageCompareLex(a[0..len], b[0..len]), scalarPageCompareLex(a[0..len], b[0..len]));
                b[pos] = orig;
            }
        }
    }

    // Unequal lengths (prefix relationship)
    expect_eq(simdPageCommonPrefixLen(a[0..17], b[0..33]), scalarPageCommonPrefixLen(a[0..17], b[0..33]));
    expect_eq(simdPageCompareLex(a[0..17], b[0..33]), scalarPageCompareLex(a[0..17], b[0..33]));
    expect_eq(simdPageCompareLex(a[0..33], b[0..17]), scalarPageCompareLex(a[0..33], b[0..17]));
}

// ---------------------------------------------------------------------------
// Test 5: 1,600-Case Deterministic PRNG Fuzz (800 PrefixBlock + 800 SlottedPage)
// ---------------------------------------------------------------------------

fn runTestDbFuzz1600(): void! {
    let rng = XorShift64 { state: 0x3608DB20261124 };

    // Part A: 800 PrefixBlock fuzz cases (400 mutated/random block decode + 400 random build-read roundtrips)
    let seed_b = PrefixBlockBuilder.init(3);
    expect_eq(seed_b.add("aaa_0001", "val1"), PrefixBlockStatus.ok);
    expect_eq(seed_b.add("aaa_0002", "val2"), PrefixBlockStatus.ok);
    expect_eq(seed_b.add("bbb_0003", "val3"), PrefixBlockStatus.ok);
    expect_eq(seed_b.add("bbb_0004", "val4"), PrefixBlockStatus.ok);
    const seed_block = seed_b.finish();

    let fbuf: [512]u8 = [0; 512];
    let kout: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
    let vout: [256]u8 = [0; 256];
    let kout_s: []u8 = kout[0..];
    let vout_s: []u8 = vout[0..];

    let i: usize = 0;
    while (i < 800) : (i += 1) {
        if (i < 400) {
            let flen: usize = 0;
            if ((i & 1) == 0) {
                flen = seed_block.len;
                simdPageCopyBytes(fbuf[0..flen], seed_block);
                const mode = rng.next() % 3;
                if (mode == 0) {
                    const p = (rng.next() as usize) % flen;
                    fbuf[p] ^= ((rng.next() & 0xFF) as u8) | 1;
                } else if (mode == 1) {
                    flen = (rng.next() as usize) % flen;
                } else {
                    const p = (rng.next() as usize) % flen;
                    fbuf[p] = 0xFF;
                }
            } else {
                flen = (rng.next() as usize) % 256;
                let j: usize = 0;
                while (j < flen) : (j += 1) {
                    fbuf[j] = (rng.next() & 0xFF) as u8;
                }
            }
            const sl = fbuf[0..flen];
            const strict_ok = validatePrefixBlockStrict(sl);
            if (r := PrefixBlockReader.open(sl)) {
                * = r.get("aaa_0002", kout_s, vout_s);
                let cur = r.seek("aaa_0001");
                let steps: usize = 0;
                while (cur.isValid() and steps < 64) : (steps += 1) {
                    if (!cur.next()) break;
                }
            } else {
                expect_false(strict_ok)?;
            }
        } else {
            // Valid random prefix block roundtrip
            const interval: u32 = ((rng.next() % 5) + 1) as u32;
            let rb = PrefixBlockBuilder.init(interval);
            const cnt = ((rng.next() % 12) + 1) as usize;
            let kb: [32]u8 = [0; 32];
            let vb: [32]u8 = [0; 32];
            let kb_s: []u8 = kb[0..];
            let vb_s: []u8 = vb[0..];
            let base_step: usize = ((rng.next() % 10) + 1) as usize;
            let e: usize = 0;
            while (e < cnt) : (e += 1) {
                const klen = formatKeyLen(kb_s, "fuzz:key:", e * base_step + 1);
                const vlen = formatKeyLen(vb_s, "val:", e);
                expect_eq(rb.add(kb[0..klen], vb[0..vlen]), PrefixBlockStatus.ok);
            }
            const enc = rb.finish();
            expect_true(validatePrefixBlockStrict(enc))?;
            const rr = PrefixBlockReader.open(enc).?;
            const pick = (rng.next() as usize) % cnt;
            const pklen = formatKeyLen(kb_s, "fuzz:key:", pick * base_step + 1);
            const pvlen = formatKeyLen(vb_s, "val:", pick);
            const hit = rr.get(kb[0..pklen], kout_s, vout_s);
            expect_eq(hit.status, PrefixBlockLookupStatus.found);
            expect_eq(simdPageCompareLex(vout[0..hit.val_len], vb[0..pvlen]), 0);
        }
    }

    // Part B: 800 SlottedPage fuzz cases (400 stateful differential ops + 400 malformed page bytes)
    let page: [512]u8 = [0; 512];
    let ps: []u8 = page[0..];
    expect_true(initPage(ps, 1, 1, 0))?;

    // Reference model: up to 32 slots, each up to 48 bytes
    let model_alive: [32]bool = [false; 32];
    let model_len: [32]usize = [0; 32];
    let model_data: [32][48]u8 = [[0; 48]; 32];
    let payload: [48]u8 = [0; 48];

    let op_i: usize = 0;
    while (op_i < 400) : (op_i += 1) {
        const op = rng.next() % 4;
        if (op == 0) {
            // Insert
            const plen = ((rng.next() % 40) + 1) as usize;
            let k: usize = 0;
            while (k < plen) : (k += 1) {
                payload[k] = (rng.next() & 0xFF) as u8;
            }
            if (sid := insertRecord(ps, payload[0..plen])) {
                const su = sid as usize;
                if (su < 32) {
                    model_alive[su] = true;
                    model_len[su] = plen;
                    simdPageCopyBytes(model_data[su][0..plen], payload[0..plen]);
                }
            }
        } else if (op == 1) {
            // Update
            const su = (rng.next() as usize) % 32;
            const plen = ((rng.next() % 40) + 1) as usize;
            let k: usize = 0;
            while (k < plen) : (k += 1) {
                payload[k] = (rng.next() & 0xFF) as u8;
            }
            const ok = updateRecordInPlace(ps, su as u16, payload[0..plen]);
            if (ok) {
                expect_true(model_alive[su])?;
                model_len[su] = plen;
                simdPageCopyBytes(model_data[su][0..plen], payload[0..plen]);
            }
        } else if (op == 2) {
            // Delete
            const su = (rng.next() as usize) % 32;
            const ok = deleteRecord(ps, su as u16);
            expect_eq(ok, model_alive[su]);
            if (ok) {
                model_alive[su] = false;
                model_len[su] = 0;
            }
        } else {
            // Compact
            expect_true(compactPage(ps))?;
        }

        // Seal & verify integrity + exact differential match on all 32 model slots
        * = sealChecksum(ps).?;
        expect_eq(verifyPageIntegrity(ps), SlottedPageStatus.ok);
        let s: usize = 0;
        while (s < 32) : (s += 1) {
            const got = getRecord(ps, s as u16);
            if (model_alive[s]) {
                const rec = got.?;
                expect_eq(simdPageCompareLex(rec, model_data[s][0..model_len[s]]), 0);
            } else {
                expect_true(got == null)?;
            }
        }
    }

    // 400 malformed slotted page bytes (must never panic / OOB)
    let mpage: [256]u8 = [0; 256];
    let mi: usize = 0;
    while (mi < 400) : (mi += 1) {
        const mlen = (rng.next() as usize) % 256;
        let j: usize = 0;
        while (j < mlen) : (j += 1) {
            mpage[j] = (rng.next() & 0xFF) as u8;
        }
        // Inject valid magic on half the cases to exercise deep header/slot checks
        if ((mi & 1) == 0 and mlen >= 64) {
            mpage[0] = 'C';
            mpage[1] = 'P';
            mpage[2] = 'S';
            mpage[3] = '1';
        }
        const msl = mpage[0..mlen];
        * = verifyPageIntegrity(msl);
        * = readPageHeader(msl);
        * = freeSpaceBytes(msl);
        * = livePayloadBytes(msl);
        * = getRecord(msl, 0);
    }
}

fn main() {
    test("PrefixBlockBuilder/Reader & SstSparseIndex roundtrip (empty/single/multi-restart)", () => {
        runTestPrefixBlockAndSparseIndex()?;
    });
    test("PrefixBlock malformed / overflow / non-monotonic restart rejection", () => {
        runTestPrefixBlockMalformed()?;
    });
    test("SlottedPage 4KB/8KB CRUD, compaction, space conservation & CRC32 bit-flip detection", () => {
        runTestSlottedPageStrict()?;
    });
    test("16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (800 PrefixBlock + 800 SlottedPage)", () => {
        runTestDbFuzz1600()?;
    });
}
