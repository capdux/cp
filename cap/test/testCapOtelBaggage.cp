import { test, expect_eq } from "cap:test";
import {
    BaggageMember,
    parseBaggage,
    getDecodedValue,
    countMembers,
    formatBaggage,
    isValidKey,
} from "../../packages/@cap/otel-baggage/src/index.cp";

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
    test("1. @cap/otel-baggage — Standard multi-member parsing & countMembers", () => {
        const hdr = "userId=alice, serverNode=DF%2028, isProduction=false";
        expect_eq(countMembers(hdr), 3 as usize);

        let out: [4]BaggageMember = undefined;
        const n = parseBaggage(hdr, out[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(testEq(out[0].key, "userId"), true);
        expect_eq(testEq(out[0].value, "alice"), true);
        expect_eq(testEq(out[1].key, "serverNode"), true);
        expect_eq(testEq(out[1].value, "DF%2028"), true);
        expect_eq(testEq(out[2].key, "isProduction"), true);
        expect_eq(testEq(out[2].value, "false"), true);
    });

    test("2. @cap/otel-baggage — Members with optional ;properties", () => {
        const hdr = "key1=val1;ttl=60;region=us-east, key2=val2";
        let out: [4]BaggageMember = undefined;
        const n = parseBaggage(hdr, out[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(testEq(out[0].key, "key1"), true);
        expect_eq(testEq(out[0].value, "val1"), true);
        expect_eq(testEq(out[0].properties, "ttl=60;region=us-east"), true);
        expect_eq(testEq(out[1].properties, ""), true);
    });

    test("3. @cap/otel-baggage — getDecodedValue percent-decoding", () => {
        const hdr = "serverNode=DF%2028, query=a%3D1%2Cb%3B2%253";
        let buf: [64]u8 = undefined;
        let len: usize = 0;

        expect_eq(getDecodedValue(hdr, "serverNode", buf[0..64], &len), true);
        expect_eq(testEq(buf[0..len], "DF 28"), true);

        expect_eq(getDecodedValue(hdr, "query", buf[0..64], &len), true);
        expect_eq(testEq(buf[0..len], "a=1,b;2%3"), true);
    });

    test("4. @cap/otel-baggage — Duplicate key (last wins) & missing key lookup", () => {
        const hdr = "env=staging, tenant=acme, env=prod";
        let buf: [64]u8 = undefined;
        let len: usize = 0;

        expect_eq(getDecodedValue(hdr, "env", buf[0..64], &len), true);
        expect_eq(testEq(buf[0..len], "prod"), true);
        expect_eq(getDecodedValue(hdr, "missing", buf[0..64], &len), false);
    });

    test("5. @cap/otel-baggage — formatBaggage percent-encoding & roundtrip", () => {
        const members: [2]BaggageMember = [
            BaggageMember { key: "node", value: "DF 28,a=b;c", properties: "ttl=60" },
            BaggageMember { key: "rate", value: "100%", properties: "" },
        ];
        let wire: [128]u8 = undefined;
        const n = formatBaggage(members[0..2], wire[0..128]);
        const expected = "node=DF%2028%2Ca%3Db%3Bc;ttl=60,rate=100%25";
        expect_eq(n, expected.len);
        expect_eq(testEq(wire[0..n], expected), true);

        let dec_buf: [64]u8 = undefined;
        let dec_len: usize = 0;
        expect_eq(getDecodedValue(wire[0..n], "node", dec_buf[0..64], &dec_len), true);
        expect_eq(testEq(dec_buf[0..dec_len], "DF 28,a=b;c"), true);
        expect_eq(getDecodedValue(wire[0..n], "rate", dec_buf[0..64], &dec_len), true);
        expect_eq(testEq(dec_buf[0..dec_len], "100%"), true);
    });

    test("6. @cap/otel-baggage — isValidKey & undersized buffer protection", () => {
        expect_eq(isValidKey("user_id-1.0"), true);
        expect_eq(isValidKey(""), false);
        expect_eq(isValidKey("bad key"), false);
        expect_eq(isValidKey("bad=key"), false);
        expect_eq(isValidKey("bad,key"), false);

        const members: [1]BaggageMember = [
            BaggageMember { key: "longkey", value: "longvalue", properties: "" },
        ];
        let tiny: [5]u8 = undefined;
        expect_eq(formatBaggage(members[0..1], tiny[0..5]), 0 as usize);

        let out_len: usize = 0;
        expect_eq(getDecodedValue("k=toolongvalue", "k", tiny[0..5], &out_len), false);
    });
}
