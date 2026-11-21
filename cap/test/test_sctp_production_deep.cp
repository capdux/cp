// Source: cp/cap/test/test_sctp_production_deep.cp (Production deep tests for cap:sctp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SCTP_HEADER_SIZE,
    SCTP_CHUNK_HEADER_SIZE,
    CHUNK_DATA,
    CHUNK_INIT,
    CHUNK_INIT_ACK,
    CHUNK_SACK,
    CHUNK_HEARTBEAT,
    CHUNK_HEARTBEAT_ACK,
    CHUNK_ABORT,
    CHUNK_SHUTDOWN,
    CHUNK_SHUTDOWN_ACK,
    CHUNK_ERROR,
    CHUNK_COOKIE_ECHO,
    CHUNK_COOKIE_ACK,
    CHUNK_SHUTDOWN_COMPLETE,
    DATA_FLAG_END,
    DATA_FLAG_BEGIN,
    DATA_FLAG_UNORDERED,
    DATA_FLAG_IMMEDIATE,
    crc32c,
    sctpCrc32c,
    verifySctpChecksum,
    SctpHeader,
    SctpChunk,
    SctpChunkIter,
    SctpPacket,
    parseSctpPacket,
    SctpDataChunk,
    parseSctpData,
    SctpInitChunk,
    parseSctpInit,
    SctpSackChunk,
    parseSctpSack,
    SctpWriter,
} from "cap:sctp";

