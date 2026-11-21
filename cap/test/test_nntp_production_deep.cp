// Source: cp/cap/test/test_nntp_production_deep.cp (Production deep tests for cap:nntp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NNTP_TCP_PORT,
    NNTPS_TCP_PORT,
    NNTP_MAX_CMD_LEN,
    CODE_READY_POST,
    CODE_READY_NOPOST,
    CODE_CLOSING,
    CODE_GROUP_OK,
    CODE_LIST_FOLLOWS,
    CODE_ARTICLE_FOLLOWS,
    CODE_HEAD_FOLLOWS,
    CODE_BODY_FOLLOWS,
    CODE_STAT_OK,
    CODE_OVER_FOLLOWS,
    CODE_SEND_ARTICLE,
    CODE_NO_SUCH_GROUP,
    CODE_NO_SUCH_ARTICLE,
    CODE_SYNTAX_ERROR,
    NntpResponse,
    parseNntpResponse,
    NntpGroupInfo,
    parseNntpGroup,
    NntpArticleStat,
    parseNntpArticleStat,
    NntpOverview,
    parseNntpOverview,
    NntpMultiLineIter,
    encodeNntpCommand,
} from "cap:nntp";

test("1. NNTP constants & 3-digit status response classification (1xx..5xx)", () => {
    expect_eq(NNTP_TCP_PORT, 119 as u16);
    expect_eq(NNTPS_TCP_PORT, 563 as u16);
    expect_eq(NNTP_MAX_CMD_LEN, 512 as usize);
    expect_eq(CODE_READY_POST, 200 as u16);
    expect_eq(CODE_READY_NOPOST, 201 as u16);
    expect_eq(CODE_CLOSING, 205 as u16);
    expect_eq(CODE_GROUP_OK, 211 as u16);
    expect_eq(CODE_LIST_FOLLOWS, 215 as u16);
    expect_eq(CODE_ARTICLE_FOLLOWS, 220 as u16);
    expect_eq(CODE_HEAD_FOLLOWS, 221 as u16);
    expect_eq(CODE_BODY_FOLLOWS, 222 as u16);
    expect_eq(CODE_STAT_OK, 223 as u16);
    expect_eq(CODE_OVER_FOLLOWS, 224 as u16);
    expect_eq(CODE_SEND_ARTICLE, 340 as u16);
    expect_eq(CODE_NO_SUCH_GROUP, 411 as u16);
    expect_eq(CODE_NO_SUCH_ARTICLE, 430 as u16);
    expect_eq(CODE_SYNTAX_ERROR, 500 as u16);

    const r1 = parseNntpResponse("100 Help text follows\r\n") ?? unreachable;
    expect_eq(r1.code, 100 as u16);
    expect_eq(r1.isInfo(), true);
    expect_eq(r1.isOk(), false);
    expect_eq(std.mem.eql(u8, r1.text, "Help text follows"), true);

    const r2 = parseNntpResponse("200 news.example.org ready (posting allowed)\r\n") ?? unreachable;
    expect_eq(r2.code, CODE_READY_POST);
    expect_eq(r2.isOk(), true);
    expect_eq(r2.isPermError(), false);
    expect_eq(std.mem.eql(u8, r2.text, "news.example.org ready (posting allowed)"), true);

    const r3 = parseNntpResponse("340 Send article to be posted\r\n") ?? unreachable;
    expect_eq(r3.code, CODE_SEND_ARTICLE);
    expect_eq(r3.isContinue(), true);

    const r4 = parseNntpResponse("411 No such newsgroup\r\n") ?? unreachable;
    expect_eq(r4.code, CODE_NO_SUCH_GROUP);
    expect_eq(r4.isTempError(), true);

    const r5 = parseNntpResponse("500 Unknown command\r\n") ?? unreachable;
    expect_eq(r5.code, CODE_SYNTAX_ERROR);
    expect_eq(r5.isPermError(), true);

    // Code-only status line without trailing text
    const r_bare = parseNntpResponse("205\r\n") ?? unreachable;
    expect_eq(r_bare.code, CODE_CLOSING);
    expect_eq(r_bare.text.len, 0 as usize);
});

