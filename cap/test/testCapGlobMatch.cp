import { test, expect_eq } from "cap:test";
import {
    GlobSpan,
    globMatch,
    globMatchAny,
    globMatchCaptures,
    isStaticGlob,
} from "../../packages/@cap/glob-match/src/index.cp";

fn testEqStr(a: string, b: string): bool {
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

fn main() {
    test("1. @cap/glob-match — Single-segment * and ? wildcards do not cross /", () => {
        expect_eq(globMatch("*.cp", "main.cp"), true);
        expect_eq(globMatch("*.cp", ".cp"), true);
        expect_eq(globMatch("*.cp", "src/main.cp"), false);
        expect_eq(globMatch("src/*.cp", "src/index.cp"), true);
        expect_eq(globMatch("src/*.cp", "src/sub/index.cp"), false);
        expect_eq(globMatch("file?.txt", "file1.txt"), true);
        expect_eq(globMatch("file?.txt", "file/.txt"), false);
        expect_eq(globMatch("file?.txt", "file12.txt"), false);
        expect_eq(globMatch("a*b*c", "axxxbyyyc"), true);
        expect_eq(globMatch("a*b*c", "ax/byc"), false);
    });

    test("2. @cap/glob-match — Cross-directory ** globstar matching", () => {
        expect_eq(globMatch("**", ""), true);
        expect_eq(globMatch("**", "a/b/c.cp"), true);
        expect_eq(globMatch("**/index.cp", "index.cp"), true);
        expect_eq(globMatch("**/index.cp", "src/index.cp"), true);
        expect_eq(globMatch("**/index.cp", "a/b/c/index.cp"), true);
        expect_eq(globMatch("**/index.cp", "xindex.cp"), false);
        expect_eq(globMatch("src/**/mod.cp", "src/mod.cp"), true);
        expect_eq(globMatch("src/**/mod.cp", "src/a/mod.cp"), true);
        expect_eq(globMatch("src/**/mod.cp", "src/a/b/c/mod.cp"), true);
        expect_eq(globMatch("src/**/mod.cp", "src/xmod.cp"), false);
        expect_eq(globMatch("src/**", "src/a/b/c.cp"), true);
        expect_eq(globMatch("src/**", "src/"), true);
        expect_eq(globMatch("src/**", "other/a.cp"), false);
    });

    test("3. @cap/glob-match — Character classes [abc], [a-z], [!0-9], [^0-9]", () => {
        expect_eq(globMatch("file[abc].cp", "filea.cp"), true);
        expect_eq(globMatch("file[abc].cp", "filec.cp"), true);
        expect_eq(globMatch("file[abc].cp", "filed.cp"), false);
        expect_eq(globMatch("v[0-9].cp", "v7.cp"), true);
        expect_eq(globMatch("v[0-9].cp", "va.cp"), false);
        expect_eq(globMatch("v[!0-9].cp", "va.cp"), true);
        expect_eq(globMatch("v[!0-9].cp", "v5.cp"), false);
        expect_eq(globMatch("v[^0-9].cp", "vz.cp"), true);
        expect_eq(globMatch("v[^0-9].cp", "v0.cp"), false);
        // Character classes must not match `/`.
        expect_eq(globMatch("a[!x]b", "a/b"), false);
        expect_eq(globMatch("a[^x]b", "a/b"), false);
    });

    test("4. @cap/glob-match — Brace alternation {rs,cp,zig} and backslash escaping", () => {
        expect_eq(globMatch("*.{rs,cp,zig}", "main.rs"), true);
        expect_eq(globMatch("*.{rs,cp,zig}", "main.cp"), true);
        expect_eq(globMatch("*.{rs,cp,zig}", "main.zig"), true);
        expect_eq(globMatch("*.{rs,cp,zig}", "main.py"), false);
        expect_eq(globMatch("{src,lib}/**/*.{cp,zig}", "src/net/http.cp"), true);
        expect_eq(globMatch("{src,lib}/**/*.{cp,zig}", "lib/core.zig"), true);
        expect_eq(globMatch("{src,lib}/**/*.{cp,zig}", "test/core.cp"), false);
        // Empty branch in braces
        expect_eq(globMatch("foo{,.cp}", "foo"), true);
        expect_eq(globMatch("foo{,.cp}", "foo.cp"), true);
        // Backslash escaping of wildcard metacharacters
        expect_eq(globMatch("a\\*b", "a*b"), true);
        expect_eq(globMatch("a\\*b", "axxb"), false);
        expect_eq(globMatch("a\\?b", "a?b"), true);
        expect_eq(globMatch("a\\?b", "axb"), false);
        expect_eq(globMatch("\\{a,b\\}", "{a,b}"), true);
        expect_eq(globMatch("\\[abc\\]", "[abc]"), true);
    });

    test("5. @cap/glob-match — globMatchAny and globMatchCaptures span extraction", () => {
        const pats: [3]string = ["**/*.cp", "Cargo.toml", "docs/*.md"];
        expect_eq(globMatchAny(pats[0..3], "src/a/b.cp"), true);
        expect_eq(globMatchAny(pats[0..3], "Cargo.toml"), true);
        expect_eq(globMatchAny(pats[0..3], "docs/guide.md"), true);
        expect_eq(globMatchAny(pats[0..3], "README.txt"), false);

        let caps: [8]GlobSpan = undefined;
        const p1: string = "packages/glob-match/src/v2.cp";
        const n1: usize = globMatchCaptures("packages/*/src/v?.cp", p1, caps[0..8]);
        expect_eq(n1, 2 as usize);
        expect_eq(testEqStr(p1[caps[0].start..caps[0].end], "glob-match"), true);
        expect_eq(testEqStr(p1[caps[1].start..caps[1].end], "2"), true);

        const p2: string = "a/x/y/b/file.cp";
        const n2: usize = globMatchCaptures("a/**/b/*.cp", p2, caps[0..8]);
        expect_eq(n2, 2 as usize);
        expect_eq(testEqStr(p2[caps[0].start..caps[0].end], "x/y"), true);
        expect_eq(testEqStr(p2[caps[1].start..caps[1].end], "file"), true);

        // Non-matching pattern returns 0 captures.
        const n3: usize = globMatchCaptures("packages/*/src/*.rs", p1, caps[0..8]);
        expect_eq(n3, 0 as usize);
    });

    test("6. @cap/glob-match — isStaticGlob and anti-backtracking stress resilience", () => {
        expect_eq(isStaticGlob(""), true);
        expect_eq(isStaticGlob("src/main.cp"), true);
        expect_eq(isStaticGlob("a\\*b\\?c\\[d\\]\\{e\\}"), true);
        expect_eq(isStaticGlob("src/*.cp"), false);
        expect_eq(isStaticGlob("src/a?b.cp"), false);
        expect_eq(isStaticGlob("src/[a-z].cp"), false);
        expect_eq(isStaticGlob("src/{a,b}.cp"), false);

        // Classic exponential-backtracking trap must finish instantly in linear time.
        const trap_pat: string = "a*a*a*a*a*a*a*a*a*a*a*a*b";
        const trap_str: string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac";
        expect_eq(globMatch(trap_pat, trap_str), false);

        const trap_dstar_pat: string = "a/**/a/**/a/**/a/**/a/**/b";
        const trap_dstar_str: string = "a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/c";
        expect_eq(globMatch(trap_dstar_pat, trap_dstar_str), false);
    });
}
