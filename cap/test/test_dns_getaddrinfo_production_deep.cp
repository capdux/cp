// Production Deep Tests for cap:dns GetAddrInfo (lib/dns/getaddrinfo.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_DNS_HOSTNAME_LEN,
    AI_FLAG_PASSIVE,
    AI_FLAG_CANONNAME,
    AI_FLAG_NUMERICHOST,
    AI_FLAG_V4MAPPED,
    AI_FLAG_ALL,
    AI_FLAG_ADDRCONFIG,
    AI_FLAG_NUMERICSERV,
    GetAddrInfoError,
    AddrInfoFamily,
    AddrInfoSocketType,
    AddrInfoProtocol,
    AddrInfoBackend,
    AddrInfoFlags,
    GetAddrInfoOptions,
    GetAddrInfoQuery,
} from "cap:dns";

test("dns getaddrinfo: Family, SocketType, Protocol & Backend parsing + POSIX constants", fn() {
    expect_eq(AddrInfoFamily.parse("IPv4") catch unreachable == .inet, true);
    expect_eq(AddrInfoFamily.parse("ipv6") catch unreachable == .inet6, true);
    expect_eq(AddrInfoFamily.parse("any") catch unreachable == .unspecified, true);
    expect_eq(AddrInfoFamily.parse("unix") catch unreachable == .unix, true);
    expect_eq(AddrInfoFamily.inet.toPosixAf(), 2);
    expect_eq(AddrInfoFamily.inet6.toPosixAf(), 10);
    expect_eq(AddrInfoFamily.unix.toPosixAf(), 1);

    expect_eq(AddrInfoSocketType.parse("stream") catch unreachable == .stream, true);
    expect_eq(AddrInfoSocketType.parse("DGRAM") catch unreachable == .dgram, true);
    expect_eq(AddrInfoSocketType.stream.toPosixSockType(), 1);
    expect_eq(AddrInfoSocketType.dgram.toPosixSockType(), 2);

    expect_eq(AddrInfoProtocol.parse("tcp") catch unreachable == .tcp, true);
    expect_eq(AddrInfoProtocol.parse("UDP") catch unreachable == .udp, true);
    expect_eq(AddrInfoProtocol.tcp.toPosixProto(), 6);
    expect_eq(AddrInfoProtocol.udp.toPosixProto(), 17);

    // All Bun GetAddrInfo.Backend aliases
    expect_eq(AddrInfoBackend.parse("c-ares") catch unreachable == .c_ares, true);
    expect_eq(AddrInfoBackend.parse("c_ares") catch unreachable == .c_ares, true);
    expect_eq(AddrInfoBackend.parse("cares") catch unreachable == .c_ares, true);
    expect_eq(AddrInfoBackend.parse("async") catch unreachable == .c_ares, true);
    expect_eq(AddrInfoBackend.parse("system") catch unreachable == .system, true);
    expect_eq(AddrInfoBackend.parse("libc") catch unreachable == .libc, true);
    expect_eq(AddrInfoBackend.parse("getaddrinfo") catch unreachable == .libc, true);
});

test("dns getaddrinfo: AddrInfoFlags bitmask validation & unknown bit rejection", fn() {
    const flags = AddrInfoFlags.fromBits(AI_FLAG_V4MAPPED | AI_FLAG_ADDRCONFIG | AI_FLAG_CANONNAME) catch unreachable;
    expect_eq(flags.has(AI_FLAG_V4MAPPED), true);
    expect_eq(flags.has(AI_FLAG_ADDRCONFIG), true);
    expect_eq(flags.has(AI_FLAG_PASSIVE), false);

    const flags2 = flags.withFlag(AI_FLAG_NUMERICSERV);
    expect_eq(flags2.has(AI_FLAG_NUMERICSERV), true);
    expect_eq(flags.eql(flags2), false);

    // Unknown high bit must be rejected with InvalidFlags
    let bad_flags = false;
    * = AddrInfoFlags.fromBits(0x80000000) catch { bad_flags = true; };
    expect_eq(bad_flags, true);
});

