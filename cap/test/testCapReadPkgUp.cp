import { test, expect_eq } from "cap:test";
import {
    PkgFileEntry,
    ReadPkgUpResult,
    readPackageUpInEntries,
} from "../../packages/@cap/read-pkg-up/src/index.cp";

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
    test("1. @cap/read-pkg-up — finds closest ancestor package.json and normalizes metadata", () => {
        let entries: [2]PkgFileEntry = [
            PkgFileEntry {
                path: "/repo/package.json",
                content: "{\"name\":\"root-pkg\",\"version\":\"1.0.0\",\"license\":\"MIT\"}",
            },
            PkgFileEntry {
                path: "/repo/packages/app/package.json",
                content: "{\"name\":\"@cap/app\",\"version\":\"  v2.3.4 \",\"license\":\"MIT\",\"main\":\"./src/index.cp\",\"private\":false}",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res: ReadPkgUpResult = readPackageUpInEntries(
            "/repo/packages/app/src/utils",
            "",
            true,
            entries[0..2],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(eqStr(path_buf[0..res.pkg_path_len] as string, "/repo/packages/app/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(eqStr(res.pkg.meta.name, "@cap/app"), true);
        expect_eq(eqStr(res.pkg.meta.version, "2.3.4"), true);
        expect_eq(eqStr(res.pkg.meta.main_entry, "src/index.cp"), true);
        expect_eq(res.pkg.normalized.valid, true);
    });

    test("2. @cap/read-pkg-up — finds package.json directly in cwd with trailing slash", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/workspace/demo/package.json",
                content: "{\"name\":\"demo\",\"version\":\"1.0.0\",\"license\":\"Apache-2.0\"}",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res: ReadPkgUpResult = readPackageUpInEntries(
            "/workspace/demo/",
            "",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(eqStr(path_buf[0..res.pkg_path_len] as string, "/workspace/demo/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(eqStr(res.pkg.meta.name, "demo"), true);
    });

    test("3. @cap/read-pkg-up — stop_at halts upward lookup before higher ancestor", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/repo/package.json",
                content: "{\"name\":\"root-pkg\",\"version\":\"1.0.0\",\"license\":\"MIT\"}",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res: ReadPkgUpResult = readPackageUpInEntries(
            "/repo/packages/sub/src",
            "/repo/packages",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, false);
        expect_eq(res.pkg_path_len, 0);
        expect_eq(res.pkg.parsed, false);
    });

    test("4. @cap/read-pkg-up — stop_at is inclusive of the boundary directory", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/repo/packages/package.json",
                content: "{\"name\":\"pkgs-scope\",\"version\":\"v0.5.0\",\"main\":\"./lib/main.cp\"}",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res: ReadPkgUpResult = readPackageUpInEntries(
            "/repo/packages/sub/src",
            "/repo/packages",
            false,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(eqStr(path_buf[0..res.pkg_path_len] as string, "/repo/packages/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(eqStr(res.pkg.meta.version, "v0.5.0"), true);
        expect_eq(eqStr(res.pkg.meta.main_entry, "./lib/main.cp"), true);
        expect_eq(res.pkg.normalized.valid, false);
    });

    test("5. @cap/read-pkg-up — handles malformed package.json at matched path", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/broken/package.json",
                content: "{\"name\":\"unclosed\"",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res: ReadPkgUpResult = readPackageUpInEntries(
            "/broken/deep/dir",
            "",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(eqStr(path_buf[0..res.pkg_path_len] as string, "/broken/package.json"), true);
        expect_eq(res.pkg.parsed, false);
    });

    test("6. @cap/read-pkg-up — handles root package.json and empty cwd safely", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/package.json",
                content: "{\"name\":\"root-only\",\"version\":\"1.0.0\",\"license\":\"MIT\"}",
            },
        ];
        let path_buf: [256]u8 = [0; 256];
        let res_root: ReadPkgUpResult = readPackageUpInEntries(
            "/a/b",
            "",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res_root.found, true);
        expect_eq(eqStr(path_buf[0..res_root.pkg_path_len] as string, "/package.json"), true);
        expect_eq(res_root.pkg.parsed, true);
        expect_eq(eqStr(res_root.pkg.meta.name, "root-only"), true);

        let res_empty: ReadPkgUpResult = readPackageUpInEntries(
            "",
            "",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res_empty.found, false);
        expect_eq(res_empty.pkg_path_len, 0);
    });
}
