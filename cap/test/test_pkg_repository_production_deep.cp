// cap/test/test_pkg_repository_production_deep.cp
// Production-deep regression tests for cap:pkg repository module
// Source: /data/dev/bun/src/install/repository.cp & /data/dev/bun/docs/pm/cli/add.mdx

import { test, expect } from "cap:test";
import {
    GIT_DEFAULT_ASKPASS,
    GIT_DEFAULT_SSH_COMMAND,
    GitRepoError,
    GitRepoHost,
    GitRepoSpec,
    isScpLikeGitPath,
    parseGitConfigFlags,
    resolveNonInteractiveGitEnv,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("pkg repository: parseAny / parseGit / parseGithub across all Git specifier styles") {
    const gh = GitRepoSpec.parseAny("github:oven-sh/bun#v1.3.0") catch { expect(false); return; };
    expect(gh.host == GitRepoHost.github);
    expect(strEq(gh.host.domainSuffix(), ".com"));
    expect(strEq(gh.owner, "oven-sh"));
    expect(strEq(gh.repo, "bun"));
    expect(strEq(gh.committish, "v1.3.0"));
    expect(strEq(gh.derivePackageName(), "bun"));

    const bb = GitRepoSpec.parseAny("bitbucket:team/pkg") catch { expect(false); return; };
    expect(bb.host == GitRepoHost.bitbucket);
    expect(strEq(bb.host.domainSuffix(), ".org"));
    expect(strEq(bb.owner, "team"));
    expect(strEq(bb.repo, "pkg"));
    expect(bb.committish.len == 0);

    const https = GitRepoSpec.parseAny("git+https://github.com/acme/sdk.git#main") catch { expect(false); return; };
    expect(https.host == GitRepoHost.generic);
    expect(strEq(https.repo, "https://github.com/acme/sdk.git"));
    expect(strEq(https.committish, "main"));
    expect(strEq(https.derivePackageName(), "sdk"));

    const scp = GitRepoSpec.parseAny("git@github.com:acme/private-lib.git#abcdef1") catch { expect(false); return; };
    expect(strEq(scp.repo, "git@github.com:acme/private-lib.git"));
    expect(strEq(scp.committish, "abcdef1"));
    expect(strEq(scp.derivePackageName(), "private-lib"));

    const bare = GitRepoSpec.parseAny("lodash/lodash#4.17.21") catch { expect(false); return; };
    expect(bare.host == GitRepoHost.github);
    expect(strEq(bare.owner, "lodash"));
    expect(strEq(bare.repo, "lodash"));
    expect(strEq(bare.committish, "4.17.21"));
}

test("pkg repository: SCP-like path detection boundaries") {
    expect(isScpLikeGitPath("git@github.com:oven-sh/bun.git"));
    expect(isScpLikeGitPath("host:repo"));
    expect(!isScpLikeGitPath("https://github.com/oven-sh/bun.git"));
    expect(!isScpLikeGitPath("ssh://git@github.com/oven-sh/bun.git"));
    expect(!isScpLikeGitPath("C:\\Users\\dev\\repo"));
    expect(!isScpLikeGitPath("/abs/path:with:colon"));
    expect(!isScpLikeGitPath(":leading"));
    expect(!isScpLikeGitPath("trailing:"));
    expect(!isScpLikeGitPath(""));
}

test("pkg repository: canonical & isolated store path formatting + round-trip") {
    let buf: [160]u8 = [0; 160];

    // Resolved field with `-<sha>` strips prefix before last `-`
    const s1 = GitRepoSpec{
        host: GitRepoHost.github,
        owner: "oven-sh",
        repo: "bun",
        committish: "main",
        resolved: "bun-1a2b3c4d",
    };
    const n1 = s1.formatCanonical("github:", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "github:oven-sh/bun#1a2b3c4d"));

    // Round-trip parse of formatted canonical string
    const rt = GitRepoSpec.parseGithub(buf[0..n1]) catch { expect(false); return; };
    expect(strEq(rt.owner, "oven-sh"));
    expect(strEq(rt.repo, "bun"));
    expect(strEq(rt.committish, "1a2b3c4d"));

    const sn1 = s1.formatStorePath("github:", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..sn1], "github+oven-sh+bun+1a2b3c4d"));

    // SCP path adds `ssh://` in canonical and `ssh++` in store path
    const s2 = GitRepoSpec{
        repo: "git@github.com:acme/pkg.git",
        committish: "v2",
    };
    const n2 = s2.formatCanonical("git+", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "git+ssh://git@github.com:acme/pkg.git#v2"));

    const sn2 = s2.formatStorePath("git+", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..sn2], "git+ssh++git@github.com+acme+pkg.git+v2"));
}

