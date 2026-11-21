import {
    HostsIter,
    lookupHost,
    lookupHostV4,
    lookupHostV6,
    reverseLookup,
} from "cap:hosts_file";
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

const SAMPLE_HOSTS =
    "# System hosts file\r\n" ++
    "127.0.0.1\tlocalhost localhost.localdomain # loopback\r\n" ++
    "::1        localhost ip6-localhost ip6-loopback\n" ++
    "   \n" ++
    "10.0.0.5   api.internal api.local  # internal gateway\n" ++
    "fe80::1    gateway.local\n" ++
    "192.168.1.10\n";

test("HostsIter skips comments, blanks, and malformed single-column lines", () => {
    let it = HostsIter.init(SAMPLE_HOSTS);

    const e1 = it.next().?;
    expect(strEq(e1.ip, "127.0.0.1"));
    expect(strEq(e1.canonical_host, "localhost"));
    expect(!e1.is_ipv6);
    expect(e1.hostCount() == 2);
    expect(e1.matchesHost("LOCALHOST.LOCALDOMAIN"));

    const e2 = it.next().?;
    expect(strEq(e2.ip, "::1"));
    expect(e2.is_ipv6);
    expect(e2.hostCount() == 3);
    expect(e2.matchesHost("ip6-loopback"));

    const e3 = it.next().?;
    expect(strEq(e3.ip, "10.0.0.5"));
    expect(e3.matchesHost("api.local"));

    const e4 = it.next().?;
    expect(strEq(e4.ip, "fe80::1"));
    expect(e4.is_ipv6);

    expect(it.next() == null);
});

test("lookupHost, lookupHostV4, lookupHostV6, and reverseLookup", () => {
    expect(strEq(lookupHost(SAMPLE_HOSTS, "localhost").?, "127.0.0.1"));
    expect(strEq(lookupHostV4(SAMPLE_HOSTS, "localhost").?, "127.0.0.1"));
    expect(strEq(lookupHostV6(SAMPLE_HOSTS, "localhost").?, "::1"));
    expect(strEq(lookupHost(SAMPLE_HOSTS, "API.LOCAL").?, "10.0.0.5"));
    expect(lookupHostV6(SAMPLE_HOSTS, "api.local") == null);
    expect(lookupHost(SAMPLE_HOSTS, "missing.example") == null);

    expect(strEq(reverseLookup(SAMPLE_HOSTS, "10.0.0.5").?, "api.internal"));
    expect(strEq(reverseLookup(SAMPLE_HOSTS, "FE80::1").?, "gateway.local"));
    expect(reverseLookup(SAMPLE_HOSTS, "8.8.8.8") == null);
});