test("1. Castagnoli CRC32C standard vector and DATA + SACK multi-chunk roundtrip", () => {
    // RFC 3720 / Castagnoli CRC32C standard check value for "123456789"
    expect_eq(crc32c("123456789"), 0xE306_9283 as u32);
    expect_eq(SCTP_HEADER_SIZE, 12);
    expect_eq(SCTP_CHUNK_HEADER_SIZE, 4);
    expect_eq(CHUNK_SHUTDOWN_COMPLETE, 14 as u8);

    // Build a multi-chunk packet containing DATA + SACK via SctpWriter
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let w = SctpWriter.init(buf[0..], 5000, 9899, 0xCAFE_BABE);

    const flags = DATA_FLAG_BEGIN | DATA_FLAG_END | DATA_FLAG_UNORDERED;
    expect_eq(w.writeDataChunk(flags, 0x0000_1001, 3, 7, 51, "SCTP-DATA"), true);

    // Build 12B SACK value (no gaps, no dups): cum_tsn=0x1000, a_rwnd=65536, gaps=0, dups=0
    const sack_val: [12]u8 = [
        0x00, 0x00, 0x10, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
    ];
    expect_eq(w.writeChunk(CHUNK_SACK, 0, sack_val[0..]), true);

    const wire = w.finish() ?? unreachable;
    // 12 (hdr) + (4 + 12 + 9 = 25 -> padded to 28) + (4 + 12 = 16) = 56 bytes
    expect_eq(wire.len, 56);
    expect_eq(verifySctpChecksum(wire), true);

    const pkt = parseSctpPacket(wire) ?? unreachable;
    expect_eq(pkt.header.src_port, 5000 as u16);
    expect_eq(pkt.header.dst_port, 9899 as u16);
    expect_eq(pkt.header.verification_tag, 0xCAFE_BABE as u32);
    expect_eq(pkt.header.checksum, sctpCrc32c(wire));
    expect_eq(pkt.verifyChecksum(wire), true);

    // Verify checksum is stored Little-Endian at bytes [8..12]
    const expected_crc = pkt.header.checksum;
    expect_eq(wire[8], (expected_crc & 0xFF) as u8);
    expect_eq(wire[9], ((expected_crc >> 8) & 0xFF) as u8);
    expect_eq(wire[10], ((expected_crc >> 16) & 0xFF) as u8);
    expect_eq(wire[11], ((expected_crc >> 24) & 0xFF) as u8);

    // Iterate chunks
    let it = pkt.chunks();
    const c0 = it.next() ?? unreachable;
    expect_eq(c0.chunk_type, CHUNK_DATA);
    expect_eq(c0.length, 25 as u16);

    const dc = parseSctpData(c0) ?? unreachable;
    expect_eq(dc.tsn, 0x0000_1001 as u32);
    expect_eq(dc.stream_id, 3 as u16);
    expect_eq(dc.stream_seq, 7 as u16);
    expect_eq(dc.ppid, 51 as u32);
    expect_eq(dc.isBegin(), true);
    expect_eq(dc.isEnd(), true);
    expect_eq(dc.isUnordered(), true);
    expect_eq(dc.isImmediate(), false);
    expect_eq(std.mem.eql(u8, dc.user_data, "SCTP-DATA"), true);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.chunk_type, CHUNK_SACK);
    expect_eq(c1.length, 16 as u16);

    const sc = parseSctpSack(c1) ?? unreachable;
    expect_eq(sc.cumulative_tsn_ack, 0x0000_1000 as u32);
    expect_eq(sc.a_rwnd, 65536 as u32);
    expect_eq(sc.num_gap_ack_blocks, 0 as u16);
    expect_eq(sc.num_dup_tsns, 0 as u16);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("2. INIT and SACK chunk detailed field decoding with params, gap blocks, and dup TSNs", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let w = SctpWriter.init(buf[0..], 12345, 54321, 0);

    // INIT value: 16B fixed + 8B optional parameter (IPv4 addr param type=5 len=8)
    const init_val: [24]u8 = [
        0x12, 0x34, 0x56, 0x78, // initiate_tag
        0x00, 0x02, 0x00, 0x00, // a_rwnd = 131072
        0x00, 0x10,             // outbound_streams = 16
        0x00, 0x20,             // inbound_streams = 32
        0xAA, 0xBB, 0xCC, 0xDD, // initial_tsn
        0x00, 0x05, 0x00, 0x08, // param header
        192, 168, 1, 10,        // IPv4 address
    ];
    expect_eq(w.writeChunk(CHUNK_INIT, 0, init_val[0..]), true);

    // SACK value: 12B fixed + 2 gap ack blocks (8B) + 1 dup TSN (4B) = 24B
    const sack_val: [24]u8 = [
        0x00, 0x00, 0x01, 0x00, // cumulative_tsn_ack = 256
        0x00, 0x04, 0x00, 0x00, // a_rwnd = 262144
        0x00, 0x02,             // num_gap_ack_blocks = 2
        0x00, 0x01,             // num_dup_tsns = 1
        0x00, 0x02, 0x00, 0x04, // gap block 0: start=2, end=4
        0x00, 0x06, 0x00, 0x09, // gap block 1: start=6, end=9
        0x00, 0x00, 0x01, 0x02, // dup TSN 0 = 258
    ];
    expect_eq(w.writeChunk(CHUNK_SACK, 0, sack_val[0..]), true);

    const wire = w.finish() ?? unreachable;
    expect_eq(verifySctpChecksum(wire), true);

    const pkt = parseSctpPacket(wire) ?? unreachable;
    let it = pkt.chunks();

    const c_init = it.next() ?? unreachable;
    const init_chunk = parseSctpInit(c_init) ?? unreachable;
    expect_eq(init_chunk.initiate_tag, 0x1234_5678 as u32);
    expect_eq(init_chunk.a_rwnd, 131072 as u32);
    expect_eq(init_chunk.outbound_streams, 16 as u16);
    expect_eq(init_chunk.inbound_streams, 32 as u16);
    expect_eq(init_chunk.initial_tsn, 0xAABB_CCDD as u32);
    expect_eq(init_chunk.params_slice.len, 8);
    expect_eq(init_chunk.params_slice[4], 192 as u8);
    expect_eq(init_chunk.params_slice[7], 10 as u8);

    const c_sack = it.next() ?? unreachable;
    const sack_chunk = parseSctpSack(c_sack) ?? unreachable;
    expect_eq(sack_chunk.cumulative_tsn_ack, 256 as u32);
    expect_eq(sack_chunk.a_rwnd, 262144 as u32);
    expect_eq(sack_chunk.num_gap_ack_blocks, 2 as u16);
    expect_eq(sack_chunk.num_dup_tsns, 1 as u16);
    expect_eq(sack_chunk.gap_blocks_slice.len, 8);
    expect_eq(sack_chunk.dup_tsns_slice.len, 4);
    expect_eq(sack_chunk.dup_tsns_slice[3], 0x02 as u8);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("3. Odd-length payloads (5B, 1B, 3B) 4-byte alignment padding and unpadded final chunk at EOF", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    // Pre-fill buffer with non-zero bytes to verify SctpWriter zeroes padding bytes
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0xFF;
        i += 1;
    }

    let w = SctpWriter.init(buf[0..], 4000, 4001, 0x0102_0304);
    // 5B user_data -> chunk_len = 21, pad = 3 -> 24B
    expect_eq(w.writeDataChunk(DATA_FLAG_BEGIN | DATA_FLAG_END, 1, 0, 0, 50, "hello"), true);
    // 1B value -> chunk_len = 5, pad = 3 -> 8B
    expect_eq(w.writeChunk(CHUNK_HEARTBEAT, 0, "X"), true);
    // 3B value -> chunk_len = 7, pad = 1 -> 8B
    expect_eq(w.writeChunk(CHUNK_COOKIE_ECHO, 0, "ABC"), true);

    const wire = w.finish() ?? unreachable;
    expect_eq(wire.len, 12 + 24 + 8 + 8);
    expect_eq(verifySctpChecksum(wire), true);

    // Verify padding bytes after first chunk (offset 12 + 21 .. 12 + 24) are zeroed
    expect_eq(wire[33], 0 as u8);
    expect_eq(wire[34], 0 as u8);
    expect_eq(wire[35], 0 as u8);

    const pkt = parseSctpPacket(wire) ?? unreachable;
    let it = pkt.chunks();

    const ch0 = it.next() ?? unreachable;
    const d0 = parseSctpData(ch0) ?? unreachable;
    expect_eq(ch0.length, 21 as u16);
    expect_eq(std.mem.eql(u8, d0.user_data, "hello"), true);

    const ch1 = it.next() ?? unreachable;
    expect_eq(ch1.chunk_type, CHUNK_HEARTBEAT);
    expect_eq(ch1.length, 5 as u16);
    expect_eq(std.mem.eql(u8, ch1.value, "X"), true);

    const ch2 = it.next() ?? unreachable;
    expect_eq(ch2.chunk_type, CHUNK_COOKIE_ECHO);
    expect_eq(ch2.length, 7 as u16);
    expect_eq(std.mem.eql(u8, ch2.value, "ABC"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);

    // RFC 4960: final chunk without trailing padding bytes at EOF (slice off last 1B pad)
    const unpadded_slice = wire[0..(wire.len - 1)];
    const pkt2 = parseSctpPacket(unpadded_slice) ?? unreachable;
    let it2 = pkt2.chunks();
    * = it2.next() ?? unreachable;
    * = it2.next() ?? unreachable;
    const last_ch = it2.next() ?? unreachable;
    expect_eq(last_ch.chunk_type, CHUNK_COOKIE_ECHO);
    expect_eq(std.mem.eql(u8, last_ch.value, "ABC"), true);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), false);
});

