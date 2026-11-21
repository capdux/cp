// Source: cp/cap/test/test_dhcp_production_deep.cp
// Deep Production Test Suite for cap:dhcp
// Covers:
//   1. DHCPDISCOVER & DHCPOFFER round-trip via DhcpWriter & parseDhcp
//   2. DHCPREQUEST & DHCPACK full typed Option extraction (leaseTime, subnetMask, serverId, requestedIp, hostname)
//   3. OPT_PAD (0) skipping & OPT_END (255) hard stop ignoring trailing padding bytes
//   4. DhcpOptIter full traversal over multiple options (DNS, Router, DomainName)
//   5. Strict header guards: Magic Cookie mismatch, invalid op, hlen > 16, truncated < 240B
//   6. Malformed/truncated Option iterator error propagation & DhcpWriter overflow guards

import { test, expect_eq } from "cap:test";
import {
    DHCP_SERVER_PORT,
    DHCP_CLIENT_PORT,
    DHCP_FIXED_HEADER_SIZE,
    DHCP_MIN_PACKET_SIZE,
    DHCP_MAGIC_COOKIE,
    DHCP_OP_REQUEST,
    DHCP_OP_REPLY,
    DHCP_FLAG_BROADCAST,
    OPT_PAD,
    OPT_SUBNET_MASK,
    OPT_ROUTER,
    OPT_DNS,
    OPT_HOSTNAME,
    OPT_DOMAIN_NAME,
    OPT_REQUESTED_IP,
    OPT_LEASE_TIME,
    OPT_MSG_TYPE,
    OPT_SERVER_ID,
    OPT_PARAM_REQ_LIST,
    OPT_MAX_MSG_SIZE,
    OPT_RENEWAL_TIME,
    OPT_REBINDING_TIME,
    OPT_VENDOR_CLASS,
    OPT_CLIENT_ID,
    OPT_END,
    DhcpMsgType,
    DhcpHeader,
    DhcpOption,
    DhcpOptIter,
    DhcpPacket,
    DhcpWriter,
    parseDhcp,
    isDhcp,
} from "cap:dhcp";

