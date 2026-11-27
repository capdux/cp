import { test, expect_eq } from "cap:test";
import {
    RunPathOptions, buildRunPath, dirnameOf, countAncestorDirs, pathKey
} from "../../packages/npm-run-path/src/index.cp";

fn main() {
    test("1. @cap/npm-run-path — POSIX ancestor walk prepends node_modules/.bin", () => {
        let opts = RunPathOptions.defaults("/a/b", "/usr/bin");
        let buf: [256]u8 = [0; 256];
        let n = buildRunPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/a/b/node_modules/.bin:/a/node_modules/.bin:/node_modules/.bin:/usr/bin");
    });

    test("2. @cap/npm-run-path — forCap uses cap_modules/.bin", () => {
        let opts = RunPathOptions.forCap("/app", "/bin");
        let buf: [256]u8 = [0; 256];
        let n = buildRunPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/app/cap_modules/.bin:/cap_modules/.bin:/bin");
    });

    test("3. @cap/npm-run-path — add_exec_path inserts runtime binary directory", () => {
        let opts = RunPathOptions.defaults("/proj", "/usr/bin");
        opts.add_exec_path = true;
        opts.exec_path = "/opt/cap/bin/cap";
        let buf: [256]u8 = [0; 256];
        let n = buildRunPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/proj/node_modules/.bin:/node_modules/.bin:/opt/cap/bin:/usr/bin");
    });

    test("4. @cap/npm-run-path — prefer_local=false preserves PATH only", () => {
        let opts = RunPathOptions.defaults("/proj/sub", "/usr/local/bin:/usr/bin");
        opts.prefer_local = false;
        let buf: [256]u8 = [0; 256];
        let n = buildRunPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "/usr/local/bin:/usr/bin");
    });

    test("5. @cap/npm-run-path — Windows delimiter (;) and backslash normalization", () => {
        let opts = RunPathOptions.defaults("C:\\app\\pkg", "C:\\Windows");
        opts.is_windows = true;
        let buf: [256]u8 = [0; 256];
        let n = buildRunPath(opts, buf[0..256]);
        expect_eq(buf[0..n] as string, "C:\\app\\pkg\\node_modules\\.bin;C:\\app\\node_modules\\.bin;C:\\node_modules\\.bin;C:\\Windows");
        expect_eq(pathKey(true), "Path");
        expect_eq(pathKey(false), "PATH");
    });

    test("6. @cap/npm-run-path — dirnameOf & countAncestorDirs", () => {
        expect_eq(dirnameOf("/a/b/c", false), "/a/b");
        expect_eq(dirnameOf("/a", false), "/");
        expect_eq(dirnameOf("/", false), "/");
        expect_eq(countAncestorDirs("/a/b/c/", false), 4);
    });
}
