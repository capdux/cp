import {
    ServiceEntry,
    EtcServicesIter,
    findServicePort,
    findServiceByPort,
} from "cap:sys";
import { test, expect, expect_eq } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_SERVICES =
    "# /etc/services sample\n" ++
    "\n" ++
    "   # indented comment\r\n" ++
    "ssh             22/tcp                          # SSH Remote Login Protocol\n" ++
    "domain          53/tcp          dns             # Domain Name Server\r\n" ++
    "domain          53/udp          dns             # Domain Name Server\n" ++
    "http            80/tcp          www www-http    # WorldWideWeb HTTP\n" ++
    "broken_no_port\n" ++
    "broken_no_proto 8080\n" ++
    "broken_zero     0/tcp\n" ++
    "broken_high     65536/tcp\n" ++
    "broken_slash    80/tcp/extra\n" ++
    "https           443/tcp         ssl-http\n" ++
    "ephemeral-max   65535/udp       maxport";

test("EtcServicesIter streams valid entries and skips comments, blanks, and malformed lines", () => {
    let it = EtcServicesIter.init(SAMPLE_SERVICES);

    const s0 = it.next().?;
    expect(strEq(s0.name, "ssh"));
    expect_eq(s0.port, 22);
    expect(strEq(s0.proto, "tcp"));
    expect_eq(s0.aliases.len, 0);
    expect_eq(s0.aliasCount(), 0);

    const s1 = it.next().?;
    expect(strEq(s1.name, "domain"));
    expect_eq(s1.port, 53);
    expect(strEq(s1.proto, "tcp"));
    expect(strEq(s1.aliases, "dns"));
    expect_eq(s1.aliasCount(), 1);

    const s2 = it.next().?;
    expect(strEq(s2.name, "domain"));
    expect_eq(s2.port, 53);
    expect(strEq(s2.proto, "udp"));

    const s3 = it.next().?;
    expect(strEq(s3.name, "http"));
    expect_eq(s3.port, 80);
    expect(strEq(s3.proto, "tcp"));
    expect(strEq(s3.aliases, "www www-http"));
    expect_eq(s3.aliasCount(), 2);

    const s4 = it.next().?;
    expect(strEq(s4.name, "https"));
    expect_eq(s4.port, 443);
    expect(strEq(s4.aliases, "ssl-http"));

    const s5 = it.next().?;
    expect(strEq(s5.name, "ephemeral-max"));
    expect_eq(s5.port, 65535);
    expect(strEq(s5.proto, "udp"));
    expect(strEq(s5.aliases, "maxport"));

    expect(it.next() == null);
    expect(it.next() == null);
});

test("ServiceEntry.hasNameOrAlias exact matching without substring false positives", () => {
    const http = findServiceByPort(SAMPLE_SERVICES, 80, "tcp").?;
    expect(http.hasNameOrAlias("http"));
    expect(http.hasNameOrAlias("www"));
    expect(http.hasNameOrAlias("www-http"));

    // Substring / prefix / suffix / empty queries must fail
    expect(!http.hasNameOrAlias("ht"));
    expect(!http.hasNameOrAlias("ww"));
    expect(!http.hasNameOrAlias("www-"));
    expect(!http.hasNameOrAlias("www www-http"));
    expect(!http.hasNameOrAlias(""));

    const ssh = findServiceByPort(SAMPLE_SERVICES, 22, "tcp").?;
    expect(ssh.hasNameOrAlias("ssh"));
    expect(!ssh.hasNameOrAlias("SSH"));
    expect(!ssh.hasNameOrAlias(""));
});

