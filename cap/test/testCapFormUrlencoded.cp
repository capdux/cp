import { test, expect_eq } from "cap:test";
import {
    FormPair,
    FormSerializer,
    percentEncode,
    percentDecode,
    parseFormPairs,
    getParamDecoded,
} from "../../packages/querystringify/src/index.cp";

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

fn main() {
    test("1. @cap/form-urlencoded — percentEncode/Decode (+ vs %20) & UTF-8 round-trip", () => {
        let enc_buf: [128]u8 = [0; 128];
        let dec_buf: [128]u8 = [0; 128];

        let n1: usize = percentEncode("hello world! a-b_c.d*e", true, enc_buf[0..128]);
        expect_eq(eqStr(enc_buf[0..128], n1, "hello+world%21+a-b_c.d*e"), true);

        let n2: usize = percentEncode("hello world!", false, enc_buf[0..128]);
        expect_eq(eqStr(enc_buf[0..128], n2, "hello%20world%21"), true);

        // UTF-8 Chinese round-trip
        let zh: string = "中文 100%";
        let n3: usize = percentEncode(zh, true, enc_buf[0..128]);
        expect_eq(n3 > zh.len, true);
        let d3: usize = percentDecode(enc_buf[0..n3], true, dec_buf[0..128]);
        expect_eq(eqStr(dec_buf[0..128], d3, zh), true);
    });

    test("2. @cap/form-urlencoded — percentDecode tolerant preservation of invalid %2G", () => {
        let out: [64]u8 = [0; 64];
        let n1: usize = percentDecode("a%2Gb%20c%2f%", false, out[0..64]);
        expect_eq(eqStr(out[0..64], n1, "a%2Gb c/%"), true);

        let n2: usize = percentDecode("foo+bar%2Bbaz", true, out[0..64]);
        expect_eq(eqStr(out[0..64], n2, "foo bar+baz"), true);
    });

    test("3. @cap/form-urlencoded — parseFormPairs leading ?, empty segments & bare keys", () => {
        let pairs: [8]FormPair = [
            FormPair { key_start: 0, key_len: 0, val_start: 0, val_len: 0 };
            8
        ];
        let q: string = "?&&flag&a=1&b=2=3&&";
        let count: usize = parseFormPairs(q, pairs[0..8]);
        expect_eq(count, 3 as usize);

        // Pair 0: bare key "flag"
        expect_eq(eqStr(q[pairs[0].key_start..(pairs[0].key_start + pairs[0].key_len)], pairs[0].key_len, "flag"), true);
        expect_eq(pairs[0].val_len, 0 as usize);

        // Pair 1: a=1
        expect_eq(eqStr(q[pairs[1].key_start..(pairs[1].key_start + pairs[1].key_len)], pairs[1].key_len, "a"), true);
        expect_eq(eqStr(q[pairs[1].val_start..(pairs[1].val_start + pairs[1].val_len)], pairs[1].val_len, "1"), true);

        // Pair 2: b=2=3
        expect_eq(eqStr(q[pairs[2].val_start..(pairs[2].val_start + pairs[2].val_len)], pairs[2].val_len, "2=3"), true);
    });

    test("4. @cap/form-urlencoded — FormSerializer multi-pair streaming serialization", () => {
        let buf: [128]u8 = [0; 128];
        let ser: FormSerializer = FormSerializer.new();
        expect_eq(ser.appendPair(buf[0..128], "q", "hello world"), true);
        expect_eq(ser.appendPair(buf[0..128], "page", "2"), true);
        expect_eq(ser.appendPair(buf[0..128], "tag", "a&b=c"), true);
        expect_eq(ser.count, 3 as usize);
        expect_eq(eqStr(buf[0..128], ser.len, "q=hello+world&page=2&tag=a%26b%3Dc"), true);
    });

    test("5. @cap/form-urlencoded — getParamDecoded finds encoded keys & UTF-8 values", () => {
        let q: string = "?user+name=Alice+Smith&msg=%E4%B8%AD%E6%96%87&flag";
        let out: [64]u8 = [0; 64];

        let n1: i64 = getParamDecoded(q, "user name", out[0..64]);
        expect_eq(n1 > 0, true);
        expect_eq(eqStr(out[0..64], n1 as usize, "Alice Smith"), true);

        let n2: i64 = getParamDecoded(q, "msg", out[0..64]);
        expect_eq(n2 > 0, true);
        expect_eq(eqStr(out[0..64], n2 as usize, "中文"), true);

        let n3: i64 = getParamDecoded(q, "flag", out[0..64]);
        expect_eq(n3, 0 as i64);

        expect_eq(getParamDecoded(q, "missing", out[0..64]), -1 as i64);
    });

    test("6. @cap/form-urlencoded — empty input & small buffer boundary safety", () => {
        let tiny: [4]u8 = [0; 4];
        expect_eq(percentEncode("", true, tiny[0..4]), 0 as usize);
        expect_eq(percentDecode("", true, tiny[0..4]), 0 as usize);
        expect_eq(percentEncode("hello world", true, tiny[0..4]), 0 as usize);
        expect_eq(percentDecode("hello", true, tiny[0..4]), 0 as usize);

        let ser: FormSerializer = FormSerializer.new();
        expect_eq(ser.appendPair(tiny[0..4], "longkey", "val"), false);
        expect_eq(ser.len, 0 as usize);
        expect_eq(ser.count, 0 as usize);

        expect_eq(getParamDecoded("", "a", tiny[0..4]), -1 as i64);
        expect_eq(getParamDecoded("a=12345", "a", tiny[0..4]), -1 as i64);
    });
}
