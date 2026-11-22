// Origin: Pure-CP Stdlib Test Suite (AI Agent Bounded Priority Mailbox with Backpressure & Key Coalescing) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/mailbox.cp
// Covers:
//   1. 4-tier priority descending dequeue (System > High > Normal > Low) + intra-tier FIFO ordering & peekNext
//   2. RejectNew backpressure policy rejecting incoming messages when full
//   3. DropLowestOldest evicting oldest lowest-priority entry while strictly protecting System/higher priority
//   4. CoalesceByKey in-place state overwrite (including on full mailbox), priority upgrade & coalesced_total
//   5. Empty mailbox, clear(), zero-capacity guard & exact MailboxStats accounting

import { test, expect_eq } from "cap:test";
import {
    MailPriority,
    BackpressurePolicy,
    MailEnvelope,
    SendOutcome,
    MailboxStats,
    AgentMailbox,
} from "../../lib/agent/index.cp";

fn testFourTierPriorityAndFifoOrdering()! {
    let slots_buf: [8]MailEnvelope = [MailEnvelope{}; 8];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.RejectNew);

    expect_eq(mbox.send(1, MailPriority.Low, 0, 1, 10) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(2, MailPriority.Normal, 0, 2, 20) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(3, MailPriority.Normal, 0, 2, 21) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(4, MailPriority.High, 0, 3, 30) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(5, MailPriority.System, 0, 4, 40) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(6, MailPriority.High, 0, 3, 31) == SendOutcome.Enqueued, true);

    expect_eq(mbox.len(), 6);
    expect_eq(mbox.isEmpty(), false);

    // peekNext does not remove the top message
    const peeked = mbox.peekNext() ?? MailEnvelope{};
    expect_eq(peeked.payload_u64, 40);
    expect_eq(mbox.len(), 6);

    // Exact dequeue order: System(40) -> High(30) -> High(31) -> Normal(20) -> Normal(21) -> Low(10)
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 40);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 30);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 31);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 20);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 21);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 10);

    expect_eq(mbox.isEmpty(), true);
    expect_eq(mbox.recvNext() == null, true);
}

fn testRejectNewBackpressureWhenFull()! {
    let slots_buf: [2]MailEnvelope = [MailEnvelope{}; 2];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.RejectNew);

    expect_eq(mbox.send(1, MailPriority.Low, 0, 1, 100) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(2, MailPriority.Low, 0, 1, 200) == SendOutcome.Enqueued, true);
    // Full mailbox rejects even a System message under RejectNew
    expect_eq(mbox.send(3, MailPriority.System, 0, 9, 900) == SendOutcome.RejectedFull, true);

    const st: MailboxStats = mbox.stats();
    expect_eq(st.len, 2);
    expect_eq(st.capacity, 2);
    expect_eq(st.enqueued_total, 2);
    expect_eq(st.rejected_total, 1);
    expect_eq(st.dropped_total, 0);
}

fn testDropLowestOldestAndSystemProtection()! {
    let slots_buf: [3]MailEnvelope = [MailEnvelope{}; 3];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.DropLowestOldest);

    // Fill with: System(1), Low(old=10), Low(newer=11)
    expect_eq(mbox.send(1, MailPriority.System, 0, 1, 1) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(2, MailPriority.Low, 0, 2, 10) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(3, MailPriority.Low, 0, 2, 11) == SendOutcome.Enqueued, true);

    // Incoming High(50) must evict the oldest Low(10), preserving System(1) and Low(11)
    expect_eq(mbox.send(4, MailPriority.High, 0, 3, 50) == SendOutcome.EvictedAndEnqueued, true);
    expect_eq(mbox.stats().dropped_total, 1);

    // Fill remaining Low slot with System(2) -> evicts Low(11)
    expect_eq(mbox.send(5, MailPriority.System, 0, 1, 2) == SendOutcome.EvictedAndEnqueued, true);
    // Now mailbox has: System(1), High(50), System(2).
    // Replace High(50) with System(3) -> evicts High(50). Now all 3 slots are System!
    expect_eq(mbox.send(6, MailPriority.System, 0, 1, 3) == SendOutcome.EvictedAndEnqueued, true);

    // Low or Normal or High message MUST NOT evict System messages -> RejectedFull!
    expect_eq(mbox.send(7, MailPriority.High, 0, 3, 99) == SendOutcome.RejectedFull, true);
    expect_eq(mbox.send(8, MailPriority.Normal, 0, 2, 88) == SendOutcome.RejectedFull, true);

    expect_eq(mbox.stats().dropped_total, 3);
    expect_eq(mbox.stats().rejected_total, 2);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 1);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 2);
    expect_eq((mbox.recvNext() ?? MailEnvelope{}).payload_u64, 3);
}

