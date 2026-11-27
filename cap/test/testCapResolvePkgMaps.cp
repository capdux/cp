import { test, expect_eq } from "cap:test";
import {
    ExportRule,
    matchSubpathPattern,
    resolveExportTarget,
} from "../../packages/resolve-pkg-maps/src/index.cp";

fn main() {
    test("1. @cap/resolve-pkg-maps — matchSubpathPattern supports exact and wildcard patterns", () => {
        let star: [64]u8 = [0 as u8; 64];
        expect_eq(matchSubpathPattern(".", ".", star[0..64]), 0 as i64);
        expect_eq(matchSubpathPattern(".", "./foo", star[0..64]), -1 as i64);

        const len1 = matchSubpathPattern("./*", "./utils/math", star[0..64]);
        expect_eq(len1, 10 as i64);
        expect_eq(star[0..usize(len1)] as string, "utils/math");

        const len2 = matchSubpathPattern("./features/*.js", "./features/auth.js", star[0..64]);
        expect_eq(len2, 4 as i64);
        expect_eq(star[0..usize(len2)] as string, "auth");

        expect_eq(matchSubpathPattern("./features/*.js", "./features/auth.ts", star[0..64]), -1 as i64);
    });

    test("2. @cap/resolve-pkg-maps — resolves exact root export by condition priority", () => {
        const rules: [3]ExportRule = [
            ExportRule{ pattern: ".", condition: "require", target: "./dist/index.cjs" },
            ExportRule{ pattern: ".", condition: "import", target: "./dist/index.mjs" },
            ExportRule{ pattern: ".", condition: "default", target: "./dist/fallback.js" },
        ];
        const conds_esm: [2]string = ["import", "default"];
        const conds_cjs: [2]string = ["require", "default"];
        let out: [128]u8 = [0 as u8; 128];

        const n1 = resolveExportTarget(rules[0..3], ".", conds_esm[0..2], out[0..128]);
        expect_eq(out[0..n1] as string, "./dist/index.mjs");

        const n2 = resolveExportTarget(rules[0..3], ".", conds_cjs[0..2], out[0..128]);
        expect_eq(out[0..n2] as string, "./dist/index.cjs");
    });

    test("3. @cap/resolve-pkg-maps — substitutes wildcard capture into target", () => {
        const rules: [2]ExportRule = [
            ExportRule{ pattern: "./*", condition: "import", target: "./dist/*.mjs" },
            ExportRule{ pattern: "./*", condition: "require", target: "./dist/*.cjs" },
        ];
        const conds: [1]string = ["import"];
        let out: [128]u8 = [0 as u8; 128];

        const n = resolveExportTarget(rules[0..2], "./net/http", conds[0..1], out[0..128]);
        expect_eq(out[0..n] as string, "./dist/net/http.mjs");
    });

    test("4. @cap/resolve-pkg-maps — prefers exact match and longest wildcard prefix", () => {
        const rules: [3]ExportRule = [
            ExportRule{ pattern: "./*", condition: "default", target: "./generic/*.js" },
            ExportRule{ pattern: "./internal/*", condition: "default", target: "./private/*.js" },
            ExportRule{ pattern: "./internal/core", condition: "default", target: "./exact/core.js" },
        ];
        const conds: [1]string = ["default"];
        let out: [128]u8 = [0 as u8; 128];

        const n1 = resolveExportTarget(rules[0..3], "./internal/core", conds[0..1], out[0..128]);
        expect_eq(out[0..n1] as string, "./exact/core.js");

        const n2 = resolveExportTarget(rules[0..3], "./internal/util", conds[0..1], out[0..128]);
        expect_eq(out[0..n2] as string, "./private/util.js");
    });

    test("5. @cap/resolve-pkg-maps — rejects parent directory traversal (..)", () => {
        const rules: [1]ExportRule = [
            ExportRule{ pattern: "./*", condition: "default", target: "./dist/*.js" },
        ];
        const conds: [1]string = ["default"];
        let out: [128]u8 = [0 as u8; 128];

        expect_eq(resolveExportTarget(rules[0..1], "./../secret", conds[0..1], out[0..128]), 0 as usize);
        expect_eq(resolveExportTarget(rules[0..1], "./a/../../etc/passwd", conds[0..1], out[0..128]), 0 as usize);
    });

    test("6. @cap/resolve-pkg-maps — returns 0 when no rule or condition matches", () => {
        const rules: [1]ExportRule = [
            ExportRule{ pattern: "./only-node", condition: "node", target: "./node.js" },
        ];
        const conds: [1]string = ["browser"];
        let out: [128]u8 = [0 as u8; 128];

        expect_eq(resolveExportTarget(rules[0..1], "./only-node", conds[0..1], out[0..128]), 0 as usize);
        expect_eq(resolveExportTarget(rules[0..1], "./missing", conds[0..1], out[0..128]), 0 as usize);
    });
}