test("2. GROUP 211 response payload parsing", () => {
    const resp = parseNntpResponse("211 1234 3000234 3002322 misc.test\r\n") ?? unreachable;
    expect_eq(resp.code, CODE_GROUP_OK);
    expect_eq(resp.isOk(), true);

    const grp = parseNntpGroup(resp.text) ?? unreachable;
    expect_eq(grp.count, 1234 as u64);
    expect_eq(grp.low, 3000234 as u64);
    expect_eq(grp.high, 3002322 as u64);
    expect_eq(std.mem.eql(u8, grp.name, "misc.test"), true);

    // Empty group with optional trailing server comment
    const empty_grp = parseNntpGroup("0 0 0 comp.lang.cp group selected\r\n") ?? unreachable;
    expect_eq(empty_grp.count, 0 as u64);
    expect_eq(empty_grp.low, 0 as u64);
    expect_eq(empty_grp.high, 0 as u64);
    expect_eq(std.mem.eql(u8, empty_grp.name, "comp.lang.cp"), true);
});

test("3. ARTICLE/HEAD/BODY/STAT (220..223) article number & <message-id> parsing", () => {
    const r220 = parseNntpResponse("220 3000234 <45223423@example.com> article retrieved\r\n") ?? unreachable;
    expect_eq(r220.code, CODE_ARTICLE_FOLLOWS);
    const s220 = parseNntpArticleStat(r220.text) ?? unreachable;
    expect_eq(s220.number, 3000234 as u64);
    expect_eq(std.mem.eql(u8, s220.message_id, "<45223423@example.com>"), true);

    // Retrieval by message-id where server reports article number 0
    const r223 = parseNntpResponse("223 0 <abc.123@news.cp.dev>\r\n") ?? unreachable;
    expect_eq(r223.code, CODE_STAT_OK);
    const s223 = parseNntpArticleStat(r223.text) ?? unreachable;
    expect_eq(s223.number, 0 as u64);
    expect_eq(std.mem.eql(u8, s223.message_id, "<abc.123@news.cp.dev>"), true);
});

test("4. RFC 3977 section 8.3 TAB-delimited 8-field OVER/XOVER overview parsing", () => {
    const line1 = "3000234\tRe: CP Zero-Alloc NNTP\tAlice <alice@example.com>\tSun, 22 Nov 2026 07:00:00 +0000\t<msg2@example.com>\t<msg1@example.com>\t4096\t58\r\n";
    const ov1 = parseNntpOverview(line1) ?? unreachable;
    expect_eq(ov1.article_num, 3000234 as u64);
    expect_eq(std.mem.eql(u8, ov1.subject, "Re: CP Zero-Alloc NNTP"), true);
    expect_eq(std.mem.eql(u8, ov1.from, "Alice <alice@example.com>"), true);
    expect_eq(std.mem.eql(u8, ov1.date, "Sun, 22 Nov 2026 07:00:00 +0000"), true);
    expect_eq(std.mem.eql(u8, ov1.message_id, "<msg2@example.com>"), true);
    expect_eq(std.mem.eql(u8, ov1.references, "<msg1@example.com>"), true);
    expect_eq(ov1.bytes, 4096 as u64);
    expect_eq(ov1.lines, 58 as u64);

    // Root article with empty references field and optional 9th Xref extension field
    const line2 = "101\tWelcome\tBob <bob@example.com>\tMon, 23 Nov 2026 08:00:00 +0000\t<root@example.com>\t\t512\t12\tXref: news.example.com misc.test:101\r\n";
    const ov2 = parseNntpOverview(line2) ?? unreachable;
    expect_eq(ov2.article_num, 101 as u64);
    expect_eq(ov2.references.len, 0 as usize);
    expect_eq(ov2.bytes, 512 as u64);
    expect_eq(ov2.lines, 12 as u64);
});

