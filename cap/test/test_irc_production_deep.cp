// Source: cp/cap/test/test_irc_production_deep.cp (Production deep tests for cap:irc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IRC_TCP_PORT,
    IRCS_TCP_PORT,
    IRC_MAX_LINE_LEN,
    IRCV3_MAX_TAGS_LEN,
    RPL_WELCOME,
    RPL_YOURHOST,
    RPL_CREATED,
    RPL_MYINFO,
    RPL_ISUPPORT,
    RPL_TOPIC,
    RPL_NAMREPLY,
    RPL_ENDOFNAMES,
    RPL_MOTD,
    RPL_ENDOFMOTD,
    ERR_NOSUCHNICK,
    ERR_NOSUCHCHANNEL,
    ERR_NICKNAMEINUSE,
    IrcTag,
    IrcTagIter,
    unescapeTagValue,
    escapeTagValue,
    IrcPrefix,
    parseIrcPrefix,
    IrcParamIter,
    IrcMessage,
    parseIrcLine,
    IrcWriter,
} from "cap:irc";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. Standard PRIVMSG, JOIN, PING parsing with CRLF/LF/empty trailing/IPv6 param", () => {
    expect_eq(IRC_TCP_PORT, 6667 as u16);
    expect_eq(IRCS_TCP_PORT, 6697 as u16);
    expect_eq(IRC_MAX_LINE_LEN, 512);
    expect_eq(IRCV3_MAX_TAGS_LEN, 8191);

    // PRIVMSG with prefix, middle param, and trailing containing spaces & colons
    const priv_msg = parseIrcLine(":alice!ident@irc.example.org PRIVMSG #general :hello: world!\r\n") ?? unreachable;
    expect_eq(priv_msg.hasTags(), false);
    expect_eq(priv_msg.hasPrefix(), true);
    expect_eq(priv_msg.isNumeric(), false);
    expect_eq(priv_msg.numericCode() == null, true);
    expectBytesEq(priv_msg.command, "PRIVMSG");
    expectBytesEq(priv_msg.trailing() ?? unreachable, "hello: world!");

    let pit = priv_msg.params();
    expectBytesEq(pit.next() ?? unreachable, "#general");
    expectBytesEq(pit.next() ?? unreachable, "hello: world!");
    expect_eq(pit.next() == null, true);

    // JOIN with LF-only ending and no trailing
    const join_msg = parseIrcLine(":bob!b@h JOIN #cp-lang\n") ?? unreachable;
    expectBytesEq(join_msg.command, "JOIN");
    expect_eq(join_msg.trailing() == null, true);
    let jit = join_msg.params();
    expectBytesEq(jit.next() ?? unreachable, "#cp-lang");
    expect_eq(jit.next() == null, true);

    // PING without prefix and with empty trailing
    const ping_msg = parseIrcLine("PING :token-12345") ?? unreachable;
    expect_eq(ping_msg.hasPrefix(), false);
    expect_eq(ping_msg.prefix() == null, true);
    expectBytesEq(ping_msg.command, "PING");
    expectBytesEq(ping_msg.trailing() ?? unreachable, "token-12345");

    // Empty trailing parameter (`:`) and middle param containing colon (IPv6)
    const empty_tr = parseIrcLine("NOTICE 2001:db8::1 :\r\n") ?? unreachable;
    expectBytesEq(empty_tr.command, "NOTICE");
    expectBytesEq(empty_tr.trailing() ?? unreachable, "");
    let eit = empty_tr.params();
    expectBytesEq(eit.next() ?? unreachable, "2001:db8::1");
    expectBytesEq(eit.next() ?? unreachable, "");
    expect_eq(eit.next() == null, true);
});

test("2. IRCv3 @tags multi-tag iteration, client-only tags, and escape/unescape rules", () => {
    const line = "@k1=v1;k2;+client/tag=hello\\sworld\\:a\\\\b\\rc\\nd;empty= :nick!u@h PRIVMSG #v3 :hi\r\n";
    const msg = parseIrcLine(line) ?? unreachable;
    expect_eq(msg.hasTags(), true);

    let tit = msg.tags();
    const t0 = tit.next() ?? unreachable;
    expectBytesEq(t0.key, "k1");
    expectBytesEq(t0.value, "v1");
    expect_eq(t0.has_value, true);
    expect_eq(t0.isClientOnly(), false);

    const t1 = tit.next() ?? unreachable;
    expectBytesEq(t1.key, "k2");
    expectBytesEq(t1.value, "");
    expect_eq(t1.has_value, false);
    expect_eq(t1.isClientOnly(), false);

    const t2 = tit.next() ?? unreachable;
    expectBytesEq(t2.key, "+client/tag");
    expect_eq(t2.has_value, true);
    expect_eq(t2.isClientOnly(), true);

    let out_buf: [64]u8 = [0; 64];
    const dec_len = unescapeTagValue(t2.value, out_buf[0..]) ?? unreachable;
    expectBytesEq(out_buf[0..dec_len], "hello world;a\\b\rc\nd");

    const t3 = tit.next() ?? unreachable;
    expectBytesEq(t3.key, "empty");
    expectBytesEq(t3.value, "");
    expect_eq(t3.has_value, true);
    expect_eq(tit.next() == null, true);

    // findTag lookup
    const found = msg.findTag("+client/tag") ?? unreachable;
    expectBytesEq(found.key, "+client/tag");
    expect_eq(msg.findTag("missing") == null, true);

    // IRCv3 spec edge cases: unknown escape drops backslash, lone trailing backslash discarded
    const edge_len = unescapeTagValue("foo\\abar\\", out_buf[0..]) ?? unreachable;
    expectBytesEq(out_buf[0..edge_len], "fooabar");

    // Buffer too small returns null
    let tiny: [3]u8 = [0; 3];
    expect_eq(unescapeTagValue("abcd", tiny[0..]) == null, true);
    expect_eq(escapeTagValue("a;b", tiny[0..]) == null, true);
});

