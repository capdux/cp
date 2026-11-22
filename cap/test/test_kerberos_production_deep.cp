// Source: cp/cap/test/test_kerberos_production_deep.cp (Production deep tests for cap:kerberos)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KRB_PORT,
    KRB_VERSION,
    MSG_AS_REQ,
    MSG_AS_REP,
    MSG_TGS_REQ,
    MSG_TGS_REP,
    MSG_AP_REQ,
    MSG_AP_REP,
    MSG_KRB_SAFE,
    MSG_KRB_PRIV,
    MSG_KRB_CRED,
    MSG_KRB_ERROR,
    ETYPE_AES128_CTS_HMAC_SHA1,
    ETYPE_AES256_CTS_HMAC_SHA1,
    ETYPE_RC4_HMAC,
    NT_UNKNOWN,
    NT_PRINCIPAL,
    NT_SRV_INST,
    NT_SRV_HST,
    NT_ENTERPRISE,
    KDC_ERR_NONE,
    KDC_ERR_C_PRINCIPAL_UNKNOWN,
    KDC_ERR_S_PRINCIPAL_UNKNOWN,
    KDC_ERR_PREAUTH_FAILED,
    KDC_ERR_PREAUTH_REQUIRED,
    KRB_AP_ERR_TKT_EXPIRED,
    KRB_AP_ERR_SKEW,
    KrbStringIter,
    KrbPrincipalName,
    KrbEncryptedData,
    KrbTicket,
    KrbMessageInfo,
    KrbError,
    unwrapKrbTcpRecord,
    peekKrbAppTag,
    isKerberos,
    parseKrbMessageInfo,
    parseKrbTicket,
    parseKrbError,
} from "cap:auth";

// Minimal zero-alloc DER test builder (supports short < 128 and 1-byte long 128..255 lengths)
fn writeDerTlv(out: []u8, pos: usize, tag: u8, payload: []const u8): usize {
    let p = pos;
    out[p] = tag;
    p += 1;
    if (payload.len < 128) {
        out[p] = payload.len as u8;
        p += 1;
    } else {
        out[p] = 0x81;
        out[p + 1] = payload.len as u8;
        p += 2;
    }
    let i: usize = 0;
    while (i < payload.len) {
        out[p + i] = payload[i];
        i += 1;
    }
    return p + payload.len;
}

fn buildIntTlv(out: []u8, val: u8): usize {
    const v: [1]u8 = [val];
    return writeDerTlv(out, 0, 0x02, v[0..]);
}

fn buildCtxInt(out: []u8, pos: usize, ctx: u8, val: u8): usize {
    let tmp: [4]u8 = std.mem.zeroes([4]u8);
    const n = buildIntTlv(tmp[0..], val);
    return writeDerTlv(out, pos, 0xA0 | ctx, tmp[0..n]);
}

fn buildCtxStr(out: []u8, pos: usize, ctx: u8, str_tag: u8, s: []const u8): usize {
    let tmp: [64]u8 = std.mem.zeroes([64]u8);
    const n = writeDerTlv(tmp[0..], 0, str_tag, s);
    return writeDerTlv(out, pos, 0xA0 | ctx, tmp[0..n]);
}

fn buildPrincipalName(out: []u8, name_type: u8, comp0: []const u8, comp1: []const u8): usize {
    let names: [64]u8 = std.mem.zeroes([64]u8);
    let np: usize = 0;
    np = writeDerTlv(names[0..], np, 0x1B, comp0);
    if (comp1.len > 0) {
        np = writeDerTlv(names[0..], np, 0x1B, comp1);
    }
    let names_seq: [72]u8 = std.mem.zeroes([72]u8);
    const nsn = writeDerTlv(names_seq[0..], 0, 0x30, names[0..np]);

    let body: [96]u8 = std.mem.zeroes([96]u8);
    let bp: usize = 0;
    bp = buildCtxInt(body[0..], bp, 0, name_type);
    bp = writeDerTlv(body[0..], bp, 0xA1, names_seq[0..nsn]);
    return writeDerTlv(out, 0, 0x30, body[0..bp]);
}

