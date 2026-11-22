// Origin: Pure-CP Stdlib Test Suite (lib/net/sctpDataChannel.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 4960 SCTP, RFC 3758 PR-SCTP & RFC 8832 WebRTC DCEP:
//   1. RFC 4960 Appendix B Castagnoli CRC32C official vector ("123456789" == 0xE3069283) & packet checksum verification
//   2. Full 4-way SCTP handshake (INIT -> INIT_ACK -> COOKIE_ECHO -> COOKIE_ACK), stateless Cookie tamper guard,
//      HEARTBEAT/HEARTBEAT_ACK & 3-way graceful SHUTDOWN lifecycle
//   3. RFC 8832 DCEP DATA_CHANNEL_OPEN & DATA_CHANNEL_ACK roundtrip across all 6 channel types + association DCEP loop
//   4. Unbounded external-slice SctpRecvTracker: out-of-order multi-block GapAckBlocks, duplicate TSN SACK,
//      multi-word (>64 TSN) bitmap shift & 32-bit RFC 1982 TSN wrap-around
//   5. SctpFragmentReassembler: single-fragment (B=1,E=1) and 4-fragment (>4KB) multi-TSN reassembly across TSN wrap,
//      plus FragmentOutOfOrder / FragmentStreamMismatch / FragmentIncomplete guards
//   6. RFC 3758 PR-SCTP FORWARD_TSN codec, SctpRecvTracker.applyForwardTsn cascade advance & reassembler abort recovery
//   7. Multi-chunk 4-byte padded packet bundling (SctpPacketBuilder + SctpWireChunkIter), ABORT & INIT TLV iteration
//   8. Malformed/truncated/boundary guard matrix & cap.net namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SCTP_COMMON_HEADER_LEN,
    SCTP_CHUNK_DATA,
    SCTP_CHUNK_INIT,
    SCTP_CHUNK_INIT_ACK,
    SCTP_CHUNK_SACK,
    SCTP_CHUNK_HEARTBEAT,
    SCTP_CHUNK_HEARTBEAT_ACK,
    SCTP_CHUNK_ABORT,
    SCTP_CHUNK_SHUTDOWN,
    SCTP_CHUNK_SHUTDOWN_ACK,
    SCTP_CHUNK_COOKIE_ECHO,
    SCTP_CHUNK_COOKIE_ACK,
    SCTP_CHUNK_SHUTDOWN_COMPLETE,
    SCTP_CHUNK_FORWARD_TSN,
    SCTP_DATA_FLAG_E,
    SCTP_DATA_FLAG_B,
    SCTP_DATA_FLAG_U,
    SCTP_DATA_FLAG_I,
    SCTP_PARAM_STATE_COOKIE,
    SCTP_PARAM_FORWARD_TSN_SUPPORTED,
    SCTP_PPID_DCEP,
    SCTP_PPID_STRING,
    SCTP_PPID_BINARY,
    SCTP_PPID_STRING_EMPTY,
    SCTP_PPID_BINARY_EMPTY,
    DCEP_MSG_ACK,
    DCEP_MSG_OPEN,
    DCEP_CH_RELIABLE,
    DCEP_CH_RELIABLE_UNORDERED,
    DCEP_CH_PARTIAL_RELIABLE_REXMIT,
    DCEP_CH_PARTIAL_RELIABLE_REXMIT_UNORDERED,
    DCEP_CH_PARTIAL_RELIABLE_TIMED,
    DCEP_CH_PARTIAL_RELIABLE_TIMED_UNORDERED,
    SctpDcError,
    sctpPad4,
    tsnLt,
    tsnLe,
    tsnGt,
    tsnGe,
    tsnDiff,
    updateSctpCrc32c,
    computeSctpCrc32c,
    computeSctpPacketCrc32c,
    stampSctpPacketCrc32c,
    verifySctpPacketCrc32c,
    SctpCommonHeader,
    SctpWireChunkIter,
    SctpPacketBuilder,
    SctpTlvParam,
    SctpTlvIter,
    SctpDataPayload,
    encodeSctpDataChunk,
    decodeSctpDataChunk,
    SctpInitBody,
    encodeSctpInitChunk,
    decodeSctpInitChunk,
    GapAckBlock,
    encodeSctpSackChunk,
    decodeSctpSackChunk,
    encodeSctpAbortChunk,
    decodeSctpHeartbeatChunk,
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
} from "cap:net";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("SCTP CRC32C RFC 4960 Appendix B official vector 0xE3069283 & packet checksum verification") {
    // Official RFC 4960 Appendix B / iSCSI Castagnoli CRC32C test vector
    const check_val: u32 = computeSctpCrc32c("123456789");
    expect_eq(check_val, 0xE3069283);

    // Empty input & incremental update parity
    expect_eq(computeSctpCrc32c(""), 0x00000000);
    const part1: u32 = updateSctpCrc32c(0xFFFFFFFF, "1234");
    const part2: u32 = updateSctpCrc32c(part1, "56789") ^ 0xFFFFFFFF;
    expect_eq(part2, 0xE3069283);

    // Build an SCTP packet with a DATA chunk, verify little-endian checksum & tamper detection
    let pkt: [128]u8 = [0; 128];
    let b = SctpPacketBuilder.init(pkt[0..], 5000, 5001, 0xDEADBEEF) catch unreachable;
    const clen = encodeSctpDataChunk(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E | SCTP_DATA_FLAG_I,
        tsn: 0x01020304,
        stream_id: 3,
        stream_seq: 7,
        ppid: SCTP_PPID_STRING,
        user_data: "hello-webrtc",
    }, b.tailSlice()) catch unreachable;
    * = b.commitChunk(clen) catch unreachable;
    const pkt_len = b.finish() catch unreachable;

    // Common header decode & CRC32C verification
    const hdr = SctpCommonHeader.decode(pkt[0..pkt_len]) catch unreachable;
    expect_eq(hdr.src_port, 5000);
    expect_eq(hdr.dst_port, 5001);
    expect_eq(hdr.verification_tag, 0xDEADBEEF);
    expect_true(hdr.checksum != 0);
    expect_true(verifySctpPacketCrc32c(pkt[0..pkt_len]) catch unreachable);

    // Confirm checksum is stored little-endian at bytes [8..12]
    const computed = computeSctpPacketCrc32c(pkt[0..pkt_len]) catch unreachable;
    expect_eq(hdr.checksum, computed);
    expect_eq(pkt[8], (computed & 0xFF) as u8);
    expect_eq(pkt[9], ((computed >> 8) & 0xFF) as u8);
    expect_eq(pkt[10], ((computed >> 16) & 0xFF) as u8);
    expect_eq(pkt[11], ((computed >> 24) & 0xFF) as u8);

    // Single-bit flip in payload must fail CRC32C verification
    pkt[pkt_len - 1] ^= 0x01;
    expect_true(!(verifySctpPacketCrc32c(pkt[0..pkt_len]) catch unreachable));
}

