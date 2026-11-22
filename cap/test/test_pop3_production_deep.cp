// Source: cp/cap/test/test_pop3_production_deep.cp (Production deep tests for cap:pop3)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    POP3_TCP_PORT,
    POP3S_TCP_PORT,
    POP3_MAX_CMD_LEN,
    POP3_MAX_LINE_LEN,
    isPop3ResponsePrefix,
    Pop3Response,
    parsePop3Response,
    Pop3Stat,
    parsePop3Stat,
    Pop3ListItem,
    parsePop3ListLine,
    Pop3UidlItem,
    parsePop3UidlLine,
    Pop3Command,
    parsePop3Command,
    Pop3MultiLineIter,
    encodePop3Command,
    pop3DotStuff,
} from "cap:net";

test("1. Constants, +OK/-ERR status lines, and RFC 2449 [RESP-CODE] parsing", () => {
    expect_eq(POP3_TCP_PORT, 110 as u16);
    expect_eq(POP3S_TCP_PORT, 995 as u16);
    expect_eq(POP3_MAX_CMD_LEN, 255 as usize);
    expect_eq(POP3_MAX_LINE_LEN, 512 as usize);

    expect_eq(isPop3ResponsePrefix("+OK ready\r\n"), true);
    expect_eq(isPop3ResponsePrefix("+OK"), true);
    expect_eq(isPop3ResponsePrefix("-ERR failed\r\n"), true);
    expect_eq(isPop3ResponsePrefix("-ERR"), true);
    expect_eq(isPop3ResponsePrefix("+OKAY"), false);
    expect_eq(isPop3ResponsePrefix("-ERROR"), false);
    expect_eq(isPop3ResponsePrefix("220 SMTP"), false);

    // Basic +OK with message
    const r1 = parsePop3Response("+OK POP3 server ready <1896.697170952@dbc.mtview.ca.us>\r\n") ?? unreachable;
    expect_eq(r1.isOk(), true);
    expect_eq(r1.isErr(), false);
    expect_eq(r1.hasRespCode(), false);
    expect_eq(r1.resp_code.len, 0 as usize);
    expect_eq(std.mem.eql(u8, r1.message, "POP3 server ready <1896.697170952@dbc.mtview.ca.us>"), true);

    // Bare +OK / -ERR without message
    const r2 = parsePop3Response("+OK\r\n") ?? unreachable;
    expect_eq(r2.isOk(), true);
    expect_eq(r2.message.len, 0 as usize);

    const r3 = parsePop3Response("-ERR") ?? unreachable;
    expect_eq(r3.isErr(), true);
    expect_eq(r3.message.len, 0 as usize);

    // RFC 2449 [IN-USE], [LOGIN-DELAY], [SYS/TEMP], and code-only response
    const r4 = parsePop3Response("-ERR [IN-USE] Do you have another POP session running?\r\n") ?? unreachable;
    expect_eq(r4.isErr(), true);
    expect_eq(r4.hasRespCode(), true);
    expect_eq(std.mem.eql(u8, r4.resp_code, "IN-USE"), true);
    expect_eq(std.mem.eql(u8, r4.message, "Do you have another POP session running?"), true);

    const r5 = parsePop3Response("-ERR [LOGIN-DELAY] Logins must be at least 15 minutes apart\r\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, r5.resp_code, "LOGIN-DELAY"), true);
    expect_eq(std.mem.eql(u8, r5.message, "Logins must be at least 15 minutes apart"), true);

    const r6 = parsePop3Response("-ERR [SYS/TEMP] Temporary system problem\r\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, r6.resp_code, "SYS/TEMP"), true);
    expect_eq(std.mem.eql(u8, r6.message, "Temporary system problem"), true);

    const r7 = parsePop3Response("-ERR [AUTH]\r\n") ?? unreachable;
    expect_eq(r7.isErr(), true);
    expect_eq(std.mem.eql(u8, r7.resp_code, "AUTH"), true);
    expect_eq(r7.message.len, 0 as usize);
});

test("2. STAT, LIST, and UIDL payload parsing", () => {
    // STAT from Pop3Response.message
    const stat_resp = parsePop3Response("+OK 2 320\r\n") ?? unreachable;
    const st = parsePop3Stat(stat_resp.message) ?? unreachable;
    expect_eq(st.message_count, 2 as u64);
    expect_eq(st.total_octets, 320 as u64);

    // STAT with zero messages and extra trailing info
    const st_zero = parsePop3Stat("0 0 (empty mailbox)\r\n") ?? unreachable;
    expect_eq(st_zero.message_count, 0 as u64);
    expect_eq(st_zero.total_octets, 0 as u64);

    // LIST scan entries
    const l1 = parsePop3ListLine("1 120") ?? unreachable;
    expect_eq(l1.msg_num, 1 as u64);
    expect_eq(l1.octets, 120 as u64);

    const l2 = parsePop3ListLine("42  9876543210\r\n") ?? unreachable;
    expect_eq(l2.msg_num, 42 as u64);
    expect_eq(l2.octets, 9876543210 as u64);

    // UIDL entries (including 70-char max RFC 1939 UID)
    const u1 = parsePop3UidlLine("1 whqtswO00WBw418f9t5JxYwZ") ?? unreachable;
    expect_eq(u1.msg_num, 1 as u64);
    expect_eq(std.mem.eql(u8, u1.uid, "whqtswO00WBw418f9t5JxYwZ"), true);

    const u2 = parsePop3UidlLine("2 QhdPYR:00WBw1Ph7x7\r\n") ?? unreachable;
    expect_eq(u2.msg_num, 2 as u64);
    expect_eq(std.mem.eql(u8, u2.uid, "QhdPYR:00WBw1Ph7x7"), true);

    const max_uid_line = "9 1234567890123456789012345678901234567890123456789012345678901234567890";
    const u_max = parsePop3UidlLine(max_uid_line) ?? unreachable;
    expect_eq(u_max.msg_num, 9 as u64);
    expect_eq(u_max.uid.len, 70 as usize);
});

test("3. Pop3MultiLineIter zero-copy iteration, byte-unstuffing, and pipelined remainder", () => {
    const body =
        "From: alice@cp.dev\r\n" ++
        "..leading single dot\r\n" ++
        "...leading double dot\r\n" ++
        "..\r\n" ++
        "normal.dot.inside\r\n" ++
        "\r\n" ++
        ".\r\n" ++
        "+OK next pipelined reply\r\n";

    let it = Pop3MultiLineIter.init(body);

    const line1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, line1, "From: alice@cp.dev"), true);

    const line2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, line2, ".leading single dot"), true);

    const line3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, line3, "..leading double dot"), true);

    const line4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, line4, "."), true);

    const line5 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, line5, "normal.dot.inside"), true);

    const line6 = it.next() ?? unreachable;
    expect_eq(line6.len, 0 as usize);

    // Terminated cleanly at standalone "."
    expect_eq(it.next() == null, true);
    expect_eq(it.isComplete(), true);
    expect_eq(it.hasError(), false);
    expect_eq(std.mem.eql(u8, it.remaining(), "+OK next pipelined reply\r\n"), true);

    // Subsequent calls remain null and complete
    expect_eq(it.next() == null, true);
    expect_eq(it.isComplete(), true);

    // Empty multi-line body (immediate ".\r\n")
    let empty_it = Pop3MultiLineIter.init(".\r\n");
    expect_eq(empty_it.next() == null, true);
    expect_eq(empty_it.isComplete(), true);
    expect_eq(empty_it.hasError(), false);
    expect_eq(empty_it.remaining().len, 0 as usize);
});