test("4. Checksum tampering detection across header, payload, and checksum fields", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = SctpWriter.init(buf[0..], 8080, 9090, 0xDEAD_BEEF);
    expect_eq(w.writeDataChunk(DATA_FLAG_BEGIN | DATA_FLAG_END, 42, 1, 1, 51, "integrity"), true);
    const wire = w.finish() ?? unreachable;
    const total = wire.len;
    expect_eq(verifySctpChecksum(buf[0..total]), true);

    // 1. Flip a bit in payload
    buf[total - 4] = buf[total - 4] ^ 0x01;
    expect_eq(verifySctpChecksum(buf[0..total]), false);
    const p_bad1 = parseSctpPacket(buf[0..total]) ?? unreachable;
    expect_eq(p_bad1.verifyChecksum(buf[0..total]), false);
    // Restore
    buf[total - 4] = buf[total - 4] ^ 0x01;
    expect_eq(verifySctpChecksum(buf[0..total]), true);

    // 2. Flip a bit in header verification_tag
    buf[5] = buf[5] ^ 0x80;
    expect_eq(verifySctpChecksum(buf[0..total]), false);
    buf[5] = buf[5] ^ 0x80;
    expect_eq(verifySctpChecksum(buf[0..total]), true);

    // 3. Flip a bit in checksum field itself
    buf[8] = buf[8] ^ 0x01;
    expect_eq(verifySctpChecksum(buf[0..total]), false);
    buf[8] = buf[8] ^ 0x01;
    expect_eq(verifySctpChecksum(buf[0..total]), true);
});