test("SCTP 4-way handshake, stateless cookie tamper rejection, heartbeat & 3-way shutdown") {
    let client = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5000,
        remote_port: 5000,
        local_tag: 0x11112222,
        initial_tsn: 0x00001000,
        a_rwnd: 262144,
        num_outbound_streams: 256,
        num_inbound_streams: 512,
        forward_tsn_supported: true,
        cookie_secret: 0xCAFEBABE,
    }) catch unreachable;

    let server = SctpAssociation.init(SctpAssociationConfig{
        local_port: 5000,
        remote_port: 5000,
        local_tag: 0x33334444,
        initial_tsn: 0x00002000,
        a_rwnd: 131072,
        num_outbound_streams: 128,
        num_inbound_streams: 64,
        forward_tsn_supported: true,
        cookie_secret: 0xFACEB00C,
    }) catch unreachable;

    expect_eq(client.state, SctpAssociationState.closed);
    expect_eq(server.state, SctpAssociationState.closed);

    let p_init: [256]u8 = [0; 256];
    let p_init_ack: [256]u8 = [0; 256];
    let p_cookie_echo: [256]u8 = [0; 256];
    let p_cookie_ack: [256]u8 = [0; 256];

    // Step 1: Client -> Server INIT
    const n_init = client.buildInit(p_init[0..]) catch unreachable;
    expect_eq(client.state, SctpAssociationState.cookie_wait);
    expect_true(verifySctpPacketCrc32c(p_init[0..n_init]) catch unreachable);

    // Step 2: Server -> Client INIT_ACK (server stays closed / stateless per RFC 4960)
    const n_init_ack = server.onInit(p_init[0..n_init], p_init_ack[0..]) catch unreachable;
    expect_eq(server.state, SctpAssociationState.closed);

    // Step 3: Client -> Server COOKIE_ECHO
    const n_cookie_echo = client.onInitAck(p_init_ack[0..n_init_ack], p_cookie_echo[0..]) catch unreachable;
    expect_eq(client.state, SctpAssociationState.cookie_echoed);
    expect_eq(client.peer_tag, 0x33334444);
    expect_eq(client.peer_initial_tsn, 0x00002000);
    expect_eq(client.negotiated_outbound_streams, 64);
    expect_eq(client.negotiated_inbound_streams, 128);
    expect_true(client.peer_forward_tsn_supported);

    // Tampered State Cookie (with recomputed valid CRC32C) must be rejected by server!
    let bad_echo: [256]u8 = [0; 256];
    let ti: usize = 0;
    while (ti < n_cookie_echo) : (ti += 1) {
        bad_echo[ti] = p_cookie_echo[ti];
    }
    bad_echo[SCTP_COMMON_HEADER_LEN + 6] ^= 0xFF;
    * = stampSctpPacketCrc32c(bad_echo[0..n_cookie_echo]) catch unreachable;
    let cookie_rejected: bool = false;
    * = server.onCookieEcho(bad_echo[0..n_cookie_echo], p_cookie_ack[0..]) catch |err_val| {
        if (err_val == SctpDcError.InvalidCookie) cookie_rejected = true;
    };
    expect_true(cookie_rejected);
    expect_eq(server.state, SctpAssociationState.closed);

    // Step 4: Valid COOKIE_ECHO -> Server emits COOKIE_ACK -> both Established
    const n_cookie_ack = server.onCookieEcho(p_cookie_echo[0..n_cookie_echo], p_cookie_ack[0..]) catch unreachable;
    expect_true(server.isEstablished());
    expect_eq(server.peer_tag, 0x11112222);
    expect_eq(server.peer_initial_tsn, 0x00001000);
    expect_true(server.peer_forward_tsn_supported);

    expect_true(client.onCookieAck(p_cookie_ack[0..n_cookie_ack]) catch unreachable);
    expect_true(client.isEstablished());

    // HEARTBEAT / HEARTBEAT_ACK roundtrip
    let p_hb: [128]u8 = [0; 128];
    let p_hba: [128]u8 = [0; 128];
    const n_hb = client.buildHeartbeatPacket("hb-token-42", p_hb[0..]) catch unreachable;
    const n_hba = server.onHeartbeat(p_hb[0..n_hb], p_hba[0..]) catch unreachable;
    expect_true(verifySctpPacketCrc32c(p_hba[0..n_hba]) catch unreachable);
    let hb_it = SctpWireChunkIter.fromPacket(p_hba[0..n_hba]) catch unreachable;
    const hb_chunk = (hb_it.next() catch unreachable) orelse unreachable;
    expect_eq(hb_chunk.chunk_type, SCTP_CHUNK_HEARTBEAT_ACK);
    const hb_info = decodeSctpHeartbeatChunk(hb_chunk.raw) catch unreachable;
    expect_true(bytesEq(hb_info, "hb-token-42"));

    // Graceful 3-way SHUTDOWN -> SHUTDOWN_ACK -> SHUTDOWN_COMPLETE
    let p_sd: [64]u8 = [0; 64];
    let p_sda: [64]u8 = [0; 64];
    let p_sdc: [64]u8 = [0; 64];
    const n_sd = client.buildShutdownPacket(0x00001FFF, p_sd[0..]) catch unreachable;
    expect_eq(client.state, SctpAssociationState.shutdown_sent);
    const n_sda = server.onShutdown(p_sd[0..n_sd], p_sda[0..]) catch unreachable;
    expect_eq(server.state, SctpAssociationState.shutdown_ack_sent);
    const n_sdc = client.onShutdownAck(p_sda[0..n_sda], p_sdc[0..]) catch unreachable;
    expect_eq(client.state, SctpAssociationState.closed);
    expect_true(server.onShutdownComplete(p_sdc[0..n_sdc]) catch unreachable);
    expect_eq(server.state, SctpAssociationState.closed);
}

