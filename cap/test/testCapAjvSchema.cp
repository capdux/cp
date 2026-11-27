import { test, expect_eq } from "cap:test";
import {
    AJV_ERR_INVALID_SCHEMA,
    AJV_ERR_INVALID_DATA,
    AJV_ERR_TYPE,
    AJV_ERR_REQUIRED,
    AJV_ERR_ADDITIONAL_PROPERTIES,
    AJV_ERR_MINIMUM,
    AJV_ERR_MAXIMUM,
    AJV_ERR_EXCLUSIVE_MINIMUM,
    AJV_ERR_EXCLUSIVE_MAXIMUM,
    AJV_ERR_MIN_LENGTH,
    AJV_ERR_MAX_LENGTH,
    AJV_ERR_MIN_ITEMS,
    AJV_ERR_MAX_ITEMS,
    AJV_ERR_ENUM,
    AJV_ERR_CONST,
    AJV_ERR_FORMAT,
    AjvSchema,
    validateFormat,
    validateWithSchema,
} from "../../packages/ajv/src/index.cp";

fn main() {
    test("1. @cap/ajv-schema — Object schema (required/properties/type/min/max/minLength)", () => {
        const schema_json = "{\"type\":\"object\",\"required\":[\"id\",\"name\"],\"properties\":{\"id\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":999},\"name\":{\"type\":\"string\",\"minLength\":3,\"maxLength\":16},\"score\":{\"type\":\"number\",\"exclusiveMinimum\":0,\"exclusiveMaximum\":100}}}";
        const schema = AjvSchema.compile(schema_json);
        expect_eq(schema.ok, true);

        const ok_res = schema.validate("{\"id\":42,\"name\":\"alice\",\"score\":88.5}");
        expect_eq(ok_res.valid, true);
        expect_eq(ok_res.error_count, 0);

        const bad_res = schema.validate("{\"id\":0,\"score\":100}");
        expect_eq(bad_res.valid, false);
        expect_eq(bad_res.hasError("/name", "required", AJV_ERR_REQUIRED), true);
        expect_eq(bad_res.hasError("/id", "minimum", AJV_ERR_MINIMUM), true);
        expect_eq(bad_res.hasError("/score", "exclusiveMaximum", AJV_ERR_EXCLUSIVE_MAXIMUM), true);

        const bad_len = schema.validate("{\"id\":1.5,\"name\":\"ab\",\"score\":0}");
        expect_eq(bad_len.valid, false);
        expect_eq(bad_len.hasError("/id", "type", AJV_ERR_TYPE), true);
        expect_eq(bad_len.hasError("/name", "minLength", AJV_ERR_MIN_LENGTH), true);
        expect_eq(bad_len.hasError("/score", "exclusiveMinimum", AJV_ERR_EXCLUSIVE_MINIMUM), true);
    });

    test("2. @cap/ajv-schema — additionalProperties: false rejects undeclared fields", () => {
        const schema_json = "{\"type\":\"object\",\"additionalProperties\":false,\"properties\":{\"host\":{\"type\":\"string\"},\"port\":{\"type\":\"integer\",\"maximum\":65535}}}";
        const ok = validateWithSchema(schema_json, "{\"host\":\"localhost\",\"port\":8080}");
        expect_eq(ok.valid, true);

        const bad = validateWithSchema(schema_json, "{\"host\":\"localhost\",\"port\":70000,\"debug\":true}");
        expect_eq(bad.valid, false);
        expect_eq(bad.hasError("/port", "maximum", AJV_ERR_MAXIMUM), true);
        expect_eq(bad.hasError("/debug", "additionalProperties", AJV_ERR_ADDITIONAL_PROPERTIES), true);
    });

    test("3. @cap/ajv-schema — Format suite (email/uri/uuid/ipv4/ipv6/date) cross-package reuse", () => {
        expect_eq(validateFormat("email", "dev@cap.io"), true);
        expect_eq(validateFormat("email", "bad-email"), false);
        expect_eq(validateFormat("uri", "https://cap.dev/pkg"), true);
        expect_eq(validateFormat("uri", "not a uri"), false);
        expect_eq(validateFormat("uuid", "550e8400-e29b-41d4-a716-446655440000"), true);
        expect_eq(validateFormat("uuid", "invalid-uuid"), false);
        expect_eq(validateFormat("ipv4", "192.168.1.10"), true);
        expect_eq(validateFormat("ipv4", "999.1.1.1"), false);
        expect_eq(validateFormat("ipv6", "2001:db8::1"), true);
        expect_eq(validateFormat("ipv6", "2001:::1"), false);
        expect_eq(validateFormat("date", "2024-02-29"), true);
        expect_eq(validateFormat("date", "2023-02-29"), false);

        const schema_json = "{\"type\":\"object\",\"required\":[\"email\",\"site\",\"id\",\"v4\",\"v6\",\"dob\"],\"properties\":{\"email\":{\"type\":\"string\",\"format\":\"email\"},\"site\":{\"type\":\"string\",\"format\":\"uri\"},\"id\":{\"type\":\"string\",\"format\":\"uuid\"},\"v4\":{\"type\":\"string\",\"format\":\"ipv4\"},\"v6\":{\"type\":\"string\",\"format\":\"ipv6\"},\"dob\":{\"type\":\"string\",\"format\":\"date\"}}}";
        const ok = validateWithSchema(schema_json, "{\"email\":\"a@b.com\",\"site\":\"https://a.com\",\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"v4\":\"10.0.0.1\",\"v6\":\"::1\",\"dob\":\"2026-11-27\"}");
        expect_eq(ok.valid, true);

        const bad = validateWithSchema(schema_json, "{\"email\":\"a@b.com\",\"site\":\"https://a.com\",\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"v4\":\"256.0.0.1\",\"v6\":\"not-v6\",\"dob\":\"2026-13-01\"}");
        expect_eq(bad.valid, false);
        expect_eq(bad.error_count, 3);
        expect_eq(bad.hasError("/v4", "format", AJV_ERR_FORMAT), true);
        expect_eq(bad.hasError("/v6", "format", AJV_ERR_FORMAT), true);
        expect_eq(bad.hasError("/dob", "format", AJV_ERR_FORMAT), true);
    });

    test("4. @cap/ajv-schema — enum & const exact semantic matching", () => {
        const schema_json = "{\"type\":\"object\",\"properties\":{\"role\":{\"enum\":[\"admin\",\"user\",\"guest\"]},\"version\":{\"const\":2},\"tag\":{\"type\":\"string\",\"maxLength\":5}}}";
        const ok = validateWithSchema(schema_json, "{\"role\":\"user\",\"version\":2.0,\"tag\":\"alpha\"}");
        expect_eq(ok.valid, true);

        const bad = validateWithSchema(schema_json, "{\"role\":\"root\",\"version\":3,\"tag\":\"too-long\"}");
        expect_eq(bad.valid, false);
        expect_eq(bad.hasError("/role", "enum", AJV_ERR_ENUM), true);
        expect_eq(bad.hasError("/version", "const", AJV_ERR_CONST), true);
        expect_eq(bad.hasError("/tag", "maxLength", AJV_ERR_MAX_LENGTH), true);
    });

    test("5. @cap/ajv-schema — Array minItems, maxItems & items validation", () => {
        const schema_json = "{\"type\":\"array\",\"minItems\":2,\"maxItems\":4,\"items\":{\"type\":\"integer\",\"minimum\":10}}";
        const ok = validateWithSchema(schema_json, "[10, 20, 30]");
        expect_eq(ok.valid, true);

        const too_few = validateWithSchema(schema_json, "[10]");
        expect_eq(too_few.valid, false);
        expect_eq(too_few.hasCode(AJV_ERR_MIN_ITEMS), true);

        const too_many = validateWithSchema(schema_json, "[10, 20, 30, 40, 50]");
        expect_eq(too_many.valid, false);
        expect_eq(too_many.hasCode(AJV_ERR_MAX_ITEMS), true);

        const bad_item = validateWithSchema(schema_json, "[10, 5]");
        expect_eq(bad_item.valid, false);
        expect_eq(bad_item.hasCode(AJV_ERR_MINIMUM), true);
    });

    test("6. @cap/ajv-schema — Malformed schema & data JSON safely rejected", () => {
        const bad_schema = validateWithSchema("{invalid json", "{\"a\":1}");
        expect_eq(bad_schema.valid, false);
        expect_eq(bad_schema.hasCode(AJV_ERR_INVALID_SCHEMA), true);

        const non_obj_schema = validateWithSchema("[1, 2, 3]", "{}");
        expect_eq(non_obj_schema.valid, false);
        expect_eq(non_obj_schema.hasCode(AJV_ERR_INVALID_SCHEMA), true);

        const bad_data = validateWithSchema("{\"type\":\"object\"}", "{\"a\": 1,}");
        expect_eq(bad_data.valid, false);
        expect_eq(bad_data.hasCode(AJV_ERR_INVALID_DATA), true);
    });
}
