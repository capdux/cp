import { test, expect_eq } from "cap:test";
import {
    RimrafOptions,
    RimrafPlan,
    isRootPath,
    validateRimrafTarget,
    planBottomUpDelete,
    planFilteredDelete,
} from "../../packages/@cap/rimraf-core/src/index.cp";

fn eqStr(a: string, b: string): bool {
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
    test("1. @cap/rimraf-core — isRootPath detects POSIX, Windows drive, and empty root paths", () => {
        let opts: RimrafOptions = RimrafOptions.defaults();
        expect_eq(opts.preserve_root, true);

        expect_eq(isRootPath("/", false), true);
        expect_eq(isRootPath("///", false), true);
        expect_eq(isRootPath("", false), true);
        expect_eq(isRootPath("/tmp/build", false), false);

        expect_eq(isRootPath("C:\\", true), true);
        expect_eq(isRootPath("C:/", true), true);
        expect_eq(isRootPath("d:", true), true);
        expect_eq(isRootPath("\\\\", true), true);
        expect_eq(isRootPath("C:\\temp\\a", true), false);
        expect_eq(isRootPath("C:\\", false), false);
    });

    test("2. @cap/rimraf-core — validateRimrafTarget enforces preserve_root and rejects NUL bytes", () => {
        let safe_opts: RimrafOptions = RimrafOptions.defaults();
        expect_eq(validateRimrafTarget("/", safe_opts), false);
        expect_eq(validateRimrafTarget("", safe_opts), false);
        expect_eq(validateRimrafTarget("/tmp/a\x00b", safe_opts), false);
        expect_eq(validateRimrafTarget("/tmp/dist", safe_opts), true);

        let unsafe_opts: RimrafOptions = RimrafOptions.defaults();
        unsafe_opts.preserve_root = false;
        expect_eq(validateRimrafTarget("/", unsafe_opts), true);
        expect_eq(validateRimrafTarget("/tmp/a\x00b", unsafe_opts), false);
    });

    test("3. @cap/rimraf-core — planBottomUpDelete orders entries deepest-first", () => {
        let entries: [4]string = [
            "/app/dist",
            "/app/dist/assets/logo.png",
            "/app/dist/assets",
            "/app/dist/index.js",
        ];
        let plan: RimrafPlan = planBottomUpDelete(entries[0..4], RimrafOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 4);
        expect_eq(eqStr(plan.paths[0], "/app/dist/assets/logo.png"), true);
        expect_eq(eqStr(plan.paths[1], "/app/dist/assets"), true);
        expect_eq(eqStr(plan.paths[2], "/app/dist/index.js"), true);
        expect_eq(eqStr(plan.paths[3], "/app/dist"), true);
    });

    test("4. @cap/rimraf-core — planBottomUpDelete skips dangerous root and NUL entries", () => {
        let mixed: [4]string = ["/", "/tmp/cache", "//", "/tmp/cache/item.bin"];
        let plan1: RimrafPlan = planBottomUpDelete(mixed[0..4], RimrafOptions.defaults());
        expect_eq(plan1.valid, true);
        expect_eq(plan1.count, 2);
        expect_eq(eqStr(plan1.paths[0], "/tmp/cache/item.bin"), true);
        expect_eq(eqStr(plan1.paths[1], "/tmp/cache"), true);

        let only_roots: [2]string = ["/", ""];
        let plan2: RimrafPlan = planBottomUpDelete(only_roots[0..2], RimrafOptions.defaults());
        expect_eq(plan2.valid, false);
        expect_eq(plan2.count, 0);
    });

    test("5. @cap/rimraf-core — planFilteredDelete reuses @cap/fast-glob and orders deepest-first", () => {
        let entries: [5]string = [
            "dist/index.js",
            "dist/sub/deep/app.js",
            "dist/sub/style.css",
            "dist/sub/util.js",
            "README.md",
        ];
        let plan: RimrafPlan = planFilteredDelete(entries[0..5], "dist/**/*.js", RimrafOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 3);
        expect_eq(eqStr(plan.paths[0], "dist/sub/deep/app.js"), true);
        expect_eq(eqStr(plan.paths[1], "dist/sub/util.js"), true);
        expect_eq(eqStr(plan.paths[2], "dist/index.js"), true);
    });

    test("6. @cap/rimraf-core — planFilteredDelete handles case-insensitive Windows mode and empty pattern", () => {
        let win_opts: RimrafOptions = RimrafOptions.defaults();
        win_opts.is_windows = true;

        let entries: [3]string = ["build/TEMP.TMP", "build/nested/LOG.tmp", "build/keep.txt"];
        let plan: RimrafPlan = planFilteredDelete(entries[0..3], "build/**/*.tmp", win_opts);
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 2);
        expect_eq(eqStr(plan.paths[0], "build/nested/LOG.tmp"), true);
        expect_eq(eqStr(plan.paths[1], "build/TEMP.TMP"), true);

        let bad_plan: RimrafPlan = planFilteredDelete(entries[0..3], "", win_opts);
        expect_eq(bad_plan.valid, false);
        expect_eq(bad_plan.count, 0);
    });
}
