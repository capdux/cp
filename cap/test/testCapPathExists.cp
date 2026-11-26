import { test, expect_eq } from "cap:test";
import { ENTRY_FILE, ENTRY_DIR, FsEntry } from "../../packages/@cap/locate-path/src/index.cp";
import {
    pathExistsInEntries, fileExistsInEntries, dirExistsInEntries,
    realPathExistsInEntries, countExistingPaths
} from "../../packages/@cap/path-exists/src/index.cp";

fn main() {
    test("1. @cap/path-exists — pathExistsInEntries matches relative & absolute entries", () => {
        let entries: [2]FsEntry = [
            FsEntry { path: "/app/cap.json", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/app/src", kind: ENTRY_DIR, is_symlink: false },
        ];
        expect_eq(pathExistsInEntries("cap.json", "/app", entries[0..2]), true);
        expect_eq(pathExistsInEntries("/app/src", "/other", entries[0..2]), true);
        expect_eq(pathExistsInEntries("missing.txt", "/app", entries[0..2]), false);
    });

    test("2. @cap/path-exists — fileExistsInEntries rejects directories", () => {
        let entries: [2]FsEntry = [
            FsEntry { path: "/app/cap.json", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/app/src", kind: ENTRY_DIR, is_symlink: false },
        ];
        expect_eq(fileExistsInEntries("cap.json", "/app", entries[0..2]), true);
        expect_eq(fileExistsInEntries("src", "/app", entries[0..2]), false);
    });

    test("3. @cap/path-exists — dirExistsInEntries rejects regular files", () => {
        let entries: [2]FsEntry = [
            FsEntry { path: "/app/cap.json", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/app/src", kind: ENTRY_DIR, is_symlink: false },
        ];
        expect_eq(dirExistsInEntries("src", "/app", entries[0..2]), true);
        expect_eq(dirExistsInEntries("cap.json", "/app", entries[0..2]), false);
    });

    test("4. @cap/path-exists — realPathExistsInEntries excludes symlinks", () => {
        let entries: [2]FsEntry = [
            FsEntry { path: "/app/real.cp", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/app/link.cp", kind: ENTRY_FILE, is_symlink: true },
        ];
        expect_eq(pathExistsInEntries("link.cp", "/app", entries[0..2]), true);
        expect_eq(realPathExistsInEntries("link.cp", "/app", entries[0..2]), false);
        expect_eq(realPathExistsInEntries("real.cp", "/app", entries[0..2]), true);
    });

    test("5. @cap/path-exists — countExistingPaths returns matching indices in order", () => {
        let entries: [2]FsEntry = [
            FsEntry { path: "/p/a", kind: ENTRY_FILE, is_symlink: false },
            FsEntry { path: "/p/c", kind: ENTRY_DIR, is_symlink: false },
        ];
        let targets: [3]string = ["a", "b", "c"];
        let idxs: [4]usize = [0; 4];
        let cnt = countExistingPaths(targets[0..3], "/p", entries[0..2], idxs[0..4]);
        expect_eq(cnt, 2);
        expect_eq(idxs[0], 0);
        expect_eq(idxs[1], 2);
    });

    test("6. @cap/path-exists — empty entries returns false", () => {
        let entries: [1]FsEntry = [FsEntry { path: "/x", kind: ENTRY_FILE, is_symlink: false }];
        expect_eq(pathExistsInEntries("x", "/", entries[0..0]), false);
    });
}