fn buildTicketDer(out: []u8, tkt_vno: u8, with_kvno: bool): usize {
    // Build PrincipalName: krbtgt / EXAMPLE.COM (NT_SRV_INST = 2)
    let pname: [100]u8 = std.mem.zeroes([100]u8);
    const pn_len = buildPrincipalName(pname[0..], 2, "krbtgt", "EXAMPLE.COM");

    // Build EncryptedData: etype=18, optional kvno=3, cipher="CIPHER"
    let enc_body: [64]u8 = std.mem.zeroes([64]u8);
    let ep: usize = 0;
    ep = buildCtxInt(enc_body[0..], ep, 0, 18);
    if (with_kvno) {
        ep = buildCtxInt(enc_body[0..], ep, 1, 3);
    }
    ep = buildCtxStr(enc_body[0..], ep, 2, 0x04, "CIPHER");
    let enc_seq: [72]u8 = std.mem.zeroes([72]u8);
    const enc_len = writeDerTlv(enc_seq[0..], 0, 0x30, enc_body[0..ep]);

    // Ticket SEQUENCE
    let tkt_body: [200]u8 = std.mem.zeroes([200]u8);
    let tp: usize = 0;
    tp = buildCtxInt(tkt_body[0..], tp, 0, tkt_vno);
    tp = buildCtxStr(tkt_body[0..], tp, 1, 0x1B, "EXAMPLE.COM");
    tp = writeDerTlv(tkt_body[0..], tp, 0xA2, pname[0..pn_len]);
    tp = writeDerTlv(tkt_body[0..], tp, 0xA3, enc_seq[0..enc_len]);

    let tkt_seq: [220]u8 = std.mem.zeroes([220]u8);
    const seq_len = writeDerTlv(tkt_seq[0..], 0, 0x30, tkt_body[0..tp]);
    return writeDerTlv(out, 0, 0x61, tkt_seq[0..seq_len]);
}

test("1. Kerberos constants & TCP 4B Record Mark unwrapping", () => {
    expect_eq(KRB_PORT, 88 as u16);
    expect_eq(KRB_VERSION, 5 as u32);
    expect_eq(MSG_AS_REQ, 10 as u8);
    expect_eq(MSG_AS_REP, 11 as u8);
    expect_eq(MSG_TGS_REQ, 12 as u8);
    expect_eq(MSG_TGS_REP, 13 as u8);
    expect_eq(MSG_AP_REQ, 14 as u8);
    expect_eq(MSG_AP_REP, 15 as u8);
    expect_eq(MSG_KRB_SAFE, 20 as u8);
    expect_eq(MSG_KRB_PRIV, 21 as u8);
    expect_eq(MSG_KRB_CRED, 22 as u8);
    expect_eq(MSG_KRB_ERROR, 30 as u8);
    expect_eq(ETYPE_AES128_CTS_HMAC_SHA1, 17 as i32);
    expect_eq(ETYPE_AES256_CTS_HMAC_SHA1, 18 as i32);
    expect_eq(ETYPE_RC4_HMAC, 23 as i32);
    expect_eq(NT_UNKNOWN, 0 as i32);
    expect_eq(NT_PRINCIPAL, 1 as i32);
    expect_eq(NT_SRV_HST, 3 as i32);
    expect_eq(NT_ENTERPRISE, 10 as i32);
    expect_eq(KDC_ERR_NONE, 0 as i32);
    expect_eq(KDC_ERR_C_PRINCIPAL_UNKNOWN, 6 as i32);
    expect_eq(KDC_ERR_S_PRINCIPAL_UNKNOWN, 7 as i32);
    expect_eq(KDC_ERR_PREAUTH_FAILED, 24 as i32);
    expect_eq(KRB_AP_ERR_TKT_EXPIRED, 32 as i32);
    expect_eq(KRB_AP_ERR_SKEW, 37 as i32);

    let tkt_der: [240]u8 = std.mem.zeroes([240]u8);
    const tkt_len = buildTicketDer(tkt_der[0..], 5, true);

    let tcp_buf: [256]u8 = std.mem.zeroes([256]u8);
    tcp_buf[0] = 0;
    tcp_buf[1] = 0;
    tcp_buf[2] = ((tkt_len >> 8) & 0xFF) as u8;
    tcp_buf[3] = (tkt_len & 0xFF) as u8;
    let i: usize = 0;
    while (i < tkt_len) {
        tcp_buf[4 + i] = tkt_der[i];
        i += 1;
    }

    const unwrapped = unwrapKrbTcpRecord(tcp_buf[0..(4 + tkt_len)]) ?? unreachable;
    expect_eq(unwrapped.len, tkt_len);
    expect_eq(isKerberos(unwrapped), true);
    expect_eq(peekKrbAppTag(unwrapped) ?? 0, 1 as u8);
});

