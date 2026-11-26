// cap/test/testCapIni.cp
// Comprehensive unit tests for @cap/ini (npm ini / rust-ini parity)

import { test, expect_eq } from "cap:test";
import { IniDoc, IniWriter } from "../../packages/@cap/ini/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/ini — top-level keys and multiple [section] parsing", () => {
        const src = "global_key = root_val\n[user]\nname = Alice\nemail = alice@capdux.org\n[core]\neditor = vim\n";
        const doc: IniDoc = IniDoc.parse(src);

        expect_eq(doc.len(), 4 as usize);
        expect_eq(doc.has(src, "", "global_key"), true);
        expect_eq(sliceEq(doc.get(src, "", "global_key"), "root_val"), true);
        expect_eq(sliceEq(doc.get(src, "user", "name"), "Alice"), true);
        expect_eq(sliceEq(doc.get(src, "user", "email"), "alice@capdux.org"), true);
        expect_eq(sliceEq(doc.get(src, "core", "editor"), "vim"), true);
        expect_eq(doc.has(src, "core", "missing"), false);
        expect_eq(sliceEq(doc.get(src, "core", "missing"), ""), true);
    });

    test("2. @cap/ini — ';' and '#' comments & CRLF line endings", () => {
        const src = "; top comment\r\n# hash comment\r\n[net]\r\nhost = 127.0.0.1 ; inline\r\nport = 9000 # inline2\r\n";
        const doc: IniDoc = IniDoc.parse(src);

        expect_eq(doc.len(), 2 as usize);
        expect_eq(sliceEq(doc.get(src, "net", "host"), "127.0.0.1"), true);
        expect_eq(sliceEq(doc.get(src, "net", "port"), "9000"), true);
    });

    test("3. @cap/ini — double/single quoted values preserve ';' and surrounding spaces", () => {
        const src = "[fmt]\ngreeting = \"  hello ; world  \"\ntag = 'a # b'\n";
        const doc: IniDoc = IniDoc.parse(src);

        expect_eq(sliceEq(doc.get(src, "fmt", "greeting"), "  hello ; world  "), true);
        expect_eq(sliceEq(doc.get(src, "fmt", "tag"), "a # b"), true);
    });

    test("4. @cap/ini — getBool & getInt with typed parsing and default fallbacks", () => {
        const src = "[flags]\na = true\nb = YES\nc = on\nd = 1\ne = false\nf = no\ng = off\nh = 0\ni = maybe\n[nums]\np = 8080\nn = -42\nbad = 12x\n";
        const doc: IniDoc = IniDoc.parse(src);

        expect_eq(doc.getBool(src, "flags", "a", false), true);
        expect_eq(doc.getBool(src, "flags", "b", false), true);
        expect_eq(doc.getBool(src, "flags", "c", false), true);
        expect_eq(doc.getBool(src, "flags", "d", false), true);
        expect_eq(doc.getBool(src, "flags", "e", true), false);
        expect_eq(doc.getBool(src, "flags", "f", true), false);
        expect_eq(doc.getBool(src, "flags", "g", true), false);
        expect_eq(doc.getBool(src, "flags", "h", true), false);
        expect_eq(doc.getBool(src, "flags", "i", true), true);
        expect_eq(doc.getBool(src, "flags", "missing", false), false);

        expect_eq(doc.getInt(src, "nums", "p", 0), 8080 as i64);
        expect_eq(doc.getInt(src, "nums", "n", 0), -42 as i64);
        expect_eq(doc.getInt(src, "nums", "bad", 99), 99 as i64);
        expect_eq(doc.getInt(src, "nums", "missing", 77), 77 as i64);
    });

    test("5. @cap/ini — IniWriter serialize & IniDoc.parse round-trip", () => {
        let buf: [256]u8 = undefined;
        let w: IniWriter = IniWriter.init(buf[0..256]);
        expect_eq(w.writeEntry("app", "cap"), true);
        expect_eq(w.writeSection("db"), true);
        expect_eq(w.writeEntry("port", "5432"), true);
        expect_eq(w.writeEntry("note", "semi;colon # hash"), true);
        expect_eq(w.writeEntry("padded", "  edge  "), true);

        const text: []const u8 = w.written();
        expect_eq(w.len(), text.len);
        const doc: IniDoc = IniDoc.parse(text);

        expect_eq(sliceEq(doc.get(text, "", "app"), "cap"), true);
        expect_eq(doc.getInt(text, "db", "port", 0), 5432 as i64);
        expect_eq(sliceEq(doc.get(text, "db", "note"), "semi;colon # hash"), true);
        expect_eq(sliceEq(doc.get(text, "db", "padded"), "  edge  "), true);
    });

    test("6. @cap/ini — empty document & IniWriter buffer overflow protection", () => {
        const empty_doc: IniDoc = IniDoc.parse("");
        expect_eq(empty_doc.len(), 0 as usize);
        expect_eq(empty_doc.has("", "", "k"), false);

        let tiny: [8]u8 = undefined;
        let w: IniWriter = IniWriter.init(tiny[0..8]);
        expect_eq(w.writeSection("longsection"), false);
        expect_eq(w.overflow, true);
        expect_eq(w.writeEntry("a", "b"), false);
    });
}
