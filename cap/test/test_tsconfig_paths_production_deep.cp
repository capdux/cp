import { test, expect } from "cap:test";
import { expandConfigDir, TsConfigPaths } from "cap:tsconfig_paths";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("tsconfig_paths: longest-prefix wildcard, exact match, and fallback targets", () => {
    let out: [128]u8 = [0; 128];
    let paths = TsConfigPaths.init("./app");

    const t1: [2][]const u8 = ["./src/*", "./generated/*"];
    const t2: [1][]const u8 = ["./packages/ui/src/*"];
    const t3: [1][]const u8 = ["./vendor/lodash/index.ts"];
    expect(paths.addRule("@/*", t1[0..]));
    expect(paths.addRule("@/ui/*", t2[0..]));
    expect(paths.addRule("lodash", t3[0..]));

    // Multiple '*' in pattern rejected
    expect(!paths.addRule("a/*/b/*", t2[0..]));

    // Exact match
    expect(strEq(paths.resolve("lodash", out[0..]).?, "./app/vendor/lodash/index.ts"));

    // Longest prefix wins: '@/ui/button' matches '@/ui/*' over '@/*'
    expect(strEq(paths.resolve("@/ui/button", out[0..]).?, "./app/packages/ui/src/button"));

    // General wildcard with fallback target #1
    expect(strEq(paths.resolve("@/core/net", out[0..]).?, "./app/src/core/net"));
    expect(strEq(paths.resolveNth("@/core/net", 1, out[0..]).?, "./app/generated/core/net"));
    expect(paths.resolveNth("@/core/net", 2, out[0..]) == null);

    // Non-matching specifier returns null
    expect(paths.resolve("react", out[0..]) == null);
});

test("tsconfig_paths: expandConfigDir placeholder substitution", () => {
    let out: [128]u8 = [0; 128];
    const expanded = expandConfigDir("${configDir}/src/types:${configDir}/dist", "/repo/pkg", out[0..]).?;
    expect(strEq(expanded, "/repo/pkg/src/types:/repo/pkg/dist"));
});