test("dns getaddrinfo: Options packing, hasNoHints() & case-insensitive cache key hash", fn() {
    const default_no_hints = GetAddrInfoOptions{
        family: .unspecified,
        socktype: .unspecified,
        protocol: .unspecified,
        backend: .c_ares,
        flags: AddrInfoFlags.empty(),
    };
    expect_eq(default_no_hints.hasNoHints(), true);

    const opts = GetAddrInfoOptions{
        family: .inet,
        socktype: .stream,
        protocol: .tcp,
        backend: .c_ares,
        flags: AddrInfoFlags.fromBits(AI_FLAG_ADDRCONFIG) catch unreachable,
    };
    expect_eq(opts.hasNoHints(), false);

    const q1 = GetAddrInfoQuery.init("API.Example.COM", 443, opts) catch unreachable;
    const q2 = GetAddrInfoQuery.init("api.example.com", 443, opts) catch unreachable;
    const q3 = GetAddrInfoQuery.init("api.example.com", 80, opts) catch unreachable;

    // Case-insensitive equality & identical cache key hash
    expect_eq(q1.eql(q2), true);
    expect_eq(q1.hash(), q2.hash());
    // Different port yields distinct hash
    expect_eq(q1.eql(q3), false);
    expect_eq(q1.hash() != q3.hash(), true);
});

test("dns getaddrinfo: localhost detection & summary formatting", fn() {
    const opts = GetAddrInfoOptions{
        family: .inet6,
        socktype: .stream,
        protocol: .tcp,
        backend: .system,
    };
    const q_local = GetAddrInfoQuery.init("LOCALHOST", 8080, opts) catch unreachable;
    expect_eq(q_local.isLocalhost(), true);

    const q_v6 = GetAddrInfoQuery.init("::1", 0, opts) catch unreachable;
    expect_eq(q_v6.isLocalhost(), true);

    const q_remote = GetAddrInfoQuery.init("cp-lang.org", 443, opts) catch unreachable;
    expect_eq(q_remote.isLocalhost(), false);

    let buf: [128]u8 = undefined;
    const summary = q_remote.formatSummary(buf[0..]) catch unreachable;
    expect_eq(summary, "cp-lang.org:443 [IPv6/stream/tcp via system]");
});

test("dns getaddrinfo: rejects invalid enum strings, empty hostname & oversized hostname", fn() {
    expect_eq(MAX_DNS_HOSTNAME_LEN, 253);

    let bad_fam = false;
    * = AddrInfoFamily.parse("ipv5") catch { bad_fam = true; };
    expect_eq(bad_fam, true);

    let bad_sock = false;
    * = AddrInfoSocketType.parse("seqpacket") catch { bad_sock = true; };
    expect_eq(bad_sock, true);

    let bad_proto = false;
    * = AddrInfoProtocol.parse("sctp") catch { bad_proto = true; };
    expect_eq(bad_proto, true);

    let bad_backend = false;
    * = AddrInfoBackend.parse("unknown_dns") catch { bad_backend = true; };
    expect_eq(bad_backend, true);

    let empty_host = false;
    * = GetAddrInfoQuery.init("", 80, GetAddrInfoOptions{}) catch { empty_host = true; };
    expect_eq(empty_host, true);

    // 254-byte hostname exceeds RFC 1035 limit (253)
    let long_host: [254]u8 = undefined;
    let i: usize = 0;
    while (i < 254) {
        long_host[i] = 'a';
        i += 1;
    }
    let too_long = false;
    * = GetAddrInfoQuery.init(long_host[0..], 80, GetAddrInfoOptions{}) catch { too_long = true; };
    expect_eq(too_long, true);
});

test("dns getaddrinfo: BufferTooSmall guard on formatSummary", fn() {
    const q = GetAddrInfoQuery.init("example.com", 443, GetAddrInfoOptions{}) catch unreachable;
    let tiny: [8]u8 = undefined;
    let oob = false;
    * = q.formatSummary(tiny[0..]) catch { oob = true; };
    expect_eq(oob, true);
});
