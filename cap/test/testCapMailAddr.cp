// cap/test/testCapMailAddr.cp
// Comprehensive unit tests for @cap/mail-addr

import { test, expect_eq } from "cap:test";
import {
    MailAddress,
    parseMailbox,
    parseAddressList,
    formatMailbox,
} from "../../packages/addressparser/src/index.cp";

fn main() {
    test("1. @cap/mail-addr — Plain email address & whitespace/comment stripping", () => {
        const m1 = parseMailbox("alice@example.com");
        expect_eq(m1.valid, true);
        expect_eq(m1.display_name, "");
        expect_eq(m1.address, "alice@example.com");
        expect_eq(m1.local, "alice");
        expect_eq(m1.domain, "example.com");

        const m2 = parseMailbox("  (support team) bob.dev+tag@sub.corp.io (work)  ");
        expect_eq(m2.valid, true);
        expect_eq(m2.display_name, "");
        expect_eq(m2.address, "bob.dev+tag@sub.corp.io");
        expect_eq(m2.local, "bob.dev+tag");
        expect_eq(m2.domain, "sub.corp.io");
    });

    test("2. @cap/mail-addr — Angle-bracket display name format", () => {
        const m1 = parseMailbox("Alice Smith <alice@example.com>");
        expect_eq(m1.valid, true);
        expect_eq(m1.display_name, "Alice Smith");
        expect_eq(m1.address, "alice@example.com");
        expect_eq(m1.local, "alice");
        expect_eq(m1.domain, "example.com");

        const m2 = parseMailbox("<noreply@cap-lang.org>");
        expect_eq(m2.valid, true);
        expect_eq(m2.display_name, "");
        expect_eq(m2.address, "noreply@cap-lang.org");
    });

    test("3. @cap/mail-addr — Quoted display names with commas & escaped quotes", () => {
        const m1 = parseMailbox("\"Smith, Alice\" <alice@example.com>");
        expect_eq(m1.valid, true);
        expect_eq(m1.display_name, "Smith, Alice");
        expect_eq(m1.address, "alice@example.com");

        const m2 = parseMailbox("\"Bob \\\"The Builder\\\"\" <bob@corp.io>");
        expect_eq(m2.valid, true);
        expect_eq(m2.display_name, "Bob \\\"The Builder\\\"");
        expect_eq(m2.address, "bob@corp.io");
    });

    test("4. @cap/mail-addr — Mixed multi-recipient comma-separated list", () => {
        const list = parseAddressList("Alice <a@x.com>, \"Doe, John\" <j@y.org>, plain@z.net, (Lead) Carol <c@dev.io> (HQ)");
        expect_eq(list.count, 4 as usize);

        expect_eq(list.items[0].valid, true);
        expect_eq(list.items[0].display_name, "Alice");
        expect_eq(list.items[0].address, "a@x.com");

        expect_eq(list.items[1].valid, true);
        expect_eq(list.items[1].display_name, "Doe, John");
        expect_eq(list.items[1].address, "j@y.org");

        expect_eq(list.items[2].valid, true);
        expect_eq(list.items[2].display_name, "");
        expect_eq(list.items[2].address, "plain@z.net");

        expect_eq(list.items[3].valid, true);
        expect_eq(list.items[3].display_name, "Carol");
        expect_eq(list.items[3].address, "c@dev.io");
        expect_eq(list.get(99 as usize) == null, true);
    });

    test("5. @cap/mail-addr — formatMailbox canonical quoting & escaping", () => {
        let out1: [128]u8 = undefined;
        const mPlain = parseMailbox("plain@z.net");
        expect_eq(formatMailbox(mPlain, out1[0..128]), "plain@z.net");

        let out2: [128]u8 = undefined;
        const mSimple = parseMailbox("Alice Smith <alice@example.com>");
        expect_eq(formatMailbox(mSimple, out2[0..128]), "Alice Smith <alice@example.com>");

        let out3: [128]u8 = undefined;
        const mComma = parseMailbox("\"Doe, John\" <j@y.org>");
        expect_eq(formatMailbox(mComma, out3[0..128]), "\"Doe, John\" <j@y.org>");

        let out4: [128]u8 = undefined;
        const mQuote = MailAddress{
            valid: true,
            display_name: "Bob \"Pro\" Dev",
            address: "bob@corp.io",
            local: "bob",
            domain: "corp.io",
        };
        expect_eq(formatMailbox(mQuote, out4[0..128]), "\"Bob \\\"Pro\\\" Dev\" <bob@corp.io>");
    });

    test("6. @cap/mail-addr — Reject malformed email addresses", () => {
        expect_eq(parseMailbox("").valid, false);
        expect_eq(parseMailbox("no-at-sign.com").valid, false);
        expect_eq(parseMailbox("@example.com").valid, false);
        expect_eq(parseMailbox("alice@").valid, false);
        expect_eq(parseMailbox("alice@localhost").valid, false);
        expect_eq(parseMailbox("alice@@example.com").valid, false);
        expect_eq(parseMailbox("alice smith@example.com").valid, false);
        expect_eq(parseMailbox("Alice <alice@example.com").valid, false);
        expect_eq(parseMailbox("Alice <alice@example.com> extra").valid, false);
        expect_eq(parseMailbox("\"Unclosed <alice@example.com>").valid, false);
        expect_eq(parseMailbox(".alice@example.com").valid, false);
        expect_eq(parseMailbox("alice@example..com").valid, false);
    });
}