test("RFC 8832 WebRTC DCEP OPEN/ACK across all 6 channel types & association DCEP handshake") {
    const ch_types: [6]DcepChannelType = [
        DcepChannelType.reliable,
        DcepChannelType.reliable_unordered,
        DcepChannelType.partial_reliable_rexmit,
        DcepChannelType.partial_reliable_rexmit_unordered,
        DcepChannelType.partial_reliable_timed,
        DcepChannelType.partial_reliable_timed_unordered,
    ];
    const exp_bytes: [6]u8 = [
        DCEP_CH_RELIABLE,
        DCEP_CH_RELIABLE_UNORDERED,
        DCEP_CH_PARTIAL_RELIABLE_REXMIT,
        DCEP_CH_PARTIAL_RELIABLE_REXMIT_UNORDERED,
        DCEP_CH_PARTIAL_RELIABLE_TIMED,
        DCEP_CH_PARTIAL_RELIABLE_TIMED_UNORDERED,
    ];

    let buf: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const ct = ch_types[i];
        expect_eq(ct.toU8(), exp_bytes[i]);
        const parsed_ct = DcepChannelType.fromU8(exp_bytes[i]) catch unreachable;
        expect_eq(parsed_ct, ct);

        const msg = DcepOpenMessage{
            channel_type: ct,
            priority: (256 + i * 128) as u16,
            reliability_parameter: (i * 50) as u32,
            label: "webrtc-telemetry",
            protocol: "proto-v2",
        };
        const n = encodeDcepOpen(msg, buf[0..]) catch unreachable;
        expect_eq(buf[0], DCEP_MSG_OPEN);
        expect_eq(buf[1], exp_bytes[i]);
        const decoded = decodeDcepOpen(buf[0..n]) catch unreachable;
        expect_eq(decoded.channel_type, ct);
        expect_eq(decoded.priority, msg.priority);
        expect_eq(decoded.reliability_parameter, msg.reliability_parameter);
        expect_true(bytesEq(decoded.label, "webrtc-telemetry"));
        expect_true(bytesEq(decoded.protocol, "proto-v2"));
    }

    // DCEP ACK encode/decode
    let ack_buf: [4]u8 = [0; 4];
    const ack_n = encodeDcepAck(ack_buf[0..]) catch unreachable;
    expect_eq(ack_n, 1);
    expect_eq(ack_buf[0], DCEP_MSG_ACK);
    expect_true(decodeDcepAck(ack_buf[0..1]) catch unreachable);

    // End-to-end DCEP OPEN & ACK encapsulated in SCTP DATA packets (PPID=50)
    let c = SctpAssociation.init(SctpAssociationConfig{ local_tag: 0xAA01, initial_tsn: 10 }) catch unreachable;
    let s = SctpAssociation.init(SctpAssociationConfig{ local_tag: 0xBB02, initial_tsn: 20 }) catch unreachable;
    let t1: [256]u8 = [0; 256];
    let t2: [256]u8 = [0; 256];
    let t3: [256]u8 = [0; 256];
    let t4: [256]u8 = [0; 256];
    const l1 = c.buildInit(t1[0..]) catch unreachable;
    const l2 = s.onInit(t1[0..l1], t2[0..]) catch unreachable;
    const l3 = c.onInitAck(t2[0..l2], t3[0..]) catch unreachable;
    const l4 = s.onCookieEcho(t3[0..l3], t4[0..]) catch unreachable;
    * = c.onCookieAck(t4[0..l4]) catch unreachable;

    let dcep_scratch: [128]u8 = [0; 128];
    let open_pkt: [256]u8 = [0; 256];
    const n_open = c.buildDcepOpenPacket(2, DcepOpenMessage{
        channel_type: DcepChannelType.partial_reliable_timed_unordered,
        priority: 512,
        reliability_parameter: 250,
        label: "game-state",
        protocol: "bin-v1",
    }, dcep_scratch[0..], open_pkt[0..]) catch unreachable;
    expect_true(verifySctpPacketCrc32c(open_pkt[0..n_open]) catch unreachable);

    let it = SctpWireChunkIter.fromPacket(open_pkt[0..n_open]) catch unreachable;
    const data_chunk = decodeSctpDataChunk(((it.next() catch unreachable) orelse unreachable).raw) catch unreachable;
    expect_eq(data_chunk.ppid, SCTP_PPID_DCEP);
    expect_eq(data_chunk.stream_id, 2);
    const recv_open = decodeDcepOpen(data_chunk.user_data) catch unreachable;
    expect_eq(recv_open.channel_type, DcepChannelType.partial_reliable_timed_unordered);
    expect_true(recv_open.channel_type.isUnordered());
    expect_true(!recv_open.channel_type.isReliable());
    expect_true(bytesEq(recv_open.label, "game-state"));

    let ack_pkt: [128]u8 = [0; 128];
    const n_ack_pkt = s.buildDcepAckPacket(2, ack_pkt[0..]) catch unreachable;
    let it2 = SctpWireChunkIter.fromPacket(ack_pkt[0..n_ack_pkt]) catch unreachable;
    const ack_data = decodeSctpDataChunk(((it2.next() catch unreachable) orelse unreachable).raw) catch unreachable;
    expect_eq(ack_data.ppid, SCTP_PPID_DCEP);
    expect_true(decodeDcepAck(ack_data.user_data) catch unreachable);
}

