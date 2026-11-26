import { test, expect_eq } from "cap:test";
import {
    kindOf, kindName, kindOfName, isPrimitive, isPlainObject,
    KIND_NULL, KIND_UNDEFINED, KIND_BOOLEAN, KIND_NUMBER, KIND_STRING,
    KIND_ARRAY, KIND_OBJECT, KIND_REGEXP, KIND_DATE, KIND_BIGINT, KIND_FUNCTION, KIND_INVALID
} from "../../packages/@cap/kind-of/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/kind-of — Primitives: null, undefined, boolean, string", () => {
        expect_eq(kindOf("null"), KIND_NULL);
        expect_eq(kindOf("  undefined  "), KIND_UNDEFINED);
        expect_eq(kindOf("true"), KIND_BOOLEAN);
        expect_eq(kindOf("false"), KIND_BOOLEAN);
        expect_eq(kindOf("\"hello\""), KIND_STRING);
        expect_eq(kindOf("'world'"), KIND_STRING);
        expect_eq(isPrimitive("true"), true);
        expect_eq(isPrimitive("null"), true);
    });

    test("2. @cap/kind-of — Numbers and BigInt", () => {
        expect_eq(kindOf("42"), KIND_NUMBER);
        expect_eq(kindOf("-3.14159"), KIND_NUMBER);
        expect_eq(kindOf("1.5e-10"), KIND_NUMBER);
        expect_eq(kindOf("NaN"), KIND_NUMBER);
        expect_eq(kindOf("-Infinity"), KIND_NUMBER);
        expect_eq(kindOf("9007199254740993n"), KIND_BIGINT);
        expect_eq(kindOf("-123n"), KIND_BIGINT);
    });

    test("3. @cap/kind-of — Arrays, plain objects, and predicates", () => {
        expect_eq(kindOf("[1, 2, 3]"), KIND_ARRAY);
        expect_eq(kindOf("{\"a\": 1}"), KIND_OBJECT);
        expect_eq(isPlainObject("{\"a\": 1}"), true);
        expect_eq(isPlainObject("[1, 2]"), false);
        expect_eq(isPlainObject("null"), false);
        expect_eq(isPrimitive("[1, 2]"), false);
    });

    test("4. @cap/kind-of — RegExp, ISO Date, and Function", () => {
        expect_eq(kindOf("/^[a-z]+$/gi"), KIND_REGEXP);
        expect_eq(kindOf("\"2026-11-27T03:00:00Z\""), KIND_DATE);
        expect_eq(kindOf("\"2026-11-27\""), KIND_DATE);
        expect_eq(kindOf("function foo() {}"), KIND_FUNCTION);
        expect_eq(kindOf("async function bar() {}"), KIND_FUNCTION);
    });

    test("5. @cap/kind-of — kindName and kindOfName string output", () => {
        expect_eq(strEq(kindOfName("null"), "null"), true);
        expect_eq(strEq(kindOfName("[1]"), "array"), true);
        expect_eq(strEq(kindOfName("{}"), "object"), true);
        expect_eq(strEq(kindOfName("/abc/i"), "regexp"), true);
        expect_eq(strEq(kindName(KIND_BIGINT), "bigint"), true);
        expect_eq(strEq(kindName(99), "invalid"), true);
    });

    test("6. @cap/kind-of — Rejects malformed and empty inputs safely", () => {
        expect_eq(kindOf(""), KIND_INVALID);
        expect_eq(kindOf("   "), KIND_INVALID);
        expect_eq(kindOf("12e"), KIND_INVALID);
        expect_eq(kindOf("n"), KIND_INVALID);
        expect_eq(kindOf("foobar"), KIND_INVALID);
    });
}
