import {
    STMT_PREPARE_OK_STATUS,
    STMT_PREPARE_OK_MIN_LEN,
    STMT_PREPARE_OK_STD_LEN,
    STMT_PREPARE_OK_META_LEN,
    COM_STMT_PREPARE_CMD,
    COM_STMT_CLOSE_CMD,
    COM_STMT_RESET_CMD,
    STMT_METADATA_NONE,
    STMT_METADATA_FOLLOWS,
    StmtPrepareOkError,
    StmtPrepareOKPacket,
    StmtCommandCodec,
} from "cap:mysql";
import { expect, test } from "cap:test";

test("mysql stmtPrepareOk: standard 12-byte payload and frame roundtrip", fn () {
    const pkt = StmtPrepareOKPacket.init(42, 3, 2, 1);
    expect(pkt.status).toBe(STMT_PREPARE_OK_STATUS);
    expect(pkt.encodedPayloadLen()).toBe(STMT_PREPARE_OK_STD_LEN);
    expect(pkt.hasWarnings()).toBe(true);
    expect(pkt.expectsParamDefs()).toBe(true);
    expect(pkt.expectsColumnDefs()).toBe(true);

    let frame: [32]u8 = [0; 32];
    const n = pkt.encodeFrame(frame[0..]) catch 0;
    expect(n).toBe(16); // 4B header + 12B payload
    expect(frame[0]).toBe(12);
    expect(frame[1]).toBe(0);
    expect(frame[2]).toBe(0);
    expect(frame[3]).toBe(1);
    expect(frame[4]).toBe(0x00);

    const decoded = StmtPrepareOKPacket.decodeFrame(frame[0..n]) catch StmtPrepareOKPacket.init(0, 0, 0, 0);
    expect(decoded.eql(pkt)).toBe(true);
    expect(decoded.statement_id).toBe(42);
    expect(decoded.num_columns).toBe(3);
    expect(decoded.num_params).toBe(2);
    expect(decoded.warning_count).toBe(1);
    expect(decoded.packet_length).toBe(12);
});

test("mysql stmtPrepareOk: MySQL 8.0 13-byte optional metadata_follows flag", fn () {
    const pkt_no_meta = StmtPrepareOKPacket.initWithMetadata(0x12345678, 5, 1, 0, false);
    expect(pkt_no_meta.encodedPayloadLen()).toBe(STMT_PREPARE_OK_META_LEN);
    expect(pkt_no_meta.has_metadata_flag).toBe(true);
    expect(pkt_no_meta.metadata_follows).toBe(false);
    expect(pkt_no_meta.expectsParamDefs()).toBe(true);
    expect(pkt_no_meta.expectsColumnDefs()).toBe(false);

    let buf: [32]u8 = [0; 32];
    const n = pkt_no_meta.encodeFrame(buf[0..]) catch 0;
    expect(n).toBe(17);
    expect(buf[16]).toBe(STMT_METADATA_NONE);

    const parsed = StmtPrepareOKPacket.decodeFrame(buf[0..n]) catch StmtPrepareOKPacket.init(0, 0, 0, 0);
    expect(parsed.eql(pkt_no_meta)).toBe(true);
    expect(parsed.statement_id).toBe(0x12345678);
    expect(parsed.metadata_follows).toBe(false);

    // With CLIENT_DEPRECATE_EOF=false: 1 param + 1 EOF = 2 (columns skipped because metadata_follows=false)
    expect(parsed.expectedFollowupPackets(false)).toBe(2);
    // With CLIENT_DEPRECATE_EOF=true: 1 param only = 1
    expect(parsed.expectedFollowupPackets(true)).toBe(1);

    const pkt_with_meta = StmtPrepareOKPacket.initWithMetadata(99, 4, 2, 0, true);
    const n2 = pkt_with_meta.encodeFrame(buf[0..]) catch 0;
    expect(buf[16]).toBe(STMT_METADATA_FOLLOWS);
    const parsed2 = StmtPrepareOKPacket.decodeFrame(buf[0..n2]) catch StmtPrepareOKPacket.init(0, 0, 0, 0);
    expect(parsed2.expectsColumnDefs()).toBe(true);
    // 2 params + EOF + 4 cols + EOF = 8
    expect(parsed2.expectedFollowupPackets(false)).toBe(8);
    // 2 params + 4 cols = 6
    expect(parsed2.expectedFollowupPackets(true)).toBe(6);
});

test("mysql stmtPrepareOk: legacy 10-byte payload without warning_count", fn () {
    const legacy: [10]u8 = [
        0x00,             // status
        0x07, 0x00, 0x00, 0x00, // statement_id = 7
        0x02, 0x00,       // num_columns = 2
        0x00, 0x00,       // num_params = 0
        0x00,             // reserved filler
    ];
    const parsed = StmtPrepareOKPacket.decodePayload(legacy[0..], 2) catch StmtPrepareOKPacket.init(0, 0, 0, 0);
    expect(parsed.statement_id).toBe(7);
    expect(parsed.num_columns).toBe(2);
    expect(parsed.num_params).toBe(0);
    expect(parsed.warning_count).toBe(0);
    expect(parsed.hasWarnings()).toBe(false);
    expect(parsed.packet_length).toBe(STMT_PREPARE_OK_MIN_LEN);
    expect(parsed.sequence_id).toBe(2);
    expect(parsed.expectsParamDefs()).toBe(false);
    expect(parsed.expectsColumnDefs()).toBe(true);
    expect(parsed.expectedFollowupPackets(false)).toBe(3); // 2 cols + 1 EOF
    expect(parsed.expectedFollowupPackets(true)).toBe(2);  // 2 cols only
});

