import { test, expect_eq } from "cap:test";
import {
    KIND_INVALID,
    KIND_NULL,
    KIND_BOOL,
    KIND_NUMBER,
    KIND_STRING,
    KIND_ARRAY,
    KIND_OBJECT,
    kindOfJson,
    isPlainObjectJson,
    jsonDeepEqual,
} from "../../packages/@cap/fast-deep-equal/src/index.cp";

fn main() {
    test("1. @cap/fast-deep-equal — Primitives: null, bool & numeric equivalence (1.0==1, 1e2==100, -0==0)", () => {
        expect_eq(jsonDeepEqual("null", "  null\n"), true);
        expect_eq(jsonDeepEqual("true", "true"), true);
        expect_eq(jsonDeepEqual("true", "false"), false);
        expect_eq(jsonDeepEqual("1", "1.0"), true);
        expect_eq(jsonDeepEqual("100", "1e2"), true);
        expect_eq(jsonDeepEqual("0", "-0"), true);
        expect_eq(jsonDeepEqual("2.5", "25e-1"), true);
        expect_eq(jsonDeepEqual("1", "2"), false);
    });

    test("2. @cap/fast-deep-equal — Escape-aware string comparison", () => {
        expect_eq(jsonDeepEqual("\"hello\\nworld\"", "\"hello\\nworld\""), true);
        expect_eq(jsonDeepEqual("\"a\\u0062c\"", "\"abc\""), true);
        expect_eq(jsonDeepEqual("\"a\\/b\"", "\"a/b\""), true);
        expect_eq(jsonDeepEqual("\"quote: \\\"hi\\\"\"", "\"quote: \\\"hi\\\"\""), true);
        expect_eq(jsonDeepEqual("\"abc\"", "\"abd\""), false);
    });

    test("3. @cap/fast-deep-equal — Nested arrays are order-sensitive", () => {
        expect_eq(jsonDeepEqual("[1, [2, 3], true]", " [ 1.0 , [ 2 , 3 ] , true ] "), true);
        expect_eq(jsonDeepEqual("[1, 2, 3]", "[1, 3, 2]"), false);
        expect_eq(jsonDeepEqual("[1, 2]", "[1, 2, 3]"), false);
        expect_eq(jsonDeepEqual("[]", " [ ] "), true);
    });

    test("4. @cap/fast-deep-equal — Deep objects ignore key order recursively", () => {
        const a: string = "{\"a\":{\"x\":1,\"y\":2},\"b\":[3,4]}";
        const b: string = "{ \"b\" : [ 3.0 , 4 ] , \"a\" : { \"y\" : 2.0 , \"x\" : 1 } }";
        expect_eq(jsonDeepEqual(a, b), true);
        expect_eq(jsonDeepEqual("{}", " { } "), true);
    });

    test("5. @cap/fast-deep-equal — Missing keys, extra keys & type mismatches return false", () => {
        expect_eq(jsonDeepEqual("{\"a\":1,\"b\":2}", "{\"a\":1}"), false);
        expect_eq(jsonDeepEqual("{\"a\":1}", "{\"a\":1,\"b\":2}"), false);
        expect_eq(jsonDeepEqual("{\"a\":1}", "{\"a\":\"1\"}"), false);
        expect_eq(jsonDeepEqual("{\"a\":null}", "{\"a\":false}"), false);
        expect_eq(jsonDeepEqual("[]", "{}"), false);
    });

    test("6. @cap/fast-deep-equal — kindOfJson, isPlainObjectJson & invalid JSON safety", () => {
        expect_eq(kindOfJson("null"), KIND_NULL);
        expect_eq(kindOfJson("false"), KIND_BOOL);
        expect_eq(kindOfJson("-42.5"), KIND_NUMBER);
        expect_eq(kindOfJson("\"cp\""), KIND_STRING);
        expect_eq(kindOfJson("[1,2]"), KIND_ARRAY);
        expect_eq(kindOfJson("{\"k\":1}"), KIND_OBJECT);
        expect_eq(isPlainObjectJson("{\"k\":1}"), true);
        expect_eq(isPlainObjectJson("[1,2]"), false);

        expect_eq(kindOfJson(""), KIND_INVALID);
        expect_eq(kindOfJson("{bad}"), KIND_INVALID);
        expect_eq(kindOfJson("123 trailing"), KIND_INVALID);
        expect_eq(jsonDeepEqual("{bad}", "{bad}"), false);
        expect_eq(jsonDeepEqual("", ""), false);
    });
}
