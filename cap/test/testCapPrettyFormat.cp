import { test, expect_eq } from "cap:test";
import {
    PrettyOptions,
    PrettyWriter,
    formatStringLiteral,
    formatPrimitiveInt
} from "../../packages/@cap/pretty-format/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
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
    test("1. @cap/pretty-format — primitives, symbols, and functions", () => {
        let w = PrettyWriter.defaultWriter();

        w.writeNull();
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "null"), true);

        w.clear();
        w.writeBool(true);
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "true"), true);

        w.clear();
        w.writeInt(-42);
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "-42"), true);

        w.clear();
        w.writeFloat(3.25);
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "3.25"), true);

        w.clear();
        w.writeSymbol("iter");
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "Symbol(iter)"), true);

        w.clear();
        w.writeFunction("render");
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "[Function render]"), true);

        w.clear();
        w.writeFunction("");
        expect_eq(eqBytes(w.buf[0..w.len], w.len, "[Function anonymous]"), true);
    });

    test("2. @cap/pretty-format — string escaping and helper shortcuts", () => {
        let out: [64]u8 = undefined;
        let n1 = formatStringLiteral("a\"b\\c\nd\te", true, out[0..64]);
        expect_eq(eqBytes(out[0..64], n1, "\"a\\\"b\\\\c\\nd\\te\""), true);

        let n2 = formatStringLiteral("raw\nline", false, out[0..64]);
        expect_eq(eqBytes(out[0..64], n2, "\"raw\nline\""), true);

        let n3 = formatPrimitiveInt(-123456, out[0..64]);
        expect_eq(eqBytes(out[0..64], n3, "-123456"), true);
    });

    test("3. @cap/pretty-format — multi-line indented Object and Array formatting", () => {
        let w = PrettyWriter.defaultWriter();
        let ok = w.beginObject("Object");
        expect_eq(ok, true);
        w.writeKey("name");
        w.writeString("cp");
        w.writeKey("nums");
        let arr_ok = w.beginArray("Array");
        expect_eq(arr_ok, true);
        w.nextItem();
        w.writeInt(1);
        w.nextItem();
        w.writeInt(2);
        w.endArray();
        w.endObject();

        let expected = "Object {\n  \"name\": \"cp\",\n  \"nums\": Array [\n    1,\n    2,\n  ],\n}";
        expect_eq(eqBytes(w.buf[0..w.len], w.len, expected), true);
    });

    test("4. @cap/pretty-format — compact (min) single-line mode omits prototypes and newlines", () => {
        let w = PrettyWriter.new(PrettyOptions.compact());
        let ok = w.beginObject("Object");
        expect_eq(ok, true);
        w.writeKey("ok");
        w.writeBool(true);
        w.writeKey("items");
        let aok = w.beginArray("Array");
        expect_eq(aok, true);
        w.nextItem();
        w.writeInt(7);
        w.nextItem();
        w.writeInt(8);
        w.endArray();
        w.endObject();

        let expected = "{\"ok\": true, \"items\": [7, 8]}";
        expect_eq(eqBytes(w.buf[0..w.len], w.len, expected), true);
    });

    test("5. @cap/pretty-format — max_depth truncates nested containers to [Object] / [Array]", () => {
        let opts = PrettyOptions {
            indent: 2,
            max_depth: 1,
            min: true,
            escape_string: true,
            print_basic_prototype: false
        };
        let w = PrettyWriter.new(opts);
        expect_eq(w.beginObject("Object"), true);
        w.writeKey("obj");
        expect_eq(w.beginObject("Object"), false);
        w.writeKey("hidden");
        w.writeInt(999);
        w.endObject();
        w.writeKey("arr");
        expect_eq(w.beginArray("Array"), false);
        w.nextItem();
        w.writeInt(123);
        w.endArray();
        w.endObject();

        let expected = "{\"obj\": [Object], \"arr\": [Array]}";
        expect_eq(eqBytes(w.buf[0..w.len], w.len, expected), true);
    });

    test("6. @cap/pretty-format — empty containers and print_basic_prototype=false", () => {
        let opts = PrettyOptions {
            indent: 2,
            max_depth: 8,
            min: false,
            escape_string: true,
            print_basic_prototype: false
        };
        let w = PrettyWriter.new(opts);
        expect_eq(w.beginObject("Object"), true);
        w.writeKey("emptyObj");
        expect_eq(w.beginObject("Object"), true);
        w.endObject();
        w.writeKey("custom");
        expect_eq(w.beginArray("CustomList"), true);
        w.endArray();
        w.endObject();

        let expected = "{\n  \"emptyObj\": {},\n  \"custom\": CustomList [],\n}";
        expect_eq(eqBytes(w.buf[0..w.len], w.len, expected), true);
    });
}
