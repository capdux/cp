import { test, expect_eq } from "cap:test";
import {
    HASH_BTIH_HEX,
    HASH_BTIH_BASE32,
    HASH_BTMH_V2,
    MagnetLink,
    parseMagnet,
    formatMagnetV1,
} from "../../packages/magnet-url/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/magnet-uri — parse BitTorrent v1 40-hex btih with dn, xl & trackers", () => {
        let uri: string = "magnet:?xt=urn:btih:c12fe1c06bba254a9dc9f519b335aa7c1367a88a&dn=Ubuntu+24.04&xl=2147483648&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337&tr=https%3A%2F%2Ftracker.example.com";
        let m: MagnetLink = parseMagnet(uri);
        expect_eq(m.valid, true);
        expect_eq(m.hash_kind, HASH_BTIH_HEX);
        expect_eq(eqStr(m.info_hash, "c12fe1c06bba254a9dc9f519b335aa7c1367a88a"), true);
        expect_eq(eqStr(m.display_name, "Ubuntu+24.04"), true);
        expect_eq(m.exact_length, 2147483648);
        expect_eq(m.tracker_count, 2);
        expect_eq(eqStr(m.trackers[0], "udp%3A%2F%2Ftracker.opentrackr.org%3A1337"), true);
    });

    test("2. @cap/magnet-uri — parse 32-char Base32 btih", () => {
        let uri: string = "MAGNET:?xt=urn:btih:YEX6DQDLXISUVHOJ6UM3GNNKPQJWPKEK&dn=Archive";
        let m: MagnetLink = parseMagnet(uri);
        expect_eq(m.valid, true);
        expect_eq(m.hash_kind, HASH_BTIH_BASE32);
        expect_eq(m.info_hash.len, 32);
        expect_eq(eqStr(m.display_name, "Archive"), true);
    });

    test("3. @cap/magnet-uri — parse BitTorrent v2 btmh SHA-256 multihash", () => {
        let uri: string = "magnet:?xt=urn:btmh:1220caf1e1c30e81cb361b9ee167c4aa64228a7fa4fa9f6105232b28ad099f3a302e&dn=v2-release";
        let m: MagnetLink = parseMagnet(uri);
        expect_eq(m.valid, true);
        expect_eq(m.hash_kind, HASH_BTMH_V2);
        expect_eq(m.info_hash.len, 68);
    });

    test("4. @cap/magnet-uri — formatMagnetV1 roundtrip", () => {
        let trs: [1]string = undefined;
        trs[0] = "udp://tracker.example:6969";
        let buf: [256]u8 = undefined;
        let n: usize = formatMagnetV1("c12fe1c06bba254a9dc9f519b335aa7c1367a88a", "CP-ISO", trs[0..1], buf[0..]);
        expect_eq(n > 0, true);
        let parsed: MagnetLink = parseMagnet(buf[0..n]);
        expect_eq(parsed.valid, true);
        expect_eq(eqStr(parsed.info_hash, "c12fe1c06bba254a9dc9f519b335aa7c1367a88a"), true);
        expect_eq(eqStr(parsed.display_name, "CP-ISO"), true);
        expect_eq(parsed.tracker_count, 1);
    });

    test("5. @cap/magnet-uri — reject invalid scheme, missing xt & malformed hash", () => {
        expect_eq(parseMagnet("https://example.com").valid, false);
        expect_eq(parseMagnet("magnet:?dn=NoHash").valid, false);
        expect_eq(parseMagnet("magnet:?xt=urn:btih:tooshort").valid, false);
        expect_eq(parseMagnet("magnet:?xt=urn:btmh:9999deadbeef").valid, false);
    });

    test("6. @cap/magnet-uri — small buffer & bad hash protection in formatMagnetV1", () => {
        let empty_trs: [0]string = undefined;
        let tiny: [16]u8 = undefined;
        expect_eq(formatMagnetV1("c12fe1c06bba254a9dc9f519b335aa7c1367a88a", "x", empty_trs[0..0], tiny[0..]), 0);
        let big: [128]u8 = undefined;
        expect_eq(formatMagnetV1("invalid", "x", empty_trs[0..0], big[0..]), 0);
    });
}
