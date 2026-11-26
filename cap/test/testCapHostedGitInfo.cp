import { test, expect_eq } from "cap:test";
import {
    HOST_GITHUB, HOST_GITLAB, HOST_BITBUCKET, fromUrl
} from "../../packages/@cap/hosted-git-info/src/index.cp";

fn main() {
    test("1. @cap/hosted-git-info — parses GitHub shorthand and formats HTTPS/SSH/browse/bugs", () => {
        let info = fromUrl("capdux/cp#main");
        expect_eq(info.valid, true);
        expect_eq(info.host_type, HOST_GITHUB);
        expect_eq(info.user, "capdux");
        expect_eq(info.project, "cp");
        expect_eq(info.committish, "main");

        let buf: [128]u8 = [0; 128];
        let n1 = info.formatHttps(buf[0..128]);
        expect_eq(buf[0..n1] as string, "git+https://github.com/capdux/cp.git#main");

        let n2 = info.formatSsh(buf[0..128]);
        expect_eq(buf[0..n2] as string, "git@github.com:capdux/cp.git#main");

        let n3 = info.formatBrowse(buf[0..128]);
        expect_eq(buf[0..n3] as string, "https://github.com/capdux/cp/tree/main");

        let n4 = info.formatBugs(buf[0..128]);
        expect_eq(buf[0..n4] as string, "https://github.com/capdux/cp/issues");
    });

    test("2. @cap/hosted-git-info — parses git+https:// and .git suffix stripping", () => {
        let info = fromUrl("git+https://github.com/rust-lang/rust.git");
        expect_eq(info.valid, true);
        expect_eq(info.user, "rust-lang");
        expect_eq(info.project, "rust");
        expect_eq(info.committish, "");
    });

    test("3. @cap/hosted-git-info — parses GitLab prefix and SSH syntax", () => {
        let a = fromUrl("gitlab:org/service#v2");
        expect_eq(a.valid, true);
        expect_eq(a.host_type, HOST_GITLAB);
        expect_eq(a.domain, "gitlab.com");
        expect_eq(a.committish, "v2");

        let b = fromUrl("git@gitlab.com:org/service.git");
        expect_eq(b.valid, true);
        expect_eq(b.project, "service");
    });

    test("4. @cap/hosted-git-info — parses Bitbucket prefix and HTTPS URL", () => {
        let a = fromUrl("bitbucket:team/app");
        expect_eq(a.valid, true);
        expect_eq(a.host_type, HOST_BITBUCKET);
        expect_eq(a.domain, "bitbucket.org");

        let b = fromUrl("https://bitbucket.org/team/app.git");
        expect_eq(b.valid, true);
        expect_eq(b.user, "team");
        expect_eq(b.project, "app");
    });

    test("5. @cap/hosted-git-info — parses git:// protocol", () => {
        let info = fromUrl("git://github.com/foo/bar.git");
        expect_eq(info.valid, true);
        expect_eq(info.user, "foo");
        expect_eq(info.project, "bar");
    });

    test("6. @cap/hosted-git-info — rejects unknown hosts, empty strings, and malformed paths", () => {
        expect_eq(fromUrl("").valid, false);
        expect_eq(fromUrl("https://example.com/a/b").valid, false);
        expect_eq(fromUrl("github:onlyuser").valid, false);
        expect_eq(fromUrl("github:a/b/c").valid, false);
    });
}
