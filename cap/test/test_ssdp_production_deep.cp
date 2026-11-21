// Source: cp/cap/test/test_ssdp_production_deep.cp (Production deep tests for cap:ssdp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SSDP_UDP_PORT,
    SSDP_MULTICAST_V4,
    SSDP_MULTICAST_V6_LINK,
    SSDP_MAN_DISCOVER,
    SSDP_ST_ALL,
    SSDP_ST_ROOTDEVICE,
    SSDP_NTS_ALIVE,
    SSDP_NTS_BYEBYE,
    SSDP_NTS_UPDATE,
    SsdpKind,
    SsdpHeader,
    SsdpHeaderIter,
    SsdpMessage,
    isSsdp,
    parseSsdp,
    encodeSsdpMSearch,
    encodeSsdpNotify,
} from "cap:ssdp";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. Protocol constants & M-SEARCH encode/parse round-trip", () => {
    expect_eq(SSDP_UDP_PORT, 1900 as u16);
    expectBytesEq(SSDP_MULTICAST_V4, "239.255.255.250");
    expectBytesEq(SSDP_MULTICAST_V6_LINK, "ff02::c");
    expectBytesEq(SSDP_MAN_DISCOVER, "\"ssdp:discover\"");
    expectBytesEq(SSDP_ST_ALL, "ssdp:all");
    expectBytesEq(SSDP_ST_ROOTDEVICE, "upnp:rootdevice");

    let buf: [256]u8 = [0; 256];
    const n = encodeSsdpMSearch(SSDP_ST_ROOTDEVICE, 3, buf[0..]) ?? unreachable;
    expect_eq(isSsdp(buf[0..n]), true);

    const msg = parseSsdp(buf[0..n]) ?? unreachable;
    expect_eq(msg.kind == SsdpKind.m_search, true);
    expect_eq(msg.status_code, 0 as u16);
    expectBytesEq(msg.st() ?? unreachable, SSDP_ST_ROOTDEVICE);
    expectBytesEq(msg.man() ?? unreachable, SSDP_MAN_DISCOVER);
    expect_eq(msg.mx() ?? 0, 3 as u32);
    expectBytesEq(msg.findHeader("host") ?? unreachable, "239.255.255.250:1900");
});

test("2. NOTIFY ssdp:alive and ssdp:byebye encode/parse round-trip", () => {
    let buf: [512]u8 = [0; 512];
    const n_alive = encodeSsdpNotify(
        SSDP_ST_ROOTDEVICE,
        SSDP_NTS_ALIVE,
        "uuid:1234-5678::upnp:rootdevice",
        "http://192.168.1.10:8080/desc.xml",
        1800,
        buf[0..],
    ) ?? unreachable;

    const alive = parseSsdp(buf[0..n_alive]) ?? unreachable;
    expect_eq(alive.kind == SsdpKind.notify, true);
    expect_eq(alive.isAlive(), true);
    expect_eq(alive.isByebye(), false);
    expectBytesEq(alive.nt() ?? unreachable, SSDP_ST_ROOTDEVICE);
    expectBytesEq(alive.nts() ?? unreachable, SSDP_NTS_ALIVE);
    expectBytesEq(alive.usn() ?? unreachable, "uuid:1234-5678::upnp:rootdevice");
    expectBytesEq(alive.location() ?? unreachable, "http://192.168.1.10:8080/desc.xml");
    expect_eq(alive.maxAge() ?? 0, 1800 as u32);

    // ssdp:byebye without location or cache-control
    const n_bye = encodeSsdpNotify(
        SSDP_ST_ROOTDEVICE,
        SSDP_NTS_BYEBYE,
        "uuid:1234-5678::upnp:rootdevice",
        "",
        0,
        buf[0..],
    ) ?? unreachable;

    const bye = parseSsdp(buf[0..n_bye]) ?? unreachable;
    expect_eq(bye.kind == SsdpKind.notify, true);
    expect_eq(bye.isAlive(), false);
    expect_eq(bye.isByebye(), true);
    expect_eq(bye.location() == null, true);
    expect_eq(bye.maxAge() == null, true);
});

