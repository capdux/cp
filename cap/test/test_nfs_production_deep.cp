// Source: cp/cap/test/test_nfs_production_deep.cp (Production deep tests for cap:nfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NFS_PORT,
    RPC_VERSION,
    RPC_CALL,
    RPC_REPLY,
    RPC_MAX_AUTH_LEN,
    RPC_CALL_NONE_HEADER_SIZE,
    RPC_PROG_PORTMAP,
    RPC_PROG_NFS,
    RPC_PROG_MOUNT,
    NFS_V3,
    NFS_V4,
    NFS3_FHSIZE,
    NFS3_FATTR_SIZE,
    AUTH_NONE,
    AUTH_SYS,
    MSG_ACCEPTED,
    MSG_DENIED,
    ACCEPT_SUCCESS,
    ACCEPT_PROC_UNAVAIL,
    NFS3_GETATTR,
    NFS3_READ,
    NFS3_OK,
    FTYPE_REG,
    FTYPE_DIR,
    RpcRecordFrag,
    parseRpcRecordFrag,
    encodeRpcRecordMark,
    RpcAuth,
    RpcCall,
    RpcReply,
    parseRpcCall,
    parseRpcReply,
    encodeRpcCallHeader,
    Nfs3Fattr,
    parseNfs3FileHandle,
    parseNfs3Fattr,
} from "cap:nfs";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn writeU32BeTest(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn writeU64BeTest(buf: []u8, off: usize, val: u64) {
    writeU32BeTest(buf, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    writeU32BeTest(buf, off + 4, (val & 0xFFFFFFFF) as u32);
}

test("1. TCP Record Marking multi-fragment parsing and encodeRpcRecordMark round-trip", () => {
    expect_eq(NFS_PORT, 2049 as u16);
    expect_eq(RPC_VERSION, 2 as u32);
    expect_eq(RPC_PROG_NFS, 100003 as u32);
    expect_eq(NFS3_FATTR_SIZE, 84);

    // Two back-to-back fragments: frag1 (last=false, len=5 "hello"), frag2 (last=true, len=3 "nfs")
    let stream: [16]u8 = [0; 16];
    expect_eq(encodeRpcRecordMark(false, 5, stream[0..4]) ?? 0, 4);
    stream[4] = 'h';
    stream[5] = 'e';
    stream[6] = 'l';
    stream[7] = 'l';
    stream[8] = 'o';
    expect_eq(encodeRpcRecordMark(true, 3, stream[9..13]) ?? 0, 4);
    stream[13] = 'n';
    stream[14] = 'f';
    stream[15] = 's';

    const f1 = parseRpcRecordFrag(stream[0..]) ?? unreachable;
    expect_eq(f1.is_last, false);
    expect_eq(f1.consumed, 9);
    expectBytesEq(f1.payload, "hello");

    const f2 = parseRpcRecordFrag(stream[f1.consumed..]) ?? unreachable;
    expect_eq(f2.is_last, true);
    expect_eq(f2.consumed, 7);
    expectBytesEq(f2.payload, "nfs");
});

test("2. ONC RPC v2 Call with AUTH_SYS and encodeRpcCallHeader round-trip", () => {
    // 40-byte AUTH_NONE Call header round-trip
    let hdr40: [44]u8 = [0; 44];
    const hlen = encodeRpcCallHeader(0x11223344, RPC_PROG_NFS, NFS_V3, NFS3_GETATTR, hdr40[0..]) ?? unreachable;
    expect_eq(hlen, RPC_CALL_NONE_HEADER_SIZE);
    writeU32BeTest(hdr40[0..], 40, 0xCAFEBABE);

    const call0 = parseRpcCall(hdr40[0..]) ?? unreachable;
    expect_eq(call0.xid, 0x11223344 as u32);
    expect_eq(call0.rpcvers, RPC_VERSION);
    expect_eq(call0.prog, RPC_PROG_NFS);
    expect_eq(call0.vers, NFS_V3);
    expect_eq(call0.proc, NFS3_GETATTR);
    expect_eq(call0.cred.flavor, AUTH_NONE);
    expect_eq(call0.cred.body.len, 0);
    expect_eq(call0.verf.flavor, AUTH_NONE);
    expect_eq(call0.params.len, 4);

    // RPC Call with AUTH_SYS cred (5 bytes + 3 zero pad) + params ("data")
    let buf: [64]u8 = [0; 64];
    writeU32BeTest(buf[0..], 0, 0x99887766);
    writeU32BeTest(buf[0..], 4, RPC_CALL);
    writeU32BeTest(buf[0..], 8, RPC_VERSION);
    writeU32BeTest(buf[0..], 12, RPC_PROG_NFS);
    writeU32BeTest(buf[0..], 16, NFS_V3);
    writeU32BeTest(buf[0..], 20, NFS3_READ);
    // cred: AUTH_SYS, len=5 ("sys01") + 3B zero pad
    writeU32BeTest(buf[0..], 24, AUTH_SYS);
    writeU32BeTest(buf[0..], 28, 5);
    buf[32] = 's';
    buf[33] = 'y';
    buf[34] = 's';
    buf[35] = '0';
    buf[36] = '1';
    // verf: AUTH_NONE, len=0 at offset 40
    writeU32BeTest(buf[0..], 40, AUTH_NONE);
    writeU32BeTest(buf[0..], 44, 0);
    // params at offset 48..52
    buf[48] = 'd';
    buf[49] = 'a';
    buf[50] = 't';
    buf[51] = 'a';

    const call_sys = parseRpcCall(buf[0..52]) ?? unreachable;
    expect_eq(call_sys.cred.flavor, AUTH_SYS);
    expectBytesEq(call_sys.cred.body, "sys01");
    expectBytesEq(call_sys.params, "data");
});

test("3. ONC RPC v2 Reply parsing for MSG_ACCEPTED (success & error) and MSG_DENIED", () => {
    // MSG_ACCEPTED + ACCEPT_SUCCESS + 4B result
    let ok_reply: [28]u8 = [0; 28];
    writeU32BeTest(ok_reply[0..], 0, 0x55443322);
    writeU32BeTest(ok_reply[0..], 4, RPC_REPLY);
    writeU32BeTest(ok_reply[0..], 8, MSG_ACCEPTED);
    writeU32BeTest(ok_reply[0..], 12, AUTH_NONE);
    writeU32BeTest(ok_reply[0..], 16, 0);
    writeU32BeTest(ok_reply[0..], 20, ACCEPT_SUCCESS);
    writeU32BeTest(ok_reply[0..], 24, NFS3_OK);

    const r_ok = parseRpcReply(ok_reply[0..]) ?? unreachable;
    expect_eq(r_ok.xid, 0x55443322 as u32);
    expect_eq(r_ok.isSuccess(), true);
    expect_eq(r_ok.results.len, 4);

    // MSG_ACCEPTED + ACCEPT_PROC_UNAVAIL
    writeU32BeTest(ok_reply[0..], 20, ACCEPT_PROC_UNAVAIL);
    const r_unavail = parseRpcReply(ok_reply[0..24]) ?? unreachable;
    expect_eq(r_unavail.isSuccess(), false);
    expect_eq(r_unavail.accept_stat, ACCEPT_PROC_UNAVAIL);

    // MSG_DENIED + reject_stat=1 (AUTH_ERROR) + 4B auth_stat
    let denied: [20]u8 = [0; 20];
    writeU32BeTest(denied[0..], 0, 0x10000001);
    writeU32BeTest(denied[0..], 4, RPC_REPLY);
    writeU32BeTest(denied[0..], 8, MSG_DENIED);
    writeU32BeTest(denied[0..], 12, 1); // AUTH_ERROR
    writeU32BeTest(denied[0..], 16, 2); // AUTH_REJECTEDCRED

    const r_den = parseRpcReply(denied[0..]) ?? unreachable;
    expect_eq(r_den.isSuccess(), false);
    expect_eq(r_den.reply_stat, MSG_DENIED);
    expect_eq(r_den.reject_stat, 1 as u32);
    expect_eq(r_den.results.len, 4);
});

test("4. NFSv3 file handle (nfs_fh3) and 84-byte fattr3 parsing", () => {
    // 10-byte file handle + 2B zero padding = 16 bytes total
    let fh_wire: [16]u8 = [0; 16];
    writeU32BeTest(fh_wire[0..], 0, 10);
    let i: usize = 0;
    while (i < 10) {
        fh_wire[4 + i] = (0xA0 + i) as u8;
        i += 1;
    }
    const fh = parseNfs3FileHandle(fh_wire[0..]) ?? unreachable;
    expect_eq(fh.len, 10);
    expect_eq(fh[0], 0xA0 as u8);
    expect_eq(fh[9], 0xA9 as u8);

    // 84-byte fattr3
    let fattr_wire: [84]u8 = [0; 84];
    writeU32BeTest(fattr_wire[0..], 0, FTYPE_REG);
    writeU32BeTest(fattr_wire[0..], 4, 0x01A4); // 0644
    writeU32BeTest(fattr_wire[0..], 8, 2);      // nlink
    writeU32BeTest(fattr_wire[0..], 12, 1000);  // uid
    writeU32BeTest(fattr_wire[0..], 16, 1000);  // gid
    writeU64BeTest(fattr_wire[0..], 20, 4096);  // size
    writeU64BeTest(fattr_wire[0..], 28, 8192);  // used
    writeU32BeTest(fattr_wire[0..], 36, 8);     // rdev_major
    writeU32BeTest(fattr_wire[0..], 40, 1);     // rdev_minor
    writeU64BeTest(fattr_wire[0..], 44, 0x1122334455667788); // fsid
    writeU64BeTest(fattr_wire[0..], 52, 987654321);          // fileid
    writeU32BeTest(fattr_wire[0..], 60, 1700000001);         // atime_sec
    writeU32BeTest(fattr_wire[0..], 64, 111);                // atime_nsec
    writeU32BeTest(fattr_wire[0..], 68, 1700000002);         // mtime_sec
    writeU32BeTest(fattr_wire[0..], 72, 222);                // mtime_nsec
    writeU32BeTest(fattr_wire[0..], 76, 1700000003);         // ctime_sec
    writeU32BeTest(fattr_wire[0..], 80, 333);                // ctime_nsec

    const fa = parseNfs3Fattr(fattr_wire[0..]) ?? unreachable;
    expect_eq(fa.ftype, FTYPE_REG);
    expect_eq(fa.mode, 0x01A4 as u32);
    expect_eq(fa.nlink, 2 as u32);
    expect_eq(fa.uid, 1000 as u32);
    expect_eq(fa.gid, 1000 as u32);
    expect_eq(fa.size, 4096 as u64);
    expect_eq(fa.used, 8192 as u64);
    expect_eq(fa.rdev_major, 8 as u32);
    expect_eq(fa.rdev_minor, 1 as u32);
    expect_eq(fa.fsid, 0x1122334455667788 as u64);
    expect_eq(fa.fileid, 987654321 as u64);
    expect_eq(fa.atime_sec, 1700000001 as u32);
    expect_eq(fa.mtime_nsec, 222 as u32);
    expect_eq(fa.ctime_nsec, 333 as u32);
});

test("5. Malformed RPC/NFS rejection (rpcvers!=2, auth>400B, fh>64B, non-zero pad, truncation)", () => {
    // 1. Wrong rpcvers (3 instead of 2)
    let bad_ver: [40]u8 = [0; 40];
    * = encodeRpcCallHeader(1, RPC_PROG_NFS, NFS_V3, 0, bad_ver[0..]);
    writeU32BeTest(bad_ver[0..], 8, 3);
    expect_eq(parseRpcCall(bad_ver[0..]) == null, true);

    // 2. Auth body > 400 bytes rejected
    let big_auth: [450]u8 = [0; 450];
    writeU32BeTest(big_auth[0..], 4, RPC_CALL);
    writeU32BeTest(big_auth[0..], 8, RPC_VERSION);
    writeU32BeTest(big_auth[0..], 24, AUTH_SYS);
    writeU32BeTest(big_auth[0..], 28, 404);
    expect_eq(parseRpcCall(big_auth[0..]) == null, true);

    // 3. NFSv3 file handle > 64 bytes rejected
    let big_fh: [80]u8 = [0; 80];
    writeU32BeTest(big_fh[0..], 0, 65);
    expect_eq(parseNfs3FileHandle(big_fh[0..]) == null, true);

    // 4. Non-zero XDR pad byte in file handle rejected
    let bad_fh_pad: [8]u8 = [0, 0, 0, 1, 0x55, 0, 0, 1];
    expect_eq(parseNfs3FileHandle(bad_fh_pad[0..]) == null, true);

    // 5. Truncated fattr3 (< 84B), truncated record fragment, small encoder buffer
    let short_fa: [83]u8 = [0; 83];
    expect_eq(parseNfs3Fattr(short_fa[0..]) == null, true);
    let trunc_rm: [6]u8 = [0x80, 0, 0, 10, 1, 2];
    expect_eq(parseRpcRecordFrag(trunc_rm[0..]) == null, true);
    let tiny: [20]u8 = [0; 20];
    expect_eq(encodeRpcCallHeader(1, 2, 3, 4, tiny[0..]) == null, true);
});
