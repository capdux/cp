import { test, expect_eq } from "cap:test";
import {
    MacAddr6,
    MacAddr8
} from "../../packages/@cap/macaddr/src/index.cp";

fn macBufEqStr(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/macaddr — MacAddr6 parses colon, hyphen, Cisco dot, and mixed case", () => {
        let m1 = MacAddr6.parse("01:23:45:67:89:ab") ?? MacAddr6.nil();
        let m2 = MacAddr6.parse("01-23-45-67-89-AB") ?? MacAddr6.nil();
        let m3 = MacAddr6.parse("0123.4567.89aB") ?? MacAddr6.nil();

        expect_eq(m1.equals(m2), true);
        expect_eq(m1.equals(m3), true);
        expect_eq(m1.bytes[0], 0x01);
        expect_eq(m1.bytes[5], 0xAB);

        let oui = m1.oui();
        expect_eq(oui[0], 0x01);
        expect_eq(oui[1], 0x23);
        expect_eq(oui[2], 0x45);
    });

    test("2. @cap/macaddr — MacAddr6 IEEE 802 flags (nil, broadcast, multicast/unicast, local/universal)", () => {
        let nil_m = MacAddr6.nil();
        expect_eq(nil_m.isNil(), true);
        expect_eq(nil_m.isBroadcast(), false);
        expect_eq(nil_m.isUnicast(), true);
        expect_eq(nil_m.isUniversal(), true);

        let bcast = MacAddr6.broadcast();
        expect_eq(bcast.isBroadcast(), true);
        expect_eq(bcast.isMulticast(), true);
        expect_eq(bcast.isLocal(), true);

        // 02:00:5e:00:01:02 -> unicast (bit0=0), locally administered (bit1=1)
        let local_uni = MacAddr6.parse("02:00:5e:00:01:02") ?? MacAddr6.nil();
        expect_eq(local_uni.isUnicast(), true);
        expect_eq(local_uni.isMulticast(), false);
        expect_eq(local_uni.isLocal(), true);
        expect_eq(local_uni.isUniversal(), false);
    });

    test("3. @cap/macaddr — MacAddr6 formats into colon, hyphen, and Cisco dot notations", () => {
        let m = MacAddr6.new(0x00, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e);
        let out: [32]u8 = [0; 32];

        let n1 = m.formatColonInto(out[0..32]);
        expect_eq(macBufEqStr(out[0..32], n1, "00:1a:2b:3c:4d:5e"), true);

        let n2 = m.formatHyphenInto(out[0..32]);
        expect_eq(macBufEqStr(out[0..32], n2, "00-1a-2b-3c-4d-5e"), true);

        let n3 = m.formatDotInto(out[0..32]);
        expect_eq(macBufEqStr(out[0..32], n3, "001a.2b3c.4d5e"), true);
    });

    test("4. @cap/macaddr — MacAddr8 parses colon, hyphen, dot and formats colon", () => {
        let e1 = MacAddr8.parse("01:23:45:67:89:ab:cd:ef") ?? MacAddr8.nil();
        let e2 = MacAddr8.parse("01-23-45-67-89-AB-CD-EF") ?? MacAddr8.nil();
        let e3 = MacAddr8.parse("0123.4567.89ab.cdef") ?? MacAddr8.nil();

        expect_eq(e1.equals(e2), true);
        expect_eq(e1.equals(e3), true);
        expect_eq(e1.isMulticast(), true);
        expect_eq(e1.isLocal(), false);

        let out: [32]u8 = [0; 32];
        let n = e1.formatColonInto(out[0..32]);
        expect_eq(macBufEqStr(out[0..32], n, "01:23:45:67:89:ab:cd:ef"), true);
    });

    test("5. @cap/macaddr — MacAddr8.fromMac6Eui64 inserts FF:FE and flips U/L bit", () => {
        // Classic RFC 4291 example: 00:0c:29:68:22:3c -> 02:0c:29:ff:fe:68:22:3c
        let m6 = MacAddr6.parse("00:0c:29:68:22:3c") ?? MacAddr6.nil();
        let e64 = MacAddr8.fromMac6Eui64(m6);
        let expected = MacAddr8.parse("02:0c:29:ff:fe:68:22:3c") ?? MacAddr8.nil();
        expect_eq(e64.equals(expected), true);
        expect_eq(e64.isLocal(), true);
        expect_eq(e64.isNil(), false);
        expect_eq(e64.isBroadcast(), false);
    });

    test("6. @cap/macaddr — strictly rejects invalid lengths, non-hex chars, and mixed separators", () => {
        expect_eq(MacAddr6.parse("") == null, true);
        expect_eq(MacAddr6.parse("01:23:45:67:89") == null, true);
        expect_eq(MacAddr6.parse("01:23:45:67:89:ab:cd") == null, true);
        expect_eq(MacAddr6.parse("01:23-45:67:89:ab") == null, true);
        expect_eq(MacAddr6.parse("01:23:45:67:89:zz") == null, true);
        expect_eq(MacAddr6.parse("0123.4567:89ab") == null, true);

        expect_eq(MacAddr8.parse("01:23:45:67:89:ab") == null, true);
        expect_eq(MacAddr8.parse("01:23-45:67:89:ab:cd:ef") == null, true);
        expect_eq(MacAddr8.parse("0123.4567.89ab.cdeg") == null, true);
    });
}
