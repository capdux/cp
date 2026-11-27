// cap/test/testCapJsonSchema.cp
// Comprehensive unit tests for @cap/json-schema

import { test, expect_eq } from "cap:test";
import {
    JsonType,
    SchemaRule,
    ValidationError,
    ValidationResult,
    detectJsonType,
    validateJson,
    stringRule,
    numberRule,
    integerRule,
    arrayRule,
    objectRule,
} from "../../packages/ajv/src/index.cp";

fn main() {
    test("1. @cap/json-schema — detectJsonType (Integer vs Number, String, Bool, Null, Array, Object)", () => {
        expect_eq(detectJsonType("null") == JsonType.Null, true);
        expect_eq(detectJsonType("true") == JsonType.Boolean, true);
        expect_eq(detectJsonType("false") == JsonType.Boolean, true);
        expect_eq(detectJsonType("42") == JsonType.Integer, true);
        expect_eq(detectJsonType("-100") == JsonType.Integer, true);
        expect_eq(detectJsonType("3.14") == JsonType.Number, true);
        expect_eq(detectJsonType("1e5") == JsonType.Number, true);
        expect_eq(detectJsonType("\"hello\"") == JsonType.String, true);
        expect_eq(detectJsonType("[1, 2, 3]") == JsonType.Array, true);
        expect_eq(detectJsonType("{\"k\": \"v\"}") == JsonType.Object, true);
        expect_eq(detectJsonType("{broken") == JsonType.Any, true);
    });

    test("2. @cap/json-schema — String minLength / maxLength (incl. UTF-8 & escapes)", () => {
        const rule = stringRule(2, 5);
        expect_eq(validateJson("\"ok\"", rule).valid, true);
        expect_eq(validateJson("\"你好CP\"", rule).valid, true); // 4 chars
        expect_eq(validateJson("\"a\\nb\"", rule).valid, true); // 3 logical chars

        const too_short = validateJson("\"x\"", rule);
        expect_eq(too_short.valid, false);
        expect_eq(too_short.first_error.code, "minLength");

        const too_long = validateJson("\"abcdef\"", rule);
        expect_eq(too_long.valid, false);
        expect_eq(too_long.first_error.code, "maxLength");

        const wrong_type = validateJson("123", rule);
        expect_eq(wrong_type.valid, false);
        expect_eq(wrong_type.first_error.code, "type");
    });

    test("3. @cap/json-schema — Integer & Number bounds (inclusive & exclusive)", () => {
        const age_rule = integerRule(0.0, 150.0);
        expect_eq(validateJson("0", age_rule).valid, true);
        expect_eq(validateJson("25", age_rule).valid, true);
        expect_eq(validateJson("150", age_rule).valid, true);
        expect_eq(validateJson("25.5", age_rule).valid, false); // float rejected for Integer
        expect_eq(validateJson("-1", age_rule).first_error.code, "minimum");
        expect_eq(validateJson("200", age_rule).first_error.code, "maximum");

        let ratio_rule = numberRule(0.0, 1.0);
        ratio_rule.setExclusive(true, true);
        expect_eq(validateJson("0.5", ratio_rule).valid, true);
        expect_eq(validateJson("0", ratio_rule).first_error.code, "exclusiveMinimum");
        expect_eq(validateJson("1.0", ratio_rule).first_error.code, "exclusiveMaximum");
    });

    test("4. @cap/json-schema — Array minItems / maxItems with nested elements", () => {
        const rule = arrayRule(1, 3);
        expect_eq(validateJson("[1]", rule).valid, true);
        expect_eq(validateJson("[{\"a\": [1, 2]}, \"x\", true]", rule).valid, true);

        const empty = validateJson("[]", rule);
        expect_eq(empty.valid, false);
        expect_eq(empty.first_error.code, "minItems");

        const too_many = validateJson("[1, 2, 3, 4]", rule);
        expect_eq(too_many.valid, false);
        expect_eq(too_many.first_error.code, "maxItems");
    });

    test("5. @cap/json-schema — Object required keys (top-level only, ignores nested keys)", () => {
        let user_rule = objectRule();
        user_rule.addRequired("id");
        user_rule.addRequired("email");
        user_rule.addRequired("role");

        const good = validateJson("{\"id\": 42, \"email\": \"dev@capdux.org\", \"role\": \"admin\"}", user_rule);
        expect_eq(good.valid, true);

        // Nested "email" must NOT satisfy top-level required "email"
        const nested_only = validateJson("{\"id\": 1, \"meta\": {\"email\": \"x@y.z\", \"role\": \"u\"}}", user_rule);
        expect_eq(nested_only.valid, false);
        expect_eq(nested_only.error_count, 2 as usize);
        expect_eq(nested_only.first_error.code, "required");
        expect_eq(nested_only.first_error.path, "email");
    });

    test("6. @cap/json-schema — Enum allowlist & invalid JSON rejection", () => {
        let status_rule = stringRule(1, 20);
        status_rule.addEnum("pending");
        status_rule.addEnum("active");
        status_rule.addEnum("closed");

        expect_eq(validateJson("\"active\"", status_rule).valid, true);
        const bad_enum = validateJson("\"deleted\"", status_rule);
        expect_eq(bad_enum.valid, false);
        expect_eq(bad_enum.first_error.code, "enum");

        // Malformed JSON
        const bad_json = validateJson("{\"a\": 1,}", objectRule());
        expect_eq(bad_json.valid, false);
        expect_eq(bad_json.first_error.code, "invalid_json");
    });
}