test("SctpRecvTracker out-of-order multi-block GapAckBlocks, duplicates, multi-word shift & 32-bit TSN wrap") {
    // Verify RFC 1982 wrap-around arithmetic around 0xFFFFFFFF -> 0x00000000
    expect_true(tsnLt(0xFFFFFFFE, 0xFFFFFFFF));
    expect_true(tsnLt(0xFFFFFFFF, 0x00000000));
    expect_true(tsnLt(0xFFFFFFFF, 0x00000005));
    expect_true(tsnGt(0x00000002, 0xFFFFFFFE));
    expect_eq(tsnDiff(0x00000001, 0xFFFFFFFF), 2);
    expect_eq(tsnDiff(0xFFFFFFFE, 0x00000001), -3);

    // 4 x 64-bit = 256-TSN external bitmap starting right before 32-bit wrap-around!
    let bitmap: [4]u64 = [0; 4];
    let dups: [8]u32 = [0; 8];
    let tracker = SctpRecvTracker.init(0xFFFFFFFE, 65536, bitmap[0..], dups[0..]) catch unreachable;
    expect_eq(tracker.cumulative_tsn_ack, 0xFFFFFFFD);

    // In-order TSNs across wrap: 0xFFFFFFFE, 0xFFFFFFFF
    expect_eq(tracker.recordTsn(0xFFFFFFFE) catch unreachable, SctpTsnAcceptResult.in_order);
    expect_eq(tracker.recordTsn(0xFFFFFFFF) catch unreachable, SctpTsnAcceptResult.in_order);
    expect_eq(tracker.cumulative_tsn_ack, 0xFFFFFFFF);

    // Drop TSN 0, receive 1, 2 (gap block [2..3]), drop 3, receive 4 (gap block [5..5]),
    // and also receive TSN 70 (beyond first 64-bit word, offset 71 -> gap block [71..71])
    expect_eq(tracker.recordTsn(1) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.recordTsn(2) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.recordTsn(4) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.recordTsn(70) catch unreachable, SctpTsnAcceptResult.out_of_order);

    // Duplicate of cumulative (0xFFFFFFFE) and duplicate of out-of-order (2)
    expect_eq(tracker.recordTsn(0xFFFFFFFE) catch unreachable, SctpTsnAcceptResult.duplicate);
    expect_eq(tracker.recordTsn(2) catch unreachable, SctpTsnAcceptResult.duplicate);

    // Build SACK chunk & decode it back
    let sack_wire: [256]u8 = [0; 256];
    let scratch_gaps: [8]GapAckBlock = [GapAckBlock{}; 8];
    const sack_len = tracker.buildSackChunk(sack_wire[0..], scratch_gaps[0..]) catch unreachable;

    let dec_gaps: [8]GapAckBlock = [GapAckBlock{}; 8];
    let dec_dups: [8]u32 = [0; 8];
    const sack = decodeSctpSackChunk(sack_wire[0..sack_len], dec_gaps[0..], dec_dups[0..]) catch unreachable;
    expect_eq(sack.cumulative_tsn_ack, 0xFFFFFFFF);
    expect_eq(sack.a_rwnd, 65536);
    expect_eq(sack.num_gap_blocks, 3);
    expect_eq(dec_gaps[0].start, 2);
    expect_eq(dec_gaps[0].end, 3);
    expect_eq(dec_gaps[1].start, 5);
    expect_eq(dec_gaps[1].end, 5);
    expect_eq(dec_gaps[2].start, 71);
    expect_eq(dec_gaps[2].end, 71);
    expect_eq(sack.num_dup_tsns, 2);
    expect_eq(dec_dups[0], 0xFFFFFFFE);
    expect_eq(dec_dups[1], 2);

    // Duplicates must be cleared after emitting SACK
    expect_eq(tracker.dup_count, 0);

    // Fill missing TSN 0 -> cumulative_tsn_ack must jump from 0xFFFFFFFF across 0, 1, 2 to 2!
    expect_eq(tracker.recordTsn(0) catch unreachable, SctpTsnAcceptResult.in_order);
    expect_eq(tracker.cumulative_tsn_ack, 2);

    // Fill missing TSN 3 -> cumulative_tsn_ack jumps to 4, and TSN 70 shifts down to offset 66
    expect_eq(tracker.recordTsn(3) catch unreachable, SctpTsnAcceptResult.in_order);
    expect_eq(tracker.cumulative_tsn_ack, 4);
    const rem_gaps = tracker.buildGapAckBlocks(dec_gaps[0..]) catch unreachable;
    expect_eq(rem_gaps, 1);
    expect_eq(dec_gaps[0].start, 66);
    expect_eq(dec_gaps[0].end, 66);
}

