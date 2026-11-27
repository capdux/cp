import { test, expect_eq } from "cap:test";
import {
    FastGlobOptions,
    GlobPatternSet,
    isDynamicPattern,
    escapePath,
    convertPathToPattern
} from "../../packages/fast-glob/src/index.cp";

fn main() {
    test("fast-glob: isDynamicPattern detects wildcards, braces, classes, and negation", () => {
        expect_eq(isDynamicPattern("src/index.cp"), false);
        expect_eq(isDynamicPattern("a/b/c.txt"), false);
        expect_eq(isDynamicPattern("src/*.cp"), true);
        expect_eq(isDynamicPattern("src/**/*.cp"), true);
        expect_eq(isDynamicPattern("src/{a,b}.cp"), true);
        expect_eq(isDynamicPattern("src/file[0-9].cp"), true);
        expect_eq(isDynamicPattern("!dist/**"), true);
        expect_eq(isDynamicPattern("src/?.cp"), true);
    });

    test("fast-glob: escapePath escapes glob metacharacters into caller buffer", () => {
        let buf: [128]u8 = [0 as u8; 128];
        let n1 = escapePath("src/[id]/file*(1).cp", buf[0..128]);
        let s1: string = buf[0..n1];
        expect_eq(s1, "src/\\[id\\]/file\\*\\(1\\).cp");

        let n2 = escapePath("plain/path.txt", buf[0..128]);
        let s2: string = buf[0..n2];
        expect_eq(s2, "plain/path.txt");
    });

    test("fast-glob: convertPathToPattern normalizes Windows separators and escapes metas", () => {
        let buf: [128]u8 = [0 as u8; 128];
        let n = convertPathToPattern("C:\\Users\\dev\\[app]\\*.cp", buf[0..128]);
        let s: string = buf[0..n];
        expect_eq(s, "C:/Users/dev/\\[app\\]/\\*.cp");
    });

    test("fast-glob: GlobPatternSet matches positive patterns and excludes ! negation patterns", () => {
        let set = GlobPatternSet.new(FastGlobOptions.defaults());
        * = set.addPattern("src/**/*.{cp,ts}");
        * = set.addPattern("!src/**/*.test.cp");
        * = set.addIgnore("src/generated/**");

        expect_eq(set.include_count, 1 as usize);
        expect_eq(set.ignore_count, 2 as usize);

        expect_eq(set.matches("src/index.cp", false), true);
        expect_eq(set.matches("src/net/http.ts", false), true);
        expect_eq(set.matches("./src/core/app.cp", false), true);
        expect_eq(set.matches("src/core/app.test.cp", false), false);
        expect_eq(set.matches("src/generated/out.cp", false), false);
        expect_eq(set.matches("README.md", false), false);
    });

    test("fast-glob: enforces only_files, only_directories, dot, and deep options", () => {
        let opts = FastGlobOptions.defaults();
        opts.deep = 2;
        let set = GlobPatternSet.new(opts);
        * = set.addPattern("**/*");

        // only_files = true by default
        expect_eq(set.matches("src/main.cp", false), true);
        expect_eq(set.matches("src/sub", true), false);
        // deep = 2 rejects depth 3
        expect_eq(set.matches("src/sub/deep.cp", false), false);
        // dot = false rejects hidden segments
        expect_eq(set.matches(".env", false), false);
        expect_eq(set.matches("src/.hidden", false), false);

        // Allow dot files and directories only
        let dir_opts = FastGlobOptions.defaults();
        dir_opts.dot = true;
        dir_opts.only_files = false;
        dir_opts.only_directories = true;
        let dir_set = GlobPatternSet.new(dir_opts);
        * = dir_set.addPattern("**/*");

        expect_eq(dir_set.matches(".git", true), true);
        expect_eq(dir_set.matches(".git/config", false), false);
    });

    test("fast-glob: supports case_sensitive false matching without heap allocation", () => {
        let opts = FastGlobOptions.defaults();
        opts.case_sensitive = false;
        let set = GlobPatternSet.new(opts);
        * = set.addPattern("SRC/**/*.CP");
        * = set.addPattern("!src/Ignore/**");

        expect_eq(set.matches("src/App/Main.cp", false), true);
        expect_eq(set.matches("SRC/UTIL/HELPER.CP", false), true);
        expect_eq(set.matches("src/ignore/skip.cp", false), false);
    });
}
