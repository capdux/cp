// Source: cp/cap/test/test_plist_production_deep.cp (Production deep tests for cap:plist)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PlistValueKind,
    PlistDictEntry,
    isXmlPlist,
    PlistDictIter,
    findPlistString,
    findPlistInt,
    findPlistBool,
    formatPlistStringEntry,
} from "cap:plist";

const INFO_PLIST: []const u8 =
    "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" ++
    "<plist version=\"1.0\">\n" ++
    "<dict>\n" ++
    "  <!-- Bundle metadata -->\n" ++
    "  <key>CFBundleIdentifier</key>\n" ++
    "  <string>dev.cp.cap</string>\n" ++
    "  <key>BuildNumber</key>\n" ++
    "  <integer>4201</integer>\n" ++
    "  <key>NegativeOffset</key>\n" ++
    "  <integer>-128</integer>\n" ++
    "  <key>NSHighResolutionCapable</key>\n" ++
    "  <true/>\n" ++
    "  <key>LSUIElement</key>\n" ++
    "  <false/>\n" ++
    "  <key>ScaleFactor</key>\n" ++
    "  <real>2.5</real>\n" ++
    "  <key>NestedConfig</key>\n" ++
    "  <dict>\n" ++
    "    <key>CFBundleIdentifier</key>\n" ++
    "    <string>should.not.shadow</string>\n" ++
    "    <key>InnerDict</key>\n" ++
    "    <dict><key>Deep</key><integer>999</integer></dict>\n" ++
    "  </dict>\n" ++
    "  <key>SupportedArchs</key>\n" ++
    "  <array>\n" ++
    "    <string>arm64</string>\n" ++
    "    <string>x86_64</string>\n" ++
    "  </array>\n" ++
    "  <key>ReleaseDate</key>\n" ++
    "  <date>2026-11-21T12:00:00Z</date>\n" ++
    "  <key>Signature</key>\n" ++
    "  <data>AQIDBA==</data>\n" ++
    "</dict>\n" ++
    "</plist>";

test("1. isXmlPlist validates BOM/XML/DOCTYPE headers and rejects non-plist XML", () => {
    expect_eq(isXmlPlist(INFO_PLIST), true);
    expect_eq(isXmlPlist("<plist version=\"1.0\"><dict/></plist>"), true);

    expect_eq(isXmlPlist(""), false);
    expect_eq(isXmlPlist("<plist version=\"1.0\"/>"), false);
    expect_eq(isXmlPlist("<plist version=\"1.0\"><dict></dict>"), false);
    expect_eq(isXmlPlist("<feed><title>Not plist</title></feed>"), false);
});

test("2. PlistDictIter yields all 9 value kinds and skips nested dict/array subtrees cleanly", () => {
    let it = PlistDictIter.init(INFO_PLIST);
    let count: i64 = 0;
    let saw_nested_dict = false;
    let saw_array = false;
    let saw_real = false;
    let saw_date = false;
    let saw_data = false;

    while (entry := it.next()) {
        count += 1;
        // Nested keys must never leak to top-level iterator
        expect_eq(std.mem.eql(u8, entry.key, "Deep"), false);
        expect_eq(std.mem.eql(u8, entry.key, "InnerDict"), false);

        if (std.mem.eql(u8, entry.key, "NestedConfig")) {
            expect_eq(entry.kind == PlistValueKind.dict, true);
            saw_nested_dict = true;
        } else if (std.mem.eql(u8, entry.key, "SupportedArchs")) {
            expect_eq(entry.kind == PlistValueKind.array, true);
            saw_array = true;
        } else if (std.mem.eql(u8, entry.key, "ScaleFactor")) {
            expect_eq(entry.kind == PlistValueKind.real, true);
            expect_eq(std.mem.eql(u8, entry.raw_value, "2.5"), true);
            saw_real = true;
        } else if (std.mem.eql(u8, entry.key, "ReleaseDate")) {
            expect_eq(entry.kind == PlistValueKind.date, true);
            expect_eq(std.mem.eql(u8, entry.raw_value, "2026-11-21T12:00:00Z"), true);
            saw_date = true;
        } else if (std.mem.eql(u8, entry.key, "Signature")) {
            expect_eq(entry.kind == PlistValueKind.data, true);
            expect_eq(std.mem.eql(u8, entry.raw_value, "AQIDBA=="), true);
            saw_data = true;
        }
    }

    expect_eq(count, 10);
    expect_eq(saw_nested_dict, true);
    expect_eq(saw_array, true);
    expect_eq(saw_real, true);
    expect_eq(saw_date, true);
    expect_eq(saw_data, true);
});

test("3. findPlistString, findPlistInt & findPlistBool typed lookup and mismatch guards", () => {
    expect_eq(std.mem.eql(u8, findPlistString(INFO_PLIST, "CFBundleIdentifier") ?? "", "dev.cp.cap"), true);
    expect_eq(findPlistInt(INFO_PLIST, "BuildNumber") ?? 0, 4201);
    expect_eq(findPlistInt(INFO_PLIST, "NegativeOffset") ?? 0, -128);
    expect_eq(findPlistBool(INFO_PLIST, "NSHighResolutionCapable") ?? false, true);
    expect_eq(findPlistBool(INFO_PLIST, "LSUIElement") ?? true, false);

    // Type mismatch & missing keys return null
    expect_eq(findPlistInt(INFO_PLIST, "CFBundleIdentifier") == null, true);
    expect_eq(findPlistString(INFO_PLIST, "BuildNumber") == null, true);
    expect_eq(findPlistBool(INFO_PLIST, "BuildNumber") == null, true);
    expect_eq(findPlistString(INFO_PLIST, "Deep") == null, true);
    expect_eq(findPlistString(INFO_PLIST, "NoSuchKey") == null, true);
});

test("4. formatPlistStringEntry XML escaping and buffer overflow protection", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const out = formatPlistStringEntry("App&Name", "A < B > \"C\" 'D'", buf[0..]) ?? unreachable;
    const expected =
        "<key>App&amp;Name</key>\n" ++
        "<string>A &lt; B &gt; &quot;C&quot; &apos;D&apos;</string>\n";
    expect_eq(std.mem.eql(u8, out, expected), true);

    // Buffer too small returns null cleanly
    expect_eq(formatPlistStringEntry("App&Name", "A < B", buf[0..20]) == null, true);
    expect_eq(formatPlistStringEntry("K", "V", buf[0..0]) == null, true);
});

test("5. Empty dict, self-closing empty string, and malformed integer/dict handling", () => {
    const empty_dict = "<plist version=\"1.0\"><dict/></plist>";
    let it_empty = PlistDictIter.init(empty_dict);
    expect_eq(it_empty.next() == null, true);

    const edge_plist =
        "<plist version=\"1.0\"><dict>" ++
        "<key>EmptyStr</key><string/>" ++
        "<key>BadInt</key><integer>12abc</integer>" ++
        "<key>OverflowInt</key><integer>99999999999999999999999</integer>" ++
        "</dict></plist>";
    expect_eq(std.mem.eql(u8, findPlistString(edge_plist, "EmptyStr") ?? "x", ""), true);
    expect_eq(findPlistInt(edge_plist, "BadInt") == null, true);
    expect_eq(findPlistInt(edge_plist, "OverflowInt") == null, true);
});
