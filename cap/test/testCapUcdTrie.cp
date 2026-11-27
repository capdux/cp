// cap/test/testCapUcdTrie.cp
// Comprehensive unit tests for @cap/ucd-trie

import { test, expect_eq } from "cap:test";
import { UcdTrie } from "../../packages/ucd-trie/src/index.cp";

fn main() {
    test("1. @cap/ucd-trie — Single codepoint insertion & O(1) contains", () => {
        let level1: [1024]u16 = undefined;
        let chunks: [32]u64 = undefined;
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..1024], chunks[0..32]), true);
        expect_eq(trie.max_cp, 0xFFFF as u32);
        expect_eq(trie.dedupChunkCount(), 0 as usize);

        expect_eq(trie.insertCodepoint(level1[0..1024], chunks[0..32], 0x0041), true); // 'A'
        expect_eq(trie.insertCodepoint(level1[0..1024], chunks[0..32], 0x007A), true); // 'z'
        expect_eq(trie.insertCodepoint(level1[0..1024], chunks[0..32], 0x4E2D), true); // '中'

        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x0041), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x007A), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4E2D), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x0040), false);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x0042), false);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4E2C), false);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4E2E), false);
    });

    test("2. @cap/ucd-trie — insertRange('A', 'Z') + insertRange(0x4E00, 0x4EFF) exact boundaries", () => {
        let level1: [1024]u16 = undefined;
        let chunks: [32]u64 = undefined;
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..1024], chunks[0..32]), true);

        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x0041, 0x005A), true);
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x4E00, 0x4EFF), true);

        // ASCII 'A'..'Z' boundaries
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x0040), false);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x0041), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x004D), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x005A), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x005B), false);

        // CJK 0x4E00..0x4EFF boundaries
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4DFF), false);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4E00), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4E7F), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4EFF), true);
        expect_eq(trie.contains(level1[0..1024], chunks[0..32], 0x4F00), false);
    });

    test("3. @cap/ucd-trie — Sparse non-contiguous BMP high compression (dedupChunkCount)", () => {
        let level1: [1024]u16 = undefined;
        let chunks: [32]u64 = undefined;
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..1024], chunks[0..32]), true);

        // Insert into 3 widely separated 64-codepoint blocks across the 1024-block BMP
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x0041, 0x005A), true); // block 1
        expect_eq(trie.insertCodepoint(level1[0..1024], chunks[0..32], 0x20AC), true);     // block 130 (€)
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x4E00, 0x4E3F), true); // block 312 (64 cps)

        // Only 3 non-zero chunks allocated out of 1024 BMP blocks (>99.7% compression!)
        expect_eq(trie.dedupChunkCount(), 3 as usize);
        expect_eq(chunks[0], 0 as u64);
    });

    test("4. @cap/ucd-trie — countMembers exact population count with overlap idempotency", () => {
        let level1: [1024]u16 = undefined;
        let chunks: [32]u64 = undefined;
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..1024], chunks[0..32]), true);

        // 26 uppercase + 256 CJK = 282 members
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x0041, 0x005A), true);
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x4E00, 0x4EFF), true);
        expect_eq(trie.countMembers(level1[0..1024], chunks[0..32]), 282 as u64);

        // Re-inserting overlapping range does not double-count
        expect_eq(trie.insertRange(level1[0..1024], chunks[0..32], 0x0050, 0x005A), true);
        expect_eq(trie.countMembers(level1[0..1024], chunks[0..32]), 282 as u64);
    });

    test("5. @cap/ucd-trie — Out-of-range codepoints & reversed ranges safely return false", () => {
        let level1: [16]u16 = undefined; // covers 0..1023
        let chunks: [8]u64 = undefined;
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..16], chunks[0..8]), true);
        expect_eq(trie.max_cp, 1023 as u32);

        expect_eq(trie.insertCodepoint(level1[0..16], chunks[0..8], 1024), false);
        expect_eq(trie.insertCodepoint(level1[0..16], chunks[0..8], 0x110000), false);
        expect_eq(trie.contains(level1[0..16], chunks[0..8], 1024), false);
        expect_eq(trie.contains(level1[0..16], chunks[0..8], 0x10FFFF), false);

        // Reversed range (lo > hi) & range exceeding max_cp
        expect_eq(trie.insertRange(level1[0..16], chunks[0..8], 50, 40), false);
        expect_eq(trie.insertRange(level1[0..16], chunks[0..8], 1000, 1024), false);
    });

    test("6. @cap/ucd-trie — Chunk pool exhaustion boundary safety", () => {
        let level1: [64]u16 = undefined;
        let chunks: [3]u64 = undefined; // chunks[0] = zero block, only 2 allocatable non-zero chunks!
        let trie = UcdTrie.new();
        expect_eq(trie.init(level1[0..64], chunks[0..3]), true);

        // Block 0 & Block 1 succeed
        expect_eq(trie.insertCodepoint(level1[0..64], chunks[0..3], 0), true);
        expect_eq(trie.insertCodepoint(level1[0..64], chunks[0..3], 64), true);
        expect_eq(trie.dedupChunkCount(), 2 as usize);

        // Another codepoint in already-allocated block 1 still succeeds
        expect_eq(trie.insertCodepoint(level1[0..64], chunks[0..3], 65), true);

        // New block 2 fails cleanly when pool is full
        expect_eq(trie.insertCodepoint(level1[0..64], chunks[0..3], 128), false);
        expect_eq(trie.contains(level1[0..64], chunks[0..3], 128), false);
        expect_eq(trie.countMembers(level1[0..64], chunks[0..3]), 3 as u64);
    });
}
