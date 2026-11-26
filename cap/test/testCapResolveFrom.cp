import { test, expect_eq } from "cap:test";
import {
    isRelativeSpecifier, isScopedSpecifier, resolveFrom
} from "../../packages/@cap/resolve-from/src/index.cp";

fn main() {
    test("1. @cap/resolve-from — resolves relative ./ and ../ modules with extension probing", () => {
        let files: [2]string = ["/app/src/util.cp", "/app/shared/config.js"];
        let out: [256]u8 = [0; 256];
        let n1 = resolveFrom("/app/src", "./util", files[0..2], out[0..256]) ?? 0;
        expect_eq(out[0..n1] as string, "/app/src/util.cp");

        let n2 = resolveFrom("/app/src", "../shared/config", files[0..2], out[0..256]) ?? 0;
        expect_eq(out[0..n2] as string, "/app/shared/config.js");
    });

    test("2. @cap/resolve-from — resolves directory /index.cp and /index.js", () => {
        let files: [1]string = ["/app/lib/index.cp"];
        let out: [256]u8 = [0; 256];
        let n = resolveFrom("/app", "./lib", files[0..1], out[0..256]) ?? 0;
        expect_eq(out[0..n] as string, "/app/lib/index.cp");
    });

    test("3. @cap/resolve-from — walks upward through ancestor node_modules", () => {
        let files: [1]string = ["/repo/node_modules/chalk/index.js"];
        let out: [256]u8 = [0; 256];
        let n = resolveFrom("/repo/packages/cli/src", "chalk", files[0..1], out[0..256]) ?? 0;
        expect_eq(out[0..n] as string, "/repo/node_modules/chalk/index.js");
    });

    test("4. @cap/resolve-from — resolves scoped @cap/pkg from cap_modules", () => {
        let files: [1]string = ["/proj/cap_modules/@cap/semver/index.cp"];
        let out: [256]u8 = [0; 256];
        expect_eq(isScopedSpecifier("@cap/semver"), true);
        let n = resolveFrom("/proj/src", "@cap/semver", files[0..1], out[0..256]) ?? 0;
        expect_eq(out[0..n] as string, "/proj/cap_modules/@cap/semver/index.cp");
    });

    test("5. @cap/resolve-from — resolves absolute paths & returns null on missing module", () => {
        let files: [1]string = ["/opt/lib/mod.ts"];
        let out: [256]u8 = [0; 256];
        let n = resolveFrom("/any", "/opt/lib/mod", files[0..1], out[0..256]) ?? 0;
        expect_eq(out[0..n] as string, "/opt/lib/mod.ts");
        expect_eq(resolveFrom("/any", "nonexistent", files[0..1], out[0..256]) == null, true);
    });

    test("6. @cap/resolve-from — specifier classification helpers", () => {
        expect_eq(isRelativeSpecifier("./a"), true);
        expect_eq(isRelativeSpecifier("../b"), true);
        expect_eq(isRelativeSpecifier("lodash"), false);
        expect_eq(isScopedSpecifier("lodash"), false);
    });
}
