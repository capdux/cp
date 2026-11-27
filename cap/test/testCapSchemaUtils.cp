import { test, expect_eq } from "cap:test";
import {
    SCHU_OK,
    SCHU_ERR_REQUIRED,
    SCHU_ERR_MIN_LEN,
    SCHU_ERR_MAX_LEN,
    SCHU_ERR_INT_MIN,
    SCHU_ERR_INT_MAX,
    SCHU_ERR_ENUM,
    SCHU_ERR_TYPE,
    SCHU_ERR_PATTERN,
    SCHU_ERR_ADDITIONAL_PROP,
    SCHU_ERR_MULTIPLE_OF,
    FieldConstraint,
    ValidationResult,
    SchemaType,
    SchemaProperty,
    SchemaValue,
    ObjectSchema,
    validateStringField,
    validateIntField,
    validateEnumField,
    validatePatternField,
    matchPattern,
    validate,
    formatValidationError,
    formatValidationReport,
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

    test("7. @cap/schema-utils — matchPattern & validatePatternField", () => {
        expect_eq(matchPattern("^[a-z][a-z0-9_-]*$", "my_loader-1"), true);
        expect_eq(matchPattern("^[a-z][a-z0-9_-]*$", "1bad"), false);
        expect_eq(matchPattern("^\\d+$", "8080"), true);
        expect_eq(matchPattern("^\\d+$", "80a0"), false);
        expect_eq(validatePatternField("abc.js", "\\.js$").valid, true);
        expect_eq(validatePatternField("abc.ts", "\\.js$").error_code, SCHU_ERR_PATTERN);
    });

    test("8. @cap/schema-utils — validate full ObjectSchema (type/required/min/max/pattern/additionalProperties)", () => {
        let port_prop = SchemaProperty.intProp("port", true, true, 1, true, 65535);
        port_prop.multiple_of = 2;
        const props: [3]SchemaProperty = [
            SchemaProperty.stringProp("name", true, 2, 16, "^[a-z0-9-]+$"),
            port_prop,
            SchemaProperty.boolProp("minify", false),
        ];
        const schema = ObjectSchema {
            title: "BundlerPlugin",
            properties: props[0..3],
            additional_properties: false,
        };

        const good: [3]SchemaValue = [
            SchemaValue.str("name", "app-bundle"),
            SchemaValue.int("port", 8080),
            SchemaValue.boolean("minify", true),
        ];
        const ok = validate(schema, good[0..3]);
        expect_eq(ok.valid, true);
        expect_eq(ok.error_count, 0 as usize);

        const bad: [3]SchemaValue = [
            SchemaValue.str("name", "INVALID!"),
            SchemaValue.int("port", 8081),
            SchemaValue.str("extra", "nope"),
        ];
        const rep = validate(schema, bad[0..3]);
        expect_eq(rep.valid, false);
        expect_eq(rep.error_count, 3 as usize);
        expect_eq(rep.errors[0].code, SCHU_ERR_PATTERN);
        expect_eq(rep.errors[1].code, SCHU_ERR_MULTIPLE_OF);
        expect_eq(rep.errors[2].code, SCHU_ERR_ADDITIONAL_PROP);

        let err_buf: [256]u8 = [0; 256];
        const en = formatValidationError("BundlerPlugin", rep.errors[0], err_buf[0..256]);
        expect_eq(en > 40 as usize, true);

        let rep_buf: [512]u8 = [0; 512];
        const rn = formatValidationReport("BundlerPlugin", rep, rep_buf[0..512]);
        expect_eq(rn > en, true);
    });

    test("9. @cap/schema-utils — validate catches type mismatch & missing required fields", () => {
        const props: [2]SchemaProperty = [
            SchemaProperty.stringProp("entry", true, 1, 64, ""),
            SchemaProperty.intProp("retries", true, true, 0, true, 10),
        ];
        const schema = ObjectSchema {
            title: "Loader",
            properties: props[0..2],
            additional_properties: true,
        };
        const bad: [1]SchemaValue = [
            SchemaValue.int("entry", 42),
        ];
        const rep = validate(schema, bad[0..1]);
        expect_eq(rep.valid, false);
        expect_eq(rep.error_count, 2 as usize);
        expect_eq(rep.errors[0].code, SCHU_ERR_TYPE);
        expect_eq(rep.errors[1].code, SCHU_ERR_REQUIRED);
    });
}