test("5. Malformed, truncated, chunk_len < 4, and out-of-bounds resilience", () => {
    // 1. Packet shorter than 12B common header
    const short_pkt: [11]u8 = [0; 11];
    expect_eq(parseSctpPacket(short_pkt[0..]) == null, true);
    expect_eq(verifySctpChecksum(short_pkt[0..]), false);

    // 2. Chunk with length < 4 (length = 3)
    const bad_len_pkt: [16]u8 = [
        0x00, 0x50, 0x00, 0x51,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        CHUNK_DATA, 0x00, 0x00, 0x03, // length = 3 < 4
    ];
    const p1 = parseSctpPacket(bad_len_pkt[0..]) ?? unreachable;
    let it1 = p1.chunks();
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);

    // 3. Chunk declaring length beyond packet bounds (length = 40, only 4B available)
    const oob_pkt: [16]u8 = [
        0x00, 0x50, 0x00, 0x51,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        CHUNK_HEARTBEAT, 0x00, 0x00, 40,
    ];
    const p2 = parseSctpPacket(oob_pkt[0..]) ?? unreachable;
    let it2 = p2.chunks();
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);

    // 4. Truncated chunk header (only 2 bytes after 12B common header)
    const trunc_hdr: [14]u8 = [0; 14];
    const p3 = parseSctpPacket(trunc_hdr[0..]) ?? unreachable;
    let it3 = p3.chunks();
    expect_eq(it3.next() == null, true);
    expect_eq(it3.hasError(), true);

    // 5. Truncated DATA / INIT / SACK chunks & wrong chunk types
    const short_data = SctpChunk{ .chunk_type = CHUNK_DATA, .flags = 0, .length = 12, .value = bad_len_pkt[0..8] };
    expect_eq(parseSctpData(short_data) == null, true);
    expect_eq(parseSctpInit(short_data) == null, true);
    expect_eq(parseSctpSack(short_data) == null, true);

    // SACK claiming 2 gap blocks (needs 12 + 8 = 20B) but only 12B value provided
    const bad_sack_val: [12]u8 = [
        0, 0, 0, 1,
        0, 0, 1, 0,
        0, 2, // 2 gap blocks declared
        0, 0,
    ];
    const bad_sack = SctpChunk{ .chunk_type = CHUNK_SACK, .flags = 0, .length = 16, .value = bad_sack_val[0..] };
    expect_eq(parseSctpSack(bad_sack) == null, true);

    // 6. SctpWriter buffer too small (< 12B) and write overflow atomicity
    let tiny: [8]u8 = [0; 8];
    let w_tiny = SctpWriter.init(tiny[0..], 1, 2, 3);
    expect_eq(w_tiny.writeChunk(CHUNK_COOKIE_ACK, 0, ""), false);
    expect_eq(w_tiny.finish() == null, true);

    let tight: [20]u8 = [0; 20];
    let w_tight = SctpWriter.init(tight[0..], 1, 2, 3);
    // 12B header used, 8B left; writing 16B DATA chunk must fail atomically
    expect_eq(w_tight.writeDataChunk(0, 1, 0, 0, 0, ""), false);
    expect_eq(w_tight.remaining(), 8);
    // Writing 4B COOKIE_ACK (0B value) succeeds
    expect_eq(w_tight.writeChunk(CHUNK_COOKIE_ACK, 0, ""), true);
    const fin = w_tight.finish() ?? unreachable;
    expect_eq(fin.len, 16);
    expect_eq(verifySctpChecksum(fin), true);
});
