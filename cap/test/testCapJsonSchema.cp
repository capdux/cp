// cap/test/testCapJsonSchema.cp
// Comprehensive unit tests for @cap/ajv (JSON Schema Draft-07 validation)

import { test, expect_eq } from "cap:test";
import {
    AJV_ERR_TYPE,
    AJV_ERR_REQUIRED,
    AJV_ERR_MINIMUM,
    AJV_ERR_MIN_LENGTH,
    AjvSchema,
    validateFormat,
    validateWithSchema,
} from "../../packages/ajv/src/index.cp";

fn main() {
    test("1. @cap/ajv — compiled object schema validation & error codes", () => {
        const schema_json = "{\"type\":\"object\",\"required\":[\"name\",\"age\"],\"properties\":{\"name\":{\"type\":\"string\",\"minLength\":2},\"age\":{\"type\":\"integer\",\"minimum\":18}}}";
        const schema = AjvSchema.compile(schema_json);
        expect_eq(schema.ok, true);

        const ok = schema.validate("{\"name\":\"Alice\",\"age\":30}");
        expect_eq(ok.valid, true);
        expect_eq(ok.error_count, 0 as usize);

        const bad = schema.validate("{\"name\":\"A\",\"age\":12}");
        expect_eq(bad.valid, false);
        expect_eq(bad.hasError("/name", "minLength", AJV_ERR_MIN_LENGTH), true);
        expect_eq(bad.hasError("/age", "minimum", AJV_ERR_MINIMUM), true);

        const missing = schema.validate("{\"age\":\"oops\"}");
        expect_eq(missing.valid, false);
        expect_eq(missing.hasError("/name", "required", AJV_ERR_REQUIRED), true);
        expect_eq(missing.hasError("/age", "type", AJV_ERR_TYPE), true);
    });

    test("2. @cap/ajv — validateWithSchema & built-in format checks", () => {
        expect_eq(validateFormat("email", "user@example.com"), true);
        expect_eq(validateFormat("email", "not-an-email"), false);
        expect_eq(validateFormat("ipv4", "10.0.0.1"), true);
        expect_eq(validateFormat("uuid", "550e8400-e29b-41d4-a716-446655440000"), true);

        const s = "{\"type\":\"array\",\"minItems\":2,\"items\":{\"type\":\"integer\"}}";
        expect_eq(validateWithSchema(s, "[1,2,3]").valid, true);
        expect_eq(validateWithSchema(s, "[1]").valid, false);
    });
}
