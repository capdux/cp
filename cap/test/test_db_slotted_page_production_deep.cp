// Origin: Pure-CP Stdlib Test Suite (lib/db/slotted_page.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Database Slotted-Page Binary Layout & Compaction Engine:
//   1. 4KB slotted page init, insert, zero-copy getRecord, LSN update & CRC32 seal/verify
//   2. Tombstone delete, slot reuse & automatic O(1)-memory in-place compaction on fragmented insert
//   3. updateRecordInPlace: same-size, shrink (fragment accounting), growing compaction & atomic OOM guard
//   4. 8KB high-density page stress test (100 variable-length records, half-delete, compact, full slot reuse)
//   5. Bit-flip tamper detection (checksum_mismatch) & corrupt magic detection (invalid_magic)
//   6. Deep structural integrity checks: overlapping_records & corrupt_slot_bounds detection
//   7. Page size boundary guards (< 64 rejected, exact 64B page filled to byte 64) & deleted/OOB slot guards
//   8. Empty record, oversized record & full-page rejection preserving page integrity

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SLOTTED_PAGE_MAGIC,
    SLOTTED_PAGE_HEADER_SIZE,
    SLOTTED_SLOT_SIZE,
    SLOTTED_PAGE_MIN_SIZE,
    SLOTTED_PAGE_MAX_SIZE,
    SLOT_FLAG_ALIVE,
    SLOT_FLAG_DELETED,
    SlottedPageStatus,
    SlottedPageHeader,
    SlottedSlotMeta,
    SlottedFreeSpace,
    slottedPageCrc32,
    initPage,
    readPageHeader,
    readSlotMeta,
    freeSpaceBytes,
    compactPage,
    insertRecord,
    getRecord,
    updateRecordInPlace,
    deleteRecord,
    setPageLsn,
    sealChecksum,
    verifyPageIntegrity,
} from "cap:db";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: 4KB Slotted Page Init, Insert, Zero-Copy Read & CRC32 Integrity
// ============================================================================

fn test4KbPageBasicCrudAndChecksum()! {
    let page_arr: [4096]u8 = [0; 4096];
    let page: []u8 = page_arr[0..];

    expect_eq(cap.db.initPage(*page, 42, 1001, 0x0007), true);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);

    if (hdr := readPageHeader(page[0..])) {
        expect_eq(hdr.page_id, 42);
        expect_eq(hdr.lsn, 1001);
        expect_eq(hdr.flags, 0x0007);
        expect_eq(hdr.slot_count, 0);
        expect_eq(hdr.free_start, SLOTTED_PAGE_HEADER_SIZE as u16);
        expect_eq(hdr.free_end, 4096);
        expect_eq(hdr.fragmented_bytes, 0);
    } else {
        expect_eq(true, false);
    }

    const s0 = insertRecord(*page, "row:alpha:100") ?? 999;
    const s1 = insertRecord(*page, "row:beta:20000") ?? 999;
    const s2 = insertRecord(*page, "row:gamma") ?? 999;
    expect_eq(s0, 0);
    expect_eq(s1, 1);
    expect_eq(s2, 2);

    // Zero-copy reads match inserted bytes exactly
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, "row:alpha:100"), true);
    } else {
        expect_eq(true, false);
    }
    if (r1 := getRecord(page[0..], s1)) {
        expect_eq(bytesEq(r1, "row:beta:20000"), true);
    } else {
        expect_eq(true, false);
    }
    if (r2 := getRecord(page[0..], s2)) {
        expect_eq(bytesEq(r2, "row:gamma"), true);
    } else {
        expect_eq(true, false);
    }

    expect_eq(setPageLsn(*page, 2002), true);
    const crc_opt = sealChecksum(*page);
    expect_eq(crc_opt != null, true);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);
    if (hdr2 := readPageHeader(page[0..])) {
        expect_eq(hdr2.lsn, 2002);
        expect_eq(hdr2.slot_count, 3);
        expect_eq(hdr2.crc32, slottedPageCrc32(page[0..]));
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 2: Tombstone Delete, Slot Reuse & Automatic In-Place Compaction
// ============================================================================

fn testAutoCompactionAndTombstoneSlotReuse()! {
    let page_arr: [256]u8 = [0; 256];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 7, 1, 0), true);

    let r_a: [40]u8 = [0x41; 40]; // 'A' x 40
    let r_b: [40]u8 = [0x42; 40]; // 'B' x 40
    let r_c: [40]u8 = [0x43; 40]; // 'C' x 40
    let r_d: [40]u8 = [0x44; 40]; // 'D' x 40

    const s0 = insertRecord(*page, r_a[0..]) ?? 999;
    const s1 = insertRecord(*page, r_b[0..]) ?? 999;
    const s2 = insertRecord(*page, r_c[0..]) ?? 999;
    const s3 = insertRecord(*page, r_d[0..]) ?? 999;
    expect_eq(s0, 0);
    expect_eq(s3, 3);

    // 256 - 32 (hdr) - 24 (4 slots) - 160 (4*40) = 40 bytes contiguous free
    if (fs1 := freeSpaceBytes(page[0..])) {
        expect_eq(fs1.contiguous_bytes, 40);
        expect_eq(fs1.fragmented_bytes, 0);
        expect_eq(fs1.reusable_slots, 0);
    } else {
        expect_eq(true, false);
    }

    // Delete middle slots 1 and 2 -> 80 bytes fragmented, 2 reusable slots
    expect_eq(deleteRecord(*page, s1), true);
    expect_eq(deleteRecord(*page, s2), true);
    expect_eq(getRecord(page[0..], s1) == null, true);
    expect_eq(getRecord(page[0..], s2) == null, true);

    if (fs2 := freeSpaceBytes(page[0..])) {
        expect_eq(fs2.contiguous_bytes, 40);
        expect_eq(fs2.fragmented_bytes, 80);
        expect_eq(fs2.total_reclaimable_bytes, 120);
        expect_eq(fs2.reusable_slots, 2);
    } else {
        expect_eq(true, false);
    }

    // Insert 100-byte record: exceeds 40B contiguous gap, triggers automatic compactPage + reuses slot 1!
    let big: [100]u8 = [0x5A; 100]; // 'Z' x 100
    const s_reused = insertRecord(*page, big[0..]) ?? 999;
    expect_eq(s_reused, s1);

    // Verify all live records survived compaction bit-for-bit
    if (g0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(g0, r_a[0..]), true);
    } else {
        expect_eq(true, false);
    }
    if (g1 := getRecord(page[0..], s_reused)) {
        expect_eq(bytesEq(g1, big[0..]), true);
    } else {
        expect_eq(true, false);
    }
    if (g3 := getRecord(page[0..], s3)) {
        expect_eq(bytesEq(g3, r_d[0..]), true);
    } else {
        expect_eq(true, false);
    }

    if (fs3 := freeSpaceBytes(page[0..])) {
        expect_eq(fs3.fragmented_bytes, 0);
        expect_eq(fs3.contiguous_bytes, 20); // 120 - 100
        expect_eq(fs3.reusable_slots, 1);    // slot 2 still deleted
    } else {
        expect_eq(true, false);
    }

    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);
}

