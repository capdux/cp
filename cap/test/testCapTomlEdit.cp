// cap/test/testCapTomlEdit.cp
// Comprehensive unit tests for @cap/toml-edit

import { test, expect_eq } from "cap:test";
import {
    TOML_STR,
    TOML_INT,
    TOML_BOOL,
    TomlEntry,
    TomlDoc,
} from "../../packages/@cap/toml-edit/src/index.cp";

fn strEq(a: string, b: string) -> bool {
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
    test("1. @cap/toml-edit — parse root keys, [table], [a.b], comments & strings", () => {
        let src = "# top comment\ntitle = \"CP Engine\" # inline\n[package]\nname = 'cap-core'\n[server.http]\nhost = \"127.0.0.1\"\n";
        let doc = TomlDoc.parse(src);

        expect_eq(doc.len(), 3 as usize);
        expect_eq(doc.hasKey("", "title"), true);
        expect_eq(doc.hasKey("package", "name"), true);
        expect_eq(doc.hasKey("server.http", "host"), true);
        expect_eq(doc.hasKey("package", "missing"), false);

        expect_eq(strEq(doc.getStr("", "title") ?? "", "CP Engine"), true);
        expect_eq(strEq(doc.getStr("package", "name") ?? "", "cap-core"), true);
        expect_eq(strEq(doc.getStr("server.http", "host") ?? "", "127.0.0.1"), true);
        expect_eq(doc.getInt("", "title") == null, true);
    });

    test("2. @cap/toml-edit — integers with underscores, negative numbers & booleans", () => {
        let src = "[limits]\nmax_conn = 1_000_000\noffset = -42_500\nzero = 0\nenabled = true # flag\ndebug = false\n";
        let doc = TomlDoc.parse(src);

        expect_eq(doc.len(), 5 as usize);
        expect_eq(doc.getInt("limits", "max_conn") ?? (0 as i64), 1000000 as i64);
        expect_eq(doc.getInt("limits", "offset") ?? (0 as i64), -42500 as i64);
        expect_eq(doc.getInt("limits", "zero") ?? (-1 as i64), 0 as i64);
        expect_eq(doc.getBool("limits", "enabled") ?? false, true);
        expect_eq(doc.getBool("limits", "debug") ?? true, false);
        expect_eq(doc.getStr("limits", "enabled") == null, true);
    });

    test("3. @cap/toml-edit — dotted keys at root (a.b = 1, a.b.c = 'ok')", () => {
        let src = "db.port = 5_432\ndb.ssl = true\nnet.tls.mode = 'strict'\n";
        let doc = TomlDoc.parse(src);

        expect_eq(doc.len(), 3 as usize);
        expect_eq(doc.getInt("db", "port") ?? (0 as i64), 5432 as i64);
        expect_eq(doc.getBool("db", "ssl") ?? false, true);
        expect_eq(strEq(doc.getStr("net.tls", "mode") ?? "", "strict"), true);
    });

    test("4. @cap/toml-edit — setStr & setInt in-place update and upsert", () => {
        let doc = TomlDoc.new();
        expect_eq(doc.len(), 0 as usize);
        expect_eq(TOML_STR, 1 as u8);
        expect_eq(TOML_INT, 2 as u8);
        expect_eq(TOML_BOOL, 3 as u8);

        expect_eq(doc.setStr("app", "env", "dev"), true);
        expect_eq(doc.setInt("app", "workers", 4 as i64), true);
        expect_eq(doc.len(), 2 as usize);

        // Overwrite existing keys without increasing len
        expect_eq(doc.setStr("app", "env", "prod"), true);
        expect_eq(doc.setInt("app", "workers", 16 as i64), true);
        expect_eq(doc.len(), 2 as usize);
        expect_eq(strEq(doc.getStr("app", "env") ?? "", "prod"), true);
        expect_eq(doc.getInt("app", "workers") ?? (0 as i64), 16 as i64);
    });

    test("5. @cap/toml-edit — format canonical TOML & parse round-trip", () => {
        let doc = TomlDoc.new();
        * = doc.setStr("", "version", "1.0");
        * = doc.setStr("pkg", "name", "slotmap");
        * = doc.setInt("pkg", "edition", 2024 as i64);
        * = doc.setBool("pkg", "no_std", true);
        * = doc.setInt("net", "timeout", -1 as i64);

        let buf: [256]u8 = undefined;
        let n = doc.format(buf[0..256]);
        expect_eq(n > (0 as usize), true);

        let expected = "version = \"1.0\"\n\n[pkg]\nname = \"slotmap\"\nedition = 2024\nno_std = true\n\n[net]\ntimeout = -1\n";
        expect_eq(n, expected.len);
        let i: usize = 0;
        while (i < n) {
            expect_eq(buf[i], expected[i]);
            i += 1;
        }
    });

    test("6. @cap/toml-edit — malformed lines ignored & capacity limit (64 entries)", () => {
        let doc = TomlDoc.parse("invalid_line_no_eq\n= missing_key\nbad_int = 12__34\nok = 7\n");
        expect_eq(doc.len(), 1 as usize);
        expect_eq(doc.getInt("", "ok") ?? (0 as i64), 7 as i64);

        // Fill up to 64 distinct entries using empty doc
        let full = TomlDoc.new();
        let keys = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-";
        let idx: usize = 0;
        while (idx < (64 as usize)) {
            expect_eq(full.setInt("", keys[idx..(idx + (1 as usize))], idx as i64), true);
            idx += 1;
        }
        expect_eq(full.len(), 64 as usize);
        expect_eq(full.setInt("", "overflow", 99 as i64), false);
        expect_eq(full.len(), 64 as usize);
    });
}
