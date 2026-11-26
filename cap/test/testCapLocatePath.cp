import { test, expect_eq } from "cap:test";
import {
    ENTRY_ANY,
    ENTRY_FILE,
    ENTRY_DIR,
    LocateOptions,
    FsEntry,
    isAbsolutePath,
    locatePathInEntries,
    locateAllPathsInEntries,
} from "../../packages/@cap/locate-path/src/index.cp";

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
    test("1. @cap/locate-path — isAbsolutePath detects POSIX, Windows drive, and UNC paths", () => {
        expect_eq(isAbsolutePath("/usr/local/bin"), true);
        expect_eq(isAbsolutePath("C:/Users/dev"), true);
        expect_eq(isAbsolutePath("d:\\work\\cp"), true);
        expect_eq(isAbsolutePath("\\\\server\\share"), true);
        expect_eq(isAbsolutePath("src/index.cp"), false);
        expect_eq(isAbsolutePath("./cap.json"), false);
        expect_eq(isAbsolutePath(""), false);
    });

    test("2. @cap/locate-path — locatePathInEntries resolves relative and absolute candidates in order", () => {
        let candidates: [3]string = ["missing.cp", "./cap.json", "/etc/hosts"];
        let entries: [2]FsEntry = [
            FsEntry { path: "/etc/hosts", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/app/cap.json", kind: ENTRY_FILE, is_symlink: false },
        ];
        let opts: LocateOptions = LocateOptions.defaults("/app");
        let out: [128]u8 = undefined;

        let n_opt: usize? = locatePathInEntries(candidates[0..3], entries[0..2], opts, out[0..128]);
        expect_eq(n_opt != null, true);
        let n: usize = n_opt ?? 0;
        let found: string = out[0..n];
        expect_eq(eqStr(found, "/app/cap.json"), true);
    });

    test("3. @cap/locate-path — entry_type filters ENTRY_FILE, ENTRY_DIR, and ENTRY_ANY", () => {
        let candidates: [2]string = ["dist", "README.md"];
        let entries: [2]FsEntry = [
            FsEntry { path: "/proj/dist", kind: ENTRY_DIR, is_symlink: false },
            FsEntry { path: "/proj/README.md", kind: ENTRY_FILE, is_symlink: false },
        ];
        let out: [128]u8 = undefined;

        // Default is ENTRY_FILE, so `dist` directory is skipped and `README.md` is matched.
        let file_opts: LocateOptions = LocateOptions.defaults("/proj");
        let n1: usize = locatePathInEntries(candidates[0..2], entries[0..2], file_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n1], "/proj/README.md"), true);

        // ENTRY_DIR matches `dist`.
        let dir_opts: LocateOptions = LocateOptions {
            cwd: "/proj",
            entry_type: ENTRY_DIR,
            allow_symlinks: true,
            preserve_order: true,
        };
        let n2: usize = locatePathInEntries(candidates[0..2], entries[0..2], dir_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n2], "/proj/dist"), true);

        // ENTRY_ANY matches first candidate `dist`.
        let any_opts: LocateOptions = LocateOptions {
            cwd: "/proj",
            entry_type: ENTRY_ANY,
            allow_symlinks: true,
            preserve_order: true,
        };
        let n3: usize = locatePathInEntries(candidates[0..2], entries[0..2], any_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n3], "/proj/dist"), true);
    });

    test("4. @cap/locate-path — allow_symlinks controls symlink matching", () => {
        let candidates: [2]string = ["link.cp", "real.cp"];
        let entries: [2]FsEntry = [
            FsEntry { path: "/work/link.cp", kind: ENTRY_FILE, is_symlink: true },
            FsEntry { path: "/work/real.cp", kind: ENTRY_FILE, is_symlink: false },
        ];
        let out: [128]u8 = undefined;

        let allow_opts: LocateOptions = LocateOptions.defaults("/work");
        let n1: usize = locatePathInEntries(candidates[0..2], entries[0..2], allow_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n1], "/work/link.cp"), true);

        let deny_opts: LocateOptions = LocateOptions {
            cwd: "/work",
            entry_type: ENTRY_FILE,
            allow_symlinks: false,
            preserve_order: true,
        };
        let n2: usize = locatePathInEntries(candidates[0..2], entries[0..2], deny_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n2], "/work/real.cp"), true);
    });

    test("5. @cap/locate-path — preserve_order false prefers earliest matching entry", () => {
        let candidates: [2]string = ["first.cp", "second.cp"];
        let entries: [2]FsEntry = [
            FsEntry { path: "/repo/second.cp", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/repo/first.cp", kind: ENTRY_FILE, is_symlink: false },
        ];
        let out: [128]u8 = undefined;

        let unord_opts: LocateOptions = LocateOptions {
            cwd: "/repo",
            entry_type: ENTRY_FILE,
            allow_symlinks: true,
            preserve_order: false,
        };
        let n: usize = locatePathInEntries(candidates[0..2], entries[0..2], unord_opts, out[0..128]) ?? 0;
        expect_eq(eqStr(out[0..n], "/repo/second.cp"), true);
    });

    test("6. @cap/locate-path — locateAllPathsInEntries collects all matching candidate indices", () => {
        let candidates: [4]string = ["a.cp", "b.cp", "c.cp", "d.cp"];
        let entries: [3]FsEntry = [
            FsEntry { path: "/pkg/a.cp", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/pkg/b.cp", kind: ENTRY_DIR, is_symlink: false },
            FsEntry { path: "/pkg/d.cp", kind: ENTRY_FILE, is_symlink: false },
        ];
        let opts: LocateOptions = LocateOptions.defaults("/pkg");
        let indices: [4]usize = [0; 4];

        let count: usize = locateAllPathsInEntries(candidates[0..4], entries[0..3], opts, indices[0..4]);
        expect_eq(count, 2);
        expect_eq(indices[0], 0);
        expect_eq(indices[1], 3);
    });
}
