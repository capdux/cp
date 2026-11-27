import { test, expect_eq } from "cap:test";
import {
    EntityTag,
    parseEtag,
    formatEtag,
    strongEqual,
    weakEqual,
    generateStatEtag,
    generateContentEtag,
    matchIfNoneMatch,
    matchIfMatch,
} from "../../packages/etag/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/etag — Strong & weak ETag parsing + invalid rejection", () => {
        let e = EntityTag { weak: false, tag: "" };
        expect_eq(parseEtag("\"abc-123\"", &e), true);
        expect_eq(e.weak, false);
        expect_eq(testEq(e.tag, "abc-123"), true);

        expect_eq(parseEtag("  W/\"weak-tag\"  ", &e), true);
        expect_eq(e.weak, true);
        expect_eq(testEq(e.tag, "weak-tag"), true);

        // Reject lowercase w/, missing quotes, and control/space bytes
        expect_eq(parseEtag("w/\"bad\"", &e), false);
        expect_eq(parseEtag("no-quotes", &e), false);
        expect_eq(parseEtag("\"has space\"", &e), false);
        expect_eq(parseEtag("\"bad\nctrl\"", &e), false);
    });

    test("2. @cap/etag — strongEqual vs weakEqual", () => {
        const s1 = EntityTag { weak: false, tag: "v1" };
        const s2 = EntityTag { weak: false, tag: "v1" };
        const w1 = EntityTag { weak: true, tag: "v1" };
        const diff = EntityTag { weak: false, tag: "v2" };

        expect_eq(strongEqual(s1, s2), true);
        expect_eq(strongEqual(s1, w1), false);
        expect_eq(strongEqual(w1, w1), false);
        expect_eq(strongEqual(s1, diff), false);

        expect_eq(weakEqual(s1, s2), true);
        expect_eq(weakEqual(s1, w1), true);
        expect_eq(weakEqual(w1, w1), true);
        expect_eq(weakEqual(s1, diff), false);
    });

    test("3. @cap/etag — generateStatEtag & generateContentEtag", () => {
        let buf: [64]u8 = undefined;
        // 256 -> 0x100, 4095 -> 0xfff => W/"100-fff"
        const n1 = generateStatEtag(256, 4095, buf[0..64]);
        expect_eq(testEq(buf[0..n1], "W/\"100-fff\""), true);

        let c1: [64]u8 = undefined;
        let c2: [64]u8 = undefined;
        const m1 = generateContentEtag("hello world", false, c1[0..64]);
        const m2 = generateContentEtag("hello world", false, c2[0..64]);
        expect_eq(m1 > 0, true);
        expect_eq(testEq(c1[0..m1], c2[0..m2]), true);

        let parsed = EntityTag { weak: false, tag: "" };
        expect_eq(parseEtag(c1[0..m1], &parsed), true);
        expect_eq(parsed.weak, false);

        // Weak content ETag
        let cw: [64]u8 = undefined;
        const mw = generateContentEtag("hello world", true, cw[0..64]);
        expect_eq(parseEtag(cw[0..mw], &parsed), true);
        expect_eq(parsed.weak, true);
    });

    test("4. @cap/etag — matchIfNoneMatch comma list & '*' wildcard", () => {
        const cur = EntityTag { weak: true, tag: "rev-2" };
        expect_eq(matchIfNoneMatch("*", cur), true);
        expect_eq(matchIfNoneMatch("\"rev-1\", \"rev-2\", \"rev-3\"", cur), true);
        expect_eq(matchIfNoneMatch("W/\"rev-2\"", cur), true);
        expect_eq(matchIfNoneMatch("\"rev-1\", \"rev-3\"", cur), false);
        expect_eq(matchIfNoneMatch("", cur), false);
    });

    test("5. @cap/etag — matchIfMatch strong comparison rejects weak ETags", () => {
        const strong_cur = EntityTag { weak: false, tag: "rev-2" };
        const weak_cur = EntityTag { weak: true, tag: "rev-2" };

        expect_eq(matchIfMatch("*", strong_cur), true);
        expect_eq(matchIfMatch("\"rev-1\", \"rev-2\"", strong_cur), true);
        // Weak header or weak current tag must be rejected by strong comparison
        expect_eq(matchIfMatch("W/\"rev-2\"", strong_cur), false);
        expect_eq(matchIfMatch("\"rev-2\"", weak_cur), false);
    });

    test("6. @cap/etag — formatEtag roundtrip & undersized buffer protection", () => {
        let buf: [64]u8 = undefined;
        const n1 = formatEtag(false, "xyz99", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "\"xyz99\""), true);

        const n2 = formatEtag(true, "xyz99", buf[0..64]);
        expect_eq(testEq(buf[0..n2], "W/\"xyz99\""), true);

        let parsed = EntityTag { weak: false, tag: "" };
        expect_eq(parseEtag(buf[0..n2], &parsed), true);
        expect_eq(parsed.weak, true);
        expect_eq(testEq(parsed.tag, "xyz99"), true);

        let tiny: [3]u8 = undefined;
        expect_eq(formatEtag(false, "xyz99", tiny[0..3]), 0 as usize);
        expect_eq(generateStatEtag(100, 200, tiny[0..3]), 0 as usize);
    });
}
