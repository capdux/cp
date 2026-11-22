// Deep production regression tests for cap:cli pmLink.cp
// Covers: NPM link package name validation (scoped/unscoped, traversal & uppercase guards),
// CLI invocation parsing (RegisterLocal, LinkIntoProject with --save, UnlinkLocal),
// POSIX symlink vs Windows junction global link planning, trailing slash normalization,
// `"link:<pkg>"` specifier formatting, and success message rendering.

import { test, expect, expectEq } from "cap:test";
import {
    PmLinkMode,
    PmLinkMethod,
    isValidNpmLinkName,
    extractLinkScope,
    trimTrailingPathSep,
    formatLinkDependencySpec,
    formatLinkSuccessMessage,
    PmLinkInvocation,
    PmGlobalLinkPlan,
} from "cap:cli";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0 || haystack.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEq(haystack[i .. i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("pmLink: validates scoped and unscoped NPM names and rejects malformed/traversal names", fn () {
    expect(isValidNpmLinkName("cool-pkg"));
    expect(isValidNpmLinkName("@acme/design-system"));
    expect(isValidNpmLinkName("lodash.debounce"));

    expect(!isValidNpmLinkName(""));
    expect(!isValidNpmLinkName("UpperCase"));
    expect(!isValidNpmLinkName("../escape"));
    expect(!isValidNpmLinkName("@scope/.."));
    expect(!isValidNpmLinkName("@scope/a/b"));
    expect(!isValidNpmLinkName("@/pkg"));
    expect(!isValidNpmLinkName("bad name"));

    expect(strEq(extractLinkScope("@acme/ui").?, "@acme"));
    expect(extractLinkScope("cool-pkg") == null);
});

test("pmLink: parses RegisterLocal, LinkIntoProject (--save), and UnlinkLocal CLI args", fn () {
    const empty: [0][]const u8 = [];
    const reg = PmLinkInvocation.parseArgs(empty[0..], false) catch unreachable;
    expectEq(reg.mode, PmLinkMode.RegisterLocal);
    expectEq(reg.targetSlice().len, @as(usize, 0));

    const link_args: [3][]const u8 = ["--save", "cool-pkg", "@acme/ui"];
    const link_inv = PmLinkInvocation.parseArgs(link_args[0..], false) catch unreachable;
    expectEq(link_inv.mode, PmLinkMode.LinkIntoProject);
    expect(link_inv.save_to_pkg_json);
    expectEq(link_inv.targetSlice().len, @as(usize, 2));
    expect(strEq(link_inv.targetSlice()[0], "cool-pkg"));
    expect(strEq(link_inv.targetSlice()[1], "@acme/ui"));

    const unlink_inv = PmLinkInvocation.parseArgs(empty[0..], true) catch unreachable;
    expectEq(unlink_inv.mode, PmLinkMode.UnlinkLocal);
});

test("pmLink: plans POSIX symlink and Windows junction paths with scope directory detection", fn () {
    const posix_plan = PmGlobalLinkPlan.planRegister(
        "@acme/ui",
        "/home/dev/acme-ui/",
        "/home/dev/.bun/install/global/",
        false,
        true,
    ) catch unreachable;
    expectEq(posix_plan.method, PmLinkMethod.PosixSymlink);
    expect(posix_plan.needsScopeMkdir());
    expect(posix_plan.has_global_bins);
    expect(strEq(posix_plan.sourceDir(), "/home/dev/acme-ui"));
    expect(strEq(posix_plan.destLinkPath(), "/home/dev/.bun/install/global/node_modules/@acme/ui"));

    const win_plan = PmGlobalLinkPlan.planRegister(
        "@acme/ui",
        "C:\\dev\\ui\\",
        "C:\\Users\\dev\\.bun\\install\\global\\",
        true,
        false,
    ) catch unreachable;
    expectEq(win_plan.method, PmLinkMethod.WindowsJunction);
    expect(strEq(win_plan.sourceDir(), "C:\\dev\\ui"));
    expect(strEq(win_plan.destLinkPath(), "C:\\Users\\dev\\.bun\\install\\global\\node_modules\\@acme\\ui"));
});

test("pmLink: trimTrailingPathSep preserves POSIX root and Windows drive root", fn () {
    expect(strEq(trimTrailingPathSep("/"), "/"));
    expect(strEq(trimTrailingPathSep("/tmp/pkg///"), "/tmp/pkg"));
    expect(strEq(trimTrailingPathSep("C:\\"), "C:\\"));
    expect(strEq(trimTrailingPathSep("C:\\work\\app\\"), "C:\\work\\app"));
});

test("pmLink: formats canonical link:<pkg> spec and Bun registration guidance message", fn () {
    let spec_buf: [64]u8 = [0; 64];
    const sn = formatLinkDependencySpec("cool-pkg", spec_buf[0..]) catch unreachable;
    expect(strEq(spec_buf[0..sn], "link:cool-pkg"));

    let msg_buf: [256]u8 = [0; 256];
    const mn = formatLinkSuccessMessage("cool-pkg", msg_buf[0..]) catch unreachable;
    const msg = msg_buf[0..mn];
    expect(containsStr(msg, "Success! Registered \"cool-pkg\""));
    expect(containsStr(msg, "bun link cool-pkg"));
    expect(containsStr(msg, "\"cool-pkg\": \"link:cool-pkg\""));
});

test("pmLink: rejects invalid package names, empty directories, and undersized output buffers", fn () {
    const bad_args: [1][]const u8 = ["../evil"];
    let err_bad_arg = false;
    * = PmLinkInvocation.parseArgs(bad_args[0..], false) catch {
        err_bad_arg = true;
    };
    expect(err_bad_arg);

    let err_empty_dir = false;
    * = PmGlobalLinkPlan.planRegister("cool-pkg", "", "/global", false, false) catch {
        err_empty_dir = true;
    };
    expect(err_empty_dir);

    let tiny: [4]u8 = [0; 4];
    let err_tiny = false;
    * = formatLinkDependencySpec("cool-pkg", tiny[0..]) catch {
        err_tiny = true;
    };
    expect(err_tiny);
});
