import { test, expect_eq } from "cap:test";
import {
    QueryPair,
    encodeComponent,
    decodeComponent,
    parseRaw,
    countPairs,
    getDecoded,
    stringify,
} from "../../packages/qs/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
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
    test("1. @cap/qs-lite — parseRaw with leading '?' and bare flags", () => {
        let pairs: [8]QueryPair = undefined;
        let n: usize = parseRaw("?foo=bar&baz=123&flag&&empty=", pairs[0..]);
        expect_eq(n, 4);
        expect_eq(strEq(pairs[0].key, "foo"), true);
        expect_eq(strEq(pairs[0].value, "bar"), true);
        expect_eq(strEq(pairs[1].key, "baz"), true);
        expect_eq(strEq(pairs[1].value, "123"), true);
        expect_eq(strEq(pairs[2].key, "flag"), true);
        expect_eq(strEq(pairs[2].value, ""), true);
        expect_eq(strEq(pairs[3].key, "empty"), true);
        expect_eq(strEq(pairs[3].value, ""), true);
        expect_eq(countPairs("?foo=bar&baz=123&flag&&empty="), 4);
    });

    test("2. @cap/qs-lite — encodeComponent and decodeComponent (+ vs %20)", () => {
        let buf: [128]u8 = undefined;
        let n1: usize = encodeComponent("hello world/a=b&c", true, buf[0..]);
        expect_eq(strEq(buf[0..n1], "hello+world%2Fa%3Db%26c"), true);

        let dec_buf: [128]u8 = undefined;
        let d1: usize = decodeComponent(buf[0..n1], true, dec_buf[0..]);
        expect_eq(strEq(dec_buf[0..d1], "hello world/a=b&c"), true);

        let n2: usize = encodeComponent("a b", false, buf[0..]);
        expect_eq(strEq(buf[0..n2], "a%20b"), true);
    });

    test("3. @cap/qs-lite — getDecoded lookup with encoded keys & values", () => {
        let q: string = "?user%20name=Alice+Smith&role=admin&lang=zh%2DCN";
        let val: [64]u8 = undefined;
        let vlen: usize = 0;

        expect_eq(getDecoded(q, "user name", val[0..], &vlen), true);
        expect_eq(strEq(val[0..vlen], "Alice Smith"), true);

        expect_eq(getDecoded(q, "lang", val[0..], &vlen), true);
        expect_eq(strEq(val[0..vlen], "zh-CN"), true);

        expect_eq(getDecoded(q, "missing", val[0..], &vlen), false);
        expect_eq(vlen, 0);
    });

    test("4. @cap/qs-lite — stringify roundtrip", () => {
        let pairs: [3]QueryPair = undefined;
        pairs[0] = QueryPair { key: "q", value: "cp lang" };
        pairs[1] = QueryPair { key: "page", value: "2" };
        pairs[2] = QueryPair { key: "tag", value: "a&b" };

        let buf: [128]u8 = undefined;
        let n: usize = stringify(pairs[0..3], true, buf[0..]);
        expect_eq(strEq(buf[0..n], "q=cp+lang&page=2&tag=a%26b"), true);

        let v: [64]u8 = undefined;
        let vl: usize = 0;
        expect_eq(getDecoded(buf[0..n], "tag", v[0..], &vl), true);
        expect_eq(strEq(v[0..vl], "a&b"), true);
    });

    test("5. @cap/qs-lite — malformed percent escape passthrough", () => {
        let out: [32]u8 = undefined;
        let n: usize = decodeComponent("100%ZZok%", true, out[0..]);
        expect_eq(strEq(out[0..n], "100%ZZok%"), true);
    });

    test("6. @cap/qs-lite — empty input and tiny buffer protection", () => {
        expect_eq(countPairs(""), 0);
        expect_eq(countPairs("?"), 0);
        let pairs: [1]QueryPair = undefined;
        pairs[0] = QueryPair { key: "longkey", value: "longvalue" };
        let tiny: [4]u8 = undefined;
        expect_eq(stringify(pairs[0..1], true, tiny[0..]), 0);
    });
}
