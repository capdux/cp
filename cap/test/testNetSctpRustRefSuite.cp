// Wave 29 R8: RFC 4960 / RFC 3758 / RFC 8832 SCTP DataChannel Rust Reference Hardening Suite
// Reference-hardened against webrtc-rs/sctp (MIT/Apache-2.0)

import { test, expect_eq } from "cap:test";
import {
    SCTP_CHUNK_HEARTBEAT_ACK,
    SCTP_CHUNK_COOKIE_ACK,
    SCTP_DATA_FLAG_E,
    SCTP_DATA_FLAG_B,
    SCTP_DATA_FLAG_U,
    SCTP_PPID_STRING,
    SCTP_PPID_BINARY,
    SctpDcError,
    simdCopyBytes,
    simdIsAllZero,
    tsnLt,
    tsnGt,
    tsnDiff,
    updateSctpCrc32cScalar,
    updateSctpCrc32c,
    computeSctpCrc32c,
    stampSctpPacketCrc32c,
    verifySctpPacketCrc32c,
    validateSctpPacket,
    SctpCommonHeader,
    SctpWireChunkIter,
    SctpPacketBuilder,
    SctpTlvParam,
    encodeSctpTlvParam,
    SctpDataPayload,
    encodeSctpDataChunk,
    decodeSctpDataChunk,
    GapAckBlock,
    encodeSctpSackChunk,
    decodeSctpSackChunk,
    decodeSctpHeartbeatChunk,
    decodeSctpAbortChunk,
    ForwardTsnStreamEntry,
    encodeSctpForwardTsnChunk,
    decodeSctpForwardTsnChunk,
    SctpTsnAcceptResult,
    SctpRecvTracker,
    SctpReassembleOutcome,
    SctpFragmentReassembler,
    DcepChannelType,
    DcepOpenMessage,
    encodeDcepOpen,
    decodeDcepOpen,
    encodeDcepAck,
    decodeDcepAck,
    SctpAssociationState,
    SctpAssociationConfig,
    SctpAssociation,
} from "../../lib/net/sctpDataChannel.cp";

fn prngNext(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn checkValidateErr(pkt: []const u8, expected: SctpDcError): bool {
    * = validateSctpPacket(pkt) catch |e| {
        return e == expected;
    };
    return false;
}

fn checkCookieEchoErr(srv: *SctpAssociation, pkt: []const u8, out_ack: []u8, expected: SctpDcError): bool {
    * = srv.onCookieEcho(pkt, out_ack) catch |e| {
        return e == expected;
    };
    return false;
}

fn checkSackDecodeErr(
    raw: []const u8,
    gaps: []GapAckBlock,
    dups: []u32,
    expected: SctpDcError,
): bool {
    * = decodeSctpSackChunk(raw, gaps, dups) catch |e| {
        return e == expected;
    };
    return false;
}

fn checkDcepOpenErr(raw: []const u8, expected: SctpDcError): bool {
    * = decodeDcepOpen(raw) catch |e| {
        return e == expected;
    };
    return false;
}

fn runCrc32cAndPacketStamp(): bool ! {
    expect_eq(computeSctpCrc32c("123456789"), 0xE3069283 as u32);
    expect_eq(computeSctpCrc32c(""), 0x00000000 as u32);

    let sample: [68]u8 = [0; 68];
    let k: usize = 0;
    while (k < 68) : (k += 1) {
        sample[k] = ((k * 37 + 11) & 0xFF) as u8;
    }
    let n: usize = 0;
    while (n <= 67) : (n += 1) {
        const fast = updateSctpCrc32c(0xFFFFFFFF, sample[0..n]) ^ 0xFFFFFFFF;
        const ref_crc = updateSctpCrc32cScalar(0xFFFFFFFF, sample[0..n]) ^ 0xFFFFFFFF;
        expect_eq(fast, ref_crc);
    }

    let pkt: [256]u8 = [0; 256];
    let b = SctpPacketBuilder.init(pkt[0..256], 5000, 5001, 0xCAFEBABE)?;
    const c1 = encodeSctpDataChunk(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
        tsn: 100,
        stream_id: 1,
        stream_seq: 0,
        ppid: SCTP_PPID_STRING,
        user_data: "hello",
    }, b.tailSlice())?;
    * = b.commitChunk(c1)?;
    const c2 = encodeSctpDataChunk(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
        tsn: 101,
        stream_id: 1,
        stream_seq: 1,
        ppid: SCTP_PPID_BINARY,
        user_data: "world!",
    }, b.tailSlice())?;
    * = b.commitChunk(c2)?;
    const pkt_len = b.finish()?;

    expect_eq(verifySctpPacketCrc32c(pkt[0..pkt_len])?, true);
    expect_eq(validateSctpPacket(pkt[0..pkt_len])?, 2 as usize);

    pkt[pkt_len - 2] ^= 0x01;
    expect_eq(verifySctpPacketCrc32c(pkt[0..pkt_len])?, false);
    return true;
}

