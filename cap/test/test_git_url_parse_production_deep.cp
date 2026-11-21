// Source: cp/cap/test/test_git_url_parse_production_deep.cp (Production deep tests for cap:git_url_parse)

import { test, expect_eq } from "cap:test";
import {
    GitProtocol,
    GitRemoteUrl,
} from "cap:git_url_parse";

test("git_url_parse: SCP shorthand git@github.com:org/repo.git and formatHttps conversion") {
    const u = GitRemoteUrl.parse("git@github.com:org/repo.git").?;
    expect_eq(u.protocol == .scp, true);
    expect_eq(std.mem.eql(u8, u.protocol.asStr(), "scp"), true);
    expect_eq(std.mem.eql(u8, u.user.?, "git"), true);
    expect_eq(std.mem.eql(u8, u.host.?, "github.com"), true);
    expect_eq(u.port == null, true);
    expect_eq(std.mem.eql(u8, u.path, "org/repo"), true);
    expect_eq(std.mem.eql(u8, u.owner.?, "org"), true);
    expect_eq(std.mem.eql(u8, u.repo, "repo"), true);

    let buf: [128]u8 = [0; 128];
    expect_eq(std.mem.eql(u8, u.formatHttps(buf[0..]).?, "https://github.com/org/repo.git"), true);
    expect_eq(std.mem.eql(u8, u.formatScp(buf[0..]).?, "git@github.com:org/repo.git"), true);
}

test("git_url_parse: ssh:// with custom port and nested subgroup owner") {
    const u = GitRemoteUrl.parse("ssh://git@gitlab.internal:2222/team/sub/service.git").?;
    expect_eq(u.protocol == .ssh, true);
    expect_eq(std.mem.eql(u8, u.user.?, "git"), true);
    expect_eq(std.mem.eql(u8, u.host.?, "gitlab.internal"), true);
    expect_eq(u.port.?, 2222);
    expect_eq(std.mem.eql(u8, u.path, "team/sub/service"), true);
    expect_eq(std.mem.eql(u8, u.owner.?, "team/sub"), true);
    expect_eq(std.mem.eql(u8, u.repo, "service"), true);

    let buf: [128]u8 = [0; 128];
    expect_eq(std.mem.eql(u8, u.formatScp(buf[0..]).?, "git@gitlab.internal:team/sub/service.git"), true);
    expect_eq(std.mem.eql(u8, u.formatHttps(buf[0..]).?, "https://gitlab.internal/team/sub/service.git"), true);
}

test("git_url_parse: https:// with token auth and git:// protocol") {
    const u1 = GitRemoteUrl.parse("https://ghp_secret@github.com/cp-lang/cp").?;
    expect_eq(u1.protocol == .https, true);
    expect_eq(std.mem.eql(u8, u1.user.?, "ghp_secret"), true);
    expect_eq(std.mem.eql(u8, u1.host.?, "github.com"), true);
    expect_eq(std.mem.eql(u8, u1.owner.?, "cp-lang"), true);
    expect_eq(std.mem.eql(u8, u1.repo, "cp"), true);

    // 转 SCP 时自动使用 "git" 用户，不泄露 HTTPS token
    let buf: [128]u8 = [0; 128];
    expect_eq(std.mem.eql(u8, u1.formatScp(buf[0..]).?, "git@github.com:cp-lang/cp.git"), true);
    expect_eq(std.mem.eql(u8, u1.formatHttps(buf[0..]).?, "https://github.com/cp-lang/cp.git"), true);

    const u2 = GitRemoteUrl.parse("git://git.kernel.org/pub/scm/linux.git").?;
    expect_eq(u2.protocol == .git, true);
    expect_eq(std.mem.eql(u8, u2.repo, "linux"), true);
}

test("git_url_parse: file:/// local repository URL") {
    const u = GitRemoteUrl.parse("file:///var/repos/app.git").?;
    expect_eq(u.protocol == .file, true);
    expect_eq(u.host == null, true);
    expect_eq(std.mem.eql(u8, u.path, "var/repos/app"), true);
    expect_eq(std.mem.eql(u8, u.owner.?, "var/repos"), true);
    expect_eq(std.mem.eql(u8, u.repo, "app"), true);

    let buf: [64]u8 = [0; 64];
    expect_eq(u.formatHttps(buf[0..]) == null, true);
    expect_eq(u.formatScp(buf[0..]) == null, true);
}

test("git_url_parse: malformed URLs and small output buffer overflow guards") {
    expect_eq(GitRemoteUrl.parse("") == null, true);
    expect_eq(GitRemoteUrl.parse("   ") == null, true);
    expect_eq(GitRemoteUrl.parse("ftp://example.com/a/b.git") == null, true);
    expect_eq(GitRemoteUrl.parse("https://github.com/") == null, true);
    expect_eq(GitRemoteUrl.parse("https://github.com") == null, true);
    expect_eq(GitRemoteUrl.parse("https://github.com:99999/a/b") == null, true);
    expect_eq(GitRemoteUrl.parse("git@github.com:") == null, true);
    expect_eq(GitRemoteUrl.parse("just-a-local-path/no-colon") == null, true);

    const u = GitRemoteUrl.parse("https://github.com/org/repo.git").?;
    let tiny: [8]u8 = [0; 8];
    expect_eq(u.formatHttps(tiny[0..]) == null, true);
    expect_eq(u.formatScp(tiny[0..]) == null, true);
}
