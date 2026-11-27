// cap/test/testCapHostsFile.cp
// Comprehensive unit tests for @cap/hosts-file

import { test, expect_eq } from "cap:test";
import {
    parseHosts,
    lookupIp,
    lookupIpByFamily,
    reverseLookup,
    formatHosts,
} from "../../packages/hostile/src/index.cp";

const SAMPLE_HOSTS = "# System hosts file\n\n127.0.0.1\tlocalhost local.capdux.com api.local # loopback aliases\r\n::1             localhost ip6-localhost\nfe80::1%lo0     link-local.dev\n10.0.0.42       db.internal   # primary database\n";

fn main() {
    test("1. @cap/hosts-file — multi-alias line expansion & inline comment stripping", () => {
        const table = parseHosts(SAMPLE_HOSTS);
        expect_eq(table.count, 7 as usize);

        expect_eq(table.entryAt(0 as usize).ip, "127.0.0.1");
        expect_eq(table.entryAt(0 as usize).hostname, "localhost");
        expect_eq(table.entryAt(0 as usize).is_ipv6, false);

        expect_eq(table.entryAt(1 as usize).ip, "127.0.0.1");
        expect_eq(table.entryAt(1 as usize).hostname, "local.capdux.com");

        expect_eq(table.entryAt(2 as usize).ip, "127.0.0.1");
        expect_eq(table.entryAt(2 as usize).hostname, "api.local");
    });

    test("2. @cap/hosts-file — IPv4 & IPv6 detection including scoped fe80::1%lo0", () => {
        const table = parseHosts(SAMPLE_HOSTS);

        const v6_loop = table.entryAt(3 as usize);
        expect_eq(v6_loop.ip, "::1");
        expect_eq(v6_loop.hostname, "localhost");
        expect_eq(v6_loop.is_ipv6, true);

        const scoped = table.entryAt(5 as usize);
        expect_eq(scoped.ip, "fe80::1%lo0");
        expect_eq(scoped.hostname, "link-local.dev");
        expect_eq(scoped.is_ipv6, true);

        const db = table.entryAt(6 as usize);
        expect_eq(db.ip, "10.0.0.42");
        expect_eq(db.hostname, "db.internal");
        expect_eq(db.is_ipv6, false);
    });

    test("3. @cap/hosts-file — case-insensitive forward lookupIp", () => {
        const table = parseHosts(SAMPLE_HOSTS);

        expect_eq(lookupIp(table, "LOCAL.CAPDUX.COM").?, "127.0.0.1");
        expect_eq(lookupIp(table, "Api.Local").?, "127.0.0.1");
        expect_eq(lookupIp(table, "DB.INTERNAL").?, "10.0.0.42");
        expect_eq(lookupIp(table, "LINK-LOCAL.DEV").?, "fe80::1%lo0");
        expect_eq(lookupIp(table, "nonexistent.host") == null, true);
    });

    test("4. @cap/hosts-file — lookupIpByFamily distinguishes IPv4 vs IPv6", () => {
        const table = parseHosts(SAMPLE_HOSTS);

        expect_eq(lookupIpByFamily(table, "localhost", false).?, "127.0.0.1");
        expect_eq(lookupIpByFamily(table, "LOCALHOST", true).?, "::1");
        expect_eq(lookupIpByFamily(table, "db.internal", false).?, "10.0.0.42");
        expect_eq(lookupIpByFamily(table, "db.internal", true) == null, true);
        expect_eq(lookupIpByFamily(table, "ip6-localhost", false) == null, true);
        expect_eq(lookupIpByFamily(table, "ip6-localhost", true).?, "::1");
    });

    test("5. @cap/hosts-file — reverseLookup returns primary hostname", () => {
        const table = parseHosts(SAMPLE_HOSTS);

        expect_eq(reverseLookup(table, "127.0.0.1").?, "localhost");
        expect_eq(reverseLookup(table, "::1").?, "localhost");
        expect_eq(reverseLookup(table, "FE80::1%LO0").?, "link-local.dev");
        expect_eq(reverseLookup(table, "10.0.0.42").?, "db.internal");
        expect_eq(reverseLookup(table, "8.8.8.8") == null, true);
    });

    test("6. @cap/hosts-file — formatHosts alignment & round-trip parsing", () => {
        const table = parseHosts("127.0.0.1 localhost api.local\n::1 localhost\n");
        let buf: [512]u8 = undefined;
        const formatted = formatHosts(table, buf[0..512]);
        expect_eq(formatted, "127.0.0.1       localhost api.local\n::1             localhost");

        // Round-trip re-parse yields identical entries
        const rt = parseHosts(formatted);
        expect_eq(rt.count, 3 as usize);
        expect_eq(lookupIpByFamily(rt, "api.local", false).?, "127.0.0.1");
        expect_eq(lookupIpByFamily(rt, "localhost", true).?, "::1");

        // Empty hosts file
        const empty = parseHosts("# comment only\n   \n");
        expect_eq(empty.count, 0 as usize);
        expect_eq(formatHosts(empty, buf[0..512]), "");
    });
}