fn runSimdCopyAndZero(): bool ! {
    let src: [67]u8 = [0; 67];
    let dst: [80]u8 = [0; 80];
    let i: usize = 0;
    while (i < 67) : (i += 1) {
        src[i] = ((i * 19 + 3) & 0xFF) as u8;
    }
    simdCopyBytes(dst[0..80], 5, src[0..67]);
    expect_eq(bytesEqual(dst[5..72], src[0..67]), true);

    let zeros: [48]u8 = [0; 48];
    expect_eq(simdIsAllZero(zeros[0..48]), true);
    expect_eq(simdIsAllZero(zeros[0..15]), true);
    expect_eq(simdIsAllZero(zeros[0..16]), true);

    let pos: usize = 0;
    while (pos < 48) : (pos += 1) {
        zeros[pos] = 0x80;
        expect_eq(simdIsAllZero(zeros[0..48]), false);
        zeros[pos] = 0;
    }
    expect_eq(simdIsAllZero(zeros[0..48]), true);
    return true;
}

fn runPacketValidationRules(): bool ! {
    let p_zero_port: [64]u8 = [0; 64];
    let b0 = SctpPacketBuilder.init(p_zero_port[0..64], 0, 5000, 0x1111)?;
    const len0 = b0.finish()?;
    expect_eq(checkValidateErr(p_zero_port[0..len0], SctpDcError.ZeroPort), true);

    let p_bundled: [128]u8 = [0; 128];
    let client = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5000,
        remote_port: 5001,
        local_tag: 0x12345678,
        initial_tsn: 100,
    })?;
    const init_len = client.buildInit(p_bundled[0..128])?;
    p_bundled[init_len] = SCTP_CHUNK_COOKIE_ACK;
    p_bundled[init_len + 1] = 0;
    p_bundled[init_len + 2] = 0;
    p_bundled[init_len + 3] = 4;
    * = stampSctpPacketCrc32c(p_bundled[0..(init_len + 4)])?;
    expect_eq(checkValidateErr(p_bundled[0..(init_len + 4)], SctpDcError.InitChunkBundled), true);

    p_bundled[4] = 0x01;
    * = stampSctpPacketCrc32c(p_bundled[0..init_len])?;
    expect_eq(checkValidateErr(p_bundled[0..init_len], SctpDcError.InvalidVerificationTag), true);
    return true;
}

