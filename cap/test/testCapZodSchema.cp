import { test, expect_eq } from "cap:test";
import {
    Z_STRING,
    Z_NUMBER,
    Z_INT,
    Z_BOOL,
    Z_EMAIL,
    Z_URL,
    Z_UUID,
    Z_ENUM,
    Z_ARRAY,
    Z_OBJECT,
    Z_ERR_INVALID_JSON,
    Z_ERR_NOT_OBJECT,
    Z_ERR_REQUIRED,
    Z_ERR_INVALID_TYPE,
    Z_ERR_NOT_INT,
    Z_ERR_TOO_SMALL,
    Z_ERR_TOO_BIG,
    Z_ERR_INVALID_EMAIL,
    Z_ERR_INVALID_URL,
    Z_ERR_INVALID_UUID,
    Z_ERR_INVALID_ENUM,
    ZFieldRule,
    ZObjectSchema,
    zString,
    zNumber,
    zEmail,
    zUrl,
    zUuid,
    zEnum,
} from "../../packages/@cap/zod-schema/src/index.cp";

fn main() {
    test("1. @cap/zod-schema — Standalone validators (string/number/email/url/uuid/enum)", () => {
        expect_eq(zString("alice", 3, 16), true);
        expect_eq(zString("al", 3, 16), false);
        expect_eq(zString("\"hello\"", 1, 10), true);

        expect_eq(zNumber("42", 0.0, 100.0), true);
        expect_eq(zNumber("-3.5", -10.0, 10.0), true);
        expect_eq(zNumber("150", 0.0, 100.0), false);
        expect_eq(zNumber("NaN", 0.0, 100.0), false);
        expect_eq(zNumber("abc", 0.0, 100.0), false);

        expect_eq(zEmail("alice.smith+dev@example.com"), true);
        expect_eq(zEmail("bad@@example.com"), false);
        expect_eq(zEmail("no-domain@localhost"), false);

        expect_eq(zUrl("https://cap.dev/docs?v=1#top"), true);
        expect_eq(zUrl("not-a-valid-url"), false);

        expect_eq(zUuid("550e8400-e29b-41d4-a716-446655440000"), true);
        expect_eq(zUuid("550e8400-e29b-41d4-a716-44665544000Z"), false);
        expect_eq(zUuid("too-short"), false);

        expect_eq(zEnum("admin", "admin,user,guest"), true);
        expect_eq(zEnum("guest", "admin,user,guest"), true);
        expect_eq(zEnum("root", "admin,user,guest"), false);
    });

    test("2. @cap/zod-schema — ZObjectSchema.safeParseJson valid user signup payload", () => {
        let schema = ZObjectSchema.new();
        expect_eq(schema.field(ZFieldRule.strRange("username", true, 3, 20)), true);
        expect_eq(schema.field(ZFieldRule.basic("email", Z_EMAIL, true)), true);
        expect_eq(schema.field(ZFieldRule.basic("website", Z_URL, false)), true);
        expect_eq(schema.field(ZFieldRule.basic("id", Z_UUID, true)), true);
        expect_eq(schema.field(ZFieldRule.enumRule("role", true, "admin,user,guest")), true);
        expect_eq(schema.field(ZFieldRule.numRange("age", Z_INT, true, 18.0, 120.0)), true);
        expect_eq(schema.field(ZFieldRule.basic("active", Z_BOOL, true)), true);
        expect_eq(schema.field(ZFieldRule.basic("tags", Z_ARRAY, false)), true);
        expect_eq(schema.field(ZFieldRule.basic("meta", Z_OBJECT, false)), true);

        const payload = "{\"username\":\"alice\",\"email\":\"alice@example.com\",\"website\":\"https://alice.dev/home\",\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"role\":\"admin\",\"age\":28,\"active\":true,\"tags\":[\"cp\",\"zig\"],\"meta\":{\"plan\":\"pro\"}}";
        const res = schema.safeParseJson(payload);
        expect_eq(res.ok, true);
        expect_eq(res.issue_count, 0);
    });

    test("3. @cap/zod-schema — Collects multiple required & type mismatch issues in one pass", () => {
        let schema = ZObjectSchema.new();
        schema.field(ZFieldRule.basic("username", Z_STRING, true));
        schema.field(ZFieldRule.basic("age", Z_INT, true));
        schema.field(ZFieldRule.basic("active", Z_BOOL, true));
        schema.field(ZFieldRule.basic("email", Z_EMAIL, true));

        // username missing, age is float (not int), active is string (wrong type), email missing
        const payload = "{\"age\":21.5,\"active\":\"yes\"}";
        const res = schema.safeParseJson(payload);
        expect_eq(res.ok, false);
        expect_eq(res.issue_count, 4);
        expect_eq(res.hasIssue("age", Z_ERR_NOT_INT), true);
        expect_eq(res.hasIssue("active", Z_ERR_INVALID_TYPE), true);
        expect_eq(res.hasIssue("username", Z_ERR_REQUIRED), true);
        expect_eq(res.hasIssue("email", Z_ERR_REQUIRED), true);
    });

    test("4. @cap/zod-schema — Numeric min/max & string min_len/max_len boundary checks", () => {
        let schema = ZObjectSchema.new();
        schema.field(ZFieldRule.strRange("code", true, 4, 8));
        schema.field(ZFieldRule.numRange("score", Z_NUMBER, true, 0.0, 100.0));

        const res_low = schema.safeParseJson("{\"code\":\"ab\",\"score\":-0.1}");
        expect_eq(res_low.ok, false);
        expect_eq(res_low.issue_count, 2);
        expect_eq(res_low.hasIssue("code", Z_ERR_TOO_SMALL), true);
        expect_eq(res_low.hasIssue("score", Z_ERR_TOO_SMALL), true);

        const res_high = schema.safeParseJson("{\"code\":\"123456789\",\"score\":100.5}");
        expect_eq(res_high.ok, false);
        expect_eq(res_high.issue_count, 2);
        expect_eq(res_high.hasIssue("code", Z_ERR_TOO_BIG), true);
        expect_eq(res_high.hasIssue("score", Z_ERR_TOO_BIG), true);

        const res_exact = schema.safeParseJson("{\"code\":\"1234\",\"score\":100}");
        expect_eq(res_exact.ok, true);
    });

    test("5. @cap/zod-schema — Cross-package @cap/whatwg-url validation & enum rejection", () => {
        let schema = ZObjectSchema.new();
        schema.field(ZFieldRule.basic("homepage", Z_URL, true));
        schema.field(ZFieldRule.enumRule("role", true, "admin,user,guest"));
        schema.field(ZFieldRule.basic("token", Z_UUID, true));

        const bad = "{\"homepage\":\"://missing-scheme\",\"role\":\"superadmin\",\"token\":\"not-a-uuid\"}";
        const res = schema.safeParseJson(bad);
        expect_eq(res.ok, false);
        expect_eq(res.issue_count, 3);
        expect_eq(res.hasIssue("homepage", Z_ERR_INVALID_URL), true);
        expect_eq(res.hasIssue("role", Z_ERR_INVALID_ENUM), true);
        expect_eq(res.hasIssue("token", Z_ERR_INVALID_UUID), true);
    });

    test("6. @cap/zod-schema — Malformed & non-object JSON safely rejected", () => {
        let schema = ZObjectSchema.new();
        schema.field(ZFieldRule.basic("name", Z_STRING, true));

        const r1 = schema.safeParseJson("");
        expect_eq(r1.ok, false);
        expect_eq(r1.hasIssue("$", Z_ERR_INVALID_JSON), true);

        const r2 = schema.safeParseJson("[1, 2, 3]");
        expect_eq(r2.ok, false);
        expect_eq(r2.hasIssue("$", Z_ERR_NOT_OBJECT), true);

        const r3 = schema.safeParseJson("{\"name\": \"unterminated");
        expect_eq(r3.ok, false);
        expect_eq(r3.hasIssue("$", Z_ERR_INVALID_JSON), true);

        const r4 = schema.safeParseJson("{\"name\": \"ok\",}");
        expect_eq(r4.ok, false);
        expect_eq(r4.hasIssue("$", Z_ERR_INVALID_JSON), true);
    });
}
