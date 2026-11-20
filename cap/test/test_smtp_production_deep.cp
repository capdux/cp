// Source: cp/cap/test/test_smtp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:smtp & cap.smtp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SmtpError,
    SmtpReply,
    SmtpCommandCodec,
    SmtpAttachment,
    MailBuilder,
    parseReply,
    dotStuff,
    dotUnstuff,
} from "cap:smtp";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Single-line reply
    let rep1 = parseReply(alloc, "250 OK\r\n")?;
    defer rep1.deinit(alloc);
    expect_eq(rep1.code, 250 as u16);
    expect_eq(rep1.enhanced == null, true);
    expect_eq(rep1.lines.len, 1 as usize);
    expect_eq(std.mem.eql(u8, rep1.lines[0], "OK"), true);
    expect_eq(rep1.isSuccess(), true);
    expect_eq(rep1.isTransientError(), false);
    expect_eq(rep1.isPermanentError(), false);

    // 2. Multi-line EHLO reply
    let rep2 = parseReply(alloc, "250-PIPELINING\r\n250 8BITMIME\r\n250 AUTH PLAIN LOGIN\r\n")?;
    defer rep2.deinit(alloc);
    expect_eq(rep2.code, 250 as u16);
    expect_eq(rep2.lines.len, 3 as usize);
    expect_eq(std.mem.eql(u8, rep2.lines[0], "PIPELINING"), true);
    expect_eq(std.mem.eql(u8, rep2.lines[1], "8BITMIME"), true);
    expect_eq(std.mem.eql(u8, rep2.lines[2], "AUTH PLAIN LOGIN"), true);

    // 3. Enhanced status codes (4xx transient & 5xx permanent)
    let rep3 = parseReply(alloc, "451 4.3.2 Temporary server error\r\n")?;
    defer rep3.deinit(alloc);
    expect_eq(rep3.code, 451 as u16);
    expect_eq(rep3.isTransientError(), true);
    expect_eq(std.mem.eql(u8, rep3.enhanced.?, "4.3.2"), true);

    let rep4 = parseReply(alloc, "550-5.1.1 User unknown\r\n550 5.1.1 Please check recipient\r\n")?;
    defer rep4.deinit(alloc);
    expect_eq(rep4.code, 550 as u16);
    expect_eq(rep4.isPermanentError(), true);
    expect_eq(std.mem.eql(u8, rep4.enhanced.?, "5.1.1"), true);

    // 4. SmtpCommandCodec commands
    const ehlo = SmtpCommandCodec.ehlo(alloc, "client.cp.dev")?;
    defer alloc.free(ehlo);
    expect_eq(std.mem.eql(u8, ehlo, "EHLO client.cp.dev\r\n"), true);

    // RFC 4616: "\x00alice\x00secret" -> Base64 "AGFsaWNlAHNlY3JldA=="
    const auth = SmtpCommandCodec.authPlain(alloc, "alice", "secret")?;
    defer alloc.free(auth);
    expect_eq(std.mem.eql(u8, auth, "AUTH PLAIN AGFsaWNlAHNlY3JldA==\r\n"), true);

    const mf = SmtpCommandCodec.mailFrom(alloc, "alice@cp.dev")?;
    defer alloc.free(mf);
    expect_eq(std.mem.eql(u8, mf, "MAIL FROM:<alice@cp.dev>\r\n"), true);

    const mf_null = SmtpCommandCodec.mailFrom(alloc, "")?;
    defer alloc.free(mf_null);
    expect_eq(std.mem.eql(u8, mf_null, "MAIL FROM:<>\r\n"), true);

    const rt = SmtpCommandCodec.rcptTo(alloc, "bob@cp.dev")?;
    defer alloc.free(rt);
    expect_eq(std.mem.eql(u8, rt, "RCPT TO:<bob@cp.dev>\r\n"), true);

    expect_eq(std.mem.eql(u8, SmtpCommandCodec.dataStart(), "DATA\r\n"), true);
    expect_eq(std.mem.eql(u8, SmtpCommandCodec.quit(), "QUIT\r\n"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // Leading dots on multiple lines, double dots, mid-line dots
    const raw = ".leading single\r\n..leading double\r\nnormal.dot\r\n.\r\n...";
    const stuffed = dotStuff(alloc, raw)?;
    defer alloc.free(stuffed);

    expect_eq(
        std.mem.eql(u8, stuffed, "..leading single\r\n...leading double\r\nnormal.dot\r\n..\r\n....\r\n.\r\n"),
        true,
    );

    const unstuffed = dotUnstuff(alloc, stuffed)?;
    defer alloc.free(unstuffed);
    expect_eq(std.mem.eql(u8, unstuffed, raw), true);

    // Empty body round-trip
    const empty_stuffed = dotStuff(alloc, "")?;
    defer alloc.free(empty_stuffed);
    expect_eq(std.mem.eql(u8, empty_stuffed, "\r\n.\r\n"), true);
    const empty_unstuffed = dotUnstuff(alloc, empty_stuffed)?;
    defer alloc.free(empty_unstuffed);
    expect_eq(empty_unstuffed.len, 0 as usize);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    let mb = MailBuilder.init(alloc, "sender@cp.dev", "你好 CP Mail");
    defer mb.deinit();

    mb.addTo("to1@cp.dev")?;
    mb.addTo("to2@cp.dev")?;
    mb.addCc("cc1@cp.dev")?;
    mb.setTextBody("Plain text body");
    mb.setHtmlBody("<html><body><h1>HTML body</h1></body></html>");

    // 70-byte binary payload (>57 bytes so Base64 wraps across 76-col boundary)
    let bin_buf: [70]u8 = undefined;
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        bin_buf[i] = (i & 0xFF) as u8;
    }
    mb.addAttachment(SmtpAttachment{
        filename: "report.bin",
        content_type: "application/octet-stream",
        data: &bin_buf,
    })?;

    const msg = mb.build(alloc, "b-outer-01")?;
    defer alloc.free(msg);

    expect_eq(std.mem.indexOf(u8, msg, "From: sender@cp.dev\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "To: to1@cp.dev, to2@cp.dev\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "Cc: cc1@cp.dev\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "Subject: =?UTF-8?B?") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "multipart/mixed; boundary=\"b-outer-01\"") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "multipart/alternative; boundary=\"b-outer-01_alt\"") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "Plain text body") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "<h1>HTML body</h1>") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "filename=\"report.bin\"") != null, true);
    expect_eq(std.mem.indexOf(u8, msg, "Content-Transfer-Encoding: base64\r\n\r\n") != null, true);
    expect_eq(std.mem.endsWith(u8, msg, "--b-outer-01--\r\n"), true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. HeaderInjection in commands
    let inj_ehlo = false;
    * = SmtpCommandCodec.ehlo(alloc, "evil.com\r\nQUIT") catch |e| {
        if (e == SmtpError.HeaderInjection) inj_ehlo = true;
    };
    expect_eq(inj_ehlo, true);

    let inj_mf = false;
    * = SmtpCommandCodec.mailFrom(alloc, "a@b.com>\r\nRCPT TO:<evil@x.com") catch |e| {
        if (e == SmtpError.HeaderInjection) inj_mf = true;
    };
    expect_eq(inj_mf, true);

    let inj_auth = false;
    * = SmtpCommandCodec.authPlain(alloc, "user\n", "pass") catch |e| {
        if (e == SmtpError.HeaderInjection) inj_auth = true;
    };
    expect_eq(inj_auth, true);

    // 2. InvalidAddress
    let bad_rcpt = false;
    * = SmtpCommandCodec.rcptTo(alloc, "not-an-email") catch |e| {
        if (e == SmtpError.InvalidAddress) bad_rcpt = true;
    };
    expect_eq(bad_rcpt, true);

    // 3. HeaderInjection in MailBuilder (To, Subject, Attachment filename, From)
    let mb1 = MailBuilder.init(alloc, "ok@cp.dev", "Subject\r\nBcc: spy@evil.com");
    defer mb1.deinit();
    let inj_subj = false;
    * = mb1.build(alloc, "b1") catch |e| {
        if (e == SmtpError.HeaderInjection) inj_subj = true;
    };
    expect_eq(inj_subj, true);

    let mb2 = MailBuilder.init(alloc, "ok@cp.dev", "Clean");
    defer mb2.deinit();
    let inj_att = false;
    mb2.addAttachment(SmtpAttachment{
        filename: "bad\r\nname.txt",
        content_type: "text/plain",
        data: "x",
    }) catch |e| {
        if (e == SmtpError.HeaderInjection) inj_att = true;
    };
    expect_eq(inj_att, true);

    // 4. InvalidReply & TruncatedLine
    let bad_rep = false;
    * = parseReply(alloc, "250-First\r\n251 Mismatched\r\n") catch |e| {
        if (e == SmtpError.InvalidReply) bad_rep = true;
    };
    expect_eq(bad_rep, true);

    let trunc_rep = false;
    * = parseReply(alloc, "250-Only continuation\r\n") catch |e| {
        if (e == SmtpError.TruncatedLine) trunc_rep = true;
    };
    expect_eq(trunc_rep, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.smtp
    let rep = cap.smtp.parseReply(alloc, "220 mail.cp.dev ESMTP ready\r\n")?;
    defer rep.deinit(alloc);
    expect_eq(rep.code, 220 as u16);
    expect_eq(rep.isSuccess(), true);

    const ehlo = cap.smtp.SmtpCommandCodec.ehlo(alloc, "app.cp.dev")?;
    defer alloc.free(ehlo);
    expect_eq(std.mem.eql(u8, ehlo, "EHLO app.cp.dev\r\n"), true);

    let mb = cap.smtp.MailBuilder.init(alloc, "bot@cp.dev", "ASCII Subject");
    defer mb.deinit();
    mb.addTo("dev@cp.dev")?;
    mb.setTextBody("Ping");
    const mail = mb.build(alloc, "b-cap")?;
    defer alloc.free(mail);

    const wire = cap.smtp.dotStuff(alloc, mail)?;
    defer alloc.free(wire);
    const back = cap.smtp.dotUnstuff(alloc, wire)?;
    defer alloc.free(back);
    expect_eq(std.mem.eql(u8, back, mail), true);
}

test("cap:smtp 1 - single/multi-line reply parsing, enhanced codes, and command codec") {
    runTest1() catch |e| {
        std.debug.print("test1 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:smtp 2 - RFC 5321 dotStuff and dotUnstuff round-trip") {
    runTest2() catch |e| {
        std.debug.print("test2 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:smtp 3 - MailBuilder UTF-8 RFC 2047 subject, alternative + mixed + Base64 attachment") {
    runTest3() catch |e| {
        std.debug.print("test3 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:smtp 4 - HeaderInjection, InvalidAddress, InvalidReply, TruncatedLine guards") {
    runTest4() catch |e| {
        std.debug.print("test4 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:smtp 5 - cap.smtp dynamic mount and std.testing.allocator zero-leak") {
    runTest5() catch |e| {
        std.debug.print("test5 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}
