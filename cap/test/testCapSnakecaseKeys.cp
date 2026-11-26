// cap/test/testCapSnakecaseKeys.cp
// Comprehensive unit tests for @cap/snakecase-keys

import { test, expect_eq } from "cap:test";
import { MapObjResult } from "../../packages/@cap/map-obj/src/index.cp";
import {
    SnakecaseKeysResult,
    snakecaseKeys,
    snakecaseKeyInto,
} from "../../packages/@cap/snakecase-keys/src/index.cp";

fn main() {
    test("1. @cap/snakecase-keys — snakecaseKeyInto single-key conversion", () => {
        let buf: [64]u8 = [0; 64];

        let n1 = snakecaseKeyInto("fooBarBaz", buf[0..64]);
        expect_eq(buf[0..n1], "foo_bar_baz");

        let n2 = snakecaseKeyInto("HTTPResponseCode", buf[0..64]);
        expect_eq(buf[0..n2], "http_response_code");

        let n3 = snakecaseKeyInto("user-first-name", buf[0..64]);
        expect_eq(buf[0..n3], "user_first_name");

        let n4 = snakecaseKeyInto("", buf[0..64]);
        expect_eq(n4, 0 as usize);
    });

    test("2. @cap/snakecase-keys — snakecaseKeys converts all active keys & preserves values", () => {
        let src = MapObjResult.empty();
        * = src.push("firstName", "Ada");
        * = src.push("lastName", "Lovelace");
        * = src.push("user-id", "42");
        * = src.push("HTTPStatus", "200");

        let no_excl: [0]string = undefined;
        let out = snakecaseKeys(src, no_excl[0..0]);

        expect_eq(out.len(), 4 as usize);
        expect_eq(out.get("first_name") ?? "", "Ada");
        expect_eq(out.get("last_name") ?? "", "Lovelace");
        expect_eq(out.get("user_id") ?? "", "42");
        expect_eq(out.get("http_status") ?? "", "200");
    });

    test("3. @cap/snakecase-keys — snakecaseKeys respects exclude list", () => {
        let src = MapObjResult.empty();
        * = src.push("firstName", "Grace");
        * = src.push("keepMeCamel", "unchanged");
        * = src.push("apiKey", "secret");
        * = src.push("rawHeader-X", "v1");

        let excl: [2]string = ["keepMeCamel", "rawHeader-X"];
        let out = snakecaseKeys(src, excl[0..2]);

        expect_eq(out.len(), 4 as usize);
        expect_eq(out.get("first_name") ?? "", "Grace");
        expect_eq(out.get("api_key") ?? "", "secret");
        expect_eq(out.get("keepMeCamel") ?? "", "unchanged");
        expect_eq(out.get("rawHeader-X") ?? "", "v1");
        expect_eq(out.get("keep_me_camel") == null, true);
    });

    test("4. @cap/snakecase-keys — get returns null for missing or pre-conversion keys", () => {
        let src = MapObjResult.empty();
        * = src.push("camelCaseKey", "val1");

        let no_excl: [0]string = undefined;
        let out = snakecaseKeys(src, no_excl[0..0]);

        expect_eq(out.get("camel_case_key") ?? "", "val1");
        expect_eq(out.get("camelCaseKey") == null, true);
        expect_eq(out.get("nonexistent") == null, true);
    });

    test("5. @cap/snakecase-keys — empty object, already snake_case & inactive entries skipped", () => {
        let empty_src = MapObjResult.empty();
        let no_excl: [0]string = undefined;
        let empty_out = snakecaseKeys(empty_src, no_excl[0..0]);
        expect_eq(empty_out.len(), 0 as usize);
        expect_eq(empty_out.get("anything") == null, true);

        let src = MapObjResult.empty();
        * = src.push("already_snake", "ok");
        * = src.push("skipInactive", "hidden");
        src.entries[1].active = false;
        * = src.push("nextKey", "visible");

        let out = snakecaseKeys(src, no_excl[0..0]);
        expect_eq(out.len(), 2 as usize);
        expect_eq(out.get("already_snake") ?? "", "ok");
        expect_eq(out.get("next_key") ?? "", "visible");
        expect_eq(out.get("skip_inactive") == null, true);
    });

    test("6. @cap/snakecase-keys — multiple entries & key buffer pool tracking", () => {
        let src = MapObjResult.empty();
        * = src.push("alphaOne", "1");
        * = src.push("betaTwo", "2");
        * = src.push("gammaThree", "3");
        * = src.push("deltaFour", "4");
        * = src.push("epsilonFive", "5");
        * = src.push("zetaSix", "6");

        let excl: [1]string = ["gammaThree"];
        let out = snakecaseKeys(src, excl[0..1]);

        expect_eq(out.len(), 6 as usize);
        expect_eq(out.get("alpha_one") ?? "", "1");
        expect_eq(out.get("beta_two") ?? "", "2");
        expect_eq(out.get("gammaThree") ?? "", "3");
        expect_eq(out.get("delta_four") ?? "", "4");
        expect_eq(out.get("epsilon_five") ?? "", "5");
        expect_eq(out.get("zeta_six") ?? "", "6");
        expect_eq(out.key_buf_len > 0, true);
    });
}
