// Source: cp/cap/test/test_smb_production_deep.cp (Production deep tests for cap:smb)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SMB_TCP_PORT,
    SMB2_HEADER_SIZE,
    SMB3_TRANSFORM_HEADER_SIZE,
    NETBIOS_SESSION_HEADER_SIZE,
    SMB2_MAGIC,
    SMB3_TRANSFORM_MAGIC,
    CMD_NEGOTIATE,
    CMD_SESSION_SETUP,
    CMD_LOGOFF,
    CMD_TREE_CONNECT,
    CMD_TREE_DISCONNECT,
    CMD_CREATE,
    CMD_CLOSE,
    CMD_FLUSH,
    CMD_READ,
    CMD_WRITE,
    CMD_LOCK,
    CMD_IOCTL,
    CMD_CANCEL,
    CMD_ECHO,
    CMD_QUERY_DIRECTORY,
    CMD_CHANGE_NOTIFY,
    CMD_QUERY_INFO,
    CMD_SET_INFO,
    CMD_OPLOCK_BREAK,
    FLAG_SERVER_TO_REDIR,
    FLAG_ASYNC_COMMAND,
    FLAG_RELATED_OPERATIONS,
    FLAG_SIGNED,
    DIALECT_202,
    DIALECT_210,
    DIALECT_300,
    DIALECT_302,
    DIALECT_311,
    unwrapNetbiosSession,
    isSmb2,
    isSmb3Transform,
    Smb2Header,
    Smb2Packet,
    parseSmb2,
    Smb2ChainIter,
    iterSmb2Chain,
    Smb2NegotiateReq,
    parseSmb2NegotiateReq,
    Smb2NegotiateResp,
    parseSmb2NegotiateResp,
    Smb3TransformHeader,
    parseSmb3Transform,
} from "cap:smb";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn initSmb2Header(buf: []u8, off: usize, cmd: u16, flags: u32, next_cmd: u32, msg_id: u64) {
    buf[off] = SMB2_MAGIC[0];
    buf[off + 1] = SMB2_MAGIC[1];
    buf[off + 2] = SMB2_MAGIC[2];
    buf[off + 3] = SMB2_MAGIC[3];
    putU16Le(buf, off + 4, 64);
    putU16Le(buf, off + 6, 1); // credit_charge
    putU32Le(buf, off + 8, 0); // status
    putU16Le(buf, off + 12, cmd);
    putU16Le(buf, off + 14, 16); // credits
    putU32Le(buf, off + 16, flags);
    putU32Le(buf, off + 20, next_cmd);
    putU64Le(buf, off + 24, msg_id);
}

