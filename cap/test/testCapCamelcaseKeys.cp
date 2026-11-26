import { test, expect_eq } from "cap:test";
import {
    convertJsonKeysToCamel,
    convertJsonKeysToSnake,
    convertJsonKeysToPascal,
} from "../../packages/@cap/camelcase-keys/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/camelcase-keys — Flat object converts keys only, preserves string values", () => {
        let buf: [128]u8 = undefined;
        const src: string = "{\"user_id\":1,\"first_name\":\"foo_bar\",\"is_active\":true}";
        const n = convertJsonKeysToCamel(src, &buf);
        expect_eq(
            strEq(buf[0..n], "{\"userId\":1,\"firstName\":\"foo_bar\",\"isActive\":true}"),
            true
        );
    });

    test("2. @cap/camelcase-keys — Nested objects & arrays of objects converted recursively", () => {
        let buf: [256]u8 = undefined;
        const src: string = "{\"api_data\":{\"item_list\":[{\"created_at\":100,\"tag_name\":\"raw_tag\"}]}}";
        const n = convertJsonKeysToCamel(src, &buf);
        expect_eq(
            strEq(buf[0..n], "{\"apiData\":{\"itemList\":[{\"createdAt\":100,\"tagName\":\"raw_tag\"}]}}"),
            true
        );
    });

    test("3. @cap/camelcase-keys — Kebab-case keys like 'content-type' convert to 'contentType'", () => {
        let buf: [128]u8 = undefined;
        const src: string = "{\"content-type\":\"application/json\",\"x-request-id\":\"abc-def\"}";
        const n = convertJsonKeysToCamel(src, &buf);
        expect_eq(
            strEq(buf[0..n], "{\"contentType\":\"application/json\",\"xRequestId\":\"abc-def\"}"),
            true
        );
    });

    test("4. @cap/camelcase-keys — convertJsonKeysToSnake reverse conversion", () => {
        let buf: [128]u8 = undefined;
        const src: string = "{\"userId\":42,\"firstName\":\"camelValue\",\"httpStatus\":200}";
        const n = convertJsonKeysToSnake(src, &buf);
        expect_eq(
            strEq(buf[0..n], "{\"user_id\":42,\"first_name\":\"camelValue\",\"http_status\":200}"),
            true
        );
    });

    test("5. @cap/camelcase-keys — convertJsonKeysToPascal conversion", () => {
        let buf: [128]u8 = undefined;
        const src: string = "{\"user_id\":7,\"display-name\":\"alice_smith\"}";
        const n = convertJsonKeysToPascal(src, &buf);
        expect_eq(
            strEq(buf[0..n], "{\"UserId\":7,\"DisplayName\":\"alice_smith\"}"),
            true
        );
    });

    test("6. @cap/camelcase-keys — Escaped quotes in values, empty object & small buffer safety", () => {
        let buf: [128]u8 = undefined;
        const escaped: string = "{\"raw_msg\":\"he said \\\"fake_key\\\": 1\"}";
        const n1 = convertJsonKeysToCamel(escaped, &buf);
        expect_eq(
            strEq(buf[0..n1], "{\"rawMsg\":\"he said \\\"fake_key\\\": 1\"}"),
            true
        );

        const n_empty_obj = convertJsonKeysToCamel("{}", &buf);
        expect_eq(strEq(buf[0..n_empty_obj], "{}"), true);
        expect_eq(convertJsonKeysToCamel("", &buf), 0);

        // Small buffer truncation safety
        let small: [8]u8 = undefined;
        const nt = convertJsonKeysToCamel("{\"long_key_name\":123}", &small);
        expect_eq(nt, 8);
        expect_eq(strEq(small[0..nt], "{\"longKe"), true);
    });
}