fn runHandshakeHeartbeatAbortShutdown(): bool ! {
    let client = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5000,
        remote_port: 5001,
        local_tag: 0x10203040,
        initial_tsn: 1000,
        num_outbound_streams: 256,
        num_inbound_streams: 512,
    })?;
    let server = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5001,
        remote_port: 5000,
        local_tag: 0x50607080,
        initial_tsn: 2000,
        num_outbound_streams: 128,
        num_inbound_streams: 128,
        cookie_secret: 0x5A3C96E1,
    })?;

    let p1: [256]u8 = [0; 256];
    let p2: [256]u8 = [0; 256];
    let p3: [256]u8 = [0; 256];
    let p4: [256]u8 = [0; 256];

    const l1 = client.buildInit(p1[0..256])?;
    expect_eq(client.state == SctpAssociationState.cookie_wait, true);

    const l2 = server.onInit(p1[0..l1], p2[0..256])?;
    expect_eq(server.state == SctpAssociationState.closed, true);

    const l3 = client.onInitAck(p2[0..l2], p3[0..256])?;
    expect_eq(client.state == SctpAssociationState.cookie_echoed, true);

    let tampered: [256]u8 = [0; 256];
    simdCopyBytes(tampered[0..256], 0, p3[0..l3]);
    tampered[l3 - 1] ^= 0xFF;
    * = stampSctpPacketCrc32c(tampered[0..l3])?;
    expect_eq(checkCookieEchoErr(*server, tampered[0..l3], p4[0..256], SctpDcError.InvalidCookie), true);

    const l4 = server.onCookieEcho(p3[0..l3], p4[0..256])?;
    expect_eq(server.state == SctpAssociationState.established, true);
    expect_eq(client.negotiated_outbound_streams, 128 as u16);

    expect_eq(client.onCookieAck(p4[0..l4])?, true);
    expect_eq(client.state == SctpAssociationState.established, true);

    // HEARTBEAT / HEARTBEAT_ACK roundtrip
    let hb_pkt: [128]u8 = [0; 128];
    let hb_ack: [128]u8 = [0; 128];
    const hb_len = client.buildHeartbeatPacket("ping-42", hb_pkt[0..128])?;
    const hba_len = server.onHeartbeat(hb_pkt[0..hb_len], hb_ack[0..128])?;
    let it_hb = SctpWireChunkIter.fromPacket(hb_ack[0..hba_len])?;
    const hb_c = (it_hb.next() catch |e| return e) orelse return false;
    expect_eq(hb_c.chunk_type, SCTP_CHUNK_HEARTBEAT_ACK);
    const hb_info = decodeSctpHeartbeatChunk(hb_c.raw)?;
    expect_eq(bytesEqual(hb_info, "ping-42"), true);

    // Graceful 3-way shutdown
    let s1: [64]u8 = [0; 64];
    let s2: [64]u8 = [0; 64];
    let s3: [64]u8 = [0; 64];
    const sl1 = client.buildShutdownPacket(1999, s1[0..64])?;
    const sl2 = server.onShutdown(s1[0..sl1], s2[0..64])?;
    const sl3 = client.onShutdownAck(s2[0..sl2], s3[0..64])?;
    expect_eq(server.onShutdownComplete(s3[0..sl3])?, true);
    expect_eq(client.state == SctpAssociationState.closed, true);
    expect_eq(server.state == SctpAssociationState.closed, true);

    // ABORT with TLV error cause & T-bit verification
    let c2 = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5000,
        remote_port: 5001,
        local_tag: 0xAAAAAAAA,
        initial_tsn: 10,
    })?;
    let sv2 = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5001,
        remote_port: 5000,
        local_tag: 0xBBBBBBBB,
        initial_tsn: 20,
    })?;
    const k1 = c2.buildInit(p1[0..256])?;
    const k2 = sv2.onInit(p1[0..k1], p2[0..256])?;
    const k3 = c2.onInitAck(p2[0..k2], p3[0..256])?;
    const k4 = sv2.onCookieEcho(p3[0..k3], p4[0..256])?;
    * = c2.onCookieAck(p4[0..k4])?;

    let cause_buf: [16]u8 = [0; 16];
    const cause_len = encodeSctpTlvParam(SctpTlvParam{ param_type: 12, value: "bye" }, cause_buf[0..16])?;
    let abort_pkt: [128]u8 = [0; 128];
    const abort_len = c2.buildAbortPacket(false, cause_buf[0..cause_len], abort_pkt[0..128])?;
    const av = sv2.onAbort(abort_pkt[0..abort_len])?;
    expect_eq(av.t_bit, false);
    expect_eq(av.error_causes.len, cause_len);
    expect_eq(sv2.state == SctpAssociationState.closed, true);
    return true;
}

