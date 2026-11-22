import std from "std";
import { test, expect_eq } from "cap:test";
import { NetrcIter, lookupNetrc } from "cap:net";

test("1. NetrcIter parses multi-line, single-line, comments, and account fields", () => {
    const src =
        "# global comment\n"
        ++ "machine github.com\n"
        ++ "  login octocat\n"
        ++ "  password secret#123\n"
        ++ "  account org-admin\n"
        ++ "# inline style\n"
        ++ "machine npm.pkg.github.com login ci-bot password ghp_abc123\n"
        ++ "default login anonymous password guest@example.com\n";

    let it = NetrcIter.init(src);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.is_default, false);
    expect_eq(std.mem.eql(u8, e0.machine ?? unreachable, "github.com"), true);
    expect_eq(std.mem.eql(u8, e0.login ?? unreachable, "octocat"), true);
    expect_eq(std.mem.eql(u8, e0.password ?? unreachable, "secret#123"), true);
    expect_eq(std.mem.eql(u8, e0.account ?? unreachable, "org-admin"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.machine ?? unreachable, "npm.pkg.github.com"), true);
    expect_eq(std.mem.eql(u8, e1.login ?? unreachable, "ci-bot"), true);
    expect_eq(std.mem.eql(u8, e1.password ?? unreachable, "ghp_abc123"), true);
    expect_eq(e1.account == null, true);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.is_default, true);
    expect_eq(e2.machine == null, true);
    expect_eq(std.mem.eql(u8, e2.login ?? unreachable, "anonymous"), true);
    expect_eq(std.mem.eql(u8, e2.password ?? unreachable, "guest@example.com"), true);

    expect_eq(it.next() == null, true);
});

test("2. NetrcIter handles double-quoted values containing spaces", () => {
    const src = "machine \"my host.local\" login \"Alice Smith\" password \"p@ss word 42\"\n";
    let it = NetrcIter.init(src);

    const e = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e.machine ?? unreachable, "my host.local"), true);
    expect_eq(std.mem.eql(u8, e.login ?? unreachable, "Alice Smith"), true);
    expect_eq(std.mem.eql(u8, e.password ?? unreachable, "p@ss word 42"), true);
    expect_eq(it.next() == null, true);
});

test("3. NetrcIter skips macdef blocks until blank line without corrupting subsequent entries", () => {
    const src =
        "machine ftp.example.com\n"
        ++ "  login uploader\n"
        ++ "  password pass1\n"
        ++ "  macdef init\n"
        ++ "    machine fake.inside.macdef\n"
        ++ "    binary\n"
        ++ "    put file.bin\n"
        ++ "\n"
        ++ "machine real.after.macdef\n"
        ++ "  login realuser\n"
        ++ "  password realpass\n";

    let it = NetrcIter.init(src);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.machine ?? unreachable, "ftp.example.com"), true);
    expect_eq(std.mem.eql(u8, e0.login ?? unreachable, "uploader"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.machine ?? unreachable, "real.after.macdef"), true);
    expect_eq(std.mem.eql(u8, e1.login ?? unreachable, "realuser"), true);
    expect_eq(std.mem.eql(u8, e1.password ?? unreachable, "realpass"), true);

    expect_eq(it.next() == null, true);
});

test("4. lookupNetrc case-insensitive exact match and default fallback", () => {
    const src =
        "default login fallback-user password fallback-pass\n"
        ++ "machine Registry.Npmjs.Org login npm-user password npm-token\n";

    // 精确匹配优先于先出现的 default，且大小写不敏感
    const exact = lookupNetrc(src, "registry.npmjs.org") ?? unreachable;
    expect_eq(exact.is_default, false);
    expect_eq(std.mem.eql(u8, exact.login ?? unreachable, "npm-user"), true);
    expect_eq(std.mem.eql(u8, exact.password ?? unreachable, "npm-token"), true);

    // 未命中任何 machine 时回退到 default
    const fb = lookupNetrc(src, "unknown.example.com") ?? unreachable;
    expect_eq(fb.is_default, true);
    expect_eq(std.mem.eql(u8, fb.login ?? unreachable, "fallback-user"), true);
});

test("5. empty input, comment-only input, and no-default miss return null", () => {
    expect_eq(lookupNetrc("", "example.com") == null, true);
    expect_eq(lookupNetrc("# only comments\n   \n", "example.com") == null, true);
    expect_eq(lookupNetrc("machine a.com login u password p", "") == null, true);
    expect_eq(lookupNetrc("machine a.com login u password p", "b.com") == null, true);
});
