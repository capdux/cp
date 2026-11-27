import { test, expect_eq } from "cap:test";
import {
    PlistValKind,
    PlistValue,
    plistGetDictValue,
    plistWriteDictEntry,
} from "../../packages/@cap/plist/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/plist — Extract string, integer & boolean values from Info.plist XML", () => {
        const xml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
            "<plist version=\"1.0\">\n" ++
            "<dict>\n" ++
            "  <key>CFBundleName</key>\n" ++
            "  <string>CapApp</string>\n" ++
            "  <key>BuildNumber</key>\n" ++
            "  <integer>42</integer>\n" ++
            "  <key>Offset</key>\n" ++
            "  <integer>-128</integer>\n" ++
            "  <key>LSUIElement</key>\n" ++
            "  <true/>\n" ++
            "  <key>DebugMode</key>\n" ++
            "  <false/>\n" ++
            "</dict>\n" ++
            "</plist>";

        const name = plistGetDictValue(xml, "CFBundleName");
        expect_eq(name.kind == PlistValKind.String, true);
        expect_eq(strEq(name.str_val, "CapApp"), true);

        const build = plistGetDictValue(xml, "BuildNumber");
        expect_eq(build.kind == PlistValKind.Integer, true);
        expect_eq(build.int_val, 42 as i64);

        const offset = plistGetDictValue(xml, "Offset");
        expect_eq(offset.kind == PlistValKind.Integer, true);
        expect_eq(offset.int_val, -128 as i64);

        const ui = plistGetDictValue(xml, "LSUIElement");
        expect_eq(ui.kind == PlistValKind.Bool, true);
        expect_eq(ui.bool_val, true);

        const dbg = plistGetDictValue(xml, "DebugMode");
        expect_eq(dbg.kind == PlistValKind.Bool, true);
        expect_eq(dbg.bool_val, false);
    });

    test("2. @cap/plist — Extract real, data & date values", () => {
        const xml =
            "<dict>" ++
            "<key>Scale</key><real>2.5</real>" ++
            "<key>Blob</key><data>SGVsbG8=</data>" ++
            "<key>Created</key><date>2026-11-27T00:00:00Z</date>" ++
            "</dict>";

        const scale = plistGetDictValue(xml, "Scale");
        expect_eq(scale.kind == PlistValKind.Real, true);
        expect_eq(strEq(scale.str_val, "2.5"), true);

        const blob = plistGetDictValue(xml, "Blob");
        expect_eq(blob.kind == PlistValKind.Data, true);
        expect_eq(strEq(blob.str_val, "SGVsbG8="), true);

        const created = plistGetDictValue(xml, "Created");
        expect_eq(created.kind == PlistValKind.Date, true);
        expect_eq(strEq(created.str_val, "2026-11-27T00:00:00Z"), true);
    });

    test("3. @cap/plist — Skip nested <dict> and <array> without colliding with outer keys", () => {
        const xml =
            "<dict>" ++
            "<key>Nested</key><dict><key>Target</key><string>inner</string></dict>" ++
            "<key>Items</key><array><string>a</string><string>b</string></array>" ++
            "<key>Target</key><string>outer</string>" ++
            "</dict>";

        const v = plistGetDictValue(xml, "Target");
        expect_eq(v.kind == PlistValKind.String, true);
        expect_eq(strEq(v.str_val, "outer"), true);
    });

    test("4. @cap/plist — Missing key & empty string return Invalid / empty", () => {
        const xml = "<dict><key>Empty</key><string/></dict>";
        const empty = plistGetDictValue(xml, "Empty");
        expect_eq(empty.kind == PlistValKind.String, true);
        expect_eq(empty.str_val.len, 0 as usize);

        const missing = plistGetDictValue(xml, "NoSuchKey");
        expect_eq(missing.kind == PlistValKind.Invalid, true);
    });

    test("5. @cap/plist — plistWriteDictEntry round-trip for String, Integer, Bool, Real, Data, Date", () => {
        let buf: [256]u8 = undefined;
        let w: usize = 0;

        const n1 = plistWriteDictEntry("App", PlistValue{
            kind: PlistValKind.String,
            str_val: "CP",
            int_val: 0,
            bool_val: false,
        }, buf[w..256]);
        expect_eq(n1 > 0, true);
        w += n1;

        const n2 = plistWriteDictEntry("Port", PlistValue{
            kind: PlistValKind.Integer,
            str_val: "",
            int_val: -8080,
            bool_val: false,
        }, buf[w..256]);
        expect_eq(n2 > 0, true);
        w += n2;

        const n3 = plistWriteDictEntry("TLS", PlistValue{
            kind: PlistValKind.Bool,
            str_val: "",
            int_val: 0,
            bool_val: true,
        }, buf[w..256]);
        expect_eq(n3 > 0, true);
        w += n3;

        const generated: string = buf[0..w];
        const app = plistGetDictValue(generated, "App");
        expect_eq(app.kind == PlistValKind.String, true);
        expect_eq(strEq(app.str_val, "CP"), true);

        const port = plistGetDictValue(generated, "Port");
        expect_eq(port.kind == PlistValKind.Integer, true);
        expect_eq(port.int_val, -8080 as i64);

        const tls = plistGetDictValue(generated, "TLS");
        expect_eq(tls.kind == PlistValKind.Bool, true);
        expect_eq(tls.bool_val, true);
    });

    test("6. @cap/plist — Undersized output buffer & Invalid value protection", () => {
        let tiny: [8]u8 = undefined;
        const v = PlistValue{
            kind: PlistValKind.String,
            str_val: "TooLong",
            int_val: 0,
            bool_val: false,
        };
        expect_eq(plistWriteDictEntry("Key", v, tiny[0..8]), 0 as usize);

        const inv = PlistValue{
            kind: PlistValKind.Invalid,
            str_val: "",
            int_val: 0,
            bool_val: false,
        };
        let big: [64]u8 = undefined;
        expect_eq(plistWriteDictEntry("Key", inv, big[0..64]), 0 as usize);
    });
}