test("3. 3-digit numeric replies (001, 353, 433) and protocol constants", () => {
    expect_eq(RPL_WELCOME, 1 as u16);
    expect_eq(RPL_YOURHOST, 2 as u16);
    expect_eq(RPL_CREATED, 3 as u16);
    expect_eq(RPL_MYINFO, 4 as u16);
    expect_eq(RPL_ISUPPORT, 5 as u16);
    expect_eq(RPL_TOPIC, 332 as u16);
    expect_eq(RPL_NAMREPLY, 353 as u16);
    expect_eq(RPL_ENDOFNAMES, 366 as u16);
    expect_eq(RPL_MOTD, 372 as u16);
    expect_eq(RPL_ENDOFMOTD, 376 as u16);
    expect_eq(ERR_NOSUCHNICK, 401 as u16);
    expect_eq(ERR_NOSUCHCHANNEL, 403 as u16);
    expect_eq(ERR_NICKNAMEINUSE, 433 as u16);

    const m001 = parseIrcLine(":irc.libera.chat 001 alice :Welcome to the IRC Network\r\n") ?? unreachable;
    expect_eq(m001.isNumeric(), true);
    expect_eq(m001.numericCode() ?? 0, RPL_WELCOME);
    expectBytesEq(m001.trailing() ?? unreachable, "Welcome to the IRC Network");

    const m353 = parseIrcLine(":irc.libera.chat 353 alice = #cp :@op +voice guest\r\n") ?? unreachable;
    expect_eq(m353.numericCode() ?? 0, RPL_NAMREPLY);
    let p353 = m353.params();
    expectBytesEq(p353.next() ?? unreachable, "alice");
    expectBytesEq(p353.next() ?? unreachable, "=");
    expectBytesEq(p353.next() ?? unreachable, "#cp");
    expectBytesEq(p353.next() ?? unreachable, "@op +voice guest");
    expect_eq(p353.next() == null, true);

    const m433 = parseIrcLine(":irc.libera.chat 433 * alice :Nickname is already in use.\r\n") ?? unreachable;
    expect_eq(m433.numericCode() ?? 0, ERR_NICKNAMEINUSE);
    expectBytesEq(m433.trailing() ?? unreachable, "Nickname is already in use.");
});

test("4. Prefix splitting for nick!user@host, nick@host, nick!user, and servername", () => {
    const full = parseIrcPrefix("alice!~ident@2001:db8::42");
    expectBytesEq(full.raw, "alice!~ident@2001:db8::42");
    expectBytesEq(full.nick, "alice");
    expectBytesEq(full.user, "~ident");
    expectBytesEq(full.host, "2001:db8::42");
    expect_eq(full.isUser(), true);
    expect_eq(full.isServer(), false);

    const no_user = parseIrcPrefix("bob@chat.example.net");
    expectBytesEq(no_user.nick, "bob");
    expectBytesEq(no_user.user, "");
    expectBytesEq(no_user.host, "chat.example.net");
    expect_eq(no_user.isUser(), true);

    const no_host = parseIrcPrefix("carol!local");
    expectBytesEq(no_host.nick, "carol");
    expectBytesEq(no_host.user, "local");
    expectBytesEq(no_host.host, "");
    expect_eq(no_host.isUser(), true);

    const srv = parseIrcPrefix("irc.libera.chat");
    expectBytesEq(srv.nick, "irc.libera.chat");
    expectBytesEq(srv.user, "");
    expectBytesEq(srv.host, "");
    expect_eq(srv.isServer(), true);
    expect_eq(srv.isUser(), false);

    // Via IrcMessage.prefix()
    const msg = parseIrcLine(":dan!d@localhost PRIVMSG #chan :yo\r\n") ?? unreachable;
    const pfx = msg.prefix() ?? unreachable;
    expectBytesEq(pfx.nick, "dan");
    expectBytesEq(pfx.user, "d");
    expectBytesEq(pfx.host, "localhost");
});

