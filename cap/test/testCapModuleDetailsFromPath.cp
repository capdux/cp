import { test, expect_eq } from "cap:test";
import {
    ModuleDetails,
    parseModulePath,
} from "../../packages/module-details-from-path/src/index.cp";

fn main() {
    test("1. @cap/module-details-from-path — parses standard node_modules POSIX path", () => {
        const d = parseModulePath("/usr/app/node_modules/lodash/fp/map.js");
        expect_eq(d.found, true);
        expect_eq(d.name, "lodash");
        expect_eq(d.basedir, "/usr/app/node_modules/lodash");
        expect_eq(d.subpath, "fp/map.js");
    });

    test("2. @cap/module-details-from-path — parses scoped @scope/name in cap_modules", () => {
        const d = parseModulePath("/data/dev/cp/cap_modules/@cap/http/src/server.cp");
        expect_eq(d.found, true);
        expect_eq(d.name, "@cap/http");
        expect_eq(d.basedir, "/data/dev/cp/cap_modules/@cap/http");
        expect_eq(d.subpath, "src/server.cp");
    });

    test("3. @cap/module-details-from-path — selects innermost nested node_modules", () => {
        const d = parseModulePath("/app/node_modules/express/node_modules/qs/lib/parse.js");
        expect_eq(d.found, true);
        expect_eq(d.name, "qs");
        expect_eq(d.basedir, "/app/node_modules/express/node_modules/qs");
        expect_eq(d.subpath, "lib/parse.js");
    });

    test("4. @cap/module-details-from-path — supports Windows backslash paths", () => {
        const d = parseModulePath("C:\\Users\\dev\\proj\\node_modules\\@types\\node\\fs.d.ts");
        expect_eq(d.found, true);
        expect_eq(d.name, "@types\\node");
        expect_eq(d.basedir, "C:\\Users\\dev\\proj\\node_modules\\@types\\node");
        expect_eq(d.subpath, "fs.d.ts");
    });

    test("5. @cap/module-details-from-path — handles package root without subpath", () => {
        const d = parseModulePath("cap_modules/zod");
        expect_eq(d.found, true);
        expect_eq(d.name, "zod");
        expect_eq(d.basedir, "cap_modules/zod");
        expect_eq(d.subpath, "");
    });

    test("6. @cap/module-details-from-path — returns found=false for non-module or malformed scoped paths", () => {
        const d1 = parseModulePath("/usr/src/my_node_modules_backup/foo.js");
        expect_eq(d1.found, false);
        expect_eq(d1.name, "");

        const d2 = parseModulePath("/app/node_modules/@incomplete");
        expect_eq(d2.found, false);
        expect_eq(d2.basedir, "");
    });
}
