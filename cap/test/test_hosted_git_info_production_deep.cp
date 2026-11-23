import { test, expect_eq } from "cap:test";
import { GitProvider, HostedGitInfo } from "cap:git";

test("1. GitHub shorthand, HTTPS, git+https, tree URL & all to* serializers", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const g1 = HostedGitInfo.parse("oven-sh/bun#main").?;
    expect_eq(g1.provider == GitProvider.github, true);
    expect_eq(std.mem.eql(u8, g1.user.?, "oven-sh"), true);
    expect_eq(std.mem.eql(u8, g1.project, "bun"), true);
    expect_eq(std.mem.eql(u8, g1.committish.?, "main"), true);

    expect_eq(std.mem.eql(u8, g1.toHttps(buf[0..]).?, "https://github.com/oven-sh/bun.git#main"), true);
    expect_eq(std.mem.eql(u8, g1.toSsh(buf[0..]).?, "git@github.com:oven-sh/bun.git#main"), true);
    expect_eq(std.mem.eql(u8, g1.toSshUrl(buf[0..]).?, "git+ssh://git@github.com/oven-sh/bun.git#main"), true);
    expect_eq(std.mem.eql(u8, g1.toShortcut(buf[0..]).?, "github:oven-sh/bun#main"), true);
    expect_eq(std.mem.eql(u8, g1.toTarball(buf[0..]).?, "https://codeload.github.com/oven-sh/bun/tar.gz/main"), true);
    expect_eq(std.mem.eql(u8, g1.toBrowse(buf[0..]).?, "https://github.com/oven-sh/bun/tree/main"), true);
    expect_eq(std.mem.eql(u8, g1.toBugs(buf[0..]).?, "https://github.com/oven-sh/bun/issues"), true);

    // /tree/<branch> URL extraction
    const g_tree = HostedGitInfo.parse("https://github.com/cp-lang/cp/tree/release-1.0").?;
    expect_eq(std.mem.eql(u8, g_tree.project, "cp"), true);
    expect_eq(std.mem.eql(u8, g_tree.committish.?, "release-1.0"), true);
});

test("2. SSH SCP & git+ssh:// URL parsing with .git stripping", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const scp = HostedGitInfo.parse("git@github.com:cp-lang/stdlib.git#v2").?;
    expect_eq(scp.provider == GitProvider.github, true);
    expect_eq(std.mem.eql(u8, scp.user.?, "cp-lang"), true);
    expect_eq(std.mem.eql(u8, scp.project, "stdlib"), true);
    expect_eq(std.mem.eql(u8, scp.committish.?, "v2"), true);
    expect_eq(scp.auth == null, true);

    const ssh_url = HostedGitInfo.parse("git+ssh://git@gitlab.com/team/app.git").?;
    expect_eq(ssh_url.provider == GitProvider.gitlab, true);
    expect_eq(std.mem.eql(u8, ssh_url.project, "app"), true);
    expect_eq(std.mem.eql(u8, ssh_url.toTarball(buf[0..]).?, "https://gitlab.com/team/app/repository/archive.tar.gz?ref=HEAD"), true);
});

test("3. GitLab & Bitbucket shortcuts, HTTPS & provider-specific URLs", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const gl = HostedGitInfo.parse("gitlab:org/service#dev").?;
    expect_eq(gl.provider == GitProvider.gitlab, true);
    expect_eq(std.mem.eql(u8, gl.provider.domain(), "gitlab.com"), true);
    expect_eq(std.mem.eql(u8, gl.toBrowse(buf[0..]).?, "https://gitlab.com/org/service/-/tree/dev"), true);
    expect_eq(std.mem.eql(u8, gl.toBugs(buf[0..]).?, "https://gitlab.com/org/service/-/issues"), true);

    const bb = HostedGitInfo.parse("bitbucket:acme/core#v3").?;
    expect_eq(bb.provider == GitProvider.bitbucket, true);
    expect_eq(std.mem.eql(u8, bb.provider.domain(), "bitbucket.org"), true);
    expect_eq(std.mem.eql(u8, bb.toTarball(buf[0..]).?, "https://bitbucket.org/acme/core/get/v3.tar.gz"), true);
    expect_eq(std.mem.eql(u8, bb.toBrowse(buf[0..]).?, "https://bitbucket.org/acme/core/src/v3"), true);
});

test("4. Gist shortcut & HTTPS URL with/without user + HTTPS auth preservation", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const gist = HostedGitInfo.parse("gist:1a2b3c4d#rev1").?;
    expect_eq(gist.provider == GitProvider.gist, true);
    expect_eq(gist.user == null, true);
    expect_eq(std.mem.eql(u8, gist.project, "1a2b3c4d"), true);
    expect_eq(std.mem.eql(u8, gist.toShortcut(buf[0..]).?, "gist:1a2b3c4d#rev1"), true);
    expect_eq(std.mem.eql(u8, gist.toTarball(buf[0..]).?, "https://codeload.github.com/gist/1a2b3c4d/tar.gz/rev1"), true);

    const authed = HostedGitInfo.parse("https://tok123@github.com/org/private.git").?;
    expect_eq(std.mem.eql(u8, authed.auth.?, "tok123"), true);
    expect_eq(std.mem.eql(u8, authed.toHttps(buf[0..]).?, "https://tok123@github.com/org/private.git"), true);
});

test("5. Invalid URLs, unknown hosts & buffer-too-small guards", () => {
    expect_eq(HostedGitInfo.parse("") == null, true);
    expect_eq(HostedGitInfo.parse("   ") == null, true);
    expect_eq(HostedGitInfo.parse("https://example.com/owner/repo") == null, true);
    expect_eq(HostedGitInfo.parse("https://github.com/onlyuser") == null, true);
    expect_eq(HostedGitInfo.parse("./local/path") == null, true);
    expect_eq(HostedGitInfo.parse("not-a-repo") == null, true);
    expect_eq(HostedGitInfo.parse("git@unknown.host:a/b.git") == null, true);

    const ok = HostedGitInfo.parse("github:a/b").?;
    let tiny: [4]u8 = [0, 0, 0, 0];
    expect_eq(ok.toHttps(tiny[0..]) == null, true);
    expect_eq(ok.toTarball(tiny[0..]) == null, true);
});