test("findServicePort resolves primary names and aliases with case-insensitive protocol", () => {
    expect_eq(findServicePort(SAMPLE_SERVICES, "ssh", "tcp").?, 22);
    expect_eq(findServicePort(SAMPLE_SERVICES, "ssh", "TCP").?, 22);
    expect_eq(findServicePort(SAMPLE_SERVICES, "www", "Tcp").?, 80);
    expect_eq(findServicePort(SAMPLE_SERVICES, "www-http", "tcp").?, 80);
    expect_eq(findServicePort(SAMPLE_SERVICES, "dns", "UDP").?, 53);
    expect_eq(findServicePort(SAMPLE_SERVICES, "ssl-http", "TCP").?, 443);
    expect_eq(findServicePort(SAMPLE_SERVICES, "maxport", "Udp").?, 65535);

    // Wrong protocol or missing service
    expect(findServicePort(SAMPLE_SERVICES, "ssh", "udp") == null);
    expect(findServicePort(SAMPLE_SERVICES, "unknown", "tcp") == null);
    expect(findServicePort(SAMPLE_SERVICES, "", "tcp") == null);
    expect(findServicePort(SAMPLE_SERVICES, "http", "") == null);
});

test("findServiceByPort reverse lookup distinguishes TCP vs UDP on shared ports", () => {
    const dns_tcp = findServiceByPort(SAMPLE_SERVICES, 53, "TCP").?;
    expect(strEq(dns_tcp.name, "domain"));
    expect(strEq(dns_tcp.proto, "tcp"));

    const dns_udp = findServiceByPort(SAMPLE_SERVICES, 53, "UDP").?;
    expect(strEq(dns_udp.name, "domain"));
    expect(strEq(dns_udp.proto, "udp"));

    const https = findServiceByPort(SAMPLE_SERVICES, 443, "Tcp").?;
    expect(strEq(https.name, "https"));

    expect(findServiceByPort(SAMPLE_SERVICES, 22, "udp") == null);
    expect(findServiceByPort(SAMPLE_SERVICES, 0, "tcp") == null);
    expect(findServiceByPort(SAMPLE_SERVICES, 80, "") == null);
    expect(findServiceByPort(SAMPLE_SERVICES, 12345, "tcp") == null);
});

test("Malformed line rejection, port boundary validation, and inline comment stripping", () => {
    // Blank and comment-only lines
    expect(ServiceEntry.parseLine("") == null);
    expect(ServiceEntry.parseLine("   \t  ") == null);
    expect(ServiceEntry.parseLine("# http 80/tcp") == null);
    expect(ServiceEntry.parseLine("  # comment only") == null);

    // Missing or malformed port/protocol
    expect(ServiceEntry.parseLine("http") == null);
    expect(ServiceEntry.parseLine("http 80") == null);
    expect(ServiceEntry.parseLine("http /tcp") == null);
    expect(ServiceEntry.parseLine("http 80/") == null);
    expect(ServiceEntry.parseLine("http 80/tcp/extra") == null);
    expect(ServiceEntry.parseLine("http abc/tcp") == null);
    expect(ServiceEntry.parseLine("http -1/tcp") == null);

    // Port range boundaries: 0 and > 65535 rejected; 1 and 65535 accepted
    expect(ServiceEntry.parseLine("zero 0/tcp") == null);
    expect(ServiceEntry.parseLine("over 65536/tcp") == null);
    expect(ServiceEntry.parseLine("huge 99999999/udp") == null);

    const min_port = ServiceEntry.parseLine("\ttcpmux\t1/tcp\t# TCP port multiplexer\r").?;
    expect(strEq(min_port.name, "tcpmux"));
    expect_eq(min_port.port, 1);
    expect(strEq(min_port.proto, "tcp"));
    expect_eq(min_port.aliases.len, 0);

    const with_comment = ServiceEntry.parseLine("custom 8080/sctp a1  a2#inline-comment").?;
    expect(strEq(with_comment.name, "custom"));
    expect_eq(with_comment.port, 8080);
    expect(strEq(with_comment.proto, "sctp"));
    expect(strEq(with_comment.aliases, "a1  a2"));
    expect(with_comment.hasNameOrAlias("a1"));
    expect(with_comment.hasNameOrAlias("a2"));
    expect(!with_comment.hasNameOrAlias("#inline-comment"));

    // Empty content queries
    let empty_it = EtcServicesIter.init("");
    expect(empty_it.next() == null);
    expect(findServicePort("", "http", "tcp") == null);
    expect(findServiceByPort("", 80, "tcp") == null);
});
