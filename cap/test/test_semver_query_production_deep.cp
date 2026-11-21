import { test, expect_eq } from "cap:test";
import {
    DependencyKind,
    WorkspaceSpecKind,
    DependencySpec,
    PackageName,
} from "cap:semver_query";

test("1. npm semver range & dist-tag resolution", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const s1 = DependencySpec.parse("^1.2.3");
    expect_eq(s1.kind == DependencyKind.npm, true);
    expect_eq(s1.isLocal(), false);
    expect_eq(std.mem.eql(u8, s1.toCanonical(buf[0..]), "^1.2.3"), true);

    const s2 = DependencySpec.parse(">=2.0.0 <3.0.0");
    expect_eq(s2.kind == DependencyKind.npm, true);

    const s_empty = DependencySpec.parse("");
    expect_eq(s_empty.kind == DependencyKind.npm, true);
    expect_eq(std.mem.eql(u8, s_empty.value, "*"), true);

    const tag = DependencySpec.parse("latest");
    expect_eq(tag.kind == DependencyKind.dist_tag, true);
    expect_eq(std.mem.eql(u8, tag.value, "latest"), true);
});

test("2. workspace: (*, ^, ~, semver) & catalog: protocol parsing", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const w_star = DependencySpec.parse("workspace:*");
    expect_eq(w_star.isWorkspace(), true);
    expect_eq(w_star.isLocal(), true);
    expect_eq(w_star.workspace.?.kind == WorkspaceSpecKind.star, true);
    expect_eq(std.mem.eql(u8, w_star.toCanonical(buf[0..]), "workspace:*"), true);

    const w_caret = DependencySpec.parse("workspace:^");
    expect_eq(w_caret.workspace.?.kind == WorkspaceSpecKind.caret, true);

    const w_tilde = DependencySpec.parse("workspace:~");
    expect_eq(w_tilde.workspace.?.kind == WorkspaceSpecKind.tilde, true);

    const w_ver = DependencySpec.parse("workspace:^1.0.0");
    expect_eq(w_ver.workspace.?.kind == WorkspaceSpecKind.semver, true);
    expect_eq(std.mem.eql(u8, w_ver.workspace.?.version, "^1.0.0"), true);

    const c_def = DependencySpec.parse("catalog:");
    expect_eq(c_def.kind == DependencyKind.catalog, true);
    expect_eq(std.mem.eql(u8, c_def.value, "default"), true);
    expect_eq(std.mem.eql(u8, c_def.toCanonical(buf[0..]), "catalog:default"), true);

    const c_named = DependencySpec.parse("catalog:react19");
    expect_eq(std.mem.eql(u8, c_named.value, "react19"), true);
});

test("3. npm: alias (scoped & unscoped), github: & git+ URLs", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const alias_scoped = DependencySpec.parse("npm:@scope/pkg@^2.0.0");
    expect_eq(alias_scoped.kind == DependencyKind.npm_alias, true);
    expect_eq(std.mem.eql(u8, alias_scoped.npm_alias.?.alias_name, "@scope/pkg"), true);
    expect_eq(std.mem.eql(u8, alias_scoped.npm_alias.?.version_spec, "^2.0.0"), true);
    expect_eq(std.mem.eql(u8, alias_scoped.toCanonical(buf[0..]), "npm:@scope/pkg@^2.0.0"), true);

    const gh = DependencySpec.parse("github:oven-sh/bun#main");
    expect_eq(gh.kind == DependencyKind.github, true);
    expect_eq(gh.isGit(), true);
    expect_eq(std.mem.eql(u8, gh.git.?.owner, "oven-sh"), true);
    expect_eq(std.mem.eql(u8, gh.git.?.repo, "bun"), true);
    expect_eq(std.mem.eql(u8, gh.git.?.committish.?, "main"), true);
    expect_eq(std.mem.eql(u8, gh.toCanonical(buf[0..]), "github:oven-sh/bun#main"), true);

    // GitHub shorthand
    const gh_short = DependencySpec.parse("cp-lang/cp#v1.0");
    expect_eq(gh_short.kind == DependencyKind.github, true);
    expect_eq(std.mem.eql(u8, gh_short.git.?.owner, "cp-lang"), true);
    expect_eq(std.mem.eql(u8, gh_short.git.?.repo, "cp"), true);

    const git_url = DependencySpec.parse("git+https://github.com/oven-sh/bun.git#release");
    expect_eq(git_url.kind == DependencyKind.git, true);
    expect_eq(git_url.isGit(), true);
    expect_eq(std.mem.eql(u8, git_url.git.?.committish.?, "release"), true);
});

test("4. file:, link:, local folder & tarball URLs", () => {
    const f = DependencySpec.parse("file:../local");
    expect_eq(f.kind == DependencyKind.file, true);
    expect_eq(f.isLocal(), true);
    expect_eq(std.mem.eql(u8, f.value, "../local"), true);

    const l = DependencySpec.parse("link:./pkg");
    expect_eq(l.kind == DependencyKind.link, true);
    expect_eq(l.isLocal(), true);
    expect_eq(std.mem.eql(u8, l.value, "./pkg"), true);

    const d = DependencySpec.parse("../sibling-pkg");
    expect_eq(d.kind == DependencyKind.folder, true);
    expect_eq(d.isLocal(), true);

    const tgz = DependencySpec.parse("https://registry.npmjs.org/zod/-/zod-3.23.0.tgz");
    expect_eq(tgz.kind == DependencyKind.tarball, true);
    expect_eq(tgz.isLocal(), false);
    expect_eq(tgz.isGit(), false);
});

test("5. PackageName validation & scoped/unscoped parsing boundaries", () => {
    const scoped = PackageName.parse("@cp-lang/http_client").?;
    expect_eq(scoped.isScoped(), true);
    expect_eq(std.mem.eql(u8, scoped.scope.?, "cp-lang"), true);
    expect_eq(std.mem.eql(u8, scoped.name, "http_client"), true);

    const plain = PackageName.parse("lodash.debounce").?;
    expect_eq(plain.isScoped(), false);
    expect_eq(plain.scope == null, true);
    expect_eq(std.mem.eql(u8, plain.name, "lodash.debounce"), true);

    // Invalid names: empty, uppercase, space, leading dot/underscore, malformed scope, >214 chars
    expect_eq(PackageName.isValid(""), false);
    expect_eq(PackageName.isValid("MyPkg"), false);
    expect_eq(PackageName.isValid("has space"), false);
    expect_eq(PackageName.isValid(".hidden"), false);
    expect_eq(PackageName.isValid("_private"), false);
    expect_eq(PackageName.isValid("@scope"), false);
    expect_eq(PackageName.isValid("@scope/a/b"), false);
    expect_eq(PackageName.isValid("@/name"), false);

    let too_long: [215]u8 = std.mem.zeroes([215]u8);
    let i: usize = 0;
    while (i < 215) {
        too_long[i] = 'a';
        i += 1;
    }
    expect_eq(PackageName.isValid(too_long[0..214]), true);
    expect_eq(PackageName.isValid(too_long[0..215]), false);
});
