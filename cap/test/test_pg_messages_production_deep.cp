import { test, expect_eq } from "cap:test";
import {
    TAG_ERROR_RESPONSE,
    TAG_NOTICE_RESPONSE,
    TAG_BACKEND_KEY_DATA,
    TAG_PARAMETER_STATUS,
    TAG_READY_FOR_QUERY,
    TAG_NOTIFICATION_RESPONSE,
    TAG_COMMAND_COMPLETE,
    PgMessageError,
    DiagSqlStateClass,
    classifySqlState,
    TransactionStatus,
    decodeErrorResponse,
    decodeNoticeResponse,
    decodeBackendKeyData,
    decodeParameterStatus,
    decodeReadyForQuery,
    decodeNotificationResponse,
    decodeCommandComplete,
} from "cap:pg";

fn writeFrameHeader(buf: []u8, tag: u8, body_len: usize): usize {
    const total_len: u32 = (body_len + 4) as u32;
    buf[0] = tag;
    buf[1] = ((total_len >> 24) & 0xFF) as u8;
    buf[2] = ((total_len >> 16) & 0xFF) as u8;
    buf[3] = ((total_len >> 8) & 0xFF) as u8;
    buf[4] = (total_len & 0xFF) as u8;
    return 5;
}

fn appendField(buf: []u8, pos: usize, code: u8, val: string): usize {
    buf[pos] = code;
    @memcpy(buf[pos + 1 .. pos + 1 + val.len], val);
    buf[pos + 1 + val.len] = 0;
    return pos + 1 + val.len + 1;
}

test("1. ErrorResponse & NoticeResponse full diagnostic fields + SQLSTATE classification", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 5;
    pos = appendField(buf[0..], pos, 'S', "ERROR");
    pos = appendField(buf[0..], pos, 'V', "ERROR");
    pos = appendField(buf[0..], pos, 'C', "23505");
    pos = appendField(buf[0..], pos, 'M', "duplicate key value violates unique constraint");
    pos = appendField(buf[0..], pos, 'D', "Key (email)=(a@b.com) already exists.");
    pos = appendField(buf[0..], pos, 'H', "Use ON CONFLICT DO NOTHING.");
    pos = appendField(buf[0..], pos, 'P', "24");
    pos = appendField(buf[0..], pos, 's', "public");
    pos = appendField(buf[0..], pos, 't', "users");
    pos = appendField(buf[0..], pos, 'c', "email");
    pos = appendField(buf[0..], pos, 'n', "users_email_key");
    buf[pos] = 0; // terminating NUL
    pos += 1;
    * = writeFrameHeader(buf[0..], TAG_ERROR_RESPONSE, pos - 5);

    const err_msg = decodeErrorResponse(buf[0..pos]) catch unreachable;
    expect_eq(err_msg.is_error, true);
    expect_eq(err_msg.isFatal(), false);
    expect_eq(err_msg.bytes_read, pos);
    expect_eq(std.mem.eql(u8, err_msg.severity.?, "ERROR"), true);
    expect_eq(std.mem.eql(u8, err_msg.code.?, "23505"), true);
    expect_eq(err_msg.sqlStateClass() == DiagSqlStateClass.IntegrityConstraintViolation, true);
    expect_eq(std.mem.eql(u8, err_msg.schema.?, "public"), true);
    expect_eq(std.mem.eql(u8, err_msg.table.?, "users"), true);
    expect_eq(std.mem.eql(u8, err_msg.column.?, "email"), true);
    expect_eq(std.mem.eql(u8, err_msg.constraint.?, "users_email_key"), true);
    expect_eq(std.mem.eql(u8, err_msg.position.?, "24"), true);

    // NoticeResponse ('N')
    let nbuf: [64]u8 = std.mem.zeroes([64]u8);
    let npos: usize = 5;
    npos = appendField(nbuf[0..], npos, 'S', "NOTICE");
    npos = appendField(nbuf[0..], npos, 'C', "00000");
    npos = appendField(nbuf[0..], npos, 'M', "relation already exists");
    nbuf[npos] = 0;
    npos += 1;
    * = writeFrameHeader(nbuf[0..], TAG_NOTICE_RESPONSE, npos - 5);

    const notice = decodeNoticeResponse(nbuf[0..npos]) catch unreachable;
    expect_eq(notice.is_error, false);
    expect_eq(notice.sqlStateClass() == DiagSqlStateClass.Success, true);
    expect_eq(classifySqlState("42601") == DiagSqlStateClass.SyntaxOrAccessError, true);
    expect_eq(classifySqlState("08006") == DiagSqlStateClass.ConnectionException, true);
});