fn ip4Eq(a: [4]u8, b: [4]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

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

test("dhcp: DHCPDISCOVER & DHCPOFFER round-trip via DhcpWriter & parseDhcp") {
    expect_eq(DHCP_SERVER_PORT, 67);
    expect_eq(DHCP_CLIENT_PORT, 68);
    expect_eq(DHCP_FIXED_HEADER_SIZE, 236);
    expect_eq(DHCP_MIN_PACKET_SIZE, 240);
    expect_eq(DHCP_MAGIC_COOKIE, 0x63825363);

    let buf: [512]u8 = [0; 512];
    const mac: [6]u8 = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55];
    let w = DhcpWriter.init(buf[0..512], DHCP_OP_REQUEST, 0xDEADBEEF, mac, true);
    expect_eq(w.writeMsgType(DhcpMsgType.discover), true);
    expect_eq(w.writeOption(OPT_HOSTNAME, "workstation-01"), true);
    expect_eq(w.writeOption(OPT_PARAM_REQ_LIST, "\x01\x03\x06\x0F"), true);

    if (pkt := w.finish()) {
        expect_eq(isDhcp(pkt), true);
        if (d := parseDhcp(pkt)) {
            expect_eq(d.header.op, DHCP_OP_REQUEST);
            expect_eq(d.header.xid, 0xDEADBEEF);
            expect_eq(d.header.isBroadcast(), true);
            const got_mac = d.header.macAddr();
            expect_eq(got_mac[0], 0x00);
            expect_eq(got_mac[5], 0x55);
            expect_eq(d.msgType() == DhcpMsgType.discover, true);
            if (hn := d.hostname()) {
                expect_eq(strEq(hn, "workstation-01"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // DHCPOFFER reply
    let off_buf: [512]u8 = [0; 512];
    let wo = DhcpWriter.init(off_buf[0..512], DHCP_OP_REPLY, 0xDEADBEEF, mac, false);
    wo.setYiaddr([192, 168, 1, 100]);
    wo.setSiaddr([192, 168, 1, 1]);
    expect_eq(wo.writeMsgType(DhcpMsgType.offer), true);
    expect_eq(wo.writeIpOption(OPT_SERVER_ID, [192, 168, 1, 1]), true);
    expect_eq(wo.writeU32Option(OPT_LEASE_TIME, 86400), true);
    expect_eq(wo.writeIpOption(OPT_SUBNET_MASK, [255, 255, 255, 0]), true);

    if (pkt := wo.finish()) {
        if (d := parseDhcp(pkt)) {
            expect_eq(d.header.op, DHCP_OP_REPLY);
            expect_eq(d.header.isBroadcast(), false);
            expect_eq(ip4Eq(d.header.yiaddr, [192, 168, 1, 100]), true);
            expect_eq(ip4Eq(d.header.siaddr, [192, 168, 1, 1]), true);
            expect_eq(d.msgType() == DhcpMsgType.offer, true);
            expect_eq(d.leaseTime() == 86400, true);
            if (sid := d.serverId()) {
                expect_eq(ip4Eq(sid, [192, 168, 1, 1]), true);
            } else {
                expect_eq(true, false);
            }
            if (mask := d.subnetMask()) {
                expect_eq(ip4Eq(mask, [255, 255, 255, 0]), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dhcp: DHCPREQUEST & DHCPACK full typed Option extraction") {
    let req_buf: [512]u8 = [0; 512];
    let wr = DhcpWriter.init(req_buf[0..512], DHCP_OP_REQUEST, 0x11223344, [1, 2, 3, 4, 5, 6], false);
    expect_eq(wr.writeMsgType(DhcpMsgType.request), true);
    expect_eq(wr.writeIpOption(OPT_REQUESTED_IP, [10, 0, 0, 50]), true);
    expect_eq(wr.writeIpOption(OPT_SERVER_ID, [10, 0, 0, 1]), true);

    if (pkt := wr.finish()) {
        if (d := parseDhcp(pkt)) {
            expect_eq(d.msgType() == DhcpMsgType.request, true);
            if (rip := d.requestedIp()) {
                expect_eq(ip4Eq(rip, [10, 0, 0, 50]), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    let ack_buf: [512]u8 = [0; 512];
    let wa = DhcpWriter.init(ack_buf[0..512], DHCP_OP_REPLY, 0x11223344, [1, 2, 3, 4, 5, 6], false);
    wa.setYiaddr([10, 0, 0, 50]);
    expect_eq(wa.writeMsgType(DhcpMsgType.ack), true);
    expect_eq(wa.writeIpOption(OPT_ROUTER, [10, 0, 0, 1]), true);
    expect_eq(wa.writeOption(OPT_DNS, "\x08\x08\x08\x08\x01\x01\x01\x01"), true);
    expect_eq(wa.writeOption(OPT_DOMAIN_NAME, "corp.local"), true);

    if (pkt := wa.finish()) {
        if (d := parseDhcp(pkt)) {
            expect_eq(d.msgType() == DhcpMsgType.ack, true);
            if (dns := d.findOption(OPT_DNS)) {
                expect_eq(dns.value.len, 8);
                expect_eq(dns.value[0], 8);
                expect_eq(dns.value[4], 1);
            } else {
                expect_eq(true, false);
            }
            if (dom := d.findOption(OPT_DOMAIN_NAME)) {
                expect_eq(strEq(dom.value, "corp.local"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dhcp: OPT_PAD skipping & OPT_END stops before trailing garbage") {
    let buf: [300]u8 = [0; 300];
    let w = DhcpWriter.init(buf[0..300], DHCP_OP_REQUEST, 0x55667788, [9, 8, 7, 6, 5, 4], true);
    expect_eq(w.writePad(), true);
    expect_eq(w.writePad(), true);
    expect_eq(w.writeMsgType(DhcpMsgType.inform), true);
    expect_eq(w.writePad(), true);
    expect_eq(w.writeOption(OPT_HOSTNAME, "pad-host"), true);

    if (pkt := w.finish()) {
        // Append trailing bytes after OPT_END (255) into buf and parse full 300B slice
        buf[pkt.len] = OPT_LEASE_TIME;
        buf[pkt.len + 1] = 4;
        buf[pkt.len + 2] = 0x00;
        buf[pkt.len + 3] = 0x00;
        buf[pkt.len + 4] = 0x0E;
        buf[pkt.len + 5] = 0x10;

        if (d := parseDhcp(buf[0..300])) {
            expect_eq(d.msgType() == DhcpMsgType.inform, true);
            // Option placed after OPT_END must NOT be seen!
            expect_eq(d.leaseTime() == null, true);

            let it = d.options();
            let count: u32 = 0;
            while (opt := it.next()) {
                * = opt;
                count += 1;
            }
            expect_eq(count, 2);
            expect_eq(it.hasEndOption(), true);
            expect_eq(it.hasError(), false);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dhcp: strict header guards (Magic Cookie, op, hlen, truncated < 240B)") {
    let valid: [240]u8 = [0; 240];
    valid[0] = DHCP_OP_REQUEST;
    valid[1] = 1;
    valid[2] = 6;
    valid[236] = 0x63; valid[237] = 0x82; valid[238] = 0x53; valid[239] = 0x63;
    expect_eq(isDhcp(valid[0..240]), true);

    // 1. Truncated < 240B
    expect_eq(parseDhcp(valid[0..239]) == null, true);

    // 2. Wrong Magic Cookie
    let bad_cookie = valid;
    bad_cookie[239] = 0x64;
    expect_eq(parseDhcp(bad_cookie[0..240]) == null, true);

    // 3. Invalid op (0 and 3)
    let bad_op0 = valid;
    bad_op0[0] = 0;
    expect_eq(parseDhcp(bad_op0[0..240]) == null, true);

    let bad_op3 = valid;
    bad_op3[0] = 3;
    expect_eq(parseDhcp(bad_op3[0..240]) == null, true);

    // 4. hlen > 16
    let bad_hlen = valid;
    bad_hlen[2] = 17;
    expect_eq(parseDhcp(bad_hlen[0..240]) == null, true);
}

test("dhcp: malformed Option iterator & DhcpWriter overflow guards") {
    // 1. Option missing length byte -> hasError()
    let it1 = DhcpOptIter.init("\x35");
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);

    // 2. Option claims length 10, only 3 bytes available -> hasError()
    let it2 = DhcpOptIter.init("\x0C\x0Aabc");
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);

    // 3. DhcpWriter buffer < 240B -> failed
    let tiny: [100]u8 = [0; 100];
    let w_tiny = DhcpWriter.init(tiny[0..100], DHCP_OP_REQUEST, 1, [0; 6], false);
    expect_eq(w_tiny.hasError(), true);
    expect_eq(w_tiny.finish() == null, true);

    // 4. DhcpWriter option overflow
    let exact: [242]u8 = [0; 242];
    let w_ovf = DhcpWriter.init(exact[0..242], DHCP_OP_REQUEST, 1, [0; 6], false);
    expect_eq(w_ovf.writeOption(OPT_HOSTNAME, "too-long"), false);
    expect_eq(w_ovf.hasError(), true);
    expect_eq(w_ovf.finish() == null, true);
}
