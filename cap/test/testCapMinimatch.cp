import { test, expect_eq } from "cap:test";
import {
    MinimatchOptions, minimatch, minimatchWithOptions,
    isCommentPattern, isNegatedPattern, minimatchList,
    minimatchFilterAll, minimatchRejectAll
} from "../../packages/minimatch/src/index.cp";

fn main() {
    test("1. @cap/minimatch — Basic globs, globstar and brace ranges", () => {
        expect_eq(minimatch("bar.foo", "*.foo"), true);
        expect_eq(minimatch("bar.foo", "*.bar"), false);
        expect_eq(minimatch("src/a/b/main.cp", "src/**/*.cp"), true);
        expect_eq(minimatch("file-2.cp", "file-{1..3}.cp"), true);
        expect_eq(minimatch("file-4.cp", "file-{1..3}.cp"), false);
    });

    test("2. @cap/minimatch — Comments (#) and nocomment option", () => {
        let def = MinimatchOptions.defaults();
        expect_eq(isCommentPattern("# ignore me", def), true);
        expect_eq(minimatch("#foo", "#foo"), false);

        let nc = MinimatchOptions.defaults();
        nc.nocomment = true;
        expect_eq(isCommentPattern("#foo", nc), false);
        expect_eq(minimatchWithOptions("#foo", "#foo", nc), true);
    });

    test("3. @cap/minimatch — Negation (!), flip_negate, and nonegate", () => {
        let def = MinimatchOptions.defaults();
        expect_eq(isNegatedPattern("!*.log", def), true);
        expect_eq(minimatch("app.cp", "!*.log"), true);
        expect_eq(minimatch("err.log", "!*.log"), false);

        let flip = MinimatchOptions.defaults();
        flip.flip_negate = true;
        expect_eq(minimatchWithOptions("err.log", "!*.log", flip), true);
        expect_eq(minimatchWithOptions("app.cp", "!*.log", flip), false);

        let nn = MinimatchOptions.defaults();
        nn.nonegate = true;
        expect_eq(minimatchWithOptions("!foo", "!foo", nn), true);
        expect_eq(minimatchWithOptions("bar", "!foo", nn), false);
    });

    test("4. @cap/minimatch — match_base, dot, and nocase options", () => {
        let mb = MinimatchOptions.defaults();
        mb.match_base = true;
        expect_eq(minimatchWithOptions("a/b/c/test.cp", "*.cp", mb), true);

        let dot_opt = MinimatchOptions.defaults();
        expect_eq(minimatchWithOptions(".env", "*", dot_opt), false);
        dot_opt.dot = true;
        expect_eq(minimatchWithOptions(".env", "*", dot_opt), true);

        let ci = MinimatchOptions.defaults();
        ci.nocase = true;
        expect_eq(minimatchWithOptions("README.MD", "readme.md", ci), true);
    });

    test("5. @cap/minimatch — minimatchList filters matching indices", () => {
        let paths: [4]string = ["a.cp", "b.zig", "c.cp", "d.md"];
        let out: [4]usize = undefined;
        let n = minimatchList(paths[0..4], "*.cp", MinimatchOptions.defaults(), out[0..4]);
        expect_eq(n, 2);
        expect_eq(out[0], 0);
        expect_eq(out[1], 2);
    });

    test("6. @cap/minimatch — minimatchFilterAll and minimatchRejectAll", () => {
        let paths: [4]string = ["src/a.cp", "src/b.test.cp", "lib/c.cp", "README.md"];
        let pats: [2]string = ["**/*.cp", "!**/*.test.cp"];
        let keep: [4]usize = undefined;
        let drop: [4]usize = undefined;
        let nk = minimatchFilterAll(paths[0..4], pats[0..2], MinimatchOptions.defaults(), keep[0..4]);
        expect_eq(nk, 2);
        expect_eq(keep[0], 0);
        expect_eq(keep[1], 2);

        let nd = minimatchRejectAll(paths[0..4], pats[0..2], MinimatchOptions.defaults(), drop[0..4]);
        expect_eq(nd, 2);
        expect_eq(drop[0], 1);
        expect_eq(drop[1], 3);
    });
}
