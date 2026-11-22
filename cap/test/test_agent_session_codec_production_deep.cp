// Origin: Pure-CP Stdlib Test Suite (lib/agent/session_codec.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Versioned & CRC32-Verified Session Snapshot Codec:
//   1. Full-field roundtrip (large tokens, cost, signed updated_ms, is_paused & UTF-8 summaries)
//   2. Empty optional summaries & minimal 68-byte empty snapshot roundtrip
//   3. Single-bit flip CRC32 tamper interception (header, payload & footer)
//   4. Invalid magic, unsupported version & invalid flags error classification
//   5. Truncated input & trailing garbage bytes rejection
//   6. Exact-fit vs undersized output buffer atomic encode guard

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AGENT_SNAPSHOT_MAGIC,
    AGENT_SNAPSHOT_VERSION,
    SnapshotDecodeError,
    AgentSessionSnapshot,
    encodedSnapshotSize,
    encodeSessionSnapshot,
    decodeSessionSnapshot,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn testCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc = crc ^ (data[i] as u32);
        let bit: u8 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return ~crc;
}

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn expectDecodeErr(data: []const u8, expected: SnapshotDecodeError): bool {
    let matched = false;
    * = decodeSessionSnapshot(data) catch (e) {
        matched = e == expected;
        return matched;
    };
    return false;
}

// ============================================================================
// Test 1: Full-Field Roundtrip (Including Negative Timestamp & UTF-8 Text)
// ============================================================================

fn testFullFieldRoundtrip()! {
    expect_eq(AGENT_SNAPSHOT_MAGIC, 0x41475353);
    expect_eq(AGENT_SNAPSHOT_VERSION, 1);

    const orig = AgentSessionSnapshot{
        session_id: "sess_9f8e7d6c",
        agent_name: "nexus-coder",
        turn_count: 14,
        tool_call_count: 37,
        input_tokens: 256000,
        output_tokens: 48200,
        cost_micro_usd: 1491000,
        updated_ms: -42000, // signed negative timestamp verified via ZigZag
        is_paused: true,
        goal_summary: "迁移并验收 CP 标准库模块",
        last_checkpoint_note: "waiting on tool #37 result",
    };

    const expected_sz = cap.agent.encodedSnapshotSize(orig);
    let buf: [512]u8 = [0; 512];
    const written = encodeSessionSnapshot(orig, buf[0..]) ?? return error.EncodeFailed;
    expect_eq(written, expected_sz);

    const restored = decodeSessionSnapshot(buf[0..written])?;
    expect_eq(strEq(restored.session_id, orig.session_id), true);
    expect_eq(strEq(restored.agent_name, orig.agent_name), true);
    expect_eq(restored.turn_count, 14);
    expect_eq(restored.tool_call_count, 37);
    expect_eq(restored.input_tokens, 256000);
    expect_eq(restored.output_tokens, 48200);
    expect_eq(restored.cost_micro_usd, 1491000);
    expect_eq(restored.updated_ms, -42000);
    expect_eq(restored.is_paused, true);
    expect_eq(strEq(restored.goal_summary, orig.goal_summary), true);
    expect_eq(strEq(restored.last_checkpoint_note, orig.last_checkpoint_note), true);
}

// ============================================================================
// Test 2: Empty Optional Summaries & Minimal 68-Byte Snapshot
// ============================================================================

fn testEmptyOptionalSummaries()! {
    const empty_snap = AgentSessionSnapshot{
        session_id: "",
        agent_name: "",
        turn_count: 0,
        tool_call_count: 0,
        input_tokens: 0,
        output_tokens: 0,
        cost_micro_usd: 0,
        updated_ms: 0,
        is_paused: false,
        goal_summary: "",
        last_checkpoint_note: "",
    };

    expect_eq(encodedSnapshotSize(empty_snap), 68);
    let buf: [68]u8 = [0; 68];
    const written = encodeSessionSnapshot(empty_snap, buf[0..]) ?? return error.EncodeFailed;
    expect_eq(written, 68);

    const restored = decodeSessionSnapshot(buf[0..written])?;
    expect_eq(restored.session_id.len, 0);
    expect_eq(restored.agent_name.len, 0);
    expect_eq(restored.goal_summary.len, 0);
    expect_eq(restored.last_checkpoint_note.len, 0);
    expect_eq(restored.is_paused, false);
}

// ============================================================================
// Test 3: Single-Bit Flip CRC32 Tamper Interception
// ============================================================================