test("mysql stmtPrepareOk: rejects malformed status, truncation, reserved byte and bad metadata flag", fn () {
    // Error packet header 0xFF instead of 0x00
    const err_pkt: [12]u8 = [0xFF, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let got_invalid_status = false;
    * = StmtPrepareOKPacket.decodePayload(err_pkt[0..], 1) catch |e| {
        got_invalid_status = e == StmtPrepareOkError.InvalidStatus;
    };
    expect(got_invalid_status).toBe(true);

    // Truncated payload (< 10 bytes)
    const short_pkt: [9]u8 = [0x00, 1, 0, 0, 0, 1, 0, 1, 0];
    let got_truncated = false;
    * = StmtPrepareOKPacket.decodePayload(short_pkt[0..], 1) catch |e| {
        got_truncated = e == StmtPrepareOkError.Truncated;
    };
    expect(got_truncated).toBe(true);

    // Strict mode rejects non-zero reserved filler byte at offset 9
    const bad_reserved: [12]u8 = [0x00, 1, 0, 0, 0, 1, 0, 0, 0, 0x01, 0, 0];
    let got_bad_reserved = false;
    * = StmtPrepareOKPacket.decodePayloadStrict(bad_reserved[0..], 1) catch |e| {
        got_bad_reserved = e == StmtPrepareOkError.InvalidReservedByte;
    };
    expect(got_bad_reserved).toBe(true);

    // Invalid metadata_follows flag (must be 0 or 1)
    const bad_meta: [13]u8 = [0x00, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0x02];
    let got_bad_meta = false;
    * = StmtPrepareOKPacket.decodePayload(bad_meta[0..], 1) catch |e| {
        got_bad_meta = e == StmtPrepareOkError.InvalidMetadataFlag;
    };
    expect(got_bad_meta).toBe(true);

    // Truncated frame header / body
    const short_frame: [8]u8 = [12, 0, 0, 1, 0, 1, 0, 0];
    let got_frame_trunc = false;
    * = StmtPrepareOKPacket.decodeFrame(short_frame[0..]) catch |e| {
        got_frame_trunc = e == StmtPrepareOkError.Truncated;
    };
    expect(got_frame_trunc).toBe(true);
});

test("mysql stmtPrepareOk: COM_STMT_PREPARE, COM_STMT_CLOSE and COM_STMT_RESET command frames", fn () {
    let out: [128]u8 = [0; 128];
    const sql = "SELECT id, name FROM users WHERE id = ?";
    const n_prep = StmtCommandCodec.encodePrepareFrame(sql, 0, out[0..]) catch 0;
    expect(n_prep).toBe(4 + 1 + sql.len);
    expect(out[0]).toBe(@as(u8, @intCast(1 + sql.len)));
    expect(out[1]).toBe(0);
    expect(out[2]).toBe(0);
    expect(out[3]).toBe(0);
    expect(out[4]).toBe(COM_STMT_PREPARE_CMD);
    expect(out[5]).toBe('S');

    // Empty SQL rejected
    let got_empty = false;
    * = StmtCommandCodec.encodePrepareFrame("", 0, out[0..]) catch |e| {
        got_empty = e == StmtPrepareOkError.EmptySql;
    };
    expect(got_empty).toBe(true);

    // COM_STMT_CLOSE (0x19)
    const n_close = StmtCommandCodec.encodeCloseFrame(0x04030201, 0, out[0..]) catch 0;
    expect(n_close).toBe(9);
    expect(out[0]).toBe(5);
    expect(out[4]).toBe(COM_STMT_CLOSE_CMD);
    expect(out[5]).toBe(0x01);
    expect(out[6]).toBe(0x02);
    expect(out[7]).toBe(0x03);
    expect(out[8]).toBe(0x04);

    // COM_STMT_RESET (0x1A)
    const n_reset = StmtCommandCodec.encodeResetFrame(0x0000002A, 3, out[0..]) catch 0;
    expect(n_reset).toBe(9);
    expect(out[0]).toBe(5);
    expect(out[3]).toBe(3);
    expect(out[4]).toBe(COM_STMT_RESET_CMD);
    expect(out[5]).toBe(0x2A);
});

test("mysql stmtPrepareOk: buffer too small guards on payload, frame and command encoders", fn () {
    const pkt = StmtPrepareOKPacket.init(1, 1, 1, 0);
    let tiny: [8]u8 = [0; 8];

    let payload_small = false;
    * = pkt.encodePayload(tiny[0..]) catch |e| {
        payload_small = e == StmtPrepareOkError.BufferTooSmall;
    };
    expect(payload_small).toBe(true);

    let frame_small = false;
    * = pkt.encodeFrame(tiny[0..]) catch |e| {
        frame_small = e == StmtPrepareOkError.BufferTooSmall;
    };
    expect(frame_small).toBe(true);

    let close_small = false;
    * = StmtCommandCodec.encodeCloseFrame(1, 0, tiny[0..]) catch |e| {
        close_small = e == StmtPrepareOkError.BufferTooSmall;
    };
    expect(close_small).toBe(true);

    let reset_small = false;
    * = StmtCommandCodec.encodeResetFrame(1, 0, tiny[0..]) catch |e| {
        reset_small = e == StmtPrepareOkError.BufferTooSmall;
    };
    expect(reset_small).toBe(true);
});