test("2. APPLICATION [1] Ticket parsing with PrincipalName iteration & optional kvno", () => {
    // With kvno
    let der1: [240]u8 = std.mem.zeroes([240]u8);
    const n1 = buildTicketDer(der1[0..], 5, true);
    const tkt1 = parseKrbTicket(der1[0..n1]) ?? unreachable;
    expect_eq(tkt1.tkt_vno, 5 as u32);
    expect_eq(std.mem.eql(u8, tkt1.realm, "EXAMPLE.COM"), true);
    expect_eq(tkt1.sname.name_type, NT_SRV_INST);

    let it = tkt1.sname.names();
    const c0 = it.next() ?? unreachable;
    const c1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, c0, "krbtgt"), true);
    expect_eq(std.mem.eql(u8, c1, "EXAMPLE.COM"), true);
    expect_eq(it.next() == null, true);

    expect_eq(tkt1.enc_part.etype, ETYPE_AES256_CTS_HMAC_SHA1);
    expect_eq(tkt1.enc_part.kvno ?? 0, 3 as u32);
    expect_eq(std.mem.eql(u8, tkt1.enc_part.cipher, "CIPHER"), true);

    // Without kvno
    let der2: [240]u8 = std.mem.zeroes([240]u8);
    const n2 = buildTicketDer(der2[0..], 5, false);
    const tkt2 = parseKrbTicket(der2[0..n2]) ?? unreachable;
    expect_eq(tkt2.enc_part.kvno == null, true);
    expect_eq(std.mem.eql(u8, tkt2.enc_part.cipher, "CIPHER"), true);
});

test("3. APPLICATION [30] KRB-ERROR parsing (PREAUTH_REQUIRED, optional ctime/crealm, e-text & e-data)", () => {
    let sname: [100]u8 = std.mem.zeroes([100]u8);
    const sn_len = buildPrincipalName(sname[0..], 2, "krbtgt", "EXAMPLE.COM");

    let body: [240]u8 = std.mem.zeroes([240]u8);
    let p: usize = 0;
    p = buildCtxInt(body[0..], p, 0, 5); // [0] pvno = 5
    p = buildCtxInt(body[0..], p, 1, MSG_KRB_ERROR); // [1] msg-type = 30
    p = buildCtxStr(body[0..], p, 2, 0x18, "20261122080000Z"); // [2] optional ctime
    p = buildCtxStr(body[0..], p, 4, 0x18, "20261122080001Z"); // [4] stime
    p = buildCtxInt(body[0..], p, 5, 42); // [5] susec
    p = buildCtxInt(body[0..], p, 6, 25); // [6] error-code = KDC_ERR_PREAUTH_REQUIRED
    p = buildCtxStr(body[0..], p, 7, 0x1B, "CLIENT.COM"); // [7] optional crealm
    p = buildCtxStr(body[0..], p, 9, 0x1B, "EXAMPLE.COM"); // [9] realm
    p = writeDerTlv(body[0..], p, 0xAA, sname[0..sn_len]); // [10] sname
    p = buildCtxStr(body[0..], p, 11, 0x1B, "NEEDED_PREAUTH"); // [11] e-text
    p = buildCtxStr(body[0..], p, 12, 0x04, "PADATA"); // [12] e-data

    let seq: [250]u8 = std.mem.zeroes([250]u8);
    const seq_len = writeDerTlv(seq[0..], 0, 0x30, body[0..p]);
    let msg: [260]u8 = std.mem.zeroes([260]u8);
    const msg_len = writeDerTlv(msg[0..], 0, 0x60 | MSG_KRB_ERROR, seq[0..seq_len]);

    const err = parseKrbError(msg[0..msg_len]) ?? unreachable;
    expect_eq(err.pvno, 5 as u32);
    expect_eq(err.msg_type, 30 as u32);
    expect_eq(std.mem.eql(u8, err.stime, "20261122080001Z"), true);
    expect_eq(err.susec, 42 as u32);
    expect_eq(err.error_code, KDC_ERR_PREAUTH_REQUIRED);
    expect_eq(std.mem.eql(u8, err.realm, "EXAMPLE.COM"), true);
    expect_eq(std.mem.eql(u8, err.e_text, "NEEDED_PREAUTH"), true);
    expect_eq(std.mem.eql(u8, err.e_data, "PADATA"), true);

    let sit = err.sname.names();
    expect_eq(std.mem.eql(u8, sit.next() ?? unreachable, "krbtgt"), true);
    expect_eq(std.mem.eql(u8, sit.next() ?? unreachable, "EXAMPLE.COM"), true);
});