test("1. NetBIOS 4B unwrap & 64B Sync/Async SMB2 header parsing", () => {
    expect_eq(SMB_TCP_PORT, 445 as u16);
    expect_eq(SMB2_HEADER_SIZE, 64 as usize);
    expect_eq(SMB3_TRANSFORM_HEADER_SIZE, 52 as usize);
    expect_eq(NETBIOS_SESSION_HEADER_SIZE, 4 as usize);
    expect_eq(CMD_SESSION_SETUP, 0x01 as u16);
    expect_eq(CMD_LOGOFF, 0x02 as u16);
    expect_eq(CMD_TREE_CONNECT, 0x03 as u16);
    expect_eq(CMD_TREE_DISCONNECT, 0x04 as u16);
    expect_eq(CMD_FLUSH, 0x07 as u16);
    expect_eq(CMD_WRITE, 0x09 as u16);
    expect_eq(CMD_LOCK, 0x0A as u16);
    expect_eq(CMD_IOCTL, 0x0B as u16);
    expect_eq(CMD_CANCEL, 0x0C as u16);
    expect_eq(CMD_ECHO, 0x0D as u16);
    expect_eq(CMD_QUERY_DIRECTORY, 0x0E as u16);
    expect_eq(CMD_CHANGE_NOTIFY, 0x0F as u16);
    expect_eq(CMD_QUERY_INFO, 0x10 as u16);
    expect_eq(CMD_SET_INFO, 0x11 as u16);
    expect_eq(CMD_OPLOCK_BREAK, 0x12 as u16);

    // Build NetBIOS (4B) + Sync SMB2 Header (64B) + 4B body = 72B
    let frame: [72]u8 = std.mem.zeroes([72]u8);
    frame[0] = 0x00;
    frame[1] = 0x00;
    frame[2] = 0x00;
    frame[3] = 68; // 64 + 4
    initSmb2Header(frame[0..], 4, CMD_ECHO, FLAG_SIGNED, 0, 42);
    putU32Le(frame[0..], 4 + 32, 0x0000FEFF); // process_id
    putU32Le(frame[0..], 4 + 36, 0x00000007); // tree_id
    putU64Le(frame[0..], 4 + 40, 0x0102030405060708); // session_id
    frame[4 + 48] = 0xAB;
    frame[4 + 64] = 0x04;

    const unwrapped = unwrapNetbiosSession(frame[0..]) ?? unreachable;
    expect_eq(unwrapped.len, 68 as usize);
    expect_eq(isSmb2(unwrapped), true);

    const pkt = parseSmb2(unwrapped) ?? unreachable;
    expect_eq(pkt.header.command, CMD_ECHO);
    expect_eq(pkt.header.credit_charge, 1 as u16);
    expect_eq(pkt.header.credits, 16 as u16);
    expect_eq(pkt.header.message_id, 42 as u64);
    expect_eq(pkt.header.is_async, false);
    expect_eq(pkt.header.process_id, 0x0000FEFF as u32);
    expect_eq(pkt.header.tree_id, 7 as u32);
    expect_eq(pkt.header.session_id, 0x0102030405060708 as u64);
    expect_eq(pkt.header.signature[0], 0xAB as u8);
    expect_eq(pkt.header.isResponse(), false);
    expect_eq(pkt.header.isSigned(), true);
    expect_eq(pkt.header.isRelated(), false);
    expect_eq(pkt.header.isChained(), false);
    expect_eq(pkt.body.len, 4 as usize);

    // Async SMB2 Response Header
    let async_buf: [68]u8 = std.mem.zeroes([68]u8);
    initSmb2Header(async_buf[0..], 0, CMD_CHANGE_NOTIFY, FLAG_SERVER_TO_REDIR | FLAG_ASYNC_COMMAND, 0, 99);
    putU32Le(async_buf[0..], 8, 0x00000103); // STATUS_PENDING
    putU64Le(async_buf[0..], 32, 0x1122334455667788); // async_id
    const apkt = parseSmb2(async_buf[0..]) ?? unreachable;
    expect_eq(apkt.header.is_async, true);
    expect_eq(apkt.header.isResponse(), true);
    expect_eq(apkt.header.status, 0x00000103 as u32);
    expect_eq(apkt.header.async_id, 0x1122334455667788 as u64);
    expect_eq(apkt.header.tree_id, 0 as u32);
});

