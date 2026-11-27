import { test, expect_eq } from "cap:test";
import {
    JpSegKind,
    JpSegment,
    parseJsonPath,
    formatJsonPath,
} from "../../packages/@cap/jsonpath-rust/src/index.cp";

fn makeEmptySeg(): JpSegment {
    return JpSegment{
        kind: JpSegKind.Root,
        name: "",
        index: 0,
        slice_start: 0,
        slice_end: 0,
        has_start: false,
        has_end: false,
    };
}

fn main() {
    test("1. @cap/jsonpath-rust — parses dot properties and array index", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        const n = parseJsonPath("$.store.book[0].title", segs[0..8]);
        expect_eq(n, 5 as i64);
        expect_eq(segs[0].kind == JpSegKind.Root, true);
        expect_eq(segs[1].kind == JpSegKind.Property, true);
        expect_eq(segs[1].name, "store");
        expect_eq(segs[2].name, "book");
        expect_eq(segs[3].kind == JpSegKind.Index, true);
        expect_eq(segs[3].index, 0 as i64);
        expect_eq(segs[4].name, "title");
    });

    test("2. @cap/jsonpath-rust — parses recursive descent and wildcards", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        const n1 = parseJsonPath("$..author", segs[0..8]);
        expect_eq(n1, 2 as i64);
        expect_eq(segs[1].kind == JpSegKind.RecursiveProp, true);
        expect_eq(segs[1].name, "author");

        const n2 = parseJsonPath("$.items[*]..*", segs[0..8]);
        expect_eq(n2, 4 as i64);
        expect_eq(segs[2].kind == JpSegKind.Wildcard, true);
        expect_eq(segs[3].kind == JpSegKind.RecursiveWildcard, true);
    });

    test("3. @cap/jsonpath-rust — parses array slices and negative indices", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        const n = parseJsonPath("$.arr[1:4][-1][2:]", segs[0..8]);
        expect_eq(n, 5 as i64);
        expect_eq(segs[2].kind == JpSegKind.Slice, true);
        expect_eq(segs[2].has_start, true);
        expect_eq(segs[2].has_end, true);
        expect_eq(segs[2].slice_start, 1 as i64);
        expect_eq(segs[2].slice_end, 4 as i64);
        expect_eq(segs[3].kind == JpSegKind.Index, true);
        expect_eq(segs[3].index, -1 as i64);
        expect_eq(segs[4].kind == JpSegKind.Slice, true);
        expect_eq(segs[4].has_start, true);
        expect_eq(segs[4].has_end, false);
        expect_eq(segs[4].slice_start, 2 as i64);
    });

    test("4. @cap/jsonpath-rust — parses quoted bracket property keys", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        const n = parseJsonPath("$['quoted.key'][\"a b\"]", segs[0..8]);
        expect_eq(n, 3 as i64);
        expect_eq(segs[1].kind == JpSegKind.Property, true);
        expect_eq(segs[1].name, "quoted.key");
        expect_eq(segs[2].name, "a b");
    });

    test("5. @cap/jsonpath-rust — formatJsonPath round-trips canonical JSONPath", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        let buf: [128]u8 = [0; 128];

        const n1 = parseJsonPath("$.store.book[0].title", segs[0..8]);
        const len1 = formatJsonPath(segs[0..(n1 as usize)], buf[0..128]);
        expect_eq(buf[0..len1] as string, "$.store.book[0].title");

        const n2 = parseJsonPath("$..author['first.name'][1:4][*]", segs[0..8]);
        const len2 = formatJsonPath(segs[0..(n2 as usize)], buf[0..128]);
        expect_eq(buf[0..len2] as string, "$..author['first.name'][1:4][*]");
    });

    test("6. @cap/jsonpath-rust — rejects malformed JSONPath expressions", () => {
        let segs: [8]JpSegment = [
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
            makeEmptySeg(), makeEmptySeg(), makeEmptySeg(), makeEmptySeg(),
        ];
        expect_eq(parseJsonPath("", segs[0..8]), -1 as i64);
        expect_eq(parseJsonPath("store.book", segs[0..8]), -1 as i64);
        expect_eq(parseJsonPath("$.", segs[0..8]), -1 as i64);
        expect_eq(parseJsonPath("$.arr[]", segs[0..8]), -1 as i64);
        expect_eq(parseJsonPath("$['unterminated]", segs[0..8]), -1 as i64);
    });
}
