import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    PkgFileEntry,
    ReadPkgUpOptions,
    ReadPkgUpResult,
    parentDir,
    joinPath,
    extractPkgStringField,
    parseQuickPkgMeta,
    readPackageUpNamed,
    readPackageUpInEntries,
    readPackageUpWithOptions,
} from "../../packages/read-pkg-up/src/index.cp";

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
        let path_buf: [256]u8 = undefined;
        const res: ReadPkgUpResult = readPackageUpInEntries(
            "/repo/packages/app/src/utils",
            "",
            true,
            entries[0..2],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(strEq(path_buf[0..res.pkg_path_len] as string, "/repo/packages/app/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(strEq(res.pkg.meta.name, "@cap/app"), true);
        expect_eq(strEq(res.pkg.meta.version, "2.3.4"), true);
        expect_eq(strEq(res.pkg.meta.main_entry, "src/index.cp"), true);
        expect_eq(res.pkg.normalized.valid, true);
    });

    test("2. @cap/read-pkg-up — finds package.json directly in cwd with trailing slash", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/workspace/demo/package.json",
                content: "{\"name\":\"demo\",\"version\":\"1.0.0\",\"license\":\"Apache-2.0\"}",
            },
        ];
        let path_buf: [256]u8 = undefined;
        const res: ReadPkgUpResult = readPackageUpInEntries(
            "/workspace/demo/",
            "",
            true,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(strEq(path_buf[0..res.pkg_path_len] as string, "/workspace/demo/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(strEq(res.pkg.meta.name, "demo"), true);
    });

    test("3. @cap/read-pkg-up — stop_at halts upward lookup before higher ancestor", () => {
        let entries: [1]PkgFileEntry = [
            PkgFileEntry {
                path: "/repo/package.json",
                content: "{\"name\":\"root-pkg\",\"version\":\"1.0.0\",\"license\":\"MIT\"}",
            },
        ];
        let path_buf: [256]u8 = undefined;
        const res: ReadPkgUpResult = readPackageUpInEntries(
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
        let path_buf: [256]u8 = undefined;
        const res: ReadPkgUpResult = readPackageUpInEntries(
            "/repo/packages/sub/src",
            "/repo/packages",
            false,
            entries[0..1],
            path_buf[0..256],
        );
        expect_eq(res.found, true);
        expect_eq(strEq(path_buf[0..res.pkg_path_len] as string, "/repo/packages/package.json"), true);
        expect_eq(res.pkg.parsed, true);
        expect_eq(strEq(res.pkg.meta.version, "v0.5.0"), true);
        expect_eq(strEq(res.pkg.meta.main_entry, "./lib/main.cp"), true);
        expect_eq(res.pkg.normalized.valid, false);
    });

    test("5. @cap/read-pkg-up — handles malformed package.json and root/empty cwd", () => {
        let entries: [2]PkgFileEntry = [
            PkgFileEntry {
                path: "/broken/package.json",
                content: "{\"name\":\"unclosed\"",
            },
            PkgFileEntry {
                path: "/package.json",
                content: "{\"name\":\"root-only\",\"version\":\"1.0.0\",\"license\":\"MIT\"}",
            },
        ];
        let path_buf: [256]u8 = undefined;
        const res_bad = readPackageUpInEntries("/broken/deep/dir", "", true, entries[0..2], path_buf[0..256]);
        expect_eq(res_bad.found, true);
        expect_eq(res_bad.pkg.parsed, false);

        const res_root = readPackageUpInEntries("/a/b", "", true, entries[0..2], path_buf[0..256]);
        expect_eq(res_root.found, true);
        expect_eq(strEq(path_buf[0..res_root.pkg_path_len] as string, "/package.json"), true);

        const res_empty = readPackageUpInEntries("", "", true, entries[0..2], path_buf[0..256]);
        expect_eq(res_empty.found, false);
    });

    test("6. @cap/read-pkg-up — cap.json lookup via readPackageUpNamed & readPackageUpWithOptions", () => {
        let entries: [2]PkgFileEntry = [
            PkgFileEntry {
                path: "/proj/package.json",
                content: "{\"name\":\"node-pkg\",\"version\":\"1.0.0\"}",
            },
            PkgFileEntry {
                path: "/proj/cap.json",
                content: "{\"name\":\"cp-pkg\",\"version\":\"2.0.0\",\"description\":\"CP native pkg\"}",
            },
        ];
        let path_buf: [256]u8 = undefined;

        const r1 = readPackageUpNamed("/proj/src", "", "cap.json", true, entries[0..2], path_buf[0..256]);
        expect_eq(r1.found, true);
        expect_eq(strEq(path_buf[0..r1.pkg_path_len] as string, "/proj/cap.json"), true);
        expect_eq(strEq(r1.pkg.meta.name, "cp-pkg"), true);

        let opts = ReadPkgUpOptions.defaults("/proj/src");
        opts.prefer_cap_json = true;
        const r2 = readPackageUpWithOptions(opts, entries[0..2], path_buf[0..256]);
        expect_eq(r2.found, true);
        expect_eq(strEq(path_buf[0..r2.pkg_path_len] as string, "/proj/cap.json"), true);
    });

    test("7. @cap/read-pkg-up — parentDir & joinPath pure CP path helpers", () => {
        expect_eq(strEq(parentDir("/a/b/c").?, "/a/b"), true);
        expect_eq(strEq(parentDir("/a/b/").?, "/a"), true);
        expect_eq(strEq(parentDir("/a").?, "/"), true);
        expect_eq(parentDir("/") == null, true);
        expect_eq(parentDir("") == null, true);

        let out: [64]u8 = undefined;
        const n1 = joinPath("/repo/app/", "cap.json", out[0..64]);
        expect_eq(strEq(out[0..n1] as string, "/repo/app/cap.json"), true);
        const n2 = joinPath("/", "package.json", out[0..64]);
        expect_eq(strEq(out[0..n2] as string, "/package.json"), true);
    });

    test("8. @cap/read-pkg-up — extractPkgStringField & parseQuickPkgMeta (ignoring nested keys)", () => {
        const json = "{\"author\":{\"name\":\"Nested Author\"},\"name\":\"@cap/demo\",\"ver\":\"1.2.3\",\"description\":\"Fast CP pkg\"}";
        expect_eq(strEq(extractPkgStringField(json, "name").?, "@cap/demo"), true);
        expect_eq(strEq(extractPkgStringField(json, "description").?, "Fast CP pkg"), true);
        expect_eq(extractPkgStringField(json, "missing") == null, true);

        const meta = parseQuickPkgMeta(json);
        expect_eq(meta.has_name, true);
        expect_eq(meta.has_version, true);
        expect_eq(meta.has_description, true);
        expect_eq(strEq(meta.name, "@cap/demo"), true);
        expect_eq(strEq(meta.version, "1.2.3"), true);
        expect_eq(strEq(meta.description, "Fast CP pkg"), true);
    });
}