test("5. NntpMultiLineIter zero-copy iteration & RFC 3977 dot-unstuffing", () => {
    const payload = "From: alice@example.com\r\n\r\nHello world\r\n..Leading dot line\r\n...\r\n.\r\nNEXT_RESP";
    let it = NntpMultiLineIter.init(payload);

    const l0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l0, "From: alice@example.com"), true);

    const l1 = it.next() ?? unreachable;
    expect_eq(l1.len, 0 as usize);

    const l2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l2, "Hello world"), true);

    const l3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l3, ".Leading dot line"), true);

    const l4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l4, ".."), true);

    expect_eq(it.isTerminated(), false);
    expect_eq(it.next() == null, true);
    expect_eq(it.isTerminated(), true);
    expect_eq(std.mem.eql(u8, payload[it.bytesConsumed()..], "NEXT_RESP"), true);

    // Calling next() after termination stays null
    expect_eq(it.next() == null, true);

    // Truncated stream without terminating ".\r\n"
    let trunc = NntpMultiLineIter.init("Line1\r\nPartial");
    const tl0 = trunc.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, tl0, "Line1"), true);
    expect_eq(trunc.next() == null, true);
    expect_eq(trunc.isTerminated(), false);
});

test("6. Zero-alloc encodeNntpCommand & malformed/injection/boundary rejection", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const n1 = encodeNntpCommand("GROUP", "comp.lang.cp", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "GROUP comp.lang.cp\r\n"), true);

    const n2 = encodeNntpCommand("QUIT", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "QUIT\r\n"), true);

    // CRLF / NUL injection & invalid verb & buffer overflow rejection
    expect_eq(encodeNntpCommand("", "arg", buf[0..]) == null, true);
    expect_eq(encodeNntpCommand("GROUP\r\nQUIT", "x", buf[0..]) == null, true);
    expect_eq(encodeNntpCommand("GROUP", "misc.test\r\nQUIT", buf[0..]) == null, true);
    expect_eq(encodeNntpCommand("GROUP", "misc.test\n", buf[0..]) == null, true);
    expect_eq(encodeNntpCommand("GROUP X", "y", buf[0..]) == null, true);
    expect_eq(encodeNntpCommand("ARTICLE", "<very-long-id@example.com>", buf[0..8]) == null, true);

    // Exceeding RFC 3977 NNTP_MAX_CMD_LEN (512 bytes) must be rejected
    let big_arg: [510]u8 = [0; 510];
    let i: usize = 0;
    while (i < big_arg.len) {
        big_arg[i] = 'a';
        i += 1;
    }
    let big_out: [600]u8 = std.mem.zeroes([600]u8);
    expect_eq(encodeNntpCommand("GROUP", big_arg[0..], big_out[0..]) == null, true);

    // Malformed status responses
    expect_eq(parseNntpResponse("") == null, true);
    expect_eq(parseNntpResponse("20") == null, true);
    expect_eq(parseNntpResponse("099 Invalid low code") == null, true);
    expect_eq(parseNntpResponse("600 Invalid high code") == null, true);
    expect_eq(parseNntpResponse("20A Non-digit") == null, true);
    expect_eq(parseNntpResponse("200NoSpace") == null, true);
    expect_eq(parseNntpResponse("200-HyphenNotAllowedInNntp") == null, true);

    // Malformed GROUP / ARTICLE STAT / OVER payloads
    expect_eq(parseNntpGroup("123 1 10") == null, true);
    expect_eq(parseNntpGroup("abc 1 10 misc.test") == null, true);
    expect_eq(parseNntpArticleStat("12345") == null, true);
    expect_eq(parseNntpArticleStat("12345 missing-brackets@example.com") == null, true);
    expect_eq(parseNntpArticleStat("12345 <>") == null, true);
    expect_eq(parseNntpOverview("1\tsubj\tfrom\tdate\t<id@x>\t\t100") == null, true);
    expect_eq(parseNntpOverview("1\tsubj\tfrom\tdate\tbad-id\t\t100\t5") == null, true);
    expect_eq(parseNntpOverview("1\tsubj\tfrom\tdate\t<id@x>\t\tnotnum\t5") == null, true);
});
