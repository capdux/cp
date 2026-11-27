import { test, expect_eq } from "cap:test";
import {
    MAKEDIR_DEFAULT_MODE, MakeDirOptions, isValidDirPath, planMakeDir, applyUmask
} from "../../packages/make-dir/src/index.cp";

fn main() {
    test("1. @cap/make-dir — plans all missing nested ancestor directories in order", () => {
        let existing: [1]string = ["/app"];
        let plan = planMakeDir("/app/a/b/c/", existing[0..1], MakeDirOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 3);
        expect_eq(plan.dirs[0], "/app/a");
        expect_eq(plan.dirs[1], "/app/a/b");
        expect_eq(plan.dirs[2], "/app/a/b/c");
    });

    test("2. @cap/make-dir — returns 0 steps when target already exists", () => {
        let existing: [2]string = ["/app", "/app/dist"];
        let plan = planMakeDir("/app/dist", existing[0..2], MakeDirOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 0);
    });

    test("3. @cap/make-dir — handles relative nested paths cleanly", () => {
        let existing: [1]string = ["build"];
        let plan = planMakeDir("build/linux/x64", existing[0..1], MakeDirOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 2);
        expect_eq(plan.dirs[0], "build/linux");
        expect_eq(plan.dirs[1], "build/linux/x64");
    });

    test("4. @cap/make-dir — Windows forbidden characters rejected", () => {
        expect_eq(isValidDirPath("C:/valid/dir", true), true);
        expect_eq(isValidDirPath("C:/bad|name", true), false);
        expect_eq(isValidDirPath("C:/bad?name", true), false);
        expect_eq(isValidDirPath("C:/a:b", true), false);
        expect_eq(isValidDirPath("", false), false);
    });

    test("5. @cap/make-dir — root '/' requires 0 creation steps", () => {
        let existing: [1]string = [""];
        let plan = planMakeDir("/", existing[0..0], MakeDirOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 0);
    });

    test("6. @cap/make-dir — applyUmask computes effective permissions (0o777 & ~0o022 = 0o755)", () => {
        expect_eq(MAKEDIR_DEFAULT_MODE, 511);
        expect_eq(applyUmask(511, 18), 493); // 0o755 == 493
        expect_eq(applyUmask(448, 18), 448); // 0o700 unchanged
    });
}
