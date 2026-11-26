import { test, expect_eq } from "cap:test";
import {
    DynValueKind,
    DynValue,
    typeCheck,
    isValidTypeExpr,
} from "../../packages/@cap/type-check/src/index.cp";

fn main() {
    test("1. @cap/type-check — primitive types and wildcard *", () => {
        expect_eq(typeCheck("String", DynValue.of(.String)), true);
        expect_eq(typeCheck("String", DynValue.of(.Number)), false);
        expect_eq(typeCheck("Number", DynValue.of(.Int)), true);
        expect_eq(typeCheck("Int", DynValue.of(.Float)), false);
        expect_eq(typeCheck("*", DynValue.of(.Object)), true);
    });

    test("2. @cap/type-check — union expressions A | B | C", () => {
        expect_eq(typeCheck("String | Number", DynValue.of(.String)), true);
        expect_eq(typeCheck("String | Number", DynValue.of(.Int)), true);
        expect_eq(typeCheck("String | Number", DynValue.of(.Boolean)), false);
    });

    test("3. @cap/type-check — Maybe T allows Null, Undefined, or T", () => {
        expect_eq(typeCheck("Maybe String", DynValue.of(.Null)), true);
        expect_eq(typeCheck("Maybe String", DynValue.of(.Undefined)), true);
        expect_eq(typeCheck("Maybe String", DynValue.of(.String)), true);
        expect_eq(typeCheck("Maybe String", DynValue.of(.Int)), false);
    });

    test("4. @cap/type-check — typed arrays [T] and union element arrays [String | Int]", () => {
        expect_eq(typeCheck("[String]", DynValue.arrayOf(.String, 3)), true);
        expect_eq(typeCheck("[String]", DynValue.arrayOf(.Int, 3)), false);
        expect_eq(typeCheck("[String]", DynValue.arrayOf(.Int, 0)), true); // empty array
        expect_eq(typeCheck("[String | Int]", DynValue.arrayOf(.Int, 2)), true);
    });

    test("5. @cap/type-check — parenthesized and object shape expressions", () => {
        expect_eq(typeCheck("(String | Boolean)", DynValue.of(.Boolean)), true);
        expect_eq(typeCheck("{a: Number, b: String}", DynValue.of(.Object)), true);
        expect_eq(typeCheck("{a: Number}", DynValue.of(.Array)), false);
    });

    test("6. @cap/type-check — isValidTypeExpr catches unbalanced delimiters", () => {
        expect_eq(isValidTypeExpr("[String | (Int | Null)]"), true);
        expect_eq(isValidTypeExpr("[String"), false);
        expect_eq(isValidTypeExpr("(Number]"), false);
        expect_eq(isValidTypeExpr(""), false);
    });
}