fn testCoalesceByKeyInPlaceOverwrite()! {
    let slots_buf: [2]MailEnvelope = [MailEnvelope{}; 2];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.CoalesceByKey);

    expect_eq(mbox.send(1, MailPriority.Normal, 0xCAFE, 10, 100) == SendOutcome.Enqueued, true);
    expect_eq(mbox.send(2, MailPriority.Normal, 0xBEEF, 20, 200) == SendOutcome.Enqueued, true);

    // Mailbox is now full (2/2), yet matching key 0xCAFE coalesces in place & upgrades priority to High!
    expect_eq(mbox.send(3, MailPriority.High, 0xCAFE, 11, 199) == SendOutcome.Coalesced, true);
    expect_eq(mbox.send(4, MailPriority.Low, 0xCAFE, 12, 299) == SendOutcome.Coalesced, true);

    // Unmatched key on full mailbox is rejected
    expect_eq(mbox.send(5, MailPriority.Normal, 0xDEAD, 30, 300) == SendOutcome.RejectedFull, true);

    const st = mbox.stats();
    expect_eq(st.len, 2);
    expect_eq(st.enqueued_total, 2);
    expect_eq(st.coalesced_total, 2);
    expect_eq(st.rejected_total, 1);

    // Coalesced 0xCAFE kept upgraded High priority (did not downgrade to Low) and latest payload 299
    const first = mbox.recvNext() ?? MailEnvelope{};
    expect_eq(first.coalesce_key, 0xCAFE);
    expect_eq(first.priority == MailPriority.High, true);
    expect_eq(first.payload_tag, 12);
    expect_eq(first.payload_u64, 299);

    const second = mbox.recvNext() ?? MailEnvelope{};
    expect_eq(second.coalesce_key, 0xBEEF);
    expect_eq(second.payload_u64, 200);
}

fn testEmptyClearZeroCapacityAndStats()! {
    // 1. Zero-capacity mailbox never panics
    let zero_buf: [0]MailEnvelope = [];
    let zero_slots: []MailEnvelope = zero_buf[0..];
    let zbox = AgentMailbox.init(zero_slots, BackpressurePolicy.DropLowestOldest);
    expect_eq(zbox.isEmpty(), true);
    expect_eq(zbox.peekNext() == null, true);
    expect_eq(zbox.recvNext() == null, true);
    expect_eq(zbox.send(1, MailPriority.System, 0, 1, 1) == SendOutcome.RejectedFull, true);
    expect_eq(zbox.stats().rejected_total, 1);

    // 2. clear() empties slots while retaining cumulative counters
    let slots_buf: [4]MailEnvelope = [MailEnvelope{}; 4];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.RejectNew);

    * = mbox.send(1, MailPriority.Normal, 0, 1, 10);
    * = mbox.send(2, MailPriority.High, 0, 2, 20);
    * = mbox.recvNext();
    expect_eq(mbox.len(), 1);

    mbox.clear();
    expect_eq(mbox.len(), 0);
    expect_eq(mbox.isEmpty(), true);
    expect_eq(mbox.peekNext() == null, true);

    const st = mbox.stats();
    expect_eq(st.enqueued_total, 2);
    expect_eq(st.dequeued_total, 1);
}

test("agent mailbox: 4-tier priority descending and intra-tier FIFO dequeue", testFourTierPriorityAndFifoOrdering);
test("agent mailbox: RejectNew rejects messages when full", testRejectNewBackpressureWhenFull);
test("agent mailbox: DropLowestOldest evicts lowest oldest while protecting System priority", testDropLowestOldestAndSystemProtection);
test("agent mailbox: CoalesceByKey in-place overwrite on full mailbox and priority preservation", testCoalesceByKeyInPlaceOverwrite);
test("agent mailbox: empty mailbox, clear, zero-capacity guard and stats accounting", testEmptyClearZeroCapacityAndStats);
