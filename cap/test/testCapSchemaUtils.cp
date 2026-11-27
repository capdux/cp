import { test, expect_eq } from "cap:test";
import {
    SCHU_OK,
    SCHU_ERR_REQUIRED,
    SCHU_ERR_MIN_LEN,
    SCHU_ERR_MAX_LEN,
    SCHU_ERR_INT_MIN,
    SCHU_ERR_INT_MAX,
    SCHU_ERR_ENUM,
    FieldConstraint,
    ValidationResult,
    validateStringField,
    validateIntField,
    validateEnumField,
} from "../../packages/schema-utils/src/index.cp";

fn main() {
    test("1. @cap/schema-utils — validateStringField checks required vs optional missing", () => {
        const req = FieldConstraint{
            name: "entry",
            required: true,
            min_len: 1,
            max_len: 64,
            has_int_range: false,
            min_int: 0,
            max_int: 0,
        };
        const r1 = validateStringField("", false, req);
        expect_eq(r1.valid, false);
        expect_eq(r1.error_code, SCHU_ERR_REQUIRED);

        const opt = FieldConstraint{
            name: "label",
            required: false,
            min_len: 3,
            max_len: 10,
            has_int_range: false,
            min_int: 0,
            max_int: 0,
        };
        const r2 = validateStringField("", false, opt);
        expect_eq(r2.valid, true);
        expect_eq(r2.error_code, SCHU_OK);
    });

    test("2. @cap/schema-utils — validateStringField enforces min_len and max_len", () => {
        const c = FieldConstraint{
            name: "name",
            required: true,
            min_len: 3,
            max_len: 8,
            has_int_range: false,
            min_int: 0,
            max_int: 0,
        };
        expect_eq(validateStringField("ab", true, c).error_code, SCHU_ERR_MIN_LEN);
        expect_eq(validateStringField("abcdefghi", true, c).error_code, SCHU_ERR_MAX_LEN);
        const ok = validateStringField("webpack", true, c);
        expect_eq(ok.valid, true);
        expect_eq(ok.error_code, SCHU_OK);
    });

    test("3. @cap/schema-utils — validateIntField enforces required and range bounds", () => {
        const c = FieldConstraint{
            name: "port",
            required: true,
            min_len: 0,
            max_len: 0,
            has_int_range: true,
            min_int: 1,
            max_int: 65535,
        };
        expect_eq(validateIntField(0, false, c).error_code, SCHU_ERR_REQUIRED);
        expect_eq(validateIntField(0, true, c).error_code, SCHU_ERR_INT_MIN);
        expect_eq(validateIntField(70000, true, c).error_code, SCHU_ERR_INT_MAX);
        const ok = validateIntField(8080, true, c);
        expect_eq(ok.valid, true);
        expect_eq(ok.error_code, SCHU_OK);
    });

    test("4. @cap/schema-utils — validateIntField supports negative bounds and unbounded mode", () => {
        const neg = FieldConstraint{
            name: "offset",
            required: false,
            min_len: 0,
            max_len: 0,
            has_int_range: true,
            min_int: -100,
            max_int: -10,
        };
        expect_eq(validateIntField(-50, true, neg).valid, true);
        expect_eq(validateIntField(-101, true, neg).error_code, SCHU_ERR_INT_MIN);
        expect_eq(validateIntField(-5, true, neg).error_code, SCHU_ERR_INT_MAX);

        const unbounded = FieldConstraint{
            name: "any_int",
            required: false,
            min_len: 0,
            max_len: 0,
            has_int_range: false,
            min_int: 0,
            max_int: 0,
        };
        expect_eq(validateIntField(-999999, true, unbounded).valid, true);
    });

    test("5. @cap/schema-utils — validateEnumField accepts whitelisted values", () => {
        const modes: [3]string = ["development", "production", "none"];
        expect_eq(validateEnumField("development", modes[0..3]).valid, true);
        expect_eq(validateEnumField("production", modes[0..3]).valid, true);
        expect_eq(validateEnumField("none", modes[0..3]).valid, true);
    });

    test("6. @cap/schema-utils — validateEnumField rejects unknown values with SCHU_ERR_ENUM", () => {
        const modes: [2]string = ["cjs", "esm"];
        const res = validateEnumField("umd", modes[0..2]);
        expect_eq(res.valid, false);
        expect_eq(res.error_code, SCHU_ERR_ENUM);
        expect_eq(res.message, "value is not in allowed enum list");
    });
}
