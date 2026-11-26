import { test, expect_eq } from "cap:test";
import {
    ValidationIssue,
    FormatErrorOptions,
    defaultFormatErrorOptions,
    formatValidationIssues,
} from "../../packages/@cap/zod-validation-error/src/index.cp";

fn main() {
    test("1. @cap/zod-validation-error — formats multiple issues with default options", () => {
        const issues: [2]ValidationIssue = [
            ValidationIssue{ path: "user.email", message: "Expected string", code: "invalid_type" },
            ValidationIssue{ path: "age", message: "Number must be >= 0", code: "too_small" },
        ];
        let buf: [256]u8 = [0; 256];
        const n = formatValidationIssues(issues[0..2], defaultFormatErrorOptions(), buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "Validation error: Expected string at \"user.email\"; Number must be >= 0 at \"age\"",
        );
    });

    test("2. @cap/zod-validation-error — omits path suffix when path is empty or include_path is false", () => {
        const issues: [2]ValidationIssue = [
            ValidationIssue{ path: "", message: "Invalid input", code: "custom" },
            ValidationIssue{ path: "name", message: "Required", code: "invalid_type" },
        ];
        let buf: [256]u8 = [0; 256];
        let opts = defaultFormatErrorOptions();
        opts.include_path = false;
        const n = formatValidationIssues(issues[0..2], opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "Validation error: Invalid input; Required");
    });

    test("3. @cap/zod-validation-error — truncates issues using max_issues", () => {
        const issues: [3]ValidationIssue = [
            ValidationIssue{ path: "a", message: "Err A", code: "c1" },
            ValidationIssue{ path: "b", message: "Err B", code: "c2" },
            ValidationIssue{ path: "c", message: "Err C", code: "c3" },
        ];
        let buf: [256]u8 = [0; 256];
        let opts = defaultFormatErrorOptions();
        opts.max_issues = 2;
        const n = formatValidationIssues(issues[0..3], opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "Validation error: Err A at \"a\"; Err B at \"b\"");
    });

    test("4. @cap/zod-validation-error — custom prefix and separators", () => {
        const issues: [2]ValidationIssue = [
            ValidationIssue{ path: "host", message: "Invalid URL", code: "invalid_string" },
            ValidationIssue{ path: "port", message: "Out of range", code: "too_big" },
        ];
        const opts = FormatErrorOptions{
            prefix: "ConfigError",
            prefix_sep: " -> ",
            issue_sep: " | ",
            max_issues: 0,
            include_path: true,
        };
        let buf: [256]u8 = [0; 256];
        const n = formatValidationIssues(issues[0..2], opts, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "ConfigError -> Invalid URL at \"host\" | Out of range at \"port\"",
        );
    });

    test("5. @cap/zod-validation-error — empty prefix and empty issues list", () => {
        const dummy: [1]ValidationIssue = [
            ValidationIssue{ path: "x", message: "Bad", code: "c" },
        ];
        let buf: [128]u8 = [0; 128];
        const n_empty = formatValidationIssues(dummy[0..0], defaultFormatErrorOptions(), buf[0..128]);
        expect_eq(buf[0..n_empty] as string, "Validation error");

        let no_prefix = defaultFormatErrorOptions();
        no_prefix.prefix = "";
        const n_no_prefix = formatValidationIssues(dummy[0..1], no_prefix, buf[0..128]);
        expect_eq(buf[0..n_no_prefix] as string, "Bad at \"x\"");
    });

    test("6. @cap/zod-validation-error — safely truncates when output buffer is small", () => {
        const issues: [1]ValidationIssue = [
            ValidationIssue{ path: "email", message: "Expected string", code: "invalid_type" },
        ];
        let small: [18]u8 = [0; 18];
        const n = formatValidationIssues(issues[0..1], defaultFormatErrorOptions(), small[0..18]);
        expect_eq(n, 18);
        expect_eq(small[0..n] as string, "Validation error: ");
    });
}