// ============================================================================
// Test 3: updateRecordInPlace (Same-Size, Shrink, Growing Compaction, Atomic Fail)
// ============================================================================

fn testUpdateRecordInPlaceVariants()! {
    let page_arr: [256]u8 = [0; 256];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 9, 10, 0), true);

    const s0 = insertRecord(*page, "1234567890") ?? 999; // 10B
    const s1 = insertRecord(*page, "abcdefghij") ?? 999; // 10B

    // 1) Same-size overwrite in place
    expect_eq(updateRecordInPlace(*page, s0, "ABCDEFGHIJ"), true);
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, "ABCDEFGHIJ"), true);
    } else {
        expect_eq(true, false);
    }
    if (fs := freeSpaceBytes(page[0..])) {
        expect_eq(fs.fragmented_bytes, 0);
    } else {
        expect_eq(true, false);
    }

    // 2) Shrinking update (10B -> 4B) adds 6B to fragmented_bytes
    expect_eq(updateRecordInPlace(*page, s0, "MINI"), true);
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, "MINI"), true);
    } else {
        expect_eq(true, false);
    }
    if (fs := freeSpaceBytes(page[0..])) {
        expect_eq(fs.fragmented_bytes, 6);
    } else {
        expect_eq(true, false);
    }

    // 3) Growing update requiring compaction: fill remaining contiguous space first
    let filler: [170]u8 = [0x2E; 170];
    const s2 = insertRecord(*page, filler[0..]) ?? 999;
    expect_eq(s2, 2);

    // Remaining total for s0 growth: 256 - 32 - 18(3 slots) - 10(s1) - 170(s2) = 26 bytes
    let grown: [24]u8 = [0x47; 24]; // 'G' x 24
    expect_eq(updateRecordInPlace(*page, s0, grown[0..]), true);
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, grown[0..]), true);
    } else {
        expect_eq(true, false);
    }
    // Neighboring records intact
    if (r1 := getRecord(page[0..], s1)) {
        expect_eq(bytesEq(r1, "abcdefghij"), true);
    } else {
        expect_eq(true, false);
    }

    // 4) Oversized update (> 26B) fails atomically without corrupting s0
    let too_big: [40]u8 = [0x58; 40];
    expect_eq(updateRecordInPlace(*page, s0, too_big[0..]), false);
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, grown[0..]), true);
    } else {
        expect_eq(true, false);
    }

    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);
}

