import { test, expect_eq } from "cap:test";
import {
    isRelativeSpecifier,
    isScopedSpecifier,
    isBareSpecifier,
    resolveFrom,
    resolveFromSilent,
    resolveFromWithOptions,
    ResolveOptions,
    RESOLVE_OK,
    RESOLVE_ERR_EMPTY_INPUT,
    RESOLVE_ERR_NOT_FOUND,
    RESOLVE_ERR_BUFFER_TOO_SMALL,
} from "../../packages/resolve-from/src/index.cp";

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
        expect_eq(isBareSpecifier("lodash"), true);
        expect_eq(isBareSpecifier("@cap/semver"), true);
        expect_eq(isBareSpecifier("./local"), false);
        expect_eq(isBareSpecifier("/abs"), false);
    });

    test("7. @cap/resolve-from — resolveFromSilent returns null cleanly on miss", () => {
        let files: [1]string = ["/repo/node_modules/pkg/index.js"];
        let out: [256]u8 = [0; 256];
        const hit = resolveFromSilent("/repo/src", "pkg", files[0..1], out[0..256]);
        expect_eq(hit != null, true);
        expect_eq(out[0..(hit ?? 0)] as string, "/repo/node_modules/pkg/index.js");

        const miss = resolveFromSilent("/repo/src", "missing-pkg", files[0..1], out[0..256]);
        expect_eq(miss == null, true);
    });

    test("8. @cap/resolve-from — resolveFromWithOptions custom extensions, module dirs & error codes", () => {
        let files: [2]string = ["/repo/cap_modules/foo/data.json", "/repo/node_modules/bar/index.ts"];
        let out: [256]u8 = [0; 256];
        let opts = ResolveOptions.defaults();
        opts.include_node_modules = false;

        const r1 = resolveFromWithOptions("/repo/src", "foo/data", files[0..2], opts, out[0..256]);
        expect_eq(r1.found, true);
        expect_eq(r1.err_code, RESOLVE_OK);
        expect_eq(out[0..r1.len] as string, "/repo/cap_modules/foo/data.json");

        // node_modules disabled -> bar not found
        const r2 = resolveFromWithOptions("/repo/src", "bar", files[0..2], opts, out[0..256]);
        expect_eq(r2.found, false);
        expect_eq(r2.err_code, RESOLVE_ERR_NOT_FOUND);

        // Enable node_modules -> bar found, test small buffer error code
        opts.include_node_modules = true;
        let tiny: [4]u8 = [0; 4];
        const r3 = resolveFromWithOptions("/repo/src", "bar", files[0..2], opts, tiny[0..4]);
        expect_eq(r3.found, false);
        expect_eq(r3.err_code, RESOLVE_ERR_BUFFER_TOO_SMALL);

        const r4 = resolveFromWithOptions("", "bar", files[0..2], opts, out[0..256]);
        expect_eq(r4.err_code, RESOLVE_ERR_EMPTY_INPUT);
    });
}
