import { test, expect } from "cap:test";
import { MatcherBehavior, PackageMatcher, wildcardMatch } from "cap:pkg";

test("pkg_matcher: all_include, all_exclude, and mixed semantics", () => {
    const inc_pats: [2][]const u8 = ["@babel/*", "eslint-plugin-*"];
    const inc = PackageMatcher.fromPatterns(inc_pats[0..]);
    expect(inc.behavior == MatcherBehavior.all_include);
    expect(inc.isMatch("@babel/core"));
    expect(inc.isMatch("eslint-plugin-react"));
    expect(!inc.isMatch("lodash"));

    const exc_pats: [2][]const u8 = ["!@types/*", "!typescript"];
    const exc = PackageMatcher.fromPatterns(exc_pats[0..]);
    expect(exc.behavior == MatcherBehavior.all_exclude);
    expect(exc.isMatch("react"));
    expect(!exc.isMatch("@types/node"));
    expect(!exc.isMatch("typescript"));

    const mix_pats: [3][]const u8 = ["@scope/*", "!@scope/internal-*", "cap-*"];
    const mix = PackageMatcher.fromPatterns(mix_pats[0..]);
    expect(mix.behavior == MatcherBehavior.mixed);
    expect(mix.isMatch("@scope/public"));
    expect(!mix.isMatch("@scope/internal-secret"));
    expect(mix.isMatch("cap-cli"));
    expect(!mix.isMatch("other"));
});

test("pkg_matcher: wildcardMatch edge cases and star-all", () => {
    expect(wildcardMatch("*", "anything"));
    expect(wildcardMatch("a*b*c", "axxxbyyyc"));
    expect(wildcardMatch("foo-?", "foo-1"));
    expect(!wildcardMatch("foo-?", "foo-12"));
    expect(wildcardMatch("****", ""));

    const all_pats: [2][]const u8 = ["*", "!bad-pkg"];
    const m = PackageMatcher.fromPatterns(all_pats[0..]);
    expect(m.isMatch("good-pkg"));
    expect(!m.isMatch("bad-pkg"));

    const empty = PackageMatcher.init();
    expect(!empty.isMatch("anything"));
});