test("pkg repository: eql() & compare() semantics (resolved vs committish fallback)") {
    const a = GitRepoSpec{ owner: "org", repo: "lib", committish: "main", resolved: "lib-aaa" };
    const b = GitRepoSpec{ owner: "org", repo: "lib", committish: "main", resolved: "" };
    const c = GitRepoSpec{ owner: "org", repo: "lib", committish: "main", resolved: "lib-bbb" };
    const d = GitRepoSpec{ owner: "org", repo: "lib", committish: "v1", resolved: "" };

    // If either resolved is empty, compares committish
    expect(a.eql(b));
    // If both resolved are non-empty, compares resolved
    expect(!a.eql(c));
    expect(!b.eql(d));

    expect(a.compare(d) < 0); // "main" < "v1"
    expect(d.compare(a) > 0);
    expect(a.compare(b) == 0);
}

test("pkg repository: .gitconfig sloppy scanner & non-interactive env policy") {
    const cfg_text =
        "# global comment\n" ++
        "; semicolon comment\n" ++
        "[user]\n" ++
        "  name = Dev\n" ++
        "  askpass = ignored_outside_core\n" ++
        "[CORE]\n" ++
        "  sshCommand = ssh -i ~/.ssh/id_ed25519\n";

    const flags1 = parseGitConfigFlags(cfg_text);
    expect(!flags1.has_askpass);
    expect(flags1.has_ssh_command);

    const env1 = resolveNonInteractiveGitEnv(flags1, false, false);
    expect(env1.set_askpass);
    expect(strEq(env1.askpass_value, GIT_DEFAULT_ASKPASS));
    expect(!env1.set_ssh_command);

    // Top-level dotted `core.askpass=custom` outside [core]
    const flags2 = parseGitConfigFlags("core.askpass=/usr/bin/my-askpass\n");
    expect(flags2.has_askpass);
    expect(!flags2.has_ssh_command);

    // Existing env vars suppress overrides
    const env2 = resolveNonInteractiveGitEnv(flags2, false, true);
    expect(!env2.set_askpass);
    expect(!env2.set_ssh_command);

    // Default fallback injects both when neither config nor env specifies them
    const env3 = resolveNonInteractiveGitEnv(parseGitConfigFlags(""), false, false);
    expect(env3.set_askpass);
    expect(env3.set_ssh_command);
    expect(strEq(env3.ssh_command_value, GIT_DEFAULT_SSH_COMMAND));
}

test("pkg repository: malformed inputs & BufferTooSmall defenses") {
    let e1 = false;
    * = GitRepoSpec.parseAny("   ") catch |e| {
        expect(e == GitRepoError.EmptyInput);
        e1 = true;
    };
    expect(e1);

    let e2 = false;
    * = GitRepoSpec.parseGithub("github:onlyowner") catch |e| {
        expect(e == GitRepoError.InvalidShorthand);
        e2 = true;
    };
    expect(e2);

    let e3 = false;
    * = GitRepoSpec.parseGithub("github:owner/#main") catch |e| {
        expect(e == GitRepoError.MissingRepoName);
        e3 = true;
    };
    expect(e3);

    let e4 = false;
    * = GitRepoSpec.parseGit("git+") catch |e| {
        expect(e == GitRepoError.MissingRepoName);
        e4 = true;
    };
    expect(e4);

    const s = GitRepoSpec{ owner: "oven-sh", repo: "bun", committish: "main" };
    let tiny: [6]u8 = [0; 6];
    let e5 = false;
    * = s.formatCanonical("github:", tiny[0..]) catch |e| {
        expect(e == GitRepoError.BufferTooSmall);
        e5 = true;
    };
    expect(e5);
}
