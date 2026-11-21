import { test, expect_eq } from "cap:test";
import {
    ExactSizeMatcher,
    Matcher4,
    Matcher8,
    Matcher16,
    packU32,
    packU32Lower,
    packU64,
    packU64Lower,
    packU128,
    packU128Lower,
} from "cap:exact_matcher";

test("1. Matcher4 exact switch dispatch and boundary sentinel", () => {
    expect_eq(Matcher4.match("GET"), Matcher4.case("GET"));
    expect_eq(Matcher4.match("POST"), Matcher4.case("POST"));
    expect_eq(Matcher4.match("PUT"), Matcher4.case("PUT"));
    expect_eq(Matcher4.match(""), u32(0));
    expect_eq(Matcher4.match("DELETE"), std.math.maxInt(u32));
    expect_eq(packU32("POST"), Matcher4.case("POST"));
});

test("2. Matcher4 matchLower handles both shorter and exact max_bytes lengths", () => {
    expect_eq(Matcher4.matchLower("GET"), Matcher4.case("get"));
    expect_eq(Matcher4.matchLower("Post"), Matcher4.case("post"));
    expect_eq(Matcher4.matchLower("HEAD"), Matcher4.case("head"));
    expect_eq(packU32Lower("PoSt"), Matcher4.case("post"));
    expect_eq(Matcher4.matchLower("TOO_LONG"), std.math.maxInt(u32));
});

test("3. Matcher8 HTTP methods and JS keywords dispatch", () => {
    expect_eq(Matcher8.match("OPTIONS"), Matcher8.case("OPTIONS"));
    expect_eq(Matcher8.match("CONNECT"), Matcher8.case("CONNECT"));
    expect_eq(Matcher8.match("function"), Matcher8.case("function"));
    expect_eq(packU64("function"), Matcher8.case("function"));
    expect_eq(packU64Lower("FuNcTiOn"), Matcher8.case("function"));
    expect_eq(Matcher8.match("123456789"), std.math.maxInt(u64));
});

test("4. Matcher16 exact 16-byte and shorter MIME/header tokens", () => {
    expect_eq(Matcher16.match("application/json"), Matcher16.case("application/json"));
    expect_eq(packU128("application/json"), Matcher16.case("application/json"));
    expect_eq(packU128Lower("Application/JSON"), Matcher16.case("application/json"));
    expect_eq(Matcher16.match("text/plain"), Matcher16.case("text/plain"));
    expect_eq(Matcher16.match("application/octet-stream"), std.math.maxInt(u128));
});

test("5. Custom 1-byte, 2-byte, and 12-byte matchers", () => {
    const M1 = ExactSizeMatcher(1);
    const M2 = ExactSizeMatcher(2);
    const M12 = ExactSizeMatcher(12);

    expect_eq(M1.match("a"), M1.case("a"));
    expect_eq(M1.matchLower("Z"), M1.case("z"));
    expect_eq(M1.match("ab"), std.math.maxInt(u8));

    expect_eq(M2.match("js"), M2.case("js"));
    expect_eq(M2.matchLower("TS"), M2.case("ts"));
    expect_eq(M2.match("tsx"), std.math.maxInt(u16));

    expect_eq(M12.match("content-type"), M12.case("content-type"));
    expect_eq(M12.matchLower("Content-Type"), M12.case("content-type"));
});