test("4. AS-REQ ([1]/[2] KDC-REQ layout) & TGS-REP ([0]/[1] layout) generic header probing", () => {
    // AS-REQ (APPLICATION 10) with RFC 4120 [1] pvno=5, [2] msg-type=10
    let as_body: [32]u8 = std.mem.zeroes([32]u8);
    let ap: usize = 0;
    ap = buildCtxInt(as_body[0..], ap, 1, 5);
    ap = buildCtxInt(as_body[0..], ap, 2, MSG_AS_REQ);
    let as_seq: [40]u8 = std.mem.zeroes([40]u8);
    const as_seq_len = writeDerTlv(as_seq[0..], 0, 0x30, as_body[0..ap]);
    let as_msg: [48]u8 = std.mem.zeroes([48]u8);
    const as_len = writeDerTlv(as_msg[0..], 0, 0x60 | MSG_AS_REQ, as_seq[0..as_seq_len]);

    const as_info = parseKrbMessageInfo(as_msg[0..as_len]) ?? unreachable;
    expect_eq(as_info.app_tag, MSG_AS_REQ);
    expect_eq(as_info.pvno, 5 as u32);
    expect_eq(as_info.msg_type, 10 as u32);
    expect_eq(as_info.seq_body.len, ap);

    // TGS-REP (APPLICATION 13) with [0] pvno=5, [1] msg-type=13
    let tgs_body: [32]u8 = std.mem.zeroes([32]u8);
    let tp: usize = 0;
    tp = buildCtxInt(tgs_body[0..], tp, 0, 5);
    tp = buildCtxInt(tgs_body[0..], tp, 1, MSG_TGS_REP);
    let tgs_seq: [40]u8 = std.mem.zeroes([40]u8);
    const tgs_seq_len = writeDerTlv(tgs_seq[0..], 0, 0x30, tgs_body[0..tp]);
    let tgs_msg: [48]u8 = std.mem.zeroes([48]u8);
    const tgs_len = writeDerTlv(tgs_msg[0..], 0, 0x60 | MSG_TGS_REP, tgs_seq[0..tgs_seq_len]);

    const tgs_info = parseKrbMessageInfo(tgs_msg[0..tgs_len]) ?? unreachable;
    expect_eq(tgs_info.app_tag, MSG_TGS_REP);
    expect_eq(tgs_info.pvno, 5 as u32);
    expect_eq(tgs_info.msg_type, 13 as u32);
});

test("5. Non-v5 version, DER truncation/indefinite length, bad tags & malformed TCP mark rejection", () => {
    // TCP Record Mark with MSB=1 or truncated payload
    const bad_msb: [8]u8 = [0x80, 0, 0, 4, 1, 2, 3, 4];
    expect_eq(unwrapKrbTcpRecord(bad_msb[0..]) == null, true);
    const trunc_tcp: [6]u8 = [0, 0, 0, 10, 1, 2];
    expect_eq(unwrapKrbTcpRecord(trunc_tcp[0..]) == null, true);

    // Ticket with wrong version (tkt_vno = 4 instead of 5)
    let bad_vno: [240]u8 = std.mem.zeroes([240]u8);
    const bv_len = buildTicketDer(bad_vno[0..], 4, false);
    expect_eq(parseKrbTicket(bad_vno[0..bv_len]) == null, true);

    // Truncated DER payload
    expect_eq(parseKrbTicket(bad_vno[0..10]) == null, true);

    // Indefinite BER length (0x80) must be rejected
    const indef: [4]u8 = [0x61, 0x80, 0x00, 0x00];
    expect_eq(peekKrbAppTag(indef[0..]) == null, true);
    expect_eq(isKerberos(indef[0..]), false);

    // Unknown application tag (APPLICATION [7] = 0x67)
    const bad_app: [4]u8 = [0x67, 0x02, 0x30, 0x00];
    expect_eq(peekKrbAppTag(bad_app[0..]) == null, true);

    // KRB-ERROR missing required stime/realm/sname
    let incomplete_body: [32]u8 = std.mem.zeroes([32]u8);
    let ip: usize = 0;
    ip = buildCtxInt(incomplete_body[0..], ip, 0, 5);
    ip = buildCtxInt(incomplete_body[0..], ip, 1, MSG_KRB_ERROR);
    let inc_seq: [40]u8 = std.mem.zeroes([40]u8);
    const inc_seq_len = writeDerTlv(inc_seq[0..], 0, 0x30, incomplete_body[0..ip]);
    let inc_msg: [48]u8 = std.mem.zeroes([48]u8);
    const inc_len = writeDerTlv(inc_msg[0..], 0, 0x60 | MSG_KRB_ERROR, inc_seq[0..inc_seq_len]);
    expect_eq(parseKrbError(inc_msg[0..inc_len]) == null, true);
});
