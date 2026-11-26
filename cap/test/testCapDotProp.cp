// cap/test/testCapDotProp.cp
// Comprehensive unit tests for @cap/dot-prop

import { test, expect_eq } from "cap:test";
import {
    parseDotPath,
    getJsonPath,
    getJsonString,
    hasJsonPath,
    escapePathSegment,
} from "../../packages/@cap/dot-prop/src/index.cp";

fn main() {
    test("1. @cap/dot-prop — parseDotPath with dots, brackets & escaped keys", () => {
        let kbuf1: [128]u8 = undefined;
        const p1 = parseDotPath("items[0].tags[2]", kbuf1[0..128]);
        expect_eq(p1.valid, true);
        expect_eq(p1.count, 4 as usize);
        expect_eq(p1.segs[0].key, "items");
        expect_eq(p1.segs[1].is_index, true);
        expect_eq(p1.segs[1].index, 0 as usize);
        expect_eq(p1.segs[2].key, "tags");
        expect_eq(p1.segs[3].is_index, true);
        expect_eq(p1.segs[3].index, 2 as usize);

        let kbuf2: [128]u8 = undefined;
        const p2 = parseDotPath("config.a\\.b.value", kbuf2[0..128]);
        expect_eq(p2.valid, true);
        expect_eq(p2.count, 3 as usize);
        expect_eq(p2.segs[0].key, "config");
        expect_eq(p2.segs[1].key, "a.b");
        expect_eq(p2.segs[2].key, "value");
    });

    test("2. @cap/dot-prop — Deep nested object & primitive reading", () => {
        const doc = "{\"user\":{\"profile\":{\"name\":\"Alice\",\"age\":30,\"active\":true,\"bio\":null}}}";
        expect_eq(getJsonString(doc, "user.profile.name") ?? "", "Alice");
        expect_eq(getJsonPath(doc, "user.profile.age") ?? "", "30");
        expect_eq(getJsonPath(doc, "user.profile.active") ?? "", "true");
        expect_eq(getJsonPath(doc, "user.profile.bio") ?? "", "null");
        expect_eq(getJsonString(doc, "user.profile.age") == null, true);
    });

    test("3. @cap/dot-prop — Array indexing with [0] and .0 syntax", () => {
        const doc = "{\"items\":[{\"id\":10,\"tags\":[\"fast\",\"safe\",\"zero-alloc\"]},{\"id\":20,\"tags\":[\"cp\"]}]}";
        expect_eq(getJsonPath(doc, "items[0].id") ?? "", "10");
        expect_eq(getJsonString(doc, "items[0].tags[2]") ?? "", "zero-alloc");
        expect_eq(getJsonString(doc, "items.1.tags.0") ?? "", "cp");
        expect_eq(getJsonPath(doc, "items[1].tags") ?? "", "[\"cp\"]");
    });

    test("4. @cap/dot-prop — Escaped dot keys & strings containing braces/brackets", () => {
        const doc = "{\"noise\":\"a {b} [c] \\\"quoted\\\"\",\"config\":{\"a.b\":{\"value\":42}}}";
        expect_eq(getJsonPath(doc, "config.a\\.b.value") ?? "", "42");
        expect_eq(hasJsonPath(doc, "config.a\\.b.value"), true);
        expect_eq(hasJsonPath(doc, "config.a.b.value"), false);

        let escBuf: [64]u8 = undefined;
        expect_eq(escapePathSegment("a.b[0]\\c", escBuf[0..64]), "a\\.b\\[0\\]\\\\c");
    });

    test("5. @cap/dot-prop — hasJsonPath & sub-object/sub-array slice extraction", () => {
        const doc = "{\"matrix\":[[1,2],[3,4]],\"meta\":{\"ver\":\"1.0\"}}";
        expect_eq(hasJsonPath(doc, "matrix[1][0]"), true);
        expect_eq(getJsonPath(doc, "matrix[1][0]") ?? "", "3");
        expect_eq(getJsonPath(doc, "meta") ?? "", "{\"ver\":\"1.0\"}");
        expect_eq(hasJsonPath(doc, "meta.missing"), false);
    });

    test("6. @cap/dot-prop — Missing keys, out-of-bounds indices & malformed paths", () => {
        const doc = "{\"a\":{\"b\":[10,20]}}";
        expect_eq(getJsonPath(doc, "a.b[5]") == null, true);
        expect_eq(getJsonPath(doc, "a.missing") == null, true);
        expect_eq(getJsonPath(doc, "a.b.foo") == null, true);
        expect_eq(getJsonPath(doc, "") == null, true);
        expect_eq(getJsonPath(doc, "a..b") == null, true);
        expect_eq(getJsonPath(doc, "a.b[abc]") == null, true);
        expect_eq(getJsonPath(doc, "a.b[0") == null, true);
        expect_eq(getJsonPath("not json", "a") == null, true);
    });
}
