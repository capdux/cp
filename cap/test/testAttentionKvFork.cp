// Origin: Pure-CP Stdlib Test Suite (lib/attention/kvFork.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Best-of-N / Tree-of-Thoughts Shared KV-Cache CoW Fork Manager:
//   1. Best-of-4 zero-copy sequence forking, refcount tracking & 750-permille memory savings
//   2. Copy-on-Write (`prepareWriteBlock`): shared split vs exclusive in-place write & stale-hash invalidation
//   3. Content-hash zero-copy prefix deduplication (`reused_by_hash == true`) & `sealBlockHash`
//   4. Tree-of-Thoughts multi-level branching & full release zero-leak verification
//   5. Physical block pool OOM atomic safety (failed CoW leaves refcounts & mappings intact)
//   6. Invalid ID, duplicate seq, 32-seq / 32-block capacity & OOB logical index guards
//   7. `cap.attention` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    KV_FORK_MAX_PHYS_BLOCKS,
    KV_FORK_MAX_SEQS,
    KV_FORK_MAX_LOGICAL_BLOCKS,
    KvForkOpStatus,
    KvForkManager,
} from "cap:attention";

test("KvFork: Best-of-4 zero-copy forking, refcounts & memorySavingsPermille") {
    let mgr = KvForkManager.init(64) ?? return;
    expect_eq(mgr.createSequence(1), KvForkOpStatus.ok);

    // Append 4 prefix blocks to sequence 1
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        const app = mgr.appendBlock(1, 0x1000 + (i as u64));
        expect_eq(app.status, KvForkOpStatus.ok);
        expect_true(!app.reused_by_hash);
    }

    // Fork into 3 additional beams (total 4 sequences sharing 4 physical blocks)
    expect_eq(mgr.forkSequence(1, 2), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(1, 3), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(1, 4), KvForkOpStatus.ok);

    // All 4 sequences share identical physical blocks with refcount == 4
    const p0 = mgr.getPhysicalBlock(1, 0) ?? 999;
    expect_eq(mgr.getPhysicalBlock(2, 0) ?? 999, p0);
    expect_eq(mgr.getPhysicalBlock(4, 0) ?? 999, p0);
    expect_eq(mgr.blockRefcount(p0), 4 as u16);

    // 16 logical refs backed by 4 physical blocks -> (16 - 4) * 1000 / 16 = 750 permille savings!
    expect_eq(mgr.memorySavingsPermille(), 750 as u16);
    const st = mgr.stats();
    expect_eq(st.phys_used, 4 as usize);
    expect_eq(st.total_logical_refs, 16 as usize);
    expect_eq(st.total_forks, 3 as u64);
}

test("KvFork: Copy-on-Write split vs exclusive in-place write & stale-hash invalidation") {
    let mgr = KvForkManager.init(16) ?? return;
    * = mgr.createSequence(1);
    const b0 = mgr.appendBlock(1, 0xAAAA);
    const b1 = mgr.appendBlock(1, 0xBBBB);
    expect_eq(b0.status, KvForkOpStatus.ok);
    expect_eq(b1.status, KvForkOpStatus.ok);

    expect_eq(mgr.forkSequence(1, 2), KvForkOpStatus.ok);
    expect_eq(mgr.blockRefcount(b1.phys_block), 2 as u16);

    // Sequence 2 mutates shared logical block 1 -> triggers CoW split!
    const w1 = mgr.prepareWriteBlock(2, 1);
    expect_eq(w1.status, KvForkOpStatus.ok);
    expect_true(w1.copied);
    expect_eq(w1.old_phys, b1.phys_block);
    expect_true(w1.new_phys != w1.old_phys);
    expect_eq(mgr.blockRefcount(w1.old_phys), 1 as u16);
    expect_eq(mgr.blockRefcount(w1.new_phys), 1 as u16);
    expect_eq(mgr.getPhysicalBlock(2, 1) ?? 999, w1.new_phys);
    expect_eq(mgr.getPhysicalBlock(1, 1) ?? 999, w1.old_phys);

    // Second write on sequence 2's now-exclusive block -> in-place (copied == false)
    const w2 = mgr.prepareWriteBlock(2, 1);
    expect_eq(w2.status, KvForkOpStatus.ok);
    expect_true(!w2.copied);
    expect_eq(w2.old_phys, w1.new_phys);
    expect_eq(w2.new_phys, w1.new_phys);

    // Sequence 1 mutating its now-exclusive block 1 clears stale hash 0xBBBB
    const w3 = mgr.prepareWriteBlock(1, 1);
    expect_eq(w3.status, KvForkOpStatus.ok);
    expect_true(!w3.copied);

    // A new sequence 3 appending 0xBBBB must NOT falsely dedup against mutated block 1
    * = mgr.createSequence(3);
    const b3 = mgr.appendBlock(3, 0xBBBB);
    expect_eq(b3.status, KvForkOpStatus.ok);
    expect_true(!b3.reused_by_hash);
    expect_true(b3.phys_block != b1.phys_block);
}

