// cap/test/testCapSocks5Frame.cp
// Comprehensive unit tests for @cap/socks5-frame

import { test, expect_eq } from "cap:test";
import {
    SOCKS_V5,
    AUTH_NONE,
    AUTH_USERPASS,
    AUTH_NO_ACCEPTABLE,
    CMD_CONNECT,
    ATYP_IPV4,
    ATYP_DOMAIN,
    ATYP_IPV6,
    S5_OK,
    S5_INCOMPLETE,
    S5_INVALID,
    Socks5Request,
    encodeGreeting,
    selectMethod,
    encodeUserPassAuth,
    encodeConnectDomain,
    encodeConnectIpv4,
    decodeRequest,
    encodeReplyIpv4,
} from "../../packages/fast-socks5/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn emptyReq(): Socks5Request {
    let v4: [4]u8 = undefined;
    let idx4: usize = 0;
    while (idx4 < 4) {
        v4[idx4] = 0 as u8;
        idx4 += 1;
    }
    let v6: [16]u8 = undefined;
    let idx6: usize = 0;
    while (idx6 < 16) {
        v6[idx6] = 0 as u8;
        idx6 += 1;
    }
    return Socks5Request{
        cmd: 0 as u8,
        atyp: 0 as u8,
        ipv4: v4,
        ipv6: v6,
        domain: "",
        port: 0 as u16,
        consumed: 0,
    };
}

