import { test, expect_eq } from "cap:test";
import {
    CASE_UPPER,
    CASE_LOWER,
    CASE_TITLE,
    CASE_CAMEL,
    CASE_PASCAL,
    CASE_SNAKE,
    CASE_UPPER_SNAKE,
    CASE_KEBAB,
    CASE_UPPER_KEBAB,
    CASE_COBOL,
    CASE_TRAIN,
    CASE_FLAT,
    CASE_UPPER_FLAT,
    CASE_ALTERNATING,
    convertCase,
    toFlatCase,
    toUpperFlatCase,
    toAlternatingCase,
    toLowerWords,
    toUpperWords,
} from "../../packages/convert_case/src/index.cp";

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
    test("1. @cap/convert-case — core heck-backed styles (camel, pascal, snake, kebab, title, train)", () => {
        let buf: [128]u8 = undefined;

        const n1 = convertCase("hello_world", CASE_CAMEL, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "helloWorld"), true);

        const n2 = convertCase("hello_world", CASE_PASCAL, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "HelloWorld"), true);

        const n3 = convertCase("XMLHttpRequest", CASE_SNAKE, buf[0..]);
        expect_eq(eqStr(buf[0..n3], "xml_http_request"), true);

        const n4 = convertCase("helloWorld", CASE_UPPER_SNAKE, buf[0..]);
        expect_eq(eqStr(buf[0..n4], "HELLO_WORLD"), true);

        const n5 = convertCase("helloWorld", CASE_KEBAB, buf[0..]);
        expect_eq(eqStr(buf[0..n5], "hello-world"), true);

        const n6 = convertCase("hello_world", CASE_TITLE, buf[0..]);
        expect_eq(eqStr(buf[0..n6], "Hello World"), true);

        const n7 = convertCase("hello_world", CASE_TRAIN, buf[0..]);
        expect_eq(eqStr(buf[0..n7], "Hello-World"), true);
    });

    test("2. @cap/convert-case — COBOL & UPPER_KEBAB styles", () => {
        let buf: [128]u8 = undefined;

        const n1 = convertCase("helloWorld", CASE_COBOL, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "HELLO-WORLD"), true);

        const n2 = convertCase("my_http_server", CASE_UPPER_KEBAB, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "MY-HTTP-SERVER"), true);
    });

    test("3. @cap/convert-case — toFlatCase & toUpperFlatCase", () => {
        let buf: [128]u8 = undefined;

        const n1 = toFlatCase("Hello-World_Test", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "helloworldtest"), true);

        const n2 = toUpperFlatCase("helloWorld", buf[0..]);
        expect_eq(eqStr(buf[0..n2], "HELLOWORLD"), true);

        const n3 = convertCase("XMLHttpRequest", CASE_FLAT, buf[0..]);
        expect_eq(eqStr(buf[0..n3], "xmlhttprequest"), true);

        const n4 = convertCase("XMLHttpRequest", CASE_UPPER_FLAT, buf[0..]);
        expect_eq(eqStr(buf[0..n4], "XMLHTTPREQUEST"), true);
    });

    test("4. @cap/convert-case — toLowerWords & toUpperWords", () => {
        let buf: [128]u8 = undefined;

        const n1 = toLowerWords("HelloWorld", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "hello world"), true);

        const n2 = toUpperWords("hello_world", buf[0..]);
        expect_eq(eqStr(buf[0..n2], "HELLO WORLD"), true);

        const n3 = convertCase("myVarName", CASE_LOWER, buf[0..]);
        expect_eq(eqStr(buf[0..n3], "my var name"), true);

        const n4 = convertCase("myVarName", CASE_UPPER, buf[0..]);
        expect_eq(eqStr(buf[0..n4], "MY VAR NAME"), true);
    });

    test("5. @cap/convert-case — toAlternatingCase", () => {
        let buf: [128]u8 = undefined;

        const n1 = toAlternatingCase("hello_world", buf[0..]);
        expect_eq(eqStr(buf[0..n1], "hElLo WoRlD"), true);

        const n2 = convertCase("FooBarBaz", CASE_ALTERNATING, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "fOo BaR bAz"), true);
    });

    test("6. @cap/convert-case — empty input & unknown style safety", () => {
        let buf: [64]u8 = undefined;

        expect_eq(convertCase("", CASE_SNAKE, buf[0..]), 0 as usize);
        expect_eq(convertCase("", CASE_ALTERNATING, buf[0..]), 0 as usize);
        expect_eq(convertCase("hello", 99 as u8, buf[0..]), 0 as usize);
    });
}