test("KvFork: content-hash zero-copy prefix deduplication & sealBlockHash") {
    let mgr = KvForkManager.init(32) ?? return;
    * = mgr.createSequence(10);
    * = mgr.createSequence(20);

    const a1 = mgr.appendBlock(10, 0xDEAD_BEEF);
    const a2 = mgr.appendBlock(10, 0); // unhashed private block
    expect_true(!a1.reused_by_hash);
    expect_true(!a2.reused_by_hash);

    // Seal private block a2 with hash 0xCAFE_BABE
    expect_eq(mgr.sealBlockHash(10, 1, 0xCAFE_BABE), KvForkOpStatus.ok);

    // Independent sequence 20 appends same two hashes -> zero-copy dedup hits both!
    const c1 = mgr.appendBlock(20, 0xDEAD_BEEF);
    const c2 = mgr.appendBlock(20, 0xCAFE_BABE);
    expect_true(c1.reused_by_hash);
    expect_true(c2.reused_by_hash);
    expect_eq(c1.phys_block, a1.phys_block);
    expect_eq(c2.phys_block, a2.phys_block);
    expect_eq(mgr.stats().phys_used, 2 as usize);
    expect_eq(mgr.stats().total_hash_dedups, 2 as u64);
}

test("KvFork: Tree-of-Thoughts multi-level branching & zero-leak full release") {
    let mgr = KvForkManager.init(64) ?? return;

    // Root sequence 1 (3 blocks)
    * = mgr.createSequence(1);
    * = mgr.appendBlock(1, 0x01);
    * = mgr.appendBlock(1, 0x02);
    * = mgr.appendBlock(1, 0x03);

    // Level 1 branches: 10 and 20
    expect_eq(mgr.forkSequence(1, 10), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(1, 20), KvForkOpStatus.ok);
    * = mgr.appendBlock(10, 0x10);
    * = mgr.appendBlock(20, 0x20);

    // Level 2 branches under 10 and 20: 101, 102, 201, 202
    expect_eq(mgr.forkSequence(10, 101), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(10, 102), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(20, 201), KvForkOpStatus.ok);
    expect_eq(mgr.forkSequence(20, 202), KvForkOpStatus.ok);

    // Trigger CoW on leaf 101 and release intermediate parents early
    expect_true(mgr.prepareWriteBlock(101, 3).copied);
    expect_eq(mgr.releaseSequence(1), KvForkOpStatus.ok);
    expect_eq(mgr.releaseSequence(10), KvForkOpStatus.ok);
    expect_eq(mgr.releaseSequence(20), KvForkOpStatus.ok);

    // Root prefix blocks must still be alive (referenced by 4 leaves)
    expect_eq(mgr.blockRefcount(mgr.getPhysicalBlock(101, 0) ?? 999), 4 as u16);

    // Release all remaining leaves -> zero physical blocks leaked!
    expect_eq(mgr.releaseSequence(101), KvForkOpStatus.ok);
    expect_eq(mgr.releaseSequence(102), KvForkOpStatus.ok);
    expect_eq(mgr.releaseSequence(201), KvForkOpStatus.ok);
    expect_eq(mgr.releaseSequence(202), KvForkOpStatus.ok);

    const st = mgr.stats();
    expect_eq(st.active_sequences, 0 as usize);
    expect_eq(st.phys_used, 0 as usize);
    expect_eq(st.phys_free, 64 as usize);
    expect_eq(st.total_releases, 7 as u64);
}