fn runFragmentReassembly(): bool ! {
    let storage: [256]u8 = [0; 256];
    let reasm = SctpFragmentReassembler.init(storage[0..256])?;

    const r1 = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B,
        tsn: 0xFFFFFFFE,
        stream_id: 3,
        stream_seq: 7,
        ppid: SCTP_PPID_STRING,
        user_data: "Hello, ",
    })?;
    expect_eq(r1 == SctpReassembleOutcome.in_progress, true);

    const r2 = reasm.pushFragment(SctpDataPayload{
        flags: 0,
        tsn: 0xFFFFFFFF,
        stream_id: 3,
        stream_seq: 7,
        ppid: SCTP_PPID_STRING,
        user_data: "SCTP ",
    })?;
    expect_eq(r2 == SctpReassembleOutcome.in_progress, true);

    const r3 = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_E,
        tsn: 0x00000000,
        stream_id: 3,
        stream_seq: 7,
        ppid: SCTP_PPID_STRING,
        user_data: "Wrap!",
    })?;
    expect_eq(r3 == SctpReassembleOutcome.complete, true);
    expect_eq(bytesEqual(reasm.message(), "Hello, SCTP Wrap!"), true);
    expect_eq(reasm.fragment_count, 3 as usize);

    // Unordered (U=1) 2-fragment message
    const uf1 = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_U,
        tsn: 10,
        stream_id: 5,
        stream_seq: 0,
        ppid: SCTP_PPID_BINARY,
        user_data: "PartA-",
    })?;
    expect_eq(uf1 == SctpReassembleOutcome.in_progress, true);

    const uf2 = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_E | SCTP_DATA_FLAG_U,
        tsn: 11,
        stream_id: 5,
        stream_seq: 99,
        ppid: SCTP_PPID_BINARY,
        user_data: "PartB",
    })?;
    expect_eq(uf2 == SctpReassembleOutcome.complete, true);
    expect_eq(bytesEqual(reasm.message(), "PartA-PartB"), true);
    return true;
}

