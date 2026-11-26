import { test, expect_eq } from "cap:test";
import {
    DomainInfo,
    PslRuleList,
    parseDomain,
    suffix,
    registrableDomain,
    isPublicSuffix
} from "../../packages/@cap/publicsuffix/src/index.cp";

fn main() {
    test("1. @cap/publicsuffix — single-level TLD eTLD and registrable domain", () => {
        expect_eq(suffix("www.example.com") ?? "", "com");
        expect_eq(registrableDomain("www.example.com") ?? "", "example.com");
        expect_eq(registrableDomain("a.b.c.example.dev") ?? "", "example.dev");
        expect_eq(isPublicSuffix("com"), true);
        expect_eq(isPublicSuffix("example.com"), false);
        expect_eq(registrableDomain("com") == null, true);
    });

    test("2. @cap/publicsuffix — multi-level public suffixes (co.uk, com.cn, github.io, s3.amazonaws.com)", () => {
        expect_eq(suffix("www.bbc.co.uk") ?? "", "co.uk");
        expect_eq(registrableDomain("www.bbc.co.uk") ?? "", "bbc.co.uk");
        expect_eq(isPublicSuffix("co.uk"), true);
        expect_eq(registrableDomain("co.uk") == null, true);

        expect_eq(suffix("api.news.com.cn") ?? "", "com.cn");
        expect_eq(registrableDomain("api.news.com.cn") ?? "", "news.com.cn");

        expect_eq(suffix("user.github.io") ?? "", "github.io");
        expect_eq(registrableDomain("user.github.io") ?? "", "user.github.io");
        expect_eq(isPublicSuffix("github.io"), true);

        expect_eq(suffix("mybucket.s3.amazonaws.com") ?? "", "s3.amazonaws.com");
        expect_eq(registrableDomain("mybucket.s3.amazonaws.com") ?? "", "mybucket.s3.amazonaws.com");
    });

    test("3. @cap/publicsuffix — custom PslRuleList wildcard (*.ck) and exception (!www.ck) precedence", () => {
        let list = PslRuleList.new();
        expect_eq(list.addRule("*.ck"), true);
        expect_eq(list.addRule("!www.ck"), true);

        // Wildcard *.ck makes foo.ck a public suffix, so bar.foo.ck has registrable bar.foo.ck
        expect_eq(list.suffix("bar.foo.ck") ?? "", "foo.ck");
        expect_eq(list.registrableDomain("bar.foo.ck") ?? "", "bar.foo.ck");
        expect_eq(list.isPublicSuffix("foo.ck"), true);

        // Exception !www.ck overrides *.ck: suffix is ck, registrable is www.ck
        expect_eq(list.suffix("www.ck") ?? "", "ck");
        expect_eq(list.registrableDomain("www.ck") ?? "", "www.ck");
        expect_eq(list.suffix("sub.www.ck") ?? "", "ck");
        expect_eq(list.registrableDomain("sub.www.ck") ?? "", "www.ck");
        expect_eq(list.isPublicSuffix("www.ck"), false);
    });

    test("4. @cap/publicsuffix — case normalization and trailing FQDN dot handling", () => {
        let info_opt = parseDomain("API.Example.CO.UK.");
        expect_eq(info_opt != null, true);
        let info = info_opt ?? DomainInfo {
            norm: [0; 256],
            norm_len: 0,
            suffix_start: 0,
            suffix_len: 0,
            root_start: 0,
            root_len: 0,
            has_root: false,
            is_known_suffix: false,
        };
        expect_eq(info.hostEq("api.example.co.uk"), true);
        expect_eq(info.suffixEq("co.uk"), true);
        expect_eq(info.registrableEq("example.co.uk"), true);

        let buf: [64]u8 = [0; 64];
        let n = info.writeRegistrable(buf[0..64]);
        expect_eq(n, 13);
        expect_eq(buf[0], 101); // 'e'
        expect_eq(buf[12], 107); // 'k'
    });

    test("5. @cap/publicsuffix — unknown TLD fallback to single-label implicit wildcard", () => {
        let info_opt = parseDomain("sub.myhost.unknownbrand");
        expect_eq(info_opt != null, true);
        let info = info_opt ?? DomainInfo {
            norm: [0; 256],
            norm_len: 0,
            suffix_start: 0,
            suffix_len: 0,
            root_start: 0,
            root_len: 0,
            has_root: false,
            is_known_suffix: false,
        };
        expect_eq(info.is_known_suffix, false);
        expect_eq(info.suffixEq("unknownbrand"), true);
        expect_eq(info.registrableEq("myhost.unknownbrand"), true);
        expect_eq(isPublicSuffix("unknownbrand"), false);
    });

    test("6. @cap/publicsuffix — rejects malformed hostnames (empty, double dot, leading/trailing hyphen)", () => {
        expect_eq(parseDomain("") == null, true);
        expect_eq(parseDomain(".") == null, true);
        expect_eq(parseDomain("example..com") == null, true);
        expect_eq(parseDomain(".example.com") == null, true);
        expect_eq(parseDomain("example.com..") == null, true);
        expect_eq(parseDomain("-bad.com") == null, true);
        expect_eq(parseDomain("bad-.com") == null, true);
        expect_eq(parseDomain("bad host.com") == null, true);
    });
}
