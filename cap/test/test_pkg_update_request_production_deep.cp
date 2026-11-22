// Production Deep Tests for cap:pkg UpdateRequest (lib/pkg/update_request.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_UPDATE_REQUESTS,
    UpdateRequestError,
    PackageSubcommand,
    UpdateSpecKind,
    UpdateRequest,
    UpdateRequestList,
} from "cap:pkg";

test("pkg update_request: npm semver, dist-tag, bare package & scoped package parsing", fn() {
    const r1 = UpdateRequest.parse("  lodash@^4.17.21  ", .add) catch unreachable;
    expect_eq(r1.name, "lodash");
    expect_eq(r1.spec, "^4.17.21");
    expect_eq(r1.kind == .npm_semver, true);
    expect_eq(r1.is_aliased, true);
    expect_eq(r1.matchesName("lodash"), true);
    expect_eq(r1.matchesName("underscore"), false);

    const r2 = UpdateRequest.parse("@types/node@22.10.0", .add) catch unreachable;
    expect_eq(r2.name, "@types/node");
    expect_eq(r2.spec, "22.10.0");
    expect_eq(r2.kind == .npm_semver, true);

    const r3 = UpdateRequest.parse("react@canary", .add) catch unreachable;
    expect_eq(r3.name, "react");
    expect_eq(r3.spec, "canary");
    expect_eq(r3.kind == .dist_tag, true);

    const r4 = UpdateRequest.parse("zod", .add) catch unreachable;
    expect_eq(r4.name, "zod");
    expect_eq(r4.spec, "");
    expect_eq(r4.kind == .dist_tag, true);

    let fmt_buf: [64]u8 = undefined;
    expect_eq(r2.formatSpec(fmt_buf[0..]) catch unreachable, "@types/node@22.10.0");
    expect_eq(r4.formatSpec(fmt_buf[0..]) catch unreachable, "zod");
});

test("pkg update_request: npm alias, workspace:, file:, tarball, and git/github protocols", fn() {
    const alias_req = UpdateRequest.parse("my-lodash@npm:lodash@^4.17.0", .add) catch unreachable;
    expect_eq(alias_req.name, "my-lodash");
    expect_eq(alias_req.spec, "npm:lodash@^4.17.0");
    expect_eq(alias_req.kind == .npm_alias, true);

    const ws_req = UpdateRequest.parse("@acme/ui@workspace:*", .add) catch unreachable;
    expect_eq(ws_req.name, "@acme/ui");
    expect_eq(ws_req.spec, "workspace:*");
    expect_eq(ws_req.kind == .workspace, true);

    const file_req = UpdateRequest.parse("./packages/local-lib", .add) catch unreachable;
    expect_eq(file_req.kind == .file, true);
    expect_eq(file_req.is_aliased, false);
    expect_eq(file_req.getName(), "./packages/local-lib");

    const tgz_req = UpdateRequest.parse("https://registry.example.com/foo-1.0.0.tgz", .add) catch unreachable;
    expect_eq(tgz_req.kind == .tarball, true);
    expect_eq(tgz_req.is_aliased, false);

    const gh_req = UpdateRequest.parse("oven-sh/bun#main", .add) catch unreachable;
    expect_eq(gh_req.kind == .git, true);
    expect_eq(gh_req.spec, "oven-sh/bun#main");

    const git_req = UpdateRequest.parse("git+https://github.com/oven-sh/bun.git", .add) catch unreachable;
    expect_eq(git_req.kind == .git, true);
});

test("pkg update_request: link & unlink subcommand auto-normalization", fn() {
    const l1 = UpdateRequest.parse("my-cli", .link) catch unreachable;
    expect_eq(l1.name, "my-cli");
    expect_eq(l1.kind == .link, true);
    expect_eq(l1.is_aliased, true);

    let buf: [64]u8 = undefined;
    expect_eq(l1.formatSpec(buf[0..]) catch unreachable, "link:my-cli");

    const l2 = UpdateRequest.parse("link:@scope/tool", .unlink) catch unreachable;
    expect_eq(l2.name, "@scope/tool");
    expect_eq(l2.spec, "link:@scope/tool");
    expect_eq(l2.kind == .link, true);
    expect_eq(l2.formatSpec(buf[0..]) catch unreachable, "link:@scope/tool");
});

test("pkg update_request: UpdateRequestList batch parsing & first-wins deduplication", fn() {
    const args = [
        "lodash@^4.17.21",
        "@types/node@22.0.0",
        "lodash@latest",
        "zod",
        "@types/node@20.0.0",
    ];
    const list = UpdateRequestList.parseAll(args[0..], .add) catch unreachable;
    expect_eq(list.count(), 3);
    expect_eq(list.contains("lodash"), true);
    expect_eq(list.contains("@types/node"), true);
    expect_eq(list.contains("zod"), true);
    expect_eq(list.contains("react"), false);

    const items = list.items();
    // First occurrence wins on deduplication
    expect_eq(items[0].name, "lodash");
    expect_eq(items[0].spec, "^4.17.21");
    expect_eq(items[1].name, "@types/node");
    expect_eq(items[1].spec, "22.0.0");
    expect_eq(items[2].name, "zod");
});

test("pkg update_request: rejects empty input, trailing '@', empty 'npm:', and invalid names", fn() {
    let empty_err = false;
    * = UpdateRequest.parse("   ", .add) catch { empty_err = true; };
    expect_eq(empty_err, true);

    // Trailing '@' with empty spec
    let trailing_at = false;
    * = UpdateRequest.parse("lodash@", .add) catch { trailing_at = true; };
    expect_eq(trailing_at, true);

    // Empty `npm:` alias target
    let empty_alias = false;
    * = UpdateRequest.parse("foo@npm:", .add) catch { empty_alias = true; };
    expect_eq(empty_alias, true);

    // Invalid package name with uppercase/spaces
    let bad_name = false;
    * = UpdateRequest.parse("Bad Package Name", .add) catch { bad_name = true; };
    expect_eq(bad_name, true);

    // Invalid link target
    let bad_link = false;
    * = UpdateRequest.parse("link:", .link) catch { bad_link = true; };
    expect_eq(bad_link, true);
});

test("pkg update_request: buffer-too-small guard & capacity constant", fn() {
    expect_eq(MAX_UPDATE_REQUESTS, 64);

    const req = UpdateRequest.parse("@scope/very-long-package-name@^1.2.3", .add) catch unreachable;
    let tiny: [6]u8 = undefined;
    let oob = false;
    * = req.formatSpec(tiny[0..]) catch { oob = true; };
    expect_eq(oob, true);
});
