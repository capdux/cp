// Source: cp/cap/test/test_email_production_deep.cp
// Deep Production Verification Suite for cap:email & cap.email

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EmailError,
    EmailAddress,
    containsHeaderInjection,
    QuotedPrintable,
    EmailAttachment,
    EmailBuilder,
    ParsedEmail,
    MimeParser,
    SmtpReply,
    SmtpCodec,
} from "cap:email";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn hasSubstr(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn testRfc5322AddressAndInjectionGuard(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Display name + angle-bracket address
    const a1 = EmailAddress.parse("Alice <alice@example.com>")?;
    expect_eq(strEql(a1.display_name, "Alice"), true);
    expect_eq(strEql(a1.local_part, "alice"), true);
    expect_eq(strEql(a1.domain, "example.com"), true);

    const f1 = a1.format(alloc)?;
    defer alloc.free(f1);
    expect_eq(strEql(f1, "Alice <alice@example.com>"), true);

    const e1 = a1.emailOnly(alloc)?;
    defer alloc.free(e1);
    expect_eq(strEql(e1, "alice@example.com"), true);

    // 2. Quoted display name with comma & bare address
    const a2 = EmailAddress.parse("\"Doe, Bob\" <bob+tag@sub.example.org>")?;
    expect_eq(strEql(a2.display_name, "Doe, Bob"), true);
    expect_eq(strEql(a2.local_part, "bob+tag"), true);
    expect_eq(strEql(a2.domain, "sub.example.org"), true);

    const f2 = a2.format(alloc)?;
    defer alloc.free(f2);
    expect_eq(strEql(f2, "\"Doe, Bob\" <bob+tag@sub.example.org>"), true);

    const a3 = EmailAddress.parse("bob@sub.example.org")?;
    expect_eq(strEql(a3.display_name, ""), true);
    const f3 = a3.format(alloc)?;
    defer alloc.free(f3);
    expect_eq(strEql(f3, "bob@sub.example.org"), true);

    // 3. Valid & invalid addresses (local > 64, total > 254, .., spaces, missing domain dot)
    expect_eq(EmailAddress.isValid("user.name+1@cp-lang.org"), true);
    expect_eq(EmailAddress.isValid("a..b@example.com"), false);
    expect_eq(EmailAddress.isValid(".alice@example.com"), false);
    expect_eq(EmailAddress.isValid("alice.@example.com"), false);
    expect_eq(EmailAddress.isValid("alice @example.com"), false);
    expect_eq(EmailAddress.isValid("alice@example..com"), false);
    expect_eq(EmailAddress.isValid("alice@localhost"), false);
    expect_eq(EmailAddress.isValid("missing-at-sign.com"), false);

    // Local part > 64 bytes (65 + 12 = 77)
    let long_local: [77]u8 = undefined;
    @memset(long_local[0..65], 'a');
    @memcpy(long_local[65..77], "@example.com");
    expect_eq(EmailAddress.isValid(&long_local), false);

    // Total > 254 bytes (64 + 1 + 190 = 255)
    let over_254: [255]u8 = undefined;
    @memset(over_254[0..64], 'a');
    over_254[64] = '@';
    @memset(over_254[65..128], 'b');
    over_254[128] = '.';
    @memset(over_254[129..192], 'c');
    over_254[192] = '.';
    @memset(over_254[193..251], 'd');
    @memcpy(over_254[251..255], ".com");
    expect_eq(EmailAddress.isValid(&over_254), false);

    // 4. CRLF & NUL header injection guard
    expect_eq(containsHeaderInjection("ok@example.com"), false);
    expect_eq(containsHeaderInjection("bad@example.com\r\nBcc: evil@x.com"), true);

    let got_inj_cr = false;
    * = EmailAddress.parse("alice@example.com\r\nBcc: evil@evil.org") catch |e| {
        if (e == EmailError.HeaderInjection) got_inj_cr = true;
    };
    expect_eq(got_inj_cr, true);

    let got_inj_lf = false;
    * = EmailAddress.parse("Alice\n<alice@example.com>") catch |e| {
        if (e == EmailError.HeaderInjection) got_inj_lf = true;
    };
    expect_eq(got_inj_lf, true);
}

fn testQuotedPrintableRoundTrip(): !void {
    const alloc = std.heap.page_allocator;

    // 1. UTF-8, '=' sign, trailing space before CRLF/EOF, and 76-col soft wrapping
    const original = "Price = $100 \r\nUTF-8: 邮件测试 🚀\r\n" ++
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    const encoded = QuotedPrintable.encode(alloc, original)?;
    defer alloc.free(encoded);

    expect_eq(hasSubstr(encoded, "=3D"), true);
    expect_eq(hasSubstr(encoded, "=20\r\n"), true);
    expect_eq(hasSubstr(encoded, "=\r\n"), true);

    // Verify every physical QP line length <= 76
    let line_iter = std.mem.splitScalar(u8, encoded, '\n');
    while (raw_line := line_iter.next()) {
        const line = if (raw_line.len > 0 && raw_line[raw_line.len - 1] == '\r') raw_line[0 .. raw_line.len - 1] else raw_line;
        expect_eq(line.len <= 76, true);
    }

    const decoded = QuotedPrintable.decode(alloc, encoded)?;
    defer alloc.free(decoded);
    expect_eq(strEql(decoded, original), true);

    // 2. Explicit soft line breaks + lowercase/uppercase hex decoding
    const soft_in = "Hello =\r\nWorld=21=0aNext=3dLine";
    const soft_out = QuotedPrintable.decode(alloc, soft_in)?;
    defer alloc.free(soft_out);
    expect_eq(strEql(soft_out, "Hello World!\nNext=Line"), true);

    // 3. Reject malformed '=' escape
    let got_invalid = false;
    * = QuotedPrintable.decode(alloc, "bad=ZZescape") catch |e| {
        if (e == EmailError.InvalidMime) got_invalid = true;
    };
    expect_eq(got_invalid, true);
}

fn testMimeBuilderAndParserRoundTrip(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Subject CRLF injection must be blocked
    let guard_builder = EmailBuilder.init(alloc);
    defer guard_builder.deinit();
    let subj_inj = false;
    guard_builder.setSubject("Hello\r\nX-Evil: 1") catch |e| {
        if (e == EmailError.HeaderInjection) subj_inj = true;
    };
    expect_eq(subj_inj, true);

    // 2. Full multipart/mixed + nested multipart/alternative + RFC 2047 UTF-8 Subject + 76-col Base64 attachment
    let payload: [128]u8 = undefined;
    let idx: usize = 0;
    while (idx < payload.len) : (idx += 1) {
        payload[idx] = ((idx * 7 + 13) & 0xFF) as u8;
    }

    let builder = EmailBuilder.init(alloc);
    defer builder.deinit();
    builder.setFrom("Sender <sender@cp-lang.org>")?;
    builder.addTo("Alice <alice@example.com>")?;
    builder.addCc("audit@example.com")?;
    builder.setSubject("周报：系统状态更新 🚀 Status")?;
    builder.setTextBody("Plain text body: A = B");
    builder.setHtmlBody("<html><body><h1>Hello 中文</h1></body></html>");
    builder.addAttachment(EmailAttachment.init("report.bin", "application/octet-stream", &payload))?;

    const raw_msg = builder.build(alloc)?;
    defer alloc.free(raw_msg);

    // Verify wire structures: RFC 2047 UTF-8 Base64, multipart/mixed, multipart/alternative, 76-col Base64 wrap
    expect_eq(hasSubstr(raw_msg, "Subject: =?UTF-8?B?"), true);
    expect_eq(hasSubstr(raw_msg, "Content-Type: multipart/mixed;"), true);
    expect_eq(hasSubstr(raw_msg, "Content-Type: multipart/alternative;"), true);
    expect_eq(hasSubstr(raw_msg, "Content-Transfer-Encoding: base64\r\n"), true);

    // Parse back with MimeParser (including header unfolding & RFC 2047 decode)
    let parsed = MimeParser.parse(alloc, raw_msg)?;
    defer parsed.deinit();

    expect_eq(strEql(parsed.from, "Sender <sender@cp-lang.org>"), true);
    expect_eq(strEql(parsed.to, "Alice <alice@example.com>"), true);
    expect_eq(strEql(parsed.cc, "audit@example.com"), true);
    expect_eq(strEql(parsed.subject, "周报：系统状态更新 🚀 Status"), true);
    expect_eq(parsed.text_body != null, true);
    expect_eq(parsed.html_body != null, true);
    expect_eq(strEql(parsed.text_body.?, "Plain text body: A = B"), true);
    expect_eq(strEql(parsed.html_body.?, "<html><body><h1>Hello 中文</h1></body></html>"), true);
    expect_eq(parsed.attachments.len, 1);
    expect_eq(strEql(parsed.attachments[0].filename, "report.bin"), true);
    expect_eq(strEql(parsed.attachments[0].content_type, "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, parsed.attachments[0].data, &payload), true);

    // 3. Folded header parsing directly via MimeParser
    const folded_mail =
        "From: ops@cp-lang.org\r\n" ++
        "Subject: Multi-line\r\n" ++
        "   folded header\r\n" ++
        "\tvalue\r\n" ++
        "Content-Type: text/plain; charset=UTF-8\r\n" ++
        "\r\n" ++
        "Body text";
    let parsed_folded = MimeParser.parse(alloc, folded_mail)?;
    defer parsed_folded.deinit();
    expect_eq(strEql(parsed_folded.subject, "Multi-line folded header value"), true);
    expect_eq(strEql(parsed_folded.text_body.?, "Body text"), true);
}

fn testSmtpCodecAndDotStuffing(): !void {
    const alloc = std.heap.page_allocator;

    // 1. EHLO, MAIL FROM, RCPT TO, AUTH PLAIN
    const ehlo = SmtpCodec.encodeEhlo(alloc, "client.cp-lang.org")?;
    defer alloc.free(ehlo);
    expect_eq(strEql(ehlo, "EHLO client.cp-lang.org\r\n"), true);

    const mail_from = SmtpCodec.encodeMailFrom(alloc, "Alice <alice@example.com>")?;
    defer alloc.free(mail_from);
    expect_eq(strEql(mail_from, "MAIL FROM:<alice@example.com>\r\n"), true);

    const mail_bounce = SmtpCodec.encodeMailFrom(alloc, "")?;
    defer alloc.free(mail_bounce);
    expect_eq(strEql(mail_bounce, "MAIL FROM:<>\r\n"), true);

    const rcpt_to = SmtpCodec.encodeRcptTo(alloc, "bob@sub.example.org")?;
    defer alloc.free(rcpt_to);
    expect_eq(strEql(rcpt_to, "RCPT TO:<bob@sub.example.org>\r\n"), true);

    // AUTH PLAIN ("\0alice\0secret" -> base64 "AGFsaWNlAHNlY3JldA==")
    const auth = SmtpCodec.encodeAuthPlain(alloc, "alice", "secret")?;
    defer alloc.free(auth);
    expect_eq(strEql(auth, "AUTH PLAIN AGFsaWNlAHNlY3JldA==\r\n"), true);

    // 2. Dot-stuffing & dot-unstuffing round-trip (RFC 5321 §4.5.2)
    const raw_body = ".leading dot\r\nnormal line\r\n..\r\n.end";
    const stuffed = SmtpCodec.dotStuffBody(alloc, raw_body)?;
    defer alloc.free(stuffed);
    expect_eq(strEql(stuffed, "..leading dot\r\nnormal line\r\n...\r\n..end\r\n.\r\n"), true);

    const unstuffed = SmtpCodec.dotUnstuffBody(alloc, stuffed)?;
    defer alloc.free(unstuffed);
    expect_eq(strEql(unstuffed, raw_body), true);

    // 3. Single-line & multi-line 250- reply parsing
    const r_ok = SmtpCodec.parseReply("250 2.0.0 Ok: queued\r\n")?;
    expect_eq(r_ok.code, 250);
    expect_eq(r_ok.is_multiline, false);
    expect_eq(r_ok.isPositive(), true);
    expect_eq(strEql(r_ok.message, "2.0.0 Ok: queued"), true);

    const r_multi = SmtpCodec.parseReply("250-smtp.cp-lang.org\r\n250-PIPELINING\r\n250 8BITMIME\r\n")?;
    expect_eq(r_multi.code, 250);
    expect_eq(r_multi.is_multiline, true);
    expect_eq(r_multi.isPositive(), true);
    expect_eq(hasSubstr(r_multi.message, "PIPELINING"), true);
    expect_eq(hasSubstr(r_multi.message, "8BITMIME"), true);

    const r_err = SmtpCodec.parseReply("535 5.7.8 Authentication failed\r\n")?;
    expect_eq(r_err.code, 535);
    expect_eq(r_err.isPositive(), false);
    expect_eq(r_err.isPermanentNegative(), true);

    let bad_reply = false;
    * = SmtpCodec.parseReply("250-first\r\n500 mismatched\r\n") catch |e| {
        if (e == EmailError.InvalidSmtpReply) bad_reply = true;
    };
    expect_eq(bad_reply, true);
}

fn testCapEmailDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    expect_eq(cap.email.EmailAddress.isValid("agent@cp-lang.org"), true);
    const addr = cap.email.EmailAddress.parse("Agent <agent@cp-lang.org>")?;
    const formatted = addr.format(alloc)?;
    defer alloc.free(formatted);
    expect_eq(strEql(formatted, "Agent <agent@cp-lang.org>"), true);

    let builder = cap.email.EmailBuilder.init(alloc);
    defer builder.deinit();
    builder.setFrom("sender@cp-lang.org")?;
    builder.addTo("agent@cp-lang.org")?;
    builder.setSubject("Dynamic Mount 🚀")?;
    builder.setTextBody("Works via cap.email!");
    const wire = builder.build(alloc)?;
    defer alloc.free(wire);

    let parsed = cap.email.MimeParser.parse(alloc, wire)?;
    defer parsed.deinit();
    expect_eq(strEql(parsed.subject, "Dynamic Mount 🚀"), true);
    expect_eq(strEql(parsed.text_body.?, "Works via cap.email!"), true);

    const ehlo = cap.email.SmtpCodec.encodeEhlo(alloc, "cp-lang.org")?;
    defer alloc.free(ehlo);
    expect_eq(strEql(ehlo, "EHLO cp-lang.org\r\n"), true);
}

fn main() {
    test("1. RFC 5322 EmailAddress Parse, Format, Validation & CRLF Injection Guard", () => {
        testRfc5322AddressAndInjectionGuard()?;
    });
    test("2. RFC 2045 QuotedPrintable Encode/Decode & 76-Col Soft Line Breaks", () => {
        testQuotedPrintableRoundTrip()?;
    });
    test("3. EmailBuilder (multipart/mixed + alternative + 76-col Base64 + RFC 2047) & MimeParser", () => {
        testMimeBuilderAndParserRoundTrip()?;
    });
    test("4. SmtpCodec EHLO/MAIL/RCPT/AUTH PLAIN, Multi-line 250- Reply & Dot-Stuffing", () => {
        testSmtpCodecAndDotStuffing()?;
    });
    test("5. import cap from \"cap\"; cap.email Automatic Dynamic Mounting", () => {
        testCapEmailDynamicMount()?;
    });
}
