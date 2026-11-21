// Source: cp/cap/test/test_gopher_production_deep.cp (Production deep tests for cap:gopher)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GOPHER_TCP_PORT,
    TYPE_TEXT,
    TYPE_DIR,
    TYPE_CSO,
    TYPE_ERROR,
    TYPE_BINHEX,
    TYPE_DOS,
    TYPE_UUENCODED,
    TYPE_SEARCH,
    TYPE_TELNET,
    TYPE_BINARY,
    TYPE_MIRROR,
    TYPE_GIF,
    TYPE_IMAGE,
    TYPE_INFO,
    TYPE_HTML,
    TYPE_SOUND,
    GopherRequest,
    parseGopherRequest,
    encodeGopherRequest,
    GopherItem,
    parseGopherItem,
    encodeGopherItem,
    GopherMenuIter,
} from "cap:gopher";

test("1. Constants, empty root request, selector request, and TAB search request roundtrip", () => {
    expect_eq(GOPHER_TCP_PORT, 70 as u16);
    expect_eq(TYPE_TEXT, '0' as u8);
    expect_eq(TYPE_DIR, '1' as u8);
    expect_eq(TYPE_SEARCH, '7' as u8);
    expect_eq(TYPE_INFO, 'i' as u8);
    expect_eq(TYPE_HTML, 'h' as u8);

    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Empty root selector -> "\r\n"
    const n_root = encodeGopherRequest("", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_root], "\r\n"), true);
    const r_root = parseGopherRequest(buf[0..n_root]) ?? unreachable;
    expect_eq(r_root.selector.len, 0 as usize);
    expect_eq(r_root.has_search, false);

    // Normal selector
    const n_sel = encodeGopherRequest("/docs/rfc1436.txt", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_sel], "/docs/rfc1436.txt\r\n"), true);
    const r_sel = parseGopherRequest(buf[0..n_sel]) ?? unreachable;
    expect_eq(std.mem.eql(u8, r_sel.selector, "/docs/rfc1436.txt"), true);
    expect_eq(r_sel.has_search, false);

    // Search request with TAB separator
    const n_search = encodeGopherRequest("/search", "gopher protocol", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_search], "/search\tgopher protocol\r\n"), true);
    const r_search = parseGopherRequest(buf[0..n_search]) ?? unreachable;
    expect_eq(std.mem.eql(u8, r_search.selector, "/search"), true);
    expect_eq(r_search.has_search, true);
    expect_eq(std.mem.eql(u8, r_search.search_query, "gopher protocol"), true);
});

test("2. GopherMenuIter parses i/0/1/7/h(URL:) menu entries and stops at standalone '.'", () => {
    const menu =
        "iWelcome to CP Gopherhole\tfake\terror.host\t1\r\n" ++
        "0About RFC 1436\t/about.txt\tgopher.cp.dev\t70\r\n" ++
        "1Archive Directory\t/archive\tgopher.cp.dev\t70\n" ++
        "7Search Index\t/veronica\tsearch.cp.dev\t7070\r\n" ++
        "hProject Website\tURL:https://cp-lang.dev/\tgopher.cp.dev\t70\r\n" ++
        ".\r\n";

    let it = GopherMenuIter.init(menu);

    const i1 = it.next() ?? unreachable;
    expect_eq(i1.isInfo(), true);
    expect_eq(std.mem.eql(u8, i1.display, "Welcome to CP Gopherhole"), true);
    expect_eq(i1.port, 1 as u16);

    const i2 = it.next() ?? unreachable;
    expect_eq(i2.isText(), true);
    expect_eq(std.mem.eql(u8, i2.selector, "/about.txt"), true);
    expect_eq(std.mem.eql(u8, i2.host, "gopher.cp.dev"), true);
    expect_eq(i2.port, 70 as u16);

    const i3 = it.next() ?? unreachable;
    expect_eq(i3.isDir(), true);
    expect_eq(std.mem.eql(u8, i3.display, "Archive Directory"), true);

    const i4 = it.next() ?? unreachable;
    expect_eq(i4.isSearch(), true);
    expect_eq(i4.port, 7070 as u16);

    const i5 = it.next() ?? unreachable;
    expect_eq(i5.isHtmlUrl(), true);
    expect_eq(std.mem.eql(u8, i5.htmlUrl() ?? unreachable, "https://cp-lang.dev/"), true);

    // Terminated cleanly on "."
    expect_eq(it.next() == null, true);
    expect_eq(it.isTerminated(), true);
    expect_eq(it.hasError(), false);
});

