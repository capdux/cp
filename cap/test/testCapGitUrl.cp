// cap/test/testCapGitUrl.cp
// Comprehensive unit tests for @cap/git-url

import { test, expect_eq } from "cap:test";
import {
    GitProtocol,
    GitHostKind,
    ParsedGitUrl,
    parseGitUrl,
    toHttpsUrl,
    toSshUrl,
    toBrowseUrl,
} from "../../packages/@cap/git-url/src/index.cp";

fn main() {
    test("1. @cap/git-url — SCP SSH & nested GitLab groups", () => {
        const gh = parseGitUrl("git@github.com:capdux/cap.git");
        expect_eq(gh.valid, true);
        expect_eq(gh.protocol == GitProtocol.Ssh, true);
        expect_eq(gh.host_kind == GitHostKind.GitHub, true);
        expect_eq(gh.user, "git");
        expect_eq(gh.host, "github.com");
        expect_eq(gh.owner, "capdux");
        expect_eq(gh.repo, "cap");

        const gl = parseGitUrl("git@gitlab.com:group/sub/repo.git");
        expect_eq(gl.valid, true);
        expect_eq(gl.host_kind == GitHostKind.GitLab, true);
        expect_eq(gl.owner, "group/sub");
        expect_eq(gl.repo, "repo");
    });

    test("2. @cap/git-url — HTTPS, git+https#ref & SSH with custom port", () => {
        const https = parseGitUrl("git+https://github.com/capdux/cap.git#v1.0.0");
        expect_eq(https.valid, true);
        expect_eq(https.protocol == GitProtocol.Https, true);
        expect_eq(https.host_kind == GitHostKind.GitHub, true);
        expect_eq(https.owner, "capdux");
        expect_eq(https.repo, "cap");
        expect_eq(https.ref_name, "v1.0.0");

        const ssh_port = parseGitUrl("ssh://git@github.com:2222/capdux/cap.git");
        expect_eq(ssh_port.valid, true);
        expect_eq(ssh_port.protocol == GitProtocol.Ssh, true);
        expect_eq(ssh_port.user, "git");
        expect_eq(ssh_port.host, "github.com");
        expect_eq(ssh_port.port, 2222 as u16);
        expect_eq(ssh_port.owner, "capdux");
        expect_eq(ssh_port.repo, "cap");
    });

    test("3. @cap/git-url — Hosted shorthands (github/gitlab/bitbucket/gitee/bare)", () => {
        const s1 = parseGitUrl("github:capdux/cap");
        expect_eq(s1.valid, true);
        expect_eq(s1.protocol == GitProtocol.Shorthand, true);
        expect_eq(s1.host_kind == GitHostKind.GitHub, true);
        expect_eq(s1.host, "github.com");
        expect_eq(s1.owner, "capdux");
        expect_eq(s1.repo, "cap");

        const s2 = parseGitUrl("gitlab:org/repo#main");
        expect_eq(s2.valid, true);
        expect_eq(s2.host_kind == GitHostKind.GitLab, true);
        expect_eq(s2.host, "gitlab.com");
        expect_eq(s2.ref_name, "main");

        const s3 = parseGitUrl("bitbucket:team/pkg");
        expect_eq(s3.valid, true);
        expect_eq(s3.host_kind == GitHostKind.Bitbucket, true);
        expect_eq(s3.host, "bitbucket.org");

        const s4 = parseGitUrl("gitee:org/repo");
        expect_eq(s4.valid, true);
        expect_eq(s4.host_kind == GitHostKind.Gitee, true);
        expect_eq(s4.host, "gitee.com");

        const bare = parseGitUrl("capdux/cap");
        expect_eq(bare.valid, true);
        expect_eq(bare.protocol == GitProtocol.Shorthand, true);
        expect_eq(bare.host_kind == GitHostKind.GitHub, true);
        expect_eq(bare.owner, "capdux");
        expect_eq(bare.repo, "cap");
    });

    test("4. @cap/git-url — Browser URL with /tree/<ref>/<subpath>", () => {
        const p = parseGitUrl("https://github.com/capdux/cap/tree/main/packages/@cap");
        expect_eq(p.valid, true);
        expect_eq(p.owner, "capdux");
        expect_eq(p.repo, "cap");
        expect_eq(p.ref_name, "main");
        expect_eq(p.subpath, "packages/@cap");
    });

    test("5. @cap/git-url — Normalizers (toHttpsUrl, toSshUrl, toBrowseUrl)", () => {
        let out: [256]u8 = undefined;

        const p = parseGitUrl("gitlab:org/sub/repo#release-2");
        expect_eq(toHttpsUrl(p, out[0..256]), "https://gitlab.com/org/sub/repo.git");
        expect_eq(toSshUrl(p, out[0..256]), "git@gitlab.com:org/sub/repo.git");
        expect_eq(toBrowseUrl(p, out[0..256]), "https://gitlab.com/org/sub/repo/tree/release-2");

        const custom_ssh = parseGitUrl("ssh://deploy@git.corp.io:2222/team/app.git");
        expect_eq(toSshUrl(custom_ssh, out[0..256]), "ssh://deploy@git.corp.io:2222/team/app.git");
        expect_eq(toBrowseUrl(custom_ssh, out[0..256]), "https://git.corp.io/team/app");
    });

    test("6. @cap/git-url — Invalid & malformed URLs rejected", () => {
        expect_eq(parseGitUrl("").valid, false);
        expect_eq(parseGitUrl("not-a-repo").valid, false);
        expect_eq(parseGitUrl("ftp://github.com/capdux/cap.git").valid, false);
        expect_eq(parseGitUrl("https://github.com/onlyowner").valid, false);
        expect_eq(parseGitUrl("ssh://git@github.com:99999/capdux/cap.git").valid, false);
        expect_eq(parseGitUrl("./local/relative").valid, false);
        expect_eq(parseGitUrl("has spaces/repo").valid, false);
    });
}