test("5. IrcWriter zero-alloc encoding round-trip with tags, prefix, numeric, params, trailing", () => {
    let wire_buf: [512]u8 = [0; 512];
    let w = IrcWriter.init(wire_buf[0..]);
    expect_eq(w.writeTag("msgid", "msg-99"), true);
    expect_eq(w.writeTag("flag", null), true);
    expect_eq(w.writeTag("+client/meta", "a b;c\\d\re\nf"), true);
    expect_eq(w.writePrefix("alice!u@irc.example.com"), true);
    expect_eq(w.writeCommand("PRIVMSG"), true);
    expect_eq(w.writeParam("#cp"), true);
    expect_eq(w.writeTrailing("Hello IRCv3!"), true);

    const encoded = w.finish() ?? unreachable;
    expect_eq(w.hasError(), false);
    expectBytesEq(
        encoded,
        "@msgid=msg-99;flag;+client/meta=a\\sb\\:c\\\\d\\re\\nf :alice!u@irc.example.com PRIVMSG #cp :Hello IRCv3!\r\n",
    );

    // Round-trip parse back
    const parsed = parseIrcLine(encoded) ?? unreachable;
    expect_eq(parsed.hasTags(), true);
    expectBytesEq(parsed.command, "PRIVMSG");
    expectBytesEq(parsed.trailing() ?? unreachable, "Hello IRCv3!");

    const pfx = parsed.prefix() ?? unreachable;
    expectBytesEq(pfx.nick, "alice");
    expectBytesEq(pfx.user, "u");
    expectBytesEq(pfx.host, "irc.example.com");

    const meta = parsed.findTag("+client/meta") ?? unreachable;
    let dec_buf: [64]u8 = [0; 64];
    const n = unescapeTagValue(meta.value, dec_buf[0..]) ?? unreachable;
    expectBytesEq(dec_buf[0..n], "a b;c\\d\re\nf");

    // Numeric reply writer round-trip
    let num_buf: [128]u8 = [0; 128];
    let nw = IrcWriter.init(num_buf[0..]);
    expect_eq(nw.writePrefix("irc.local"), true);
    expect_eq(nw.writeNumeric(RPL_WELCOME), true);
    expect_eq(nw.writeParam("alice"), true);
    expect_eq(nw.writeTrailing("Welcome!"), true);
    const num_wire = nw.finish() ?? unreachable;
    expectBytesEq(num_wire, ":irc.local 001 alice :Welcome!\r\n");

    const parsed_num = parseIrcLine(num_wire) ?? unreachable;
    expect_eq(parsed_num.numericCode() ?? 0, RPL_WELCOME);
});

test("6. Malformed line rejection and IrcWriter invariant/overflow protection", () => {
    // Empty and CRLF-only lines
    expect_eq(parseIrcLine("") == null, true);
    expect_eq(parseIrcLine("\r\n") == null, true);
    expect_eq(parseIrcLine("\n") == null, true);

    // Empty @ tag, tags without command, empty : prefix, prefix without command
    expect_eq(parseIrcLine("@ PRIVMSG #a :hi\r\n") == null, true);
    expect_eq(parseIrcLine("@k=v\r\n") == null, true);
    expect_eq(parseIrcLine("@k=v   \r\n") == null, true);
    expect_eq(parseIrcLine(": PRIVMSG #a :hi\r\n") == null, true);
    expect_eq(parseIrcLine(":irc.local\r\n") == null, true);
    expect_eq(parseIrcLine(":irc.local   \r\n") == null, true);

    // Invalid command tokens (2 digits, 4 digits, mixed alphanumeric)
    expect_eq(parseIrcLine("12 param\r\n") == null, true);
    expect_eq(parseIrcLine("1234 param\r\n") == null, true);
    expect_eq(parseIrcLine("PRIV1 #a :hi\r\n") == null, true);

    // IrcWriter ordering & validation guards
    let buf: [64]u8 = [0; 64];
    let w1 = IrcWriter.init(buf[0..]);
    // Cannot finish without command
    expect_eq(w1.finish() == null, true);
    expect_eq(w1.hasError(), true);

    let w2 = IrcWriter.init(buf[0..]);
    expect_eq(w2.writeCommand("PING"), true);
    // Cannot write tag or prefix after command
    expect_eq(w2.writeTag("a", "b"), false);
    expect_eq(w2.hasError(), true);

    let w3 = IrcWriter.init(buf[0..]);
    expect_eq(w3.writeCommand("PRIVMSG"), true);
    // Middle param cannot contain space or start with ':'
    expect_eq(w3.writeParam(":bad"), false);

    // Buffer overflow protection
    let small: [6]u8 = [0; 6];
    let w4 = IrcWriter.init(small[0..]);
    expect_eq(w4.writeCommand("PRIVMSG"), false);
    expect_eq(w4.finish() == null, true);
});
