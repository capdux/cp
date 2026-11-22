// Source: cp/cap/test/test_ftp_production_deep.cp (Production deep tests for cap:ftp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FTP_CONTROL_PORT,
    FTP_DATA_PORT,
    FTPS_IMPLICIT_PORT,
    RPL_DATA_OPEN,
    RPL_FILE_OK,
    RPL_OK,
    RPL_READY,
    RPL_CLOSING,
    RPL_TRANSFER_OK,
    RPL_PASV,
    RPL_EPSV,
    RPL_LOGGED_IN,
    RPL_PATHNAME,
    RPL_NEED_PASSWORD,
    RPL_NOT_LOGGED_IN,
    RPL_FILE_UNAVAILABLE,
    FtpCommand,
    parseFtpCommand,
    FtpReplyLine,
    parseFtpReplyLine,
    FtpPasvAddr,
    parsePasvReply,
    parseEpsvPort,
    FtpMlsxFact,
    FtpMlsxFactIter,
    FtpMlsxEntry,
    parseMlsxLine,
    encodeFtpCommand,
    encodePortCommand,
    encodeEprtCommand,
} from "cap:net";

test("1. FTP command & single/multi-line reply parsing and reply class helpers", () => {
    expect_eq(FTP_CONTROL_PORT, 21 as u16);
    expect_eq(FTP_DATA_PORT, 20 as u16);
    expect_eq(FTPS_IMPLICIT_PORT, 990 as u16);

    const c1 = parseFtpCommand("USER anonymous\r\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, c1.verb, "USER"), true);
    expect_eq(std.mem.eql(u8, c1.arg, "anonymous"), true);

    const c2 = parseFtpCommand("QUIT\r\n") ?? unreachable;
    expect_eq(std.mem.eql(u8, c2.verb, "QUIT"), true);
    expect_eq(c2.arg.len, 0);

    // Multi-line continuation reply
    const r1 = parseFtpReplyLine("220-Welcome to CP FTP\r\n") ?? unreachable;
    expect_eq(r1.code, RPL_READY);
    expect_eq(r1.is_continuation, true);
    expect_eq(r1.isPositive(), true);
    expect_eq(std.mem.eql(u8, r1.message, "Welcome to CP FTP"), true);

    // Final reply line
    const r2 = parseFtpReplyLine("220 Ready\r\n") ?? unreachable;
    expect_eq(r2.is_continuation, false);

    // Status classes 1xx / 3xx / 4xx / 5xx
    expect_eq((parseFtpReplyLine("150 Opening") ?? unreachable).isPreliminary(), true);
    expect_eq((parseFtpReplyLine("331 Password required") ?? unreachable).isIntermediate(), true);
    expect_eq((parseFtpReplyLine("421 Service not available") ?? unreachable).isTransientError(), true);
    expect_eq((parseFtpReplyLine("550 File unavailable") ?? unreachable).isPermanentError(), true);
    expect_eq(RPL_FILE_UNAVAILABLE, 550 as u16);
    expect_eq(RPL_NEED_PASSWORD, 331 as u16);
    expect_eq(RPL_NOT_LOGGED_IN, 530 as u16);
});

test("2. PASV (h1,h2,h3,h4,p1,p2) and EPSV (|||port|) / custom delimiter parsing", () => {
    // 19 * 256 + 136 = 5000
    const pasv = parsePasvReply("Entering Passive Mode (192,168,1,10,19,136).") ?? unreachable;
    expect_eq(pasv.ip[0], 192 as u8);
    expect_eq(pasv.ip[1], 168 as u8);
    expect_eq(pasv.ip[2], 1 as u8);
    expect_eq(pasv.ip[3], 10 as u8);
    expect_eq(pasv.port, 5000 as u16);

    // RFC 2428 EPSV standard delimiter '|'
    const epsv_port = parseEpsvPort("Entering Extended Passive Mode (|||6446|)") ?? unreachable;
    expect_eq(epsv_port, 6446 as u16);

    // RFC 2428 EPSV alternate printable delimiter '#'
    const epsv_alt = parseEpsvPort("Extended Passive (###49152#)") ?? unreachable;
    expect_eq(epsv_alt, 49152 as u16);
});

test("3. Zero-alloc encodeFtpCommand, encodePortCommand, and encodeEprtCommand", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const n1 = encodeFtpCommand("RETR", "archive.tar.gz", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "RETR archive.tar.gz\r\n"), true);

    const n2 = encodeFtpCommand("PWD", "", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "PWD\r\n"), true);

    const ip: [4]u8 = [10, 20, 30, 40];
    const n3 = encodePortCommand(ip, 5000, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "PORT 10,20,30,40,19,136\r\n"), true);

    const n4 = encodeEprtCommand(false, "132.235.1.2", 6275, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n4], "EPRT |1|132.235.1.2|6275|\r\n"), true);

    const n5 = encodeEprtCommand(true, "1080::8:800:200C:417A", 5282, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n5], "EPRT |2|1080::8:800:200C:417A|5282|\r\n"), true);
});

