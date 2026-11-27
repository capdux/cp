import { test, expect_eq } from "cap:test";
import {
    MAKEDIR_DEFAULT_MODE,
    MakeDirOptions,
    isValidDirPath,
    planMakeDir,
    makeDirPlan,
    applyUmask,
    validateDirPath,
    normalizeDirPath,
    iterParentDirs,
    collectParentDirs,
    DIR_PATH_OK,
    DIR_PATH_ERR_EMPTY,
    DIR_PATH_ERR_INVALID_CHAR,
    DIR_PATH_ERR_INVALID_COLON,
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

    test("7. @cap/make-dir — validateDirPath & normalizeDirPath", () => {
        const ok = validateDirPath("C:/work/proj", true);
        expect_eq(ok.valid, true);
        expect_eq(ok.err_code, DIR_PATH_OK);

        const empty = validateDirPath("", false);
        expect_eq(empty.valid, false);
        expect_eq(empty.err_code, DIR_PATH_ERR_EMPTY);

        const bad_ch = validateDirPath("C:/a*b", true);
        expect_eq(bad_ch.valid, false);
        expect_eq(bad_ch.err_code, DIR_PATH_ERR_INVALID_CHAR);
        expect_eq(bad_ch.bad_index, 4 as usize);

        const bad_col = validateDirPath("C:/a:b", true);
        expect_eq(bad_col.valid, false);
        expect_eq(bad_col.err_code, DIR_PATH_ERR_INVALID_COLON);

        let buf: [64]u8 = undefined;
        const n1 = normalizeDirPath("C:\\a\\\\b\\c\\", true, buf[0..64]);
        expect_eq(buf[0..n1] as string, "C:/a/b/c");
        const n2 = normalizeDirPath("C:\\\\", true, buf[0..64]);
        expect_eq(buf[0..n2] as string, "C:/");
        const n3 = normalizeDirPath("///foo//bar/", false, buf[0..64]);
        expect_eq(buf[0..n3] as string, "/foo/bar");
    });

    test("8. @cap/make-dir — makeDirPlan alias, iterParentDirs & collectParentDirs", () => {
        let existing: [1]string = ["/var"];
        const plan = makeDirPlan("/var/log/app", existing[0..1], MakeDirOptions.defaults());
        expect_eq(plan.valid, true);
        expect_eq(plan.count, 2 as usize);
        expect_eq(plan.dirs[0], "/var/log");
        expect_eq(plan.dirs[1], "/var/log/app");

        let it = iterParentDirs("/a/b/c/");
        expect_eq(it.next() ?? "", "/a");
        expect_eq(it.next() ?? "", "/a/b");
        expect_eq(it.next() ?? "", "/a/b/c");
        expect_eq(it.next() == null, true);

        let out: [4]string = [""; 4];
        const cnt = collectParentDirs("x/y/z", out[0..4]);
        expect_eq(cnt, 3 as usize);
        expect_eq(out[0], "x");
        expect_eq(out[1], "x/y");
        expect_eq(out[2], "x/y/z");
    });
}
