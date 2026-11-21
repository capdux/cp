import { MacAddr } from "cap:mac_addr";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("MacAddr parses all 4 notations, classifies bits, and derives EUI-64", () => {
    let buf: [32]u8 = [0; 32];

    const m1 = MacAddr.parse("00:1A:2B:3C:4D:5E").?;
    expect(m1.isUnicast() && !m1.isMulticast() && !m1.isLocallyAdministered());
    expect(strEq(m1.formatColon(buf[0..]).?, "00:1a:2b:3c:4d:5e"));
    expect(strEq(m1.formatHyphenUpper(buf[0..]).?, "00-1A-2B-3C-4D-5E"));

    // RFC 4291 Modified EUI-64 flips U/L bit (0x00 -> 0x02) and inserts FF:FE
    const eui = m1.toEui64();
    expect(eui[0] == 0x02 && eui[3] == 0xFF && eui[4] == 0xFE && eui[7] == 0x5E);

    const m2 = MacAddr.parse("001a.2b3c.4d5e").?;
    expect(strEq(m2.formatColon(buf[0..]).?, "00:1a:2b:3c:4d:5e"));

    const m3 = MacAddr.parse("02-00-00-00-00-01").?;
    expect(m3.isLocallyAdministered() && m3.isUnicast());

    const bcast = MacAddr.parse("ffffffffffff").?;
    expect(bcast.isBroadcast() && bcast.isMulticast());

    expect(MacAddr.parse("00:1a-2b:3c:4d:5e") == null); // mixed separators rejected
    expect(MacAddr.parse("zz:1a:2b:3c:4d:5e") == null);
    expect(MacAddr.parse("00:1a:2b") == null);
});
