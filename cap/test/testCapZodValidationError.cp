import { test, expect_eq } from "cap:test";
import {
    VALIDATION_ERROR_NAME,
    ValidationIssue,
    FormatErrorOptions,
    MessageBuilderOptions,
    ValidationError,
    defaultFormatErrorOptions,
    defaultMessageBuilderOptions,
    formatValidationIssues,
    formatValidationIssue,
    formatIssuePath,
    createMessage,
    fromZodError,
    fromError,
    fromErrorMessage,
    isValidationError,
    isValidationErrorLike,
} from "../../packages/zod-validation-error/src/index.cp";

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

    test("7. @cap/zod-validation-error — fromZodError, fromError, ValidationError & guards", () => {
        const issues: [3]ValidationIssue = [
            ValidationIssue{ path: "items[0].sku", message: "Required", code: "invalid_type" },
            ValidationIssue{ path: "items[1].qty", message: "Must be > 0", code: "too_small" },
            ValidationIssue{ path: "total", message: "Too large", code: "too_big" },
        ];
        let opts = defaultMessageBuilderOptions();
        opts.max_issues_in_message = 2;
        opts.include_code = true;

        let verr = fromZodError(issues[0..3], opts);
        expect_eq(verr.name, VALIDATION_ERROR_NAME);
        expect_eq(verr.isValidationError(), true);
        expect_eq(verr.issueCount(), 3 as usize);
        expect_eq(verr.detailsCount(), 2 as usize);
        expect_eq(verr.isTruncated(), true);
        expect_eq(
            verr.toString(),
            "Validation error: Required [invalid_type] at \"items[0].sku\"; Must be > 0 [too_small] at \"items[1].qty\"",
        );

        let verr2 = fromError(issues[0..1], defaultMessageBuilderOptions());
        expect_eq(verr2.message(), "Validation error: Required at \"items[0].sku\"");

        let plain = fromErrorMessage("Unexpected token", defaultMessageBuilderOptions());
        expect_eq(plain.toString(), "Validation error: Unexpected token");

        expect_eq(isValidationError("ZodValidationError"), true);
        expect_eq(isValidationError("Error"), false);
        expect_eq(isValidationErrorLike("ZodValidationError", true), true);
        expect_eq(isValidationErrorLike("ZodValidationError", false), false);
    });

    test("8. @cap/zod-validation-error — formatIssuePath & formatValidationIssue", () => {
        const segs: [4]string = ["orders", "0", "meta.key", "title"];
        let buf: [128]u8 = [0; 128];
        const n = formatIssuePath(segs[0..4], buf[0..128]);
        expect_eq(buf[0..n] as string, "orders[0][\"meta.key\"].title");

        const single = ValidationIssue{ path: "id", message: "Invalid UUID", code: "invalid_string" };
        const m = formatValidationIssue(single, true, true, buf[0..128]);
        expect_eq(buf[0..m] as string, "Invalid UUID [invalid_string] at \"id\"");

        const one_issue: [1]ValidationIssue = [
            ValidationIssue{ path: "k", message: "Bad", code: "c" },
        ];
        const cm = createMessage(
            one_issue[0..1],
            defaultMessageBuilderOptions(),
            buf[0..128],
        );
        expect_eq(buf[0..cm] as string, "Validation error: Bad at \"k\"");
    });
}
