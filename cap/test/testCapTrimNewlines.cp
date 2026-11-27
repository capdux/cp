import { test, expect_eq } from "cap:test";
import {
    trimNewlines,
    trimNewlinesStart,
    trimNewlinesEnd,
    countLeadingNewlines,
    countTrailingNewlines,
} from "../../packages/trim-newlines/src/index.cp";

fn main() {
    test("1. @cap/trim-newlines — trims leading and trailing LF/CRLF", () => {
        expect_eq(trimNewlines("\nunicorn\r\n"), "unicorn");
        expect_eq(trimNewlines("\r\n\n\runicorn\n\r\n"), "unicorn");
    });

    test("2. @cap/trim-newlines — preserves internal newlines and spaces/tabs", () => {
        expect_eq(trimNewlines("\n  hello\nworld  \r\n"), "  hello\nworld  ");
        expect_eq(trimNewlines("\tfoo\t\n"), "\tfoo\t");
    });

    test("3. @cap/trim-newlines — trimNewlinesStart only trims start", () => {
        expect_eq(trimNewlinesStart("\r\n\nunicorn\r\n"), "unicorn\r\n");
        expect_eq(trimNewlinesStart("unicorn\n"), "unicorn\n");
    });

    test("4. @cap/trim-newlines — trimNewlinesEnd only trims end", () => {
        expect_eq(trimNewlinesEnd("\r\nunicorn\r\n\n"), "\r\nunicorn");
        expect_eq(trimNewlinesEnd("\nunicorn"), "\nunicorn");
    });

    test("5. @cap/trim-newlines — empty and newline-only strings", () => {
        expect_eq(trimNewlines(""), "");
        expect_eq(trimNewlines("\r\n\n\r"), "");
        expect_eq(trimNewlinesStart("\n\n"), "");
        expect_eq(trimNewlinesEnd("\r\r"), "");
    });

    test("6. @cap/trim-newlines — counts leading and trailing newline bytes", () => {
        expect_eq(countLeadingNewlines("\r\n\nabc\r\n"), 3 as usize);
        expect_eq(countTrailingNewlines("\r\n\nabc\r\n"), 2 as usize);
        expect_eq(countLeadingNewlines("abc"), 0 as usize);
        expect_eq(countTrailingNewlines("abc"), 0 as usize);
    });
}