test("2. SMB2 Negotiate Request (multi-dialect) & Negotiate Response (+ Security Buffer)", () => {
    // Negotiate Request: 64B header + 36B fixed body + 5 dialects (10B) = 110B
    let req_buf: [110]u8 = std.mem.zeroes([110]u8);
    initSmb2Header(req_buf[0..], 0, CMD_NEGOTIATE, 0, 0, 1);
    putU16Le(req_buf[0..], 64 + 0, 36); // StructureSize
    putU16Le(req_buf[0..], 64 + 2, 5); // DialectCount
    putU16Le(req_buf[0..], 64 + 4, 1); // SecurityMode (signing enabled)
    putU32Le(req_buf[0..], 64 + 8, 0x0000007F); // Capabilities
    req_buf[64 + 12] = 0xDE;
    req_buf[64 + 27] = 0xAD;
    putU16Le(req_buf[0..], 64 + 36, DIALECT_202);
    putU16Le(req_buf[0..], 64 + 38, DIALECT_210);
    putU16Le(req_buf[0..], 64 + 40, DIALECT_300);
    putU16Le(req_buf[0..], 64 + 42, DIALECT_302);
    putU16Le(req_buf[0..], 64 + 44, DIALECT_311);

    const req_pkt = parseSmb2(req_buf[0..]) ?? unreachable;
    const neg_req = parseSmb2NegotiateReq(req_pkt) ?? unreachable;
    expect_eq(neg_req.dialect_count, 5 as u16);
    expect_eq(neg_req.security_mode, 1 as u16);
    expect_eq(neg_req.capabilities, 0x0000007F as u32);
    expect_eq(neg_req.client_guid[0], 0xDE as u8);
    expect_eq(neg_req.client_guid[15], 0xAD as u8);
    expect_eq(neg_req.dialectAt(0) ?? 0, DIALECT_202);
    expect_eq(neg_req.dialectAt(2) ?? 0, DIALECT_300);
    expect_eq(neg_req.dialectAt(4) ?? 0, DIALECT_311);
    expect_eq(neg_req.dialectAt(5) == null, true);

    // Negotiate Response: 64B header + 64B fixed body + 6B SPNEGO token = 134B
    let resp_buf: [134]u8 = std.mem.zeroes([134]u8);
    initSmb2Header(resp_buf[0..], 0, CMD_NEGOTIATE, FLAG_SERVER_TO_REDIR, 0, 1);
    putU16Le(resp_buf[0..], 64 + 0, 65); // StructureSize = 65
    putU16Le(resp_buf[0..], 64 + 2, 3); // SecurityMode
    putU16Le(resp_buf[0..], 64 + 4, DIALECT_311);
    resp_buf[64 + 8] = 0xCA;
    resp_buf[64 + 23] = 0xFE;
    putU32Le(resp_buf[0..], 64 + 24, 0x00000047); // Capabilities
    putU32Le(resp_buf[0..], 64 + 28, 8388608); // MaxTransactSize (8MB)
    putU32Le(resp_buf[0..], 64 + 32, 8388608); // MaxReadSize
    putU32Le(resp_buf[0..], 64 + 36, 8388608); // MaxWriteSize
    putU16Le(resp_buf[0..], 64 + 56, 128); // SecurityBufferOffset from SMB2 header start
    putU16Le(resp_buf[0..], 64 + 58, 6); // SecurityBufferLength
    const spnego = "SPNEGO";
    let i: usize = 0;
    while (i < 6) {
        resp_buf[128 + i] = spnego[i];
        i += 1;
    }

    const resp_pkt = parseSmb2(resp_buf[0..]) ?? unreachable;
    const neg_resp = parseSmb2NegotiateResp(resp_pkt) ?? unreachable;
    expect_eq(neg_resp.dialect_revision, DIALECT_311);
    expect_eq(neg_resp.security_mode, 3 as u16);
    expect_eq(neg_resp.server_guid[0], 0xCA as u8);
    expect_eq(neg_resp.server_guid[15], 0xFE as u8);
    expect_eq(neg_resp.max_transact_size, 8388608 as u32);
    expect_eq(neg_resp.max_read_size, 8388608 as u32);
    expect_eq(neg_resp.max_write_size, 8388608 as u32);
    expect_eq(std.mem.eql(u8, neg_resp.security_buffer, "SPNEGO"), true);
});

test("3. Compound/Chained SMB2 multi-message zero-copy iteration", () => {
    // 3 chained packets: [0..80] CREATE, [80..160] READ, [160..232] CLOSE
    let chain: [232]u8 = std.mem.zeroes([232]u8);
    initSmb2Header(chain[0..], 0, CMD_CREATE, 0, 80, 10);
    initSmb2Header(chain[0..], 80, CMD_READ, FLAG_RELATED_OPERATIONS, 80, 11);
    initSmb2Header(chain[0..], 160, CMD_CLOSE, FLAG_RELATED_OPERATIONS, 0, 12);

    let it = iterSmb2Chain(chain[0..]);

    const p0 = it.next() ?? unreachable;
    expect_eq(p0.header.command, CMD_CREATE);
    expect_eq(p0.header.message_id, 10 as u64);
    expect_eq(p0.header.isChained(), true);
    expect_eq(p0.body.len, 16 as usize);

    const p1 = it.next() ?? unreachable;
    expect_eq(p1.header.command, CMD_READ);
    expect_eq(p1.header.message_id, 11 as u64);
    expect_eq(p1.header.isRelated(), true);
    expect_eq(p1.header.isChained(), true);
    expect_eq(p1.body.len, 16 as usize);

    const p2 = it.next() ?? unreachable;
    expect_eq(p2.header.command, CMD_CLOSE);
    expect_eq(p2.header.message_id, 12 as u64);
    expect_eq(p2.header.isRelated(), true);
    expect_eq(p2.header.isChained(), false);
    expect_eq(p2.body.len, 8 as usize);

    expect_eq(it.next() == null, true);
    expect_eq(it.isMalformed(), false);
});