fn runWraparoundSackAndForwardTsn(): bool ! {
    expect_eq(tsnLt(0xFFFFFFFE, 0x00000002), true);
    expect_eq(tsnGt(0x00000002, 0xFFFFFFFE), true);
    expect_eq(tsnDiff(0x00000002, 0xFFFFFFFE), 4 as i32);

    let bitmap: [4]u64 = [0; 4];
    let dups: [8]u32 = [0; 8];
    let tracker = SctpRecvTracker.init(0xFFFFFFFE, 65536, bitmap[0..4], dups[0..8])?;

    expect_eq((tracker.recordTsn(0xFFFFFFFE)?) == SctpTsnAcceptResult.in_order, true);
    expect_eq(tracker.cumulative_tsn_ack, 0xFFFFFFFE as u32);
    expect_eq((tracker.recordTsn(0x00000000)?) == SctpTsnAcceptResult.out_of_order, true);
    expect_eq((tracker.recordTsn(0x00000001)?) == SctpTsnAcceptResult.out_of_order, true);
    expect_eq((tracker.recordTsn(0x00000003)?) == SctpTsnAcceptResult.out_of_order, true);
    expect_eq((tracker.recordTsn(0x00000000)?) == SctpTsnAcceptResult.duplicate, true);

    let sack_buf: [128]u8 = [0; 128];
    let scratch_gaps: [8]GapAckBlock = [GapAckBlock{}; 8];
    const sack_len = tracker.buildSackChunk(sack_buf[0..128], scratch_gaps[0..8])?;

    let dec_gaps: [8]GapAckBlock = [GapAckBlock{}; 8];
    let dec_dups: [8]u32 = [0; 8];
    const sb = decodeSctpSackChunk(sack_buf[0..sack_len], dec_gaps[0..8], dec_dups[0..8])?;
    expect_eq(sb.cumulative_tsn_ack, 0xFFFFFFFE as u32);
    expect_eq(sb.num_gap_blocks, 2 as u16);
    expect_eq(dec_gaps[0].start, 2 as u16);
    expect_eq(dec_gaps[0].end, 3 as u16);
    expect_eq(dec_gaps[1].start, 5 as u16);
    expect_eq(dec_gaps[1].end, 5 as u16);
    expect_eq(sb.num_dup_tsns, 1 as u16);
    expect_eq(dec_dups[0], 0x00000000 as u32);

    let bad_sack: [64]u8 = [0; 64];
    let bad_gaps: [2]GapAckBlock = [
        GapAckBlock{ start: 2, end: 4 },
        GapAckBlock{ start: 4, end: 6 },
    ];
    const bs_len = encodeSctpSackChunk(10, 4096, bad_gaps[0..2], dec_dups[0..0], bad_sack[0..64])?;
    expect_eq(checkSackDecodeErr(bad_sack[0..bs_len], dec_gaps[0..8], dec_dups[0..8], SctpDcError.InvalidChunkLength), true);

    let fwd_buf: [64]u8 = [0; 64];
    let entries: [1]ForwardTsnStreamEntry = [ForwardTsnStreamEntry{ stream_id: 2, stream_seq: 5 }];
    const fwd_len = encodeSctpForwardTsnChunk(0xFFFFFFFF, entries[0..1], fwd_buf[0..64])?;
    let out_entries: [4]ForwardTsnStreamEntry = [ForwardTsnStreamEntry{}; 4];
    const fb = decodeSctpForwardTsnChunk(fwd_buf[0..fwd_len], out_entries[0..4])?;
    expect_eq(fb.new_cumulative_tsn, 0xFFFFFFFF as u32);
    expect_eq(fb.num_streams, 1 as usize);

    expect_eq(tracker.applyForwardTsn(fb.new_cumulative_tsn), 3 as u32);
    expect_eq(tracker.cumulative_tsn_ack, 0x00000001 as u32);
    return true;
}

fn runDcepAll6Types(): bool ! {
    const types: [6]DcepChannelType = [
        DcepChannelType.reliable,
        DcepChannelType.reliable_unordered,
        DcepChannelType.partial_reliable_rexmit,
        DcepChannelType.partial_reliable_rexmit_unordered,
        DcepChannelType.partial_reliable_timed,
        DcepChannelType.partial_reliable_timed_unordered,
    ];
    let buf: [128]u8 = [0; 128];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const msg = DcepOpenMessage{
            channel_type: types[i],
            priority: (256 + i * 64) as u16,
            reliability_parameter: (i * 10) as u32,
            label: "chat-channel",
            protocol: "json-v1",
        };
        const n = encodeDcepOpen(msg, buf[0..128])?;
        const decoded = decodeDcepOpen(buf[0..n])?;
        expect_eq(decoded.channel_type.toU8(), types[i].toU8());
        expect_eq(decoded.priority, msg.priority);
        expect_eq(decoded.reliability_parameter, msg.reliability_parameter);
        expect_eq(bytesEqual(decoded.label, "chat-channel"), true);
        expect_eq(bytesEqual(decoded.protocol, "json-v1"), true);
    }

    let ack_buf: [4]u8 = [0; 4];
    expect_eq(encodeDcepAck(ack_buf[0..4])?, 1 as usize);
    expect_eq(decodeDcepAck(ack_buf[0..1])?, true);
    expect_eq(checkDcepOpenErr(buf[0..10], SctpDcError.InvalidDcepMessage), true);
    return true;
}

