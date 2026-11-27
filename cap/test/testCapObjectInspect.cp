import { test, expect_eq } from "cap:test";
import {
    InspectOptions,
    defaultInspectOptions,
    inspectString,
    inspectBool,
    inspectInt,
    inspectBytes,
    inspectStringArray,
} from "../../packages/object-inspect/src/index.cp";

fn main() {
    test("1. @cap/object-inspect — quotes and escapes control characters in strings", () => {
        const opts = defaultInspectOptions();
        let buf: [128]u8 = [0; 128];
        const n = inspectString("it's\na\tpath\\x\x00", opts, buf[0..128]);
        expect_eq(buf[0..n] as string, "'it\\'s\\na\\tpath\\\\x\\x00'");
    });

    test("2. @cap/object-inspect — supports double quotes and max_string_len truncation", () => {
        const opts = InspectOptions { quote_style_double: true, max_string_len: 5 };
        let buf: [128]u8 = [0; 128];
        const n = inspectString("abcdefghij", opts, buf[0..128]);
        expect_eq(buf[0..n] as string, "\"abcde...\"");
    });

    test("3. @cap/object-inspect — inspects booleans and signed integers", () => {
        let buf: [64]u8 = [0; 64];
        expect_eq(buf[0..inspectBool(true, buf[0..64])] as string, "true");
        expect_eq(buf[0..inspectBool(false, buf[0..64])] as string, "false");
        expect_eq(buf[0..inspectInt(0, buf[0..64])] as string, "0");
        expect_eq(buf[0..inspectInt(-42099, buf[0..64])] as string, "-42099");
    });

    test("4. @cap/object-inspect — inspects byte buffers with optional truncation", () => {
        const raw: [5]u8 = [0xDE, 0xAD, 0xBE, 0xEF, 0x01];
        let buf: [128]u8 = [0; 128];
        const n1 = inspectBytes(raw[0..5], 0, buf[0..128]);
        expect_eq(buf[0..n1] as string, "<Buffer de ad be ef 01>");
        const n2 = inspectBytes(raw[0..5], 2, buf[0..128]);
        expect_eq(buf[0..n2] as string, "<Buffer de ad ...>");
    });

    test("5. @cap/object-inspect — inspects string arrays", () => {
        const items: [3]string = ["foo", "bar", "baz"];
        let buf: [128]u8 = [0; 128];
        const n = inspectStringArray(items[0..3], defaultInspectOptions(), buf[0..128]);
        expect_eq(buf[0..n] as string, "[ 'foo', 'bar', 'baz' ]");
    });

    test("6. @cap/object-inspect — inspects empty string arrays and empty buffers", () => {
        const items: [1]string = ["x"];
        let buf: [64]u8 = [0; 64];
        expect_eq(buf[0..inspectStringArray(items[0..0], defaultInspectOptions(), buf[0..64])] as string, "[]");
        const raw: [1]u8 = [0];
        expect_eq(buf[0..inspectBytes(raw[0..0], 0, buf[0..64])] as string, "<Buffer>");
    });
}
