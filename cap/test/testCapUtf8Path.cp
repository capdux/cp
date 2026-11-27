// cap/test/testCapUtf8Path.cp
// Comprehensive unit tests for @cap/camino (UTF-8 path manipulation)

import { test, expect_eq } from "cap:test";
import { Utf8PathBuf } from "../../packages/camino/src/index.cp";

fn main() {
    test("1. @cap/camino — Utf8PathBuf fromStr, push, pop & parent", () => {
        let p = Utf8PathBuf.fromStr("/var/log");
        expect_eq(p.isAbsolute(), true);
        expect_eq(p.isRelative(), false);

        p.push("nginx/access.log");
        expect_eq(p.equals("/var/log/nginx/access.log"), true);

        const par = p.parent() ?? Utf8PathBuf.fromStr("");
        expect_eq(par.equals("/var/log/nginx"), true);

        expect_eq(p.pop(), true);
        expect_eq(p.equals("/var/log/nginx"), true);
    });

    test("2. @cap/camino — fromBytes UTF-8 validation & absolute replace", () => {
        let bad: [3]u8 = [0x2F, 0xFF, 0x61];
        expect_eq(Utf8PathBuf.fromBytes(bad[0..3]) == null, true);

        let p = Utf8PathBuf.fromStr("rel/path");
        expect_eq(p.isRelative(), true);
        p.push("/etc/hosts");
        expect_eq(p.isAbsolute(), true);
        expect_eq(p.equals("/etc/hosts"), true);
    });
}