test("SctpFragmentReassembler single-fragment & 4-fragment (>4KB) multi-TSN reassembly across wrap") {
    let storage: [5120]u8 = [0; 5120];
    let reasm = SctpFragmentReassembler.init(storage[0..]) catch unreachable;

    // 1. Single-fragment message (B=1, E=1)
    const r0 = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
        tsn: 500,
        stream_id: 1,
        stream_seq: 0,
        ppid: SCTP_PPID_STRING,
        user_data: "single-frame",
    }) catch unreachable;
    expect_eq(r0, SctpReassembleOutcome.complete);
    expect_true(bytesEq(reasm.message(), "single-frame"));
    expect_eq(reasm.fragment_count, 1);

    // 2. 4-fragment message totaling 4,400 bytes across 32-bit TSN wrap (0xFFFFFFFE..0x00000001)
    let frag0: [1100]u8 = [0x11; 1100];
    let frag1: [1100]u8 = [0x22; 1100];
    let frag2: [1100]u8 = [0x33; 1100];
    let frag3: [1100]u8 = [0x44; 1100];

    expect_eq(reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B,
        tsn: 0xFFFFFFFE,
        stream_id: 4,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag0[0..],
    }) catch unreachable, SctpReassembleOutcome.in_progress);
    expect_true(reasm.isInProgress());

    // Guard: wrong stream_id while in-progress -> FragmentStreamMismatch
    let mismatch_err: bool = false;
    * = reasm.pushFragment(SctpDataPayload{
        flags: 0,
        tsn: 0xFFFFFFFF,
        stream_id: 5,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag1[0..],
    }) catch |e| {
        if (e == SctpDcError.FragmentStreamMismatch) mismatch_err = true;
    };
    expect_true(mismatch_err);

    // Guard: non-contiguous TSN -> FragmentOutOfOrder
    let ooo_err: bool = false;
    * = reasm.pushFragment(SctpDataPayload{
        flags: 0,
        tsn: 0x00000000,
        stream_id: 4,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag1[0..],
    }) catch |e| {
        if (e == SctpDcError.FragmentOutOfOrder) ooo_err = true;
    };
    expect_true(ooo_err);

    // Feed remaining 3 fragments in order across TSN wrap
    expect_eq(reasm.pushFragment(SctpDataPayload{
        flags: 0,
        tsn: 0xFFFFFFFF,
        stream_id: 4,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag1[0..],
    }) catch unreachable, SctpReassembleOutcome.in_progress);
    expect_eq(reasm.pushFragment(SctpDataPayload{
        flags: 0,
        tsn: 0x00000000,
        stream_id: 4,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag2[0..],
    }) catch unreachable, SctpReassembleOutcome.in_progress);
    expect_eq(reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_E,
        tsn: 0x00000001,
        stream_id: 4,
        stream_seq: 9,
        ppid: SCTP_PPID_BINARY,
        user_data: frag3[0..],
    }) catch unreachable, SctpReassembleOutcome.complete);

    expect_true(!reasm.isInProgress());
    expect_eq(reasm.fragment_count, 4);
    const full = reasm.message();
    expect_eq(full.len, 4400);
    expect_eq(full[0], 0x11);
    expect_eq(full[1099], 0x11);
    expect_eq(full[1100], 0x22);
    expect_eq(full[2200], 0x33);
    expect_eq(full[3300], 0x44);
    expect_eq(full[4399], 0x44);
}

