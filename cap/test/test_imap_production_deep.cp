// Source: cp/cap/test/test_imap_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:imap & cap.imap

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ImapError,
    ImapStatus,
    ImapResponseKind,
    ImapStreamParser,
    ImapCommandCodec,
    parseCapability,
    parseSearchUids,
    parseExistsCount,
    parseFetchFlags,
} from "cap:net";

fn freeStringSlice(alloc: std.mem.Allocator, items: [][]const u8) {
    let i: usize = 0;
    while (i < items.len) : (i += 1) {
        alloc.free(items[i]);
    }
    alloc.free(items);
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    let codec = ImapCommandCodec.init();

    // 1. LOGIN with quotes and backslashes escaped -> A0001
    const c1 = codec.login(alloc, "alice@cp.dev", "pa\"ss\\word")?;
    defer alloc.free(c1);
    expect_eq(std.mem.eql(u8, c1, "A0001 LOGIN \"alice@cp.dev\" \"pa\\\"ss\\\\word\"\r\n"), true);

    // 2. SELECT -> A0002
    const c2 = codec.select(alloc, "INBOX/Archive \"2026\"")?;
    defer alloc.free(c2);
    expect_eq(std.mem.eql(u8, c2, "A0002 SELECT \"INBOX/Archive \\\"2026\\\"\"\r\n"), true);

    // 3. EXAMINE -> A0003
    const c3 = codec.examine(alloc, "Sent")?;
    defer alloc.free(c3);
    expect_eq(std.mem.eql(u8, c3, "A0003 EXAMINE \"Sent\"\r\n"), true);

    // 4. SEARCH -> A0004
    const c4 = codec.search(alloc, "UNSEEN SINCE 1-Jan-2026")?;
    defer alloc.free(c4);
    expect_eq(std.mem.eql(u8, c4, "A0004 SEARCH UNSEEN SINCE 1-Jan-2026\r\n"), true);

    // 5. FETCH -> A0005
    const c5 = codec.fetch(alloc, "1:10", "(FLAGS BODY[HEADER])")?;
    defer alloc.free(c5);
    expect_eq(std.mem.eql(u8, c5, "A0005 FETCH 1:10 (FLAGS BODY[HEADER])\r\n"), true);

    // 6. STORE -> A0006
    const c6 = codec.store(alloc, "42", "+FLAGS", "\\Seen \\Flagged")?;
    defer alloc.free(c6);
    expect_eq(std.mem.eql(u8, c6, "A0006 STORE 42 +FLAGS (\\Seen \\Flagged)\r\n"), true);

    // 7. EXPUNGE, NOOP, LOGOUT -> A0007, A0008, A0009
    const c7 = codec.expunge(alloc)?;
    defer alloc.free(c7);
    expect_eq(std.mem.eql(u8, c7, "A0007 EXPUNGE\r\n"), true);

    const c8 = codec.noop(alloc)?;
    defer alloc.free(c8);
    expect_eq(std.mem.eql(u8, c8, "A0008 NOOP\r\n"), true);

    const c9 = codec.logout(alloc)?;
    defer alloc.free(c9);
    expect_eq(std.mem.eql(u8, c9, "A0009 LOGOUT\r\n"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    let parser = ImapStreamParser.init(alloc);
    defer parser.deinit();

    const wire =
        "* OK [CAPABILITY IMAP4rev1 STARTTLS AUTH=PLAIN] Dovecot ready\r\n" ++
        "* 42 EXISTS\r\n" ++
        "* SEARCH 1 5 42 108\r\n" ++
        "+ Ready for additional command text\r\n" ++
        "A0001 OK [READ-WRITE] SELECT completed\r\n";

    // Feed in awkward small chunks splitting across lines
    parser.feed(wire[0..17])?;
    expect_eq((parser.nextResponse()?) == null, true);

    parser.feed(wire[17..65])?;
    let r1 = (parser.nextResponse()?).?;
    defer r1.deinit(alloc);
    switch (r1) {
        .untagged_status u => {
            expect_eq(u.status == ImapStatus.Ok, true);
            expect_eq(std.mem.eql(u8, u.code.?, "CAPABILITY IMAP4rev1 STARTTLS AUTH=PLAIN"), true);
            expect_eq(std.mem.eql(u8, u.text, "Dovecot ready"), true);

            const caps = parseCapability(alloc, u.code.?)?;
            defer freeStringSlice(alloc, caps);
            expect_eq(caps.len, 3 as usize);
            expect_eq(std.mem.eql(u8, caps[0], "IMAP4rev1"), true);
            expect_eq(std.mem.eql(u8, caps[1], "STARTTLS"), true);
            expect_eq(std.mem.eql(u8, caps[2], "AUTH=PLAIN"), true);
        },
        else => expect_eq(true, false),
    }

    parser.feed(wire[65..])?;

    // * 42 EXISTS
    let r2 = (parser.nextResponse()?).?;
    defer r2.deinit(alloc);
    switch (r2) {
        .untagged_data d => {
            expect_eq(std.mem.eql(u8, d.keyword, "EXISTS"), true);
            expect_eq(std.mem.eql(u8, d.rest, "42"), true);
            expect_eq(parseExistsCount("* 42 EXISTS").?, 42 as u32);
            expect_eq(parseExistsCount("* 2 RECENT").?, 2 as u32);
        },
        else => expect_eq(true, false),
    }

    // * SEARCH 1 5 42 108
    let r3 = (parser.nextResponse()?).?;
    defer r3.deinit(alloc);
    switch (r3) {
        .untagged_data d => {
            expect_eq(std.mem.eql(u8, d.keyword, "SEARCH"), true);
            const uids = parseSearchUids(alloc, d.rest)?;
            defer alloc.free(uids);
            expect_eq(uids.len, 4 as usize);
            expect_eq(uids[0], 1 as u32);
            expect_eq(uids[1], 5 as u32);
            expect_eq(uids[2], 42 as u32);
            expect_eq(uids[3], 108 as u32);
        },
        else => expect_eq(true, false),
    }

    // + Ready for additional command text
    let r4 = (parser.nextResponse()?).?;
    defer r4.deinit(alloc);
    switch (r4) {
        .continuation c => {
            expect_eq(std.mem.eql(u8, c, "Ready for additional command text"), true);
        },
        else => expect_eq(true, false),
    }

    // A0001 OK [READ-WRITE] SELECT completed
    let r5 = (parser.nextResponse()?).?;
    defer r5.deinit(alloc);
    switch (r5) {
        .tagged t => {
            expect_eq(std.mem.eql(u8, t.tag, "A0001"), true);
            expect_eq(t.status == ImapStatus.Ok, true);
            expect_eq(std.mem.eql(u8, t.code.?, "READ-WRITE"), true);
            expect_eq(std.mem.eql(u8, t.text, "SELECT completed"), true);
        },
        else => expect_eq(true, false),
    }

    expect_eq((parser.nextResponse()?) == null, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    let parser = ImapStreamParser.init(alloc);
    defer parser.deinit();

    // Multi-literal FETCH response with embedded \r\n inside binary payloads and non-sync {N+}
    const lit1 = "Subject: Hi\r\nFrom: a@b.com\r\n\r\n"; // 30 bytes
    const lit2 = "Body\r\nLine2"; // 11 bytes
    const wire =
        "* 12 FETCH (UID 100 FLAGS (\\Seen \\Flagged $Important) BODY[HEADER] {30}\r\n" ++
        "Subject: Hi\r\nFrom: a@b.com\r\n\r\n" ++
        " BODY[TEXT] {11+}\r\n" ++
        "Body\r\nLine2" ++
        ")\r\n";

    // Feed byte-by-byte / small chunks across literal boundaries
    let pos: usize = 0;
    while (pos < wire.len) {
        let end = pos + 7;
        if (end > wire.len) end = wire.len;
        parser.feed(wire[pos..end])?;
        if (end < wire.len) {
            expect_eq((parser.nextResponse()?) == null, true);
        }
        pos = end;
    }

    let resp = (parser.nextResponse()?).?;
    defer resp.deinit(alloc);
    switch (resp) {
        .untagged_data d => {
            expect_eq(std.mem.eql(u8, d.keyword, "FETCH"), true);
            expect_eq(d.literals.len, 2 as usize);
            expect_eq(std.mem.eql(u8, d.literals[0], lit1), true);
            expect_eq(std.mem.eql(u8, d.literals[1], lit2), true);

            const flags = parseFetchFlags(alloc, d.rest)?;
            defer freeStringSlice(alloc, flags);
            expect_eq(flags.len, 3 as usize);
            expect_eq(std.mem.eql(u8, flags[0], "\\Seen"), true);
            expect_eq(std.mem.eql(u8, flags[1], "\\Flagged"), true);
            expect_eq(std.mem.eql(u8, flags[2], "$Important"), true);
        },
        else => expect_eq(true, false),
    }
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    let codec = ImapCommandCodec.init();

    // 1. CommandInjection guards (CR, LF, NUL)
    let inj_login = false;
    * = codec.login(alloc, "alice\r\nA999 LOGOUT", "secret") catch |e| {
        if (e == ImapError.CommandInjection) inj_login = true;
    };
    expect_eq(inj_login, true);

    let inj_select = false;
    * = codec.select(alloc, "INBOX\nNOOP") catch |e| {
        if (e == ImapError.CommandInjection) inj_select = true;
    };
    expect_eq(inj_select, true);

    let inj_fetch = false;
    * = codec.fetch(alloc, "1", "FLAGS\x00") catch |e| {
        if (e == ImapError.CommandInjection) inj_fetch = true;
    };
    expect_eq(inj_fetch, true);

    // 2. TruncatedLiteral guard
    let p_trunc = ImapStreamParser.init(alloc);
    defer p_trunc.deinit();
    p_trunc.feed("* 1 FETCH (BODY[1] {20}\r\nonly10byte")?;
    p_trunc.markEof();
    let got_trunc = false;
    * = p_trunc.nextResponse() catch |e| {
        if (e == ImapError.TruncatedLiteral) got_trunc = true;
    };
    expect_eq(got_trunc, true);

    // 3. InvalidResponse & InvalidTag guards
    let p_bad = ImapStreamParser.init(alloc);
    defer p_bad.deinit();
    p_bad.feed("A0001 UNKNOWN_STATUS oops\r\n")?;
    let got_invalid_resp = false;
    * = p_bad.nextResponse() catch |e| {
        if (e == ImapError.InvalidResponse) got_invalid_resp = true;
    };
    expect_eq(got_invalid_resp, true);

    p_bad.feed("BAD(TAG) OK hi\r\n")?;
    let got_invalid_tag = false;
    * = p_bad.nextResponse() catch |e| {
        if (e == ImapError.InvalidTag) got_invalid_tag = true;
    };
    expect_eq(got_invalid_tag, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.imap
    let codec = cap.net.ImapCommandCodec.init();
    const cmd = codec.login(alloc, "bot@cp.dev", "tok")?;
    defer alloc.free(cmd);
    expect_eq(std.mem.eql(u8, cmd, "A0001 LOGIN \"bot@cp.dev\" \"tok\"\r\n"), true);

    let parser = cap.net.ImapStreamParser.init(alloc);
    defer parser.deinit();
    parser.feed("* CAPABILITY IMAP4rev1 IDLE\r\nA0001 OK logged in\r\n")?;

    let r1 = (parser.nextResponse()?).?;
    defer r1.deinit(alloc);
    switch (r1) {
        .untagged_data d => {
            const caps = cap.net.parseCapability(alloc, d.rest)?;
            defer freeStringSlice(alloc, caps);
            expect_eq(caps.len, 2 as usize);
            expect_eq(std.mem.eql(u8, caps[0], "IMAP4rev1"), true);
            expect_eq(std.mem.eql(u8, caps[1], "IDLE"), true);
        },
        else => expect_eq(true, false),
    }

    let r2 = (parser.nextResponse()?).?;
    defer r2.deinit(alloc);
    switch (r2) {
        .tagged t => {
            expect_eq(t.status == cap.net.ImapStatus.Ok, true);
            expect_eq(std.mem.eql(u8, t.text, "logged in"), true);
        },
        else => expect_eq(true, false),
    }
}

test("cap:imap 1 - ImapCommandCodec monotonic tags, quoting, and commands") {
    runTest1() catch |e| {
        std.debug.print("test1 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:imap 2 - ImapStreamParser chunked untagged status, EXISTS, SEARCH, continuation, tagged") {
    runTest2() catch |e| {
        std.debug.print("test2 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:imap 3 - multi-literal {N}/{N+} with embedded CRLF across chunks and parseFetchFlags") {
    runTest3() catch |e| {
        std.debug.print("test3 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:imap 4 - CommandInjection, TruncatedLiteral, InvalidResponse, InvalidTag guards") {
    runTest4() catch |e| {
        std.debug.print("test4 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}

test("cap:imap 5 - cap.imap dynamic mount and std.testing.allocator zero-leak") {
    runTest5() catch |e| {
        std.debug.print("test5 error: {any}\n", .{e});
        expect_eq(true, false);
    };
}
