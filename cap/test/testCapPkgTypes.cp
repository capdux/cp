import { test, expect_eq } from "cap:test";
import {
    PkgManifest,
    PkgNameParts,
    parsePkgManifest,
    isValidPkgName,
    splitPkgScope,
} from "../../packages/@cap/pkg-types/src/index.cp";

fn main() {
    test("1. @cap/pkg-types — parsePkgManifest extracts top-level package.json fields", () => {
        const json = "{\"name\": \"@cap/pkg-types\", \"version\": \"1.3.1\", \"license\": \"MIT\", \"main\": \"dist/index.cjs\", \"module\": \"dist/index.mjs\", \"private\": true}";
        const m = parsePkgManifest(json);
        expect_eq(m.valid, true);
        expect_eq(m.name, "@cap/pkg-types");
        expect_eq(m.version, "1.3.1");
        expect_eq(m.license, "MIT");
        expect_eq(m.main, "dist/index.cjs");
        expect_eq(m.module, "dist/index.mjs");
        expect_eq(m.is_private, true);
    });

    test("2. @cap/pkg-types — parsePkgManifest extracts cap.json entry and skips nested objects", () => {
        const json = "{\n  \"name\": \"@cap/demo\",\n  \"version\": \"2.0.0\",\n  \"entry\": \"src/index.cp\",\n  \"source\": { \"name\": \"nested\", \"version\": \"9.9.9\" },\n  \"keywords\": [\"a\", \"b\"],\n  \"private\": false\n}";
        const m = parsePkgManifest(json);
        expect_eq(m.valid, true);
        expect_eq(m.name, "@cap/demo");
        expect_eq(m.version, "2.0.0");
        expect_eq(m.entry, "src/index.cp");
        expect_eq(m.is_private, false);
    });

    test("3. @cap/pkg-types — parsePkgManifest rejects malformed JSON", () => {
        const bad1 = parsePkgManifest("not json");
        expect_eq(bad1.valid, false);

        const bad2 = parsePkgManifest("{\"name\": \"unclosed\"");
        expect_eq(bad2.valid, false);

        const bad3 = parsePkgManifest("{\"name\": \"ok\"} trailing");
        expect_eq(bad3.valid, false);
    });

    test("4. @cap/pkg-types — isValidPkgName accepts valid scoped and unscoped names", () => {
        expect_eq(isValidPkgName("lodash"), true);
        expect_eq(isValidPkgName("pkg-types"), true);
        expect_eq(isValidPkgName("a.b_c~1"), true);
        expect_eq(isValidPkgName("@cap/pkg-types"), true);
        expect_eq(isValidPkgName("@unjs/pkg-types"), true);
    });

    test("5. @cap/pkg-types — isValidPkgName rejects invalid names", () => {
        expect_eq(isValidPkgName(""), false);
        expect_eq(isValidPkgName("UpperCase"), false);
        expect_eq(isValidPkgName(".leading-dot"), false);
        expect_eq(isValidPkgName("_leading-under"), false);
        expect_eq(isValidPkgName("has space"), false);
        expect_eq(isValidPkgName("@scope-only"), false);
        expect_eq(isValidPkgName("@scope/a/b"), false);
        expect_eq(isValidPkgName("@/pkg"), false);
        expect_eq(isValidPkgName("@Scope/pkg"), false);
    });

    test("6. @cap/pkg-types — splitPkgScope splits scoped and unscoped names", () => {
        const scoped = splitPkgScope("@cap/pkg-types");
        expect_eq(scoped.has_scope, true);
        expect_eq(scoped.scope, "@cap");
        expect_eq(scoped.pkg_name, "pkg-types");

        const plain = splitPkgScope("express");
        expect_eq(plain.has_scope, false);
        expect_eq(plain.scope, "");
        expect_eq(plain.pkg_name, "express");
    });
}
