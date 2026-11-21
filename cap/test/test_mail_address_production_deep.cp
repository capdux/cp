import {
    isValidAddrSpec,
    Mailbox,
    MailboxListIter,
} from "cap:mail_address";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("isValidAddrSpec and Mailbox.parse", () => {
    expect(isValidAddrSpec("alice+tag@sub.example.com"));
    expect(!isValidAddrSpec(".bad@example.com"));
    expect(!isValidAddrSpec("a..b@example.com"));
    expect(!isValidAddrSpec("a@-bad.com"));
    expect(!isValidAddrSpec("a@b@c.com"));

    const m1 = Mailbox.parse("\"Smith, Alice\" <alice@example.com>").?;
    expect(strEq(m1.display_name.?, "Smith, Alice"));
    expect(strEq(m1.address, "alice@example.com"));
    expect(strEq(m1.local_part, "alice"));
    expect(strEq(m1.domain, "example.com"));

    const m2 = Mailbox.parse("bob@cp-lang.dev").?;
    expect(m2.display_name == null);
    expect(strEq(m2.address, "bob@cp-lang.dev"));
});

test("MailboxListIter handles quoted commas across multiple recipients", () => {
    const hdr = "\"Doe, Jane\" <jane@a.com>, Bob <bob@b.org>, plain@c.net";
    let it = MailboxListIter.init(hdr);

    const r1 = it.next().?;
    expect(strEq(r1.display_name.?, "Doe, Jane") && strEq(r1.address, "jane@a.com"));
    const r2 = it.next().?;
    expect(strEq(r2.display_name.?, "Bob") && strEq(r2.address, "bob@b.org"));
    const r3 = it.next().?;
    expect(r3.display_name == null && strEq(r3.address, "plain@c.net"));
    expect(it.next() == null);
});
