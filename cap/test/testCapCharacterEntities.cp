import { test, expect_eq } from "cap:test";
import {
    mapInvalidCharRef,
    decodeNamedEntity,
    decodeNumericEntity,
    decodeEntities,
    encodeEntities,
} from "../../packages/character-entities/src/index.cp";

fn main() {
    test("1. @cap/character-entities — decodes standard HTML5 named entities", () => {
        expect_eq(decodeNamedEntity("amp") ?? 0, 0x26 as u32);
        expect_eq(decodeNamedEntity("lt") ?? 0, 0x3C as u32);
        expect_eq(decodeNamedEntity("copy") ?? 0, 0xA9 as u32);
        expect_eq(decodeNamedEntity("euro") ?? 0, 0x20AC as u32);
        expect_eq(decodeNamedEntity("unknown_xyz") == null, true);
    });

    test("2. @cap/character-entities — decodes decimal and hexadecimal numeric references", () => {
        expect_eq(decodeNumericEntity("65") ?? 0, 65 as u32);
        expect_eq(decodeNumericEntity("x1F600") ?? 0, 0x1F600 as u32);
        expect_eq(decodeNumericEntity("X20AC") ?? 0, 0x20AC as u32);
        expect_eq(decodeNumericEntity("0") ?? 0, 0xFFFD as u32);
        expect_eq(decodeNumericEntity("xD800") ?? 0, 0xFFFD as u32);
    });

    test("3. @cap/character-entities — applies WHATWG Windows-1252 C1 invalid char ref mapping", () => {
        expect_eq(mapInvalidCharRef(0x80), 0x20AC as u32); // €
        expect_eq(mapInvalidCharRef(0x97), 0x2014 as u32); // —
        expect_eq(decodeNumericEntity("128") ?? 0, 0x20AC as u32);
    });

    test("4. @cap/character-entities — decodeEntities decodes mixed text, named & numeric refs", () => {
        let buf: [128]u8 = [0; 128];
        const n = decodeEntities("a &lt; b &amp; &#67;&#x50; &copy;", buf[0..128]);
        expect_eq(buf[0..n] as string, "a < b & CP ©");
    });

    test("5. @cap/character-entities — leaves unrecognized entities untouched", () => {
        let buf: [128]u8 = [0; 128];
        const n = decodeEntities("keep &notreal; and bare & ok", buf[0..128]);
        expect_eq(buf[0..n] as string, "keep &notreal; and bare & ok");
    });

    test("6. @cap/character-entities — encodeEntities escapes HTML special characters", () => {
        let buf: [128]u8 = [0; 128];
        const n = encodeEntities("<div title=\"a&b\">'hi'</div>", buf[0..128]);
        expect_eq(buf[0..n] as string, "&lt;div title=&quot;a&amp;b&quot;&gt;&#x27;hi&#x27;&lt;/div&gt;");
    });
}