test("4. SMB3 Transform header (0xFD SMB) 52B encrypted frame parsing", () => {
    let tbuf: [84]u8 = std.mem.zeroes([84]u8);
    tbuf[0] = SMB3_TRANSFORM_MAGIC[0];
    tbuf[1] = SMB3_TRANSFORM_MAGIC[1];
    tbuf[2] = SMB3_TRANSFORM_MAGIC[2];
    tbuf[3] = SMB3_TRANSFORM_MAGIC[3];
    tbuf[4] = 0xAA; // signature[0]
    tbuf[19] = 0xBB; // signature[15]
    tbuf[20] = 0xCC; // nonce[0]
    tbuf[35] = 0xDD; // nonce[15]
    putU32Le(tbuf[0..], 36, 32); // orig_msg_size
    putU16Le(tbuf[0..], 42, 0x0001); // Encrypted flag
    putU64Le(tbuf[0..], 44, 0x0000123456789ABC); // session_id
    tbuf[52] = 0x55;
    tbuf[83] = 0x66;

    expect_eq(isSmb3Transform(tbuf[0..]), true);
    expect_eq(isSmb2(tbuf[0..]), false);

    const tr = parseSmb3Transform(tbuf[0..]) ?? unreachable;
    expect_eq(tr.signature[0], 0xAA as u8);
    expect_eq(tr.signature[15], 0xBB as u8);
    expect_eq(tr.nonce[0], 0xCC as u8);
    expect_eq(tr.nonce[15], 0xDD as u8);
    expect_eq(tr.orig_msg_size, 32 as u32);
    expect_eq(tr.flags, 1 as u16);
    expect_eq(tr.session_id, 0x0000123456789ABC as u64);
    expect_eq(tr.encrypted_data.len, 32 as usize);
    expect_eq(tr.encrypted_data[0], 0x55 as u8);
    expect_eq(tr.encrypted_data[31], 0x66 as u8);
});

test("5. Malformed magic, wrong structure_size, unaligned/OOB next_command & truncation rejection", () => {
    // Bad NetBIOS header
    const bad_nb: [8]u8 = [0x81, 0, 0, 4, 1, 2, 3, 4];
    expect_eq(unwrapNetbiosSession(bad_nb[0..]) == null, true);
    const trunc_nb: [6]u8 = [0x00, 0, 0, 10, 1, 2];
    expect_eq(unwrapNetbiosSession(trunc_nb[0..]) == null, true);

    // Wrong SMB2 magic & truncated header
    let bad_smb: [64]u8 = std.mem.zeroes([64]u8);
    bad_smb[0] = 0xFF;
    bad_smb[1] = 'S';
    bad_smb[2] = 'M';
    bad_smb[3] = 'B';
    putU16Le(bad_smb[0..], 4, 64);
    expect_eq(parseSmb2(bad_smb[0..]) == null, true);
    expect_eq(parseSmb2(bad_smb[0..32]) == null, true);

    // Wrong structure_size != 64
    initSmb2Header(bad_smb[0..], 0, CMD_ECHO, 0, 0, 1);
    putU16Le(bad_smb[0..], 4, 62);
    expect_eq(parseSmb2(bad_smb[0..]) == null, true);

    // Unaligned next_command (65 is not 8-byte aligned) & OOB next_command
    let ch_bad: [80]u8 = std.mem.zeroes([80]u8);
    initSmb2Header(ch_bad[0..], 0, CMD_ECHO, 0, 65, 1);
    expect_eq(parseSmb2(ch_bad[0..]) == null, true);

    initSmb2Header(ch_bad[0..], 0, CMD_ECHO, 0, 128, 1);
    expect_eq(parseSmb2(ch_bad[0..]) == null, true);

    // Chain iterator flags malformed when next_command points exactly to EOF with no next packet
    initSmb2Header(ch_bad[0..], 0, CMD_ECHO, 0, 80, 1);
    let bad_it = iterSmb2Chain(ch_bad[0..]);
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.isMalformed(), true);

    // Truncated SMB3 Transform (orig_msg_size > remaining bytes)
    let bad_tr: [60]u8 = std.mem.zeroes([60]u8);
    bad_tr[0] = 0xFD;
    bad_tr[1] = 'S';
    bad_tr[2] = 'M';
    bad_tr[3] = 'B';
    putU32Le(bad_tr[0..], 36, 100);
    expect_eq(parseSmb3Transform(bad_tr[0..]) == null, true);
});