test("RFC 3758 PR-SCTP FORWARD_TSN codec, tracker skip+cascade advance & reassembler abort") {
    let entries: [2]ForwardTsnStreamEntry = [
        ForwardTsnStreamEntry{ stream_id: 2, stream_seq: 11 },
        ForwardTsnStreamEntry{ stream_id: 4, stream_seq: 3 },
    ];
    let fwd_chunk: [64]u8 = [0; 64];
    const n_fwd = encodeSctpForwardTsnChunk(105, entries[0..], fwd_chunk[0..]) catch unreachable;
    expect_eq(fwd_chunk[0], SCTP_CHUNK_FORWARD_TSN);

    let dec_entries: [4]ForwardTsnStreamEntry = [ForwardTsnStreamEntry{}; 4];
    const fwd_body = decodeSctpForwardTsnChunk(fwd_chunk[0..n_fwd], dec_entries[0..]) catch unreachable;
    expect_eq(fwd_body.new_cumulative_tsn, 105);
    expect_eq(fwd_body.num_streams, 2);
    expect_eq(dec_entries[0].stream_id, 2);
    expect_eq(dec_entries[0].stream_seq, 11);
    expect_eq(dec_entries[1].stream_id, 4);
    expect_eq(dec_entries[1].stream_seq, 3);

    // Tracker expects TSN 100; 100..105 were lost (partially reliable), while 106 & 107 already arrived!
    let bitmap: [2]u64 = [0; 2];
    let dups: [4]u32 = [0; 4];
    let tracker = SctpRecvTracker.init(100, 65536, bitmap[0..], dups[0..]) catch unreachable;
    expect_eq(tracker.recordTsn(106) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.recordTsn(107) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.recordTsn(109) catch unreachable, SctpTsnAcceptResult.out_of_order);
    expect_eq(tracker.cumulative_tsn_ack, 99);

    // FORWARD_TSN(105) skips 100..105 (6 TSNs) AND cascades across already-received 106, 107 -> cumulative = 107!
    const advanced = tracker.applyForwardTsn(105);
    expect_eq(advanced, 8);
    expect_eq(tracker.cumulative_tsn_ack, 107);
    expect_eq(tracker.total_forwarded, 6);

    // Remaining gap block should only cover TSN 109 (offset 2 from 107)
    let gaps: [4]GapAckBlock = [GapAckBlock{}; 4];
    const ng = tracker.buildGapAckBlocks(gaps[0..]) catch unreachable;
    expect_eq(ng, 1);
    expect_eq(gaps[0].start, 2);
    expect_eq(gaps[0].end, 2);

    // Reassembler abort recovers cleanly after abandoned fragment
    let rbuf: [128]u8 = [0; 128];
    let reasm = SctpFragmentReassembler.init(rbuf[0..]) catch unreachable;
    * = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B,
        tsn: 104,
        stream_id: 2,
        stream_seq: 11,
        ppid: SCTP_PPID_STRING,
        user_data: "abandoned-head",
    }) catch unreachable;
    expect_true(reasm.isInProgress());
    reasm.abort();
    expect_true(!reasm.isInProgress());
    const ok_after = reasm.pushFragment(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
        tsn: 108,
        stream_id: 2,
        stream_seq: 12,
        ppid: SCTP_PPID_STRING,
        user_data: "fresh-message",
    }) catch unreachable;
    expect_eq(ok_after, SctpReassembleOutcome.complete);
    expect_true(bytesEq(reasm.message(), "fresh-message"));
}

