// cap/test/testCapDecamelizeKeys.cp
// Comprehensive unit tests for @cap/decamelize-keys

import { test, expect_eq } from "cap:test";
import { MapObjResult } from "../../packages/@cap/map-obj/src/index.cp";
import {
    DecamelizeKeysResult,
    decamelizeKeys,
} from "../../packages/@cap/decamelize-keys/src/index.cp";

fn main() {
    test("1. @cap/decamelize-keys — underscore separator converts camelCase & PascalCase keys", () => {
        let src = MapObjResult.empty();
        * = src.push("fooBar", "1");
        * = src.push("unicornRainbow", "2");
        * = src.push("UserProfileId", "3");

        let no_excl: [0]string = undefined;
        let out = decamelizeKeys(src, "_", no_excl[0..0]);

        expect_eq(out.len(), 3 as usize);
        expect_eq(out.get("foo_bar") ?? "", "1");
        expect_eq(out.get("unicorn_rainbow") ?? "", "2");
        expect_eq(out.get("user_profile_id") ?? "", "3");
    });

    test("2. @cap/decamelize-keys — custom separators ('-' and '.')", () => {
        let src = MapObjResult.empty();
        * = src.push("contentType", "application/json");
        * = src.push("cacheControl", "no-cache");

        let no_excl: [0]string = undefined;
        let dash = decamelizeKeys(src, "-", no_excl[0..0]);
        expect_eq(dash.len(), 2 as usize);
        expect_eq(dash.get("content-type") ?? "", "application/json");
        expect_eq(dash.get("cache-control") ?? "", "no-cache");

        let dot = decamelizeKeys(src, ".", no_excl[0..0]);
        expect_eq(dot.get("content.type") ?? "", "application/json");
        expect_eq(dot.get("cache.control") ?? "", "no-cache");
    });

    test("3. @cap/decamelize-keys — exclude list keeps matching keys unchanged", () => {
        let src = MapObjResult.empty();
        * = src.push("fooBar", "converted");
        * = src.push("keepCamel", "preserved");
        * = src.push("anotherKey", "converted2");
        * = src.push("rawOption", "preserved2");

        let excl: [2]string = ["keepCamel", "rawOption"];
        let out = decamelizeKeys(src, "-", excl[0..2]);

        expect_eq(out.len(), 4 as usize);
        expect_eq(out.get("foo-bar") ?? "", "converted");
        expect_eq(out.get("another-key") ?? "", "converted2");
        expect_eq(out.get("keepCamel") ?? "", "preserved");
        expect_eq(out.get("rawOption") ?? "", "preserved2");
        expect_eq(out.get("keep-camel") == null, true);
    });

    test("4. @cap/decamelize-keys — empty separator defaults to '_' & missing keys return null", () => {
        let src = MapObjResult.empty();
        * = src.push("maxRetryCount", "5");

        let no_excl: [0]string = undefined;
        let out = decamelizeKeys(src, "", no_excl[0..0]);

        expect_eq(out.get("max_retry_count") ?? "", "5");
        expect_eq(out.get("maxRetryCount") == null, true);
        expect_eq(out.get("does_not_exist") == null, true);
    });

    test("5. @cap/decamelize-keys — empty object, lowercase keys & inactive entries skipped", () => {
        let empty_src = MapObjResult.empty();
        let no_excl: [0]string = undefined;
        let empty_out = decamelizeKeys(empty_src, "_", no_excl[0..0]);
        expect_eq(empty_out.len(), 0 as usize);
        expect_eq(empty_out.get("x") == null, true);

        let src = MapObjResult.empty();
        * = src.push("plain", "ok");
        * = src.push("inactiveKey", "hidden");
        src.entries[1].active = false;
        * = src.push("activeKey", "shown");

        let out = decamelizeKeys(src, "_", no_excl[0..0]);
        expect_eq(out.len(), 2 as usize);
        expect_eq(out.get("plain") ?? "", "ok");
        expect_eq(out.get("active_key") ?? "", "shown");
        expect_eq(out.get("inactive_key") == null, true);
    });

    test("6. @cap/decamelize-keys — batch conversion & key buffer pool tracking", () => {
        let src = MapObjResult.empty();
        * = src.push("firstItem", "a");
        * = src.push("secondItem", "b");
        * = src.push("thirdItem", "c");
        * = src.push("fourthItem", "d");
        * = src.push("fifthItem", "e");

        let excl: [1]string = ["thirdItem"];
        let out = decamelizeKeys(src, "_", excl[0..1]);

        expect_eq(out.len(), 5 as usize);
        expect_eq(out.get("first_item") ?? "", "a");
        expect_eq(out.get("second_item") ?? "", "b");
        expect_eq(out.get("thirdItem") ?? "", "c");
        expect_eq(out.get("fourth_item") ?? "", "d");
        expect_eq(out.get("fifth_item") ?? "", "e");
        expect_eq(out.key_buf_len > 0, true);
    });
}