test("2. BackendKeyData (pid + secret_key) & ParameterStatus (name + value)", () => {
    // BackendKeyData: 'K', len=12, pid=0x00001234, secret=0xDEADBEEF
    const bkd_pkt: [13]u8 = [
        TAG_BACKEND_KEY_DATA, 0, 0, 0, 12,
        0x00, 0x00, 0x12, 0x34,
        0xDE, 0xAD, 0xBE, 0xEF,
    ];
    const bkd = decodeBackendKeyData(bkd_pkt[0..]) catch unreachable;
    expect_eq(bkd.process_id, u32(0x1234));
    expect_eq(bkd.secret_key, u32(0xDEADBEEF));
    expect_eq(bkd.bytes_read, usize(13));

    // ParameterStatus: 'S' + len + "server_version\0" + "16.2\0"
    let pbuf: [64]u8 = std.mem.zeroes([64]u8);
    const k = "server_version";
    const v = "16.2";
    @memcpy(pbuf[5 .. 5 + k.len], k);
    pbuf[5 + k.len] = 0;
    const v_off = 5 + k.len + 1;
    @memcpy(pbuf[v_off .. v_off + v.len], v);
    pbuf[v_off + v.len] = 0;
    const total = v_off + v.len + 1;
    * = writeFrameHeader(pbuf[0..], TAG_PARAMETER_STATUS, total - 5);

    const ps = decodeParameterStatus(pbuf[0..total]) catch unreachable;
    expect_eq(std.mem.eql(u8, ps.name, "server_version"), true);
    expect_eq(std.mem.eql(u8, ps.value, "16.2"), true);
    expect_eq(ps.bytes_read, total);
});

test("3. ReadyForQuery & TransactionStatus (I/T/E)", () => {
    const rfq_idle: [6]u8 = [TAG_READY_FOR_QUERY, 0, 0, 0, 5, 'I'];
    const r1 = decodeReadyForQuery(rfq_idle[0..]) catch unreachable;
    expect_eq(r1.status == TransactionStatus.idle, true);
    expect_eq(r1.status.isIdle(), true);
    expect_eq(r1.status.toByte(), u8('I'));

    const rfq_tx: [6]u8 = [TAG_READY_FOR_QUERY, 0, 0, 0, 5, 'T'];
    const r2 = decodeReadyForQuery(rfq_tx[0..]) catch unreachable;
    expect_eq(r2.status.isInTransaction(), true);

    const rfq_fail: [6]u8 = [TAG_READY_FOR_QUERY, 0, 0, 0, 5, 'E'];
    const r3 = decodeReadyForQuery(rfq_fail[0..]) catch unreachable;
    expect_eq(r3.status.isFailed(), true);
});