test("KvFork: physical block pool OOM atomic safety during CoW") {
    let mgr = KvForkManager.init(2) ?? return;
    * = mgr.createSequence(1);
    const b0 = mgr.appendBlock(1, 0x11);
    const b1 = mgr.appendBlock(1, 0x22);
    expect_eq(b0.status, KvForkOpStatus.ok);
    expect_eq(b1.status, KvForkOpStatus.ok);

    // Pool is now full (2/2), so appending a new unhashed block fails
    expect_eq(mgr.appendBlock(1, 0).status, KvForkOpStatus.out_of_phys_blocks);

    // Fork still succeeds (zero-copy!)
    expect_eq(mgr.forkSequence(1, 2), KvForkOpStatus.ok);

    // CoW on shared block fails cleanly with out_of_phys_blocks, preserving refcount == 2!
    const fail_cow = mgr.prepareWriteBlock(2, 0);
    expect_eq(fail_cow.status, KvForkOpStatus.out_of_phys_blocks);
    expect_true(!fail_cow.copied);
    expect_eq(mgr.blockRefcount(b0.phys_block), 2 as u16);
    expect_eq(mgr.getPhysicalBlock(2, 0) ?? 999, b0.phys_block);

    // Releasing seq 1 drops refcount to 1 -> seq 2 can now write in-place with zero extra blocks!
    expect_eq(mgr.releaseSequence(1), KvForkOpStatus.ok);
    const ok_write = mgr.prepareWriteBlock(2, 0);
    expect_eq(ok_write.status, KvForkOpStatus.ok);
    expect_true(!ok_write.copied);
}

test("KvFork: invalid args, duplicate seq, 32-seq / 32-block limits & OOB guards") {
    expect_true(KvForkManager.init(0) == null);
    expect_true(KvForkManager.init(KV_FORK_MAX_PHYS_BLOCKS + 1) == null);

    let mgr = KvForkManager.initDefault();
    expect_eq(mgr.createSequence(0), KvForkOpStatus.invalid_arg);
    expect_eq(mgr.createSequence(1), KvForkOpStatus.ok);
    expect_eq(mgr.createSequence(1), KvForkOpStatus.duplicate_seq_id);
    expect_eq(mgr.forkSequence(999, 2), KvForkOpStatus.seq_not_found);
    expect_eq(mgr.prepareWriteBlock(1, 0).status, KvForkOpStatus.invalid_logical_index);
    expect_eq(mgr.releaseSequence(999), KvForkOpStatus.seq_not_found);

    // Fill sequence 1 to 32 logical blocks
    let b: usize = 0;
    while (b < KV_FORK_MAX_LOGICAL_BLOCKS) : (b += 1) {
        expect_eq(mgr.appendBlock(1, 0).status, KvForkOpStatus.ok);
    }
    expect_eq(mgr.appendBlock(1, 0).status, KvForkOpStatus.seq_blocks_full);
    expect_eq(mgr.sequenceLength(1) ?? 0, KV_FORK_MAX_LOGICAL_BLOCKS);

    // Fill remaining 31 sequence slots up to KV_FORK_MAX_SEQS (32)
    let sid: u64 = 2;
    while (sid <= KV_FORK_MAX_SEQS as u64) : (sid += 1) {
        expect_eq(mgr.createSequence(sid), KvForkOpStatus.ok);
    }
    expect_eq(mgr.createSequence(99), KvForkOpStatus.seq_capacity_full);
}

test("KvFork: cap.attention namespace facade parity") {
    let mgr = cap.attention.KvForkManager.init(32) ?? return;
    expect_eq(mgr.createSequence(1), cap.attention.KvForkOpStatus.ok);
    * = mgr.appendBlock(1, 0x5555);
    expect_eq(mgr.forkSequence(1, 2), cap.attention.KvForkOpStatus.ok);
    expect_eq(mgr.memorySavingsPermille(), 500 as u16);
    expect_true(mgr.prepareWriteBlock(2, 0).copied);
    expect_eq(mgr.memorySavingsPermille(), 0 as u16);
}