test("4. RFC 3659 MLSD / MLST zero-copy fact parsing and case-insensitive lookup", () => {
    const line = "type=file;size=4096;modify=20261122070000;Perm=r; readme.md\r\n";
    const entry = parseMlsxLine(line) ?? unreachable;
    expect_eq(std.mem.eql(u8, entry.filename, "readme.md"), true);
    expect_eq(std.mem.eql(u8, entry.entryType() ?? unreachable, "file"), true);
    expect_eq(entry.size() ?? 0, 4096 as u64);
    expect_eq(std.mem.eql(u8, entry.findFact("MODIFY") ?? unreachable, "20261122070000"), true);
    expect_eq(std.mem.eql(u8, entry.findFact("perm") ?? unreachable, "r"), true);
    expect_eq(entry.findFact("missing") == null, true);

    let it = entry.facts();
    let count: usize = 0;
    while (true) {
        if (f := it.next()) {
            * = f;
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 4);
});

test("5. Invalid reply code, >255 PASV octet, malformed EPSV, and CRLF injection rejection", () => {
    // 1. Invalid reply codes (099, 600, non-digit, bad separator)
    expect_eq(parseFtpReplyLine("099 bad") == null, true);
    expect_eq(parseFtpReplyLine("600 bad") == null, true);
    expect_eq(parseFtpReplyLine("22A bad") == null, true);
    expect_eq(parseFtpReplyLine("220*bad") == null, true);
    expect_eq(parseFtpReplyLine("22") == null, true);

    // 2. Malformed PASV (octet > 255, wrong tuple count, missing parens)
    expect_eq(parsePasvReply("(192,168,1,256,19,136)") == null, true);
    expect_eq(parsePasvReply("(192,168,1,10,19)") == null, true);
    expect_eq(parsePasvReply("(192,168,1,10,19,1,2)") == null, true);
    expect_eq(parsePasvReply("192,168,1,10,19,136") == null, true);

    // 3. Malformed EPSV (port 0, port > 65535, mismatched delimiters)
    expect_eq(parseEpsvPort("(|||0|)") == null, true);
    expect_eq(parseEpsvPort("(|||70000|)") == null, true);
    expect_eq(parseEpsvPort("(||#6446|)") == null, true);
    expect_eq(parseEpsvPort("(||||)") == null, true);

    // 4. CRLF injection & short buffer rejected by encoders
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    expect_eq(encodeFtpCommand("USER", "admin\r\nQUIT", buf[0..]) == null, true);
    expect_eq(encodeEprtCommand(false, "1.2.3.4|evil", 21, buf[0..]) == null, true);
    let tiny: [4]u8 = [0; 4];
    expect_eq(encodeFtpCommand("USER", "root", tiny[0..]) == null, true);
});
