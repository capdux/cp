import { test, expect_eq } from "cap:test";
import {
    Purl,
    parsePurl,
    isValidType,
    getQualifier,
    formatPurl,
} from "../../packages/packageurl-js/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/purl — Scoped npm package pkg:npm/%40angular/core@16.0.0", () => {
        const p = parsePurl("pkg:npm/%40angular/core@16.0.0");
        expect_eq(p.valid, true);
        expect_eq(testEq(p.pkg_type, "npm"), true);
        expect_eq(testEq(p.namespace, "%40angular"), true);
        expect_eq(testEq(p.name, "core"), true);
        expect_eq(testEq(p.version, "16.0.0"), true);
        expect_eq(testEq(p.qualifiers, ""), true);
        expect_eq(testEq(p.subpath, ""), true);
    });

    test("2. @cap/purl — Cargo package without namespace pkg:cargo/serde@1.0.210", () => {
        const p = parsePurl("PKG:cargo/serde@1.0.210");
        expect_eq(p.valid, true);
        expect_eq(testEq(p.pkg_type, "cargo"), true);
        expect_eq(testEq(p.namespace, ""), true);
        expect_eq(testEq(p.name, "serde"), true);
        expect_eq(testEq(p.version, "1.0.210"), true);
    });

    test("3. @cap/purl — Golang multi-segment namespace & #subpath", () => {
        const p = parsePurl("pkg:golang/github.com/gorilla/mux@v1.8.0#sub/dir");
        expect_eq(p.valid, true);
        expect_eq(testEq(p.pkg_type, "golang"), true);
        expect_eq(testEq(p.namespace, "github.com/gorilla"), true);
        expect_eq(testEq(p.name, "mux"), true);
        expect_eq(testEq(p.version, "v1.8.0"), true);
        expect_eq(testEq(p.subpath, "sub/dir"), true);
    });

    test("4. @cap/purl — Qualifiers parsing & case-insensitive getQualifier", () => {
        const p = parsePurl("pkg:deb/debian/curl@7.50.3-1?arch=amd64&distro=jessie");
        expect_eq(p.valid, true);
        expect_eq(testEq(getQualifier(p, "arch"), "amd64"), true);
        expect_eq(testEq(getQualifier(p, "DISTRO"), "jessie"), true);
        expect_eq(testEq(getQualifier(p, "missing"), ""), true);
    });

    test("5. @cap/purl — formatPurl canonical lowercase-type roundtrip", () => {
        const orig = Purl {
            pkg_type: "Maven",
            namespace: "org.apache.xmlgraphics",
            name: "batik-anim",
            version: "1.9.1",
            qualifiers: "classifier=sources",
            subpath: "src/main",
            valid: true,
        };
        let buf: [256]u8 = undefined;
        const n = formatPurl(orig, buf[0..256]);
        const expected = "pkg:maven/org.apache.xmlgraphics/batik-anim@1.9.1?classifier=sources#src/main";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);

        const reparsed = parsePurl(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.pkg_type, "maven"), true);
        expect_eq(testEq(reparsed.name, "batik-anim"), true);
    });

    test("6. @cap/purl — Rejects missing pkg:, invalid type, empty name & undersized buffer", () => {
        expect_eq(isValidType("cargo"), true);
        expect_eq(isValidType("c-plus+1.0"), true);
        expect_eq(isValidType("1bad"), false);
        expect_eq(isValidType("bad_type"), false);

        expect_eq(parsePurl("npm/lodash@4.17.21").valid, false);
        expect_eq(parsePurl("pkg:123/foo").valid, false);
        expect_eq(parsePurl("pkg:npm/").valid, false);
        expect_eq(parsePurl("pkg:npm/@1.0.0").valid, false);

        const valid_p = parsePurl("pkg:cargo/serde@1.0.210");
        let tiny: [6]u8 = undefined;
        expect_eq(formatPurl(valid_p, tiny[0..6]), 0 as usize);
    });
}
