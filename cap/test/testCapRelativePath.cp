import { test, expect_eq } from "cap:test";
import { RelPath, normalizeRelPath, joinRelPath, relativeTo } from "@cap/relative-path";

fn main() {
    test("1. @cap/relative-path — fromStr converts backslashes, strips leading/trailing slashes", () => {
        let p1 = RelPath.fromStr("/foo\\bar//baz/");
        expect_eq(p1.equals("foo/bar/baz"), true);
        let p2 = RelPath.fromStr("///");
        expect_eq(p2.isEmpty(), true);
    });

    test("2. @cap/relative-path — lexical normalize resolves '.' and '..' while preserving leading '..'", () => {
        let p1 = normalizeRelPath("a/b/../c/./d");
        expect_eq(p1.equals("a/c/d"), true);
        let p2 = normalizeRelPath("../a/../../b/c");
        expect_eq(p2.equals("../../b/c"), true);
        let p3 = normalizeRelPath("././.");
        expect_eq(p3.isEmpty(), true);
    });

    test("3. @cap/relative-path — join, joinNormalized & toLogicalPath", () => {
        let base = RelPath.fromStr("src/utils");
        let joined = base.join("../core/mod.cp");
        expect_eq(joined.equals("src/utils/../core/mod.cp"), true);
        let norm = joinRelPath("src/utils", "../core/mod.cp");
        expect_eq(norm.equals("src/core/mod.cp"), true);
        let rel = RelPath.fromStr("../assets/logo.png");
        let logical = rel.toLogicalPath("app/web/pages");
        expect_eq(logical.equals("app/web/assets/logo.png"), true);
    });

    test("4. @cap/relative-path — parent, fileNameInto, extensionInto & withExtension", () => {
        let p = RelPath.fromStr("dir/sub/archive.tar.gz");
        let par = p.parent();
        expect_eq(par != null, true);
        if (pv := par) {
            expect_eq(pv.equals("dir/sub"), true);
        }

        let fbuf: [64]u8 = [0; 64];
        let flen = p.fileNameInto(fbuf[0..64]);
        expect_eq(flen != null, true);
        if (n := flen) {
            expect_eq(n, 14 as usize);
        }

        let ebuf: [32]u8 = [0; 32];
        let elen = p.extensionInto(ebuf[0..32]);
        expect_eq(elen != null, true);
        if (n := elen) {
            expect_eq(n, 2 as usize);
            expect_eq(ebuf[0] == 'g' && ebuf[1] == 'z', true);
        }

        let replaced = p.withExtension("bz2");
        expect_eq(replaced.equals("dir/sub/archive.tar.bz2"), true);
        let stripped = p.withExtension("");
        expect_eq(stripped.equals("dir/sub/archive.tar"), true);
    });

    test("5. @cap/relative-path — hasPrefixPath & stripPrefix component boundary safety", () => {
        let p = RelPath.fromStr("foo/bar/baz.txt");
        expect_eq(p.hasPrefixPath("foo/bar"), true);
        expect_eq(p.hasPrefixPath("foo/ba"), false);

        let rest = p.stripPrefix("foo/bar");
        expect_eq(rest != null, true);
        if (rv := rest) {
            expect_eq(rv.equals("baz.txt"), true);
        }
        let bad = p.stripPrefix("foo/ba");
        expect_eq(bad == null, true);
    });

    test("6. @cap/relative-path — relativeTo computes sibling & nested relative hops", () => {
        let r1 = relativeTo("a/b/c", "a/d/e");
        expect_eq(r1.equals("../../d/e"), true);
        let r2 = relativeTo("a/b", "a/b/c/d");
        expect_eq(r2.equals("c/d"), true);
        let r3 = relativeTo("a/b", "a/b");
        expect_eq(r3.isEmpty(), true);
    });
}