fn main() {
    test("1. @cap/socks5-frame — Greeting encoding & auth method negotiation", () => {
        let offered: [2]u8 = undefined;
        offered[0] = AUTH_NONE;
        offered[1] = AUTH_USERPASS;

        let buf: [8]u8 = undefined;
        const n = encodeGreeting(offered[0..2], buf[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(buf[0], SOCKS_V5);
        expect_eq(buf[1], 2 as u8);

        let sup1: [1]u8 = undefined;
        sup1[0] = AUTH_USERPASS;
        let chosen: u8 = 0 as u8;
        expect_eq(selectMethod(buf[0..n], sup1[0..1], &chosen), S5_OK);
        expect_eq(chosen, AUTH_USERPASS);

        let supNone: [1]u8 = undefined;
        supNone[0] = 0x05 as u8;
        expect_eq(selectMethod(buf[0..n], supNone[0..1], &chosen), S5_OK);
        expect_eq(chosen, AUTH_NO_ACCEPTABLE);
    });

    test("2. @cap/socks5-frame — RFC 1929 Username/Password subnegotiation encoding", () => {
        let buf: [32]u8 = undefined;
        const n = encodeUserPassAuth("alice", "s3cr3t", buf[0..32]);
        expect_eq(n, 14 as usize);
        expect_eq(buf[0], 0x01 as u8);
        expect_eq(buf[1], 5 as u8);
        expect_eq(sliceEq(buf[2..7], "alice"), true);
        expect_eq(buf[7], 6 as u8);
        expect_eq(sliceEq(buf[8..14], "s3cr3t"), true);

        expect_eq(encodeUserPassAuth("", "pass", buf[0..32]), 0 as usize);
        expect_eq(encodeUserPassAuth("user", "", buf[0..32]), 0 as usize);
    });

    test("3. @cap/socks5-frame — CONNECT IPv4 encode & decode round-trip", () => {
        let ip: [4]u8 = undefined;
        ip[0] = 93 as u8;
        ip[1] = 184 as u8;
        ip[2] = 216 as u8;
        ip[3] = 34 as u8;

        let buf: [16]u8 = undefined;
        const n = encodeConnectIpv4(ip, 443 as u16, buf[0..16]);
        expect_eq(n, 10 as usize);

        let req = emptyReq();
        expect_eq(decodeRequest(buf[0..n], &req), S5_OK);
        expect_eq(req.cmd, CMD_CONNECT);
        expect_eq(req.atyp, ATYP_IPV4);
        expect_eq(req.ipv4[0], 93 as u8);
        expect_eq(req.ipv4[3], 34 as u8);
        expect_eq(req.port, 443 as u16);
        expect_eq(req.consumed, 10 as usize);
    });

    test("4. @cap/socks5-frame — CONNECT Domain & IPv6 frame decoding", () => {
        let buf: [64]u8 = undefined;
        const n = encodeConnectDomain("api.example.com", 8443 as u16, buf[0..64]);
        expect_eq(n, 22 as usize);

        let req = emptyReq();
        expect_eq(decodeRequest(buf[0..n], &req), S5_OK);
        expect_eq(req.atyp, ATYP_DOMAIN);
        expect_eq(sliceEq(req.domain, "api.example.com"), true);
        expect_eq(req.port, 8443 as u16);
        expect_eq(req.consumed, 22 as usize);

        // Construct IPv6 ::1 port 8080 (0x1F90) frame.
        let v6Frame: [22]u8 = undefined;
        let idx: usize = 0;
        while (idx < 22) {
            v6Frame[idx] = 0 as u8;
            idx += 1;
        }
        v6Frame[0] = SOCKS_V5;
        v6Frame[1] = CMD_CONNECT;
        v6Frame[2] = 0x00 as u8;
        v6Frame[3] = ATYP_IPV6;
        v6Frame[19] = 1 as u8; // ::1
        v6Frame[20] = 0x1F as u8;
        v6Frame[21] = 0x90 as u8;

        let req6 = emptyReq();
        expect_eq(decodeRequest(v6Frame[0..22], &req6), S5_OK);
        expect_eq(req6.atyp, ATYP_IPV6);
        expect_eq(req6.ipv6[15], 1 as u8);
        expect_eq(req6.port, 8080 as u16);
        expect_eq(req6.consumed, 22 as usize);
    });

    test("5. @cap/socks5-frame — encodeReplyIpv4 standard 10-byte response", () => {
        let bindIp: [4]u8 = undefined;
        bindIp[0] = 127 as u8;
        bindIp[1] = 0 as u8;
        bindIp[2] = 0 as u8;
        bindIp[3] = 1 as u8;

        let out: [16]u8 = undefined;
        const n = encodeReplyIpv4(0x00 as u8, bindIp, 1080 as u16, out[0..16]);
        expect_eq(n, 10 as usize);
        expect_eq(out[0], SOCKS_V5);
        expect_eq(out[1], 0x00 as u8);
        expect_eq(out[2], 0x00 as u8);
        expect_eq(out[3], ATYP_IPV4);
        expect_eq(out[4], 127 as u8);
        expect_eq(out[7], 1 as u8);
        expect_eq(out[8], 0x04 as u8); // 1080 >> 8
        expect_eq(out[9], 0x38 as u8); // 1080 & 0xFF
    });

    test("6. @cap/socks5-frame — Partial frames (S5_INCOMPLETE), invalid ver/rsv & tiny buffers", () => {
        let partial: [3]u8 = undefined;
        partial[0] = SOCKS_V5;
        partial[1] = 2 as u8;
        partial[2] = AUTH_NONE;
        let m: u8 = 0 as u8;
        let sup: [1]u8 = undefined;
        sup[0] = AUTH_NONE;
        expect_eq(selectMethod(partial[0..3], sup[0..1], &m), S5_INCOMPLETE);

        let badVer: [4]u8 = undefined;
        badVer[0] = 0x04 as u8;
        badVer[1] = CMD_CONNECT;
        badVer[2] = 0x00 as u8;
        badVer[3] = ATYP_IPV4;
        let req = emptyReq();
        expect_eq(decodeRequest(badVer[0..4], &req), S5_INVALID);

        let badRsv: [10]u8 = undefined;
        let k: usize = 0;
        while (k < 10) {
            badRsv[k] = 0 as u8;
            k += 1;
        }
        badRsv[0] = SOCKS_V5;
        badRsv[1] = CMD_CONNECT;
        badRsv[2] = 0x01 as u8; // non-zero RSV
        badRsv[3] = ATYP_IPV4;
        expect_eq(decodeRequest(badRsv[0..10], &req), S5_INVALID);

        // Incomplete IPv4 request (only 8 of 10 bytes).
        badRsv[2] = 0x00 as u8;
        expect_eq(decodeRequest(badRsv[0..8], &req), S5_INCOMPLETE);

        let tiny: [5]u8 = undefined;
        let ip0: [4]u8 = undefined;
        ip0[0] = 0 as u8;
        ip0[1] = 0 as u8;
        ip0[2] = 0 as u8;
        ip0[3] = 0 as u8;
        expect_eq(encodeConnectIpv4(ip0, 80 as u16, tiny[0..5]), 0 as usize);
        expect_eq(encodeReplyIpv4(0 as u8, ip0, 80 as u16, tiny[0..5]), 0 as usize);
    });
}