test("4. NotificationResponse (pid/channel/payload) & CommandComplete row count parsing", () => {
    // NotificationResponse: 'A' + len + pid(42) + "events\0" + "{\"ok\":true}\0"
    let nbuf: [64]u8 = std.mem.zeroes([64]u8);
    nbuf[5] = 0;
    nbuf[6] = 0;
    nbuf[7] = 0;
    nbuf[8] = 42;
    const ch = "events";
    const pl = "{\"ok\":true}";
    @memcpy(nbuf[9 .. 9 + ch.len], ch);
    nbuf[9 + ch.len] = 0;
    const pl_off = 9 + ch.len + 1;
    @memcpy(nbuf[pl_off .. pl_off + pl.len], pl);
    nbuf[pl_off + pl.len] = 0;
    const ntotal = pl_off + pl.len + 1;
    * = writeFrameHeader(nbuf[0..], TAG_NOTIFICATION_RESPONSE, ntotal - 5);

    const notif = decodeNotificationResponse(nbuf[0..ntotal]) catch unreachable;
    expect_eq(notif.pid, u32(42));
    expect_eq(std.mem.eql(u8, notif.channel, "events"), true);
    expect_eq(std.mem.eql(u8, notif.payload, "{\"ok\":true}"), true);

    // CommandComplete tags: SELECT 42, INSERT 0 5, UPDATE 3, CREATE TABLE
    let cbuf: [64]u8 = std.mem.zeroes([64]u8);

    const t1 = "SELECT 42";
    @memcpy(cbuf[5 .. 5 + t1.len], t1);
    cbuf[5 + t1.len] = 0;
    * = writeFrameHeader(cbuf[0..], TAG_COMMAND_COMPLETE, t1.len + 1);
    const c1 = decodeCommandComplete(cbuf[0 .. 5 + t1.len + 1]) catch unreachable;
    expect_eq(std.mem.eql(u8, c1.command(), "SELECT"), true);
    expect_eq(c1.rowsAffected().?, u64(42));

    const t2 = "INSERT 0 5";
    @memcpy(cbuf[5 .. 5 + t2.len], t2);
    cbuf[5 + t2.len] = 0;
    * = writeFrameHeader(cbuf[0..], TAG_COMMAND_COMPLETE, t2.len + 1);
    const c2 = decodeCommandComplete(cbuf[0 .. 5 + t2.len + 1]) catch unreachable;
    expect_eq(std.mem.eql(u8, c2.command(), "INSERT"), true);
    expect_eq(c2.rowsAffected().?, u64(5));

    const t3 = "CREATE TABLE";
    @memcpy(cbuf[5 .. 5 + t3.len], t3);
    cbuf[5 + t3.len] = 0;
    * = writeFrameHeader(cbuf[0..], TAG_COMMAND_COMPLETE, t3.len + 1);
    const c3 = decodeCommandComplete(cbuf[0 .. 5 + t3.len + 1]) catch unreachable;
    expect_eq(c3.rowsAffected() == null, true);
});

test("5. Truncated packets, wrong tags, bad lengths & invalid transaction status guards", () => {
    const tiny: [3]u8 = ['Z', 0, 0];
    std.testing.expectError(PgMessageError.UnexpectedEof, decodeReadyForQuery(tiny[0..])) catch unreachable;

    // Wrong tag
    const wrong_tag: [6]u8 = ['X', 0, 0, 0, 5, 'I'];
    std.testing.expectError(PgMessageError.InvalidMessageTag, decodeReadyForQuery(wrong_tag[0..])) catch unreachable;

    // Invalid transaction status byte ('Q')
    const bad_tx: [6]u8 = ['Z', 0, 0, 0, 5, 'Q'];
    std.testing.expectError(PgMessageError.InvalidTransactionStatus, decodeReadyForQuery(bad_tx[0..])) catch unreachable;

    // BackendKeyData wrong length (len=8 instead of 12)
    const bad_bkd: [9]u8 = ['K', 0, 0, 0, 8, 1, 2, 3, 4];
    std.testing.expectError(PgMessageError.InvalidMessageLength, decodeBackendKeyData(bad_bkd[0..])) catch unreachable;

    // ErrorResponse missing terminating NUL
    const no_nul_err: [9]u8 = ['E', 0, 0, 0, 8, 'M', 'b', 'a', 'd'];
    std.testing.expectError(PgMessageError.InvalidNullTerminator, decodeErrorResponse(no_nul_err[0..])) catch unreachable;

    // Length field < 4
    const bad_len: [5]u8 = ['C', 0, 0, 0, 2];
    std.testing.expectError(PgMessageError.InvalidMessageLength, decodeCommandComplete(bad_len[0..])) catch unreachable;
});
