import { test, expect_eq } from "cap:test";
import {
    QsPair,
    QsPath,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
} from "../../packages/serde_qs/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/serde-qs — parseKeyPath nested objects, indexed arrays & empty brackets", () => {
        const p1 = parseKeyPath("user[address][city]");
        expect_eq(p1.depth, 3 as usize);
        expect_eq(eqStr(p1.segments[0], "user"), true);
        expect_eq(eqStr(p1.segments[1], "address"), true);
        expect_eq(eqStr(p1.segments[2], "city"), true);

        const p2 = parseKeyPath("items[0]");
        expect_eq(p2.depth, 2 as usize);
        expect_eq(eqStr(p2.segments[0], "items"), true);
        expect_eq(eqStr(p2.segments[1], "0"), true);

        const p3 = parseKeyPath("tags[]");
        expect_eq(p3.depth, 2 as usize);
        expect_eq(eqStr(p3.segments[0], "tags"), true);
        expect_eq(eqStr(p3.segments[1], ""), true);

        const p4 = parseKeyPath("simple");
        expect_eq(p4.depth, 1 as usize);
        expect_eq(eqStr(p4.segments[0], "simple"), true);
    });

    test("2. @cap/serde-qs — parseQs skips leading '?' and splits pairs cleanly", () => {
        let pairs: [8]QsPair = undefined;
        const n = parseQs("?user[name]=Alice&items[0]=42&flag&&tags[]=cp", pairs[0..]);
        expect_eq(n, 4 as usize);

        expect_eq(eqStr(pairs[0].key, "user[name]"), true);
        expect_eq(eqStr(pairs[0].value, "Alice"), true);

        expect_eq(eqStr(pairs[1].key, "items[0]"), true);
        expect_eq(eqStr(pairs[1].value, "42"), true);

        expect_eq(eqStr(pairs[2].key, "flag"), true);
        expect_eq(eqStr(pairs[2].value, ""), true);

        expect_eq(eqStr(pairs[3].key, "tags[]"), true);
        expect_eq(eqStr(pairs[3].value, "cp"), true);
    });

    test("3. @cap/serde-qs — getQsValueDecoded with '+' spaces & percent-encoding", () => {
        let out: [128]u8 = undefined;
        const q = "?user[name]=Alice+Smith&msg=hello%20world%21&city=%E5%8C%97%E4%BA%AC";

        const n1 = getQsValueDecoded(q, "user[name]", out[0..]);
        expect_eq(n1 != null, true);
        expect_eq(eqStr(out[0..(n1 ?? 0)], "Alice Smith"), true);

        const n2 = getQsValueDecoded(q, "msg", out[0..]);
        expect_eq(n2 != null, true);
        expect_eq(eqStr(out[0..(n2 ?? 0)], "hello world!"), true);

        const n3 = getQsValueDecoded(q, "city", out[0..]);
        expect_eq(n3 != null, true);
        expect_eq(eqStr(out[0..(n3 ?? 0)], "北京"), true);

        const missing = getQsValueDecoded(q, "nonexistent", out[0..]);
        expect_eq(missing == null, true);
    });

    test("4. @cap/serde-qs — getQsValueDecoded matches percent-encoded bracket keys", () => {
        let out: [128]u8 = undefined;
        const q = "user%5Baddress%5D%5Bcity%5D=New+York";

        const n = getQsValueDecoded(q, "user[address][city]", out[0..]);
        expect_eq(n != null, true);
        expect_eq(eqStr(out[0..(n ?? 0)], "New York"), true);
    });

    test("5. @cap/serde-qs — serializeQs encodes values & preserves bracket keys", () => {
        let pairs: [3]QsPair = [
            QsPair { key: "user[name]", value: "Alice Smith" },
            QsPair { key: "items[0]", value: "a&b=c" },
            QsPair { key: "tags[]", value: "100%" },
        ];
        let out: [256]u8 = undefined;
        const n = serializeQs(pairs[0..], out[0..]);
        expect_eq(eqStr(out[0..n], "user[name]=Alice%20Smith&items[0]=a%26b%3Dc&tags[]=100%25"), true);
    });

    test("6. @cap/serde-qs — full serializeQs -> parseQs -> getQsValueDecoded round-trip", () => {
        let src: [2]QsPair = [
            QsPair { key: "filter[q]", value: "hello + world" },
            QsPair { key: "filter[page]", value: "2" },
        ];
        let qsBuf: [256]u8 = undefined;
        const qsLen = serializeQs(src[0..], qsBuf[0..]);
        const qs = qsBuf[0..qsLen];

        let parsed: [4]QsPair = undefined;
        const count = parseQs(qs, parsed[0..]);
        expect_eq(count, 2 as usize);

        const p = parseKeyPath(parsed[0].key);
        expect_eq(p.depth, 2 as usize);
        expect_eq(eqStr(p.segments[0], "filter"), true);
        expect_eq(eqStr(p.segments[1], "q"), true);

        let decBuf: [128]u8 = undefined;
        const d1 = getQsValueDecoded(qs, "filter[q]", decBuf[0..]);
        expect_eq(d1 != null, true);
        expect_eq(eqStr(decBuf[0..(d1 ?? 0)], "hello + world"), true);

        const d2 = getQsValueDecoded(qs, "filter[page]", decBuf[0..]);
        expect_eq(d2 != null, true);
        expect_eq(eqStr(decBuf[0..(d2 ?? 0)], "2"), true);
    });
}