fn testSingleBitTamperChecksumMismatch()! {
    const snap = AgentSessionSnapshot{
        session_id: "s1",
        agent_name: "agentA",
        turn_count: 3,
        tool_call_count: 5,
        input_tokens: 1000,
        output_tokens: 500,
        cost_micro_usd: 2500,
        updated_ms: 1700000000000,
        is_paused: false,
        goal_summary: "goal",
        last_checkpoint_note: "note",
    };

    let buf: [256]u8 = [0; 256];
    const written = encodeSessionSnapshot(snap, buf[0..]) ?? return error.EncodeFailed;

    // 3a. Flip 1 bit in turn_count (header byte 8)
    buf[8] = buf[8] ^ 0x01;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.ChecksumMismatch), true);
    buf[8] = buf[8] ^ 0x01;

    // 3b. Flip 1 bit in string payload (byte 65)
    buf[65] = buf[65] ^ 0x04;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.ChecksumMismatch), true);
    buf[65] = buf[65] ^ 0x04;

    // 3c. Flip 1 bit in trailing CRC32 footer
    buf[written - 1] = buf[written - 1] ^ 0x80;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.ChecksumMismatch), true);
}

// ============================================================================
// Test 4: Invalid Magic, Unsupported Version & Invalid Flags
// ============================================================================

fn testHeaderErrorClassification()! {
    const snap = AgentSessionSnapshot{ session_id: "s", agent_name: "a" };
    let buf: [128]u8 = [0; 128];
    const written = encodeSessionSnapshot(snap, buf[0..]) ?? return error.EncodeFailed;

    // 4a. Invalid magic
    buf[0] = 0xFF;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.InvalidMagic), true);
    putU32LE(buf[0..], 0, AGENT_SNAPSHOT_MAGIC);

    // 4b. Unsupported version (version = 2)
    buf[4] = 2;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.UnsupportedVersion), true);
    buf[4] = 1;

    // 4c. Invalid flag bits (bit 2 set)
    buf[6] = 0x04;
    expect_eq(expectDecodeErr(buf[0..written], SnapshotDecodeError.InvalidFlags), true);
}

// ============================================================================
// Test 5: Truncated Input & Trailing Garbage Bytes Rejection
// ============================================================================

fn testTruncatedAndTrailingBytes()! {
    const snap = AgentSessionSnapshot{ session_id: "sid", agent_name: "ag" };
    let buf: [128]u8 = [0; 128];
    const written = encodeSessionSnapshot(snap, buf[0..]) ?? return error.EncodeFailed;

    // 5a. Short buffer (< 68 bytes) -> TruncatedInput
    expect_eq(expectDecodeErr(buf[0..32], SnapshotDecodeError.TruncatedInput), true);

    // 5b. Craft valid-CRC buffer with 2 extra trailing garbage bytes -> TrailingBytes
    let extra: [128]u8 = [0; 128];
    const body_len = written - 4;
    let i: usize = 0;
    while (i < body_len) : (i += 1) { extra[i] = buf[i]; }
    extra[body_len] = 0xDE;
    extra[body_len + 1] = 0xAD;
    const new_body = body_len + 2;
    putU32LE(extra[0..], new_body, testCrc32(extra[0..new_body]));

    expect_eq(expectDecodeErr(extra[0 .. new_body + 4], SnapshotDecodeError.TrailingBytes), true);
}

// ============================================================================
// Test 6: Exact-Fit vs Undersized Output Buffer Atomic Guard
// ============================================================================

fn testUndersizedBufferEncodeGuard()! {
    const snap = AgentSessionSnapshot{
        session_id: "sess_1",
        agent_name: "planner",
        goal_summary: "deploy",
        last_checkpoint_note: "step 3",
    };
    const needed = encodedSnapshotSize(snap);
    let buf: [256]u8 = [0; 256];

    // 1 byte short -> atomic null
    expect_eq(encodeSessionSnapshot(snap, buf[0 .. needed - 1]) == null, true);
    // Exact fit -> succeeds
    const written = encodeSessionSnapshot(snap, buf[0..needed]) ?? return error.EncodeFailed;
    expect_eq(written, needed);
}

test("agent.session_codec: full-field roundtrip with signed timestamp & UTF-8", testFullFieldRoundtrip);
test("agent.session_codec: empty optional summaries & minimal 68B snapshot", testEmptyOptionalSummaries);
test("agent.session_codec: single-bit tamper CRC32 mismatch interception", testSingleBitTamperChecksumMismatch);
test("agent.session_codec: invalid magic, unsupported version & invalid flags", testHeaderErrorClassification);
test("agent.session_codec: truncated input & trailing garbage bytes rejection", testTruncatedAndTrailingBytes);
test("agent.session_codec: undersized vs exact-fit output buffer guard", testUndersizedBufferEncodeGuard);
