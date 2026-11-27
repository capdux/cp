import { test, expect_eq } from "cap:test";
import {
    SourceMapCommentKind,
    findSourceMapComment,
    fromSource,
    toComment,
    removeComments,
} from "../../packages/convert-source-map/src/index.cp";

fn main() {
    test("1. @cap/convert-source-map — toComment and fromSource roundtrip JS line comment", () => {
        const json_map = "{\"version\":3,\"file\":\"app.js\",\"mappings\":\"AAAA\"}";
        let comment: [256]u8 = undefined;
        let clen = toComment(json_map, false, comment[0..256]);
        let decoded: [256]u8 = undefined;
        let dlen = fromSource(comment[0..clen], decoded[0..256]);
        expect_eq(dlen, json_map.len as i64);
        expect_eq(decoded[0..(dlen as usize)], json_map);
    });

    test("2. @cap/convert-source-map — supports CSS multiline /*# sourceMappingURL=... */", () => {
        const json_map = "{\"version\":3,\"sources\":[\"a.css\"]}";
        let comment: [256]u8 = undefined;
        let clen = toComment(json_map, true, comment[0..256]);
        const info = findSourceMapComment(comment[0..clen]);
        expect_eq(info.kind == .InlineBase64, true);

        let decoded: [256]u8 = undefined;
        let dlen = fromSource(comment[0..clen], decoded[0..256]);
        expect_eq(decoded[0..(dlen as usize)], json_map);
    });

    test("3. @cap/convert-source-map — detects external file sourceMappingURL", () => {
        const src = "console.log(1);\n//# sourceMappingURL=bundle.js.map";
        const info = findSourceMapComment(src);
        expect_eq(info.kind == .ExternalFile, true);
        expect_eq(info.payload, "bundle.js.map");

        let out: [64]u8 = undefined;
        expect_eq(fromSource(src, out[0..64]), -1 as i64);
    });

    test("4. @cap/convert-source-map — removeComments strips trailing sourceMappingURL", () => {
        const src = "const x = 1;\n//# sourceMappingURL=out.js.map";
        let out: [128]u8 = undefined;
        let n = removeComments(src, out[0..128]);
        expect_eq(out[0..n], "const x = 1;\n");
    });

    test("5. @cap/convert-source-map — handles legacy //@ pragma and picks last comment", () => {
        const src = "//# sourceMappingURL=old.map\ncode();\n//@ sourceMappingURL=final.map";
        const info = findSourceMapComment(src);
        expect_eq(info.kind == .ExternalFile, true);
        expect_eq(info.payload, "final.map");
    });

    test("6. @cap/convert-source-map — returns None when no sourceMappingURL exists", () => {
        const src = "function hello() { return 42; }";
        const info = findSourceMapComment(src);
        expect_eq(info.kind == .None, true);

        let out: [64]u8 = undefined;
        let n = removeComments(src, out[0..64]);
        expect_eq(out[0..n], src);
    });
}