test("3. encodeGopherItem <-> parseGopherItem roundtrip including port 0 and 65535", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const n1 = encodeGopherItem(TYPE_BINARY, "Firmware Image", "/fw.bin", "files.cp.dev", 70, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "9Firmware Image\t/fw.bin\tfiles.cp.dev\t70\r\n"), true);
    const p1 = parseGopherItem(buf[0..n1]) ?? unreachable;
    expect_eq(p1.item_type, TYPE_BINARY);
    expect_eq(std.mem.eql(u8, p1.display, "Firmware Image"), true);
    expect_eq(std.mem.eql(u8, p1.selector, "/fw.bin"), true);
    expect_eq(std.mem.eql(u8, p1.host, "files.cp.dev"), true);
    expect_eq(p1.port, 70 as u16);

    // Boundary ports: 0 and 65535
    const n_zero = encodeGopherItem(TYPE_INFO, "Notice", "", "null.host", 0, buf[0..]) ?? unreachable;
    const p_zero = parseGopherItem(buf[0..n_zero]) ?? unreachable;
    expect_eq(p_zero.port, 0 as u16);
    expect_eq(p_zero.selector.len, 0 as usize);

    const n_max = encodeGopherItem(TYPE_GIF, "Logo", "/logo.gif", "img.cp.dev", 65535, buf[0..]) ?? unreachable;
    const p_max = parseGopherItem(buf[0..n_max]) ?? unreachable;
    expect_eq(p_max.item_type, TYPE_GIF);
    expect_eq(p_max.port, 65535 as u16);
});

test("4. Gopher+ extra TAB attribute compatibility and non-URL html item", () => {
    // Gopher+ trailing "\t+" column ignored safely while parsing port
    const gp_item = parseGopherItem("0Readme\t/README\tgopher.cp.dev\t70\t+\r\n") ?? unreachable;
    expect_eq(gp_item.isText(), true);
    expect_eq(gp_item.port, 70 as u16);

    // Local HTML file (type 'h' without "URL:" prefix)
    const local_html = parseGopherItem("hLocal Page\t/index.html\tgopher.cp.dev\t70\r\n") ?? unreachable;
    expect_eq(local_html.item_type, TYPE_HTML);
    expect_eq(local_html.isHtmlUrl(), false);
    expect_eq(local_html.htmlUrl() == null, true);
});

test("5. Malformed items: missing TABs, invalid port >65535, injection chars, and broken menu lines rejected", () => {
    // Missing TAB columns
    expect_eq(parseGopherItem("0OnlyDisplay\r\n") == null, true);
    expect_eq(parseGopherItem("0Disp\t/sel\r\n") == null, true);
    expect_eq(parseGopherItem("0Disp\t/sel\thost\r\n") == null, true);

    // Invalid ports (>65535, non-numeric, empty)
    expect_eq(parseGopherItem("0Disp\t/sel\thost\t65536\r\n") == null, true);
    expect_eq(parseGopherItem("0Disp\t/sel\thost\t999999\r\n") == null, true);
    expect_eq(parseGopherItem("0Disp\t/sel\thost\tabc\r\n") == null, true);
    expect_eq(parseGopherItem("0Disp\t/sel\thost\t\r\n") == null, true);

    // Menu iterator sets hasError() on malformed line
    let bad_menu = GopherMenuIter.init("0Valid\t/a\thost\t70\r\nBROKEN_LINE_NO_TABS\r\n.\r\n");
    expect_eq(bad_menu.next() != null, true);
    expect_eq(bad_menu.next() == null, true);
    expect_eq(bad_menu.hasError(), true);
    expect_eq(bad_menu.isTerminated(), false);

    // Injection guards in encoders
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(encodeGopherRequest("/bad\r\nline", "", buf[0..]) == null, true);
    expect_eq(encodeGopherRequest("/bad\tselector", "", buf[0..]) == null, true);
    expect_eq(encodeGopherRequest("/search", "q\r\ninjected", buf[0..]) == null, true);
    expect_eq(encodeGopherItem(TYPE_TEXT, "bad\tdisp", "/sel", "host", 70, buf[0..]) == null, true);
    expect_eq(encodeGopherItem(TYPE_TEXT, "disp", "/sel\r\n", "host", 70, buf[0..]) == null, true);
    expect_eq(encodeGopherItem(TYPE_TEXT, "disp", "/sel", "host\n", 70, buf[0..]) == null, true);
    expect_eq(encodeGopherItem('\t', "disp", "/sel", "host", 70, buf[0..]) == null, true);

    // Buffer too small
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(encodeGopherItem(TYPE_TEXT, "a", "/b", "c", 70, tiny[0..]) == null, true);
});