fn fuzzOnePacket(
    pkt: []const u8,
    gaps: []GapAckBlock,
    dups: []u32,
    fwd_entries: []ForwardTsnStreamEntry,
    out_ack: []u8,
) {
    * = validateSctpPacket(pkt) catch 0;
    * = SctpCommonHeader.decode(pkt) catch SctpCommonHeader{};
    * = decodeDcepOpen(pkt) catch DcepOpenMessage{};
    * = decodeDcepAck(pkt) catch false;

    let srv = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5001,
        remote_port: 5000,
        local_tag: 0x33445566,
        initial_tsn: 500,
    }) catch {
        return;
    };
    * = srv.onInit(pkt, out_ack) catch 0;
    * = srv.onCookieEcho(pkt, out_ack) catch 0;

    let it = SctpWireChunkIter.fromPacket(pkt) catch {
        return;
    };
    let guard: usize = 0;
    while (guard < 32) : (guard += 1) {
        const opt_c = it.next() catch {
            return;
        };
        if (c := opt_c) {
            * = decodeSctpDataChunk(c.raw) catch SctpDataPayload{};
            * = decodeSctpSackChunk(c.raw, gaps, dups) catch {
                continue;
            };
            * = decodeSctpHeartbeatChunk(c.raw) catch "";
            * = decodeSctpAbortChunk(c.raw) catch {
                continue;
            };
            * = decodeSctpForwardTsnChunk(c.raw, fwd_entries) catch {
                continue;
            };
        } else {
            break;
        }
    }
}

fn runFuzz1280(): usize {
    let seed: u64 = 0x517CC1B727220A95;
    let pkt: [192]u8 = [0; 192];
    let gaps: [16]GapAckBlock = [GapAckBlock{}; 16];
    let dups: [16]u32 = [0; 16];
    let fwd_entries: [16]ForwardTsnStreamEntry = [ForwardTsnStreamEntry{}; 16];
    let out_ack: [256]u8 = [0; 256];
    let iters: usize = 0;
    let survived: usize = 0;

    while (iters < 1280) : (iters += 1) {
        const len: usize = (prngNext(*seed) % 180) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            pkt[j] = (prngNext(*seed) & 0xFF) as u8;
        }
        if (len >= 12 && (iters % 3) == 0) {
            * = stampSctpPacketCrc32c(pkt[0..len]) catch 0;
        }
        fuzzOnePacket(pkt[0..len], gaps[0..16], dups[0..16], fwd_entries[0..16], out_ack[0..256]);
        survived += 1;
    }
    return survived;
}

fn main() {
    test("rfc4960_crc32c_slicing4_vectors_and_packet_stamp", () => {
        expect_eq(runCrc32cAndPacketStamp() catch false, true);
    });
    test("simd16_copy_and_zero_check", () => {
        expect_eq(runSimdCopyAndZero() catch false, true);
    });
    test("webrtc_rs_packet_validation_rules", () => {
        expect_eq(runPacketValidationRules() catch false, true);
    });
    test("full_4way_handshake_heartbeat_abort_and_shutdown", () => {
        expect_eq(runHandshakeHeartbeatAbortShutdown() catch false, true);
    });
    test("ordered_and_unordered_fragment_reassembly", () => {
        expect_eq(runFragmentReassembly() catch false, true);
    });
    test("tsn_wraparound_sack_gaps_duplicates_and_forward_tsn", () => {
        expect_eq(runWraparoundSackAndForwardTsn() catch false, true);
    });
    test("rfc8832_dcep_all_6_channel_types_and_bounds", () => {
        expect_eq(runDcepAll6Types() catch false, true);
    });
    test("fuzz_1280_malformed_sctp_packets_and_chunks_no_panic", () => {
        expect_eq(runFuzz1280(), 1280 as usize);
    });
}