test("4. Command encoding/parsing and pop3DotStuff <-> Pop3MultiLineIter roundtrip", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    // 0-arg command: QUIT
    const n_quit = encodePop3Command("QUIT", "", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_quit], "QUIT\r\n"), true);
    const c_quit = parsePop3Command(buf[0..n_quit]) ?? unreachable;
    expect_eq(c_quit.verbEq("quit"), true);
    expect_eq(c_quit.arg1.len, 0 as usize);
    expect_eq(c_quit.arg2.len, 0 as usize);

    // 1-arg command: USER
    const n_user = encodePop3Command("USER", "mrose", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_user], "USER mrose\r\n"), true);
    const c_user = parsePop3Command(buf[0..n_user]) ?? unreachable;
    expect_eq(c_user.verbEq("USER"), true);
    expect_eq(std.mem.eql(u8, c_user.arg1, "mrose"), true);
    expect_eq(c_user.arg2.len, 0 as usize);

    // 2-arg command: TOP 1 10
    const n_top = encodePop3Command("TOP", "1", "10", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_top], "TOP 1 10\r\n"), true);
    const c_top = parsePop3Command(buf[0..n_top]) ?? unreachable;
    expect_eq(c_top.verbEq("top"), true);
    expect_eq(std.mem.eql(u8, c_top.arg1, "1"), true);
    expect_eq(std.mem.eql(u8, c_top.arg2, "10"), true);

    // PASS with spaces in password preserves full password in arg1
    const c_pass = parsePop3Command("PASS my secret pass word\r\n") ?? unreachable;
    expect_eq(c_pass.verbEq("pass"), true);
    expect_eq(std.mem.eql(u8, c_pass.arg1, "my secret pass word"), true);
    expect_eq(c_pass.arg2.len, 0 as usize);

    // Dot-stuffing roundtrip (with LF normalization & leading dots)
    const raw_body = ".first dot\r\n..two dots\nplain line\n.";
    let wire_buf: [256]u8 = std.mem.zeroes([256]u8);
    const wn = pop3DotStuff(raw_body, wire_buf[0..]) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, wire_buf[0..wn], "..first dot\r\n...two dots\r\nplain line\r\n..\r\n.\r\n"),
        true,
    );

    let it = Pop3MultiLineIter.init(wire_buf[0..wn]);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, ".first dot"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "..two dots"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "plain line"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "."), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.isComplete(), true);

    // Empty body dot-stuffing -> ".\r\n"
    const en = pop3DotStuff("", wire_buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, wire_buf[0..en], ".\r\n"), true);
});

