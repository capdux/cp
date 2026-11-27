import { test, expect_eq } from "cap:test";
import {
    parseEmail,
    isValidEmail,
} from "../../packages/@cap/email-address/src/index.cp";

fn main() {
    test("1. @cap/email-address — parses standard email with subaddressing", () => {
        const e = parseEmail("alice.smith+tag@sub.example.com");
        expect_eq(e.valid, true);
        expect_eq(e.local_part, "alice.smith+tag");
        expect_eq(e.domain, "sub.example.com");
        expect_eq(e.display_name, "");
    });

    test("2. @cap/email-address — parses display name angle-addr format", () => {
        const e = parseEmail("Alice Dev <alice@capdux.org>");
        expect_eq(e.valid, true);
        expect_eq(e.display_name, "Alice Dev");
        expect_eq(e.local_part, "alice");
        expect_eq(e.domain, "capdux.org");
    });

    test("3. @cap/email-address — unquotes quoted display names", () => {
        const e = parseEmail("\"Bob, Jr.\" <bob@example.io>");
        expect_eq(e.valid, true);
        expect_eq(e.display_name, "Bob, Jr.");
        expect_eq(e.local_part, "bob");
    });

    test("4. @cap/email-address — rejects leading, trailing, or consecutive dots", () => {
        expect_eq(isValidEmail(".alice@example.com"), false);
        expect_eq(isValidEmail("alice.@example.com"), false);
        expect_eq(isValidEmail("a..b@example.com"), false);
        expect_eq(isValidEmail("alice@.example.com"), false);
        expect_eq(isValidEmail("alice@example..com"), false);
    });

    test("5. @cap/email-address — rejects invalid domain hyphens and multiple '@'", () => {
        expect_eq(isValidEmail("alice@-bad.com"), false);
        expect_eq(isValidEmail("alice@bad-.com"), false);
        expect_eq(isValidEmail("a@b@c.com"), false);
        expect_eq(isValidEmail("no-at-sign"), false);
    });

    test("6. @cap/email-address — rejects empty and whitespace-only strings", () => {
        expect_eq(isValidEmail(""), false);
        expect_eq(isValidEmail("   "), false);
        expect_eq(isValidEmail("@example.com"), false);
        expect_eq(isValidEmail("alice@"), false);
    });
}
