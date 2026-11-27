import { test, expect_eq } from "cap:test";
import {
    IriParts,
    parseIri,
    removeDotSegments,
    resolveReference,
} from "../../packages/iri-string/src/index.cp";

fn eqStr(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn sliceEq(src: string, start: usize, len: usize, expected: string): bool {
    if (len != expected.len || start + len > src.len) {
        return false;
    }
    return eqStr(src[start..(start + len)], len, expected);
}

fn main() {
    test("1. @cap/iri-string — full 5-part URI decomposition", () => {
        let uri: string = "https://example.com:8443/v1/items?sort=asc#top";
        let p: IriParts = parseIri(uri);
        expect_eq(p.valid, true);
        expect_eq(p.has_scheme, true);
        expect_eq(p.has_auth, true);
        expect_eq(p.has_query, true);
        expect_eq(p.has_frag, true);
        expect_eq(sliceEq(uri, p.scheme_start, p.scheme_len, "https"), true);
        expect_eq(sliceEq(uri, p.auth_start, p.auth_len, "example.com:8443"), true);
        expect_eq(sliceEq(uri, p.path_start, p.path_len, "/v1/items"), true);
        expect_eq(sliceEq(uri, p.query_start, p.query_len, "sort=asc"), true);
        expect_eq(sliceEq(uri, p.frag_start, p.frag_len, "top"), true);
    });

    test("2. @cap/iri-string — UTF-8 internationalized IRI path & query", () => {
        let iri: string = "https://例え.jp/路径/文档?键=值#节";
        let p: IriParts = parseIri(iri);
        expect_eq(p.valid, true);
        expect_eq(sliceEq(iri, p.scheme_start, p.scheme_len, "https"), true);
        expect_eq(sliceEq(iri, p.auth_start, p.auth_len, "例え.jp"), true);
        expect_eq(sliceEq(iri, p.path_start, p.path_len, "/路径/文档"), true);
        expect_eq(sliceEq(iri, p.query_start, p.query_len, "键=值"), true);
        expect_eq(sliceEq(iri, p.frag_start, p.frag_len, "节"), true);
    });

    test("3. @cap/iri-string — RFC 3986 Section 5.4 official reference resolution vectors", () => {
        let base: string = "http://a/b/c/d;p?q";
        let out: [128]u8 = [0; 128];

        let n1: usize = resolveReference(base, "g", out[0..128]);
        expect_eq(eqStr(out[0..128], n1, "http://a/b/c/g"), true);

        let n2: usize = resolveReference(base, "../g", out[0..128]);
        expect_eq(eqStr(out[0..128], n2, "http://a/b/g"), true);

        let n3: usize = resolveReference(base, "/g", out[0..128]);
        expect_eq(eqStr(out[0..128], n3, "http://a/g"), true);

        let n4: usize = resolveReference(base, "?y", out[0..128]);
        expect_eq(eqStr(out[0..128], n4, "http://a/b/c/d;p?y"), true);

        let n5: usize = resolveReference(base, "#s", out[0..128]);
        expect_eq(eqStr(out[0..128], n5, "http://a/b/c/d;p?q#s"), true);

        let n6: usize = resolveReference(base, "//g", out[0..128]);
        expect_eq(eqStr(out[0..128], n6, "http://g"), true);
    });

    test("4. @cap/iri-string — removeDotSegments RFC 3986 Section 5.2.4", () => {
        let out: [128]u8 = [0; 128];

        let n1: usize = removeDotSegments("/a/b/c/./../../g", out[0..128]);
        expect_eq(eqStr(out[0..128], n1, "/a/g"), true);

        let n2: usize = removeDotSegments("mid/content=5/../6", out[0..128]);
        expect_eq(eqStr(out[0..128], n2, "mid/6"), true);

        let n3: usize = removeDotSegments("/a/b/..", out[0..128]);
        expect_eq(eqStr(out[0..128], n3, "/a/"), true);
    });

    test("5. @cap/iri-string — invalid scheme & control characters rejected", () => {
        expect_eq(parseIri("1http://example.com").valid, false);
        expect_eq(parseIri("ht!tp://example.com").valid, false);
        expect_eq(parseIri("http://example.com/a b").valid, false);
        expect_eq(parseIri("http://example.com/a\nb").valid, false);
    });

    test("6. @cap/iri-string — empty string & small buffer boundary safety", () => {
        let p_empty: IriParts = parseIri("");
        expect_eq(p_empty.valid, true);
        expect_eq(p_empty.has_scheme, false);
        expect_eq(p_empty.path_len, 0 as usize);

        let tiny: [4]u8 = [0; 4];
        expect_eq(removeDotSegments("", tiny[0..4]), 0 as usize);
        expect_eq(removeDotSegments("/very/long/path", tiny[0..4]), 0 as usize);
        expect_eq(resolveReference("http://a/b/c", "../g", tiny[0..4]), 0 as usize);
        // Base without scheme rejected
        let out: [64]u8 = [0; 64];
        expect_eq(resolveReference("/relative/base", "g", out[0..64]), 0 as usize);
    });
}