test("3. HTTP/1.1 200 OK response parsing with CACHE-CONTROL max-age, LOCATION, SERVER, USN", () => {
    const resp_wire =
        "HTTP/1.1 200 OK\r\n" +
        "Cache-Control: public, max-age=3600\r\n" +
        "EXT:\r\n" +
        "Location: http://10.0.0.5:49152/rootDesc.xml\r\n" +
        "Server: Linux/6.8 UPnP/1.1 CapMedia/1.0\r\n" +
        "st: urn:schemas-upnp-org:device:MediaServer:1\r\n" +
        "usn: uuid:abcd-ef01::urn:schemas-upnp-org:device:MediaServer:1\r\n" +
        "\r\n" +
        "IGNORED-BODY-AFTER-BLANK-LINE: should-not-appear\r\n";

    const msg = parseSsdp(resp_wire) ?? unreachable;
    expect_eq(msg.kind == SsdpKind.response, true);
    expect_eq(msg.status_code, 200 as u16);
    expect_eq(msg.maxAge() ?? 0, 3600 as u32);
    expectBytesEq(msg.location() ?? unreachable, "http://10.0.0.5:49152/rootDesc.xml");
    expectBytesEq(msg.server() ?? unreachable, "Linux/6.8 UPnP/1.1 CapMedia/1.0");
    expectBytesEq(msg.st() ?? unreachable, "urn:schemas-upnp-org:device:MediaServer:1");
    expectBytesEq(msg.usn() ?? unreachable, "uuid:abcd-ef01::urn:schemas-upnp-org:device:MediaServer:1");

    // Blank line terminates header parsing; trailing body must not be seen as a header
    expect_eq(msg.findHeader("IGNORED-BODY-AFTER-BLANK-LINE") == null, true);
});

test("4. Case-insensitive header lookup and SsdpHeaderIter direct iteration", () => {
    const raw =
        "NOTIFY * HTTP/1.1\n" +
        "  host : 239.255.255.250:1900 \n" +
        "cAcHe-CoNtRoL: MAX-AGE = 900\n" +
        "nTs: ssdp:update\n" +
        "\n";

    const msg = parseSsdp(raw) ?? unreachable;
    expect_eq(msg.maxAge() ?? 0, 900 as u32);
    expectBytesEq(msg.nts() ?? unreachable, SSDP_NTS_UPDATE);

    let it = msg.headers();
    const h0 = it.next() ?? unreachable;
    expect_eq(h0.nameEqualsIgnoreCase("HOST"), true);
    expectBytesEq(h0.value, "239.255.255.250:1900");

    const h1 = it.next() ?? unreachable;
    expect_eq(h1.nameEqualsIgnoreCase("cache-control"), true);
    const h2 = it.next() ?? unreachable;
    expect_eq(h2.nameEqualsIgnoreCase("NTS"), true);
    expect_eq(it.next() == null, true);
});

test("5. Malformed start line, CRLF/NUL injection, and buffer overflow rejection", () => {
    // Empty, no newline, invalid methods, bad HTTP status
    expect_eq(isSsdp(""), false);
    expect_eq(parseSsdp("M-SEARCH * HTTP/1.1") == null, true);
    expect_eq(parseSsdp("GET / HTTP/1.1\r\n\r\n") == null, true);
    expect_eq(parseSsdp("HTTP/1.1 20 OK\r\n\r\n") == null, true);
    expect_eq(parseSsdp("HTTP/1.1 ABC OK\r\n\r\n") == null, true);
    expect_eq(parseSsdp("HTTP/1.1 2000 OK\r\n\r\n") == null, true);

    // CRLF/NUL injection guards in encoders
    let buf: [256]u8 = [0; 256];
    expect_eq(encodeSsdpMSearch("", 3, buf[0..]) == null, true);
    expect_eq(encodeSsdpMSearch("ssdp:all\r\nX-Injected: 1", 3, buf[0..]) == null, true);
    expect_eq(encodeSsdpNotify("nt", "ssdp:alive\nEvil: 1", "usn", "", 0, buf[0..]) == null, true);
    expect_eq(encodeSsdpNotify("nt", "ssdp:alive", "usn", "http://a\r\nb", 60, buf[0..]) == null, true);
    expect_eq(encodeSsdpNotify("", "ssdp:alive", "usn", "", 0, buf[0..]) == null, true);

    // Small buffer overflow guard
    let tiny: [16]u8 = [0; 16];
    expect_eq(encodeSsdpMSearch(SSDP_ST_ALL, 2, tiny[0..]) == null, true);
    expect_eq(encodeSsdpNotify(SSDP_ST_ALL, SSDP_NTS_ALIVE, "u", "", 60, tiny[0..]) == null, true);
});