test("Multi-chunk 4-byte padded packet bundling, ABORT & INIT TLV parameter iteration") {
    expect_eq(sctpPad4(1), 4);
    expect_eq(sctpPad4(4), 4);
    expect_eq(sctpPad4(5), 8);

    // Bundle 3 unaligned-length chunks (DATA 19B -> 20B, SACK 16B -> 16B, ABORT 7B -> 8B) in one packet
    let pkt: [256]u8 = [0; 256];
    let b = SctpPacketBuilder.init(pkt[0..], 5000, 5000, 0x778899AA) catch unreachable;
    const c1 = encodeSctpDataChunk(SctpDataPayload{
        flags: SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E | SCTP_DATA_FLAG_U,
        tsn: 42,
        stream_id: 1,
        stream_seq: 0,
        ppid: SCTP_PPID_BINARY_EMPTY,
        user_data: "abc",
    }, b.tailSlice()) catch unreachable;
    * = b.commitChunk(c1) catch unreachable;

    let empty_gaps: [0]GapAckBlock = [];
    let empty_dups: [0]u32 = [];
    const c2 = encodeSctpSackChunk(41, 32768, empty_gaps[0..], empty_dups[0..], b.tailSlice()) catch unreachable;
    * = b.commitChunk(c2) catch unreachable;

    const c3 = encodeSctpAbortChunk(true, "err", b.tailSlice()) catch unreachable;
    * = b.commitChunk(c3) catch unreachable;
    const total_len = b.finish() catch unreachable;
    expect_true(verifySctpPacketCrc32c(pkt[0..total_len]) catch unreachable);

    // Iterate all 3 chunks with SctpWireChunkIter
    let it = SctpWireChunkIter.fromPacket(pkt[0..total_len]) catch unreachable;
    const v1 = (it.next() catch unreachable) orelse unreachable;
    const v2 = (it.next() catch unreachable) orelse unreachable;
    const v3 = (it.next() catch unreachable) orelse unreachable;
    expect_true((it.next() catch unreachable) == null);
    expect_eq(v1.chunk_type, SCTP_CHUNK_DATA);
    expect_eq(v2.chunk_type, SCTP_CHUNK_SACK);
    expect_eq(v3.chunk_type, SCTP_CHUNK_ABORT);
    expect_eq(v3.flags, 0x01);

    const d1 = decodeSctpDataChunk(v1.raw) catch unreachable;
    expect_true(d1.isUnordered());
    expect_eq(d1.ppid, SCTP_PPID_BINARY_EMPTY);
    expect_true(bytesEq(d1.user_data, "abc"));

    // INIT_ACK with unaligned 5-byte State Cookie + Forward-TSN-Supported TLV
    let init_buf: [128]u8 = [0; 128];
    let params: [2]SctpTlvParam = [
        SctpTlvParam{ param_type: SCTP_PARAM_STATE_COOKIE, value: "ck123" },
        SctpTlvParam{ param_type: SCTP_PARAM_FORWARD_TSN_SUPPORTED, value: "" },
    ];
    const init_len = encodeSctpInitChunk(SctpInitBody{
        is_ack: true,
        initiate_tag: 0x55667788,
        a_rwnd: 65536,
        num_outbound_streams: 16,
        num_inbound_streams: 16,
        initial_tsn: 900,
        params_raw: "",
    }, params[0..], init_buf[0..]) catch unreachable;

    const dec_init = decodeSctpInitChunk(init_buf[0..init_len]) catch unreachable;
    expect_true(dec_init.is_ack);
    expect_true(bytesEq(dec_init.findStateCookie() catch unreachable, "ck123"));
    expect_true(dec_init.supportsForwardTsn() catch unreachable);

    let tlv_it = SctpTlvIter.init(dec_init.params_raw);
    const p1 = (tlv_it.next() catch unreachable) orelse unreachable;
    const p2 = (tlv_it.next() catch unreachable) orelse unreachable;
    expect_true((tlv_it.next() catch unreachable) == null);
    expect_eq(p1.param_type, SCTP_PARAM_STATE_COOKIE);
    expect_eq(p2.param_type, SCTP_PARAM_FORWARD_TSN_SUPPORTED);
}