// ============================================================================
// Test 4: 8KB High-Density Page Stress (100 Records, Half-Delete, Compact, Reuse)
// ============================================================================

fn test8KbHighDensityCompactionStress()! {
    let page_arr: [8192]u8 = [0; 8192];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 888, 5000, 0), true);

    // Insert 100 variable-length records (20..59 bytes each)
    let i: usize = 0;
    while (i < 100) : (i += 1) {
        let rec: [64]u8 = [0; 64];
        const rlen: usize = (20 as usize) + (i % (40 as usize));
        let j: usize = 0;
        while (j < rlen) : (j += 1) {
            rec[j] = ((i + j) & 0xff) as u8;
        }
        const sid = insertRecord(*page, rec[0..rlen]) ?? 9999;
        expect_eq(sid, i as u16);
    }

    // Delete all 50 even slots
    let d: usize = 0;
    while (d < 100) : (d += 2) {
        expect_eq(deleteRecord(*page, d as u16), true);
    }

    // Explicit compactPage
    expect_eq(compactPage(*page), true);
    if (fs := freeSpaceBytes(page[0..])) {
        expect_eq(fs.fragmented_bytes, 0);
        expect_eq(fs.reusable_slots, 50);
    } else {
        expect_eq(true, false);
    }

    // Re-insert 50 new records -> must reuse all 50 even slots, keeping slot_count == 100
    let n: usize = 0;
    while (n < 50) : (n += 1) {
        let rec: [32]u8 = [0; 32];
        let j: usize = 0;
        while (j < 32) : (j += 1) {
            rec[j] = ((0xA0 + n + j) & 0xff) as u8;
        }
        const sid = insertRecord(*page, rec[0..32]) ?? 9999;
        expect_eq(sid, (n * 2) as u16);
    }

    if (hdr := readPageHeader(page[0..])) {
        expect_eq(hdr.slot_count, 100);
    } else {
        expect_eq(true, false);
    }

    // Verify every single odd (surviving) and even (reused) record
    let v: usize = 0;
    while (v < 100) : (v += 1) {
        if (got := getRecord(page[0..], v as u16)) {
            if ((v & 1) == 1) {
                const expected_len: usize = (20 as usize) + (v % (40 as usize));
                expect_eq(got.len, expected_len);
                expect_eq(got[0], (v & 0xff) as u8);
                expect_eq(got[expected_len - 1], ((v + expected_len - 1) & 0xff) as u8);
            } else {
                const n_idx: usize = v / 2;
                expect_eq(got.len, 32);
                expect_eq(got[0], ((0xA0 + n_idx) & 0xff) as u8);
            }
        } else {
            expect_eq(true, false);
        }
    }

    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);
}

// ============================================================================
// Test 5: Bit-Flip Tamper Detection & Corrupt Magic Detection
// ============================================================================

fn testChecksumTamperAndMagicGuards()! {
    let page_arr: [512]u8 = [0; 512];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 55, 777, 0), true);
    * = insertRecord(*page, "critical_tuple_payload");
    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);

    // Flip 1 bit in payload area -> checksum_mismatch
    page_arr[500] = page_arr[500] ^ 0x01;
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.checksum_mismatch, true);

    // Restore bit -> ok again
    page_arr[500] = page_arr[500] ^ 0x01;
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);

    // Corrupt magic byte -> invalid_magic
    page_arr[0] = 0x00;
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.invalid_magic, true);
    expect_eq(readPageHeader(page[0..]) == null, true);
}

// ============================================================================
// Test 6: Structural Integrity Detection (Overlapping Records & Corrupt Bounds)
// ============================================================================

fn testStructuralCorruptionDetection()! {
    let page_arr: [256]u8 = [0; 256];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 66, 1, 0), true);
    const s0 = insertRecord(*page, "AAAAAAAAAA") ?? 999; // [246..256)
    const s1 = insertRecord(*page, "BBBBBBBBBB") ?? 999; // [236..246)
    expect_eq(s0, 0);
    expect_eq(s1, 1);

    // Forge slot 1 offset to 240 so [240..250) overlaps slot 0 [246..256), then reseal CRC32
    const s1_dir: usize = SLOTTED_PAGE_HEADER_SIZE + SLOTTED_SLOT_SIZE;
    page_arr[s1_dir] = 240;
    page_arr[s1_dir + 1] = 0;
    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.overlapping_records, true);

    // Forge slot 1 offset below free_end (offset = 100 < free_end = 236), then reseal CRC32
    page_arr[s1_dir] = 100;
    page_arr[s1_dir + 1] = 0;
    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.corrupt_slot_bounds, true);
}

