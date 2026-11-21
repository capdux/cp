import { test, expect_eq } from "cap:test";
import { GitConfig } from "cap:gitconfig_ini";

test("1. [section], [section \"subsection\"], case-sensitivity & last-definition-wins override", () => {
    const src =
        "[CORE]\n"
        ++ "  autocrlf = input\n"
        ++ "  AutoCRLF = false\n"
        ++ "[remote \"Origin\"]\n"
        ++ "  url = https://example.com/a.git\n"
        ++ "[remote \"origin\"]\n"
        ++ "  url = https://github.com/cp-lang/cp.git\n";

    const cfg = GitConfig.parse(src);
    expect_eq(std.mem.eql(u8, cfg.get("core", null, "autocrlf").?, "false"), true);
    expect_eq(std.mem.eql(u8, cfg.get("remote", "Origin", "url").?, "https://example.com/a.git"), true);
    expect_eq(std.mem.eql(u8, cfg.get("remote", "origin", "URL").?, "https://github.com/cp-lang/cp.git"), true);
    expect_eq(cfg.get("remote", "ORIGIN", "url") == null, true);
});

test("2. Comments (# and ;), quoted values preserving #/;, and implicit boolean keys", () => {
    const src =
        "# top comment\n"
        ++ "; semicolon comment\n"
        ++ "[core]\n"
        ++ "  bare\n"
        ++ "  commentChar = \"# keep ; inside\" # trailing comment\n"
        ++ "  editor = vim ; inline comment\n";

    const cfg = GitConfig.parse(src);
    expect_eq(cfg.getBool("core", null, "bare").?, true);
    expect_eq(std.mem.eql(u8, cfg.get("core", null, "commentChar").?, "# keep ; inside"), true);
    expect_eq(std.mem.eql(u8, cfg.get("core", null, "editor").?, "vim"), true);
});

test("3. getBool (true/yes/on/1 vs false/no/off/0) & hasCoreAskpass / hasCoreSshCommand", () => {
    const src =
        "[core]\n"
        ++ "  a = yes\n"
        ++ "  b = ON\n"
        ++ "  c = 1\n"
        ++ "  d = no\n"
        ++ "  e = off\n"
        ++ "  f = 0\n"
        ++ "  bad = maybe\n"
        ++ "  askpass = /usr/bin/ssh-askpass\n"
        ++ "  sshCommand = ssh -i ~/.ssh/id_ed25519\n";

    const cfg = GitConfig.parse(src);
    expect_eq(cfg.getBool("core", null, "a").?, true);
    expect_eq(cfg.getBool("core", null, "b").?, true);
    expect_eq(cfg.getBool("core", null, "c").?, true);
    expect_eq(cfg.getBool("core", null, "d").?, false);
    expect_eq(cfg.getBool("core", null, "e").?, false);
    expect_eq(cfg.getBool("core", null, "f").?, false);
    expect_eq(cfg.getBool("core", null, "bad") == null, true);
    expect_eq(cfg.hasCoreAskpass(), true);
    expect_eq(cfg.hasCoreSshCommand(), true);
});

test("4. url.<base>.insteadOf longest-prefix URL rewriting", () => {
    const src =
        "[url \"https://github.com/\"]\n"
        ++ "  insteadOf = gh:\n"
        ++ "  insteadOf = git://github.com/\n"
        ++ "[url \"ssh://git@corp.internal/mirror/\"]\n"
        ++ "  insteadOf = git://github.com/corp/\n";

    const cfg = GitConfig.parse(src);
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, cfg.rewriteUrl("gh:oven-sh/bun.git", buf[0..]).?, "https://github.com/oven-sh/bun.git"), true);
    // Longest prefix wins for git://github.com/corp/
    expect_eq(std.mem.eql(u8, cfg.rewriteUrl("git://github.com/corp/api.git", buf[0..]).?, "ssh://git@corp.internal/mirror/api.git"), true);
    // Unmatched URL returned unchanged
    expect_eq(std.mem.eql(u8, cfg.rewriteUrl("https://gitlab.com/a/b.git", buf[0..]).?, "https://gitlab.com/a/b.git"), true);
    // Buffer too small -> null
    expect_eq(cfg.rewriteUrl("gh:oven-sh/bun.git", buf[0..5]) == null, true);
});

test("5. Malformed lines, empty input & missing section guards", () => {
    const empty = GitConfig.parse("");
    expect_eq(empty.len, usize(0));
    expect_eq(empty.hasCoreAskpass(), false);
    expect_eq(empty.hasCoreSshCommand(), false);

    const malformed =
        "orphan = ignored\n"
        ++ "[unclosed\n"
        ++ "[]\n"
        ++ "[bad \"unterminated]\n"
        ++ "  x = 1\n"
        ++ "[valid]\n"
        ++ "  ok = yes\n";
    const cfg = GitConfig.parse(malformed);
    expect_eq(cfg.get("bad", null, "x") == null, true);
    expect_eq(cfg.getBool("valid", null, "ok").?, true);
});