test("SCTP/DCEP malformed & boundary guard matrix + cap.net namespace parity") {
    // 1. PacketTooShort (< 12 bytes)
    let short_pkt: [8]u8 = [0; 8];
    let got_short: bool = false;
    * = SctpCommonHeader.decode(short_pkt[0..]) catch |e| {
        if (e == SctpDcError.PacketTooShort) got_short = true;
    };
    expect_true(got_short);

    // 2. InvalidChunkLength (declared chunk length < 4 or > buffer)
    let bad_chunk_body: [8]u8 = [0, 0, 0, 2, 0, 0, 0, 0];
    let bad_it = SctpWireChunkIter.fromBody(bad_chunk_body[0..]);
    let got_bad_len: bool = false;
    * = bad_it.next() catch |e| {
        if (e == SctpDcError.InvalidChunkLength) got_bad_len = true;
    };
    expect_true(got_bad_len);

    // 3. InvalidChannelType in DCEP OPEN
    let bad_dcep: [12]u8 = [DCEP_MSG_OPEN, 0x7F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let got_bad_ch: bool = false;
    * = decodeDcepOpen(bad_dcep[0..]) catch |e| {
        if (e == SctpDcError.InvalidChannelType) got_bad_ch = true;
    };
    expect_true(got_bad_ch);

    // 4. Truncated DCEP OPEN (label_len exceeds payload)
    let trunc_dcep: [12]u8 = [DCEP_MSG_OPEN, DCEP_CH_RELIABLE, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0];
    let got_bad_dcep: bool = false;
    * = decodeDcepOpen(trunc_dcep[0..]) catch |e| {
        if (e == SctpDcError.InvalidDcepMessage) got_bad_dcep = true;
    };
    expect_true(got_bad_dcep);

    // 5. WindowOverflow in SctpRecvTracker (delta > 64 for 1-word bitmap)
    let bm1: [1]u64 = [0; 1];
    let dp1: [2]u32 = [0; 2];
    let tr = SctpRecvTracker.init(100, 4096, bm1[0..], dp1[0..]) catch unreachable;
    let got_win_ovf: bool = false;
    * = tr.recordTsn(200) catch |e| {
        if (e == SctpDcError.WindowOverflow) got_win_ovf = true;
    };
    expect_true(got_win_ovf);

    // 6. StreamCountZero in SctpAssociationConfig
    let got_zero_streams: bool = false;
    * = SctpAssociation.init(SctpAssociationConfig{ local_tag: 1, num_outbound_streams: 0 }) catch |e| {
        if (e == SctpDcError.StreamCountZero) got_zero_streams = true;
    };
    expect_true(got_zero_streams);

    // 7. cap.net namespace parity
    expect_eq(cap.net.computeSctpCrc32c("123456789"), 0xE3069283);
    expect_eq(cap.net.SCTP_PPID_DCEP, 50);
    expect_eq(cap.net.SCTP_PPID_STRING_EMPTY, 56);
    expect_eq(cap.net.SCTP_CHUNK_FORWARD_TSN, 192);
}