// ============================================================================
// Test 7: Page Size Boundaries (<64 Rejected, Exact 64B Filled) & Slot Guards
// ============================================================================

fn testPageSizeAndSlotBoundaryGuards()! {
    let tiny_arr: [63]u8 = [0; 63];
    let tiny: []u8 = tiny_arr[0..];
    expect_eq(initPage(*tiny, 1, 1, 0), false);
    expect_eq(verifyPageIntegrity(tiny[0..]) == SlottedPageStatus.invalid_page_size, true);

    // Minimum valid 64B page: 32B header + 6B slot + 26B payload = 64B exact fit!
    let min_arr: [64]u8 = [0; 64];
    let min_p: []u8 = min_arr[0..];
    expect_eq(initPage(*min_p, 1, 1, 0), true);
    let exact26: [26]u8 = [0x78; 26];
    const sid = insertRecord(*min_p, exact26[0..]) ?? 999;
    expect_eq(sid, 0);
    if (fs := freeSpaceBytes(min_p[0..])) {
        expect_eq(fs.contiguous_bytes, 0);
    } else {
        expect_eq(true, false);
    }
    // Page full: 1-byte insert rejected
    expect_eq(insertRecord(*min_p, "z") == null, true);

    // Deleted slot & OOB slot guards
    expect_eq(deleteRecord(*min_p, 0), true);
    expect_eq(deleteRecord(*min_p, 0), false);
    expect_eq(deleteRecord(*min_p, 5), false);
    expect_eq(getRecord(min_p[0..], 0) == null, true);
    expect_eq(getRecord(min_p[0..], 5) == null, true);
    expect_eq(updateRecordInPlace(*min_p, 0, "abc"), false);

    if (slot_meta := readSlotMeta(min_p[0..], 0)) {
        expect_eq(slot_meta.isDeleted(), true);
        expect_eq(slot_meta.isAlive(), false);
        expect_eq(slot_meta.flags, SLOT_FLAG_DELETED);
    } else {
        expect_eq(true, false);
    }

    expect_eq(SLOTTED_PAGE_MAGIC, 0x31535043);
    expect_eq(SLOTTED_PAGE_MIN_SIZE, 64);
    expect_eq(SLOTTED_PAGE_MAX_SIZE, 65535);
    expect_eq(SLOT_FLAG_ALIVE, 1);
}

// ============================================================================
// Test 8: Empty Record & Oversized Record Guards
// ============================================================================

fn testEmptyAndOversizedRecordGuards()! {
    let page_arr: [128]u8 = [0; 128];
    let page: []u8 = page_arr[0..];
    expect_eq(initPage(*page, 12, 34, 0), true);

    // Empty record rejected on insert & update
    expect_eq(insertRecord(*page, "") == null, true);
    const s0 = insertRecord(*page, "hello") ?? 999;
    expect_eq(s0, 0);
    expect_eq(updateRecordInPlace(*page, s0, ""), false);

    // Oversized record (100B > remaining 128 - 32 - 6 - 5 = 85B) rejected cleanly
    let huge: [100]u8 = [0xFF; 100];
    expect_eq(insertRecord(*page, huge[0..]) == null, true);

    // Existing record remains intact & page verifies ok
    if (r0 := getRecord(page[0..], s0)) {
        expect_eq(bytesEq(r0, "hello"), true);
    } else {
        expect_eq(true, false);
    }
    * = sealChecksum(*page);
    expect_eq(verifyPageIntegrity(page[0..]) == SlottedPageStatus.ok, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("db.slotted_page: 4KB page init, insert, zero-copy getRecord, LSN & CRC32 verify", test4KbPageBasicCrudAndChecksum);
test("db.slotted_page: tombstone delete, slot reuse & automatic O(1) in-place compaction", testAutoCompactionAndTombstoneSlotReuse);
test("db.slotted_page: updateRecordInPlace same-size, shrink, growing compaction & atomic OOM guard", testUpdateRecordInPlaceVariants);
test("db.slotted_page: 8KB high-density 100-record compaction & full slot reuse stress", test8KbHighDensityCompactionStress);
test("db.slotted_page: bit-flip CRC32 tamper detection & invalid magic guard", testChecksumTamperAndMagicGuards);
test("db.slotted_page: structural corruption detection (overlapping_records & corrupt_slot_bounds)", testStructuralCorruptionDetection);
test("db.slotted_page: page size boundary guards (<64 rejected, exact 64B fit) & slot guards", testPageSizeAndSlotBoundaryGuards);
test("db.slotted_page: empty record & oversized record rejection preserving integrity", testEmptyAndOversizedRecordGuards);