test("5. Malformed response prefixes, RFC 2449 codes, and non-numeric STAT/LIST/UIDL rejection", () => {
    // Invalid status prefixes
    expect_eq(parsePop3Response("") == null, true);
    expect_eq(parsePop3Response("+ok lowercase\r\n") == null, true);
    expect_eq(parsePop3Response("-err lowercase\r\n") == null, true);
    expect_eq(parsePop3Response("+OKAY invalid\r\n") == null, true);
    expect_eq(parsePop3Response("-ERROR invalid\r\n") == null, true);
    expect_eq(parsePop3Response("OK missing plus\r\n") == null, true);
    expect_eq(parsePop3Response("+OK embedded\r\nline\r\n") == null, true);

    // Malformed RFC 2449 response codes
    expect_eq(parsePop3Response("-ERR [] empty code\r\n") == null, true);
    expect_eq(parsePop3Response("-ERR [UNCLOSED msg\r\n") == null, true);
    expect_eq(parsePop3Response("-ERR [IN USE] space in code\r\n") == null, true);
    expect_eq(parsePop3Response("-ERR [AUTH]missing-space\r\n") == null, true);
    expect_eq(parsePop3Response("-ERR [BAD[NEST]] msg\r\n") == null, true);

    // Non-numeric / truncated / overflow STAT
    expect_eq(parsePop3Stat("") == null, true);
    expect_eq(parsePop3Stat("abc 120") == null, true);
    expect_eq(parsePop3Stat("2 xyz") == null, true);
    expect_eq(parsePop3Stat("2") == null, true);
    expect_eq(parsePop3Stat("-1 100") == null, true);
    expect_eq(parsePop3Stat("2 320abc") == null, true);
    expect_eq(parsePop3Stat("9999999999999999999999999 10") == null, true);

    // Invalid LIST / UIDL (msg_num == 0, missing arg, >70 char UID, space in UID)
    expect_eq(parsePop3ListLine("0 100") == null, true);
    expect_eq(parsePop3ListLine("1 abc") == null, true);
    expect_eq(parsePop3UidlLine("0 abcdef") == null, true);
    expect_eq(parsePop3UidlLine("1 ") == null, true);
    expect_eq(parsePop3UidlLine("1 bad uid with space") == null, true);
    expect_eq(
        parsePop3UidlLine("1 12345678901234567890123456789012345678901234567890123456789012345678901") == null,
        true,
    );
});

test("6. Unterminated multi-line detection and encoder injection/overflow guards", () => {
    // Missing terminating ".\r\n"
    let it1 = Pop3MultiLineIter.init("1 120\r\n2 200\r\n");
    expect_eq(std.mem.eql(u8, it1.next() ?? unreachable, "1 120"), true);
    expect_eq(std.mem.eql(u8, it1.next() ?? unreachable, "2 200"), true);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);
    expect_eq(it1.isComplete(), false);

    // Truncated line without CRLF
    let it2 = Pop3MultiLineIter.init("1 120\r\n2 200");
    expect_eq(std.mem.eql(u8, it2.next() ?? unreachable, "1 120"), true);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);
    expect_eq(it2.isComplete(), false);

    // Empty buffer is unterminated
    let it3 = Pop3MultiLineIter.init("");
    expect_eq(it3.next() == null, true);
    expect_eq(it3.hasError(), true);
    expect_eq(it3.isComplete(), false);

    // Command encoder guards: CRLF injection, invalid verb, arg2 without arg1, tiny buffer
    let small: [4]u8 = std.mem.zeroes([4]u8);
    let big: [256]u8 = std.mem.zeroes([256]u8);
    expect_eq(encodePop3Command("QUIT", "", "", small[0..]) == null, true);
    expect_eq(encodePop3Command("", "", "", big[0..]) == null, true);
    expect_eq(encodePop3Command("USER", "alice\r\nDELE 1", "", big[0..]) == null, true);
    expect_eq(encodePop3Command("TOP", "", "10", big[0..]) == null, true);
    expect_eq(encodePop3Command("BAD VERB", "", "", big[0..]) == null, true);

    // Command parser rejects empty / trailing space / bad verb
    expect_eq(parsePop3Command("") == null, true);
    expect_eq(parsePop3Command("USER \r\n") == null, true);
    expect_eq(parsePop3Command("TOP 1 \r\n") == null, true);

    // Dot-stuffing buffer too small
    let tiny: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(pop3DotStuff("", tiny[0..]) == null, true);
    expect_eq(pop3DotStuff(".hello", small[0..]) == null, true);
});
