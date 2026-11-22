import {
    TAG_EXECUTE,
    TAG_DESCRIBE,
    TAG_CLOSE,
    TAG_SYNC,
    TAG_FLUSH,
    TARGET_PORTAL,
    TARGET_PREPARED_STATEMENT,
    EXECUTE_UNLIMITED_ROWS,
    ExecuteMsgError,
    PortalTarget,
    ExecuteMessage,
    ExecuteControlCodec,
} from "cap:pg";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("pg executeMsg: unnamed portal unlimited Execute roundtrip", fn () {
    const msg = ExecuteMessage.unlimited("");
    expect(msg.isUnlimited()).toBe(true);
    expect(msg.target.isUnnamed()).toBe(true);
    expect(msg.target.targetByte()).toBe(TARGET_PORTAL);
    expect(msg.encodedFrameLen()).toBe(10); // 1('E') + 4(len=9) + 1('\0') + 4(max_rows=0)

    let wire: [32]u8 = [0; 32];
    const n = msg.encodeFrame(wire[0..]) catch 0;
    expect(n).toBe(10);
    expect(wire[0]).toBe(TAG_EXECUTE);
    expect(wire[4]).toBe(9);
    expect(wire[5]).toBe(0);
    expect(wire[9]).toBe(0);

    const decoded = ExecuteMessage.decodeFrame(wire[0..n]) catch ExecuteMessage.unlimited("bad");
    expect(decoded.eql(msg)).toBe(true);
    expect(decoded.max_rows).toBe(EXECUTE_UNLIMITED_ROWS);
});

test("pg executeMsg: named portal with positive max_rows pagination", fn () {
    const msg = ExecuteMessage.init(PortalTarget.portal("cursor_42"), 500);
    expect(msg.isUnlimited()).toBe(false);
    expect(msg.encodedFrameLen()).toBe(10 + 9);

    let wire: [64]u8 = [0; 64];
    const n = msg.encodeFrame(wire[0..]) catch 0;
    expect(n).toBe(19);

    const decoded = ExecuteMessage.decodeFrame(wire[0..n]) catch ExecuteMessage.unlimited("");
    expect(decoded.eql(msg)).toBe(true);
    expect(sliceEql(decoded.target.name, "cursor_42")).toBe(true);
    expect(decoded.max_rows).toBe(500);
});

test("pg executeMsg: preparedStatement target writes empty portal name per Bun Execute.cp", fn () {
    const stmt = PortalTarget.preparedStatement("stmt_1");
    expect(stmt.targetByte()).toBe(TARGET_PREPARED_STATEMENT);
    expect(stmt.wirePortalName().len).toBe(0);

    const msg = ExecuteMessage.init(stmt, 10);
    let wire: [32]u8 = [0; 32];
    const n = msg.encodeFrame(wire[0..]) catch 0;
    expect(n).toBe(10); // empty portal name + '\0' + 4B max_rows

    const decoded = ExecuteMessage.decodeFrame(wire[0..n]) catch ExecuteMessage.unlimited("x");
    expect(decoded.target.isUnnamed()).toBe(true);
    expect(decoded.max_rows).toBe(10);
});

test("pg executeMsg: Describe, Close, Sync and Flush control frames", fn () {
    let buf: [64]u8 = [0; 64];

    const nd = ExecuteControlCodec.encodeDescribeFrame(PortalTarget.preparedStatement("s1"), buf[0..]) catch 0;
    expect(nd).toBe(9); // 'D' + 4B(8) + 'S' + "s1\0"
    expect(buf[0]).toBe(TAG_DESCRIBE);
    expect(buf[4]).toBe(8);
    expect(buf[5]).toBe(TARGET_PREPARED_STATEMENT);
    expect(buf[6]).toBe('s');
    expect(buf[7]).toBe('1');
    expect(buf[8]).toBe(0);

    const nc = ExecuteControlCodec.encodeCloseFrame(PortalTarget.portal("p9"), buf[0..]) catch 0;
    expect(nc).toBe(9);
    expect(buf[0]).toBe(TAG_CLOSE);
    expect(buf[5]).toBe(TARGET_PORTAL);

    const ns = ExecuteControlCodec.encodeSyncFrame(buf[0..]) catch 0;
    expect(ns).toBe(5);
    expect(buf[0]).toBe(TAG_SYNC);
    expect(buf[4]).toBe(4);

    const nf = ExecuteControlCodec.encodeFlushFrame(buf[0..]) catch 0;
    expect(nf).toBe(5);
    expect(buf[0]).toBe(TAG_FLUSH);
    expect(buf[4]).toBe(4);
});

test("pg executeMsg: rejects negative max_rows, embedded NUL, invalid tag and truncation", fn () {
    let wire: [32]u8 = [0; 32];

    // Negative max_rows on encode
    const neg_msg = ExecuteMessage.init(PortalTarget.unnamedPortal(), -1);
    let got_neg = false;
    * = neg_msg.encodeFrame(wire[0..]) catch |e| {
        got_neg = e == ExecuteMsgError.NegativeMaxRows;
    };
    expect(got_neg).toBe(true);

    // Embedded NUL in portal name on encode
    const nul_name: [4]u8 = ['a', 0, 'b', 'c'];
    const nul_msg = ExecuteMessage.unlimited(nul_name[0..]);
    let got_nul = false;
    * = nul_msg.encodeFrame(wire[0..]) catch |e| {
        got_nul = e == ExecuteMsgError.EmbeddedNul;
    };
    expect(got_nul).toBe(true);

    // Invalid tag on decode
    const bad_tag: [10]u8 = ['B', 0, 0, 0, 9, 0, 0, 0, 0, 0];
    let got_tag = false;
    * = ExecuteMessage.decodeFrame(bad_tag[0..]) catch |e| {
        got_tag = e == ExecuteMsgError.InvalidTag;
    };
    expect(got_tag).toBe(true);

    // Truncated frame on decode
    const trunc: [6]u8 = ['E', 0, 0, 0, 9, 0];
    let got_trunc = false;
    * = ExecuteMessage.decodeFrame(trunc[0..]) catch |e| {
        got_trunc = e == ExecuteMsgError.Truncated;
    };
    expect(got_trunc).toBe(true);

    // Missing NUL terminator before max_rows on decode
    const no_nul: [10]u8 = ['E', 0, 0, 0, 9, 'x', 0, 0, 0, 1];
    let got_len = false;
    * = ExecuteMessage.decodeFrame(no_nul[0..]) catch |e| {
        got_len = e == ExecuteMsgError.InvalidLength;
    };
    expect(got_len).toBe(true);
});

test("pg executeMsg: buffer too small guards across encoders", fn () {
    let tiny: [4]u8 = [0; 4];

    let exec_small = false;
    * = ExecuteMessage.unlimited("").encodeFrame(tiny[0..]) catch |e| {
        exec_small = e == ExecuteMsgError.BufferTooSmall;
    };
    expect(exec_small).toBe(true);

    let sync_small = false;
    * = ExecuteControlCodec.encodeSyncFrame(tiny[0..]) catch |e| {
        sync_small = e == ExecuteMsgError.BufferTooSmall;
    };
    expect(sync_small).toBe(true);

    let flush_small = false;
    * = ExecuteControlCodec.encodeFlushFrame(tiny[0..]) catch |e| {
        flush_small = e == ExecuteMsgError.BufferTooSmall;
    };
    expect(flush_small).toBe(true);
});
