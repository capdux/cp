// cap/test/testCapKebabcaseKeys.cp
// Comprehensive unit tests for @cap/kebabcase-keys

import { test, expect_eq } from "cap:test";
import { MapObjResult } from "../../packages/map-obj/src/index.cp";
import { DecamelizeKeysResult } from "../../packages/decamelize-keys/src/index.cp";
import {
    kebabcaseKeys,
    kebabcaseKeysAll,
    kebabcaseKeyInto,
    kebabcasePathInto,
    kebabcaseKeysBatch,
} from "../../packages/kebabcase-keys/src/index.cp";

fn main() {
    test("1. @cap/kebabcase-keys — kebabcaseKeys converts camelCase & PascalCase keys", () => {
        let src = MapObjResult.empty();
        * = src.push("fooBar", "1");
        * = src.push("contentType", "text/html");
        * = src.push("UserAgentHeader", "cap/1.0");

        let no_excl: [0]string = undefined;
        let out = kebabcaseKeys(src, no_excl[0..0]);

        expect_eq(out.len(), 3 as usize);
        expect_eq(out.get("foo-bar") ?? "", "1");
        expect_eq(out.get("content-type") ?? "", "text/html");
        expect_eq(out.get("user-agent-header") ?? "", "cap/1.0");
    });

    test("2. @cap/kebabcase-keys — kebabcaseKeysAll & missing key lookup returns null", () => {
        let src = MapObjResult.empty();
        * = src.push("cacheControl", "max-age=60");
        * = src.push("acceptEncoding", "gzip");

        let out = kebabcaseKeysAll(src);
        expect_eq(out.len(), 2 as usize);
        expect_eq(out.get("cache-control") ?? "", "max-age=60");
        expect_eq(out.get("accept-encoding") ?? "", "gzip");
        expect_eq(out.get("cacheControl") == null, true);
        expect_eq(out.get("missing") == null, true);
    });

    test("3. @cap/kebabcase-keys — exclude list keeps specified keys untouched", () => {
        let src = MapObjResult.empty();
        * = src.push("firstName", "Linus");
        * = src.push("keepCamelKey", "exact");
        * = src.push("lastName", "Torvalds");
        * = src.push("rawId", "99");

        let excl: [2]string = ["keepCamelKey", "rawId"];
        let out = kebabcaseKeys(src, excl[0..2]);

        expect_eq(out.len(), 4 as usize);
        expect_eq(out.get("first-name") ?? "", "Linus");
        expect_eq(out.get("last-name") ?? "", "Torvalds");
        expect_eq(out.get("keepCamelKey") ?? "", "exact");
        expect_eq(out.get("rawId") ?? "", "99");
        expect_eq(out.get("keep-camel-key") == null, true);
    });

    test("4. @cap/kebabcase-keys — kebabcaseKeyInto single key & empty/small buffer safety", () => {
        let buf: [64]u8 = [0; 64];
        let n1 = kebabcaseKeyInto("backgroundColor", buf[0..64]);
        expect_eq(buf[0..n1], "background-color");

        let n2 = kebabcaseKeyInto("", buf[0..64]);
        expect_eq(n2, 0 as usize);

        let small: [4]u8 = [0; 4];
        let n3 = kebabcaseKeyInto("borderTopWidth", small[0..4]);
        expect_eq(n3, 4 as usize);
        expect_eq(small[0..n3], "bord");
    });

    test("5. @cap/kebabcase-keys — kebabcasePathInto recursive dot-separated path conversion", () => {
        let buf: [128]u8 = [0; 128];
        let n1 = kebabcasePathInto("userProfile.billingAddress.postalCode", buf[0..128]);
        expect_eq(buf[0..n1], "user-profile.billing-address.postal-code");

        let n2 = kebabcasePathInto("singleSegment", buf[0..128]);
        expect_eq(buf[0..n2], "single-segment");

        let n3 = kebabcasePathInto("", buf[0..128]);
        expect_eq(n3, 0 as usize);
    });

    test("6. @cap/kebabcase-keys — kebabcaseKeysBatch & inactive entry filtering", () => {
        let obj1 = MapObjResult.empty();
        * = obj1.push("itemName", "alpha");
        * = obj1.push("skipMe", "hidden");
        obj1.entries[1].active = false;

        let obj2 = MapObjResult.empty();
        * = obj2.push("maxRetries", "3");
        * = obj2.push("keepMe", "yes");

        let items: [2]MapObjResult = [obj1, obj2];
        let blank = DecamelizeKeysResult.empty();
        let results: [2]DecamelizeKeysResult = [blank; 2];
        let excl: [1]string = ["keepMe"];

        let count = kebabcaseKeysBatch(items[0..2], excl[0..1], results[0..2]);
        expect_eq(count, 2 as usize);
        expect_eq(results[0].len(), 1 as usize);
        expect_eq(results[0].get("item-name") ?? "", "alpha");
        expect_eq(results[0].get("skip-me") == null, true);

        expect_eq(results[1].len(), 2 as usize);
        expect_eq(results[1].get("max-retries") ?? "", "3");
        expect_eq(results[1].get("keepMe") ?? "", "yes");
    });
}
