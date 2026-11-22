// Source: cp/cap/test/test_tacacs_production_deep.cp
// Deep Production Test Suite for cap:tacacs
// Covers:
//   1. 12B TACACS+ header, flags (Unencrypted / SingleConnect) & Authen START + REPLY parsing
//   2. Authorization REQUEST parsing + TacacsArgIter iterating multiple length-prefixed args
//   3. Authorization RESPONSE parsing + TacacsArgIter iterating reply args
//   4. Accounting REQUEST (+ TacacsArgIter) & Accounting REPLY parsing
//   5. Strict header guards: major!=0xC, invalid pkt_type (0/4), seq_no==0, truncated & oversized length
//   6. Malformed & truncated body guards across all 6 body parsers

import { test, expect_eq } from "cap:test";
import {
    TACACS_TCP_PORT,
    TACACS_HEADER_SIZE,
    TACACS_MAJOR_VER,
    TACACS_MINOR_VER_DEFAULT,
    TACACS_MINOR_VER_ONE,
    TAC_PLUS_UNENCRYPTED_FLAG,
    TAC_PLUS_SINGLE_CONNECT_FLAG,
    TacacsType,
    TacacsAuthenStatus,
    TacacsAuthorStatus,
    TacacsAcctStatus,
    TacacsHeader,
    TacacsPacket,
    TacacsArgIter,
    TacacsAuthenStart,
    TacacsAuthenReply,
    TacacsAuthorRequest,
    TacacsAuthorResponse,
    TacacsAcctRequest,
    TacacsAcctReply,
    parseTacacs,
    isTacacs,
    parseAuthenStart,
    parseAuthenReply,
    parseAuthorRequest,
    parseAuthorResponse,
    parseAcctRequest,
    parseAcctReply,
} from "cap:auth";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("tacacs: 12B header + flags + Authentication START & REPLY") {
    expect_eq(TACACS_TCP_PORT, 49);
    expect_eq(TACACS_HEADER_SIZE, 12);
    expect_eq(TACACS_MAJOR_VER, 0xC);

    // Authen START body: action=1, priv=15, type=1(ASCII), svc=1(LOGIN), user="alice"(5), port="tty0"(4), rem="10.0.0.1"(8), data="pw"(2)
    // body len = 8 + 5 + 4 + 8 + 2 = 27
    let pkt: [39]u8 = [
        // 12B Header: ver=0xC0 (12.0), type=1(authen), seq=1, flags=UNENC|SINGLE(0x05), sess=0x11223344, len=27
        0xC0, 0x01, 0x01, TAC_PLUS_UNENCRYPTED_FLAG | TAC_PLUS_SINGLE_CONNECT_FLAG,
        0x11, 0x22, 0x33, 0x44,
        0x00, 0x00, 0x00, 27,
        // Authen START fixed 8B
        0x01, 0x0F, 0x01, 0x01, 5, 4, 8, 2,
        // "alice"
        0x61, 0x6C, 0x69, 0x63, 0x65,
        // "tty0"
        0x74, 0x74, 0x79, 0x30,
        // "10.0.0.1"
        0x31, 0x30, 0x2E, 0x30, 0x2E, 0x30, 0x2E, 0x31,
        // "pw"
        0x70, 0x77,
    ];

    expect_eq(isTacacs(pkt[0..39]), true);
    if (p := parseTacacs(pkt[0..39])) {
        expect_eq(p.header.major_version, 0xC);
        expect_eq(p.header.minor_version, TACACS_MINOR_VER_DEFAULT);
        expect_eq(p.header.pkt_type == TacacsType.authen, true);
        expect_eq(p.header.seq_no, 1);
        expect_eq(p.header.isUnencrypted(), true);
        expect_eq(p.header.isSingleConnect(), true);
        expect_eq(p.header.session_id, 0x11223344);
        expect_eq(p.header.length, 27);

        if (st := parseAuthenStart(p.body)) {
            expect_eq(st.action, 1);
            expect_eq(st.priv_lvl, 15);
            expect_eq(st.authen_type, 1);
            expect_eq(st.authen_service, 1);
            expect_eq(strEq(st.user, "alice"), true);
            expect_eq(strEq(st.port, "tty0"), true);
            expect_eq(strEq(st.rem_addr, "10.0.0.1"), true);
            expect_eq(strEq(st.data, "pw"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Authen REPLY: status=PASS(1), flags=0, msg="Welcome"(7), data="OK"(2)
    let rep_body: [15]u8 = [
        0x01, 0x00, 0x00, 0x07, 0x00, 0x02,
        0x57, 0x65, 0x6C, 0x63, 0x6F, 0x6D, 0x65,
        0x4F, 0x4B,
    ];
    if (rep := parseAuthenReply(rep_body[0..15])) {
        expect_eq(rep.status == TacacsAuthenStatus.pass, true);
        expect_eq(strEq(rep.server_msg, "Welcome"), true);
        expect_eq(strEq(rep.data, "OK"), true);
    } else {
        expect_eq(true, false);
    }
}

test("tacacs: Authorization REQUEST + TacacsArgIter") {
    // 8B fixed + 2 arg_lens (13, 8) + user="bob"(3) + port="vty0"(4) + rem=""(0) + args("service=shell", "cmd=show")
    // total = 8 + 2 + 3 + 4 + 0 + 13 + 8 = 38
    let body: [38]u8 = [
        0x06, 0x0F, 0x01, 0x01, 3, 4, 0, 2,
        13, 8,
        // "bob"
        0x62, 0x6F, 0x62,
        // "vty0"
        0x76, 0x74, 0x79, 0x30,
        // "service=shell" (13)
        0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x3D, 0x73, 0x68, 0x65, 0x6C, 0x6C,
        // "cmd=show" (8)
        0x63, 0x6D, 0x64, 0x3D, 0x73, 0x68, 0x6F, 0x77,
    ];

    if (req := parseAuthorRequest(body[0..38])) {
        expect_eq(req.authen_method, 6);
        expect_eq(req.priv_lvl, 15);
        expect_eq(req.arg_cnt, 2);
        expect_eq(strEq(req.user, "bob"), true);
        expect_eq(strEq(req.port, "vty0"), true);
        expect_eq(req.rem_addr.len, 0);

        let it = req.args();
        let count: u32 = 0;
        while (arg := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(strEq(arg, "service=shell"), true);
            } else if (count == 2) {
                expect_eq(strEq(arg, "cmd=show"), true);
            }
        }
        expect_eq(count, 2);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("tacacs: Authorization RESPONSE + TacacsArgIter") {
    // 6B fixed: status=PASS_ADD(1), arg_cnt=2, msg_len=2("ok"), data_len=0
    // + 2 arg_lens (11, 7) + "ok" + "priv-lvl=15" + "timeout"
    // total = 6 + 2 + 2 + 0 + 11 + 7 = 28
    let body: [28]u8 = [
        0x01, 0x02, 0x00, 0x02, 0x00, 0x00,
        11, 7,
        0x6F, 0x6B,
        0x70, 0x72, 0x69, 0x76, 0x2D, 0x6C, 0x76, 0x6C, 0x3D, 0x31, 0x35,
        0x74, 0x69, 0x6D, 0x65, 0x6F, 0x75, 0x74,
    ];

    if (resp := parseAuthorResponse(body[0..28])) {
        expect_eq(resp.status == TacacsAuthorStatus.pass_add, true);
        expect_eq(resp.arg_cnt, 2);
        expect_eq(strEq(resp.server_msg, "ok"), true);
        expect_eq(resp.data.len, 0);

        let it = resp.args();
        let count: u32 = 0;
        while (arg := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(strEq(arg, "priv-lvl=15"), true);
            } else if (count == 2) {
                expect_eq(strEq(arg, "timeout"), true);
            }
        }
        expect_eq(count, 2);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("tacacs: Accounting REQUEST (+ TacacsArgIter) & Accounting REPLY") {
    // Acct REQUEST: 9B fixed (flags=0x02 START, method=6, priv=15, type=1, svc=1, ulen=4, plen=0, rlen=0, arg_cnt=1)
    // + 1 arg_len (10) + user="root"(4) + arg="task_id=42"(10) = 24B
    let req_body: [24]u8 = [
        0x02, 0x06, 0x0F, 0x01, 0x01, 4, 0, 0, 1,
        10,
        0x72, 0x6F, 0x6F, 0x74,
        0x74, 0x61, 0x73, 0x6B, 0x5F, 0x69, 0x64, 0x3D, 0x34, 0x32,
    ];

    if (req := parseAcctRequest(req_body[0..24])) {
        expect_eq(req.flags, 0x02);
        expect_eq(req.arg_cnt, 1);
        expect_eq(strEq(req.user, "root"), true);

        let it = req.args();
        if (arg := it.next()) {
            expect_eq(strEq(arg, "task_id=42"), true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }

    // Acct REPLY: 5B fixed (msg_len=4 "done", data_len=0, status=SUCCESS(1)) + "done" = 9B
    let rep_body: [9]u8 = [0x00, 0x04, 0x00, 0x00, 0x01, 0x64, 0x6F, 0x6E, 0x65];
    if (rep := parseAcctReply(rep_body[0..9])) {
        expect_eq(rep.status == TacacsAcctStatus.success, true);
        expect_eq(strEq(rep.server_msg, "done"), true);
        expect_eq(rep.data.len, 0);
    } else {
        expect_eq(true, false);
    }
}

test("tacacs: strict header guards (major!=0xC, bad type, seq==0, truncated/oversized)") {
    let base: [12]u8 = [0xC1, 0x02, 0x01, 0x00, 0, 0, 0, 1, 0, 0, 0, 0];
    expect_eq(isTacacs(base[0..12]), true);
    if (p := parseTacacs(base[0..12])) {
        expect_eq(p.header.minor_version, TACACS_MINOR_VER_ONE);
        expect_eq(p.header.pkt_type == TacacsType.author, true);
    } else {
        expect_eq(true, false);
    }

    // 1. Truncated < 12B
    expect_eq(parseTacacs(base[0..11]) == null, true);

    // 2. Wrong major version (0xB0 instead of 0xC0)
    let bad_ver = base;
    bad_ver[0] = 0xB0;
    expect_eq(parseTacacs(bad_ver[0..12]) == null, true);

    // 3. Invalid pkt_type (0 and 4)
    let bad_t0 = base;
    bad_t0[1] = 0;
    expect_eq(parseTacacs(bad_t0[0..12]) == null, true);

    let bad_t4 = base;
    bad_t4[1] = 4;
    expect_eq(parseTacacs(bad_t4[0..12]) == null, true);

    // 4. seq_no == 0
    let bad_seq = base;
    bad_seq[2] = 0;
    expect_eq(parseTacacs(bad_seq[0..12]) == null, true);

    // 5. Declared length > available body bytes
    let bad_len = base;
    bad_len[11] = 5;
    expect_eq(parseTacacs(bad_len[0..12]) == null, true);
}

test("tacacs: malformed & truncated body guards across all 6 body parsers") {
    // Authen START too short / field sum overflow
    expect_eq(parseAuthenStart("\x01\x01\x01\x01") == null, true);
    expect_eq(parseAuthenStart("\x01\x01\x01\x01\x05\x00\x00\x00abc") == null, true);

    // Authen REPLY msg_len exceeds body
    expect_eq(parseAuthenReply("\x01\x00\x00\x10\x00\x00ab") == null, true);

    // Author REQUEST arg_cnt exceeds body / arg_lens sum exceeds body
    expect_eq(parseAuthorRequest("\x06\x0F\x01\x01\x00\x00\x00\x05\x01\x01") == null, true);
    expect_eq(parseAuthorRequest("\x06\x0F\x01\x01\x00\x00\x00\x01\x0Aabc") == null, true);

    // Author RESPONSE truncated
    expect_eq(parseAuthorResponse("\x01\x01\x00\x04\x00\x00\x02ab") == null, true);

    // Acct REQUEST truncated
    expect_eq(parseAcctRequest("\x02\x06\x0F\x01\x01\x00\x00\x00\x01\x08ab") == null, true);

    // Acct REPLY truncated
    expect_eq(parseAcctReply("\x00\x05\x00\x00\x01ab") == null, true);
}
